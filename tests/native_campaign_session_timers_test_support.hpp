#pragma once
#include "halo2/native_campaign_session_timers_tu15.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <xmmintrin.h>

namespace {
using namespace halo2;using Json=nlohmann::json;
void check(bool ok,const std::string &why){if(!ok)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string &text){std::vector<std::byte> out;for(std::size_t n=0;n<text.size();n+=2)out.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));return out;}
std::string hex(std::span<const std::byte> values){constexpr char digits[]="0123456789abcdef";std::string out;for(auto b:values){const auto v=std::to_integer<unsigned>(b);out+=digits[v>>4u];out+=digits[v&15u];}return out;}
struct Prefix {};
struct Missing {std::uint32_t address;std::size_t width;};
struct Memory final:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    std::string phase;unsigned ordinal{};mutable unsigned attempted_reads{};unsigned attempted_writes{};
    mutable Json reads=Json::array();Json writes=Json::array();
    auto locate(std::uint32_t address,std::size_t size)const{
        auto found=banks.upper_bound(address);if(found==banks.begin())throw Missing{address,size};--found;
        if(std::uint64_t(address-found->first)+size>found->second.size())throw Missing{address,size};return found;
    }
    void read(std::uint32_t address,std::span<std::byte> out)const override{
        if(phase=="before_read"&&attempted_reads++==ordinal)throw Prefix{};
        const auto found=locate(address,out.size());std::copy_n(found->second.begin()+address-found->first,out.size(),out.begin());
        reads.push_back(Json{{"address",address},{"bytes",out.size()},{"before",hex(out)},{"stores_before",writes.size()}});
    }
    std::string external_clock_before(std::uint32_t address)const{
        const auto found=locate(address,4);return hex(std::span(found->second).subspan(address-found->first,4));
    }
    void external_clock_write(std::uint32_t address,std::uint32_t value){
        const auto found=locate(address,4);for(unsigned n=0;n<4;++n)banks.at(found->first)[address-found->first+n]=std::byte(value>>(8u*n));
    }
    void write(std::uint32_t address,std::span<const std::byte> data)override{
        if(phase=="before_write"&&attempted_writes==ordinal)throw Prefix{};
        const auto found=locate(address,data.size());const auto old=std::span(found->second).subspan(address-found->first,data.size());
        writes.push_back(Json{{"address",address},{"before",hex(old)},{"written",hex(data)}});
        std::copy(data.begin(),data.end(),banks.at(found->first).begin()+address-found->first);
        if(phase=="after_write"&&attempted_writes++==ordinal)throw Prefix{};
        if(phase!="after_write")++attempted_writes;
    }
};
struct HostEnvironment {
    alignas(16)std::array<std::byte,108> original{},before{},after{};std::uint32_t mxcsr;
    HostEnvironment():mxcsr(_mm_getcsr()){
        asm volatile("fnsave %0;fninit;fld1;fldpi;fldln2":"=m"(original)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        asm volatile("fnsave %0;frstor %0":"=m"(before)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
    }
    bool unchanged(){
        asm volatile("fnsave %0;frstor %0":"=m"(after)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        for(auto offset:{0u,4u,8u})if(!std::equal(before.begin()+offset,before.begin()+offset+2u,after.begin()+offset))return false;
        return std::equal(before.begin()+28u,before.end(),after.begin()+28u)&&mxcsr==_mm_getcsr();
    }
    ~HostEnvironment(){asm volatile("frstor %0"::"m"(original):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(mxcsr);}
};
}
