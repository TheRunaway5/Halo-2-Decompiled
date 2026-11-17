#pragma once
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <map>
#include <nlohmann/json.hpp>

namespace halo2::mopp_ray_frame_test {
using namespace physics_world_runtime_detail;
using J=nlohmann::json;
struct Boundary{};
inline void check(bool value,const std::string&message){if(!value)throw std::runtime_error(message);}
inline std::vector<std::byte>bytes(const std::string&text){
    std::vector<std::byte>out;for(std::size_t n=0;n<text.size();n+=2)out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));return out;
}
inline std::string hex(std::span<const std::byte>data){
    constexpr char digits[]="0123456789abcdef";std::string out;
    for(auto byte:data){const auto n=std::to_integer<unsigned>(byte);out+=digits[n>>4];out+=digits[n&15];}return out;
}
inline void poison(){
    asm volatile("fninit;fld1;fldz":::"memory","st","st(1)");
    const std::uint16_t cw=0x0740;asm volatile("fldcw %0"::"m"(cw));_mm_setcsr(0x8065u);
}
inline J packet(const WorldState&s,std::uint32_t pc){
    const auto abi=s.save();J live=J::array();
    for(unsigned n=0;n<abi.x87_depth;++n)live.push_back(hex(std::span(reinterpret_cast<const std::byte*>(&abi.x87[n]),10)));
    return J{{"entry",pc},{"gp",abi.gp},{"eflags",abi.eflags},{"xmm",abi.xmm},
             {"fp",J::array({abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr})},{"live_x87",live}};
}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>>banks;
    std::uint32_t pc{};mutable J operations=J::array();J writes=J::array();
    int read_fault{-1},write_fault{-1};mutable unsigned read_count{};unsigned write_count{};
    bool observer_preimages{},hostile{};
    auto at(std::uint32_t address,std::size_t size)const{
        auto i=banks.upper_bound(address);check(i!=banks.begin(),"Missing ray-frame bank");--i;
        check(std::uint64_t(address-i->first)+size<=i->second.size(),"Ray-frame bank extent");return i;
    }
    void read(std::uint32_t address,std::span<std::byte>out)const override{
        if(int(read_count++)==read_fault){if(hostile)poison();throw Boundary{};}
        operations.push_back(J{{"kind","read"},{"instruction",pc},{"address",address},{"bytes",out.size()}});
        auto i=at(address,out.size());std::copy_n(i->second.begin()+address-i->first,out.size(),out.begin());if(hostile)poison();
    }
    void read_vector(std::uint32_t address,std::span<std::byte,16>out)const override{read(address,out);}
    void write(std::uint32_t address,std::span<const std::byte>value)override{
        if(int(write_count++)==write_fault){if(hostile)poison();throw Boundary{};}
        auto i=at(address,value.size());auto&data=banks.at(i->first);
        writes.push_back(J{{"instruction",pc},{"address",address},{"before",hex(std::span(data).subspan(address-i->first,value.size()))},{"written",hex(value)}});
        operations.push_back(J{{"kind","write"},{"instruction",pc},{"address",address},{"bytes",value.size()}});
        std::copy(value.begin(),value.end(),data.begin()+address-i->first);if(hostile)poison();
    }
    J architectural_reads()const{
        J out=J::array();
        for(std::size_t n=0;n<operations.size();++n){auto event=operations[n];if(event.at("kind")!="read")continue;
            if(observer_preimages&&n+1<operations.size()){
                const auto&next=operations[n+1];if(next.at("kind")=="write"&&event.at("instruction")==next.at("instruction")&&
                    event.at("address")==next.at("address")&&event.at("bytes")==next.at("bytes"))continue;
            }
            event.erase("kind");out.push_back(event);
        }return out;
    }
};
} // namespace halo2::mopp_ray_frame_test
