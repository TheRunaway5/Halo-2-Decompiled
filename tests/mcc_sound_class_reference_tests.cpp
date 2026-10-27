#include "halo2/mcc_sound_class_reference.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
unsigned checks{},negatives{};
void check(bool ok,const char*why){++checks;if(!ok)throw std::runtime_error(why);}
std::vector<std::byte> unhex(std::string_view s){check(s.size()%2==0,"odd fixture bytes");std::vector<std::byte>b;for(std::size_t n=0;n<s.size();n+=2)b.push_back(std::byte(std::stoul(std::string(s.substr(n,2)),nullptr,16)));return b;}
void put(std::vector<std::byte>&b,unsigned p,unsigned v){for(unsigned n=0;n<4;++n)b.at(p+n)=std::byte(v>>(n*8));}
Tag tag(const nlohmann::json&j){return {j.at("group"),j.at("name"),j.at("datum"),j.at("address"),j.at("size")};}
struct Region {unsigned address;std::vector<std::byte>bytes;};
struct CopyResetReader {
 std::function<std::span<const std::byte>(unsigned,std::size_t)>read;
 std::function<void()>copied;
 std::shared_ptr<bool>armed;
 CopyResetReader(decltype(read) r,decltype(copied) c,std::shared_ptr<bool> a)
     :read(std::move(r)),copied(std::move(c)),armed(std::move(a)){}
 CopyResetReader(const CopyResetReader& other)
     :read(other.read),copied(other.copied),armed(other.armed){if(*armed)copied();}
 std::span<const std::byte>operator()(unsigned p,std::size_t n)const{return read(p,n);}
};
struct Fixture {
 std::vector<Tag>tags;std::array<Region,4>regions;
 TagHandle legacy()const{return tags.at(1).handle();}
 MccSoundRuntimeSource source(){MccSoundRuntimeSource s;s.tags=tags;s.read=[this](unsigned p,std::size_t n)->std::span<const std::byte>{for(const auto&r:regions)if(p>=r.address&&std::uint64_t(p)+n<=std::uint64_t(r.address)+r.bytes.size())return std::span(r.bytes).subspan(p-r.address,n);throw FormatError("unowned fixture class read");};return s;}
 std::vector<std::byte> candidate()const{return project_mcc_sound_class_records(regions[3].bytes);}
};
void verify(const MccSoundClassReferenceProjection&p,std::span<const std::byte>before,std::span<const std::byte>original){
 check(p.status==MccSoundClassReferenceStatus::Converted&&p.changed_words==48&&p.restored_named_rows==25&&p.native_records.size()==54*92,"complete named class conversion was not admitted");
 check(p.unresolved_classes==std::array<std::uint8_t,6>{15,16,17,39,40,41},"unproved class slots lost explicit identity");
 unsigned changed{},rows{};for(unsigned i=0;i<54;++i){const auto actual=std::span(p.native_records).subspan(i*92,92),pre=before.subspan(i*92,92);if(std::ranges::find(p.unresolved_classes,i)!=p.unresolved_classes.end())check(std::ranges::equal(actual,pre),"unproved complete class row was changed");else check(std::ranges::equal(actual,original.subspan(i*92,92)),"proved corresponding class row differs from original");bool row=false;for(unsigned n=0;n<92;++n)if(actual[n]!=pre[n]){++changed;row=true;}rows+=row;}
 check(changed==70&&rows==25,"named class write footprint escaped70bytes/25rows");
 std::set<unsigned>positions;for(const auto&w:p.words){check(w.class_index<54&&w.offset%4==0&&w.offset+4<=92&&!w.class_name.empty()&&!w.field_name.empty()&&positions.insert(w.class_index*92+w.offset).second,"typed named class receipt is incomplete or overlapping");check(Bytes(before).u32(w.class_index*92+w.offset)==w.before_bits&&Bytes(p.native_records).u32(w.class_index*92+w.offset)==w.after_bits,"named class receipt is not the exact bit replacement");}
}
void positive(Fixture&f,std::span<const std::byte>original){const auto source_before=f.regions;const auto candidate=f.candidate();const auto copy=candidate;const auto p=project_mcc_sound_class_reference(f.source(),f.legacy(),candidate);verify(p,candidate,original);const auto again=project_mcc_sound_class_reference(f.source(),f.legacy(),p.native_records);check(again.status==MccSoundClassReferenceStatus::AlreadyEquivalent&&again.changed_words==0&&again.native_records==p.native_records,"whole sound-class postimage is not idempotent");check(candidate==copy,"pure class conversion changed candidate input");for(unsigned n=0;n<4;++n)check(f.regions[n].bytes==source_before[n].bytes,"pure class conversion changed raw source/extension/route");}
void rejected(Fixture f,const std::function<void(Fixture&)>&edit){edit(f);const auto before=f.regions;const auto candidate=f.candidate();try{const auto result=project_mcc_sound_class_reference(f.source(),f.legacy(),candidate);check(result.status==MccSoundClassReferenceStatus::UnrecognizedSource&&result.native_records.empty()&&result.changed_words==0,"custom class source was canonically overwritten");}catch(const FormatError&){}for(unsigned n=0;n<4;++n)check(f.regions[n].bytes==before[n].bytes,"rejected class source was changed");++negatives;}
}
int main(int argc,char**argv)try{
 const auto fixture_path=std::filesystem::path(__FILE__).parent_path()/"fixtures/mcc_sound_class_reference.json";std::ifstream in(fixture_path);check(bool(in),"class fixture missing");const auto fixture=nlohmann::json::parse(in);check(fixture.at("schema")=="H2_MCC_SOUND_CLASS_REFERENCE_FIXTURE_V1","foreign fixture");const auto&c=fixture.at("context");const auto original=unhex(fixture.at("original_native_rows").get<std::string>());Fixture f;f.tags={tag(c.at("globals")),tag(c.at("legacy"))};const auto block=unhex(c.at("sound_block").get<std::string>()),root=unhex(c.at("class_root").get<std::string>());f.regions={Region{f.tags[0].address+0xc0,block},Region{Bytes(block).u32(4),unhex(c.at("sound_globals").get<std::string>())},Region{f.tags[1].address,root},Region{Bytes(root).u32(4),unhex(c.at("class_rows").get<std::string>())}};positive(f,original);
 for(auto member:{&Tag::group,&Tag::name})rejected(f,[=](auto&x){x.tags[1].*member="custom";});
 rejected(f,[](auto&x){x.tags[0].name="custom\\globals";});rejected(f,[](auto&x){x.tags[1].datum^=0x10000u;put(x.regions[1].bytes,12,x.tags[1].datum);});rejected(f,[](auto&x){x.tags[1].size-=4;});rejected(f,[](auto&x){x.tags.push_back(x.tags[1]);});rejected(f,[](auto&x){auto alias=x.tags[1];alias.datum+=0x10000u;x.tags.push_back(alias);});rejected(f,[](auto&x){auto alias=x.tags[1];alias.datum+=3;alias.address+=8;alias.size=8;x.tags.push_back(alias);});rejected(f,[](auto&x){auto alias=x.tags[1];alias.datum+=3;alias.address-=8;alias.size=16;x.tags.push_back(alias);});rejected(f,[](auto&x){auto g=x.tags[0];g.datum+=3;g.address+=64;x.tags.push_back(g);});
 for(unsigned offset:{0u,4u})rejected(f,[=](auto&x){x.regions[0].bytes[offset]^=std::byte{1};});
 for(unsigned offset:{0u,8u,12u,16u,40u})rejected(f,[=](auto&x){x.regions[1].bytes[offset]^=std::byte{1};});
 for(unsigned offset:{0u,4u})rejected(f,[=](auto&x){x.regions[2].bytes[offset]^=std::byte{1};});
 for(unsigned offset:{0u,32u,92u,119u,15u*120u,53u*120u+119u})rejected(f,[=](auto&x){x.regions[3].bytes[offset]^=std::byte{1};});
 auto candidate=f.candidate();const auto expected=project_mcc_sound_class_reference(f.source(),f.legacy(),candidate);for(unsigned at:{0u,32u,15u*92u,53u*92u+91u}){auto custom=candidate;custom[at]^=std::byte{1};const auto result=project_mcc_sound_class_reference(f.source(),f.legacy(),custom);check(result.status==MccSoundClassReferenceStatus::CandidateMismatch&&result.native_records.empty(),"partial/custom class candidate admitted");++negatives;}auto partial=candidate;const auto&w=expected.words[0];put(partial,w.class_index*92+w.offset,w.after_bits);check(project_mcc_sound_class_reference(f.source(),f.legacy(),partial).status==MccSoundClassReferenceStatus::CandidateMismatch,"partial previously restored class bank admitted");++negatives;
 for(auto n:{0u,54u*92u-1,54u*92u+1}){auto short_candidate=candidate;short_candidate.resize(n);const auto result=project_mcc_sound_class_reference(f.source(),f.legacy(),short_candidate);check(result.status==MccSoundClassReferenceStatus::CandidateMismatch&&result.native_records.empty(),"wrong complete class candidate extent admitted");++negatives;}
 auto callback=f.source();callback.read=[&](unsigned p,std::size_t n){return f.source().read(p,n).first(n-1);};bool short_read{};try{(void)project_mcc_sound_class_reference(callback,f.legacy(),candidate);}catch(const FormatError&){short_read=true;}check(short_read,"partial raw class callback admitted");++negatives;
 // The raw Cache views remain stable while the caller deliberately releases
 // its callable wrapper, namespace strings and candidate inside the callback.
 auto reset_source=f.source();auto reset_tags=f.tags;auto reset_candidate=f.candidate();
 reset_source.tags=reset_tags;const auto retained_read=reset_source.read;unsigned reset_reads{};
 reset_source.read=[&,retained_read](unsigned p,std::size_t n){
  reset_source={};reset_tags.clear();reset_tags.shrink_to_fit();
  reset_candidate.clear();reset_candidate.shrink_to_fit();++reset_reads;
  return retained_read(p,n);
 };
 const auto reset_projection=project_mcc_sound_class_reference(reset_source,f.legacy(),reset_candidate);
 verify(reset_projection,candidate,original);
 check(reset_reads==4&&!reset_source.read&&reset_tags.empty()&&reset_candidate.empty(),
       "class projection did not retain callback/namespace/candidate ownership");
 auto copy_source=f.source();auto copy_tags=f.tags;auto copy_candidate=f.candidate();
 copy_source.tags=copy_tags;auto armed=std::make_shared<bool>(false);unsigned copies{},copy_reads{};
 copy_source.read=CopyResetReader{
  [&,retained_read](unsigned p,std::size_t n){++copy_reads;return retained_read(p,n);},
  [&]{*armed=false;++copies;copy_tags.clear();copy_tags.shrink_to_fit();
      copy_candidate.clear();copy_candidate.shrink_to_fit();},armed};
 *armed=true;
 const auto copy_projection=project_mcc_sound_class_reference(copy_source,f.legacy(),copy_candidate);
 verify(copy_projection,candidate,original);
 check(copies==1&&copy_reads==4&&copy_source.read&&copy_tags.empty()&&copy_candidate.empty(),
       "class projection copied a callable before owning namespace/candidate");
 if(argc!=1){check(argc==3||argc==4,"usage: class-reference-test [--mcc MCC_MAP | --paired MCC_MAP ORIGINAL_MAP]");if(!*argv[2]||!std::filesystem::exists(argv[2])||(argc==4&&(!*argv[3]||!std::filesystem::exists(argv[3]))))return 77;check(std::string_view(argv[1])==(argc==3?"--mcc":"--paired"),"unknown class test mode");Cache cache(argv[2]);check(cache.is_mcc(),"actual donor is not MCC");const auto*g=cache.find_tag(cache.globals_tag(),tag_groups::globals);check(g!=nullptr,"actual globals absent");const auto sg=Bytes(cache.read_tag_bytes(g->address+0xc0,8)).u32(4);const auto handle=TagHandle{Bytes(cache.read_tag_bytes(sg,44)).u32(12)};const auto*owner=cache.find_tag(handle,TagGroup{"sncl"});check(owner!=nullptr,"actual Legacy source absent");const auto r=cache.read_tag_bytes(owner->address,8);const auto bank=cache.read_tag_bytes(Bytes(r).u32(4),54*120);const auto source_copy=std::vector(bank.begin(),bank.end());const auto pre=project_mcc_sound_class_records(bank);MccSoundRuntimeSource s;s.tags=cache.tags();s.read=[&](auto p,auto n){return cache.read_tag_bytes(p,n);};const auto actual=project_mcc_sound_class_reference(s,handle,pre);
 std::vector<std::byte>reference=original;if(argc==4){Cache reference_cache(argv[3]);check(!reference_cache.is_mcc(),"reference is not original");const Tag*original_tag{};for(const auto&t:reference_cache.tags())if(t.group=="sncl"&&t.address&&t.name=="sound\\sound_classes"){check(!original_tag,"ambiguous original class owner");original_tag=&t;}check(original_tag!=nullptr,"original classes absent");const auto rr=Bytes(reference_cache.read_tag_bytes(original_tag->address,8));check(rr.u32(0)==54,"original class count changed");const auto bytes=reference_cache.read_tag_bytes(rr.u32(4),54*92);reference.assign(bytes.begin(),bytes.end());check(reference==original,"original class authoring variant differs from proved fixture");}
 verify(actual,pre,reference);const auto again=project_mcc_sound_class_reference(s,handle,actual.native_records);check(again.status==MccSoundClassReferenceStatus::AlreadyEquivalent&&again.native_records==actual.native_records,"actual class result not idempotent");check(std::ranges::equal(bank,source_copy),"actual raw class/source extensions changed");
 // Reconstruct this actual complete Cache namespace/route in owned fixture
 // regions; coherent pointer/identity edits must remain unrecognized.
 Fixture published;published.tags={*g,*owner};
 for(const auto&t:cache.tags())if(t.handle()!=g->handle()&&t.handle()!=owner->handle())published.tags.push_back(t);
 const auto descriptor=cache.read_tag_bytes(g->address+0xc0,8);
 const auto route=cache.read_tag_bytes(sg,44);
 published.regions={Region{g->address+0xc0,{descriptor.begin(),descriptor.end()}},
   Region{sg,{route.begin(),route.end()}},Region{owner->address,{r.begin(),r.end()}},
   Region{Bytes(r).u32(4),source_copy}};
 positive(published,reference);
 rejected(published,[](auto&x){x.tags[0].address+=16;x.regions[0].address+=16;});
 rejected(published,[](auto&x){put(x.regions[0].bytes,4,x.regions[1].address+16);x.regions[1].address+=16;});
 rejected(published,[](auto&x){x.tags[1].datum^=0x10000u;put(x.regions[1].bytes,12,x.tags[1].datum);});
 rejected(published,[](auto&x){put(x.regions[2].bytes,4,x.regions[3].address+4);x.regions[3].address+=4;});
 rejected(published,[](auto&x){x.regions[3].bytes[15*120+119]^=std::byte{1};});
 rejected(published,[](auto&x){x.tags[0].size-=16;});
 std::cout<<"actual map="<<cache.name()<<" source_sha256="<<actual.source_bank_sha256<<" ";}
 std::cout<<"PASS "<<checks<<" checks/"<<negatives<<" negatives;48words25namedrows70bytes; six complete MCC rows unresolved/untouched; no caller/source writes or reference runtime dependency\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
