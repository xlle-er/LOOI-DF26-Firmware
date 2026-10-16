#include "../DevicePolicy.h"
#include <cassert>
#include <cmath>
#include <cstdio>
int main(){
 for(int mask=0;mask<8;mask++){
  MotorMapping m;m.leftInvert=mask&1;m.rightInvert=mask&2;m.swap=mask&4;
  for(float l:{-0.4f,0.0f,0.4f})for(float r:{-0.4f,0.0f,0.4f}){
   float x=l,y=r;m.apply(x,y);
   auto f=m;f.reverseForward();float a=-r,b=-l;m.apply(a,b);float c=l,d=r;f.apply(c,d);assert(std::fabs(a-c)<1e-6&&std::fabs(b-d)<1e-6);
   auto t=m;t.reverseSteering();a=r;b=l;m.apply(a,b);c=l;d=r;t.apply(c,d);assert(std::fabs(a-c)<1e-6&&std::fabs(b-d)<1e-6);
  }
 }
 ControlAuthority c;assert(c.allow("ble",0,0));assert(!c.acquire("http:phone",false,10));
 assert(c.acquire("http:phone",true,10));auto old=c.token;
 assert(!c.allow("ble",0,20));assert(c.allow("http:phone",old,20));assert(!c.allow("http:other",old,30));
 c.revoke();assert(!c.allow("http:phone",old,50));assert(c.acquire("http:phone",false,60));assert(!c.allow("http:phone",old,70));
 assert(c.allow("http:phone",c.token,70));assert(c.expired(1070));assert(!c.allow("http:phone",c.token,1071));
 assert(c.acquire("http:phone",false,0xffffff00));assert(!c.expired(0x100));assert(c.expired(0x400));
 assert(suppressHotspotAfterOta(otaHotspotMarker(0x310000,true),0x310000));
 assert(!suppressHotspotAfterOta(otaHotspotMarker(0x310000,true),0x10000));
 assert(!suppressHotspotAfterOta(otaHotspotMarker(0x310000,false),0x310000));
 assert(!suppressHotspotAfterOta(0,0x310000));
 // BOOT starts AP later than the loop's timestamp: unsigned subtraction
 // previously looked like a 49-day timeout and closed AP immediately.
 assert(!hotspotGraceExpired(1000,1001));
 assert(!hotspotGraceExpired(1001,1001));
 assert(!hotspotGraceExpired(61000,1001));
 assert(hotspotGraceExpired(61001,1001));
 // Reopening an active AP refreshes its window; zero is a valid start time.
 assert(!hotspotGraceExpired(61001,60000));
 assert(!hotspotGraceExpired(59999,0));
 assert(hotspotGraceExpired(60000,0));
 constexpr uint32_t apStart=0xfffffff0u;
 assert(!hotspotGraceExpired(0xffffffefu,apStart));
 assert(!hotspotGraceExpired(uint32_t(apStart+59999u),apStart));
 assert(hotspotGraceExpired(uint32_t(apStart+60000u),apStart));
 std::puts("PASS: hotspot stale timestamp, renewed grace window, exact timeout and clock wrap");
 std::puts("PASS: all mapping combinations, independent shortcuts, takeover, stale-token rejection, lease expiry and clock wrap");
}
