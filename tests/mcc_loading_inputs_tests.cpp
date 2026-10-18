#include "halo2/mcc_loading_inputs.hpp"
#include <chrono>
#include <fstream>
#include <iostream>

namespace {
using namespace halo2;
using Buffer=std::vector<std::byte>;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void word(Buffer& b,std::size_t at,std::uint32_t value){for(unsigned i=0;i<4;++i)b.at(at+i)=std::byte(value>>(8*i));}
void half(Buffer& b,std::size_t at,std::uint16_t value){for(unsigned i=0;i<2;++i)b.at(at+i)=std::byte(value>>(8*i));}
void save(const std::filesystem::path& path,const Buffer& b){std::ofstream out(path,std::ios::binary);out.write(reinterpret_cast<const char*>(b.data()),std::streamsize(b.size()));check(bool(out),"Loading fixture write failed");}
template<class F>void rejects(F action){try{action();}catch(const FormatError&){return;}throw std::runtime_error("Invalid MCC loading bitmap input was admitted");}
struct Fixture {Buffer map,pixels;};
Fixture fixture() {
  Fixture f{Buffer(12288),{}};auto& b=f.map;
  word(b,0,0x68656164);word(b,4,13);word(b,8,b.size());word(b,16,1024);word(b,32,4);
  word(b,36,2048);word(b,40,1024);word(b,44,3072);word(b,720,1024+0x100000-4096);
  word(b,1024,0x400000+1056);word(b,1028,1);word(b,1032,0x400000+1068);
  word(b,1036,0xffffffffu);word(b,1040,0xffffffffu);word(b,1048,4);word(b,1052,0x74616773);
  word(b,1056,0x6269746d);word(b,1060,0xffffffffu);word(b,1064,0xffffffffu);
  constexpr const char* names[]{"globals\\loading_screen\\loading_screen",
    "globals\\loading_screen\\loading_screen_logos",
    "globals\\loading_screen\\loading_screen_spinners",
    "globals\\loading_screen\\loading_screen_sweep"};
  for(unsigned i=0;i<4;++i) {
    const unsigned count=i==2?2:1,root=4096+i*1024,address=0x100000+i*1024;
    word(b,1068+i*16,0x6269746d);word(b,1072+i*16,0xe1000000u+i*0x1000001u);
    word(b,1076+i*16,address);word(b,1080+i*16,80+168*count);word(b,3072+i*4,i*256);
    for(unsigned n=0;names[i][n];++n)b[2048+i*256+n]=std::byte(names[i][n]);
    word(b,root+68,count);word(b,root+72,address+80);
    for(unsigned j=0;j<count;++j) {
      const auto record=root+80+168*j;unsigned format=11;Buffer pixels(64);
      for(unsigned pixel=0;pixel<16;++pixel){pixels[pixel*4]=std::byte{0x30};pixels[pixel*4+1]=std::byte{0x20};pixels[pixel*4+2]=std::byte{0x10};pixels[pixel*4+3]=std::byte{0xaa};}
      if(i==2 && j==0){format=14;pixels=Buffer(8);half(pixels,0,0xf800);}
      if(i==2 && j==1){format=15;pixels=Buffer(16);for(unsigned k=0;k<8;++k)pixels[k]=std::byte{0xff};half(pixels,8,0x001f);}
      if(i==3){format=1;pixels=Buffer(16,std::byte{0x7f});}
      half(b,record+4,4);half(b,record+6,4);b[record+8]=std::byte{1};half(b,record+12,format);
      word(b,record+56,0x80000000u+f.pixels.size());word(b,record+80,pixels.size()+8);
      const auto at=f.pixels.size();f.pixels.resize(at+8);word(f.pixels,at,1);word(f.pixels,at+4,std::uint32_t(-std::int32_t(pixels.size())));
      f.pixels.insert(f.pixels.end(),pixels.begin(),pixels.end());
    }
  }
  return f;
}
}
int main(int argc,char** argv) {
  const auto root=std::filesystem::temp_directory_path()/("halo2-mcc-loading-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  try {
    std::filesystem::create_directories(root);const auto original=fixture();save(root/"mainmenu.map",original.map);save(root/"textures.dat",original.pixels);
    MccLoadingBitmapInputs inputs;
    {Cache cache(root/"mainmenu.map");inputs=load_mcc_loading_bitmap_inputs(cache);}
    // Loading image ownership must outlive the Cache and its mapped source.
    std::filesystem::remove(root/"mainmenu.map");std::filesystem::remove(root/"textures.dat");
    constexpr unsigned slots[]{0,1,3,4,5},indices[]{0,1,2,2,3},frames[]{0,0,0,1,0};
    constexpr std::array<unsigned,4> colors[]{{0x10,0x20,0x30,0xaa},{0x10,0x20,0x30,0xaa},{255,0,0,255},{0,0,255,255},{127,127,127,255}};
    for(unsigned i=0;i<inputs.size();++i) {
      const auto& input=inputs[i];const auto& image=input.image;
      check(input.texture_slot==slots[i] && input.frame==frames[i],"Loading draw input identity changed");
      check(input.handle.value==0xe1000000u+indices[i]*0x1000001u,"Loading tag salt was lost");
      check(image.source_width==4 && image.source_height==4 && image.texture.width==4 && image.texture.height==4 && image.texture.format==11,"Loading source shape was resized or storage was not decoded");
      check(image.texture.pixels.size()==64 && image.texture.mips.size()==1,"Loading image allocation extent changed");
      for(unsigned p=0;p<16;++p)for(unsigned c=0;c<4;++c)check(std::to_integer<unsigned>(image.texture.pixels[p*4+c])==colors[i][c],"MCC loading BC/luma/RGBA pixels changed");
    }
    save(root/"textures.dat",original.pixels);
    auto changed=original.map;changed[2048]=std::byte{'x'};save(root/"mainmenu.map",changed);rejects([&]{Cache cache(root/"mainmenu.map");(void)load_mcc_loading_bitmap_inputs(cache);});
    changed=original.map;word(changed,4096+2*1024+68,1);save(root/"mainmenu.map",changed);rejects([&]{Cache cache(root/"mainmenu.map");(void)load_mcc_loading_bitmap_inputs(cache);});
    changed=original.map;half(changed,4096+80+10,1);save(root/"mainmenu.map",changed);rejects([&]{Cache cache(root/"mainmenu.map");(void)load_mcc_loading_bitmap_inputs(cache);});
    unsigned imported=0;
    for(int i=1;i<argc;++i) {
      Cache cache(argv[i]);const auto actual=load_mcc_loading_bitmap_inputs(cache);
      constexpr std::array<unsigned,2> shapes[]{{1024,512},{512,64},{512,512},{512,512},{2048,4}};
      for(unsigned j=0;j<actual.size();++j) {
        const auto& input=actual[j];check(input.texture_slot==slots[j] && input.frame==frames[j],"Installed MCC loading bitmap role changed");
        check(input.image.source_width==shapes[j][0] && input.image.source_height==shapes[j][1],"Installed MCC authored loading dimensions changed");
        check(cache.find_tag(input.handle,TagGroup{"bitm"}),"Installed MCC loading input lost its directory owner");
      }
      ++imported;
    }
    std::filesystem::remove_all(root);std::cout<<"Five MCC loading bitmap roles, BC1/BC2/luma/RGBA samples, independent ownership, three malformed controls and "<<imported<<" installed MCC banks passed\n";return 0;
  }catch(const std::exception& error){std::filesystem::remove_all(root);std::cerr<<error.what()<<'\n';return 1;}
}
