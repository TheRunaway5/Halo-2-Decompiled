#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace halo2 {
using namespace physics_world_runtime_detail;
#include "matrix_alignment_bodies.inc"
}
namespace {
using namespace halo2;
using J=nlohmann::json;
struct Bank final:SourcePhysicsContactMemory {
    std::uint32_t base{};std::vector<std::byte> data;
    void read(std::uint32_t address,std::span<std::byte> output)const override {
        if(address<base||address-base>data.size()||output.size()>data.size()-(address-base))
            throw std::runtime_error("matrix alignment read outside declared bank");
        std::memcpy(output.data(),data.data()+address-base,output.size());
    }
    void write(std::uint32_t address,std::span<const std::byte> input)override {
        if(address<base||address-base>data.size()||input.size()>data.size()-(address-base))
            throw std::runtime_error("matrix alignment write outside declared bank");
        std::memcpy(data.data()+address-base,input.data(),input.size());
    }
};
std::string hex(std::span<const std::byte> input) {
    constexpr char alphabet[]="0123456789abcdef";
    std::string result;result.reserve(input.size()*2);
    for(auto value:input){const auto n=std::to_integer<unsigned>(value);result+=alphabet[n>>4];result+=alphabet[n&15];}
    return result;
}
std::vector<std::byte> bytes(const std::string &text) {
    const auto digit=[](char c){return c<='9'?c-'0':c-'a'+10;};
    std::vector<std::byte> output;output.reserve(text.size()/2);
    for(unsigned i=0;i<text.size();i+=2)output.push_back(std::byte(digit(text[i])*16+digit(text[i+1])));
    return output;
}
}
int main(int argc,char **argv) {
    using namespace halo2::physics_world_runtime_detail;
    try {
        if(argc!=3)throw std::runtime_error("Supply matrix alignment inputs and output");
        std::ifstream input(argv[1]);const auto rows=J::parse(input);J results=J::array();
        for(const auto &row:rows) {
            Bank memory;memory.base=row.at("banks")[0].at("address");memory.data=bytes(row.at("banks")[0].at("before"));
            J stores=J::array();
            SourcePhysicsContactRecordObserver observe=[&](const auto &store) {
                stores.push_back({{"instruction",store.instruction},{"address",store.address},
                    {"before",hex(store.before)},{"written",hex(store.written)}});
            };
            std::uint32_t pc=0;unsigned callbacks=0;
            SourcePhysicsWorldRuntimeServices services;
            services.instruction=[&](std::uint32_t value){pc=value;++callbacks;};
            const std::array<std::uint32_t,3> sites{0x38846eu,0x21f590u,0x33b595u};
            services.instruction_cursor.position=&pc;services.instruction_cursor.executor=std::this_thread::get_id();
            services.instruction_cursor.callback_type=&services.instruction.target_type();
            services.instruction_cursor.literal_service_sites=true;services.instruction_cursor.observation_only_stores=true;
            services.instruction_cursor.service_sites=sites;
            WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
            SourcePhysicsContactRuntimeAbi initial{};initial.gp=row.at("gp");initial.eflags=row.at("eflags");
            initial.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};
            for(unsigned reg=0;reg<8;++reg) {
                const auto lanes=row.at("xmm")[reg].get<std::array<std::uint32_t,4>>();
                std::memcpy(initial.xmm[reg].data(),lanes.data(),16);
            }
            state.load(initial);bool completed=false;std::string error;
            try {
                if(row.at("entry")==0x2e44d0u)production::run_2e44d0(state);
                else production::run_2e46e0(state);
                completed=true;
            }catch(const std::exception &e){error=e.what();}
            const auto after=state.save();J xmm=J::array();
            for(const auto &raw:after.xmm) {
                std::array<std::uint32_t,4> lanes;std::memcpy(lanes.data(),raw.data(),16);xmm.push_back(lanes);
            }
            results.push_back({{"id",row.at("id")},{"completed",completed},{"error",error},
                {"pc",pc},{"gp",after.gp},{"eflags",after.eflags},{"xmm",xmm},
                {"fp",J::array({after.floating_point->x87_control_word,after.x87_status,after.floating_point->mxcsr})},
                {"writes",stores},{"after",hex(memory.data)},{"instruction_callbacks",callbacks}});
        }
        std::ofstream output(argv[2]);output<<results.dump()<<'\n';
        if(!output)throw std::runtime_error("Cannot write native matrix alignment result");
        return 0;
    }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
