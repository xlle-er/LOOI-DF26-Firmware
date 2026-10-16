#include "../HardwareConfig.h"
#include <cassert>
JsonDocument request() {
 JsonDocument d;auto h=d.to<JsonObject>();h["version"]=1;h["board"]=HARDWARE_BOARD;
 const int values[]={1,2,4,8,9,21};for(int i=0;i<6;++i)h["pins"][HARDWARE_PIN_NAMES[i]]=values[i];return d;
}
int main() {
 HardwareConfig blank;blank.load(true);assert(!blank.ready && !blank.configured());
 auto doc=request();assert(blank.save(doc.as<JsonObjectConst>()));assert(!blank.ready && !blank.configured());
 HardwareConfig afterRestart;afterRestart.load(true);assert(afterRestart.configured() && !afterRestart.ready);assert(afterRestart.profile.pins[0]==1);
 auto saved=Preferences::bytes;Preferences::writeOk=false;doc["pins"]["ena"]=10;assert(!afterRestart.save(doc.as<JsonObjectConst>()));assert(Preferences::bytes==saved);assert(afterRestart.profile.pins[0]==1);
 Preferences::writeOk=true;
 for(auto bad : {"bool","float","duplicate","reserved","version","versionbool","extra","board","missing"}) {
  auto invalid=request();
  if(!strcmp(bad,"bool"))invalid["pins"]["ena"]=true;
  if(!strcmp(bad,"float"))invalid["pins"]["ena"]=1.5;
  if(!strcmp(bad,"duplicate"))invalid["pins"]["ena"]=2;
  if(!strcmp(bad,"reserved"))invalid["pins"]["ena"]=33;
  if(!strcmp(bad,"version"))invalid["version"]=2;
  if(!strcmp(bad,"versionbool"))invalid["version"]=true;
  if(!strcmp(bad,"extra"))invalid["pins"]["extra"]=10;
  if(!strcmp(bad,"board"))invalid["board"]="unknown";
  if(!strcmp(bad,"missing"))invalid["pins"].remove("ena");
  assert(!afterRestart.save(invalid.as<JsonObjectConst>()));assert(Preferences::bytes==saved);
 }
 Preferences::bytes[0]=99;HardwareConfig corrupt;corrupt.load(true);assert(!corrupt.configured() && !corrupt.ready);
 HardwareConfig noNvs;noNvs.load(false);assert(!noNvs.configured() && !noNvs.ready);assert(!noNvs.save(doc.as<JsonObjectConst>()));
 Preferences::openOk=false;HardwareConfig openFailure;openFailure.load(true);assert(!openFailure.configured() && !openFailure.ready);
}
