#include "halo2/audio.hpp"
#include "halo2/original_sound_runtime_bank_tu15.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
using namespace halo2;
namespace {
using J=nlohmann::json;
void need(bool value,const char* message){if(!value)throw FormatError(message);}
std::string hash(std::span<const std::byte> data){std::array<unsigned char,32> bytes{};unsigned n{};
 need(EVP_Digest(data.data(),data.size(),bytes.data(),&n,EVP_sha256(),nullptr)==1&&n==32,"Chunk digest failed");
 constexpr char digits[]="0123456789abcdef";std::string out;for(auto c:bytes){out+=digits[c>>4];out+=digits[c&15];}return out;}
bool topology(const EncodedSound& e,const PreparedOriginalSoundBank::Permutation& p){
 if(e.packet_chunks.size()!=p.chunks.size())return false;std::size_t bytes{};
 for(unsigned n=0;n<p.chunks.size();++n){const auto& c=e.packet_chunks[n];if(c.bytes!=p.chunks[n].bytes||c.pcm_bytes||c.reset_after)return false;bytes+=c.bytes;}
 return bytes==e.data.size();
}
}
int main(int argc,char**argv)try{
 if(argc!=4)throw FormatError("Supply prepared original catalog, optional prior PCM report and output report");
 if(!*argv[1])return 77;
 need(std::getenv("HALO2_TEST_SCRATCH"),"Use the owned test runner");
 const auto catalog=J::parse(std::ifstream(argv[1]));J reference=J::object();
 if(*argv[2])reference=J::parse(std::ifstream(argv[2])).at("pcm");
 J report{{"passed",false},{"pcm",J::object()},{"scope","Authenticated original banks and declared native table/string component inputs; actual decoder PCM only, no source voice/device/gameplay admission"}};
 unsigned contexts{},routes{},prepared_bad{},native_bad{},multi{},chunks{};std::set<std::string> checked;
 for(const auto& row:catalog.at("cases")){
  const auto bank=PreparedOriginalSoundBank::load(row.at("directory").get<std::string>(),row.at("sha256").get<std::string>());const auto& m=bank->metadata();
  auto names=std::make_shared<std::map<std::uint32_t,std::string>>();
  for(const auto& range:m.pitch_ranges){names->emplace(*range.import_name_id,range.name);for(const auto& p:range.permutations)names->emplace(*p.import_name_id,p.name);}
  const OriginalSoundRuntimeStringsTu15 strings{names,[names](auto id,auto text){need(names->at(id)==text,"Declared imported StringID differs");return id;},[names](auto id){return names->at(id);}};
  Tag tag;tag.group="snd!";tag.name=m.name;tag.datum=m.origin->sound.value;tag.address=0x20000000;tag.size=20;
  auto tags=std::make_shared<const std::vector<Tag>>(std::vector{tag});const std::array bindings{OriginalSoundRuntimeBindingTu15{m.origin->sound,bank}};
  const auto native=OriginalSoundRuntimeBankTu15::create(0x31000000,8*1024*1024,tags,bindings,strings);
  ++contexts;
  for(unsigned pitch=0;pitch<m.pitch_ranges.size();++pitch)for(unsigned p=0;p<m.pitch_ranges[pitch].permutations.size();++p){
   const auto& meta=m.pitch_ranges[pitch].permutations[p];const auto& encoded=bank->encoded(pitch,p);const auto copy=native->encoded(m.origin->sound,pitch,p);
   ++routes;if(!topology(encoded,meta))++prepared_bad;if(!topology(copy,meta))++native_bad;
   need(encoded.data==copy.data,"Native chunk route changed original encoded payload");
   const auto key=m.name+"|"+std::to_string(pitch)+"|"+std::to_string(p)+"|"+hash(encoded.data);
   if(!checked.insert(key).second)continue;
   const auto pcm=decode_sound(encoded);const auto digest=hash(std::as_bytes(std::span(pcm.stereo)));
   if(!reference.empty()){need(reference.contains(key),"Prior PCM baseline omitted an original permutation");
    need(reference.at(key).at("sha256")==digest&&reference.at(key).at("frames")==pcm.frames(),"Chunk handling changed previously decoded PCM");}
   need(pcm.chunk_boundaries.size()==meta.chunks.size(),"Decoder omitted original resource boundaries");
   J part=J::array(),boundaries=J::array();std::size_t end{},frames{};
   for(unsigned n=0;n<meta.chunks.size();++n){const auto& c=meta.chunks[n];part.push_back(c.bytes);end+=c.bytes;
    const auto& boundary=pcm.chunk_boundaries[n];
    need(boundary.encoded_end==end&&boundary.end_frame>=frames&&boundary.end_frame<=pcm.frames(),"Decoder chunk map escapes input or emitted frames");
    frames=boundary.end_frame;boundaries.push_back({boundary.encoded_end,boundary.end_frame});}
   need(frames==pcm.frames(),"Decoder final chunk lost resampler flush frames");
   report["pcm"][key]={{"sha256",digest},{"frames",pcm.frames()},{"source_chunks",part},{"chunk_boundaries",boundaries}};
   if(meta.chunks.size()>1)++multi;chunks+=meta.chunks.size();
  }
 }
 need(contexts==908&&routes==9450&&checked.size()==273,"Original chunk cohort is incomplete");
 report.update({{"passed",prepared_bad==0&&native_bad==0},{"contexts",contexts},{"routes",routes},{"unique_permutations",checked.size()},
 {"multi_chunk_permutations",multi},{"unique_chunks",chunks},{"prepared_topology_failures",prepared_bad},{"native_topology_failures",native_bad},
 {"baseline_pcm_checked",!reference.empty()},{"native_world_or_device_verified",false}});
 std::ofstream(argv[3])<<report.dump(2)<<'\n';std::cout<<"Original chunk topology: contexts="<<contexts<<" routes="<<routes<<" prepared_failures="<<prepared_bad<<" native_failures="<<native_bad<<'\n';
 return report.at("passed").get<bool>()?0:1;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
