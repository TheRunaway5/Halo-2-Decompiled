#include "halo2/frontend.hpp"
#include "halo2/frontend_frame_texture.hpp"
#include <iostream>

int main(int argc,char **argv) {
  try {
    std::filesystem::path map,output;
    for(int i=1;i<argc;i+=2) {
      if(i+1>=argc)throw halo2::FormatError("frame edge export option has no value");
      const std::string option=argv[i];
      if(option=="--map" && map.empty())map=argv[i+1];
      else if(option=="--output" && output.empty())output=argv[i+1];
      else throw halo2::FormatError("unknown or duplicate frame edge export option");
    }
    if(map.empty() || output.empty())throw halo2::FormatError(
        "usage: export_frontend_frame_edges --map mainmenu.map --output ui/frame_edges");
    const halo2::Cache cache(map);
    const auto content=cache.load_frontend();
    const halo2::UiImage *frame=nullptr;
    for(const auto &[datum,image]:content.images)
      if(image.name=="ui\\global_bitmaps\\framing_center") {
        if(frame)throw halo2::FormatError("ambiguous original shell frame asset");
        frame=&image;
      }
    if(!frame || frame->images.size()!=1)
      throw halo2::FormatError("missing or unsupported original shell frame page");
    const auto dds=halo2::frontend_frame_dds(frame->images.front());
    std::filesystem::create_directories(output);
    for(const auto name:{"left-edge.dds","right-edge.dds"}) {
      const auto path=output/name;
      if(std::filesystem::exists(path)) {
        std::cout<<"Preserved existing "<<path.string()<<'\n';continue;
      }
      std::ofstream file(path,std::ios::binary);
      if(!file || !file.write(reinterpret_cast<const char *>(dds.data()),std::streamsize(dds.size())) ||
         !(file.flush()))throw halo2::FormatError("cannot export frame edge: "+path.string());
      std::cout<<"Exported "<<path.string()<<" ("<<frame->images.front().width<<'x'
               <<frame->images.front().height<<" DXT3; original compressed bytes unchanged)\n";
    }
    return 0;
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
