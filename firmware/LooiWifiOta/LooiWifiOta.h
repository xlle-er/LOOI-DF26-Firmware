#pragma once
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <esp_crt_bundle.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <mbedtls/sha256.h>
#include "DevicePolicy.h"
#include "HardwareConfig.h"
#include "DevicePanel.h"

constexpr char OTA_PASSWORD[] = "12345678";
constexpr char DEVICE_VERSION[] = "2.1.0";
constexpr char RELEASE_MANIFEST[] = "https://github.com/xlle-er/LOOI-DF26-Firmware/releases/latest/download/looi-s3-manifest.json";

class LooiWifiOta {
 public:
  HardwareConfig hardware;
  using Dispatcher = String (*)(const String &, const String &);
  void begin(void (*stop)(), Dispatcher dispatcher, void (*test)(float,float)) {
    stop_ = stop; dispatch_ = dispatcher; test_ = test;
    pinMode(0, INPUT_PULLUP);
    char id[18]; snprintf(id, sizeof(id), "looi-%012llx", ESP.getEfuseMac()); id_ = id;
    apName_ = "LOOI-Setup-" + id_.substring(id_.length()-6);
    char token[33]; snprintf(token,sizeof(token),"%08lx%08lx%08lx%08lx",(unsigned long)esp_random(),(unsigned long)esp_random(),(unsigned long)esp_random(),(unsigned long)esp_random()); token_ = token;
    prefs_.begin("looi-device", false);
    if (prefs_.getBytesLength("settings") == sizeof(settings_)) {
      Settings loaded; prefs_.getBytes("settings", &loaded, sizeof(loaded));
      if (loaded.version == 2 && loaded.ssid[32] == 0 && loaded.password[64] == 0) settings_ = loaded;
    }
    const auto running=esp_ota_get_running_partition();
    const uint32_t marker=prefs_.getUInt("ota_ap_once",0);
    autoApSuppressed_=running && suppressHotspotAfterOta(marker,running->address);
    if(marker) prefs_.remove("ota_ap_once");
    // WiFi.mode initializes lwIP; opening a socket before this can assert on a null tcpip mutex.
    WiFi.persistent(false);
    if(!WiFi.mode(WIFI_STA)) { error_="Wi-Fi network stack initialization failed"; Serial.printf("[NET] %s\n",error_.c_str()); return; }
    WiFi.setHostname(id_.c_str());
    Serial.println("[NET] Wi-Fi network stack initialized");
    configureRoutes(); server_.begin();
    Serial.println("[HTTP] Management server started");
    if (settings_.ssid[0]) connect(settings_.ssid, settings_.password, false);
    else if(!autoApSuppressed_) openAp();
    Serial.printf("[DEVICE] %s version=%s hold BOOT 3s for setup\n",id_.c_str(),DEVICE_VERSION);
  }
  void bleDisconnected() { if(authority_.owner=="ble") revoke(); }
  MotorMapping mapping() const { return settings_.mapping; }
  bool active() const { return ap_; }
  bool locked() const { return otaMode_ || reboot_; }
  bool uploading() const { return writing_; }
  const String &ssid() const { return apName_; }
  String url() const { return "http://" + (WiFi.status()==WL_CONNECTED ? WiFi.localIP() : WiFi.softAPIP()).toString() + "/"; }
  const String &error() const { return error_; }
  void revoke() { authority_.revoke(); stop_(); }
  bool allowMotion(const String &source, uint32_t controlToken = 0) {
    if (locked() || !hardware.ready) return false;
    if (authority_.expired(millis())) revoke();
    return authority_.allow(source.c_str(), controlToken, millis());
  }
  void loop() {
    uint32_t now = millis();
    if (digitalRead(0) == HIGH) { pressed_=false; handled_=false; }
    else if (!pressed_) { pressed_=true; pressedAt_=now; }
    else if (!handled_ && now-pressedAt_>=3000) { handled_=true; revoke(); openAp(); }
    if (authority_.expired(now)) revoke();
    if (locked()) stop_();
    if (reboot_ && int32_t(now-rebootAt_)>=0) { ESP.restart(); return; }
    if (WiFi.status() == WL_CONNECTED) {
      offlineAt_=0;
      if (!mdns_) { mdns_=MDNS.begin(id_.c_str()); if(mdns_) { MDNS.addService("looi","tcp",80); MDNS.addServiceTxt("looi","tcp","id",id_); } }
      if (connecting_) {
        connecting_=false;
        if (candidate_) {
          Settings next=settings_; pendingSsid_.toCharArray(next.ssid,sizeof(next.ssid)); pendingPassword_.toCharArray(next.password,sizeof(next.password));
          if (!persist(next)) error_="Network connected, but saving credentials failed";
        }
        candidate_=false; pendingPassword_=""; connectedAt_=now; networkState_="connected";
      }
      if(ap_ && connectedAt_ && now-connectedAt_>=60000 && !writing_ && !otaMode_) { WiFi.softAPdisconnect(true); WiFi.mode(WIFI_STA); ap_=false; }
    } else {
      if(mdns_) { MDNS.end(); mdns_=false; }
      if(!offlineAt_) offlineAt_=now;
      if(connecting_ && now-connectAt_>=30000) {
        connecting_=false; networkState_="failed"; error_="Wi-Fi connection failed; check name/password and 2.4 GHz support";
        pendingPassword_=""; candidate_=false; WiFi.disconnect(false,false); if(!autoApSuppressed_) openAp();
      } else if(!autoApSuppressed_ && !connecting_ && now-offlineAt_>=15000 && !ap_) { revoke(); openAp(); if(settings_.ssid[0]) connect(settings_.ssid,settings_.password,false); }
    }
    server_.handleClient();
    if(writing_ && millis()-lastWrite_>15000) { fail("Upload timed out"); uploadRequest_=false; }
  }
  bool handle(JsonObjectConst root, JsonDocument &reply, const String &source) {
    const String type=root["type"] | "";
    const bool known=type=="hardware_get"||type=="hardware_set"||type=="device_status"||type=="wifi_scan"||type=="wifi_configure"||type=="wifi_forget"||type=="calibration_get"||type=="calibration_set"||type=="calibration_test"||type=="control_acquire"||type=="control_renew"||type=="control_release"||type=="ota_start"||type=="ota_exit"||type=="ota_status"||type=="firmware_check"||type=="firmware_install";
    if(!known) { if(type=="stop") revoke(); return false; }
    reply["id"]=root["id"]; reply["type"]="ack"; reply["cmd"]=type; reply["accepted"]=true;
    auto reject=[&](const String &reason){ reply["type"]="error"; reply["accepted"]=false; reply["message"]=reason; };
    if((source=="ble" || source.startsWith("usb:")) && type!="hardware_get" && type!="device_status" && type!="ota_status" && type!="calibration_get" && type!="control_renew" && type!="control_release" && String(root["admin_password"] | "")!=OTA_PASSWORD) { reject("Device administrator password required"); return true; }
    if(type=="hardware_get" || type=="device_status" || type=="ota_status" || type=="calibration_get") { status(reply); return true; }
    if(locked() && type!="ota_exit" && type!="ota_start") { reject("Controls locked during update"); return true; }
    if(type=="hardware_set") {
      revoke();
      if(!hardware.save(root["hardware_profile"].as<JsonObjectConst>())) reject(hardware.error);
      else { hardware.ready=false; reply["restarting"]=true; reboot_=true; rebootAt_=millis()+1500; }
    }
    else if(type=="control_acquire") {
      if(!hardware.ready) { reject(hardware.error); return true; }
      if(!authority_.acquire(source.c_str(),root["takeover"]==true,millis())) { reject("Another controller is active; explicitly take control"); return true; }
      stop_(); reply["control_token"]=authority_.token;
    }
    else if(type=="control_renew") { if(!allowMotion(source, root["control_token"] | 0u)) reject("Control ownership lost"); }
    else if(type=="control_release") { if(authority_.owner==source.c_str()) revoke(); }
    else if(type=="wifi_scan") {
      int count=WiFi.scanComplete();
      if(count==WIFI_SCAN_RUNNING) reply["scanning"]=true;
      else if(count>=0) { auto networks=reply.createNestedArray("networks"); for(int i=0;i<count && i<20;i++) { auto n=networks.createNestedObject(); n["ssid"]=WiFi.SSID(i); n["rssi"]=WiFi.RSSI(i); n["secure"]=WiFi.encryptionType(i)!=WIFI_AUTH_OPEN; } WiFi.scanDelete(); reply["scanning"]=false; }
      else { WiFi.scanNetworks(true); reply["scanning"]=true; }
    } else if(type=="wifi_configure") {
      String ssid=root["ssid"] | "", password=root["password"] | "";
      if(ssid.isEmpty()||ssid.length()>32||password.length()>63||(!password.isEmpty()&&password.length()<8)) reject("SSID must be 1-32 bytes; password must be empty or 8-63 bytes");
      else { revoke(); openAp(); connect(ssid,password,true); reply["network_state"]="connecting"; }
    } else if(type=="wifi_forget") {
      revoke(); Settings next=settings_; next.ssid[0]=0; next.password[0]=0;
      if(!persist(next)) reject("Could not save settings"); else { WiFi.disconnect(false,false); connecting_=false; candidate_=false; pendingPassword_=""; networkState_="unconfigured"; openAp(); }
    } else if(type=="calibration_set") {
      revoke(); Settings next=settings_;
      if(root["reset"]==true) next.mapping=MotorMapping();
      else if(root["shortcut"]=="reverse_forward") next.mapping.reverseForward();
      else if(root["shortcut"]=="reverse_steering") next.mapping.reverseSteering();
      else if(!root["left_invert"].is<bool>()||!root["right_invert"].is<bool>()||!root["swap"].is<bool>()) { reject("Boolean direction fields required"); return true; }
      else { next.mapping.leftInvert=root["left_invert"]; next.mapping.rightInvert=root["right_invert"]; next.mapping.swap=root["swap"]; }
      if(!persist(next)) reject("Could not save calibration"); else status(reply);
    } else if(type=="calibration_test") {
      if(!allowMotion(source, root["control_token"] | 0u)) reject("Acquire control before testing");
      else {
        String direction=root["direction"] | ""; float l=0,r=0;
        if(direction=="forward") l=r=0.25f;
        else if(direction=="backward") l=r=-0.25f;
        else if(direction=="left") { l=-0.25f;r=0.25f; }
        else if(direction=="right") { l=0.25f;r=-0.25f; }
        else if(direction=="left_forward") l=0.25f;
        else if(direction=="left_backward") l=-0.25f;
        else if(direction=="right_forward") r=0.25f;
        else if(direction=="right_backward") r=-0.25f;
        else { reject("Unknown test direction"); return true; }
        test_(l,r); reply["duration_ms"]=300;
      }
    } else if(type=="ota_start") {
      if(writing_) { reject("Upload in progress"); return true; }
      if(!root["close_ap_after_update"].isNull()&&!root["close_ap_after_update"].is<bool>()) { reject("Boolean hotspot option required"); return true; }
      closeApAfterUpdate_=root["close_ap_after_update"] | true;
      if(!otaMode_) { revoke(); otaMode_=true; } status(reply);
    }
    else if(type=="ota_exit") { if(writing_) reject("Upload in progress"); else { otaMode_=false; revoke(); } }
    else if(type=="firmware_check") { revoke(); String channel=root["channel"] | "stable"; if(!validChannel(channel)) reject("Unknown update channel"); else if(!fetchManifest(reply,channel)) reject(error_); }
    else if(type=="firmware_install") { closeApAfterUpdate_=root["close_ap_after_update"] | true; revoke(); otaMode_=true; String channel=root["channel"] | "stable"; if(!validChannel(channel)) { otaMode_=false; reject("Unknown update channel"); } else if(!installRelease(root["version"] | "",channel)) { otaMode_=false; reject(error_); } else reply["restarting"]=true; }
    return true;
  }
 private:
  struct Settings { uint32_t version=2; MotorMapping mapping; char ssid[33]={}; char password[65]={}; } settings_;
  Preferences prefs_; WebServer server_{80}; ControlAuthority authority_;
  void (*stop_)()=nullptr; void (*test_)(float,float)=nullptr; Dispatcher dispatch_=nullptr;
  String id_,apName_,token_,error_,pendingSsid_,pendingPassword_,networkState_="unconfigured";
  bool ap_=false,mdns_=false,connecting_=false,candidate_=false,pressed_=false,handled_=false;
  bool otaMode_=false,writing_=false,uploadRequest_=false,complete_=false,reboot_=false;
  uint32_t pressedAt_=0,connectAt_=0,connectedAt_=0,offlineAt_=0,lastWrite_=0,rebootAt_=0;
  CompatibilityScanner compatibility_;
  size_t expected_=0,received_=0; String expectedSha_;
  mbedtls_sha256_context sha_;
  bool hashStarted_=false;
  bool autoApSuppressed_=false,closeApAfterUpdate_=true;
  bool persist(const Settings &next) { if(prefs_.putBytes("settings",&next,sizeof(next))!=sizeof(next)) return false; settings_=next; return true; }
  void connect(const String &ssid,const String &password,bool candidate) {
    pendingSsid_=ssid; pendingPassword_=password; candidate_=candidate; connecting_=true; connectAt_=millis(); connectedAt_=0; error_=""; networkState_="connecting";
    WiFi.begin(ssid.c_str(),password.c_str());
  }
  void openAp() { autoApSuppressed_=false; if(ap_) return; WiFi.mode(WIFI_AP_STA); ap_=WiFi.softAP(apName_.c_str(),OTA_PASSWORD,1,false,4); if(!ap_) error_="Could not start hotspot"; connectedAt_=WiFi.status()==WL_CONNECTED?millis():0; }
  void status(JsonDocument &doc) {
    hardware.status(doc);
    doc["device_id"]=id_; doc["name"]="LOOI Body"; doc["firmware_version"]=DEVICE_VERSION; doc["chip"]="ESP32-S3";
    doc["network_state"]=networkState_; doc["connected"]=WiFi.status()==WL_CONNECTED;
    doc["ssid"]=WiFi.status()==WL_CONNECTED?WiFi.SSID():String(settings_.ssid); doc["ip"]=WiFi.localIP().toString();
    doc["ota_ap_shutdown_supported"]=true; doc["close_ap_after_update"]=closeApAfterUpdate_; doc["ap_suppressed_after_ota"]=autoApSuppressed_;
    doc["ap_active"]=ap_; doc["ap_ssid"]=apName_; doc["ap_ip"]=WiFi.softAPIP().toString();
    doc["url"]=url(); doc["ota_mode"]=otaMode_; doc["ota_uploading"]=writing_; doc["error"]=error_; doc["control_owner"]=authority_.owner.c_str(); doc["control_token"]=authority_.token;
    auto map=doc.createNestedObject("calibration"); map["left_invert"]=settings_.mapping.leftInvert; map["right_invert"]=settings_.mapping.rightInvert; map["swap"]=settings_.mapping.swap;
    auto caps=doc.createNestedArray("capabilities"); for(const char *c:{"motion","wifi","calibration","ota","hardware_config","usb_management"}) caps.add(c);
    auto cmds=doc.createNestedArray("commands"); for(const char *c:{"motion","stop","ping","config_get","config_update","hardware_get","hardware_set","device_status","wifi_scan","wifi_configure","wifi_forget","calibration_get","calibration_set","calibration_test","control_acquire","control_renew","control_release","ota_start","ota_exit","ota_status","firmware_check","firmware_install"}) cmds.add(c);
  }
  bool auth(bool mutation=false) {
    if(!server_.authenticate("admin",OTA_PASSWORD)) { server_.requestAuthentication(); return false; }
    if(mutation) {
      String origin=server_.header("Origin");
      if(server_.header("X-OTA-Token")!=token_ || (!origin.isEmpty() && origin!=String("http://")+server_.header("Host"))) { server_.send(403,"application/json","{\"message\":\"Reload the panel or reconnect\"}"); return false; }
    }
    return true;
  }
  String source() { String client=server_.header("X-Looi-Client"); if(client.isEmpty()||client.length()>80) return ""; return "http:"+client; }
  void json(JsonDocument &doc,int code=200) { String out;serializeJson(doc,out);server_.sendHeader("Cache-Control","no-store");server_.send(code,"application/json",out); }
  void configureRoutes() {
    const char *headers[]={"X-OTA-Token","X-Looi-Client","X-Firmware-Size","X-Firmware-SHA256","Origin","Host"}; server_.collectHeaders(headers,6);
    server_.on("/",HTTP_GET,[this](){ if(!auth())return;server_.sendHeader("Cache-Control","no-store");server_.send_P(200,"text/html; charset=utf-8",DEVICE_PANEL); });
    server_.on("/api/status",HTTP_GET,[this](){ if(!auth())return;StaticJsonDocument<4096> doc;status(doc);doc["csrf_token"]=token_;json(doc); });
    server_.on("/api/command",HTTP_POST,[this](){ if(!auth(true))return;String client=source();if(client.isEmpty()){server_.send(400,"application/json","{\"message\":\"Client identifier required\"}");return;}String out=dispatch_(server_.arg("plain"),client);server_.sendHeader("Cache-Control","no-store");server_.send(200,"application/json",out); });
    server_.on("/update",HTTP_POST,[this](){finishUpload();},[this](){receiveUpload();});
    server_.onNotFound([this](){server_.send(404,"application/json","{\"message\":\"Not found\"}");});
  }
  void fail(const String &message) { error_=message; if(Update.isRunning())Update.abort(); if(hashStarted_){mbedtls_sha256_free(&sha_);hashStarted_=false;}writing_=false;complete_=false;Serial.printf("[OTA] %s\n",error_.c_str()); }
  bool startUpdate(size_t size,const String &sha) {
    auto partition=esp_ota_get_next_update_partition(nullptr);
    if(!partition||size<32||size>partition->size) {fail("Image exceeds OTA partition or OTA partition is missing");return false;}
    if(sha.length()!=64) {fail("SHA-256 required");return false;}
    for(size_t i=0;i<sha.length();i++) if(!isxdigit(sha[i])){fail("Invalid SHA-256");return false;}
    compatibility_.reset();
    revoke();otaMode_=true;error_="";expected_=size;received_=0;expectedSha_=sha;expectedSha_.toLowerCase();
    if(!Update.begin(size,U_FLASH)){fail(Update.errorString());return false;}
    mbedtls_sha256_init(&sha_);mbedtls_sha256_starts(&sha_,0);hashStarted_=true;writing_=true;lastWrite_=millis();return true;
  }
  bool writeChunk(uint8_t *data,size_t size) {
    if(!writing_||size>expected_-received_||(received_==0 && size && data[0]!=0xe9)){fail("Invalid application image or size");return false;}
    compatibility_.feed(data,size);
    if(Update.write(data,size)!=size){fail(Update.errorString());return false;}
    mbedtls_sha256_update(&sha_,data,size);received_+=size;lastWrite_=millis();return true;
  }
  bool verifyUpdate() {
    if(!writing_||received_!=expected_){fail("Incomplete firmware");return false;}
    uint8_t digest[32];mbedtls_sha256_finish(&sha_,digest);mbedtls_sha256_free(&sha_);hashStarted_=false;
    char hex[65];for(int i=0;i<32;i++)snprintf(hex+i*2,3,"%02x",digest[i]);
    if(expectedSha_!=hex){fail("SHA-256 mismatch");return false;}
    if(!compatibility_.found) { fail("Incompatible hardware firmware: board/configuration descriptor missing or mismatched"); return false; }
    const auto target=esp_ota_get_next_update_partition(nullptr);
    if(!target||prefs_.putUInt("ota_ap_once",otaHotspotMarker(target->address,closeApAfterUpdate_))!=sizeof(uint32_t)) { fail("Could not save post-update hotspot option"); return false; }
    if(!Update.end(false)){prefs_.remove("ota_ap_once");fail(Update.errorString());return false;}
    writing_=false;complete_=false;reboot_=true;rebootAt_=millis()+1500;return true;
  }
  void receiveUpload() {
    // Do not emit an HTTP response from multipart callbacks; authenticate again at completion.
    if(!server_.authenticate("admin",OTA_PASSWORD)||server_.header("X-OTA-Token")!=token_)return;
    String origin=server_.header("Origin");if(!origin.isEmpty()&&origin!=String("http://")+server_.header("Host"))return;
    auto &u=server_.upload();
    if(u.status==UPLOAD_FILE_START) {
      if(reboot_||uploadRequest_||writing_){fail("Only one update is allowed");return;}
      uploadRequest_=true;complete_=false;
      String size=server_.header("X-Firmware-Size");bool digits=size.length()>0&&size.length()<9;
      for(size_t i=0;i<size.length();i++)digits=digits&&isdigit(size[i]);
      if(!digits||u.name!="firmware"||!u.filename.endsWith(".bin")){fail("Application .bin and valid size required");return;}
      startUpdate(strtoul(size.c_str(),nullptr,10),server_.header("X-Firmware-SHA256"));
    } else if(u.status==UPLOAD_FILE_WRITE && writing_)writeChunk(u.buf,u.currentSize);
    else if(u.status==UPLOAD_FILE_END && writing_) { if(u.totalSize!=expected_||received_!=expected_)fail("Incomplete firmware");else complete_=true; }
    else if(u.status==UPLOAD_FILE_ABORTED){fail("Upload interrupted; retry");uploadRequest_=false;}
  }
  void finishUpload() {
    if(!auth(true))return;
    bool ok=uploadRequest_&&complete_&&verifyUpdate();
    if(!ok && error_.isEmpty())fail("No complete firmware received");
    uploadRequest_=false;StaticJsonDocument<256> doc;doc["accepted"]=ok;doc["message"]=ok?"升级成功，正在重启":error_;json(doc,ok?200:400);
  }
  bool trustedUrl(const String &url) { return url.startsWith("https://github.com/xlle-er/LOOI-DF26-Firmware/releases/")||url.startsWith("https://release-assets.githubusercontent.com/")||url.startsWith("https://objects.githubusercontent.com/"); }
  bool getHttps(const String &url,HTTPClient &http,NetworkClientSecure &client) {
    if(!trustedUrl(url)){error_="Untrusted firmware URL";return false;}
    client.useBuiltinCACertBundle();http.setTimeout(10000);http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
    String next=url;
    for(int i=0;i<5;i++) {
      if(!trustedUrl(next)||!http.begin(client,next)){error_="Invalid HTTPS firmware URL";return false;}
      const char *headers[]={"Location"};http.collectHeaders(headers,1);int code=http.GET();
      if(code==200)return true;
      if(code==301||code==302||code==307||code==308){next=http.header("Location");http.end();continue;}
      error_="Release unavailable (HTTP "+String(code)+")";http.end();return false;
    }
    error_="Too many release redirects";return false;
  }
  bool validChannel(const String &channel) { return channel=="stable" || channel=="preview"; }
  bool fetchManifest(JsonDocument &result,const String &channel="stable") {
    if(WiFi.status()!=WL_CONNECTED){error_="Connect ESP to Wi-Fi before checking online releases";return false;}
    // Certificate verification requires a valid clock; never disable TLS verification.
    if(time(nullptr)<1700000000){configTime(0,0,"pool.ntp.org","time.cloudflare.com");error_="Synchronizing clock; retry in a few seconds";return false;}
    HTTPClient http;NetworkClientSecure client;
    if(!getHttps(String("https://github.com/xlle-er/LOOI-DF26-Firmware/releases/")+(channel=="preview"?"download/preview/":"latest/download/")+MANIFEST_ASSET,http,client))return false;
    if(http.getSize()>8192){http.end();error_="Release manifest too large";return false;}
    String body=http.getString();http.end();StaticJsonDocument<8192> manifest;
    if(body.length()>8192||deserializeJson(manifest,body)||manifest["chip"]!="ESP32-S3"||manifest["partition"]!=HARDWARE_PARTITION||manifest["board"]!=HARDWARE_BOARD||manifest["hardware_protocol"]!=1||!trustedUrl(manifest["url"]|"")) {error_="Incompatible release manifest";return false;}
    manifest["channel"]=channel; result["release"]=manifest.as<JsonObjectConst>();return true;
  }
  bool installRelease(const String &version,const String &channel="stable") {
    StaticJsonDocument<8192> result;if(!fetchManifest(result,channel))return false;JsonObjectConst release=result["release"];
    if(version.isEmpty()||version!=String(release["version"]|"")){error_="Release changed; check version again";return false;}
    HTTPClient http;NetworkClientSecure client;if(!getHttps(release["url"]|"",http,client))return false;
    size_t size=release["size"]|0; if(http.getSize()!=int(size)||!startUpdate(size,release["sha256"]|"")){http.end();return false;}
    uint8_t buffer[4096];auto stream=http.getStreamPtr();
    while(received_<expected_ && millis()-lastWrite_<15000) {
      int n=stream->readBytes(buffer,min(sizeof(buffer),expected_-received_));
      if(n>0 && !writeChunk(buffer,n)){http.end();return false;}
      if(!stream->connected()&&!stream->available())break;delay(1);
    }
    http.end();return verifyUpdate();
  }
};
