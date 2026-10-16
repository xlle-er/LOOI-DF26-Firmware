#pragma once
#include <string>
#include <vector>
#include <cstring>
using String=std::string;
struct FakeEsp { uint32_t getFlashChipSize() const{return LOOI_FLASH_MB*1048576u;} uint32_t getPsramSize() const{return 8388608;} };
inline FakeEsp ESP;
class Preferences {
 public:
  inline static bool openOk=true,writeOk=true;
  inline static std::vector<uint8_t> bytes;
  bool begin(const char *,bool){return openOk;}
  size_t getBytesLength(const char *){return bytes.size();}
  size_t getBytes(const char *,void *out,size_t len){if(len!=bytes.size())return 0;memcpy(out,bytes.data(),len);return len;}
  size_t putBytes(const char *,const void *data,size_t len){if(!writeOk)return 0;const auto *p=static_cast<const uint8_t *>(data);bytes.assign(p,p+len);return len;}
};
