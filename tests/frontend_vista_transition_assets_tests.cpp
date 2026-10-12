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
Json pane(const UiPane& v) {
  check(v.buttons.empty() && v.scenes.empty(),"transition UI gained buttons or 3D scenes");
  return {{"animation",v.animation},{"buttons",Json::array()},{"scenes",Json::array()},
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
void button_field_import(const std::filesystem::path& source) {
  std::ifstream input(source,std::ios::binary | std::ios::ate);
  check(bool(input),"missing PC source for button field regression");
  std::vector<std::byte> raw(std::size_t(input.tellg()));
  input.seekg(0);input.read(reinterpret_cast<char*>(raw.data()),std::streamsize(raw.size()));
  check(bool(input),"incomplete button field source");
  check(sha(raw)==frontend_vista_menu_catalog_source_sha256(),"button field source map pin");
  // Screen293 is already admitted and has one special button. All currently
  // imported retail special buttons contain zero in these trailing fields;
  // controlled nonzero bytes make an accidental default/truncation observable
  // without admitting the still-unbound PC14 lobby.
  constexpr std::size_t button=60350176;
  check(raw.size()>button+60,"source button record missing");
  const Bytes source_bytes(raw);
  check(source_bytes.u16(button+54)==0 && source_bytes.u32(button+56)==0,
      "source screen293 button trailing fields moved");
  const auto original_top=std::bit_cast<std::int16_t>(source_bytes.u16(button+28));
  const auto original_bottom=std::bit_cast<std::int16_t>(source_bytes.u16(button+32));
  constexpr std::int16_t hit_top=-250;
  constexpr std::uint32_t flags=0xa55a0003;
  const auto hit_word=std::bit_cast<std::uint16_t>(hit_top);
  for(unsigned i=0;i<2;++i)raw[button+54+i]=std::byte(hit_word>>(8*i));
  for(unsigned i=0;i<4;++i)raw[button+56+i]=std::byte(flags>>(8*i));
  struct TemporaryCache {
    std::filesystem::path path;
    ~TemporaryCache(){std::error_code ignored;std::filesystem::remove(path,ignored);}
  } temporary{std::filesystem::temp_directory_path()/
      ("halo2-vista-button-fields-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".map")};
  {std::ofstream output(temporary.path,std::ios::binary);output.write(reinterpret_cast<const char*>(raw.data()),std::streamsize(raw.size()));
    check(bool(output),"cannot write controlled button source");}
  const auto imported=load_vista_settings(temporary.path);
  const auto& actual=imported->screens.at(293).panes.at(0).buttons.at(0);
  check(actual.vista_hit_top_offset==hit_top,"signed PC hit top offset was discarded");
  check(actual.vista_flags==flags,"full PC button flags were truncated or discarded");
  check(actual.text.bounds.top==original_top && actual.text.bounds.bottom==original_bottom,
      "trailing PC button fields changed authored draw bounds");
}
}
int main(int argc,char** argv) {
  try {
    if(argc!=3 || !std::filesystem::is_regular_file(argv[1]))return 77;
    std::ifstream source(argv[2]);check(bool(source),"missing original asset fixture");
    const auto fixture=Json::parse(source);
    check(fixture.at("schema")=="H2V_TRANSITION_UI_ASSETS_V1","fixture schema");
    check(fixture.at("source_map_sha256")==frontend_vista_menu_catalog_source_sha256(),"source pin differs from catalog");
    const auto content=load_vista_settings(argv[1]);
    // Includes original aim-assist44/E58F036D alongside the retained PC
    // settings and transition definitions. Check identity, not only count.
    constexpr std::array<unsigned,39> selected_ids{
      5,6,13,14,16,17,19,23,38,42,44,182,185,206,209,210,242,243,245,253,256,257,258,
      263,267,268,269,271,272,273,274,276,285,290,293,294,295,296,297};
    check(content->screens.size()==selected_ids.size(),"selected PC screen inventory differs");
    for(const auto id:selected_ids)
      check(content->screens.contains(id),"selected PC inventory omits screen "+std::to_string(id));
    check(content->source_fonts==std::filesystem::path(argv[1]).parent_path()/"fonts","PC font source namespace");
    check(!content->vista && !content->scene_clear,"PC UI import gained backdrop ownership");
    // Actual scenario-selected wgtz E4D002AE -> wigl E4D102AF; the PC
    // sound datum belongs to its own mainmenu cache, independently of backdrop.
    // Provenance: music-binding-20261011/assets.json beside the transition audit.
    check(content->music==0xe53d031b && content->music_silence_ms==5500,
        "PC import lost scenario-selected wigl+1BC/+1C0 music identity");
    const FrontendContent original{};
    const FrontendExtensionCatalog catalog(original);
    unsigned panes{},root_skins{},children{},images{},frames{},sequences{};
    for(const auto& [datum,expected]:fixture.at("definitions").items()) {
      const auto id=expected.at("id").get<unsigned>();
      const auto& s=content->screens.at(id);panes+=s.panes.size();
      catalog.validate_definition({FrontendMenuSource::vista,std::uint16_t(id)},s);
      const auto* entry=catalog.find({FrontendMenuSource::vista,std::uint16_t(id)});
      check(entry && entry->source_datum==std::stoul(datum),"source-local catalog datum");
      compare_json(screen(s),expected,"screen "+std::to_string(id));
    }
    for(const auto& [index,datum]:fixture.at("root_skin_datums").items()) {
      const auto id=datum.get<std::uint32_t>();
      compare_json(skin(content->skins.at(std::stoul(index))),fixture.at("skin_groups").at(std::to_string(id)),"root skin "+index);
      ++root_skins;
    }
    for(const auto& [datum,expected]:fixture.at("skin_groups").items()) {
      compare_json(skin(content->skin_groups.at(std::stoul(datum))),expected,"skin group "+datum);++children;
    }
    for(const auto& [datum,expected]:fixture.at("images").items()) {
      const auto& actual=content->images.at(std::stoul(datum));
      compare_json(image(actual),expected,"bitmap bank "+datum);++images;
      frames+=actual.images.size();sequences+=actual.sequences.size();
    }
    for(const auto& [datum,unused]:fixture.at("emblem_shaders").items()) {
      (void)unused;
      check(!content->emblem_shaders.contains(std::stoul(datum)),"unresolved PC spas was installed as renderable emblem material");
    }
    compare_json(array(content->appearance_palette,position),fixture.at("palette"),"PC color palette");
    const auto& pcr=fixture.at("pcr_appearance");
    compare_json(array(content->vista_pcr_team_palette,position),pcr.at("team_palette"),"PC PCR team palette");
    compare_json(array(content->vista_pcr_rank_ranges,[](const VistaPcrRankRange& range){
      return Json::array({range.first,range.last});}),pcr.at("rank_ranges"),"PC PCR signed rank ranges");
    check(content->vista_pcr_rank_owner_present==pcr.at("rank_owner_present").get<bool>(),"PC PCR actual rank owner presence");
    check(content->vista_pcr_team_palette.size()==9 && content->vista_pcr_rank_ranges.size()==50,"complete original PCR appearance inventory");
    compare_json(Json(content->header_fonts),fixture.at("header_fonts"),"header fonts");
    compare_json(array(content->header_bounds,rect),fixture.at("header_bounds"),"header bounds");
    compare_json(color(content->color),fixture.at("color"),"PC global text color");
    compare_json(color(content->overlay),fixture.at("overlay"),"PC overlay color");
    compare_json(Json::array({content->typewriter.interval_ms,content->typewriter.cursor,word(content->typewriter.scale)}),fixture.at("typewriter"),"PC typewriter");
    compare_json(array(content->animations,animation),fixture.at("animations"),"PC animation bank");
    check(panes==13 && root_skins==5 && children==7 && images==19 && frames==219,"exact original transition inventory");
    button_field_import(argv[1]);
    std::cout<<"PASS "<<checks<<" asset checks: 3 PC screens, "<<panes<<" panes, "<<root_skins
      <<" root skins, "<<children<<" total skin groups, "<<images<<" bitmap banks, "<<frames
      <<" frames, "<<sequences<<" sequences; all float words, children, animations and full mip pixel hashes\n";
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
