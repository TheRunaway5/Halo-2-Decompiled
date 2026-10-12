#include "halo2/frontend.hpp"
#include "halo2/frontend_extension_catalog.hpp"
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <array>
#include <bit>
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace halo2;
using Json=nlohmann::json;
namespace {
unsigned checks{};
void check(bool value,const std::string& reason) {
  ++checks;if(!value)throw std::runtime_error(reason);
}
std::uint32_t word(float value) {return std::bit_cast<std::uint32_t>(value);}
Json point(Vec2 v) {return Json::array({word(v.x),word(v.y)});}
Json position(Vec3 v) {return Json::array({word(v.x),word(v.y),word(v.z)});}
Json color(UiColor v) {return Json::array({word(v.alpha),word(v.red),word(v.green),word(v.blue)});}
Json rect(UiRect v) {return Json::array({word(v.top),word(v.left),word(v.bottom),word(v.right)});}
template<class T,class Convert> Json array(const T& values,Convert convert) {
  auto out=Json::array();for(const auto& value:values)out.push_back(convert(value));return out;
}
Json text(const UiText& v) {
  return { {"flags",v.flags},{"animation",v.animation},{"delay",v.delay},{"font",v.font},
    {"color",color(v.color)},{"bounds",rect(v.bounds)},{"string",v.string},{"layer",v.layer} };
}
Json bitmap(const UiBitmap& v) {
  return {{"flags",v.flags},{"animation",v.animation},{"delay",v.delay},{"blend",v.blend},
    {"frame",v.frame},{"position",point(v.position)},{"scroll",point(v.scroll)},
    {"datum",v.datum},{"layer",v.layer},{"fps",word(v.fps)},
    {"placeholder",v.placeholder},{"scale",point(v.scale)}};
}
Json grid(const UiGrid& v) {
  return {{"skin",v.skin},{"position",point(v.position)},{"order",v.order},
    {"count",v.count},{"rows",v.rows},{"columns",v.columns},{"step",point(v.step)}};
}
Json emblem(const UiEmblem& v) {
  return {{"flags",v.flags},{"animation",v.animation},{"delay",v.delay},{"layer",v.layer},
    {"page",v.page},{"bitmap",v.bitmap},{"shader",v.shader},{"bounds",rect(v.bounds)}};
}
Json track(const UiTrack& v) {
  return {{"milliseconds",v.milliseconds},{"keys",array(v.keys,[](const UiKey& key){
    return Json{{"alpha",word(key.alpha)},{"position",position(key.position)}};})}};
}
Json animation(const UiAnimation& v) {
  return {{"intro",track(v.intro)},{"outro",track(v.outro)},
    {"ambient",track(v.ambient)},{"loop",v.loop}};
}
Json list(const UiList& v) {
  return {{"flags",v.flags},{"skin",v.skin},{"visible",v.visible},
    {"position",point(v.position)},{"animation",v.animation},{"delay",v.delay},{"labels",v.labels}};
}
Json button(const UiButton& v) {
  return {{"text",text(v.text)},{"bitmap",v.bitmap},{"offset",point(v.offset)},
    {"vista_hit_top_offset",v.vista_hit_top_offset},{"vista_flags",v.vista_flags}};
}
Json pane(const UiPane& v) {
  check(v.scenes.empty(),"PC UI unexpectedly imported a 3D scene");
  return {{"animation",v.animation},{"buttons",array(v.buttons,button)},{"scenes",Json::array()},
    {"lists",array(v.lists,list)},{"texts",array(v.texts,text)},{"bitmaps",array(v.bitmaps,bitmap)},
    {"emblems",array(v.emblems,emblem)},{"grids",array(v.grids,grid)}};
}
Json screen(const UiScreen& v) {
  auto strings=Json::object();for(const auto& [id,value]:v.strings)strings[std::to_string(id)]=value;
  return {{"name",v.name},{"id",v.id},{"flags",v.flags},{"button_key",v.button_key},
    {"shape",v.shape},{"header",v.header},{"color",color(v.color)},{"strings",strings},
    {"bitmap_resources",v.bitmap_resources},{"panes",array(v.panes,pane)}};
}
Json skin(const UiSkin& v) {
  return {{"name",v.name},{"arrows",v.arrows},{"arrow_up",point(v.arrow_up)},
    {"arrow_down",point(v.arrow_down)},{"animations",array(v.animations,track)},
    {"texts",array(v.texts,text)},{"bitmaps",array(v.bitmaps,bitmap)},
    {"emblems",array(v.emblems,emblem)},{"grids",array(v.grids,grid)}};
}
std::string sha(std::span<const std::byte> bytes) {
  unsigned char digest[SHA256_DIGEST_LENGTH];
  SHA256(reinterpret_cast<const unsigned char*>(bytes.data()),bytes.size(),digest);
  constexpr char digits[]="0123456789abcdef";
  std::string out;for(const auto byte:digest){out+=digits[byte>>4];out+=digits[byte&15];}return out;
}
Json texture(const Texture& v) {
  return {{"width",v.width},{"height",v.height},{"format",v.format},
    {"layers",v.layers},{"depth",v.depth},{"pixels_bytes",v.pixels.size()},
    {"pixels_sha256",sha(v.pixels)},{"mips",array(v.mips,[](const TextureMip& m){
      return Json{{"width",m.width},{"height",m.height},{"offset",m.offset},{"size",m.size},{"depth",m.depth}};})}};
}
Json image(const UiImage& v) {
  return {{"name",v.name},{"images",array(v.images,texture)},
    {"sequences",array(v.sequences,[](const auto& s){return array(s,[](const UiSprite& t){
      return Json{{"image",t.image},{"uv",Json::array({word(t.left),word(t.right),word(t.top),word(t.bottom)})},
        {"registration",point(t.registration)}};});})}};
}
void compare_json(const Json& actual,const Json& expected,const std::string& context) {
  ++checks;
  if(actual!=expected)throw std::runtime_error(context+": "+Json::diff(expected,actual).dump().substr(0,2000));
}
}
int main(int argc,char** argv) {
  try {
    if(argc!=3 || !std::filesystem::is_regular_file(argv[1]))return 77;
    std::ifstream source(argv[2]);check(bool(source),"missing original UI asset fixture");
    const auto fixture=Json::parse(source);
    const bool selector=fixture.at("schema")=="H2V_SELECTOR_UI_ASSETS_V1";
    check(selector || fixture.at("schema")=="H2V_LOBBY_UI_ASSETS_V1","fixture schema");
    check(fixture.at("source_map_sha256")==frontend_vista_menu_catalog_source_sha256(),"source map pin differs from catalog");
    std::ifstream map(argv[1],std::ios::binary|std::ios::ate);
    check(bool(map),"missing source map");
    std::vector<std::byte> raw(std::size_t(map.tellg()));map.seekg(0);
    map.read(reinterpret_cast<char*>(raw.data()),std::streamsize(raw.size()));
    check(bool(map) && sha(raw)==fixture.at("source_map_sha256").get<std::string>(),"actual source map pin");
    const auto content=load_vista_settings(argv[1]);
    check(content->source_fonts==std::filesystem::path(argv[1]).parent_path()/"fonts","PC font source namespace");
    check(!content->vista && !content->scene_clear,"PC UI import gained backdrop ownership");
    const FrontendContent original{};const FrontendExtensionCatalog catalog(original);
    unsigned panes{},buttons{},texts{},grids{},lists{},groups{},images{},frames{},sequences{};
    for(const auto& [datum,expected]:fixture.at("definitions").items()) {
      const auto id=expected.at("id").get<unsigned>();
      check(selector?(id==17 || id==206):(id==14 || id==23),
            "fixture admitted a different source-local screen");
      const auto& actual=content->screens.at(id);panes+=actual.panes.size();
      catalog.validate_definition({FrontendMenuSource::vista,std::uint16_t(id)},actual);
      const auto* entry=catalog.find({FrontendMenuSource::vista,std::uint16_t(id)});
      check(entry && entry->source_datum==std::stoul(datum),"source-local catalog datum");
      compare_json(screen(actual),expected,"PC screen "+std::to_string(id));
      for(const auto& p:actual.panes){buttons+=p.buttons.size();texts+=p.texts.size();grids+=p.grids.size();lists+=p.lists.size();}
    }
    for(const auto& [index,datum]:fixture.at("root_skin_datums").items())
      compare_json(skin(content->skins.at(std::stoul(index))),fixture.at("skin_groups").at(std::to_string(datum.get<std::uint32_t>())),"root skin "+index);
    for(const auto& [datum,expected]:fixture.at("skin_groups").items()) {
      compare_json(skin(content->skin_groups.at(std::stoul(datum))),expected,"skin group "+datum);++groups;
    }
    for(const auto& [datum,expected]:fixture.at("images").items()) {
      const auto& actual=content->images.at(std::stoul(datum));
      compare_json(image(actual),expected,"bitmap bank "+datum);++images;
      frames+=actual.images.size();sequences+=actual.sequences.size();
    }
    for(const auto& [datum,unused]:fixture.at("emblem_shaders").items()) {
      (void)unused;check(!content->emblem_shaders.contains(std::stoul(datum)),"unresolved PC spas installed as renderable emblem material");
    }
    compare_json(Json(content->header_fonts),fixture.at("header_fonts"),"header font indices");
    compare_json(array(content->header_bounds,rect),fixture.at("header_bounds"),"header bounds");
    compare_json(array(content->animations,animation),fixture.at("animations"),"global PC animation bank");
    // These constants identify the actual original dependency closure. Dynamic
    // visibility, hit ownership, animation execution and final pixels have
    // independent runtime owners and are deliberately not inferred here.
    check(fixture.at("definitions").size()==2,"exact source-local screen inventory");
    if(selector)
      check(panes==2 && buttons==0 && texts==3 && grids==0 && lists==2 &&
            groups==1 && images==9 && frames==15,"exact PC17/206 authored inventory");
    else
      check(panes==4 && buttons==9 && texts==151 && grids==3 && lists==1 &&
            groups==3 && images==34 && frames==291,"exact PC14/23 authored inventory");
    std::cout<<"PASS "<<checks<<" authored asset checks: "<<(selector?"PC17/206":"PC14/23")<<", "<<panes<<" panes, "<<buttons
      <<" buttons, "<<texts<<" texts, "<<grids<<" grids, "<<groups<<" skins, "<<images
      <<" bitmap banks, "<<frames<<" mip-preserving frames, "<<sequences
      <<" sequences; no final-pixel or live owner claim\n";
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
