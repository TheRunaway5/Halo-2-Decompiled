#include "halo2/cache.hpp"
#include "halo2/source_vertex_extern.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
void check(bool ok,const char *message){if(!ok)throw FormatError(message);}
nlohmann::json read(const char *path){std::ifstream file(path);check(bool(file),"Missing original vertex extern corpus");return nlohmann::json::parse(file);}
std::vector<std::uint8_t> bytes(const std::string &hex) {
    check(hex.size()%2==0,"Odd original byte sequence");
    std::vector<std::uint8_t> out;
    for(std::size_t n=0;n<hex.size();n+=2)out.push_back(std::uint8_t(std::stoul(hex.substr(n,2),nullptr,16)));
    return out;
}
std::uint32_t word(std::span<const std::uint8_t> data,std::size_t at) {
    check(at<=data.size() && data.size()-at>=4,"Original vertex word escapes corpus");
    return std::uint32_t(data[at])|std::uint32_t(data[at+1])<<8|
        std::uint32_t(data[at+2])<<16|std::uint32_t(data[at+3])<<24;
}
void compare(const nlohmann::json &row) {
    const auto initial=bytes(row.at("initial_vertex_hex"));
    const auto expected=bytes(row.at("vertex_hex"));
    check(initial.size()==512 && expected.size()==512,"Original vertex bank extent changed");
    ShaderConstants constants{};
    for(unsigned bank=0;bank<2;++bank)for(unsigned reg=0;reg<16;++reg)for(unsigned axis=0;axis<4;++axis)
        constants[bank][reg][axis]=std::bit_cast<float>(word(initial,bank*256+reg*16+axis*4));
    std::array<float,256> scalars{};std::array<ShaderVector,256> colors{};
    const auto &source=row.contains("scalars_bits")?row.at("scalars_bits"):row.at("scalar_bits");
    for(const auto &[index,value]:source.items())scalars.at(std::stoul(index))=std::bit_cast<float>(value.get<std::uint32_t>());
    if(row.contains("colors_bits")) {
        for(const auto &[index,value]:row.at("colors_bits").items())for(unsigned axis=0;axis<3;++axis)
            colors.at(std::stoul(index))[axis]=std::bit_cast<float>(value.at(axis).get<std::uint32_t>());
    } else {
        for(unsigned index:{40u,41u})for(unsigned axis=0;axis<3;++axis)
            colors[index][axis]=std::bit_cast<float>(row.at("color"+std::to_string(index)+"_bits").at(axis).get<std::uint32_t>());
    }
    std::vector<ShaderRemap> remaps;
    const auto &mappings=row.at("mappings").is_object()?row.at("mappings").at("vertex"):row.at("mappings");
    for(const auto &raw:mappings) {
        const auto data=bytes(raw);check(data.size()==4,"Original vertex extern record extent changed");
        remaps.push_back({std::uint16_t(data[0]|unsigned(data[1])<<8),data[2],data[3]});
    }
    const auto dirty=remap_source_vertex_externs(constants,scalars,colors,remaps);
    for(unsigned bank=0;bank<2;++bank) {
        const auto before=row.contains("initial_vertex_dirty_masks")?row.at("initial_vertex_dirty_masks").at(bank).get<std::uint32_t>():0u;
        check((dirty[bank]|before)==row.at("vertex_dirty_masks").at(bank),"Original vertex dirty mask differs");
        for(unsigned reg=0;reg<16;++reg)for(unsigned axis=0;axis<4;++axis)
            if(std::bit_cast<std::uint32_t>(constants[bank][reg][axis])!=word(expected,bank*256+reg*16+axis*4))
                throw FormatError("Original complete vertex bank differs at bank "+std::to_string(bank)+
                    ", register "+std::to_string(reg)+", axis "+std::to_string(axis)+", mappings "+mappings.dump());
    }
}
}
int main(int argc,char **argv)try {
    check(argc==3,"Supply generic whole19BE0 fixture and original camouflage pass report");
    const auto generic_report=read(argv[1]),authored_report=read(argv[2]);
    unsigned generic=0,authored=0;
    for(const auto &row:generic_report.at("cases")){compare(row);++generic;}
    for(const auto &row:authored_report.at("cases")){compare(row);++authored;}
    check(generic>0 && authored==420,"Original vertex extern corpus coverage changed");
    ShaderConstants bank{};const std::array<float,1> scalars{.25f};
    for(const auto destination:{std::uint16_t(5u<<5),std::uint16_t(21u<<5)}) {
        const ShaderRemap mapping{destination,0,0};bool rejected=false;
        try{(void)remap_source_vertex_externs(bank,scalars,{},std::span(&mapping,1));}catch(const FormatError&){rejected=true;}
        check(rejected,"Unadmitted vertex value mode silently used a parameter remap");
    }
    std::cout<<generic<<" unchanged whole19BE0 cases and "<<authored<<" actual camouflage implementation banks passed\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
