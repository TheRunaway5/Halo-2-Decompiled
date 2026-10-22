#include "halo2/hud.hpp"
#include "halo2/mcc_cache.hpp"
#include "halo2/texture.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <set>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>

namespace {
using namespace halo2;
using Json=nlohmann::json;
const char* phase="Cache construction";
void require(bool ok,const char* why){if(!ok)throw FormatError(why);}
std::string digest(std::span<const std::byte> bytes) {
  std::array<unsigned char,32> out{};unsigned count{};
  require(EVP_Digest(bytes.data(),bytes.size(),out.data(),&count,EVP_sha256(),nullptr)==1 && count==32,
      "Cannot fingerprint actual HUD pixels");
  constexpr char hex[]="0123456789abcdef";std::string result;
  for(auto b:out){result+=hex[b>>4];result+=hex[b&15];}return result;
}
void equal(const Texture& a,const Texture& b) {
  require(a.width==b.width && a.height==b.height && a.format==b.format &&
      a.layers==b.layers && a.depth==b.depth && a.mips.size()==b.mips.size() && a.pixels==b.pixels,
      "HUD image differs from its exact owned bitmap pixels/shape");
  for(unsigned n=0;n<a.mips.size();++n) {
    const auto& x=a.mips[n];const auto& y=b.mips[n];
    require(x.width==y.width && x.height==y.height && x.depth==y.depth &&
        x.offset==y.offset && x.size==y.size,"HUD mip extent differs");
  }
}
struct Bank {unsigned count{},first{};};
Bank bank(const Cache& cache,unsigned at,unsigned stride,unsigned maximum) {
  const Bytes block(cache.read_tag_bytes(at,8));Bank result{block.u32(0),block.u32(4)};
  require(result.count<=maximum,"Fixture block exceeds declared source bounds");
  if(result.count)(void)cache.read_tag_bytes(result.first,std::size_t(result.count)*stride);
  return result;
}
std::array<unsigned,4> sensors(const Cache& cache) {
  const auto* globals=cache.find_tag(cache.globals_tag(),tag_groups::globals);
  require(globals && globals->address,"Fixture has no globals owner");
  const auto interfaces=bank(cache,globals->address+0x110,0x98,16);
  require(interfaces.count,"Fixture has no HUD interface owner");
  std::array<unsigned,4> result{};unsigned n{};
  for(unsigned offset:{0x38u,0x40u,0x60u,0x68u}) {
    const Bytes reference(cache.read_tag_bytes(interfaces.first+offset,8));
    result[n++]=reference.u32(4);
    require(reference.u32(0)==tag_groups::bitmap.value &&
        cache.find_tag({reference.u32(4)},tag_groups::bitmap),"Sensor lost its typed full-salt bitmap owner");
  }
  return result;
}
unsigned check_sprites(const Cache& cache,unsigned datum,const UiImage& image) {
  const auto* tag=cache.find_tag({datum},tag_groups::bitmap);
  require(tag && tag->name==image.name && !cache.find_tag({datum^0x10000u},tag_groups::bitmap),
      "HUD image lost its exact salted bitmap namespace");
  const auto sequences=bank(cache,tag->address+0x3c,60,256);
  require(sequences.count==image.sequences.size(),"HUD sequence count differs from its owner");
  unsigned sprites{};
  for(unsigned n=0;n<sequences.count;++n) {
    const auto at=sequences.first+n*60;const Bytes row(cache.read_tag_bytes(at,60));
    const auto authored=bank(cache,at+52,32,1024);const auto& actual=image.sequences[n];
    require(actual.size()==(authored.count?authored.count:row.u16(34)),"HUD sprite/fallback count differs");
    for(unsigned j=0;j<actual.size();++j) {
      const auto& value=actual[j];
      if(authored.count) {
        const Bytes source(cache.read_tag_bytes(authored.first+j*32,32));
        require(value.image==source.u16(0) && value.left==source.f32(8) &&
            value.right==source.f32(12) && value.top==source.f32(16) && value.bottom==source.f32(20) &&
            value.registration.x==source.f32(24) && value.registration.y==source.f32(28),
            "HUD sprite image/UV/registration changed from its complete source row");
      }else require(value.image==row.u16(32)+j && value.left==0 && value.right==1 &&
          value.top==0 && value.bottom==1 && value.registration.x==0 && value.registration.y==0,
          "HUD image-sequence fallback changed");
      require(value.image<image.images.size(),"HUD sprite escapes image bank");++sprites;
    }
  }
  return sprites;
}
struct Counts {unsigned huds{},images{},sprites{},sensor_images{};};
struct Locale {UiStrings strings;unsigned first{},count{},selected{};Json report;};
Locale locale(const Cache& cache,const std::filesystem::path& map) {
  const auto* globals=cache.find_tag(cache.globals_tag(),tag_groups::globals);
  const Bytes descriptor(cache.read_tag_bytes(globals->address+0x188,28));
  const auto count=descriptor.u32(8),size=descriptor.u32(12),indices=descriptor.u32(16),strings=descriptor.u32(20);
  const auto source=read_mcc_cache((indices&0x80000000u)?cache.shared_directory()/"shared.map":map);
  const Bytes file(source),refs(file.slice(indices&0x7fffffffu,std::size_t(count)*8));
  const Bytes text(file.slice(strings&0x7fffffffu,size));
  const auto interfaces=bank(cache,globals->address+0x110,0x98,16);
  const auto hudg=Bytes(cache.read_tag_bytes(interfaces.first+0x34,4)).u32(0);
  const auto* hud=cache.find_tag({hudg},TagGroup{"hudg"});require(hud,"Locale has no typed HUD globals owner");
  const auto unic=Bytes(cache.read_tag_bytes(hud->address+0x3fc,4)).u32(0);
  const auto* unicode=cache.find_tag({unic},TagGroup{"unic"});require(unicode,"Locale has no typed HUD string owner");
  const Bytes range(cache.read_tag_bytes(unicode->address+16,4));Locale out{{},range.u16(0),count,range.u16(2),{}};
  require(out.first<=count && out.selected<=count-out.first,"Actual selected HUD unic range escapes English table");
  const auto append=[&](unsigned n) {
    const auto offset=refs.u32(n*8+4);require(offset<size,"Actual localized string offset escapes its text owner");
    out.strings.emplace(refs.u32(n*8),text.string(offset,size-offset));
  };
  for(unsigned n=0;n<out.selected;++n)append(out.first+n);
  for(unsigned n=0;n<count;++n)append(n);
  out.report={{"count",count},{"reference_offset",indices},{"text_offset",strings},{"text_bytes",size},
      {"reference_sha256",digest(refs.slice(0,refs.size()))},{"text_sha256",digest(text.slice(0,text.size()))},
      {"hudg",hudg},{"unic",unic},{"unic_path",unicode->name},{"first",out.first},{"selected",out.selected}};
  return out;
}
void owners_only(const Cache& cache,const std::array<unsigned,4>& ids,Json& report) {
  unsigned banks{},images{};
  for(const auto& tag:cache.select_tags({.group=tag_groups::bitmap,.require_address=true})) {
    if(!tag.name.starts_with("ui\\hud\\"))continue;
    ++banks;const auto rows=bank(cache,tag.address+0x44,168,256);
    for(unsigned n=0;n<rows.count;++n) {
      const Bytes row(cache.read_tag_bytes(rows.first+n*168,168));
      const auto resource=cache.load_bitmap_resource(tag.datum,n);const auto texture=cache.load_bitmap(tag.datum,n);
      require(row.u32(0)==tag_groups::bitmap.value && resource.type==0 && resource.depth==1 &&
          row.u16(4)==texture.width && row.u16(6)==texture.height && texture.layers==1 && !texture.depth,
          "Source HUD bitmap row/resource/image ownership differs");++images;
    }
  }
  for(auto id:ids) {
    auto texture=cache.load_bitmap(id,0);
    if(texture.format>=14 && texture.format<=16)texture=prepare_xemu_bc_texture(texture);
    report["sensors"].push_back({{"datum",id},{"path",cache.find_tag({id},tag_groups::bitmap)->name},
        {"width",texture.width},{"height",texture.height},{"pixel_sha256",digest(texture.pixels)}});
  }
  report["mode"]="source_owners_only";report["huds"]=0;report["source_hud_banks"]=banks;
  report["source_hud_images"]=images;
}
void inspect(const Cache& cache,unsigned biped,unsigned weapon,const std::array<unsigned,4>& ids,
    const Locale& english,Counts& counts,Json& report) {
  phase="HUD loading";
  const auto hud=cache.load_hud(biped,weapon);++counts.huds;
  require(hud.strings==english.strings,"HUD localized IDs/bytes/range differ from actual source cache table");
  for(const auto& [datum,material]:hud.shaders)if(!material.rendering_gaps.empty())
    report["shader_requirements"][std::to_string(datum)]={
        {"path",material.name},{"requirements",material.rendering_gaps}};
  phase="sensor owner comparison";
  const std::array<const Texture*,4> actual{
      &hud.sensor.artwork.sweep,&hud.sensor.artwork.mask,
      &hud.sensor.artwork.blip,&hud.sensor.artwork.custom};
  for(unsigned n=0;n<ids.size();++n) {
    auto expected=cache.load_bitmap(ids[n],0);
    if(expected.format>=14 && expected.format<=16)expected=prepare_xemu_bc_texture(expected);
    equal(*actual[n],expected);++counts.sensor_images;
    if(counts.huds==1)report["sensors"].push_back({{"datum",ids[n]},
        {"path",cache.find_tag({ids[n]},tag_groups::bitmap)->name},{"width",expected.width},
        {"height",expected.height},{"pixel_sha256",digest(expected.pixels)}});
  }
  for(const auto& [datum,image]:hud.images) {
    phase="UI image owner comparison";
    require(image.images.size()==cache.bitmap_count(datum),"HUD omitted an authored image frame");
    for(unsigned frame=0;frame<image.images.size();++frame) {
      // The independent Cache decoder owns the complete MCC mip chain.
      // Existing UI behavior keeps its top image and expands BC3 only.
      const auto source=cache.load_bitmap(datum,frame);const auto& top=source.mips.at(0);
      Texture expected{source.width,source.height,source.format,
          std::vector<std::byte>(source.pixels.begin()+top.offset,source.pixels.begin()+top.offset+top.size),{top},source.layers,source.depth};
      expected.mips[0].offset=0;
      if(source.format==16)expected=prepare_xemu_bc3_texture(expected);
      equal(image.images[frame],expected);++counts.images;
    }
    phase="sprite owner comparison";counts.sprites+=check_sprites(cache,datum,image);
  }
  for(const auto& widget:hud.widgets)if(widget.bitmap!=null_datum)
    require(hud.images.contains(widget.bitmap),"HUD widget bitmap lost its full-salt image binding");
}
unsigned negatives(Cache& cache,unsigned biped,const std::filesystem::path& map) {
  const auto* globals=cache.find_tag(cache.globals_tag(),tag_groups::globals);
  const auto interfaces=bank(cache,globals->address+0x110,0x98,16);
  const auto sensor=Bytes(cache.read_tag_bytes(interfaces.first+0x3c,4)).u32(0);
  const auto* bitmap=cache.find_tag({sensor},tag_groups::bitmap);
  require(bitmap && bitmap->address,"No actual negative fixture bitmap");
  const auto images=bank(cache,bitmap->address+0x44,168,256);
  // Published decoded textures memoize immutable metadata. Mutate a fresh
  // private owner before its first HUD read so every negative reaches the
  // actual decoder/validator. No file or public runtime storage is changed.
  const auto rejected=[&](unsigned address,unsigned value,unsigned bytes) {
    Cache probe(map);
    const auto view=probe.read_tag_bytes(address,bytes);std::vector<std::byte> before(view.begin(),view.end());
    auto* destination=const_cast<std::byte*>(view.data());
    for(unsigned n=0;n<bytes;++n)destination[n]=std::byte(value>>(n*8u));
    bool failed=false;try{(void)probe.load_hud(biped,null_datum);}catch(const FormatError&){failed=true;}
    std::copy(before.begin(),before.end(),destination);
    require(failed,"Malformed/foreign MCC HUD image was admitted");
  };
  rejected(interfaces.first+0x3c,sensor^0x10000u,4);
  rejected(images.first+0xa,2,2);rejected(images.first+8,2,1);
  rejected(images.first+0x50,0,4);rejected(images.first+0x14,0xffff,2);
  rejected(bitmap->address+0x44,257,4);
  const auto indices=Bytes(cache.read_tag_bytes(globals->address+0x198,4)).u32(0);
  const auto strings=Bytes(cache.read_tag_bytes(globals->address+0x19c,4)).u32(0);
  rejected(globals->address+0x198,(indices&0x3fffffffu)|0x40000000u,4);
  rejected(globals->address+0x19c,strings^0x80000000u,4);
  rejected(globals->address+0x190,65537,4);
  const auto hudg=Bytes(cache.read_tag_bytes(interfaces.first+0x34,4)).u32(0);
  const auto* hud=cache.find_tag({hudg},TagGroup{"hudg"});
  const auto unic=Bytes(cache.read_tag_bytes(hud->address+0x3fc,4)).u32(0);
  rejected(cache.find_tag({unic},TagGroup{"unic"})->address+16,0xffff,2);
  return 10;
}
}
int main(int argc,char** argv)try {
  if(argc<2 || std::string_view(argv[1]).empty())return 77;
  if(argc>3 || (argc==3 && std::string_view(argv[2])!="--negative" &&
      std::string_view(argv[2])!="--owners-only"))return 2;
  Cache cache(argv[1]);require(cache.is_mcc(),"HUD fixture requires actual MCC metadata and DAT resources");
  Json report{{"map",cache.name()},{"sensors",Json::array()}};
  phase="sensor references";
  const auto ids=sensors(cache);const auto* globals=cache.find_tag(cache.globals_tag(),tag_groups::globals);
  phase="independent English locale owner";const auto english=locale(cache,argv[1]);report["locale"]=english.report;
  if(argc==3 && std::string_view(argv[2])=="--owners-only") {
    phase="source bitmap owners";owners_only(cache,ids,report);std::cout<<report.dump()<<'\n';return 0;
  }
  phase="player representations";
  const auto representations=bank(cache,globals->address+0x138,0xbc,16);std::set<unsigned> bipeds;
  for(unsigned n=0;n<representations.count;++n) {
    const auto datum=Bytes(cache.read_tag_bytes(representations.first+n*0xbc+0xb4,4)).u32(0);
    if(const auto* tag=cache.find_tag({datum},tag_groups::biped);tag && tag->address)bipeds.insert(datum);
  }
  require(!bipeds.empty(),"Fixture has no resident player representation");
  unsigned weapon=null_datum;
  for(const auto& tag:cache.select_tags({.group=tag_groups::weapon,.require_address=true}))
    if(tag.name=="objects\\weapons\\rifle\\smg\\smg")weapon=tag.datum;
  if(weapon==null_datum)for(const auto& tag:cache.select_tags({.group=tag_groups::weapon,.require_address=true})) {
    if(tag.size>=0x2b8 && Bytes(cache.read_tag_bytes(tag.address+0x2b4,4)).u32(0)!=null_datum){weapon=tag.datum;break;}
  }
  Counts counts;
  for(auto biped:bipeds) {
    inspect(cache,biped,null_datum,ids,english,counts,report);
    if(weapon!=null_datum)inspect(cache,biped,weapon,ids,english,counts,report);
  }
  report["huds"]=counts.huds;report["images"]=counts.images;report["sprites"]=counts.sprites;
  phase="malformed source negatives";
  report["sensor_images"]=counts.sensor_images;report["negative_cases"]=argc==3?negatives(cache,*bipeds.begin(),argv[1]):0;
  std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<phase<<": "<<e.what()<<'\n';return 1;}
