#include "halo2/menu_world_source_binding.hpp"
#include "halo2/menu_world_vertex_catalog.hpp"
#include "halo2/model.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/render_view.hpp"
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
#include <string_view>

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
void compare_vertex_metadata(const MenuWorldSourceBinding &source,Counts &counts) {
  const auto &original=menu_world_vertex_metadata_lookup(source.program_bytes(),source.declaration());
  require(&source.vertex_metadata()==&original && &menu_world_vertex_metadata_lookup(source)==&original,
      "raw source binding changed its authenticated vertex metadata");
  const std::array<std::uint32_t,5> spirv_header{0x07230203u,0x00010600u,0u,1u,0u};
  const MenuWorldVertexCatalogEntry entry{.abi=menu_world_renderer_vertex_catalog_abi,
      .class_index=original.class_index,.program_sha256=original.program_sha256,
      .program_bytes=original.program_bytes,.semantics=original.semantics,.spirv=spirv_header,
      .consumed_inputs=original.consumed_inputs,.consumed_constants=original.consumed_constants};
  const auto catalog=std::span(&entry,1);
  require(&menu_world_vertex_catalog_lookup(catalog,source)==
          &menu_world_vertex_catalog_lookup(catalog,source.program_bytes(),source.declaration()),
      "bound vertex catalog identity differs from hashing the original program bytes");
  const std::array duplicate{entry,entry};
  rejected(counts,[&]{(void)menu_world_vertex_catalog_lookup(duplicate,source);});
  rejected(counts,[&]{auto bad=entry;++bad.program_bytes;(void)menu_world_vertex_catalog_lookup(std::span(&bad,1),source);});
  rejected(counts,[&]{auto bad=entry;bad.abi=0;(void)menu_world_vertex_catalog_lookup(std::span(&bad,1),source);});
}
void compare_index_range_binding(const Cache &cache,const MenuWorldSourceBinding &source,Counts &counts) {
  const auto whole=source.draw().index_range();
  const auto strip=source.draw().topology==MenuWorldSourceTopology::triangle_strip;
  const auto first=whole.first+unsigned(strip && whole.count>3);
  const auto count=std::min(whole.count-(first-whole.first),strip?4u:3u);
  auto changed=source;
  require(&changed.constants()==&source.constants() && &changed.program()==&source.program() &&
      &changed.inputs()==&source.inputs() && changed.program_bytes().data()==source.program_bytes().data() &&
      changed.draw().object_rows==source.draw().object_rows &&
      (source.draw().object_rows.empty() || changed.draw().object_rows.data()!=source.draw().object_rows.data()),
      "binding copy lost shared immutable data or independent palette ownership");
  for(const auto range:{MenuWorldSourceIndexRange{first,count},
                        MenuWorldSourceIndexRange{whole.first+whole.count,0}}) {
    auto draw=source.draw();draw.range=range;
    const auto reference=menu_world_source_binding_class(cache,source.owner(),std::move(draw),
        source.vertex_shader(),source.declaration().class_index,source.constants());
    changed.set_index_range(range);
    require(source.draw().index_range()==whole,"range-only update changed another binding");
    require(changed.draw().range==reference.draw().range &&
        changed.draw().geometry==reference.draw().geometry && changed.draw().part==reference.draw().part &&
        changed.draw().topology==reference.draw().topology && changed.owner()==reference.owner() &&
        changed.vertex_shader()==reference.vertex_shader() && changed.declaration()==reference.declaration() &&
        std::ranges::equal(changed.program_bytes(),reference.program_bytes()) &&
        std::memcmp(changed.constants().data(),reference.constants().data(),sizeof(MenuWorldSourceConstants))==0 &&
        std::memcmp(changed.constants().data(),source.constants().data(),sizeof(MenuWorldSourceConstants))==0,
        "range-only binding differs from rebuilding the original prepared draw");
    require(std::ranges::equal(changed.draw().indices(),reference.draw().indices()) &&
        changed.draw().triangle_count()==reference.draw().triangle_count(),"range-only bound indices changed");
    for(std::size_t n=0;n<reference.draw().triangle_count();++n)
      require(changed.draw().triangle(n)==reference.draw().triangle(n),"range-only binding changed strip parity");
  }
  const auto previous=changed.draw().range;
  rejected(counts,[&]{changed.set_index_range(MenuWorldSourceIndexRange{whole.first+whole.count,1});});
  if(!strip && whole.count)rejected(counts,[&]{changed.set_index_range(MenuWorldSourceIndexRange{whole.first,1});});
  require(changed.draw().range==previous,"invalid range changed the prepared binding");
  changed.set_index_range(std::nullopt);
  require(!changed.draw().range && changed.draw().index_range()==whole,"could not restore authored part range");
}
void compare_viewport_binding(const Cache &cache,const MenuWorldSourceBinding &source,Counts &counts) {
  auto caller=source.constants();
  const auto original=viewport_constants({32,16,640,480});
  std::copy(original.begin(),original.end(),caller.begin()+8);
  caller[159]={std::bit_cast<float>(0x7fa12345u),-0.f,3.f,4.f};
  const auto base=menu_world_source_binding_class(cache,source.owner(),source.draw(),source.vertex_shader(),
      source.declaration().class_index,caller);
  const auto stable_bank=base.constants();
  const auto identity=menu_world_source_resized_viewport(base,{1.f,1.f});
  require(&identity.constants()==&base.constants(),"identity viewport scaling detached the constant bank");
  const auto twice=menu_world_source_scaled_viewport(base,2);
  const auto fractional=menu_world_source_resized_viewport(twice,{.5f,1.25f});
  const auto compare=[&](const auto &current,const auto &viewport) {
    auto expected=stable_bank;
    const auto expected_view=viewport_constants(viewport);
    std::copy(expected_view.begin(),expected_view.end(),expected.begin()+8);
    require(std::memcmp(current.constants().data(),expected.data(),sizeof(expected))==0 &&
        &current.program()==&base.program() && &current.inputs()==&base.inputs() &&
        current.draw().index_range()==base.draw().index_range() && current.draw().object_rows==base.draw().object_rows,
        "viewport resize changed the immutable binding or constants outside c8..11");
  };
  compare(twice,RenderViewport{64,32,1280,960});
  compare(fractional,RenderViewport{32,40,640,1200});
  require(&twice.constants()!=&base.constants() && &fractional.constants()!=&twice.constants() &&
      std::memcmp(base.constants().data(),stable_bank.data(),sizeof(stable_bank))==0,
      "changed viewport constants were not detached from the original binding");
  rejected(counts,[&]{(void)menu_world_source_resized_viewport(base,{0.f,1.f});});
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
void compare_resident_input_reuse(const Cache &cache,Counts &counts) {
  constexpr auto shader=0xe1cc0057u;
  auto geometry=std::make_shared<const MenuWorldSourceSection>(
      load_menu_world_source_bsp_section(cache,0,MenuWorldSourceSectionKind::bsp_instance_definition,52));
  const auto a=load_menu_world_source_bsp_owner(cache,0,58);
  const auto b=load_menu_world_source_bsp_owner(cache,0,59);
  require(a.section==52 && b.section==52 && a.rows!=b.rows,"source owners lost their shared section or separate transforms");
  const auto owner=[](const auto &instance) {
    return MenuWorldSourceOwner{MenuWorldSourceOwnerKind::bsp_instance,instance.bsp_datum,instance.instance,{}};
  };
  const auto draw=[&](const auto &instance) {
    return menu_world_source_draw(geometry,2,MenuWorldSourceTopology::triangle_list,std::span(&instance.rows,1));
  };
  MenuWorldSourceBindingCache bindings(cache);
  MenuWorldSourceConstants first{},second{};
  second[159]={std::bit_cast<float>(0x7fa12345u),std::bit_cast<float>(0x80000000u),3,4};
  const auto previous=bindings.bind(owner(a),draw(a),shader,1,first);
  const auto current=bindings.bind(owner(b),draw(b),shader,1,second);
  const auto ordinary_a=menu_world_source_binding(cache,owner(a),draw(a),shader,first);
  const auto ordinary_b=menu_world_source_binding(cache,owner(b),draw(b),shader,second);
  require(&previous.program()==&current.program() && &previous.inputs()==&current.inputs() &&
      &previous.constants()!=&current.constants() && current.owner()==ordinary_b.owner() &&
      previous.draw().object_rows==ordinary_a.draw().object_rows && current.draw().object_rows==ordinary_b.draw().object_rows &&
      std::ranges::equal(previous.program_bytes(),ordinary_a.program_bytes()) &&
      std::memcmp(previous.constants().data(),ordinary_a.constants().data(),sizeof(MenuWorldSourceConstants))==0 &&
      std::memcmp(current.constants().data(),ordinary_b.constants().data(),sizeof(MenuWorldSourceConstants))==0,
      "resident input layout reused a stale owner or current constant bank");
  rejected(counts,[&]{auto invalid=owner(b);invalid.ordinal=0xffffffffu;
    (void)bindings.bind(invalid,draw(b),shader,1,second);});
  rejected(counts,[&]{auto invalid=draw(b);invalid.object_rows.clear();
    (void)bindings.bind(owner(b),std::move(invalid),shader,1,second);});
  rejected(counts,[&]{(void)bindings.bind(owner(b),draw(b),shader,0,second);});
  rejected(counts,[&]{(void)bindings.bind(owner(b),draw(b),0xffffffffu,1,second);});
}
}
int main(int argc,char **argv) {try {
  require(argc==3,"source binding test needs mainmenu.map and an original Stage3 directory or --resident-inputs");
  if(!std::filesystem::exists(argv[1]))return 77;
  const bool resident=std::string_view(argv[2])=="--resident-inputs";
  if(!resident && !std::filesystem::exists(argv[2]))return 77;
  const Cache cache(argv[1]);Counts counts;
  if(resident) {
    compare_resident_input_reuse(cache,counts);
    std::cout<<"Resident layout reused with separate live transforms and constants, "
        <<counts.rejections<<" invalid draw controls\n";
    return 0;
  }
  const std::filesystem::path directory=argv[2];
  const auto bsp52=std::make_shared<const MenuWorldSourceSection>(load_menu_world_source_bsp_section(cache,0,MenuWorldSourceSectionKind::bsp_instance_definition,52));
  const auto bsp0=std::make_shared<const MenuWorldSourceSection>(load_menu_world_source_bsp_section(cache,0,MenuWorldSourceSectionKind::bsp_instance_definition,0));
  const auto carrier=std::make_shared<const MenuWorldSourceSection>(load_menu_world_source_model_section(cache,0xe1d70062u,2));
  const auto owner58=load_menu_world_source_bsp_owner(cache,0,58),owner0=load_menu_world_source_bsp_owner(cache,0,0);
  const auto object=carrier_owner(cache,carrier->owner_datum());
  const auto source1335=captured_binding(cache,directory,1335,0xe1cc0057u,bsp52,2,MenuWorldSourceTopology::triangle_list,
      owner58.rows,{MenuWorldSourceOwnerKind::bsp_instance,owner58.bsp_datum,owner58.instance,std::nullopt},counts);
  const auto source1448=captured_binding(cache,directory,1448,0xe1dd0068u,carrier,0,MenuWorldSourceTopology::triangle_strip,object.rows,object.identity,counts);
  compare_vertex_metadata(source1335,counts);
  compare_vertex_metadata(source1448,counts);
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
  compare_index_range_binding(cache,preservation,counts);
  compare_index_range_binding(cache,source1448,counts);
  compare_viewport_binding(cache,preservation,counts);
  compare_viewport_binding(cache,source1448,counts);
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
