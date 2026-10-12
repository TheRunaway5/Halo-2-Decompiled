#include "halo2/source_bitmap_initialization.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

using namespace halo2;
namespace {
std::vector<std::uint32_t> evaluate(std::span<const std::uint32_t> in){
    if(in.size()!=7)throw std::invalid_argument("bitmap initialization fixture input differs");
    if(in[0]==3){
        const auto r=source_bitmap_reserve_stage(in[1],in[2]);
        return {r.upper_after,r.physical_base,r.protection_bytes,
            std::bit_cast<std::uint32_t>(r.page_count),r.calls_protection_wrapper};
    }
    const auto kind=SourceBitmapStorageKind(in[0]);
    std::vector<std::byte> storage(source_bitmap_storage_bytes(kind));
    for(std::size_t i=0;i<storage.size();++i)
        storage[i]=std::byte((in[4]+i*37u+(i>>3))&255u);
    source_bitmap_initialize_storage(kind,{in[1],in[2],in[3]},storage);
    std::vector<std::uint32_t> output;
    for(std::size_t i=0;i<storage.size();i+=4){
        std::uint32_t word=0;
        for(unsigned j=0;j<4;++j)word|=std::uint32_t(std::to_integer<unsigned char>(storage[i+j]))<<(j*8);
        output.push_back(word);
    }
    return output;
}
}
int main(int argc,char **argv){try{
    const bool probe=argc==2 && std::string_view(argv[1])=="--probe";
    std::ifstream file;if(!probe){
        if(argc!=2)throw std::invalid_argument("source bitmap initialization fixture required");
        file.open(argv[1]);if(!file)throw std::invalid_argument("source bitmap initialization fixture missing");
    }
    auto &input=probe?std::cin:file;std::string line;unsigned count=0,checked=0;
    while(std::getline(input,line)){
        if(line.empty() || line[0]=='#')continue;
        const auto split=line.find('|');std::istringstream before(line.substr(0,split));before>>std::hex;
        std::uint32_t word;std::vector<std::uint32_t> in,expected;
        while(before>>word)in.push_back(word);
        if(!before.eof())throw std::invalid_argument("source bitmap constructor input malformed");
        const auto actual=evaluate(in);
        if(probe){for(auto value:actual)std::cout<<std::hex<<value<<' ';std::cout<<'\n';continue;}
        if(split==std::string::npos)throw std::invalid_argument("source bitmap constructor separator missing");
        std::istringstream after(line.substr(split+1));after>>std::hex;
        while(after>>word)expected.push_back(word);
        if(!after.eof() || actual!=expected)
            throw std::runtime_error("source bitmap initializer differs at case "+std::to_string(count));
        ++count;checked+=unsigned(actual.size());
    }
    if(!probe){
        unsigned controls=0;const auto reject=[&](auto &&action){
            bool rejected=false;try{action();}catch(const std::invalid_argument&){rejected=true;}
            if(!rejected)throw std::runtime_error("missing allocation input admitted");
            ++controls;
        };
        std::vector<std::byte> bytes(0x508c,std::byte{0x5a});
        reject([&]{source_bitmap_storage_bytes(SourceBitmapStorageKind(4));});
        reject([&]{source_bitmap_initialize_storage(SourceBitmapStorageKind::Image,{0,1,0},bytes);});
        reject([&]{source_bitmap_initialize_storage(SourceBitmapStorageKind::Image,{1,0,0},bytes);});
        reject([&]{source_bitmap_initialize_storage(SourceBitmapStorageKind::Image,{0xfffffff0,1,0},bytes);});
        reject([&]{source_bitmap_initialize_storage(SourceBitmapStorageKind::PhysicalManager,{1,1,0},bytes);});
        reject([&]{source_bitmap_initialize_storage(SourceBitmapStorageKind::Image,{1,1,0},std::span(bytes).first(1));});
        std::cout<<count<<" source bitmap initialization cases; "<<checked<<" words exact; "
            <<controls<<" missing-allocation controls\n";
    }
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
