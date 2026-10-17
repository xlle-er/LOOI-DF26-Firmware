#pragma once
#include <string>
#include <vector>
#include <map>
#include <cstring>
using String=std::string;
struct FakeEsp { uint32_t getFlashChipSize() const{return LOOI_FLASH_MB*1048576u;} uint32_t getPsramSize() const{return 8388608;} };
inline FakeEsp ESP;
class Preferences {
 public:
  inline static bool openOk=true,writeOk=true;
  inline static std::vector<uint8_t> bytes;
  bool begin(const char *name,bool){space=name;return openOk;}
  inline static std::map<std::string,std::vector<uint8_t>> values;
 private:
  std::string space;
  std::vector<uint8_t> &value(const char *key){return space=="looi-hardware"&&std::string(key)=="profile"?bytes:values[space+":"+key];}
 public:
  size_t getBytesLength(const char *key){return value(key).size();}
  size_t getBytes(const char *key,void *out,size_t len){auto &data=value(key);if(len!=data.size())return 0;memcpy(out,data.data(),len);return len;}
  size_t putBytes(const char *key,const void *data,size_t len){if(!writeOk)return 0;const auto *p=static_cast<const uint8_t *>(data);value(key).assign(p,p+len);return len;}
};
