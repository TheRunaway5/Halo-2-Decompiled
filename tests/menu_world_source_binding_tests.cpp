#include "halo2/menu_world_source_binding.hpp"
#include "halo2/model.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/source_euler.hpp"
#include "halo2/source_model_palette.hpp"
#include "halo2/source_model_pose.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>

using namespace halo2;
namespace {
void require(bool value,const char *message) {if(!value)throw FormatError(message);}
std::vector<std::byte> read(const std::filesystem::path &path) {
  std::ifstream file(path,std::ios::binary|std::ios::ate);require(bool(file),"original binding fixture missing");
  const auto size=file.tellg();require(size>=0 && size<64*1024*1024,"invalid original binding fixture size");
  std::vector<std::byte> out(static_cast<std::size_t>(size));file.seekg(0);
  if(size && !file.read(reinterpret_cast<char *>(out.data()),size))throw FormatError("short original binding fixture");
  return out;
}
MenuWorldSourceConstants bank(const std::filesystem::path &directory,unsigned event) {
  const auto raw=read(directory/("event-"+std::to_string(event)+"-constants192.bin"));
  MenuWorldSourceConstants out;require(raw.size()==sizeof(out),"original source bank is not c192");
  std::memcpy(out.data(),raw.data(),raw.size());return out;
}
struct CarrierOwner {std::array<float,12> rows;MenuWorldSourceOwner identity;};
CarrierOwner carrier_owner(const Cache &cache,std::uint32_t render_model) {
  const Tag *scenario=nullptr;for(const auto &tag:cache.tags())if(tag.group=="scnr") {
    require(!scenario,"binding fixture requires one source scenario");scenario=&tag;
  }
  require(scenario,"binding fixture scenario missing");
  const auto metadata=cache.load_source_tag_metadata(scenario->datum);
  const Bytes m(metadata.bytes_at(metadata.source_address(),metadata.size()));
  const auto pointer=[&](std::uint32_t address,std::size_t size) {
    require(address>=metadata.source_address(),"scenario binding pointer precedes payload");
    const auto at=std::size_t(address-metadata.source_address());m.require(at,size);return at;
  };
  const auto palette_count=m.u32(0x58),count=m.u32(0x50);
  const auto palette=pointer(m.u32(0x5c),palette_count*0x28),placements=pointer(m.u32(0x54),count*0x5c);
  std::optional<CarrierOwner> result;
  for(unsigned ordinal=0;ordinal<count;++ordinal) {
    const auto at=placements+ordinal*0x5c;const auto slot=m.u16(at);
    require(slot<palette_count,"source placement escapes palette");
    const auto object=m.u32(palette+slot*0x28+4);const auto definition=cache.load_initial_model_definition(object);
    if(definition.render_model!=render_model)continue;
    require(!result,"source binding fixture carrier has duplicate placements");
    const auto idle=cache.load_initialized_object_idle(object);
    const auto model_metadata=cache.load_source_tag_metadata(render_model);
    const Bytes mm(model_metadata.bytes_at(model_metadata.source_address(),model_metadata.size()));
    const auto node_address=mm.u32(0x4c);
    require(mm.u32(0x48)==1 && node_address>=model_metadata.source_address() && idle && idle->nodes.size()==1,
        "source binding carrier is not the proven one-node initialized idle");
    const auto node=node_address-model_metadata.source_address();mm.require(node,0x60);
    require(idle->nodes[0]==mm.u32(node),"source binding carrier graph/model mapping differs");
    const Vec3 position{m.f32(at+8),m.f32(at+12),m.f32(at+16)};
    const auto basis=source_euler_basis({m.f32(at+20),m.f32(at+24),m.f32(at+28)});
    const auto root=source_object_transform(position,basis.forward,basis.up,m.f32(at+32),false);
    const auto world=compose_transform(root,source_model_local_transform(sample_animation(*idle,0,0)));
    const auto inverse=cache.load_source_model_palette_definition(render_model);
    const auto rows=source_model_skin_palette(std::span<const Transform>(&world,1),inverse.inverse_binds);
    require(rows.size()==1,"source binding carrier palette is not one matrix");
    result=CarrierOwner{rows[0],{MenuWorldSourceOwnerKind::scenery_placement,scenario->datum,ordinal,std::nullopt}};
  }
  require(result.has_value(),"source binding fixture carrier owner missing");return *result;
}
struct Counts {unsigned draws{},vectors{},streams{},stream_bytes{},program_bytes{},rejections{};};
template<class Function>void rejected(Counts &counts,Function function) {
  bool caught=false;try{function();}catch(const FormatError &){caught=true;}
  require(caught,"unsupported source binding input accepted");++counts.rejections;
}
MenuWorldSourceBinding captured_binding(const Cache &cache,const std::filesystem::path &directory,unsigned event,
    std::uint32_t program,std::shared_ptr<const MenuWorldSourceSection> geometry,unsigned part,
    MenuWorldSourceTopology topology,const std::array<float,12> &rows,MenuWorldSourceOwner owner,Counts &counts) {
  const auto original=bank(directory,event);auto caller=original;
  // Deliberately destroy ONLY the slots sourced by the binding. Every expected
  // vector is still from the independently captured complete original bank.
  const float poison=std::bit_cast<float>(0x7fa12345u);
  for(const unsigned first:{50u,170u})for(unsigned row=0;row<3;++row)caller[first+row].fill(poison);
  const auto preserved=caller;
  auto draw=menu_world_source_draw(geometry,part,topology,std::span<const std::array<float,12>>(&rows,1));
  auto result=menu_world_source_binding(cache,owner,draw,program,caller);
  require(std::memcmp(result.constants().data(),original.data(),sizeof(original))==0,
      "source binding differs from original complete c192 capture bank");
  require(std::memcmp(caller.data(),preserved.data(),sizeof(caller))==0,
      "source binding modified its caller's bank");
  for(unsigned i=0;i<192;++i)if(!(i>=50 && i<53) && !(i>=170 && i<173))
    require(std::memcmp(result.constants()[i].data(),caller[i].data(),sizeof(ShaderVector))==0,
        "source binding overwrote a caller-owned constant");
  const auto program_bytes=read(directory/("event-"+std::to_string(event)+"-source1-program.bin"));
  require(std::equal(result.program_bytes().begin(),result.program_bytes().end(),program_bytes.begin(),program_bytes.end()),
      "source binding imported class0 or changed original class1 program bytes");
  const bool base=event==1335 || event==1448;
  const std::vector<std::uint16_t> semantics=base?std::vector<std::uint16_t>{0,3,4,5,6}:std::vector<std::uint16_t>{0};
  require(result.declaration().class_index==1 && result.declaration().semantics==semantics &&
      result.program().count==(base?40u:16u),"source binding lost original class1 declaration/instruction count");
  require(result.external_attribute_registers().empty(),"captured class1 binding manufactured an external input");
  const std::vector<unsigned> constants=event==1335?std::vector<unsigned>{0,1,2,3,5,6,7,10,11,18,20,26,27,50,51,52,170,171,172}:
      event==1448?std::vector<unsigned>{0,1,2,3,5,7,10,11,18,19,20,26,27,50,51,52,170,171,172}:
      std::vector<unsigned>{0,1,2,3,10,11,12,13,14,16,50,51,52,170,171};
  require(result.consumed_constants()==constants,"source binding constant-read map differs from original token audit");
  require(result.inputs().size()==semantics.size() && result.consumed_registers().size()==semantics.size(),
      "source binding input count differs from original used registers");
  const bool packed=geometry->kind()==MenuWorldSourceSectionKind::render_model;
  for(unsigned i=0;i<semantics.size();++i) {
    const auto &input=result.inputs()[i];
    const auto format=i==0?(packed?MenuWorldSourceFormat::snorm16x3:MenuWorldSourceFormat::float32x3):
        i==1?(packed?MenuWorldSourceFormat::snorm16x2:MenuWorldSourceFormat::float32x2):MenuWorldSourceFormat::packed_signed_11_11_10;
    const unsigned stride=i==0?(packed?6:12):i==1?(packed?4:8):12;
    const unsigned offset=i>=2?(i-2)*4:0;
    require(input.vertex_register==i && result.consumed_registers()[i]==i && input.consumed &&
        unsigned(input.stream.semantic)==semantics[i] && input.stream.format==format &&
        input.stream.byte_stride==stride && input.stream.byte_offset==offset,
        "source register/format/stride/offset differs from independently retained source attributes");
    const auto expected=read(directory/("event-"+std::to_string(event)+"-vertex-"+std::to_string(i)+".bin"));
    const auto source=result.resource_bytes(i);
    require(offset<=source.size() && expected.size()<=source.size()-offset &&
        std::equal(expected.begin(),expected.end(),source.begin()+offset),
        "source binding decoded/repacked original input resource bytes");
    ++counts.streams;counts.stream_bytes+=expected.size();
  }
  require(result.owner()==owner && result.vertex_shader()==program && result.draw().geometry==geometry &&
      result.draw().part==part && result.draw().topology==topology,
      "source binding conflated owner/resource/program/primitive identity");
  const auto before=result.constants();draw.object_rows[0][3]+=1;caller[4][0]=42;owner.ordinal+=1;
  require(std::memcmp(result.constants().data(),before.data(),sizeof(before))==0 &&
      result.draw().object_rows[0]==rows && result.owner().ordinal!=owner.ordinal,
      "source binding retained mutable caller-owned rows/identity/constants");
  counts.program_bytes+=program_bytes.size();counts.vectors+=192;++counts.draws;return result;
}
}
int main(int argc,char **argv) {try {
  require(argc==3,"source binding test needs mainmenu.map and original Stage3 directory");
  if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))return 77;
  const Cache cache(argv[1]);const std::filesystem::path directory=argv[2];Counts counts;
  const auto bsp52=std::make_shared<const MenuWorldSourceSection>(load_menu_world_source_bsp_section(cache,0,MenuWorldSourceSectionKind::bsp_instance_definition,52));
  const auto bsp0=std::make_shared<const MenuWorldSourceSection>(load_menu_world_source_bsp_section(cache,0,MenuWorldSourceSectionKind::bsp_instance_definition,0));
  const auto carrier=std::make_shared<const MenuWorldSourceSection>(load_menu_world_source_model_section(cache,0xe1d70062u,2));
  const auto owner58=load_menu_world_source_bsp_owner(cache,0,58),owner0=load_menu_world_source_bsp_owner(cache,0,0);
  const auto object=carrier_owner(cache,carrier->owner_datum());
  const auto source1335=captured_binding(cache,directory,1335,0xe1cc0057u,bsp52,2,MenuWorldSourceTopology::triangle_list,
      owner58.rows,{MenuWorldSourceOwnerKind::bsp_instance,owner58.bsp_datum,owner58.instance,std::nullopt},counts);
  (void)captured_binding(cache,directory,1448,0xe1dd0068u,carrier,0,MenuWorldSourceTopology::triangle_strip,object.rows,object.identity,counts);
  (void)captured_binding(cache,directory,1880,0xe1ba0045u,carrier,0,MenuWorldSourceTopology::triangle_strip,object.rows,object.identity,counts);
  (void)captured_binding(cache,directory,2409,0xe1ba0045u,bsp0,0,MenuWorldSourceTopology::triangle_list,
      owner0.rows,{MenuWorldSourceOwnerKind::bsp_instance,owner0.bsp_datum,owner0.instance,std::nullopt},counts);

  // Instance59 is a real distinct owner of the same literal definition52.
  // Its remaining caller context deliberately uses1335: a controlled sharing
  // check, not an assertion that59 was drawn with that complete captured bank.
  const auto owner59=load_menu_world_source_bsp_owner(cache,0,59);
  require(owner59.section==owner58.section,"source owner-sharing fixture no longer shares a section");
  auto context=bank(directory,1335);
  auto draw59=menu_world_source_draw(bsp52,2,MenuWorldSourceTopology::triangle_list,std::span<const std::array<float,12>>(&owner59.rows,1));
  const auto shared=menu_world_source_binding(cache,{MenuWorldSourceOwnerKind::bsp_instance,owner59.bsp_datum,owner59.instance,std::nullopt},draw59,0xe1cc0057u,context);
  require(shared.draw().geometry==source1335.draw().geometry && shared.owner()!=source1335.owner() &&
      std::memcmp(shared.constants()[50].data(),source1335.constants()[50].data(),3*sizeof(ShaderVector))!=0,
      "source shared section conflated distinct owners or transform banks");
  const auto frozen=shared.constants();auto changed=shared;
  auto other_context=context;other_context[159]={std::bit_cast<float>(0x7fa12345u),std::bit_cast<float>(0x80000000u),3,4};
  const auto preservation=menu_world_source_binding(cache,shared.owner(),draw59,0xe1cc0057u,other_context);
  require(std::memcmp(preservation.constants()[159].data(),other_context[159].data(),sizeof(ShaderVector))==0 &&
      std::memcmp(changed.constants().data(),frozen.data(),sizeof(frozen))==0,
      "source binding changed external NaN/signed-zero bits or another binding");
  auto live_owner=object.identity;live_owner.generation=0x12340007u; // Explicit controlled caller identity.
  const auto live=menu_world_source_binding(cache,live_owner,
      menu_world_source_draw(carrier,0,MenuWorldSourceTopology::triangle_strip,std::span<const std::array<float,12>>(&object.rows,1)),0xe1dd0068u,context);
  require(live.owner().generation==live_owner.generation && !shared.owner().generation,
      "source binding fabricated or discarded the caller's optional generation");

  auto valid=menu_world_source_draw(bsp52,2,MenuWorldSourceTopology::triangle_list,std::span<const std::array<float,12>>(&owner58.rows,1));
  const MenuWorldSourceOwner identity{MenuWorldSourceOwnerKind::bsp_instance,owner58.bsp_datum,owner58.instance,std::nullopt};
  rejected(counts,[&]{(void)menu_world_source_binding(cache,identity,valid,0xffffffffu,context);});
  rejected(counts,[&]{(void)menu_world_source_binding(cache,identity,valid,0xe1b60041u,context);}); // Original incident/lightmap declaration unavailable here.
  rejected(counts,[&]{(void)menu_world_source_binding(cache,identity,valid,0xe4700250u,context);}); // Original unsupported semantic10/13.
  rejected(counts,[&]{auto bad=identity;bad.scope_datum=0xffffffffu;(void)menu_world_source_binding(cache,bad,valid,0xe1cc0057u,context);});
  rejected(counts,[&]{auto bad=identity;bad.ordinal=0;(void)menu_world_source_binding(cache,bad,valid,0xe1cc0057u,context);});
  rejected(counts,[&]{auto bad=identity;bad.kind=MenuWorldSourceOwnerKind::scenery_placement;(void)menu_world_source_binding(cache,bad,valid,0xe1cc0057u,context);});
  rejected(counts,[&]{auto bad=identity;bad.kind=static_cast<MenuWorldSourceOwnerKind>(99);(void)menu_world_source_binding(cache,bad,valid,0xe1cc0057u,context);});
  rejected(counts,[&]{auto bad=identity;bad.generation=0xffffffffu;(void)menu_world_source_binding(cache,bad,valid,0xe1cc0057u,context);});
  rejected(counts,[&]{auto bad=valid;bad.geometry.reset();(void)menu_world_source_binding(cache,identity,bad,0xe1cc0057u,context);});
  rejected(counts,[&]{auto bad=valid;bad.object_rows.clear();(void)menu_world_source_binding(cache,identity,bad,0xe1cc0057u,context);});
  rejected(counts,[&]{auto bad=valid;bad.object_rows.resize(41,owner58.rows);(void)menu_world_source_binding(cache,identity,bad,0xe1cc0057u,context);});
  rejected(counts,[&]{auto bad=valid;bad.object_rows[0][0]=std::numeric_limits<float>::infinity();(void)menu_world_source_binding(cache,identity,bad,0xe1cc0057u,context);});
  rejected(counts,[&]{auto bad=valid;bad.topology=static_cast<MenuWorldSourceTopology>(99);(void)menu_world_source_binding(cache,identity,bad,0xe1cc0057u,context);});
  rejected(counts,[&]{auto bad=object.identity;bad.ordinal=0xfffffffeu;(void)menu_world_source_binding(cache,bad,
      menu_world_source_draw(carrier,0,MenuWorldSourceTopology::triangle_strip,std::span<const std::array<float,12>>(&object.rows,1)),0xe1dd0068u,context);});
  require(counts.draws==4 && counts.vectors==768 && counts.streams==12 && counts.stream_bytes==165660 && counts.program_bytes==1808,
      "not all independent original bindings/complete banks were compared");
  std::cout<<counts.draws<<" source bindings / "<<counts.vectors<<" complete original vectors / "
      <<counts.vectors*4<<" words; "<<counts.streams<<" unchanged inputs / "<<counts.stream_bytes
      <<" bytes; "<<counts.program_bytes<<" original class1 program bytes; real shared-section owners58/59; "
      <<counts.rejections<<" unsupported controls\n";return 0;
}catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}}
