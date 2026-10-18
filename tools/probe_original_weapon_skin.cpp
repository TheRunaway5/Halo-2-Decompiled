#include "model_skin_vertices.hpp"
#include <bit>
#include <fstream>
#include <iostream>

namespace {
template<class T> void read(std::istream& in,T* value,std::size_t count=1) {
    in.read(reinterpret_cast<char*>(value),std::streamsize(sizeof(T)*count));
    if(!in)throw halo2::FormatError("Truncated original skin input");
}
template<class T> void write(std::ostream& out,const T* value,std::size_t count=1) {
    out.write(reinterpret_cast<const char*>(value),std::streamsize(sizeof(T)*count));
    if(!out)throw halo2::FormatError("Incomplete native skin output");
}
void output(std::ostream& stream,const halo2::Vertex& vertex) {
    for(const auto value:{vertex.position,vertex.normal,vertex.tangent,vertex.binormal}) {
        const float words[]={value.x,value.y,value.z};write(stream,words,3);
    }
}
}
int main(int argc,char** argv) {
    try {
        static_assert(std::endian::native==std::endian::little);
        if(argc!=3)throw halo2::FormatError("Supply original skin input and native output paths");
        std::ifstream in(argv[1],std::ios::binary);
        std::ofstream out(argv[2],std::ios::binary|std::ios::trunc);
        std::uint32_t magic{},cases{};read(in,&magic);read(in,&cases);
        if(magic!=0x534b4e31 || cases>128)throw halo2::FormatError("Invalid original skin cases");
        write(out,&magic);write(out,&cases);
        for(unsigned c=0;c<cases;++c) {
            std::uint32_t event{},nodes{},count{};
            read(in,&event);read(in,&nodes);read(in,&count);
            if(nodes>256 || count>65536)throw halo2::FormatError("Original skin bank exceeds source capacity");
            std::vector<std::array<float,12>> palette(nodes);read(in,palette.data(),nodes);
            write(out,&event);write(out,&count);
            for(unsigned n=0;n<count;++n) {
                std::uint32_t ordinal{},active{},bones[4]{};std::uint8_t stored[4]{};
                read(in,&ordinal);read(in,&active);read(in,bones,4);read(in,stored,4);
                halo2::Vertex vertex{};
                for(auto* value:{&vertex.position,&vertex.normal,&vertex.tangent,&vertex.binormal}) {
                    float words[3]{};read(in,words,3);*value={words[0],words[1],words[2]};
                }
                halo2::SkinBinding binding{};
                binding.weights=halo2::source_model_skin_weights(stored,active);
                for(unsigned i=0;i<4;++i)binding.bones[i]=std::uint16_t(bones[i]);
                auto scalar=vertex,simd=vertex;
                halo2::skin_model_vertices_scalar(std::span(&scalar,1),std::span(&binding,1),palette);
                halo2::skin_model_vertices(std::span(&simd,1),std::span(&binding,1),palette,true);
                write(out,&ordinal);write(out,binding.weights.data(),4);output(out,scalar);output(out,simd);
            }
        }
        if(in.peek()!=std::char_traits<char>::eof())throw halo2::FormatError("Trailing original skin input");
        std::cout<<"Evaluated "<<cases<<" observed original skin draws through scalar/SIMD production functions\n";
        return 0;
    }catch(const std::exception& error) {
        std::cerr<<error.what()<<'\n';return 1;
    }
}
