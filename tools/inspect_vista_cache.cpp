#include "halo2/cache.hpp"
#include "halo2/source_tag_metadata_store.hpp"
#include "halo2/model.hpp"
#include "halo2/content.hpp"
#include <fstream>
#include <iostream>

int main(int argc,char** argv) {
    try {
        if(argc<2 || argc>4)throw halo2::FormatError("Supply cache [--geometry [vertices.bin] | --models | --lighting | --content | --bitmap PATH]");
        const halo2::Cache cache(argv[1]);
        std::cout<<cache.describe()<<'\n';
        std::size_t resident=0;
        for(const auto& tag:cache.tags())if(tag.address && tag.size && tag.group!="sbsp" && tag.group!="ltmp") {
            const auto metadata=cache.load_source_tag_metadata(tag.datum);
            if(metadata.source_address()!=tag.address || metadata.size()!=tag.size)
                throw halo2::FormatError("native Vista source metadata lost its original allocation");
            ++resident;
        }
        std::cout<<"Resident metadata checked: "<<resident<<"\n";
        if(argc==4 && std::string(argv[2])=="--bitmap") {
            for(const auto& tag:cache.tags())if(tag.group=="bitm" && tag.name==argv[3]) {
                const auto bitmap=cache.load_bitmap(tag.datum,0);
                const auto resource=cache.load_bitmap_resource(tag.datum,0);
                if(resource.width!=bitmap.width || resource.height!=bitmap.height)
                    throw halo2::FormatError("donor texture and residency dimensions differ");
                std::uint64_t hash=14695981039346656037ull;
                for(auto b:bitmap.pixels)hash=(hash^std::to_integer<unsigned>(b))*1099511628211ull;
                std::cout<<"Bitmap "<<tag.name<<" "<<bitmap.width<<"x"<<bitmap.height
                         <<" bytes="<<bitmap.pixels.size()<<" hash="<<std::hex<<hash<<'\n';
                return 0;
            }
            throw halo2::FormatError("bitmap path missing");
        }
        if(argc==3 && std::string(argv[2])=="--lighting") {
            for(std::size_t i=0;i<cache.bsps().size();++i) {
                const auto lighting=cache.load_bsp_lighting(i);
                std::cout<<"BSP "<<i<<" lightmaps: "<<lighting.images.size()
                         <<" buckets: "<<lighting.buckets.size()<<'\n';
            }
            return 0;
        }
        if(argc==3 && std::string(argv[2])=="--content") {
            for(std::size_t i=0;i<cache.bsps().size();++i) {
                const auto content=cache.load_content(i,true);
                std::cout<<content.describe()<<'\n';
                for(const auto& gap:content.limitations)std::cout<<"Unsupported: "<<gap<<'\n';
            }
            return 0;
        }
        if(argc==3 && std::string(argv[2])=="--models") {
            std::size_t models=0,vertices=0,empty=0;
            for(const auto& tag:cache.tags())if(tag.group=="mode" && tag.address) {
                const auto payload=cache.load_source_tag_metadata(tag.datum);
                const halo2::Bytes definition(payload.bytes_at(tag.address,tag.size));
                if(!definition.u32(0x1c) && !definition.u32(0x24)) {++empty;continue;}
                try {
                    const auto model=cache.load_model(tag.datum,false);
                    vertices+=model.mesh.vertices.size();++models;
                } catch(const halo2::FormatError& error) {
                    throw halo2::FormatError(tag.name+": "+error.what());
                }
            }
            std::cout<<"Models: "<<models<<" vertices: "<<vertices<<" authored empty: "<<empty<<"\n";
            return 0;
        }
        if(argc>=3) {
            if(std::string(argv[2])!="--geometry")throw halo2::FormatError("Unknown probe option");
            std::ofstream output;
            if(argc==4)output.open(argv[3],std::ios::binary);
            for(std::size_t i=0;i<cache.bsps().size();++i) {
                const auto mesh=cache.load_bsp(i,false);
                std::cout<<"BSP "<<i<<" vertices: "<<mesh.vertices.size()<<"\n";
                if(output)for(const auto& vertex:mesh.vertices) {
                    const float values[]{vertex.position.x,vertex.position.y,vertex.position.z,
                                         vertex.uv.x,vertex.uv.y};
                    output.write(reinterpret_cast<const char*>(values),sizeof values);
                }
            }
        }
        return 0;
    } catch(const std::exception& error) { std::cerr<<error.what()<<'\n';return 1; }
}
