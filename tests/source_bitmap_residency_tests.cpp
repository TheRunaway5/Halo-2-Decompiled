#include "halo2/source_bitmap_residency.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace halo2;
namespace {
std::uint32_t bits(float f){return std::bit_cast<std::uint32_t>(f);}
float real(std::uint32_t x){return std::bit_cast<float>(x);}
std::vector<std::uint32_t> evaluate(std::span<const std::uint32_t> words){
    if(words.size()==24 || words.size()==29){
        SourceBitmapResidentState state;state.flags=std::uint8_t(words[6]);state.bias=std::uint8_t(words[7]);
        state.cutoff=real(words[8]);state.cached_texture=words[9];state.cached_until=std::bit_cast<std::int32_t>(words[10]);
        SourceBitmapResidentContext context{words[1],bool(words[2]),bool(words[11]),real(words[3]),
            std::bit_cast<std::int32_t>(words[4]),std::bit_cast<std::int32_t>(words[5])};
        for(unsigned i=0;i<3;++i){const auto p=12+i*4;
            state.partitions[i]={words[p],words[p+1],bool(words[p+2]),std::bit_cast<std::int32_t>(words[p+3])};}
        if(words.size()==29){
            state.software_category=std::uint8_t(words[24]);
            state.software_control=std::bit_cast<std::int32_t>(words[25]);
            state.software_pending_size=std::bit_cast<std::int32_t>(words[26]);
            state.software_texture=words[27];context.software_cache_enabled=bool(words[28]);
        }
        const auto result=source_bitmap_resident_peek(state,real(words[0]),context);
        return {result.texture_token,state.cached_texture,std::bit_cast<std::uint32_t>(state.cached_until),
            std::bit_cast<std::uint32_t>(state.partitions[0].last_use),std::bit_cast<std::uint32_t>(state.partitions[1].last_use),
            std::bit_cast<std::uint32_t>(state.partitions[2].last_use),result.requires_request};
    }
    if(words.size()!=27)throw FormatError("bitmap request input length differs");
    SourceBitmapResidencyView view{real(words[0]),bool(words[1]),std::uint8_t(words[2])};
    std::array<SourceBitmapStageResidency,4> stages;
    for(unsigned i=0;i<4;++i){const auto p=3+6*i;
        stages[i]={words[p],words[p+1],std::uint8_t(words[p+2]),real(words[p+3]),real(words[p+4]),real(words[p+5])};}
    const auto requests=source_bitmap_residency_requests(stages,view);
    std::vector<std::uint32_t> out;
    for(const auto&s:stages)out.push_back(bits(s.minimum_request));
    out.push_back(unsigned(requests.size()));
    for(const auto&r:requests){out.push_back(r.stage);out.push_back(r.datum);
        out.push_back(std::uint16_t(r.image_index));out.push_back(bits(r.fractional_request));}
    return out;
}
}
int main(int argc,char**argv){try{
    const bool probe=argc==2&&std::string_view(argv[1])=="--probe";
    std::ifstream file;if(!probe){if(argc!=2)throw FormatError("bitmap residency fixture required");file.open(argv[1]);
        if(!file)throw FormatError("bitmap residency fixture missing");}
    auto&input=probe?std::cin:file;std::string line;unsigned cases=0,checked=0,requests=0;
    while(std::getline(input,line)){
        if(line.empty()||line[0]=='#')continue;
        const auto divider=line.find('|');std::istringstream before(line.substr(0,divider));before>>std::hex;
        std::vector<std::uint32_t> words,expected;std::uint32_t x;while(before>>x)words.push_back(x);
        if(!before.eof())throw FormatError("bitmap residency input malformed");
        const auto actual=evaluate(words);
        if(probe){for(auto word:actual)std::cout<<std::hex<<word<<' ';std::cout<<'\n';continue;}
        if(divider==std::string::npos)throw FormatError("bitmap residency fixture separator missing");
        std::istringstream after(line.substr(divider+1));after>>std::hex;while(after>>x)expected.push_back(x);
        if(!after.eof()||actual!=expected)throw FormatError("unchanged original bitmap request differs");
        ++cases;checked+=unsigned(actual.size());if(words.size()==27)requests+=actual[4];
    }
    if(!probe){
        const auto reject=[](auto&&call,const char*message){
            bool rejected=false;try{call();}catch(const FormatError&){rejected=true;}
            if(!rejected)throw FormatError(message);
        };
        std::array<SourceBitmapStageResidency,4> states;
        reject([&]{source_bitmap_residency_requests(states,{0,false,16});},"unproved texture stage was admitted");
        states[0].datum=1;states[0].frame=1;
        reject([&]{source_bitmap_residency_requests(states,{0,false,1});},"missing animated bitmap count was admitted");
        states[0].image_count=1;states[0].minimum_request=.5f;
        reject([&]{source_bitmap_residency_requests(states,{0,false,1});},"unproved stage minimum was admitted");
        //Inactive and NONE stages do not consume their stale metadata in1B0A0.
        if(!source_bitmap_residency_requests(states,{0,false,0}).empty())
            throw FormatError("disabled stage emitted a bitmap request");
        states[0].datum=0xffffffffu;
        if(!source_bitmap_residency_requests(states,{0,false,1}).empty())
            throw FormatError("NONE stage emitted a bitmap request");
        SourceBitmapResidentState cache;SourceBitmapResidentContext context;
        context.request_flags=4;
        reject([&]{source_bitmap_resident_peek(cache,0,context);},"unproved cache request flags were admitted");
        context.request_flags=3;context.software_images=true;cache.flags=2;
        reject([&]{source_bitmap_resident_peek(cache,0,context);},"unproved software-image producer was admitted");
        context.software_images=false;cache.cutoff=4;cache.partitions[0].handle=0;
        cache.partitions[0].ready=true;
        reject([&]{source_bitmap_resident_peek(cache,0,context);},"missing resident image token was admitted");
        context.software_images=true;context.software_cache_enabled=true;
        cache.software_category=1;cache.software_pending_size=64;
        reject([&]{source_bitmap_resident_peek(cache,0,context);},"unproved software header construction was admitted");
        cache.software_control=0x40001;
        reject([&]{source_bitmap_resident_peek(cache,0,context);},"missing software header resource was admitted");
        std::cout<<cases<<" source request cases, "<<requests<<" ordered143C0 calls, "<<checked<<" words exact\n";
    }
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
