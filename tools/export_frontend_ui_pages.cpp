#include "halo2/frontend.hpp"
#include "halo2/texture.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>

int main(int argc,char **argv) {
  try {
    if(argc!=4)throw halo2::FormatError("usage: export_frontend_ui_pages mainmenu.map exact-tag-name output-directory");
    const halo2::Cache cache(argv[1]);
    const auto content=cache.load_frontend();
    const auto found=std::find_if(content.images.begin(),content.images.end(),
      [&](const auto &entry){return entry.second.name==argv[2];});
    if(found==content.images.end())throw halo2::FormatError("requested UI image bank is not imported by the original frontend");
    const std::filesystem::path directory(argv[3]);
    std::filesystem::create_directories(directory);
    unsigned index=0;
    for(const auto &original:found->second.images) {
      const auto page=halo2::prepare_xemu_bc_texture(original);
      if(page.format!=11 || page.layers!=1 || page.depth ||
         page.pixels.size()<std::size_t(page.width)*page.height*4)
        throw halo2::FormatError("requested UI page is not a prepared ordinary RGBA bitmap");
      const auto path=directory/("frame-"+std::to_string(index++)+".pam");
      if(std::filesystem::exists(path))throw halo2::FormatError("refusing to overwrite an exported page");
      std::ofstream output(path,std::ios::binary);
      output<<"P7\nWIDTH "<<page.width<<"\nHEIGHT "<<page.height<<"\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n";
      output.write(reinterpret_cast<const char *>(page.pixels.data()),std::size_t(page.width)*page.height*4);
      if(!output.flush())throw halo2::FormatError("cannot complete UI page export");
      std::cout<<index-1<<'\t'<<page.width<<'\t'<<page.height<<'\t'<<path.string()<<'\n';
    }
    return 0;
  } catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}
}
