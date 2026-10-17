#include "../ReleaseVersion.h"
#include <cassert>
int main(){
assert((LooiReleaseVersion::compare("2.2.0","2.1.0") > 0));
assert((LooiReleaseVersion::compare("2.10.0","2.9.9") > 0));
assert((LooiReleaseVersion::compare("2.2.0","2.2.0") == 0));
assert((LooiReleaseVersion::compare("2.3.0-beta.2","2.3.0-beta.10") < 0));
assert((LooiReleaseVersion::compare("2.3.0","2.3.0-rc.1") > 0));
assert((LooiReleaseVersion::compare("2.3.0-beta","2.3.0-alpha") > 0));
assert((LooiReleaseVersion::compare("2.3.0+old","2.3.0+new") == 0));
assert((LooiReleaseVersion::compare("v2.2.0","2.2.0") == 0));
assert((LooiReleaseVersion::compare("2.3.0-beta","2.2.9") > 0));
assert((LooiReleaseVersion::compare("2.3.0-1","2.3.0-alpha") < 0));
assert((LooiReleaseVersion::compare("2.3.0-beta","2.3.0-beta.1") < 0));
}
