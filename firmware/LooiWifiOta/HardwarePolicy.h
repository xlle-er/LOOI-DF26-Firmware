#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#ifndef LOOI_FLASH_MB
#define LOOI_FLASH_MB 16
#endif
#if LOOI_FLASH_MB == 8
constexpr char HARDWARE_BOARD[] = "n8r8";
constexpr char HARDWARE_PARTITION[] = "looi_8m";
constexpr char HARDWARE_DESCRIPTOR[] = "LOOI_HW_V1|n8r8|1|looi_8m|END";
constexpr char MANIFEST_ASSET[] = "looi-s3-n8r8-manifest.json";
#else
static_assert(LOOI_FLASH_MB == 16, "Unsupported flash profile");
constexpr char HARDWARE_BOARD[] = "n16r8";
constexpr char HARDWARE_PARTITION[] = "app3M_fat9M_16MB";
constexpr char HARDWARE_DESCRIPTOR[] = "LOOI_HW_V1|n16r8|1|app3M_fat9M_16MB|END";
constexpr char MANIFEST_ASSET[] = "looi-s3-manifest.json";
#endif
constexpr const char *HARDWARE_PIN_NAMES[] = {"ena","in1","in2","in3","in4","enb"};
#include "HardwarePins.h"
struct HardwareProfile {
  uint32_t version = 1;
  uint32_t flashMb = LOOI_FLASH_MB;
  uint8_t pins[6] = {LOOI_DEFAULT_PINS};
  uint8_t reserved[2] = {0,0};
};
inline bool validHardwareProfile(const HardwareProfile &p) {
  if(p.version != 1 || p.flashMb != LOOI_FLASH_MB || p.reserved[0] || p.reserved[1]) return false;
  for(size_t i=0;i<6;++i) {
    if(!allowedMotorPin(p.pins[i])) return false;
    for(size_t j=0;j<i;++j) if(p.pins[i]==p.pins[j]) return false;
  }
  return true;
}
// Streaming descriptor matching survives arbitrary chunk boundaries.
struct CompatibilityScanner {
  size_t matched=0; bool found=false;
  void reset() { matched=0; found=false; }
  void feed(const uint8_t *data,size_t size) {
    for(size_t i=0;i<size;++i) {
      if(data[i]==uint8_t(HARDWARE_DESCRIPTOR[matched])) ++matched;
      else matched=data[i]==uint8_t(HARDWARE_DESCRIPTOR[0]) ? 1 : 0;
      if(matched==sizeof(HARDWARE_DESCRIPTOR)-1) { found=true; matched=0; }
    }
  }
};
