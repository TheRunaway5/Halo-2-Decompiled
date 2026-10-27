#include "halo2/native_weather_vertex.hpp"
#include "halo2/source_weather_plate_program.hpp"
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <random>

namespace {
using namespace halo2;
void require(bool value,const char* message) {if(!value)throw std::runtime_error(message);}
std::uint64_t compared=0;
void same(const std::array<ShaderVector,16>& a,const std::array<ShaderVector,16>& b) {
    for(unsigned output=0;output<16;++output)for(unsigned axis=0;axis<4;++axis) {
        if(std::bit_cast<std::uint32_t>(a[output][axis])!=std::bit_cast<std::uint32_t>(b[output][axis]))
            throw std::runtime_error("native weather literal output differs at"+std::to_string(output)+":"+std::to_string(axis));
        ++compared;
    }
}
template<std::size_t N>
unsigned mutations(const std::array<std::uint32_t,N>& words,bool plate) {
    auto bytes=std::vector<std::byte>(N*4);std::memcpy(bytes.data(),words.data(),bytes.size());
    unsigned rejected=0;
    for(unsigned i=0;i<bytes.size();++i) {
        bytes[i]^=std::byte{1};
        try {if(plate)native_weather::admit_plate(bytes);else native_weather::admit_point(bytes);}
        catch(const FormatError&) {++rejected;}
        bytes[i]^=std::byte{1};
    }
    require(rejected==bytes.size(),"a changed complete native weather payload was admitted");
    return rejected;
}
}
int main() {try {
    const auto original=[](const auto& words) {
        VertexProgram result;result.count=words[0]>>16;
        for(unsigned i=0;i<result.count;++i)
            std::copy_n(words.begin()+1+i*4,4,result.instructions[i].begin());
        return result;
    };
    const auto point=original(native_weather::point_program);
    const auto plate=original(native_weather::plate_program);
    std::mt19937 random(0x54553135u);
    const auto number=[&] {return float(int(random()%8193)-4096)/256.f;};
    for(unsigned fixture=0;fixture<8192;++fixture) {
        std::array<ShaderVector,12> camera;
        for(auto& row:camera)for(auto& value:row)value=number();
        ShaderVector position;for(auto& value:position)value=number();
        // Explicit paired positive/negative zero inputs supplement the finite
        // random arithmetic and exercise NV zero-times-reciprocal behavior.
        if(fixture%16==0)position[fixture%4]=std::bit_cast<float>((fixture&16)?0x80000000u:0u);
        if(fixture%31==0)camera[3]={0,0,0,0};
        const auto color=random();VertexProgramState oracle;
        std::copy(camera.begin(),camera.end(),oracle.constants.begin());oracle.attributes[0]=position;
        oracle.attributes[1]={float((color>>16)&255)/255.f,float((color>>8)&255)/255.f,
                              float(color&255)/255.f,float(color>>24)/255.f};
        execute_vertex_program(point,oracle);
        same(native_weather::point_vertex(camera,position,color),oracle.outputs);
        std::array<ShaderVector,5> registers;
        std::array<ShaderVector,3> scroll;
        for(auto& row:registers)for(auto& value:row)value=number();
        for(auto& row:scroll)for(auto& value:row)value=number();
        VertexProgramState plate_oracle;
        std::copy(registers.begin(),registers.end(),plate_oracle.attributes.begin());
        std::copy(scroll.begin(),scroll.end(),plate_oracle.constants.begin()+21);
        execute_vertex_program(plate,plate_oracle);
        same(native_weather::plate_vertex(registers,scroll),plate_oracle.outputs);
    }
    unsigned variants=0,poisoned=0;
    for(unsigned power=0;power<8;++power)for(unsigned alpha=0;alpha<2;++alpha) {
        SourceWeatherPlateProgramInput input;input.far_clip=1;input.depth_end=std::ldexp(1.f,-int(power));
        input.compositor.words[0]=alpha;
        for(unsigned i=1;i<13;++i)input.compositor.words[i]=std::bit_cast<std::uint32_t>(float(i)/16.f);
        const auto result=source_weather_plate_program(input);
        require(result.program.has_value(),"original source producer omitted a finite native plate variant");
        require(native_weather::plate_variant(result.program->words)==power*2+alpha,"original plate control tuple chose a different native kernel");
        ++variants;
        auto altered=result.program->words;altered[59]^=1;
        try {(void)native_weather::plate_variant(altered);}
        catch(const FormatError&) {++poisoned;}
    }
    require(variants==16&&poisoned==16,"native weather control identity guard failed");
    const auto rejected=mutations(native_weather::point_program,false)+mutations(native_weather::plate_program,true);
    std::cout<<"Native VS21/VS22: "<<compared<<" exact source-reference words, "<<variants
             <<" source-produced fragment variants, "<<rejected<<" byte mutations and "<<poisoned
             <<" control mutations rejected; production CPU paths use no opcode interpreter\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
