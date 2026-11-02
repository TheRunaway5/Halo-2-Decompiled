#include "halo2/source_sound_pio_native_tu15.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
using Json=nlohmann::json;struct Boundary{};
void need(bool v,const std::string& text){if(!v)throw std::runtime_error(text);}
unsigned word(std::span<const std::byte> raw){unsigned v{};for(unsigned i=0;i<raw.size();++i)v|=std::to_integer<unsigned>(raw[i])<<(i*8);return v;}
struct Memory final:SourceSoundPioActionMemory {
 const Json& row;std::map<unsigned,std::vector<std::byte>> banks;mutable unsigned fifo{};unsigned pc{};Json writes=Json::array();
 Memory(const Json& fixture,const Json& selected):row(selected){
  banks.emplace(fixture.at("bank"),std::vector<std::byte>(fixture.at("extent").get<unsigned>(),std::byte(row.at("fill").get<unsigned>())));
  banks.emplace(0x391390,std::vector<std::byte>(0x100));banks.emplace(0x38f988,std::vector<std::byte>(4));
  for(const auto& v:row.at("initial_memory"))put(v[0],v[1]);
 }
 std::span<std::byte> view(unsigned at,std::size_t n){auto e=banks.upper_bound(at);need(e!=banks.begin(),"Unknown native PIO memory");auto r=std::prev(e);auto offset=std::uint64_t(at)-r->first;need(offset+n<=r->second.size(),"Unknown PIO extent");return std::span(r->second).subspan(offset,n);}
 void put(unsigned at,unsigned value,unsigned bytes=4){auto raw=view(at,bytes);for(unsigned i=0;i<bytes;++i)raw[i]=std::byte(value>>(8*i));}
 void read(unsigned at,std::span<std::byte> out)const override{
  if(at>=0xfe800000){need(at==0xfe820010&&out.size()==4&&pc==0x38e0ce,"PIO action invented a device preimage read");need(fifo<row.at("fifo").size(),"Native PIO exceeded declared FIFO reads");auto value=row.at("fifo")[fifo++].get<unsigned>();for(unsigned i=0;i<4;++i)out[i]=std::byte(value>>(8*i));return;}
  auto raw=const_cast<Memory*>(this)->view(at,out.size());std::copy(raw.begin(),raw.end(),out.begin());
 }
 void write(unsigned at,std::span<const std::byte> bytes)override{need(at<0xfe800000,"PIO write bypassed action ownership");std::copy(bytes.begin(),bytes.end(),view(at,bytes.size()).begin());}
 void write_pio_action(unsigned site,unsigned at,std::span<const std::byte> bytes)override{need(site==pc&&bytes.size()==4,"PIO action lost its source site");writes.push_back({site,at,word(bytes)});}
};
void run(const Json& fixture,const Json& row,bool native){
 Memory memory(fixture,row),expected(fixture,row);SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp").get<decltype(abi.gp)>();abi.eflags=row.at("eflags");abi.fs_base=0x12345000;
 abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
 for(unsigned i=0;i<8;++i)for(unsigned j=0;j<4;++j)abi.xmm[i][j]=0x98760000+i*16+j;
 const auto original_xmm=abi.xmm;Json instructions=Json::array(),events=Json::array();std::map<std::pair<std::string,unsigned>,unsigned> occurrences;
 auto apply=[&](const char* kind,unsigned pc){const auto n=++occurrences[{kind,pc}];for(const auto& a:row.at("changes"))if(a.at("kind")==kind&&a.at("pc")==pc&&a.at("occurrence")==n)
  for(const auto& w:a.at("writes")){memory.put(w[0],w[1]);events.push_back({{"kind","input"},{"address",w[0]},{"value",w[1]}});}};
 SourcePhysicsWorldRuntimeServices services;std::map<unsigned,unsigned> visits;
 services.instruction=[&](unsigned pc){memory.pc=pc;const auto n=++visits[pc];const auto& stop=row.at("interrupt");if(!stop.is_null()&&stop[0]==pc&&stop[1]==n)throw Boundary{};instructions.push_back(pc);apply("instruction",pc);};
 auto observer=[&](const SourcePhysicsContactRecordStore& store){Json event={{"kind",store.address>=0xfe800000?"action":"store"},{"pc",store.instruction},{"address",store.address},{"bytes",store.written.size()},{"value",word(store.written)}};
  if(store.address<0xfe800000)event["before"]=word(store.before);else need(store.before.empty(),"PIO action manufactured a hardware preimage");events.push_back(event);apply("store",store.instruction);};
 bool returned=true;try{if(native)source_run_sound_pio_native_tu15(memory,row.at("entry"),abi,*abi.floating_point,services,observer);
  else source_run_sound_pio_storage_tu15(memory,row.at("entry"),abi,*abi.floating_point,services,observer);}catch(const Boundary&){returned=false;}
 const auto name=row.at("name").get<std::string>()+(native?" native":" retained literal");
 need(returned==row.at("returned")&&abi.gp==row.at("expected_gp").get<decltype(abi.gp)>(),name+": Original GP/return boundary differs");
 const auto mask=row.at("flags_mask").get<unsigned>();need((abi.eflags&mask)==(row.at("expected_eflags").get<unsigned>()&mask),name+": Original flags differ");
 need(instructions==row.at("instructions")&&events==row.at("events")&&memory.fifo==row.at("fifo_reads"),name+": Original instruction/store/input order differs");
 for(const auto& e:row.at("events"))if(e.at("kind")!="action"){
  if(e.at("kind")=="store")need(word(expected.view(e.at("address"),e.at("bytes")))==e.at("before"),"Original witness preimage inconsistent");
  expected.put(e.at("address"),e.at("value"),e.value("bytes",4u));
 }
 need(memory.banks==expected.banks,name+": Untouched source bytes differ");need(abi.xmm==original_xmm&&abi.fs_base==0x12345000&&abi.x87_depth==0&&abi.x87_status==0,name+": Integer owner changed unrelated ABI state");
 if(native){
  // Arbitrary opaque MMX aliases and optional FP provenance are untouched by
  // this integer owner even at its first interrupted source boundary.
  SourcePhysicsWorldRuntimeAbi opaque{};opaque.gp=row.at("gp").get<decltype(opaque.gp)>();opaque.eflags=row.at("eflags");opaque.x87_status=0x220;opaque.active_mmx.emplace();opaque.active_mmx->known_mask=0x5a;
  for(auto& r:opaque.active_mmx->physical)r.fill(std::byte{0xa5});
  auto before=opaque.active_mmx->physical;SourcePhysicsWorldRuntimeServices cancel;cancel.instruction=[](auto){throw Boundary{};};
  try{source_run_sound_pio_native_tu15(memory,row.at("entry"),opaque,{0x27f,0x1f80},cancel,{});}catch(const Boundary&){}
  need(!opaque.floating_point&&opaque.x87_status==0x220&&opaque.active_mmx&&opaque.active_mmx->known_mask==0x5a&&opaque.active_mmx->physical==before,"Native PIO touched opaque integer-unrelated FP/MMX state");
 }
}
}
int main(int argc,char** argv)try{
 need(argc==2,"Supply original native PIO fixture");const auto fixture=Json::parse(std::ifstream(argv[1]));need(fixture.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&fixture.at("original_instruction_patches").empty()&&fixture.at("substituted_callees").empty(),"Original PIO authority");
 unsigned cases{},returned{},interrupted{};for(const auto& row:fixture.at("cases")){run(fixture,row,true);run(fixture,row,false);++cases;if(row.at("returned").get<bool>())++returned;else++interrupted;}
 need(cases==36&&returned==24&&interrupted==12,"PIO matrix incomplete");
 std::cout<<"PASS "<<cases<<" unchanged original PIO cases through native and retained owners; "<<returned<<" natural returns, "<<interrupted
  <<" interruption prefixes; full GP/defined flags, ordered source/action stores, mutable reads, FIFO polling and opaque ABI preservation; no device/startup completion\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
