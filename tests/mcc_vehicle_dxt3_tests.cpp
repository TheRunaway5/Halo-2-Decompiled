#include "halo2/cache.hpp"
#include "halo2/original_data.hpp"
#include "halo2/mcc_cpu_bitmap_projection.hpp"
#include "halo2/native_tu15_world_memory.hpp"
#include "halo2/source_model_render_tu15.hpp"
#include "halo2/source_combat_action_tu15.hpp"
#include "halo2/source_bitmap_pending_header.hpp"
#include <algorithm>
#include <bit>
#include <iostream>
using namespace halo2;
namespace {
#include "fixtures/mcc_vehicle_dxt3.original.inc"
void check(bool ok,const char* message){if(!ok)throw FormatError(message);}
void put(SourceRuntimeMemory& m,unsigned at,unsigned v){std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(v>>(n*8));m.write(at,b);}
struct FirstRow:SourcePhysicsContactMemory {
 SourceRuntimeMemory& bank;unsigned descriptor,row;mutable unsigned reads{};
 FirstRow(SourceRuntimeMemory& m,unsigned d,unsigned r):bank(m),descriptor(d),row(r){}
 void read(unsigned at,std::span<std::byte>b)const override{if(at==descriptor&&b.size()==4){++reads;for(unsigned n=0;n<4;++n)b[n]=std::byte(row>>(n*8));}else bank.read(at,b);}
 void write(unsigned at,std::span<const std::byte>b)override{check(at!=descriptor,"DXT3 caller modified canonical descriptor");bank.write(at,b);}
};
}
int main(int argc,char** argv)try{
 if(argc!=2||!std::filesystem::is_regular_file(argv[1]))return 77;
 const std::filesystem::path path=argv[1];Cache cache(path,path.parent_path(),OriginalData::native_tu15());
 auto bank=native_tu15_world_memory({0x60212000u,{},"MCC DXT3 original-sample regression clock"});
 const auto canonical=MccRuntimeCacheDelivery::plan(cache,0x90000000u,0x60000000u);
 const auto image=canonical.tag_image();for(const auto&r:image.regions)bank.mount(r.address,r.bytes,SourceRuntimeMemory::Access::ReadOnly,"actual MCC DXT3 metadata");
 put(bank,0x4f0414u,image.table);bank.mount(0x70000000u,std::array<std::byte,4096>{},SourceRuntimeMemory::Access::ReadWrite,"DXT3 caller stack/color");
 private_cpu_bitmap::WorldProjection projection(cache,bank,canonical);unsigned samples{};
 for(unsigned selection=0;selection<2;++selection){
  const char* name=selection?"objects\\vehicles\\warthog\\bitmaps\\antenna":"objects\\characters\\masterchief\\bitmaps\\masterchief";
  const auto* tag=cache.query_tag({.group=tag_groups::bitmap,.name=name,.require_address=true});check(tag,"Actual DXT3 bitmap absent");
  const Bytes source(cache.read_tag_bytes(tag->address,tag->size));check(source.u32(0x44)==1,"Actual DXT3 frame count changed");
  const auto raw=cache.read_tag_bytes(source.u32(0x48),168);const Bytes row(raw);check(row.u16(12)==15&&row.u16(14)==515,"Actual proved DXT3 tuple changed");
  const auto root=canonical.runtime_address(tag->address,tag->size),descriptor=Bytes(bank.view(root+0x48u,4)).u32(0);
  const auto common=bank.view(root,tag->size);const std::vector<std::byte> before(common.begin(),common.end());
  SourceBitmapPendingHeaderMetadata metadata{std::bit_cast<std::int16_t>(row.u16(4)),std::bit_cast<std::int16_t>(row.u16(6)),15,std::bit_cast<std::int16_t>(row.u16(20)),std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(raw[8])),row.u16(10),row.u16(14)};
  const auto expected_header=selection?original_dxt3_antenna_header:original_dxt3_masterchief_header;
  const auto header=source_bitmap_pending_header(metadata,0,0x81200000u);check(std::equal(header.begin(),header.end(),expected_header),"DXT3 header differs from complete original TU1.5 1301E0");
  const auto selected=projection.first_row_for_canonical_root(tag->handle(),root);check(std::ranges::equal(bank.view(selected,24),raw.first(24)),"DXT3 source metadata changed");
  const auto expected_colors=selection?original_dxt3_antenna_colors:original_dxt3_masterchief_colors;
  const unsigned width=row.u16(4),height=row.u16(6);const std::array<std::array<unsigned,2>,8> pixels{{{0,0},{1,1},{width-1,height-1},{width/2,height/2},{10,11},{23,32},{width/3,height/3},{width-2,height-2}}};
  unsigned index{};
  for(unsigned flags:{0u,2u,0xffff0000u,0xffff0002u})for(const auto pixel:pixels){
   put(bank,0x70000800u,0x63000000u);put(bank,0x70000804u,flags);put(bank,0x70000808u,std::bit_cast<unsigned>((pixel[0]+.5f)/width));put(bank,0x7000080cu,std::bit_cast<unsigned>((pixel[1]+.5f)/height));
   SourcePhysicsWorldRuntimeAbi abi{};abi.gp={tag->datum,0x23456789u,0x3456789au,0x456789abu,0x70000800u,0x56789abcu,0x6789abcdu,0x70000c00u};abi.eflags=0x202;
   FirstRow memory(bank,root+0x48u,selected);SourcePhysicsWorldRuntimeServices services;unsigned constructors{};
   services.required_owner=[&](auto&m,const auto&c,auto&a,const auto&o){check(c.instruction==0x1d2eau&&c.target==0x407c00u&&!c.tail,"Unexpected DXT3 metadata dependency");++constructors;source_native_cache_bitmap_view_tu15(m,a,{0x27f,0x1f80},services,o);};
   source_run_model_render_tu15(memory,0xd5610u,abi,{0x27f,0x1f80},services,{});
   check(memory.reads==1&&constructors==1&&(abi.gp[0]&255u)==1&&abi.gp[4]==0x70000810u&&abi.gp[6]==0x6789abcdu,"Complete DXT3 caller/constructor return or ABI differs");
   const Bytes color(bank.view(0x70000c00u,12));for(unsigned n=0;n<3;++n)check(color.u32(n*4)==expected_colors[index][n],"DXT3 sample differs from complete original TU1.5 execution");
   ++index;++samples;
  }
  check(Bytes(bank.view(root+0x48u,4)).u32(0)==descriptor&&std::ranges::equal(bank.view(root,tag->size),before),"DXT3 samples modified canonical metadata");
 }
 const auto live=bank.checkpoint();
 const auto clean=projection.checkpoint_without_resources();
 check(live==bank.checkpoint(),"DXT3 checkpoint mutated the completed live bank");
 check(clean.regions.size()<live.regions.size(),"DXT3 checkpoint retained its derived resource regions");
 check(projection.matches_read_only_checkpoint_for_restore(clean),"DXT3 checkpoint lost its exact retained immutable domain");
 std::cout<<"PASS actual MCC DXT3: "<<samples<<" exact original TU1.5 samples, two complete original headers and caller ABIs\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
