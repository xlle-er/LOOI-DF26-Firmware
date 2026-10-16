#pragma once
#include <Preferences.h>
#include <ArduinoJson.h>
#include "HardwarePolicy.h"
class HardwareConfig {
 public:
  HardwareProfile profile;
  bool ready=false;
  String error="Hardware configuration required";
  void load(bool nvsOk) {
    ready=false;
    if(!nvsOk || !prefs_.begin("looi-hardware",false)) { error="NVS unavailable; motors disabled"; return; }
    opened_=true;
    if(prefs_.getBytesLength("profile")!=sizeof(profile)) return;
    HardwareProfile loaded;
    if(prefs_.getBytes("profile",&loaded,sizeof(loaded))!=sizeof(loaded) || !validHardwareProfile(loaded)) {
      error="Invalid hardware configuration; motors disabled"; return;
    }
    profile=loaded; valid_=true; error="Motor outputs not initialized";
  }
  bool configured() const { return valid_; }
  bool save(JsonObjectConst root) {
    if(!opened_) { error="NVS unavailable; configuration not saved"; return false; }
    if(root["board"]!=HARDWARE_BOARD || !root["version"].is<int>() || root["version"].is<bool>() || root["version"].as<int>()!=1) {
      error="Incompatible board or hardware configuration version"; return false;
    }
    if(!root["pins"].is<JsonObjectConst>() || root["pins"].size()!=6) { error="Exactly six named motor GPIOs required"; return false; }
    HardwareProfile next;
    for(size_t i=0;i<6;++i) {
      auto v=root["pins"][HARDWARE_PIN_NAMES[i]];
      if(!v.is<int>() || v.is<bool>() || v.as<int>()<0 || v.as<int>()>48) {
        error="Six integer GPIO numbers required"; return false;
      }
      next.pins[i]=v.as<int>();
    }
    if(!validHardwareProfile(next)) { error="Duplicate or reserved motor GPIO"; return false; }
    if(prefs_.putBytes("profile",&next,sizeof(next))!=sizeof(next)) { error="Could not save hardware configuration"; return false; }
    // NVS blob replacement is atomic; running outputs keep their old mapping until reboot.
    return true;
  }
  void status(JsonDocument &doc) const {
    doc["hardware_ready"]=ready; doc["hardware_configured"]=valid_;
    doc["hardware_error"]=error;
    auto p=doc["hardware_profile"].to<JsonObject>();
    p["version"]=1; p["board"]=HARDWARE_BOARD;
    auto pins=p["pins"].to<JsonObject>();
    if(valid_) for(size_t i=0;i<6;++i) pins[HARDWARE_PIN_NAMES[i]]=profile.pins[i];
    doc["physical_flash_bytes"]=ESP.getFlashChipSize(); doc["psram_bytes"]=ESP.getPsramSize();
    doc["flash_mb"]=LOOI_FLASH_MB; doc["partition"]=HARDWARE_PARTITION;
    doc["hardware_protocol"]=1;
  }
 private:
  Preferences prefs_;
  bool opened_=false,valid_=false;
};
