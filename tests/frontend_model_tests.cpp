#include "halo2/frontend_model.hpp"
#include "halo2/model.hpp"
#include "halo2/transform.hpp"
#include "halo2/content.hpp"
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>

namespace {
using namespace halo2;
void require(bool ok,const char *message){if(!ok)throw FormatError(message);}
std::string sha(std::span<const std::byte> bytes) {
  std::array<unsigned char,SHA256_DIGEST_LENGTH> digest{};
  SHA256(reinterpret_cast<const unsigned char*>(bytes.data()),bytes.size(),digest.data());
  std::string result;constexpr char digits[]="0123456789abcdef";
  for(auto byte:digest){result+=digits[byte>>4];result+=digits[byte&15];}
  return result;
}
void camera_fixtures(const std::filesystem::path &path) {
  std::ifstream input(path,std::ios::binary);require(bool(input),"original model camera fixture missing");
  unsigned cases=0;
  std::array<float,20> record;
  while(input.read(reinterpret_cast<char *>(record.data()),sizeof(record))) {
    UiScene scene;scene.camera={record[0],record[1],record[2]};
    scene.scale={record[3],record[4],record[5]};scene.field_of_view=record[9];
    const auto camera=frontend_model_camera(scene,{record[6],record[7],record[8]});
    const std::array<float,10> actual{camera.position.x,camera.position.y,camera.position.z,
      camera.forward.x,camera.forward.y,camera.forward.z,camera.up.x,camera.up.y,
      camera.up.z,*camera.projection_vertical_angle};
    require(camera.oriented,"original model camera orientation lost");
    for(unsigned i=0;i<actual.size();++i)
      if(std::bit_cast<std::uint32_t>(actual[i])!=std::bit_cast<std::uint32_t>(record[i+10]))
        throw FormatError("original model camera word differs at case "+std::to_string(cases)+
          " component "+std::to_string(i));
    ++cases;
  }
  require(input.gcount()==0 && cases==1017,"original model camera fixture coverage changed");
  UiScene invalid;invalid.field_of_view=std::numeric_limits<float>::quiet_NaN();
  bool rejected=false;try{(void)frontend_model_camera(invalid);}catch(const FormatError&){rejected=true;}
  require(rejected,"model camera admitted nonfinite input");
  std::cout<<cases<<" original model camera cases, "<<cases*10<<" float words exact\n";
}
void animations(const std::filesystem::path &fixture,const std::filesystem::path &maps) {
  Cache cache(maps/"mainmenu.map");std::ifstream input(fixture);
  require(bool(input),"original frontend animation fixture missing");
  std::string line;unsigned cases=0,spartan=0,elite=0;
  while(std::getline(input,line)) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream row(line);std::uint32_t unit,graph,index,ui_result;
    require(bool(row>>std::hex>>unit>>graph>>index>>ui_result),"invalid original frontend animation fixture");
    const auto clip=cache.load_frontend_model_animation(unit);
    require(clip.has_value(),"original menu animation selection returned no pose");
    require(clip->source_graph==graph && clip->source_index==index,
      "native menu animation differs from original UI/current request");
    if(ui_result!=null_datum){require(index==132,"original Spartan UI clip changed");++spartan;}
    else {require(index==73,"failed Elite UI request lost prior combat idle");++elite;}
    const auto initial=cache.load_frontend_model_animation(unit,false);
    require(initial && initial->source_graph==graph &&
        initial->source_index==(ui_result!=null_datum?47u:73u),
        "original menu combat initialization changed");
    // The source selection must actually remove the reported bind pose. Pose
    // decoding and hierarchy arithmetic have independent original fixtures.
    const auto model=cache.load_model(unit,false);
    const auto bind=pose_model_nodes(model),posed=pose_model_nodes(model,&*clip);
    require(bind.size()==posed.size() && !posed.empty(),"menu pose node bank changed");
    bool moved=false;
    for(unsigned i=0;i<bind.size();++i)
      if(std::bit_cast<std::array<std::uint32_t,13>>(bind[i])!=
         std::bit_cast<std::array<std::uint32_t,13>>(posed[i]))moved=true;
    require(moved,"selected menu animation still produced the bind pose");
    ++cases;
  }
  require(cases==10 && spartan==5 && elite==5,"authored preview object coverage changed");
  std::cout<<cases<<" authored preview objects match original animation selection\n";
}
void clocks(const std::filesystem::path &fixture) {
  std::ifstream input(fixture); require(bool(input),"original model clock fixture missing");
  std::string line; unsigned ticks=0,phases=0;
  while(std::getline(input,line)) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream row(line); std::string type; row>>type;
    const unsigned count=type=="tick"?7:type=="phase"?4:0;
    require(count,"invalid model clock fixture type");
    std::array<float,7> in{};std::array<std::uint32_t,4> expected{};
    for(unsigned i=0;i<count;++i){std::uint32_t word;require(bool(row>>std::hex>>word),"invalid model clock input");in[i]=std::bit_cast<float>(word);}
    for(auto &word:expected)require(bool(row>>std::hex>>word),"invalid model clock expected output");
    FrontendModelClock clock;
    if(type=="tick") {
      clock={in[4],in[3],std::uint8_t(in[5]),std::uint8_t(in[6])};
      frontend_model_clock(clock,unsigned(in[0]),unsigned(in[1]),in[2]);++ticks;
    } else {
      clock.frame=in[3];frontend_model_preserve_phase(clock,unsigned(in[0]),unsigned(in[1]),in[2]);++phases;
    }
    const std::array<std::uint32_t,4> actual{std::bit_cast<std::uint32_t>(clock.frame),
        std::bit_cast<std::uint32_t>(clock.rate),clock.flags,clock.delay};
    require(actual==expected,"native model clock differs from unchanged original executable");
  }
  require(ticks==480 && phases==2014,"model clock source fixture coverage changed");
  std::cout<<ticks<<" original model ticks and "<<phases<<" phase transfers exact\n";
}
void matchmaking_ring(const std::filesystem::path &maps) {
  std::ifstream input(std::filesystem::path(__FILE__).parent_path()/
      "fixtures/vista_matchmaking_ring_original.json");
  require(bool(input),"original matchmaking ring fixture missing");
  const auto fixture=nlohmann::json::parse(input);
  unsigned viewports=0,ticks=0;
  for(const auto &row:fixture.at("viewport_controls")) {
    const auto actual=frontend_vista_model_viewport(
        row.at("authored_words").get<std::array<std::int16_t,4>>(),
        row.at("drawable_words").get<std::array<std::int16_t,4>>());
    require(actual==row.at("physical_words").get<std::array<std::int32_t,4>>(),
        "Vista model viewport differs from unchanged PC executable");
    ++viewports;
  }
  const auto &metadata=fixture.at("ring_metadata");
  for(const auto &row:fixture.at("clock_controls")) {
    const auto before=row.at("before").get<std::array<std::uint32_t,7>>();
    FrontendModelClock clock{std::bit_cast<float>(before[4]),
        std::bit_cast<float>(before[3]),std::uint8_t(before[1]),std::uint8_t(before[0])};
    require(before[2]==63,"ring fixture changed source playback options");
    frontend_model_clock(clock,metadata.at("frames"),metadata.at("loop_frame"),
        std::bit_cast<float>(std::uint32_t(std::stoul(row.at("step_word").get<std::string>(),nullptr,16))));
    const std::array<std::uint32_t,5> actual{clock.delay,clock.flags,63,
        std::bit_cast<std::uint32_t>(clock.rate),std::bit_cast<std::uint32_t>(clock.frame)};
    require(actual==row.at("scalar_expected").get<std::array<std::uint32_t,5>>(),
        "matchmaking ring clock differs from unchanged Xbox executable");
    ++ticks;
  }
  require(viewports==48 && ticks==768,"original ring control coverage changed");
  Cache cache(maps/"mainmenu.map");
  const auto level=cache.load_content(0);
  bool found=false;
  for(const auto &object:level.objects) {
    if(object.name<0 || level.object_names.at(object.name).name!="halo_ring2")continue;
    const auto clip=cache.load_initialized_object_idle(object.definition);
    require(clip && clip->source_index==0 && clip->frame_count==121 && clip->loop_frame==1,
        "matchmaking ring did not start its authored looping idle");
    const auto model=cache.load_model(object.definition,false);
    const auto first=pose_model_nodes_frame(model,&*clip,0),later=pose_model_nodes_frame(model,&*clip,30);
    require(!first.empty() && first.size()==later.size(),"ring pose bank is missing");
    bool moved=false;
    for(unsigned i=0;i<first.size();++i)
      moved|=std::bit_cast<std::array<std::uint32_t,13>>(first[i])!=
          std::bit_cast<std::array<std::uint32_t,13>>(later[i]);
    require(moved,"ring animation still produces a fixed pose");
    found=true;
  }
  require(found,"authored halo_ring2 object missing");
  std::cout<<viewports<<" original Vista model rectangles and "<<ticks
      <<" original ring clock controls exact; authored ring pose moves\n";
}
void vista_ring_textures() {
  const auto source=std::filesystem::path(__FILE__).parent_path();
  std::ifstream input(source/"fixtures/vista_progress_textures_original.json");
  require(bool(input),"original Vista ring texture fixture missing");
  const auto fixture=nlohmann::json::parse(input);
  const auto path=source.parent_path()/fixture.at("cache").at("path").get<std::string>();
  require(std::filesystem::exists(path),"supplied Vista ring cache is missing");
  Cache cache(path);require(cache.is_vista(),"ring texture test used a non-Vista cache");
  unsigned textures=0,mips=0;
  for(const auto &row:fixture.at("textures")) {
    const auto datum=std::uint32_t(std::stoul(row.at("datum").get<std::string>(),nullptr,16));
    const auto texture=cache.load_vista_frontend_bitmap(datum,row.at("index"));
    const auto source_format=row.at("format").get<unsigned>();
    const auto output_format=source_format==10?11u:source_format;
    require(texture.width==row.at("width") && texture.height==row.at("height") &&
        texture.format==output_format && texture.layers==row.at("faces").size(),
        "Vista ring source texture declaration changed");
    unsigned mip=0;std::size_t end=0;
    for(const auto &face:row.at("faces"))for(const auto &level:face) {
      require(mip<texture.mips.size(),"Vista ring source mip is missing");
      const auto &actual=texture.mips[mip++];
      require(actual.width==level.at("width") && actual.height==level.at("height") &&
          actual.offset==end && actual.size==level.at("bytes"),"Vista ring face/mip extent changed");
      require(sha(std::span(texture.pixels).subspan(actual.offset,actual.size))==level.at("sha256").get<std::string>(),
          "Vista ring texture differs from its actual PC face/mip payload");
      end+=actual.size;++mips;
    }
    require(mip==texture.mips.size() && end==texture.pixels.size(),"Vista ring texture has extra face/mip bytes");
    bool rejected=false;
    try{(void)cache.load_vista_frontend_bitmap(datum,1);}catch(const FormatError&){rejected=true;}
    require(rejected,"Vista ring bitmap silently substituted an unavailable frame");
    ++textures;
  }
  require(textures==5 && mips==71,"Vista ring texture source coverage changed");
  std::cout<<textures<<" actual Vista source textures and "<<mips<<" face/mip payload hashes exact\n";
}
void vista_ring_geometry() {
  const auto source=std::filesystem::path(__FILE__).parent_path();
  std::ifstream input(source/"fixtures/vista_progress_geometry_original.json");
  require(bool(input),"original Vista ring geometry fixture missing");
  const auto fixture=nlohmann::json::parse(input);
  Cache cache(source.parent_path()/fixture.at("cache").at("path").get<std::string>());
  require(cache.is_vista(),"ring geometry test used a non-Vista cache");
  const auto datum=std::uint32_t(std::stoul(fixture.at("mode").get<std::string>(),nullptr,16));
  const auto model=cache.load_model(datum,false);
  const auto expected=fixture.at("vertex_sha256").get<std::vector<std::string>>();
  require(expected.size()==1622 && model.vertex_origins.size()==model.mesh.vertices.size() &&
      !model.mesh.vertices.empty(),"Vista ring source geometry coverage changed");
  std::vector<bool> used(expected.size());
  for(unsigned i=0;i<model.mesh.vertices.size();++i) {
    const auto &origin=model.vertex_origins[i];const auto &vertex=model.mesh.vertices[i];
    require(origin.section==0 && origin.vertex_class==2 && origin.vertex<expected.size(),
        "Vista ring lost its original class/section/vertex identity");
    const std::array<float,5> components{vertex.position.x,vertex.position.y,
        vertex.position.z,vertex.uv.x,vertex.uv.y};
    require(sha(std::as_bytes(std::span(components)))==expected[origin.vertex],
        "Vista ring coordinates differ from original FLOAT inputs and c170..172");
    used[origin.vertex]=true;
  }
  const auto unique=std::count(used.begin(),used.end(),true);
  require(unique>0,"Vista ring rendered no original input vertices");
  std::cout<<model.mesh.vertices.size()<<" expanded Vista ring vertices ("<<unique
      <<" unique inputs) match original position/UV constant expansion\n";
}
}
int main(int argc,char **argv) {
  try {
    require(argc==4,"frontend model tests require camera/animation fixtures and maps");
    const auto clock=std::filesystem::path(argv[1]).parent_path()/"model-clock-original.tsv";
    if(!std::filesystem::exists(std::filesystem::path(argv[3])/"mainmenu.map") ||
       !std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]) ||
       !std::filesystem::exists(clock))return 77;
    camera_fixtures(argv[1]);animations(argv[2],argv[3]);clocks(clock);
    matchmaking_ring(argv[3]);vista_ring_textures();vista_ring_geometry();return 0;
  }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
