#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Validate complete UTF-8 scalars; limits are bytes, never truncated characters.
inline bool validDeviceName(const char *name) {
  size_t n=strlen(name); if(!n || n>24) return false;
  bool visible=false;
  for(size_t i=0;i<n;) {
    uint8_t c=uint8_t(name[i++]); uint32_t cp=c; int extra=0;
    if(c>=0xc2&&c<=0xdf){cp=c&31;extra=1;}
    else if(c>=0xe0&&c<=0xef){cp=c&15;extra=2;}
    else if(c>=0xf0&&c<=0xf4){cp=c&7;extra=3;}
    else if(c>=128)return false;
    if(i+extra>n)return false;
    for(int k=0;k<extra;k++){uint8_t b=uint8_t(name[i++]);if((b&0xc0)!=0x80)return false;cp=(cp<<6)|(b&63);}
    if((extra==1&&cp<128)||(extra==2&&cp<2048)||(extra==3&&cp<65536)||cp>0x10ffff||(cp>=0xd800&&cp<=0xdfff))return false;
    if(cp<32||(cp>=127&&cp<=159)||cp==0x2028||cp==0x2029)return false;
    bool space=cp==32||cp==0xa0||cp==0x1680||(cp>=0x2000&&cp<=0x200b)||cp==0x202f||cp==0x205f||cp==0x3000||cp==0xfeff;
    if(!space)visible=true;
  }
  return visible;
}
inline bool validAdminPassword(const char *password) {
  size_t n=strlen(password); if(n<8||n>63)return false;
  for(size_t i=0;i<n;i++)if(uint8_t(password[i])<32||uint8_t(password[i])>126)return false;
  return true;
}
struct DeviceIdentity {
  uint32_t version=1;
  char name[25]={};
  char password[64]="12345678";
  bool valid() const { return version==1&&name[24]==0&&password[63]==0&&(!name[0]||validDeviceName(name))&&validAdminPassword(password); }
};
