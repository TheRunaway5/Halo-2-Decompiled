#include "halo2/menu_world_source_geometry.hpp"
#include "halo2/model.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/source_euler.hpp"
#include "halo2/source_model_palette.hpp"
#include "halo2/source_model_pose.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <limits>

using namespace halo2;
namespace {
void require(bool condition,const char *message) { if(!condition)throw FormatError(message); }
std::vector<std::byte> read(const std::filesystem::path &path) {
  std::ifstream file(path,std::ios::binary|std::ios::ate);
  require(bool(file),"original source geometry fixture is missing");const auto size=file.tellg();
  require(size>=0 && size<64*1024*1024,"invalid original geometry fixture size");
  std::vector<std::byte> out(static_cast<std::size_t>(size));file.seekg(0);
  if(size && !file.read(reinterpret_cast<char*>(out.data()),size))throw FormatError("short source fixture read");
  return out;
}
void rows_equal(const std::array<float,12> &actual,const Bytes &expected,unsigned first,const char *message) {
  for(unsigned i=0;i<12;++i)require(std::bit_cast<std::uint32_t>(actual[i])==expected.u32(first*16+i*4),message);
}
struct Counts {unsigned streams{},bytes{},draws{},triangles{},degenerates{},negative_controls{},range_controls{};};
template<class Callback> void rejected(Counts &counts,Callback call) {
  bool caught=false;try{call();}catch(const FormatError &){caught=true;}
  require(caught,"unsupported source geometry input silently accepted");++counts.negative_controls;
}
std::array<float,12> carrier_owner(const Cache &cache,std::uint32_t render_model) {
  // Derive the actual carrier owner from scenario palettes, then the existing
  // source idle/pose/palette APIs. Expected words remain independent GPU bytes.
  const Tag *scenario=nullptr;for(const auto &t:cache.tags())if(t.group=="scnr") {
    require(!scenario,"source fixture requires one scenario");scenario=&t;
  }
  require(scenario,"source fixture scenario is missing");
  const auto meta=cache.load_source_tag_metadata(scenario->datum);
  const Bytes m(meta.bytes_at(meta.source_address(),meta.size()));
  const auto pointer=[&](std::uint32_t address,std::size_t size) {
    require(address>=meta.source_address(),"scenario fixture pointer precedes metadata");
    const auto at=std::size_t(address-meta.source_address());m.require(at,size);return at;
  };
  const auto palette_count=m.u32(0x58),count=m.u32(0x50);
  const auto palette=pointer(m.u32(0x5c),palette_count*0x28),placements=pointer(m.u32(0x54),count*0x5c);
  std::optional<std::array<float,12>> result;
  for(unsigned i=0;i<count;++i) {
    const auto at=placements+i*0x5c;const auto slot=m.u16(at);
    require(slot<palette_count,"scenario fixture palette slot escapes bank");
    const auto object=m.u32(palette+slot*0x28+4);const auto definition=cache.load_initial_model_definition(object);
    if(definition.render_model!=render_model)continue;
    require(!result,"carrier source fixture has duplicate model placements");
    const auto idle=cache.load_initialized_object_idle(object);
    const auto model_meta=cache.load_source_tag_metadata(render_model);
    const Bytes mm(model_meta.bytes_at(model_meta.source_address(),model_meta.size()));
    const auto nodes=mm.u32(0x48),node_address=mm.u32(0x4c);
    require(nodes==1 && node_address>=model_meta.source_address() && idle && idle->nodes.size()==1,
            "carrier source fixture no longer has its source one-node idle");
    const auto node=node_address-model_meta.source_address();mm.require(node,0x60);
    require(idle->nodes[0]==mm.u32(node),"carrier idle graph/model node mapping differs");
    const Vec3 position{m.f32(at+8),m.f32(at+12),m.f32(at+16)};
    const auto basis=source_euler_basis({m.f32(at+20),m.f32(at+24),m.f32(at+28)});
    const auto root=source_object_transform(position,basis.forward,basis.up,m.f32(at+32),false);
    const auto local=sample_animation(*idle,0,0);
    const auto world=compose_transform(root,source_model_local_transform(local));
    const auto palette_definition=cache.load_source_model_palette_definition(render_model);
    const auto rows=source_model_skin_palette(std::span<const Transform>(&world,1),palette_definition.inverse_binds);
    require(rows.size()==1,"carrier source fixture skin palette is not one matrix");result=rows[0];
  }
  require(result.has_value(),"carrier source owner could not be derived from scenario metadata");return *result;
}
void captured_draw(const std::filesystem::path &directory,unsigned event,
    std::shared_ptr<const MenuWorldSourceSection> section,unsigned part,MenuWorldSourceTopology topology,
    const std::array<float,12> &owner,unsigned streams,Counts &counts) {
  const auto prefix="event-"+std::to_string(event);
  const auto raw_constants=read(directory/(prefix+"-constants192.bin"));const Bytes constants(raw_constants);
  rows_equal(section->compression_rows(),constants,170,"source compression c170..172 differs from original GPU bank");
  rows_equal(owner,constants,50,"source owner c50..52 differs from original GPU bank");
  const auto draw=menu_world_source_draw(section,part,topology,std::span<const std::array<float,12>>(&owner,1));
  const auto raw_indices=read(directory/(prefix+"-indices.bin"));const Bytes indices(raw_indices);
  require(raw_indices.size()==draw.indices().size()*4,"original capture index count differs from source part");
  for(unsigned i=0;i<draw.indices().size();++i)require(draw.indices()[i]==indices.u32(i*4),
      "original uint32 capture differs from literal source ushort index bank");
  require(section->streams().size()==5 && section->descriptors().size()==3,
          "source position/UV/packed basis declaration did not survive");
  for(unsigned i=0;i<streams;++i) {
    const auto expected=read(directory/(prefix+"-vertex-"+std::to_string(i)+".bin"));
    const auto &stream=section->streams()[i];const auto &source=section->resources().at(stream.resource).bytes;
    require(stream.byte_offset<=source.size() && expected.size()<=source.size()-stream.byte_offset,
            "captured vertex range escapes source resource");
    require(std::equal(expected.begin(),expected.end(),source.begin()+stream.byte_offset),
            "source geometry changed captured packed/local stream bytes");
    ++counts.streams;counts.bytes+=expected.size();
  }
  for(std::size_t i=0;i<draw.triangle_count();++i) {
    const auto triangle=draw.triangle(i);const auto original=draw.indices();
    if(topology==MenuWorldSourceTopology::triangle_strip && (i&1))
      require(triangle==std::array<std::uint16_t,3>{original[i],original[i+2],original[i+1]},
              "source strip odd order changed cyclic GS inputs");
    counts.degenerates+=triangle[0]==triangle[1] || triangle[0]==triangle[2] || triangle[1]==triangle[2];
    ++counts.triangles;
  }
  if(topology==MenuWorldSourceTopology::triangle_strip) {
    require(draw.triangle_count()==4377,"source carrier strip dropped original triangles");
    unsigned degenerates=0;for(std::size_t i=0;i<draw.triangle_count();++i) {
      const auto t=draw.triangle(i);degenerates+=t[0]==t[1] || t[0]==t[2] || t[1]==t[2];
    }
    require(degenerates==1719,"source carrier strip discarded or manufactured degenerates");
  }
  require(draw.object_rows.size()==1 && draw.object_rows[0]==owner,"source draw changed actual owner palette");
  ++counts.draws;
}
std::vector<std::uint16_t> captured_indices(const std::filesystem::path &directory,unsigned event) {
  const auto raw=read(directory/("event-"+std::to_string(event)+"-indices.bin"));const Bytes bytes(raw);
  require(raw.size()%4==0,"original range control index bank is not uint32");
  std::vector<std::uint16_t> result;
  for(std::size_t at=0;at<raw.size();at+=4) {
    require(bytes.u32(at)<=0xffff,"original range control index escapes ushort");
    result.push_back(std::uint16_t(bytes.u32(at)));
  }
  return result;
}
void explicit_ranges(const std::filesystem::path &directory,
    const std::shared_ptr<const MenuWorldSourceSection> &bsp52,
    const std::shared_ptr<const MenuWorldSourceSection> &carrier,
    const std::array<float,12> &bsp_rows,const std::array<float,12> &carrier_rows,Counts &counts) {
  const auto original=captured_indices(directory,1335);
  require(original.size()==522,"original part2 range control no longer has522 indices");
  const auto make=[&](MenuWorldSourceIndexRange range) {
    return menu_world_source_draw(bsp52,2,MenuWorldSourceTopology::triangle_list,std::span(&bsp_rows,1),range);
  };
  const auto whole=menu_world_source_draw(bsp52,2,MenuWorldSourceTopology::triangle_list,std::span(&bsp_rows,1));
  require(!whole.range && whole.index_range()==MenuWorldSourceIndexRange{702,522},
          "default source draw no longer resolves the authored whole part");
  // Frozen unchanged4DAA0 controls: whole, noncontiguous bits20/22 and
  // adjacent bits20..22. These ranges are geometry controls, not admission.
  // Authority: part-range-original/results.json, SHA df2133c27d5074541a99d3892b172adab715577eafe8ac6b17fbf0db5d5675cf.
  for(const auto range:std::array<MenuWorldSourceIndexRange,4>{{{702,522},{702,54},{810,12},{702,120}}}) {
    const auto draw=make(range);const auto first=range.first-702;
    require(draw.index_range()==range && draw.indices().size()==range.count &&
            std::equal(draw.indices().begin(),draw.indices().end(),original.begin()+first),
            "explicit source range differs from original captured index order");
    require(draw.triangle_count()==range.count/3,"explicit list range changed triangle count");
    for(std::size_t i=0;i<draw.triangle_count();++i)
      require(draw.triangle(i)==std::array<std::uint16_t,3>{original[first+i*3],original[first+i*3+1],original[first+i*3+2]},
              "explicit list range changed original triangle order");
    ++counts.range_controls;
  }
  for(const auto first:std::array{702u,1224u}) {
    const auto draw=make({first,0});
    require(draw.index_range()==MenuWorldSourceIndexRange{first,0} && draw.indices().empty() && draw.triangle_count()==0,
            "empty in-part range is not an empty geometry description");
    rejected(counts,[&]{(void)draw.triangle(0);});++counts.range_controls;
  }
  for(const auto range:std::array<MenuWorldSourceIndexRange,8>{{
      {701,3},{1225,0},{1224,3},{702,525},{703,std::numeric_limits<unsigned>::max()},
      {std::numeric_limits<unsigned>::max(),0},{702,1},{1223,2}}})
    rejected(counts,[&]{(void)make(range);});
  auto changed=make({702,54});changed.range=MenuWorldSourceIndexRange{702,525};
  rejected(counts,[&]{(void)changed.indices();}); // Revalidate a changed public description.
  const auto strip_original=captured_indices(directory,1448);
  const auto &part=carrier->parts().at(0);
  require(strip_original.size()==part.index_count,"original carrier strip range control count changed");
  const auto strip=menu_world_source_draw(carrier,0,MenuWorldSourceTopology::triangle_strip,
      std::span(&carrier_rows,1),MenuWorldSourceIndexRange{unsigned(part.index_first)+1,14});
  require(strip.triangle_count()==12,"partial strip lost source triangles");
  unsigned degenerates{};
  for(std::size_t i=0;i<strip.triangle_count();++i) {
    const auto at=i+1;
    const auto expected=(i&1)?std::array<std::uint16_t,3>{strip_original[at],strip_original[at+2],strip_original[at+1]}:
                              std::array<std::uint16_t,3>{strip_original[at],strip_original[at+1],strip_original[at+2]};
    require(strip.triangle(i)==expected,"partial strip parity did not restart at the submitted first index");
    degenerates+=expected[0]==expected[1] || expected[0]==expected[2] || expected[1]==expected[2];
  }
  require(strip_original[2]!=strip_original[3] && degenerates==7,
          "partial strip control no longer distinguishes parity and retained degenerates");
  rejected(counts,[&]{(void)strip.triangle(12);});++counts.range_controls;
  // Actual file header retained by the original range oracle, not inferred
  // from resource byte lengths. In particular signed geometry+30 is72.
  constexpr std::array<std::uint32_t,17> header_words{3,0,33,0,0,0,0,0,1224,0,544,0,72,0,3,0,0};
  const auto header=bsp52->raw_header();require(header.size()==header_words.size()*4,"literal BSP raw header extent changed");
  const Bytes header_bytes(header);
  for(unsigned i=0;i<header_words.size();++i)
    require(header_bytes.u32(i*4)==header_words[i],"literal BSP raw header was relocated or reconstructed");
  require(carrier->raw_header().empty(),"model section claims an unproved BSP header mapping");
}
}
int main(int argc,char **argv) {try {
  require(argc==3,"source geometry test needs mainmenu.map and original Stage3 export directory");
  if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))return 77;
  const Cache cache(argv[1]);const std::filesystem::path captures=argv[2];Counts counts;
  const auto bsp0=std::make_shared<const MenuWorldSourceSection>(load_menu_world_source_bsp_section(cache,0,
      MenuWorldSourceSectionKind::bsp_instance_definition,0));
  const auto bsp52=std::make_shared<const MenuWorldSourceSection>(load_menu_world_source_bsp_section(cache,0,
      MenuWorldSourceSectionKind::bsp_instance_definition,52));
  const auto carrier=std::make_shared<const MenuWorldSourceSection>(load_menu_world_source_model_section(cache,0xe1d70062u,2));
  const auto owner0=load_menu_world_source_bsp_owner(cache,0,0),owner58=load_menu_world_source_bsp_owner(cache,0,58);
  require(owner0.section==bsp0->section() && owner58.section==bsp52->section(),"source BSP owner lost definition identity");
  require(std::bit_cast<std::uint32_t>(owner0.separate_scale)==0x3f7ffffeu &&
          std::bit_cast<std::uint32_t>(owner58.separate_scale)==0x3f7fffffu,"source BSP separate scale field changed");
  require(!bsp0->model_geometry_class() && !bsp52->model_geometry_class(),"source BSP invented a model/compiled shader class");
  require(carrier->model_geometry_class()==1 && carrier->compression_flags()==3 && carrier->vertex_count()==6101 &&
          carrier->node_map().size()==1 && carrier->node_map()[0]==0,"source rigid/compression/node metadata changed");
  require(bsp52->streams()[0].format==MenuWorldSourceFormat::float32x3 &&
          bsp52->streams()[1].format==MenuWorldSourceFormat::float32x2 &&
          carrier->streams()[0].format==MenuWorldSourceFormat::snorm16x3 &&
          carrier->streams()[1].format==MenuWorldSourceFormat::snorm16x2 &&
          carrier->streams()[2].format==MenuWorldSourceFormat::packed_signed_11_11_10,
          "source packed/local stream format was replaced with CPU floats");
  // Independent literal cache inspection identifies the underlying material
  // references. Fog's selected program belongs to the pass, not these parts.
  require(bsp52->parts().at(2).material==172 && bsp52->parts().at(2).shader==0xe777054du &&
          bsp0->parts().at(0).material==27 && bsp0->parts().at(0).shader==0xe755052bu &&
          carrier->parts().at(0).material==0 && carrier->parts().at(0).shader==0xe1d80063u,
          "source geometry material shader identity differs from literal original cache");
  unsigned programs=0,program_bytes=0;
  for(const auto [event,datum]:std::array<std::array<unsigned,2>,4>{{
      {1335,0xe1cc0057u},{1448,0xe1dd0068u},{1880,0xe1ba0045u},{2409,0xe1ba0045u}}}) {
    const auto original=read(captures/("event-"+std::to_string(event)+"-source1-program.bin"));
    require(cache.load_vertex_shader_code(datum,1)==original,
            "source class1 vertex program import differs from independently retained original payload");
    ++programs;program_bytes+=original.size();
  }
  const auto owner=carrier_owner(cache,carrier->owner_datum());
  captured_draw(captures,1335,bsp52,2,MenuWorldSourceTopology::triangle_list,owner58.rows,5,counts);
  captured_draw(captures,1448,carrier,0,MenuWorldSourceTopology::triangle_strip,owner,5,counts);
  captured_draw(captures,1880,carrier,0,MenuWorldSourceTopology::triangle_strip,owner,1,counts);
  captured_draw(captures,2409,bsp0,0,MenuWorldSourceTopology::triangle_list,owner0.rows,1,counts);
  explicit_ranges(captures,bsp52,carrier,owner58.rows,owner,counts);
  rejected(counts,[&]{(void)cache.load_source_bsp_metadata(cache.bsps().size());});
  rejected(counts,[&]{(void)load_menu_world_source_bsp_section(cache,0,MenuWorldSourceSectionKind::render_model,0);});
  rejected(counts,[&]{(void)load_menu_world_source_bsp_owner(cache,0,std::numeric_limits<unsigned>::max());});
  rejected(counts,[&]{(void)load_menu_world_source_model_section(cache,carrier->owner_datum(),65535);});
  rejected(counts,[&]{(void)menu_world_source_draw(carrier,65535,MenuWorldSourceTopology::triangle_strip,
      std::span<const std::array<float,12>>(&owner,1));});
  rejected(counts,[&]{(void)menu_world_source_draw(carrier,0,MenuWorldSourceTopology::triangle_strip,{});});
  rejected(counts,[&]{(void)menu_world_source_draw(carrier,0,static_cast<MenuWorldSourceTopology>(99),
      std::span<const std::array<float,12>>(&owner,1));});
  auto bad_owner=owner;bad_owner[0]=std::numeric_limits<float>::infinity();
  rejected(counts,[&]{(void)menu_world_source_draw(carrier,0,MenuWorldSourceTopology::triangle_strip,
      std::span<const std::array<float,12>>(&bad_owner,1));});
  require(counts.draws==4 && counts.streams==12 && counts.bytes==165660 && counts.triangles==9315 && counts.degenerates==3438,
          "not all retained source geometry/strip records were checked");
  std::cout<<counts.draws<<" original draws / "<<counts.streams<<" streams / "<<counts.bytes<<" bytes; "
      <<counts.triangles<<" ordered triangles including "<<counts.degenerates<<" degenerates; "
      <<counts.range_controls<<" explicit range controls; "<<counts.negative_controls<<" unsupported inputs rejected; "<<programs<<" original class1 programs / "
      <<program_bytes<<" bytes and three literal material bindings\n";
  return 0;
}catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}}
