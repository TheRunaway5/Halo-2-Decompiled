#include "halo2/frontend_ui_assets.hpp"
#include "halo2/frontend.hpp"
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stdexcept>

using namespace halo2;
namespace {
void check(bool value,const char *message) {
  if (!value) throw std::runtime_error(message);
}
template<class F> void rejects(F call,const char *message) {
  try { call(); } catch (const FormatError &) { return; }
  throw std::runtime_error(message);
}
struct Temporary {
  std::filesystem::path path;
  Temporary() {
    const auto tick=std::chrono::steady_clock::now().time_since_epoch().count();
    for (unsigned i=0;i<100;++i) {
      path=std::filesystem::temp_directory_path()/
          ("halo2-ui-assets-"+std::to_string(tick)+"-"+std::to_string(i));
      if (std::filesystem::create_directory(path)) return;
    }
    throw std::runtime_error("cannot create isolated UI asset test directory");
  }
  ~Temporary() { std::error_code ignored;std::filesystem::remove_all(path,ignored); }
};
void write(const std::filesystem::path &path,std::span<const std::byte> data) {
  std::ofstream file(path,std::ios::binary);
  check(bool(file.write(reinterpret_cast<const char *>(data.data()),std::streamsize(data.size()))),
        "cannot write UI asset test fixture");
}
void write(const std::filesystem::path &path,const std::string &text) {
  write(path,std::as_bytes(std::span(text)));
}
void word(std::vector<std::byte> &data,unsigned offset,std::uint32_t value) {
  for (unsigned i=0;i<4;++i) data.at(offset+i)=std::byte((value>>(8*i))&255);
}
std::vector<std::byte> bytes(std::initializer_list<unsigned> values) {
  std::vector<std::byte> result;
  for (auto value:values) result.push_back(std::byte(value));
  return result;
}
std::vector<std::byte> dds(unsigned format,unsigned width,unsigned height,
                           unsigned mips,std::vector<std::byte> payload) {
  std::vector<std::byte> result(128);
  const bool compressed=format==14 || format==15;
  const unsigned bpp=format<=1?1:format==3 || format==9?2:4;
  word(result,0,0x20534444);word(result,4,124);
  word(result,8,0x1007|(compressed?0x80000:8)|(mips>1?0x20000:0));
  word(result,12,height);word(result,16,width);
  word(result,20,compressed?((width+3)/4)*((height+3)/4)*(format==14?8:16):width*bpp);
  word(result,28,mips);word(result,76,32);
  if (compressed) {
    word(result,80,4);word(result,84,format==14?0x31545844:0x33545844);
  } else if (format==10 || format==11) {
    word(result,80,format==11?0x41:0x40);word(result,88,32);
    word(result,92,0xff0000);word(result,96,0xff00);word(result,100,0xff);
    word(result,104,format==11?0xff000000:0);
  } else if (format==9) {
    word(result,80,0x41);word(result,88,16);word(result,92,0xf00);
    word(result,96,0xf0);word(result,100,0xf);word(result,104,0xf000);
  } else if (format==3) {
    word(result,80,0x20001);word(result,88,16);word(result,92,0xff);word(result,104,0xff00);
  } else if (format==1) {
    word(result,80,0x20000);word(result,88,8);word(result,92,0xff);
  } else if (format==0) {
    word(result,80,2);word(result,88,8);word(result,104,0xff);
  } else throw std::runtime_error("invalid synthetic DDS source format");
  word(result,108,0x1000|(mips>1?0x400008:0));
  result.insert(result.end(),payload.begin(),payload.end());
  return result;
}
Texture stock(unsigned width,unsigned height) {
  Texture result{width,height,11,std::vector<std::byte>(width*height*4,std::byte{0x7f}),{}};
  result.mips.push_back({width,height,0,result.pixels.size()});
  return result;
}
constexpr const char *tag="ui\\test\\plate";
std::string row(unsigned frame,unsigned count,unsigned width,unsigned height,
                const std::string &path="page.dds",const std::string &name=tag) {
  return name+'\t'+std::to_string(frame)+'\t'+std::to_string(count)+'\t'+
         std::to_string(width)+'\t'+std::to_string(height)+'\t'+path+'\n';
}
void manifest(const std::filesystem::path &root,const std::string &rows) {
  write(root/"pack.tsv","H2UI_TEXTURE_PACK_V1\n"+rows);
}
FrontendContent content(unsigned frames=1) {
  FrontendContent result;
  UiImage image;image.name=tag;
  image.images.push_back(stock(2,2));
  if (frames==2) image.images.push_back(stock(3,1));
  image.sequences={{{0,.25f,.75f,.1f,.9f,{.2f,.3f}}}};
  result.images.emplace(42,std::move(image));
  return result;
}
Texture load(const std::filesystem::path &root,const std::vector<std::byte> &file,
             unsigned logical_width=2,unsigned logical_height=2) {
  auto source=content();
  source.images.at(42).images[0]=stock(logical_width,logical_height);
  write(root/"page.dds",file);manifest(root,row(0,1,logical_width,logical_height));
  return load_frontend_ui_assets(source,root).at(tag).at(0);
}
void pixels(const Texture &texture,std::initializer_list<unsigned> expected) {
  check(texture.format==11 && texture.pixels==bytes(expected),
        "DDS source channels were decoded incorrectly");
}
void formats(const std::filesystem::path &root) {
  pixels(load(root,dds(11,1,1,1,bytes({0x12,0x34,0x56,0x78}))),{0x56,0x34,0x12,0x78});
  pixels(load(root,dds(10,1,1,1,bytes({0x12,0x34,0x56,0x78}))),{0x56,0x34,0x12,255});
  pixels(load(root,dds(9,1,1,1,bytes({0x23,0xa1}))),{17,34,51,170});
  pixels(load(root,dds(3,2,1,1,bytes({10,20,30,40})),2,1),{10,10,10,20,30,30,30,40});
  pixels(load(root,dds(1,1,1,1,bytes({123}))),{123,123,123,255});
  pixels(load(root,dds(0,1,1,1,bytes({123}))),{255,255,255,123});
  for (unsigned format:{14u,15u}) {
    const unsigned size=format==14?8:16;
    std::vector<std::byte> payload(size*3);
    for (unsigned i=0;i<payload.size();++i) payload[i]=std::byte(i);
    const auto result=load(root,dds(format,4,4,3,payload));
    check(result.width==4 && result.height==4 && result.format==format &&
          result.pixels==payload && result.mips.size()==3,
          "compressed DDS bank or mip bytes were changed");
    for (unsigned i=0;i<3;++i)
      check(result.mips[i].width==(4u>>i) && result.mips[i].height==(4u>>i) &&
            result.mips[i].offset==i*size && result.mips[i].size==size &&
            result.mips[i].depth==1,"compressed DDS mip layout changed");
  }
  std::vector<std::byte> payload(20);
  for (unsigned i=0;i<payload.size();++i) payload[i]=std::byte(i);
  const auto mip=load(root,dds(11,2,2,2,payload));
  check(mip.mips.size()==2 && mip.mips[1].width==1 && mip.mips[1].height==1 &&
        mip.mips[1].offset==16 && mip.mips[1].size==4 &&
        mip.pixels[16]==std::byte{18} && mip.pixels[17]==std::byte{17} &&
        mip.pixels[18]==std::byte{16} && mip.pixels[19]==std::byte{19},
        "linear DDS mip chain was dropped or decoded with wrong channel order");
  const auto odd=load(root,dds(14,3,5,1,std::vector<std::byte>(16)),3,5);
  check(odd.width==3 && odd.height==5 && odd.pixels.size()==16,
        "partial compressed blocks use incorrect payload extents");
}
void banks_and_paths(const std::filesystem::path &root,const std::filesystem::path &outside) {
  auto source=content(2);
  const auto before=source.images.at(42);
  check(load_frontend_ui_assets(source,{}).empty(), "empty optional asset path is not disabled");
  write(root/"page.dds",dds(11,4,4,1,std::vector<std::byte>(64)));
  write(root/"second.dds",dds(1,6,2,1,std::vector<std::byte>(12,std::byte{42})));
  manifest(root,row(1,2,3,1,"second.dds")+row(0,2,2,2));
  const auto result=load_frontend_ui_assets(source,root);
  check(result.size()==1 && result.at(tag).size()==2 && result.at(tag)[0].width==4 &&
        result.at(tag)[0].height==4 && result.at(tag)[1].width==6,
        "physical DDS pages were not returned in original bank index order");
  const auto &after=source.images.at(42);
  check(after.name==before.name && after.images.size()==before.images.size() &&
        after.sequences[0][0].left==.25f && after.sequences[0][0].registration.x==.2f,
        "texture-only pack changed original image metadata");
  for (unsigned i=0;i<after.images.size();++i)
    check(after.images[i].width==before.images[i].width &&
          after.images[i].height==before.images[i].height &&
          after.images[i].pixels==before.images[i].pixels,
          "texture-only pack changed original logical pixels or dimensions");
  write(root/"wrong-aspect.dds",dds(11,4,2,1,std::vector<std::byte>(32)));
  manifest(root,row(0,2,2,2,"wrong-aspect.dds")+row(1,2,3,1,"second.dds"));
  rejects([&]{load_frontend_ui_assets(source,root);},"valid DDS with wrong original aspect admitted");
  write(root/"mixed-density.dds",dds(1,3,1,1,std::vector<std::byte>(3)));
  manifest(root,row(0,2,2,2)+row(1,2,3,1,"mixed-density.dds"));
  rejects([&]{load_frontend_ui_assets(source,root);},"mixed isotropic bank densities admitted");
  auto bad=[&](const std::string &rows) {
    manifest(root,rows);
    rejects([&]{load_frontend_ui_assets(source,root);},"malformed original bank metadata admitted");
  };
  bad(row(0,2,2,2));
  bad(row(0,2,2,2)+row(0,2,2,2));
  bad(row(0,1,2,2));
  bad(row(2,2,2,2));
  bad(row(0,2,4,4)+row(1,2,3,1,"second.dds"));
  bad(row(0,2,2,2)+row(1,2,3,2,"second.dds"));
  bad(row(0,1,2,2,"page.dds","ui\\missing"));
  bad(row(0,1,2,2,"page.dds","ui\\FrAmInG_center"));
  bad(std::string(tag)+"\t0\t2\t2\t2\tpage.dds\textra\n");
  bad(std::string(tag)+"\t-1\t2\t2\t2\tpage.dds\n");
  bad(std::string(tag)+"\t0\t4294967296\t2\t2\tpage.dds\n");
  write(root/"pack.tsv","UNSUPPORTED\n");
  rejects([&]{load_frontend_ui_assets(source,root);},"unknown pack version admitted");
  const auto one=content();
  for (const auto &path:std::vector<std::string>{"../outside.dds","./page.dds",
       "/absolute.dds","C:/absolute.dds","C:\\absolute.dds","missing.dds"}) {
    manifest(root,row(0,1,2,2,path));
    rejects([&]{load_frontend_ui_assets(one,root);},"unsafe DDS path admitted");
  }
  write(outside,dds(1,1,1,1,bytes({10})));
  std::filesystem::create_symlink(outside,root/"escape.dds");
  manifest(root,row(0,1,2,2,"escape.dds"));
  rejects([&]{load_frontend_ui_assets(one,root);},"DDS symlink escaped pack root");
  std::filesystem::create_symlink(root/"page.dds",root/"inside.dds");
  manifest(root,row(0,1,2,2,"inside.dds"));
  check(load_frontend_ui_assets(one,root).size()==1,"contained DDS symlink rejected");
  auto ambiguous=one;
  ambiguous.images.emplace(43,ambiguous.images.at(42));
  manifest(root,row(0,1,2,2));
  rejects([&]{load_frontend_ui_assets(ambiguous,root);},"ambiguous original bitmap tag admitted");
}
void malformed_dds(const std::filesystem::path &root) {
  const auto valid=dds(11,2,2,1,std::vector<std::byte>(16));
  auto mutation=[&](unsigned offset,unsigned value) {
    auto file=valid;word(file,offset,value);
    rejects([&]{load(root,file);},"unsupported or malformed DDS header admitted");
  };
  mutation(0,0);mutation(4,123);mutation(76,31);mutation(8,0x1006);
  mutation(8,0x80100f);mutation(12,0);mutation(16,4097);
  mutation(20,12);mutation(24,1);mutation(28,4);mutation(32,1);
  mutation(80,0x40);mutation(84,0x30315844);mutation(92,0xff);
  mutation(108,0);mutation(112,0x200);mutation(116,1);mutation(120,1);mutation(124,1);
  auto too_short=valid;too_short.pop_back();
  rejects([&]{load(root,too_short);},"truncated DDS payload admitted");
  auto too_long=valid;too_long.push_back(std::byte{});
  rejects([&]{load(root,too_long);},"trailing DDS payload admitted");
  rejects([&]{load(root,std::vector<std::byte>(127));},"truncated DDS header admitted");
  auto dx10=dds(14,4,4,1,std::vector<std::byte>(8));word(dx10,84,0x30315844);
  rejects([&]{load(root,dx10);},"DX10 DDS admitted");
  auto dxt5=dx10;word(dxt5,84,0x35545844);
  rejects([&]{load(root,dxt5);},"unsupported DXT5 DDS admitted");
  auto mip=dds(14,4,4,3,std::vector<std::byte>(24));word(mip,108,0x1000);
  rejects([&]{load(root,mip);},"DDS multi-mip chain without mip caps admitted");
  mip=dds(14,4,4,3,std::vector<std::byte>(24));word(mip,8,0x81007);
  rejects([&]{load(root,mip);},"DDS multi-mip chain without count flag admitted");
}
void library_subset(const std::filesystem::path &root) {
  const auto source=content();
  write(root/"page.dds",dds(11,4,4,1,std::vector<std::byte>(64)));
  write(root/"hud.tsv","H2UI_SAMPLING_PACK_V2\n"+row(0,1,2,2)+
        row(0,1,2,2,"absent.dds","ui\\hud\\unused_weapon"));
  const auto result=load_ui_asset_library(source.images,root,"hud.tsv");
  check(result.size()==1 && result.at(tag)[0].width==4,
        "shared UI library did not select only the resident HUD images");
  const auto alignment=std::string("H2UI_HUD_ALIGNMENT_V1\n")+tag+"\t0\t0.00390625\t0.03125\n";
  write(root/"hud-alignment.tsv",alignment);
  const auto offsets=load_hud_asset_sample_offsets(result,root);
  check(offsets.size()==1 && offsets.at({tag,0})==std::array{.00390625f,.03125f},
        "HUD artwork alignment did not select the admitted frame");
  for(const auto &bad:{alignment+alignment.substr(alignment.find('\n')+1),
      std::string("H2UI_HUD_ALIGNMENT_V1\n")+tag+"\t1\t0\t0\n",
      std::string("H2UI_HUD_ALIGNMENT_V1\n")+tag+"\t0\tnan\t0\n",
      std::string("H2UI_HUD_ALIGNMENT_V1\n")+tag+"\t0\t1\t0\n"}) {
    write(root/"hud-alignment.tsv",bad);
    rejects([&]{load_hud_asset_sample_offsets(result,root);},"Invalid HUD artwork alignment admitted");
  }
  std::filesystem::remove(root/"hud-alignment.tsv");
  write(root/"hud.tsv","H2UI_SAMPLING_PACK_V2\n"+
        row(0,1,2,2,"page.dds","objects\\weapons\\diffuse"));
  rejects([&]{load_ui_asset_library(source.images,root,"hud.tsv");},
          "UI library admitted world texture replacement");
  write(root/"hud.tsv","H2UI_SAMPLING_PACK_V2\n"+row(0,1,4,4));
  rejects([&]{load_ui_asset_library(source.images,root,"hud.tsv");},
          "UI library changed logical HUD geometry");
}
void custom_ranks(const std::filesystem::path &root) {
  const auto bank=root/"cache/presentation/custom-ranks";
  std::filesystem::create_directories(bank/"bitmaps");
  const auto font=root/"cache/halo2/h2_fonts";
  std::filesystem::create_directories(font);
  const auto data=dds(11,4,4,1,std::vector<std::byte>(64,std::byte{0xff}));
  write(bank/"bitmaps/page.dds",data);
  std::array<unsigned char,32> digest{};unsigned size{};
  check(EVP_Digest(data.data(),data.size(),digest.data(),&size,EVP_sha256(),nullptr)==1,
      "Cannot hash custom rank fixture");
  constexpr char hex[]="0123456789abcdef";std::string hash;
  for(auto value:digest){hash+=hex[value>>4];hash+=hex[value&15];}
  FrontendContent source;source.source_fonts=font;
  nlohmann::json pages=nlohmann::json::array();
  unsigned datum=1;
  for(const auto *name:{"ui\\global_bitmaps\\rank_icons","ui\\global_bitmaps\\rank_icons_sm"}) {
    UiImage image;image.name=name;
    image.images.assign(50,stock(2,2));
    source.images.emplace(datum++,std::move(image));
    for(unsigned frame=43;frame<50;++frame)
      pages.push_back({{"name",name},{"frame",frame},{"logical",{2,2}},{"physical",{4,4}},
          {"path","bitmaps/page.dds"},{"bytes",data.size()},{"sha256",hash}});
  }
  nlohmann::json metadata={{"schema","H2_PORT_CUSTOM_RANKS_V1"},{"complete",true},
      {"original_logical_geometry",true},{"pages",pages}};
  const auto save=[&]{write(bank/"manifest.json",metadata.dump());};save();
  const auto result=load_frontend_ui_assets(source,{});
  check(result.size()==2,"Automatic custom celestial banks were not loaded");
  for(const auto &[id,image]:source.images) {
    const auto &sampled=result.at(image.name);
    for(unsigned frame=0;frame<50;++frame) {
      check(sampled[frame].width==(frame<43?2u:4u),"Custom art replaced a numbered rank or missed a celestial slot");
      check(image.images[frame].width==2,"Custom art changed original bank geometry");
    }
  }
  // An optional numeric bank survives; celestial pages retain the explicit
  // default custom owner even when another texture pack is selected.
  const auto optional=root/"numeric-pack";std::filesystem::create_directory(optional);
  write(optional/"page.dds",dds(11,6,6,1,std::vector<std::byte>(144)));
  std::string rows;
  for(unsigned frame=0;frame<50;++frame)
    rows+=row(frame,50,2,2,"page.dds","ui\\global_bitmaps\\rank_icons");
  manifest(optional,rows);
  const auto selected=load_frontend_ui_assets(source,optional);
  check(selected.at("ui\\global_bitmaps\\rank_icons")[0].width==6 &&
      selected.at("ui\\global_bitmaps\\rank_icons")[43].width==4,
      "Custom ranks did not preserve selected numeric art and override celestial art");
  metadata["pages"][0]["frame"]=42;save();
  rejects([&]{load_frontend_ui_assets(source,{});},"Custom bank admitted a numbered-rank replacement");
  metadata["pages"]=pages;metadata["pages"][0]["sha256"]=std::string(64,'0');save();
  rejects([&]{load_frontend_ui_assets(source,{});},"Custom bank admitted changed DDS bytes");
  metadata["pages"]=pages;metadata["pages"][0]["logical"]={2,3};save();
  rejects([&]{load_frontend_ui_assets(source,{});},"Custom bank admitted changed logical geometry");
  metadata["pages"]=pages;metadata["pages"].erase(0);save();
  rejects([&]{load_frontend_ui_assets(source,{});},"Custom bank admitted an incomplete celestial selection");
  std::filesystem::remove_all(root/"cache");
}
void real_pack(const std::filesystem::path &map,const std::filesystem::path &directory) {
  const Cache cache(map);
  const auto source=cache.load_frontend();
  const auto result=load_frontend_ui_assets(source,directory);
  check(!result.empty(),"actual UI asset pack is empty");
  unsigned frames{},enlarged{},mipped{};
  for (const auto &[name,textures]:result) {
    const UiImage *image{};
    for (const auto &[datum,candidate]:source.images)
      if (candidate.name==name) image=&candidate;
    check(image && textures.size()==image->images.size(),"actual original bank mapping differs");
    for (unsigned i=0;i<textures.size();++i) {
      const auto &texture=textures[i];
      check(texture.layers==1 && !texture.depth && !texture.mips.empty(),
            "actual DDS page lost its 2D mip chain");
      if (texture.width>image->images[i].width || texture.height>image->images[i].height)
        ++enlarged;
      if (texture.mips.size()>1) ++mipped;
      ++frames;
    }
  }
  check(enlarged && mipped,"actual asset pack lacks HD pages or authored mip chains");
  std::cout<<"Actual texture-only UI asset pack: "<<result.size()<<" banks, "
           <<frames<<" pages, "<<enlarged<<" enlarged, "<<mipped<<" with mip chains\n";
}
} // namespace
int main(int argc,char **argv) {
  if (argc!=1 && argc!=3) return 77;
  if (argc==3 && (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])))
    return 77;
  try {
    Temporary temporary;
    const auto root=temporary.path/"pack";
    std::filesystem::create_directory(root);
    formats(root);
    banks_and_paths(root,temporary.path/"outside.dds");
    malformed_dds(root);
    library_subset(root);
    custom_ranks(temporary.path);
    if (argc==3) real_pack(argv[1],argv[2]);
    std::cout<<"UI asset pack formats, mip chains, original banks, paths and malformed metadata passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr<<error.what()<<'\n';return 1;
  }
}
