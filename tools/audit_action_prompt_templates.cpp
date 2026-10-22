#include "halo2/hud.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <set>
using namespace halo2;
using Json=nlohmann::json;
namespace {
struct Row {std::string name,text;};
std::string hash(const std::filesystem::path &path) {
  std::ifstream file(path,std::ios::binary);auto *context=EVP_MD_CTX_new();
  if(!file || !context)throw FormatError("Cannot hash prompt source");
  EVP_DigestInit_ex(context,EVP_sha256(),nullptr);std::array<char,65536> buffer{};
  while(file){file.read(buffer.data(),buffer.size());EVP_DigestUpdate(context,buffer.data(),file.gcount());}
  std::array<unsigned char,32> bytes{};unsigned size{};EVP_DigestFinal_ex(context,bytes.data(),&size);EVP_MD_CTX_free(context);
  constexpr char hex[]="0123456789abcdef";std::string result;
  for(auto value:bytes){result+=hex[value>>4];result+=hex[value&15];}return result;
}
std::map<std::string,Row> rows(const Cache &cache,bool include_locale=false) {
  const auto bipeds=cache.select_tags({.group=tag_groups::biped,.require_address=true});
  if(bipeds.begin()==bipeds.end())return {};
  const auto hud=cache.load_hud((*bipeds.begin()).datum,null_datum);
  std::map<std::string,Row> result;
  const auto add=[&](const std::string &key,std::uint32_t sid) {
    if(!sid || sid==null_datum)return;
    const auto text=hud.strings.find(sid);if(text==hud.strings.end())return;
    result.emplace(key,Row{cache.string_id(sid),text->second});
  };
  for(const auto &tag:cache.select_tags({.require_address=true})) {
    const auto word=[&](unsigned offset){return Bytes(cache.read_tag_bytes(tag.address+offset,4)).u32(0);};
    if(tag.group=="weap") {
      constexpr std::array names{"pickup","swap","pickup_or_dual","swap_or_dual","dual"};
      for(unsigned i=0;i<names.size();++i)add(tag.name+"/"+names[i],word(0xcc+i*4));
      add(tag.name+"/trade",word(0xf0));
    }else if(tag.group=="ctrl")add(tag.name+"/control",word(0x124));
    else if(tag.group=="bipd" || tag.group=="vehi") {
      if(tag.group=="vehi")add(tag.name+"/flip",word(0x21c));
      const Bytes block(cache.read_tag_bytes(tag.address+0x1c8,8));
      if(block.u32(0)>64)throw FormatError("Excessive action seat bank");
      for(unsigned i=0;i<block.u32(0);++i) {
        const Bytes seat(cache.read_tag_bytes(block.u32(4)+i*0xb0,0xb0));
        const auto key=tag.name+"/seat"+std::to_string(i);
        add(key+"/entry",seat.u32(0x84));add(key+"/grenade",seat.u32(0x14));add(key+"/melee",seat.u32(0x18));
      }
    }
  }
  if(include_locale)for(const auto &[sid,text]:hud.strings) {
    const auto name=cache.string_id(sid);if(!name.empty())result.emplace("@locale/"+name,Row{name,text});
  }
  return result;
}
}
int main(int argc,char **argv)try {
  if(argc!=5)throw FormatError("Supply MCC_MAPS ORIGINAL_MAPS DLC_MAPS REPORT");
  Json report{{"schema",1},{"authority","Original Xbox authored English action templates; exact MCC counterparts only"},
      {"maps",Json::array()},{"unpaired",Json::array()},{"unpaired_fields",Json::array()},{"templates",Json::array()}};
  std::map<std::pair<std::string,std::string>,std::set<std::string>> pairs;
  unsigned fields{},locale_pairs{};
  for(const auto &entry:std::filesystem::directory_iterator(argv[1])) {
    const auto name=entry.path().filename();
    if(entry.path().extension()!=".map" || name=="mainmenu.map" || name=="shared.map" || name=="single_player_shared.map")continue;
    auto original=std::filesystem::path(argv[2])/name;
    if(!std::filesystem::is_regular_file(original))original=std::filesystem::path(argv[3])/name;
    const bool paired=std::filesystem::is_regular_file(original);
    if(!paired)report["unpaired"].push_back(name.string());
    const auto reference=paired?rows(Cache(original),true):std::map<std::string,Row>{};
    const auto actual=rows(Cache(entry.path()));
    unsigned matched{};
    for(const auto &[key,row]:actual) {
      auto found=reference.find(key);
      if(found==reference.end() || found->second.name!=row.name) {
        found=reference.find("@locale/"+row.name);
        if(found!=reference.end())++locale_pairs;
      }
      if(found==reference.end() || found->second.name!=row.name) {
        report["unpaired_fields"].push_back({{"map",name.string()},{"field",key},{"name",row.name},{"text",row.text}});
        continue;
      }
      pairs[{row.text,found->second.text}].insert(row.name+" ("+key+")");++matched;++fields;
    }
    report["maps"].push_back({{"name",name.string()},{"original_sha256",paired?Json(hash(original)):Json(nullptr)},
        {"mcc_sha256",hash(entry.path())},{"matched_fields",matched}});
  }
  std::map<std::string,std::string> unique;
  for(const auto &[pair,names]:pairs) {
    const auto [at,inserted]=unique.emplace(pair.first,pair.second);
    if(!inserted && at->second!=pair.second)throw FormatError("MCC template has ambiguous original semantics");
    report["templates"].push_back({{"mcc",pair.first},{"original",pair.second},{"owners",names}});
  }
  report["matched_fields"]=fields;report["locale_name_pairs"]=locale_pairs;report["unique_templates"]=pairs.size();
  std::ofstream output(argv[4]);output<<report.dump(2)<<'\n';
  if(!output)throw FormatError("Cannot save action template audit");
  std::cout<<"PASS paired action templates: maps="<<report["maps"].size()<<" fields="<<fields
      <<" templates="<<pairs.size()<<" unpaired_maps="<<report["unpaired"].size()
      <<" unpaired_fields="<<report["unpaired_fields"].size()<<'\n';return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
