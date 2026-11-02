// Linux test-only provider IO interposition. Never linked into production.
// The default Rust executable retains every policy/SQL/HTTP decision. Only
// fixed Valve HTTPS exchanges are replaced with independent HMAC fixture IO.
#include <curl/curl.h>
#include <dlfcn.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <nlohmann/json.hpp>
#include <cstdarg>
#include <cstdlib>
#include <fstream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <chrono>

#undef curl_easy_setopt
#undef curl_easy_getinfo
using Json=nlohmann::json;
struct State {
  std::string url,body,proxy="<unset>",encoding,authorization;
  curl_off_t size=-1;
  curl_write_callback write{},header{};
  void *output{},*headers{};
  long status{},peer=-1,host=-1,follow=-1,timeout=-1,connect=-1,decode=-1;
  bool handled{};
};
std::mutex &gate(){static std::mutex value;return value;}
std::map<CURL*,State> &states(){static std::map<CURL*,State> value;return value;}
template<class T>T original(const char *name){auto value=reinterpret_cast<T>(dlsym(RTLD_NEXT,name));if(!value)std::abort();return value;}
using Set=CURLcode(*)(CURL*,CURLoption,...);
using Info=CURLcode(*)(CURL*,CURLINFO,...);
std::string decode(const std::string &text) {
  std::string out;
  auto hex=[](unsigned char c)->int {if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return -1;};
  for(std::size_t i=0;i<text.size();++i) {
    if(text[i]=='+')out+=' ';
    else if(text[i]=='%') {if(i+2>=text.size()||hex(text[i+1])<0||hex(text[i+2])<0)throw 1;out+=char(hex(text[i+1])*16+hex(text[i+2]));i+=2;}
    else out+=text[i];
  }return out;
}
std::map<std::string,std::string> fields(const std::string &text) {
  std::map<std::string,std::string> out;std::size_t at{};
  while(at<text.size()) {auto end=text.find('&',at);if(end==std::string::npos)end=text.size();auto item=text.substr(at,end-at);auto equal=item.find('=');if(equal==std::string::npos)throw 1;
    if(!out.emplace(decode(item.substr(0,equal)),decode(item.substr(equal+1))).second)throw 1;at=end+1;}
  return out;
}
std::string signature(const std::map<std::string,std::string> &values) {
  std::string signed_text;const auto &names=values.at("openid.signed");std::size_t at{};
  while(at<names.size()) {auto end=names.find(',',at);if(end==std::string::npos)end=names.size();const auto name=names.substr(at,end-at);signed_text+=name+":"+values.at("openid."+name)+"\n";at=end+1;}
  const std::string key="independent-rust-service-openid-fixture";
  unsigned char hash[EVP_MAX_MD_SIZE];unsigned size{};
  if(!HMAC(EVP_sha256(),key.data(),int(key.size()),reinterpret_cast<const unsigned char*>(signed_text.data()),signed_text.size(),hash,&size))throw 1;
  std::string result(4*((size+2)/3)+1,'\0');const auto count=EVP_EncodeBlock(reinterpret_cast<unsigned char*>(result.data()),hash,int(size));result.resize(count);return result;
}
void record(const Json &value) {
  const auto *path=std::getenv("H2_TEST_STEAM_PROVIDER_CALLS");if(!path)throw 1;
  std::lock_guard lock(gate());std::ofstream stream(path,std::ios::app);stream<<value.dump()<<'\n';stream.flush();if(!stream)throw 1;
}
extern "C" CURLcode curl_easy_setopt(CURL *handle,CURLoption option,...) {
  try {
    auto real=original<Set>("curl_easy_setopt");va_list args;va_start(args,option);
    std::lock_guard lock(gate());auto &s=states()[handle];
    if(option<CURLOPTTYPE_OBJECTPOINT) {
      const auto value=va_arg(args,long);va_end(args);
      if(option==CURLOPT_POSTFIELDSIZE)s.size=value;
      if(option==CURLOPT_SSL_VERIFYPEER)s.peer=value;if(option==CURLOPT_SSL_VERIFYHOST)s.host=value;
      if(option==CURLOPT_FOLLOWLOCATION)s.follow=value;if(option==CURLOPT_TIMEOUT_MS)s.timeout=value;if(option==CURLOPT_CONNECTTIMEOUT_MS)s.connect=value;
      if(option==CURLOPT_TIMEOUT)s.timeout=value*1000;if(option==CURLOPT_CONNECTTIMEOUT)s.connect=value*1000;
      if(option==CURLOPT_HTTP_CONTENT_DECODING)s.decode=value;
      return real(handle,option,value);
    }
    if(option==CURLOPT_WRITEFUNCTION||option==CURLOPT_HEADERFUNCTION) {
      const auto value=va_arg(args,curl_write_callback);va_end(args);if(option==CURLOPT_WRITEFUNCTION)s.write=value;else s.header=value;
      return real(handle,option,value);
    }
    if(option>=CURLOPTTYPE_FUNCTIONPOINT&&option<CURLOPTTYPE_OFF_T) {
      auto value=va_arg(args,void(*)(void));va_end(args);return real(handle,option,value);
    }
    if(option>=CURLOPTTYPE_OFF_T&&option<CURLOPTTYPE_BLOB) {
      const auto value=va_arg(args,curl_off_t);va_end(args);if(option==CURLOPT_POSTFIELDSIZE_LARGE)s.size=value;return real(handle,option,value);
    }
    auto *value=va_arg(args,void*);va_end(args);
    if(option==CURLOPT_URL)s.url=value?static_cast<char*>(value):"";
    if(option==CURLOPT_PROXY)s.proxy=value?static_cast<char*>(value):"";
    if(option==CURLOPT_ACCEPT_ENCODING)s.encoding=value?static_cast<char*>(value):"";
    if(option==CURLOPT_HTTPHEADER) {
      s.authorization.clear();
      for(auto *item=static_cast<curl_slist*>(value);item;item=item->next) {
        const std::string field(item->data);if(field.starts_with("Authorization: "))s.authorization=field.substr(15);
      }
    }
    if(option==CURLOPT_WRITEDATA)s.output=value;if(option==CURLOPT_HEADERDATA)s.headers=value;
    if(option==CURLOPT_COPYPOSTFIELDS||option==CURLOPT_POSTFIELDS) {
      if(!value)s.body.clear();else if(s.size>=0&&s.size<=32768)s.body.assign(static_cast<char*>(value),std::size_t(s.size));else return CURLE_BAD_FUNCTION_ARGUMENT;
    }
    return real(handle,option,value);
  }catch(...){return CURLE_FAILED_INIT;}
}
extern "C" CURLcode curl_easy_getinfo(CURL *handle,CURLINFO info,...) {
  va_list args;va_start(args,info);auto *value=va_arg(args,void*);va_end(args);
  {std::lock_guard lock(gate());auto found=states().find(handle);if(info==CURLINFO_RESPONSE_CODE&&found!=states().end()&&found->second.handled){*static_cast<long*>(value)=found->second.status;return CURLE_OK;}}
  return original<Info>("curl_easy_getinfo")(handle,info,value);
}
extern "C" void curl_easy_cleanup(CURL *handle) {
  {std::lock_guard lock(gate());states().erase(handle);}original<void(*)(CURL*)>("curl_easy_cleanup")(handle);
}
extern "C" CURLcode curl_easy_perform(CURL *handle) {
  try {
    State s;{std::lock_guard lock(gate());s=states().at(handle);}
    const bool openid=s.url=="https://steamcommunity.com/openid/login";
    const bool profile=s.url.starts_with("https://api.steampowered.com/ISteamUser/GetPlayerSummaries/v2/?");
    const bool friends=s.url.starts_with("https://api.steampowered.com/ISteamUser/GetFriendList/v1/?");
    const bool discord=s.url=="https://discord.com/api/v10/oauth2/@me"||s.url=="https://discord.com/api/v10/users/@me";
    if(!openid&&!profile&&!friends&&!discord)return original<CURLcode(*)(CURL*)>("curl_easy_perform")(handle);
    if(s.proxy!=""||s.peer!=1||s.host!=2||s.follow!=0||s.timeout!=5000||s.connect!=2000||s.encoding!="identity"||s.decode!=0||!s.write||!s.header) {
      record({{"phase","unsafe-provider-options"}});return CURLE_BAD_FUNCTION_ARGUMENT;
    }
    const auto *path=std::getenv("H2_TEST_STEAM_PROVIDER_DATA");if(!path)throw 1;
    std::ifstream control(path);Json config;control>>config;if(!control)throw 1;
    auto mode=config.value("mode",std::string{});std::string body,subject;bool valid=true;
    if(discord) {
      if(s.authorization!="Bearer discord-fixture-token")return CURLE_LOGIN_DENIED;
      if(s.url.ends_with("oauth2/@me"))body=Json{{"application",{{"id",config.value("discord_bad_app",false)?"9999999999999999999":"1558522073272426519"}}},{"scopes",Json::array({"identify"})}}.dump();
      else body=Json{{"id",config.value("discord_subject",std::string("123456789012345678"))},{"username",config.value("discord_name",std::string("retained_pilot"))},{"bot",false}}.dump();
    } else if(openid) {
      const auto values=fields(s.body);valid=values.at("openid.mode")=="check_authentication"&&values.at("openid.sig")==signature(values);
      const auto &identity=values.at("openid.claimed_id");subject=identity.substr(identity.find_last_of('/')+1);
      if(mode=="reject")valid=false;
      body="ns:http://specs.openid.net/auth/2.0\nis_valid:"+std::string(valid?"true":"false")+"\n";
      if(mode=="duplicate")body+="is_valid:true\n";
      if(mode=="oversize")body+=std::string(4097,'x');
    } else if(friends) {
      const auto values=fields(s.url.substr(s.url.find('?')+1));subject=values.at("steamid");
      if(values.at("key")!=std::string(32,'1')||values.at("relationship")!="friend")throw 1;
      Json rows=Json::array();for(const auto &id:config.value("friend_ids",Json::array()))rows.push_back({{"steamid",id},{"relationship","friend"}});
      body=config.contains("friends_raw")?config.at("friends_raw").get<std::string>():Json{{"friendslist",{{"friends",rows}}}}.dump();
    } else if(config.value("relationship_fixture",false)) {
      const auto values=fields(s.url.substr(s.url.find('?')+1));subject=values.at("steamids");
      if(values.at("key")!=std::string(32,'1'))throw 1;
      Json rows=Json::array();std::size_t at{};
      while(at<subject.size()) {
        auto end=subject.find(',',at);if(end==std::string::npos)end=subject.size();const auto id=subject.substr(at,end-at);
        auto name=config.value("friends_name",std::string("Relationship Fixture"));
        if(config.contains("friend_names")&&config.at("friend_names").contains(id))name=config.at("friend_names").at(id).get<std::string>();
        rows.push_back({{"steamid",id},{"personaname",name},{"personastate",config.value("personastate",1)}});at=end+1;
      }
      for(const auto &row:config.value("summary_extra",Json::array()))rows.push_back(row);
      body=config.contains("summary_raw")?config.at("summary_raw").get<std::string>():Json{{"response",{{"players",rows}}}}.dump();
    } else {
      const auto values=fields(s.url.substr(s.url.find('?')+1));subject=values.at("steamids");
      if(values.at("key")!=std::string(32,'1'))throw 1;
      body=Json{{"response",{{"players",Json::array({Json{{"steamid",mode=="wrong-profile"?"76561197960287999":subject},{"personaname",config.value("name",std::string("Steam Fixture"))}}})}}}}.dump();
    }
    record({{"phase",discord?"discord":openid?"openid":friends?"friends":"profile"},{"signature_valid",valid},{"subject",subject},{"mode",mode},{"safe_options",true}});
    if(openid||discord)std::this_thread::sleep_for(std::chrono::milliseconds(config.value(discord?"discord_delay_ms":"delay_ms",0)));
    if(friends||(profile&&config.value("relationship_fixture",false)))std::this_thread::sleep_for(std::chrono::milliseconds(config.value("friends_delay_ms",0)));
    const auto status=friends?config.value("friends_status",mode=="redirect"?302:mode=="unavailable"?503:200):mode=="redirect"?302:mode=="unavailable"?503:200;
    {std::lock_guard lock(gate());states()[handle].handled=true;states()[handle].status=status;}
    const std::string header="HTTP/1.1 "+std::to_string(status)+" Fixture\r\n";
    if(s.header(const_cast<char*>(header.data()),1,header.size(),s.headers)!=header.size())return CURLE_WRITE_ERROR;
    if(mode=="encoded") {std::string line="Content-Encoding: gzip\r\n";if(s.header(line.data(),1,line.size(),s.headers)!=line.size())return CURLE_WRITE_ERROR;}
    const std::string blank="\r\n";s.header(const_cast<char*>(blank.data()),1,blank.size(),s.headers);
    return s.write(body.data(),1,body.size(),s.output)==body.size()?CURLE_OK:CURLE_WRITE_ERROR;
  }catch(...){return CURLE_RECV_ERROR;}
}
