#include "halo2/frontend.hpp"
#include "halo2/texture.hpp"
#include "halo2/ui_dds.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>

// Decode source resources with the production Xbox UI codec, including its
// linear row pitch, swizzle and pinned BC expansion. No image editor codec.
int main(int argc,char **argv) {
  try {
    if(argc!=2)return 2;
    std::ifstream requests(argv[1]);std::string line;unsigned count{};
    while(std::getline(requests,line)) {
      std::istringstream row(line);unsigned w,h,format,flags;std::string input,output;
      if(!(row>>w>>h>>format>>flags>>std::quoted(input)>>std::quoted(output)))throw std::runtime_error("Invalid decode request");
      std::ifstream file(input,std::ios::binary|std::ios::ate);
      if(!file || file.tellg()<0)throw std::runtime_error("Missing source resource");
      std::vector<std::byte> raw(std::size_t(file.tellg()));file.seekg(0);
      if(!file.read(reinterpret_cast<char*>(raw.data()),raw.size()))throw std::runtime_error("Truncated source resource");
      auto texture=format==65535?halo2::read_ui_dds_texture(halo2::Bytes(raw)):
        halo2::prepare_ui_bitmap(halo2::Bytes(raw),w,h,format,flags);
      if(texture.format>=14 && texture.format<=16)texture=halo2::prepare_xemu_bc_texture(texture);
      if(texture.format!=11 || texture.width!=w || texture.height!=h || texture.pixels.size()<std::size_t(w)*h*4)
        throw std::runtime_error("Unsupported decoded source resource");
      std::ofstream out(output,std::ios::binary);
      if(!out.write(reinterpret_cast<const char*>(texture.pixels.data()),std::size_t(w)*h*4))throw std::runtime_error("Cannot save decoded resource");
      ++count;
    }
    if(!requests.eof())throw std::runtime_error("Cannot read decode requests");
    std::cout<<count<<" original HUD pages decoded with the production UI codec\n";
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
