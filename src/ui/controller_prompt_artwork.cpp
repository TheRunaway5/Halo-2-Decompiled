#include "halo2/controller_prompt_artwork.hpp"
#include "halo2/frontend.hpp"
#include "halo2/frontend_ui_assets.hpp"
#include "halo2/ui_dds.hpp"
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <cstdlib>
#include <algorithm>
#include <fstream>
#include <sstream>
namespace halo2 {
namespace {
std::filesystem::path resource_directory(std::filesystem::path font) {
  // Vanilla onboarding owns the selected cache's controller banks. Prefer
  // those exact installed assets over the portable setup resources.
  for(auto path=std::filesystem::absolute(font);!path.empty();path=path.parent_path()) {
    const auto root=path/"presentation";
    if(std::filesystem::is_regular_file(root/"prompts-xbox/manifest.json") &&
        std::filesystem::is_regular_file(root/"prompts-playstation/manifest.json"))return root;
    if(path==path.parent_path())break;
  }
  if(const auto *layout=std::getenv("H2_INSTALLATION_LAYOUT")) {
    const auto value=nlohmann::json::parse(layout);
    if(value.at("schema")!="H2_INSTALLATION_LAYOUT_V1")throw FormatError("Invalid prompt installation layout");
    const auto root=std::filesystem::path(value.at("resources").get<std::string>());
    if(!root.is_absolute())throw FormatError("Controller prompt resource root must be absolute");
    return root/"resources/presentation";
  }
  for(auto start:{std::filesystem::absolute(font),std::filesystem::current_path()}) {
    for(auto path=start;!path.empty();path=path.parent_path()) {
      const auto root=path/"resources/presentation";
      if(std::filesystem::is_regular_file(root/"prompts-xbox/manifest.json") &&
          std::filesystem::is_regular_file(root/"prompts-playstation/manifest.json"))return root;
      if(path==path.parent_path())break;
    }
  }
  return {};
}
std::vector<std::byte> bytes(const std::filesystem::path &path,std::size_t maximum) {
  std::ifstream file(path,std::ios::binary|std::ios::ate);const auto size=file.tellg();
  if(!file || size<=0 || std::uint64_t(size)>maximum)throw FormatError("Invalid controller prompt file: "+path.string());
  std::vector<std::byte> result(static_cast<std::size_t>(size));file.seekg(0);
  if(!file.read(reinterpret_cast<char*>(result.data()),result.size()))throw FormatError("Truncated controller prompt file");
  return result;
}
void authenticate(std::span<const std::byte> data,const nlohmann::json &record) {
  std::array<unsigned char,32> hash{};unsigned size{};
  if(data.size()!=record.at("bytes").get<std::size_t>() ||
      EVP_Digest(data.data(),data.size(),hash.data(),&size,EVP_sha256(),nullptr)!=1 || size!=hash.size())
    throw FormatError("Controller prompt asset identity differs");
  constexpr char hex[]="0123456789abcdef";std::string actual;
  for(auto c:hash){actual+=hex[c>>4];actual+=hex[c&15];}
  if(actual!=record.at("sha256").get<std::string>())throw FormatError("Controller prompt asset hash differs");
}
}
struct ControllerPromptArtwork::Impl {
  struct Record {unsigned x,y,width,height;std::string atlas;};
  struct Bank {std::filesystem::path root;nlohmann::json manifest;
    std::map<std::pair<unsigned,unsigned>,Record> records;
    std::map<std::string,unsigned> materials;};
  Mesh &mesh;std::array<Bank,2> banks;bool enabled{};
  Impl(Mesh &m,const std::filesystem::path &font):mesh(m) {
    const auto root=resource_directory(font);if(root.empty())return;
    for(unsigned family=0;family<2;++family) {
      auto &bank=banks[family];bank.root=root/(family?"prompts-playstation":"prompts-xbox");
      const auto manifest=bytes(bank.root/"manifest.json",128*1024);
      bank.manifest=nlohmann::json::parse(reinterpret_cast<const char*>(manifest.data()),
                                        reinterpret_cast<const char*>(manifest.data()+manifest.size()));
      if(bank.manifest.at("schema")!=(family?"H2_PORT_PLAYSTATION_PROMPTS_V1":"H2_PORT_XBOX_PROMPTS_V1"))
        throw FormatError("Invalid controller prompt bank");
      const auto table=bytes(bank.root/"prompts.tsv",64*1024);authenticate(table,bank.manifest.at("files").at("prompts.tsv"));
      std::istringstream input(std::string(reinterpret_cast<const char*>(table.data()),table.size()));
      std::string line;std::getline(input,line);
      if(line!="H2_VISTA_PROMPT_RASTERS_V1")throw FormatError("Invalid controller prompt table");
      while(std::getline(input,line)) {
        unsigned selector{},code{};Record r{};std::string extra;std::istringstream row(line);
        if(!(row>>selector>>code>>r.x>>r.y>>r.width>>r.height>>r.atlas) || row>>extra || selector>=12 ||
            r.atlas.size()!=13 || r.atlas.substr(0,8)!="atlases/" || r.atlas.substr(9)!=".dds" ||
            r.atlas[8]<'0' || r.atlas[8]>'5' || !r.width || !r.height || r.width>2048 || r.height>1024 ||
            !bank.records.emplace(std::pair{selector,code},r).second)
          throw FormatError("Invalid controller prompt rectangle");
      }
      if(bank.records.size()!=331)throw FormatError("Incomplete controller prompt bank");
    }
    enabled=true;
  }
};
ControllerPromptArtwork::ControllerPromptArtwork(Mesh &mesh,const std::filesystem::path &font)
    :impl_(std::make_unique<Impl>(mesh,font)) {}
ControllerPromptArtwork::~ControllerPromptArtwork()=default;
void ControllerPromptArtwork::prepare(unsigned selector) {
  if(selector>=12)throw FormatError("Invalid controller prompt selector");
  if(!impl_->enabled)return;
  for(unsigned family=0;family<2;++family) {
    auto &bank=impl_->banks[family];
    for(const auto &[key,r]:bank.records)if(key.first==selector && !bank.materials.contains(r.atlas)) {
      const auto name=std::string("controller-prompts/")+(family?"playstation/":"xbox/")+r.atlas+"/"+
          bank.manifest.at("files").at(r.atlas).at("sha256").get<std::string>();
      const auto existing=std::find_if(impl_->mesh.materials.begin(),impl_->mesh.materials.end(),
          [&](const auto &m){return m.name==name;});
      if(existing!=impl_->mesh.materials.end()) {
        bank.materials.emplace(r.atlas,unsigned(existing-impl_->mesh.materials.begin()));continue;
      }
      const auto data=bytes(bank.root/r.atlas,32*1024*1024);
      authenticate(data,bank.manifest.at("files").at(r.atlas));
      auto texture=read_ui_dds_texture(Bytes(data));
      for(const auto &[other,sample]:bank.records)if(sample.atlas==r.atlas &&
          (sample.x>texture.width || sample.y>texture.height || sample.width>texture.width-sample.x ||
           sample.height>texture.height-sample.y))throw FormatError("Controller prompt exceeds atlas");
      const auto material=unsigned(impl_->mesh.materials.size());
      impl_->mesh.materials.push_back({name,std::move(texture)});
      auto &added=impl_->mesh.materials.back();added.ui_full_surface=true;
      added.texture_states[0].address_u=added.texture_states[0].address_v=3;
      bank.materials.emplace(r.atlas,material);
    }
  }
}
std::optional<ControllerPromptSample> ControllerPromptArtwork::sample(unsigned selector,unsigned code,PromptDevice device,
    const FontGlyph &glyph,int x,int y,int width,int height)const {
  if(device==PromptDevice::KeyboardMouse || code<0xe100 || code>0xe131 ||
      (code>0xe111 && code<0xe12c) || !glyph.width || !glyph.height)return {};
  if(!impl_->enabled) {
    if(device==PromptDevice::PlayStation)throw FormatError("PlayStation controller prompt resources are unavailable");
    return {};
  }
  const auto &bank=impl_->banks[device==PromptDevice::PlayStation?1:0];
  const auto found=bank.records.find({selector,code});if(found==bank.records.end())return {};
  const auto &r=found->second;const auto material=bank.materials.at(r.atlas);
  const auto &texture=impl_->mesh.materials.at(material).diffuse;
  if(x<0 || y<0 || width<0 || height<0 || unsigned(x+width)>glyph.width || unsigned(y+height)>glyph.height)
    throw FormatError("Invalid clipped controller prompt sample");
  const float sx=float(r.width)/glyph.width,sy=float(r.height)/glyph.height;
  return ControllerPromptSample{material,{(r.x+x*sx)/texture.width,(r.y+y*sy)/texture.height,
    (r.x+(x+width)*sx)/texture.width,(r.y+(y+height)*sy)/texture.height}};
}
}
