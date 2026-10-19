#include "halo2/cache.hpp"
#include "halo2/mcc_cpu_bitmap_projection.hpp"
#include "halo2/native_tu15_world_memory.hpp"
#include "halo2/source_model_render_tu15.hpp"
#include "halo2/source_combat_action_tu15.hpp"
#include "halo2/source_campaign_activation_tu15.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
#include "fixtures/mcc_opening_dxt1_samples.original.inc"
#include "fixtures/mcc_opening_raw_samples.original.inc"
#include "fixtures/mcc_opening_contrail_noise.original.inc"
void check(bool b,const char*m){if(!b)throw FormatError(m);}
void put(SourceRuntimeMemory&m,unsigned at,unsigned v){std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(v>>(n*8));m.write(at,b);}
// Observe the same single first-row read as the inline caller. Never write a
// projected pointer into the canonical MCC descriptor.
struct FirstRow:SourcePhysicsContactMemory {
 SourceRuntimeMemory&bank;unsigned descriptor,row;mutable unsigned reads{};
 FirstRow(SourceRuntimeMemory&m,unsigned d,unsigned r):bank(m),descriptor(d),row(r){}
 void read(unsigned at,std::span<std::byte>b)const override{if(at==descriptor&&b.size()==4){++reads;for(unsigned i=0;i<4;++i)b[i]=std::byte(row>>(i*8));}else bank.read(at,b);}
 void write(unsigned at,std::span<const std::byte>b)override{check(at!=descriptor,"Original bitmap caller modified canonical descriptor");bank.write(at,b);}
};
}
int main(int argc,char**argv)try {
 if(argc<2 || !std::filesystem::is_regular_file(argv[1]))return 77;
 Cache cache(argv[1],std::filesystem::path(argv[1]).parent_path());check(cache.is_mcc()&&cache.name()=="00a_introduction","Requires actual MCC opening");
 const auto*t=cache.query_tag({.group=tag_groups::bitmap,.name="scenarios\\bitmaps\\covenant\\military\\floors\\cov_rug",.require_address=true});check(t,"Actual MCC rug bitmap absent");
 const auto literal=cache.read_tag_bytes(t->address,t->size);const Bytes root(literal);const auto raw=cache.read_tag_bytes(root.u32(0x48),168);const Bytes row(raw);
 check(root.u32(0x44)==1 && row.u16(12)==14 && row.u16(4)==512 && row.u16(6)==128 && row.u16(14)==515,"Actual MCC opening DXT1 tuple changed");
 const auto resource=cache.load_bitmap_resource(t->datum,0);const auto texture=cache.load_bitmap(t->datum,0);
 check(texture.format==14 && texture.pixels.size()<=resource.levels[0].size()&&std::equal(texture.pixels.begin(),texture.pixels.end(),resource.levels[0].begin()),"DXT1 encoded source/texture differs");
 auto bank=native_tu15_world_memory({0x60212000u,{},"MCC bitmap test clock"});
 const auto canonical=MccRuntimeCacheDelivery::plan(cache,0x90010000u,256u*1024*1024);const auto image=canonical.tag_image();
 for(const auto&r:image.regions)bank.mount(r.address,r.bytes,SourceRuntimeMemory::Access::ReadOnly,"actual canonical MCC metadata image");
 put(bank,0x4f0414u,image.table);put(bank,0x4f2d58u,123);
 private_cpu_bitmap::WorldProjection projection(cache,bank,canonical);
 const auto canonical_root=canonical.runtime_address(t->address,t->size);
 const auto original_root_view=bank.view(canonical_root,t->size);
 const std::vector<std::byte> original_root(original_root_view.begin(),original_root_view.end());
 const auto projected=projection.first_row_for_canonical_root(t->handle(),canonical_root);
 check(std::ranges::equal(bank.view(projected,24),raw.first(24)),"DXT1 projection changed literal common metadata");
 const auto word=[&](unsigned offset){return Bytes(bank.view(projected+offset,4)).u32(0);};
 const auto header=word(0x50),encoded=word(0x54);
 check(word(0x34)==resource.levels[0].size()&&std::ranges::equal(bank.view(encoded,resource.levels[0].size()),resource.levels[0]),"DXT1 CPU delivery changed encoded bytes");
 bank.mount(0x62000000u,std::vector<std::byte>(4096),SourceRuntimeMemory::Access::ReadWrite,"actual bitmap test caller stack and color");
 nlohmann::json cases=nlohmann::json::array();
 for(unsigned flags:{0u,2u,0xffff0000u,0xffff0002u})for(const auto pixel:std::array<std::array<unsigned,2>,8>{{{0,0},{1,1},{511,127},{256,64},{10,11},{23,32},{60,72},{100,110}}}) {
  const auto x=std::bit_cast<unsigned>((pixel[0]+.5f)/512),y=std::bit_cast<unsigned>((pixel[1]+.5f)/128);
  put(bank,0x62000800u,0x63000000u);put(bank,0x62000804u,flags);put(bank,0x62000808u,x);put(bank,0x6200080cu,y);
  SourcePhysicsWorldRuntimeAbi abi{};abi.gp={t->datum,0x23456789u,0x3456789au,0x456789abu,0x62000800u,0x56789abcu,0x6789abcdu,0x62000c00u};abi.eflags=0x202;
  FirstRow memory(bank,canonical_root+0x48u,projected);SourcePhysicsWorldRuntimeServices services;unsigned constructors{};
  services.required_owner=[&](auto&m,const auto&c,auto&a,const auto&o){check(c.instruction==0x1d2eau&&c.target==0x407c00u&&!c.tail,"Unexpected bitmap constructor dependency");++constructors;source_native_cache_bitmap_view_tu15(m,a,{0x27f,0x1f80},services,o);};
  source_run_model_render_tu15(memory,0xd5610u,abi,{0x27f,0x1f80},services,{});
  check(memory.reads==1&&constructors==1&&(abi.gp[0]&255)==1&&abi.gp[4]==0x62000810u,"Complete original DXT1 caller/constructor did not return");
  std::vector<unsigned> color;const Bytes out(bank.view(0x62000c00u,12));for(unsigned i=0;i<3;++i)color.push_back(out.u32(i*4));
  check(std::equal(color.begin(),color.end(),original_dxt1_colors[cases.size()]),"Native DXT1 color differs from complete original constructor/sample");
  cases.push_back({{"flags",flags},{"pixel",pixel},{"x",x},{"y",y},{"color_bits",color}});
 }
 check(std::ranges::equal(bank.view(canonical_root,t->size),original_root),"Canonical bitmap metadata mutated");
 check(Bytes(bank.view(canonical_root,0x4c)).u32(0x48)==canonical.runtime_address(root.u32(0x48),168),"Canonical first-row pointer mutated");
 // The real new-map callbacks must produce these working values. Merely
 // mapping zeroes would avoid the missing read but change contrail behavior.
 bank.mount(0x62002000u,std::vector<std::byte>(512),SourceRuntimeMemory::Access::ReadWrite,"declared callback scalar and RNG inputs");
 put(bank,0x52be68u,0x62002000u);put(bank,0x62002004u,0x3f800000u);put(bank,0x62002008u,0x3a902de0u);
 put(bank,0x4f3cd8u,0x62002100u);put(bank,0x62002100u,0x98765432u);
 for(unsigned entry:{0x219300u,0x11e550u}) {
  put(bank,0x62000800u,0x63000000u);SourcePhysicsWorldRuntimeAbi abi{};abi.gp[4]=0x62000800u;abi.eflags=0x202;
  source_run_campaign_activation_tu15(bank,entry,abi,{0x27f,0x1f80},{});
  check(abi.gp[4]==0x62000804u,"Original contrail activation callback did not return");
 }
 check(Bytes(bank.view(0x505e84u,4)).u32(0)==0x47e7aaacu&&Bytes(bank.view(0x505e88u,4)).u32(0)==0x43027984u,
     "Original contrail scalar callback differs from complete original execution");
 check(std::ranges::equal(bank.view(0x505e90u,0xd0c),std::as_bytes(std::span(original_contrail_noise)))&&
     Bytes(bank.view(0x62002100u,4)).u32(0)==0x2beddceau,
     "Original contrail noise callback/RNG differs from complete original execution");
 const auto live=bank.checkpoint(),filtered=projection.checkpoint_without_resources();
 auto expected=live;
 std::erase_if(expected.regions,[&](const auto&r){return r.address==projected || r.address==header || r.address==encoded;});
 check(expected.regions.size()+3==live.regions.size()&&expected==filtered&&bank.checkpoint()==live,
     "DXT1 checkpoint changed live memory or omitted unrelated owners");
 check(projection.matches_read_only_checkpoint_without_resources(filtered)&&projection.matches_read_only_checkpoint_for_restore(filtered),
     "DXT1 filtered checkpoint lost its complete original ownership");
 auto foreign=filtered;const auto immutable=std::find_if(foreign.regions.begin(),foreign.regions.end(),
     [](const auto&r){return r.access==SourceRuntimeMemory::Access::ReadOnly&&!r.bytes.empty();});
 check(immutable!=foreign.regions.end(),"No immutable checkpoint control");
 immutable->bytes[0]^=std::byte{1};
 check(!projection.matches_read_only_checkpoint_without_resources(foreign)&&!projection.matches_read_only_checkpoint_for_restore(foreign),
     "DXT1 checkpoint admitted changed immutable backing");
 unsigned raw_kind{};
 for(const char*name:{"rubber_trim_a_dif","rubber_trim_a_bump"}) {
  const auto* raw_tag=cache.query_tag({.group=tag_groups::bitmap,.name=std::string("scenarios\\bitmaps\\covenant\\military\\floors\\")+name,.require_address=true});
  check(raw_tag,"Actual MCC scene01b raw bitmap absent");
  const auto raw_root=cache.read_tag_bytes(raw_tag->address,raw_tag->size);
  const auto raw_row=cache.read_tag_bytes(Bytes(raw_root).u32(0x48),168);
  check(Bytes(raw_row).u16(12)==10u+raw_kind && Bytes(raw_row).u16(4)==256 && Bytes(raw_row).u16(6)==128,
      "Actual raw CPU bitmap tuple changed");
  const auto raw_root_va=canonical.runtime_address(raw_tag->address,raw_tag->size);
  const auto raw_projected=projection.first_row_for_canonical_root(raw_tag->handle(),raw_root_va);
  check(std::ranges::equal(bank.view(raw_projected,24),raw_row.first(24)),"Raw CPU bitmap changed literal common metadata");
  const auto raw_resource=cache.load_bitmap_resource(raw_tag->datum,0);
  const auto raw_encoded=Bytes(bank.view(raw_projected+0x54u,4)).u32(0);
  check(Bytes(bank.view(raw_projected+0x34u,4)).u32(0)==raw_resource.levels[0].size()&&
      std::ranges::equal(bank.view(raw_encoded,raw_resource.levels[0].size()),raw_resource.levels[0]),
      "Raw CPU bitmap changed encoded bytes or authored mip chain");
  unsigned sample{};
  for(unsigned flags:{0u,2u,0xffff0000u,0xffff0002u})for(const auto pixel:std::array<std::array<unsigned,2>,8>{{{0,0},{1,1},{255,127},{128,64},{10,11},{23,32},{60,72},{100,110}}}) {
   put(bank,0x62000800u,0x63000000u);put(bank,0x62000804u,flags);
   put(bank,0x62000808u,std::bit_cast<unsigned>((pixel[0]+.5f)/256));put(bank,0x6200080cu,std::bit_cast<unsigned>((pixel[1]+.5f)/128));
   SourcePhysicsWorldRuntimeAbi abi{};abi.gp={raw_tag->datum,0x23456789u,0x3456789au,0x456789abu,0x62000800u,0x56789abcu,0x6789abcdu,0x62000c00u};abi.eflags=0x202;
   FirstRow memory(bank,raw_root_va+0x48u,raw_projected);SourcePhysicsWorldRuntimeServices services;unsigned constructors{};
   services.required_owner=[&](auto&m,const auto&c,auto&a,const auto&o){check(c.instruction==0x1d2eau&&c.target==0x407c00u&&!c.tail,"Unexpected raw bitmap constructor dependency");++constructors;source_native_cache_bitmap_view_tu15(m,a,{0x27f,0x1f80},services,o);};
   source_run_model_render_tu15(memory,0xd5610u,abi,{0x27f,0x1f80},services,{});
   check(memory.reads==1&&constructors==1&&(abi.gp[0]&255)==1&&abi.gp[4]==0x62000810u,"Complete original raw caller/constructor did not return");
   const Bytes color(bank.view(0x62000c00u,12));for(unsigned lane=0;lane<3;++lane)
    check(color.u32(lane*4)==original_raw_colors[raw_kind][sample][lane],"Raw bitmap color differs from complete original sample");
   ++sample;
  }
  check(Bytes(bank.view(raw_root_va,0x4c)).u32(0x48)==canonical.runtime_address(Bytes(raw_root).u32(0x48),168),"Raw CPU bitmap changed canonical descriptor");
  ++raw_kind;
 }
 const auto raw_filtered=projection.checkpoint_without_resources();
 check(projection.matches_read_only_checkpoint_without_resources(raw_filtered),"Raw CPU bitmap checkpoint lost ownership");
 if(argc==3){const std::filesystem::path out(argv[2]);std::filesystem::create_directories(out);const auto save=[&](const char*name,auto bytes){std::ofstream f(out/name,std::ios::binary);f.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());};save("common.bin",raw.first(24));save("encoded.bin",resource.levels[0]);save("header.bin",bank.view(header,20));std::ofstream(out/"samples.json")<<cases.dump(2);}
 std::cout<<nlohmann::json{{"passed",true},{"actual_bitmap",t->name},{"format",14},{"complete_native_D5610_samples",cases.size()},{"literal_source_resources_preserved",true}}.dump()<<'\n';
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
