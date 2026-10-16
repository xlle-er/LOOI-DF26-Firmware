#include "../HardwarePolicy.h"
#include <cassert>
#include <vector>
int main() {
  HardwareProfile profile;
  assert(validHardwareProfile(profile));
  for(int bad : {0,3,19,20,22,25,26,32,33,37,38,43,44,45,46,48,49,255}) {
    auto next=profile;next.pins[0]=bad;assert(!validHardwareProfile(next));
  }
  auto duplicate=profile;duplicate.pins[0]=duplicate.pins[1];assert(!validHardwareProfile(duplicate));
  auto version=profile;version.version=0;assert(!validHardwareProfile(version));
  auto wrong=profile;wrong.flashMb=LOOI_FLASH_MB==8?16:8;assert(!validHardwareProfile(wrong));
  auto custom=profile;uint8_t pins[]={1,2,4,8,9,21};for(int i=0;i<6;++i)custom.pins[i]=pins[i];assert(validHardwareProfile(custom));
  CompatibilityScanner scanner;
  const auto *marker=reinterpret_cast<const uint8_t *>(HARDWARE_DESCRIPTOR);
  for(size_t split=0;split<sizeof(HARDWARE_DESCRIPTOR);++split) {
    scanner.reset();scanner.feed(marker,split);scanner.feed(marker+split,sizeof(HARDWARE_DESCRIPTOR)-1-split);assert(scanner.found);
  }
  scanner.reset();const char legacy[]="ESP32-S3 firmware 2.0.2";scanner.feed(reinterpret_cast<const uint8_t *>(legacy),sizeof(legacy));assert(!scanner.found);
#if LOOI_FLASH_MB == 8
  const char other[]="LOOI_HW_V1|n16r8|1|app3M_fat9M_16MB|END";
#else
  const char other[]="LOOI_HW_V1|n8r8|1|looi_8m|END";
#endif
  scanner.feed(reinterpret_cast<const uint8_t *>(other),sizeof(other));assert(!scanner.found);
  scanner.feed(marker,sizeof(HARDWARE_DESCRIPTOR)-1);assert(scanner.found);
}
