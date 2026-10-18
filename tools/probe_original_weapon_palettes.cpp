#include "halo2/source_model_palette.hpp"
#include <bit>
#include <fstream>
#include <iostream>

namespace {
template<class T> void read(std::istream& in,T* value,std::size_t count=1) {
    in.read(reinterpret_cast<char*>(value),std::streamsize(sizeof(T)*count));
    if(!in)throw halo2::FormatError("Truncated original palette input");
}
template<class T> void write(std::ostream& out,const T* value,std::size_t count=1) {
    out.write(reinterpret_cast<const char*>(value),std::streamsize(sizeof(T)*count));
    if(!out)throw halo2::FormatError("Incomplete native palette output");
}
}
int main(int argc,char** argv) {
    try {
        static_assert(std::endian::native==std::endian::little);
        static_assert(sizeof(halo2::Transform)==52);
        static_assert(sizeof(halo2::SourceModelCompoundNode)==16);
        if(argc!=3)throw halo2::FormatError("Supply palette input and output paths");
        std::ifstream in(argv[1],std::ios::binary);
        std::ofstream out(argv[2],std::ios::binary|std::ios::trunc);
        std::uint32_t magic{},count{};read(in,&magic);read(in,&count);
        if(magic!=0x50414c32u || count>128)throw halo2::FormatError("Invalid original palette cases");
        write(out,&magic);write(out,&count);
        for(unsigned i=0;i<count;++i) {
            std::uint32_t id{},model{},flags{},lod{},nodes{},regions{},compounds{};
            read(in,&id);read(in,&model);read(in,&flags);read(in,&lod);
            read(in,&nodes);read(in,&regions);read(in,&compounds);
            if(lod>=6 || nodes>256 || regions>16 || compounds>256-nodes)
                throw halo2::FormatError("Original palette case exceeds source capacity");
            halo2::SourceModelPaletteDefinition definition;
            definition.model_flags=std::uint16_t(flags);definition.region_count=regions;
            definition.compound_node_counts[lod]=compounds;
            std::vector<halo2::Transform> world(nodes);
            definition.inverse_binds.resize(nodes);definition.compounds[lod].resize(compounds);
            read(in,world.data(),nodes);read(in,definition.inverse_binds.data(),nodes);
            read(in,definition.compounds[lod].data(),compounds);
            std::vector<halo2::SourceModelPaletteRegion> selections(regions);
            for(auto& selection:selections) {
                std::uint32_t section{},fallback{},size{};
                read(in,&section);read(in,&fallback);read(in,&size);
                if(section>255 || fallback>255 || size>256)
                    throw halo2::FormatError("Original palette selection exceeds source bytes");
                selection.selected_section=std::uint8_t(section);
                selection.no_map_node_count=std::uint8_t(fallback);
                selection.node_map.resize(size);read(in,selection.node_map.data(),size);
            }
            const auto palette=halo2::source_model_palette(definition,lod,world,selections);
            const std::uint32_t header[]={id,model,palette.base_count,
                std::uint32_t(palette.regions.size()),std::uint32_t(palette.matrices.size())};
            write(out,header,5);
            for(const auto& region:palette.regions) {
                const std::uint32_t range[]={region.first,region.count};write(out,range,2);
            }
            write(out,palette.matrices.data(),palette.matrices.size());
        }
        if(in.peek()!=std::char_traits<char>::eof())throw halo2::FormatError("Trailing original palette data");
        std::cout<<"Compared "<<count<<" original live palette inputs through native source_model_palette\n";
        return 0;
    }catch(const std::exception& error) {
        std::cerr<<error.what()<<'\n';return 1;
    }
}
