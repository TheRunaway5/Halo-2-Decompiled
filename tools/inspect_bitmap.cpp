#include "halo2/cache.hpp"
#include "halo2/texture.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

int main(int argc,char** argv) {
    try {
        if((argc!=4 && argc!=5) || (argc==5 && std::string_view(argv[4])!="--rgba"))
            throw halo2::FormatError("usage: inspect_bitmap CACHE TAG NEW_OUTPUT_DIRECTORY [--rgba]");
        const halo2::Cache cache(argv[1]);
        const auto output=std::filesystem::absolute(argv[3]);
        if(!std::filesystem::create_directories(output))throw halo2::FormatError("Output directory must be new");
        const halo2::Tag* selected=nullptr;
        for(const auto& tag:cache.tags())if(tag.group=="bitm" && tag.name==argv[2]) {
            if(selected)throw halo2::FormatError("Duplicate bitmap path identity");
            selected=&tag;
        }
        if(!selected)throw halo2::FormatError("Missing bitmap path identity");
        nlohmann::json frames=nlohmann::json::array();
        for(unsigned frame=0;frame<cache.bitmap_count(selected->datum);++frame) {
            auto bitmap=cache.load_bitmap(selected->datum,frame);
            if(argc==5 && bitmap.format>=14 && bitmap.format<=16)
                bitmap=halo2::prepare_xemu_bc_texture(bitmap);
            const auto filename="frame-"+std::to_string(frame)+".pixels";
            std::ofstream pixels(output/filename,std::ios::binary);
            pixels.write(reinterpret_cast<const char*>(bitmap.pixels.data()),std::streamsize(bitmap.pixels.size()));
            if(!pixels)throw halo2::FormatError("Cannot write native bitmap bytes");
            nlohmann::json mips=nlohmann::json::array();
            for(const auto& mip:bitmap.mips)mips.push_back({{"width",mip.width},{"height",mip.height},
                {"depth",mip.depth},{"offset",mip.offset},{"size",mip.size}});
            frames.push_back({{"width",bitmap.width},{"height",bitmap.height},{"format",bitmap.format},
                {"layers",bitmap.layers},{"depth",bitmap.depth},{"pixels",filename},{"mips",mips}});
        }
        const nlohmann::json report={{"cache",std::filesystem::absolute(argv[1]).string()},
            {"tag",selected->name},{"datum",selected->datum},{"frames",frames}};
        std::ofstream manifest(output/"bitmap.json");manifest<<report.dump(2)<<'\n';
        if(!manifest)throw halo2::FormatError("Cannot write native bitmap manifest");
        std::cout<<report.dump()<<'\n';
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
