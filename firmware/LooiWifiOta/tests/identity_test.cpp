#include "../DeviceIdentity.h"
#include <cassert>
#include <string>
#include <iostream>
int main(){
  assert(validDeviceName("小乐"));assert(validDeviceName("一二三四五六七八"));
  assert(!validDeviceName("一二三四五六七八九"));assert(!validDeviceName(""));assert(!validDeviceName("   "));assert(!validDeviceName("　"));
  assert(!validDeviceName("a\n"));assert(!validDeviceName("\xc0\xaf"));assert(!validDeviceName("\xed\xa0\x80"));assert(!validDeviceName("\xf4\x90\x80\x80"));assert(!validDeviceName("\xe4\xb8"));
  assert(validAdminPassword("12345678"));assert(validAdminPassword(std::string(63,'a').c_str()));assert(!validAdminPassword("1234567"));assert(!validAdminPassword(std::string(64,'a').c_str()));assert(!validAdminPassword("密码12345678"));assert(!validAdminPassword("1234567\n"));
  DeviceIdentity original;assert(original.valid());auto changed=original;strcpy(changed.name,"小乐");strcpy(changed.password,"new-password");assert(changed.valid());assert(!original.name[0]);assert(strcmp(original.password,"12345678")==0);
  changed.name[24]='x';assert(!changed.valid());changed=original;changed.password[63]='x';assert(!changed.valid());changed=original;changed.version=99;assert(!changed.valid());
  std::cout<<"PASS: UTF-8 identity bounds, visible names, password limits and persisted blob validation\n";
}
