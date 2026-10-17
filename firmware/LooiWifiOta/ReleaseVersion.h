#pragma once
#include <string>
#include <vector>
#include <cctype>
namespace LooiReleaseVersion {
inline std::vector<std::string> split(const std::string &s) {
  std::vector<std::string> out; size_t start=0;
  do { auto end=s.find('.',start);out.push_back(s.substr(start,end-start));if(end==std::string::npos)break;start=end+1; } while(true);
  return out;
}
inline bool numeric(const std::string &s) { if(s.empty())return false;for(char c:s)if(c<'0'||c>'9')return false;return true; }
inline int number(std::string a,std::string b) {
  auto trim=[](std::string s){auto n=s.find_first_not_of('0');return n==std::string::npos?std::string("0"):s.substr(n);};a=trim(a);b=trim(b);
  return a.size()!=b.size()?(a.size()>b.size()?1:-1):(a==b?0:(a>b?1:-1));
}
inline bool valid(const std::string &value) {
  std::string v=value;if(!v.empty()&&v[0]=='v')v.erase(0,1);
  auto end=v.find_first_of("-+");auto core=split(v.substr(0,end));
  if(core.size()!=3)return false;for(const auto &p:core)if(!numeric(p))return false;
  if(end!=std::string::npos){if(end+1==v.size())return false;for(size_t i=end+1;i<v.size();i++){char c=v[i];if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='.'||c=='-'||c=='+'))return false;}}
  return true;
}
// SemVer precedence; build metadata never makes an older preview newer.
inline int compare(std::string a,std::string b) {
  if(!a.empty()&&a[0]=='v')a.erase(0,1);if(!b.empty()&&b[0]=='v')b.erase(0,1);
  a=a.substr(0,a.find('+'));b=b.substr(0,b.find('+'));
  auto ad=a.find('-'),bd=b.find('-');auto ac=split(a.substr(0,ad)),bc=split(b.substr(0,bd));
  for(size_t i=0;i<ac.size()||i<bc.size();i++){int n=number(i<ac.size()?ac[i]:"0",i<bc.size()?bc[i]:"0");if(n)return n;}
  if(ad==std::string::npos||bd==std::string::npos)return ad==bd?0:(ad==std::string::npos?1:-1);
  auto ap=split(a.substr(ad+1)),bp=split(b.substr(bd+1));
  for(size_t i=0;i<ap.size()&&i<bp.size();i++){if(ap[i]==bp[i])continue;bool an=numeric(ap[i]),bn=numeric(bp[i]);if(an&&bn)return number(ap[i],bp[i]);if(an!=bn)return an?-1:1;return ap[i]>bp[i]?1:-1;}
  return ap.size()==bp.size()?0:(ap.size()>bp.size()?1:-1);
}
}
