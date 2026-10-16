#pragma once
#include <stdint.h>
struct MotorMapping {
  bool leftInvert = true;
  bool rightInvert = false;
  bool swap = false;
  void reverseForward() { leftInvert = !leftInvert; rightInvert = !rightInvert; swap = !swap; }
  void reverseSteering() { swap = !swap; }
  void apply(float &left, float &right) const {
    if (swap) { float t = left; left = right; right = t; }
    if (leftInvert) left = -left;
    if (rightInvert) right = -right;
  }
};
// Wrap-safe, fixed-size ownership state, shared by HTTP and BLE.
struct ControlLease {
  uint32_t expires = 0;
  bool owned = false;
  bool expired(uint32_t now) const { return owned && int32_t(now - expires) >= 0; }
  void renew(uint32_t now) { owned = true; expires = now + 1000; }
  void release() { owned = false; }
};
#include <string>
struct ControlAuthority {
  std::string owner;
  uint32_t token=0;
  ControlLease lease;
  bool expired(uint32_t now) const { return lease.expired(now); }
  void revoke() { ++token; owner.clear(); lease.release(); }
  bool acquire(const char *source,bool takeover,uint32_t now) {
    if(expired(now))revoke();
    if(!owner.empty()&&owner!=source&&!takeover)return false;
    revoke();owner=source;lease.renew(now);return true;
  }
  bool allow(const char *source,uint32_t supplied,uint32_t now) {
    if(expired(now))revoke();
    if(owner.empty()&&std::string(source)=="ble")acquire(source,false,now);
    if(owner!=source || (owner!="ble"&&supplied!=token))return false;
    lease.renew(now);return true;
  }
};

// Bind the one-boot preference to the partition actually activated by OTA.
constexpr uint32_t otaHotspotMarker(uint32_t address,bool close) { return (address & ~1u) | (close ? 1u : 0u); }
constexpr bool suppressHotspotAfterOta(uint32_t marker,uint32_t running) { return marker && (marker & 1u) && (marker & ~1u)==running; }

// openAp() may refresh the deadline after loop() samples now. Treat that
// slightly older sample as pending, including across millis() rollover.
constexpr bool hotspotGraceExpired(uint32_t now,uint32_t started) {
  return int32_t(now-started)>=60000;
}
