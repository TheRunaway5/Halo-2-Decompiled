#include "halo2/native_campaign_bsp_player_reset_tu15.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>

using namespace halo2;
using Json=nlohmann::json;
namespace {
constexpr std::uint32_t base=0x1000000u;
std::vector<std::byte> unhex(const std::string &hex) {
    if(hex.size()%2)throw std::runtime_error("Odd fixture hex length");
    std::vector<std::byte> out;
    out.reserve(hex.size()/2);
    for(std::size_t i=0;i<hex.size();i+=2)
        out.push_back(std::byte(std::stoul(hex.substr(i,2),nullptr,16)));
    return out;
}
std::string hex(std::span<const std::byte> bytes) {
    std::string out;
    constexpr char digits[]="0123456789abcdef";
    for(auto byte:bytes) {
        const auto n=std::to_integer<unsigned>(byte);
        out+=digits[n>>4];out+=digits[n&15];
    }
    return out;
}
struct Memory final:SourcePhysicsContactMemory {
    std::vector<std::byte> heap;
    std::vector<std::pair<std::uint32_t,std::vector<std::byte>>> cells;
    void read(std::uint32_t p,std::span<std::byte> out) const override {
        if(p>=base && std::uint64_t(p-base)+out.size()<=heap.size()) {
            std::ranges::copy(std::span(heap).subspan(p-base,out.size()),out.begin());return;
        }
        for(const auto &[address,bytes]:cells)
            if(p>=address && std::uint64_t(p-address)+out.size()<=bytes.size()) {
                std::ranges::copy(std::span(bytes).subspan(p-address,out.size()),out.begin());return;
            }
        throw std::runtime_error("Read escaped declared BSP reset banks");
    }
    void write(std::uint32_t p,std::span<const std::byte> in) override {
        if(p<base || std::uint64_t(p-base)+in.size()>heap.size())
            throw std::runtime_error("Write escaped declared BSP reset bank");
        std::ranges::copy(in,heap.begin()+(p-base));
    }
};
}

int main(int argc,char **argv) try {
    if(argc!=2)throw std::runtime_error("Expected original BSP reset fixture");
    std::ifstream file(argv[1]);
    const auto fixture=Json::parse(file);
    if(fixture.at("image_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd")
        throw std::runtime_error("Unexpected original image");
    unsigned count{};
    for(const auto &row:fixture.at("cases")) {
        Memory memory;
        memory.heap=unhex(row.at("heap").get<std::string>());
        for(const auto &cell:row.at("cells"))
            memory.cells.emplace_back(cell.at(0).get<std::uint32_t>(),unhex(cell.at(1).get<std::string>()));
        Json events=Json::array();
        const SourcePhysicsContactRecordObserver observe=[&](const auto &store) {
            events.push_back(Json::array({store.instruction,store.address,hex(store.before),hex(store.written)}));
        };
        bool refused=false;
        try {
            if(row.at("visibility")==true)native_campaign_bsp_reset_visibility_tu15(memory,observe);
            else native_campaign_bsp_reset_players_tu15(memory,observe);
        } catch(const std::runtime_error &e) {
            if(std::string_view(e.what())!="TU1.5 BSP player transition requires native 14E1F4 relocation")throw;
            refused=true;
        }
        if(refused!=row.at("refused").get<bool>() || events!=row.at("stores") ||
            hex(memory.heap)!=row.at("after").get<std::string>())
            throw std::runtime_error("Original BSP reset mismatch: "+row.at("name").get<std::string>());
        ++count;
    }
    if(count<20)throw std::runtime_error("Missing original BSP reset coverage");
    std::cout<<count<<" original BSP reset profiles passed\n";
    return 0;
} catch(const std::exception &error) {
    std::cerr<<error.what()<<'\n';return 1;
}
