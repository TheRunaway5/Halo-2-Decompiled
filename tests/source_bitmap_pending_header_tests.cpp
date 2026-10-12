#include "halo2/source_bitmap_pending_header.hpp"
#include <bit>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
std::int16_t signed_word(std::uint32_t value) {
    return std::bit_cast<std::int16_t>(std::uint16_t(value));
}
std::array<std::uint32_t,7> fields(const halo2::SourceBitmapPendingHeaderMetadata &metadata) {
    return {std::uint16_t(metadata.width),std::uint16_t(metadata.height),std::uint16_t(metadata.format),
        std::uint16_t(metadata.mipmap_count),std::uint8_t(metadata.depth),metadata.type,metadata.flags};
}
}
int main(int argc,char **argv) {try {
    const bool probe=argc==2 && std::string(argv[1])=="--probe";
    std::ifstream file;
    if(!probe) {
        if(argc!=2)throw std::runtime_error("pending header fixture required");
        file.open(argv[1]);if(!file)throw std::runtime_error("pending header fixture missing");
    }
    auto &input=probe?std::cin:file;
    std::string line;std::size_t cases=0,words=0;
    while(std::getline(input,line)) {
        if(line.empty() || line[0]=='#')continue;
        std::istringstream row(line);row>>std::hex;
        std::array<std::uint32_t,9> source{};
        for(auto &value:source)if(!(row>>value))throw std::runtime_error("pending header input truncated");
        for(std::size_t i=0;i<7;++i) {
            if(source[i]>(i==4?0xffu:0xffffu))throw std::runtime_error("pending header narrow input malformed");
        }
        const halo2::SourceBitmapPendingHeaderMetadata metadata{
            signed_word(source[0]),signed_word(source[1]),signed_word(source[2]),signed_word(source[3]),
            std::bit_cast<std::int8_t>(std::uint8_t(source[4])),std::uint16_t(source[5]),std::uint16_t(source[6])};
        const auto result=halo2::source_bitmap_pending_header(metadata,source[7],source[8]);
        const auto preserved=fields(metadata);
        for(std::size_t i=0;i<preserved.size();++i)if(preserved[i]!=source[i])
            throw std::runtime_error("pending header changed metadata input");
        if(probe) {
            for(std::size_t i=0;i<result.size();++i)
                std::cout<<(i?" ":"")<<std::hex<<std::uppercase<<std::setw(8)<<std::setfill('0')<<result[i];
            std::cout<<'\n';
        } else {
            std::string separator;if(!(row>>separator) || separator!="|")throw std::runtime_error("pending header separator missing");
            for(const auto value:result) {
                std::uint32_t expected;if(!(row>>expected) || value!=expected)
                    throw std::runtime_error("pending header result mismatch case "+std::to_string(cases));
            }
        }
        std::string extra;if(row>>extra)throw std::runtime_error("pending header trailing fixture data");
        ++cases;words+=result.size();
    }
    if(!input.eof() || (!probe && (cases!=19472 || words!=97360)))
        throw std::runtime_error("pending header frozen corpus incomplete");
    unsigned guards=0;
    for(const std::int16_t format:{std::int16_t(-32768),std::int16_t(-1),std::int16_t(24),std::int16_t(32767)}) {
        const halo2::SourceBitmapPendingHeaderMetadata metadata{1,1,format,0,1,0,0};
        try {halo2::source_bitmap_pending_header(metadata,0,0);throw std::runtime_error("pending header format admission failed");}
        catch(const std::invalid_argument &) {++guards;}
    }
    for(const std::uint32_t partition:{3u,0xffffffffu}) {
        const halo2::SourceBitmapPendingHeaderMetadata metadata{1,1,0,0,1,0,0};
        try {halo2::source_bitmap_pending_header(metadata,partition,0);throw std::runtime_error("pending header partition admission failed");}
        catch(const std::invalid_argument &) {++guards;}
    }
    if(!probe)std::cout<<cases<<" source pending header cases; "<<words<<" words exact; "<<guards<<" metadata admission controls\n";
    return 0;
}catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}}
