#include "halo2/cache.hpp"
#include "halo2/source_bitmap_cpu_selection.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
float real(std::uint32_t word){return std::bit_cast<float>(word);}
std::int32_t integer(std::uint32_t word){return std::bit_cast<std::int32_t>(word);}
std::vector<std::uint32_t> evaluate(std::span<const std::uint32_t> words){
    if(words.size()!=44)throw FormatError("source CPU bitmap fixture input length differs");
    SourceBitmapCpuState state;auto &resident=state.resident;
    SourceBitmapCpuContext context;
    context.cache.bias_enabled=bool(words[1]);context.cache.global_bias=real(words[2]);
    context.cache.clock=integer(words[3]);context.cache.use_clock=integer(words[4]);
    context.cache.software_images=bool(words[5]);context.cache.software_cache_enabled=bool(words[6]);
    context.force_zero=bool(words[7]);context.force_one=bool(words[8]);
    context.hardware_mode=integer(words[9]);context.hardware_category=integer(words[10]);
    context.fallback_marked=bool(words[11]);
    for(unsigned i=0;i<3;++i)context.default_textures[i]=words[12+i];
    resident.flags=std::uint8_t(words[15]);resident.bias=std::uint8_t(words[16]);
    resident.cutoff=real(words[17]);resident.cached_texture=words[18];resident.cached_until=integer(words[19]);
    if(words[20]!=0xffffffffu)resident.software_category=std::uint8_t(words[20]);
    resident.software_control=integer(words[21]);resident.software_pending_size=integer(words[22]);
    resident.software_texture=words[23];state.flags_low=std::uint8_t(words[24]);
    state.type=std::bit_cast<std::int16_t>(std::uint16_t(words[25]));
    for(unsigned i=0;i<3;++i){const auto at=26+i*6;
        state.resources[i]={words[at],integer(words[at+1])};
        resident.partitions[i]={words[at+2],words[at+3],bool(words[at+4]),integer(words[at+5])};
    }
    const auto selected=source_bitmap_cpu_select(state,real(words[0]),context);
    return {selected.texture_token,std::uint32_t(selected.boundary),selected.request_entered,
        selected.requested_partition,resident.cached_texture,std::bit_cast<std::uint32_t>(resident.cached_until),
        std::bit_cast<std::uint32_t>(resident.partitions[0].last_use),
        std::bit_cast<std::uint32_t>(resident.partitions[1].last_use),
        std::bit_cast<std::uint32_t>(resident.partitions[2].last_use),context.fallback_marked};
}
}
int main(int argc,char **argv){try{
    const bool probe=argc==2 && std::string_view(argv[1])=="--probe";
    std::ifstream file;
    if(!probe){if(argc!=2)throw FormatError("source CPU bitmap fixture required");file.open(argv[1]);
        if(!file)throw FormatError("source CPU bitmap fixture missing");}
    auto &input=probe?std::cin:file;std::string line;unsigned cases=0,checked=0;
    while(std::getline(input,line)){
        if(line.empty() || line[0]=='#')continue;
        const auto divider=line.find('|');
        std::istringstream before(line.substr(0,divider));before>>std::hex;
        std::vector<std::uint32_t> words,expected;std::uint32_t word;
        while(before>>word)words.push_back(word);
        if(!before.eof())throw FormatError("source CPU bitmap input malformed");
        const auto actual=evaluate(words);
        if(probe){for(const auto value:actual)std::cout<<std::hex<<value<<' ';std::cout<<'\n';continue;}
        if(divider==std::string::npos)throw FormatError("source CPU bitmap fixture separator missing");
        std::istringstream after(line.substr(divider+1));after>>std::hex;
        while(after>>word)expected.push_back(word);
        if(!after.eof() || actual!=expected)
            throw FormatError("complete original D15E0 bitmap selection differs at case"+std::to_string(cases));
        ++cases;checked+=unsigned(actual.size());
    }
    if(!probe){
        unsigned controls=0;
        const auto rejected=[&](auto &&action){
            bool failed=false;try{action();}catch(const FormatError&){failed=true;}
            if(!failed)throw FormatError("unproved source CPU bitmap input was admitted");
            ++controls;
        };
        SourceBitmapCpuState state;
        state.resident.flags=2;state.resident.cutoff=4;state.resident.software_category=1;
        SourceBitmapCpuContext context;
        context.cache.request_flags=3;
        rejected([&]{source_bitmap_cpu_select(state,0,context);});
        context.cache.request_flags=0;context.cache.software_images=true;
        state.resident.software_category.reset();
        rejected([&]{source_bitmap_cpu_select(state,0,context);});
        state.resident.software_category=1;context.cache.software_images=false;
        context.cache.software_cache_enabled=true;state.resident.software_control=0x40001;
        rejected([&]{source_bitmap_cpu_select(state,0,context);});
        state.resident.software_control=0;context.cache.software_cache_enabled=false;
        state.resident.partitions[0]={0x12340001,0,true,0};
        rejected([&]{source_bitmap_cpu_select(state,0,context);});
        state.resident.partitions[0]={0x12340001,0x1000,true,0};
        state.resident.partitions[1]={0x56780001,0x2000,true,0};
        rejected([&]{source_bitmap_cpu_select(state,0,context);});
        state.resident.partitions={};
        const auto nan=real(0x7fc00000);
        rejected([&]{source_bitmap_cpu_select(state,nan,context);});
        state.resident.cutoff=nan;
        rejected([&]{source_bitmap_cpu_select(state,0,context);});
        // Original D15E0's future cached image returns before stale request,
        // cutoff/category, aliased handles or image readiness are inspected.
        state.resident.cached_texture=0x3000;state.resident.cached_until=2;
        const auto fast=source_bitmap_cpu_select(state,nan,context);
        if(fast.texture_token!=0x3000 || fast.request_entered ||
           fast.boundary!=SourceBitmapCpuBoundary::Complete)
            throw FormatError("source CPU bitmap cached shortcut consumed inactive inputs");
        ++controls;
        std::cout<<cases<<" source CPU bitmap selection cases;"<<checked<<" words exact;"
            <<controls<<" missing-input/shortcut controls\n";
    }
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
