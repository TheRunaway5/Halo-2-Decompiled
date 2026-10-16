#include "halo2/menu_world_raster.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

using namespace halo2;
namespace {
void require(bool value,const char *message) {if(!value)throw FormatError(message);}
const MenuWorldRasterEntryInput ordinary{false,false,false};
MenuWorldRasterSnapshotInput input() {
  return {MenuWorldRasterPrimitive::ordinary_triangles,MenuWorldRasterPolygon::fill,
      std::array<std::int32_t,2>{1,1},true};
}
MenuWorldRasterContext prepared() {
  MenuWorldRasterContext result;result.viewport_reset({640,480});
  result.game_initialize_offset();result.layer_begin(MenuWorldRasterLayer::base_3);
  return result;
}
MenuWorldRasterContext drawable() {auto result=prepared();result.entry_begin(ordinary);return result;}
std::array<std::uint32_t,9> stencil(const MenuWorldRasterPacket &packet) {
  require(packet.stencil.values.has_value(),"original known stencil values became unknown");
  const auto &v=*packet.stencil.values;
  return {packet.stencil.enabled,v.fail,v.zfail,v.pass,v.function,v.reference,
      v.read_mask,v.write_mask,packet.source_stencil_mode};
}
std::array<std::uint32_t,2> offset(const MenuWorldRasterPacket &packet) {
  return {std::bit_cast<std::uint32_t>(packet.depth_factor_offset[0]),
      std::bit_cast<std::uint32_t>(packet.depth_factor_offset[1])};
}
MenuWorldRasterPolygon polygon(unsigned value) {
  require(value<3,"invalid original front polygon fixture");
  return std::array{MenuWorldRasterPolygon::point,MenuWorldRasterPolygon::line,
      MenuWorldRasterPolygon::fill}[value];
}
template<class Function>void rejects(Function function,unsigned &count) {
  bool caught=false;try{function();}catch(const FormatError &){caught=true;}
  require(caught,"unknown/unsupported ordinary raster state was admitted");++count;
}
unsigned controls() {
  unsigned count{};
  MenuWorldRasterContext fresh;
  rejects([&]{(void)fresh.snapshot(input());},count);
  rejects([&]{fresh.entry_begin(ordinary);},count);
  rejects([&]{fresh.entry_end();},count);
  rejects([&]{fresh.shadow_end();},count);
  rejects([&]{fresh.layer_begin(MenuWorldRasterLayer::shadow_10);},count);
  rejects([&]{fresh.layer_begin(static_cast<MenuWorldRasterLayer>(2));},count);
  for(const auto dims:std::array<std::array<std::uint32_t,2>,5>{{
      {0,480},{640,0},{4096,480},{640,4096},{0xffffffffu,480}}})
    rejects([&]{fresh.viewport_reset(dims);},count);
  {
    MenuWorldRasterContext missing;missing.game_initialize_offset();
    missing.layer_begin(MenuWorldRasterLayer::base_3);missing.entry_begin(ordinary);
    rejects([&]{(void)missing.snapshot(input());},count);
  }
  {
    MenuWorldRasterContext missing;missing.viewport_reset({640,480});
    missing.layer_begin(MenuWorldRasterLayer::base_3);missing.entry_begin(ordinary);
    rejects([&]{(void)missing.snapshot(input());},count);
  }
  for(unsigned field=0;field<3;++field) {
    auto context=prepared();auto entry=ordinary;
    if(field==0)entry.stencil_marked_4000.reset();if(field==1)entry.special_485a75.reset();
    if(field==2)entry.special_485a76.reset();
    rejects([&]{context.entry_begin(entry);},count);
    // A rejected setup must not leave an entry scope behind.
    context.entry_begin(ordinary);require(stencil(context.snapshot(input()))[0]==1,
        "rejected entry setup poisoned its retained context");
  }
  auto context=drawable();
  rejects([&]{context.entry_begin(ordinary);},count);
  rejects([&]{context.layer_begin(MenuWorldRasterLayer::emissive_7);},count);
  rejects([&]{context.viewport_reset({1280,720});},count);
  rejects([&]{context.game_initialize_offset();},count);
  rejects([&]{context.shadow_begin();},count);
  for(auto value:{MenuWorldRasterPrimitive::unspecified,MenuWorldRasterPrimitive::other,
      static_cast<MenuWorldRasterPrimitive>(99)}) {
    auto altered=input();altered.primitive=value;
    rejects([&]{(void)context.snapshot(altered);},count);
  }
  for(auto value:{MenuWorldRasterPolygon::unspecified,static_cast<MenuWorldRasterPolygon>(99)}) {
    auto altered=input();altered.front_polygon=value;
    rejects([&]{(void)context.snapshot(altered);},count);
  }
  {auto altered=input();altered.depth_stencil_surface_bound.reset();
   rejects([&]{(void)context.snapshot(altered);},count);}
  {auto altered=input();altered.surface_scale.reset();
   rejects([&]{(void)context.snapshot(altered);},count);}
  for(const auto scale:std::array<std::array<std::int32_t,2>,5>{{
      {0,1},{1,0},{-1,1},{1,-1},{std::numeric_limits<std::int32_t>::max(),1}}}) {
    auto altered=input();altered.surface_scale=scale;
    rejects([&]{(void)context.snapshot(altered);},count);
  }
  context.entry_end();rejects([&]{(void)context.snapshot(input());},count);
  rejects([&]{context.entry_end();},count);
  // Failure is atomic: no partial source surface reset, even if only its
  // second dimension is malformed. Exact old region remains available.
  rejects([&]{context.viewport_reset({1280,0});},count);
  context.entry_begin(ordinary);
  require(context.snapshot(input()).regions[0]==std::array<std::int32_t,4>{0,0,641,481},
      "rejected viewport reset partially mutated its context");context.entry_end();
  {
    auto shadow=prepared();shadow.shadow_begin();
    rejects([&]{shadow.shadow_begin();},count);
    rejects([&]{shadow.game_initialize_offset();},count);
    rejects([&]{shadow.layer_begin(MenuWorldRasterLayer::base_3);},count);
    rejects([&]{shadow.entry_begin(ordinary);},count);
    shadow.layer_begin(MenuWorldRasterLayer::shadow_10);shadow.entry_begin(ordinary);
    rejects([&]{shadow.shadow_end();},count);
    shadow.entry_end();shadow.shadow_end();rejects([&]{shadow.shadow_end();},count);
    //445D0 tears down layer10 and clears the caller's layer. A stale retained
    // layer here cannot admit another shadow entry without its matching setup.
    rejects([&]{shadow.entry_begin(ordinary);},count);
    rejects([&]{(void)shadow.snapshot(input());},count);
    rejects([&]{shadow.layer_begin(MenuWorldRasterLayer::shadow_10);},count);
    shadow.shadow_begin();shadow.layer_begin(MenuWorldRasterLayer::shadow_10);
    shadow.entry_begin(ordinary);
    require(offset(shadow.snapshot(input()))==std::array<std::uint32_t,2>{0x40000000,0x44098000},
        "a rejected stale shadow entry poisoned the next matched shadow pair");
    shadow.entry_end();shadow.shadow_end();
  }
  {
    //495E0 establishes fill only. Unknown point/line state must survive.
    MenuWorldRasterContext shadow;shadow.viewport_reset({640,480});shadow.shadow_begin();
    shadow.layer_begin(MenuWorldRasterLayer::shadow_10);shadow.entry_begin(ordinary);
    require(offset(shadow.snapshot(input()))==std::array<std::uint32_t,2>{0x40000000,0x44098000},
        "source shadow fill update was not available without unrelated point/line knowledge");
    require(!shadow.snapshot(input()).stencil.values,
        "disabled source stencil manufactured retained unknown fields");
    for(auto p:{MenuWorldRasterPolygon::point,MenuWorldRasterPolygon::line}) {
      auto altered=input();altered.front_polygon=p;
      rejects([&]{(void)shadow.snapshot(altered);},count);
    }
    shadow.entry_end();shadow.shadow_end();shadow.layer_begin(MenuWorldRasterLayer::emissive_7);
    shadow.entry_begin(ordinary);
    for(auto p:{MenuWorldRasterPolygon::point,MenuWorldRasterPolygon::line,MenuWorldRasterPolygon::fill}) {
      auto altered=input();altered.front_polygon=p;
      require(offset(shadow.snapshot(altered))==std::array<std::uint32_t,2>{0,0},
          "source shadow teardown failed to establish all positive-zero effective offsets");
    }
  }
  {
    // TU1.5 44360/495D0 ->49690 ->329A4/496E0 ->329B7/15350.
    // Apply must retain the completed generation teardown's zero offset and
    // disabled stencil, including marked/special receiver entries. Each
    // SetViewport consumes its own actual source surface and scale inputs.
    auto shadow=prepared();shadow.viewport_reset({128,128});shadow.shadow_begin();
    rejects([&]{shadow.layer_begin(MenuWorldRasterLayer::shadow_apply_11);},count);
    shadow.layer_begin(MenuWorldRasterLayer::shadow_10);shadow.entry_begin(ordinary);
    auto generate=input();generate.depth_stencil_surface_bound=false;
    generate.surface_scale=std::array<std::int32_t,2>{2,1};
    const auto caster=shadow.snapshot(generate);
    require(offset(caster)==std::array<std::uint32_t,2>{0x40000000,0x44098000} &&
        !caster.stencil.enabled && caster.source_stencil_mode==0 &&
        caster.regions[0]==std::array<std::int32_t,4>{0,0,258,129},
        "source shadow generation lost its actual offset/surface/scale");
    shadow.entry_end();shadow.shadow_end();shadow.viewport_reset({960,540});
    shadow.layer_begin(MenuWorldRasterLayer::shadow_apply_11);
    for(const auto entry:std::array<MenuWorldRasterEntryInput,3>{ordinary,
        MenuWorldRasterEntryInput{true,false,false},MenuWorldRasterEntryInput{false,true,true}}) {
      shadow.entry_begin(entry);const auto receiver=shadow.snapshot(input());
      require(offset(receiver)==std::array<std::uint32_t,2>{0,0} &&
          !receiver.stencil.enabled && receiver.source_stencil_mode==0 &&
          receiver.regions[0]==std::array<std::int32_t,4>{0,0,961,541},
          "source shadow Apply changed teardown/disabled-stencil/window state");
      shadow.entry_end();
    }
  }
  {
    MenuWorldRasterContext disabled;disabled.viewport_reset({640,480});disabled.game_initialize_offset();
    disabled.layer_begin(MenuWorldRasterLayer::planar_fog_16);disabled.entry_begin(ordinary);
    const auto packet=disabled.snapshot(input());
    require(!packet.stencil.enabled && !packet.stencil.values && packet.source_stencil_mode==0,
        "disabled stencil replaced unknown fields with invented defaults");
  }
  {
    auto supplied=drawable();auto scaled=input();scaled.surface_scale=std::array<std::int32_t,2>{2,3};
    const auto packet=supplied.snapshot(scaled);
    require(packet.regions[0]==std::array<std::int32_t,4>{0,0,1282,1443},
        "source AA/render scaling moved before inclusive maximum increment");
    require(packet.surface_scale==*scaled.surface_scale,"source integer surface scale was changed");
  }
  {
    auto upper=prepared();upper.viewport_reset({4095,4095});upper.entry_begin(ordinary);
    require(upper.snapshot(input()).regions[0]==std::array<std::int32_t,4>{0,0,4096,4096},
        "valid maximum12-bit source window was clamped");
  }
  {
    auto flags=prepared();flags.entry_begin(menu_world_raster_entry_from_source_flags(0xffffbfffu,false,false));
    require(stencil(flags.snapshot(input()))==stencil(drawable().snapshot(input())),
        "unrelated source entry bits invented a stencil override");
  }
  return count;
}
}
int main(int argc,char **argv) {try {
  require(argc==2,"ordinary raster test requires its original caller fixture");
  std::ifstream file(argv[1]);require(bool(file),"missing original ordinary raster fixture");
  std::string line;require(bool(std::getline(file,line)) && line=="H2_MENU_WORLD_RASTER_V1",
      "invalid original ordinary raster fixture header");
  std::array<unsigned,4> counts{};unsigned words{};
  while(std::getline(file,line)) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream row(line);char kind;row>>kind;
    if(kind=='V') {
      std::string label;std::array<std::uint32_t,2> dimensions;std::array<std::int32_t,2> scale;
      row>>label>>dimensions[0]>>dimensions[1]>>scale[0]>>scale[1];
      std::array<std::array<std::int32_t,4>,8> expected;
      for(auto &region:expected)for(auto &value:region)row>>value;
      MenuWorldRasterContext context;context.viewport_reset(dimensions);context.game_initialize_offset();
      context.layer_begin(MenuWorldRasterLayer::emissive_7);context.entry_begin(ordinary);
      auto supplied=input();supplied.surface_scale=scale;const auto packet=context.snapshot(supplied);
      require(!packet.window_exclusive && packet.regions==expected,
          "source viewport reset packet differs from actual SetViewport/SetScissors words");
      require(packet.surface_scale==scale,"source scale packet changed");++counts[0];words+=32;
    } else if(kind=='O') {
      std::string phase;unsigned p;std::array<std::uint32_t,2> expected;row>>phase>>p>>expected[0]>>expected[1];
      auto context=prepared();
      if(phase=="setup" || phase=="teardown") {
        context.shadow_begin();context.layer_begin(MenuWorldRasterLayer::shadow_10);
        if(phase=="teardown"){context.shadow_end();context.layer_begin(MenuWorldRasterLayer::base_3);}
      } else require(phase=="initial","unknown original raster offset phase");
      context.entry_begin(ordinary);auto supplied=input();supplied.front_polygon=polygon(p);
      require(offset(context.snapshot(supplied))==expected,
          "effective polygon terms differ from original enable/register contract");++counts[1];words+=2;
    } else if(kind=='S') {
      unsigned layer,flags,a,b;std::array<std::uint32_t,9> during,restored;row>>layer>>flags>>a>>b;
      for(auto &value:during)row>>value;for(auto &value:restored)row>>value;
      require(a<=1 && b<=1,"invalid original special context bit");
      auto context=prepared(); // Original oracle's known retained mode1 bank.
      context.layer_begin(static_cast<MenuWorldRasterLayer>(layer));
      context.entry_begin(menu_world_raster_entry_from_source_flags(flags,a!=0,b!=0));
      require(stencil(context.snapshot(input()))==during,
          "entry stencil differs from original4D640/15370 outputs");context.entry_end();
      // A new non-overridden draw observes4D720's actual restored state.
      context.entry_begin(ordinary);
      require(stencil(context.snapshot(input()))==restored,
          "entry stencil restore differs from original4D720 outputs");++counts[2];words+=18;
    } else if(kind=='D') {
      unsigned logical,effective;std::array<std::uint32_t,8> retained;row>>logical>>effective;
      for(auto &value:retained)row>>value;auto context=drawable();
      auto yes=stencil(context.snapshot(input()));require(yes[0]==logical,
          "logical source stencil enable differs");auto supplied=input();supplied.depth_stencil_surface_bound=false;
      auto no=stencil(context.snapshot(supplied));require(no[0]==effective,
          "missing actual depth surface did not disable hardware stencil");
      for(unsigned i=0;i<8;++i)require(yes[i+1]==retained[i] && no[i+1]==retained[i],
          "actual attachment presence changed retained source stencil fields");++counts[3];words+=9;
    } else throw FormatError("unknown ordinary raster fixture record");
    require(bool(row),"truncated original ordinary raster fixture");std::string extra;
    require(!(row>>extra),"trailing original ordinary raster fixture input");
  }
  require(file.eof() && counts==std::array<unsigned,4>{6,9,24,1} && words==651,
      "incomplete original ordinary raster fixture coverage");
  const auto rejected=controls();
  std::cout<<"40 source-pinned caller cases / "<<words<<" packet words exact; "<<rejected
      <<" unknown/malformed/lifecycle inputs rejected; source-only context, no live admission\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
