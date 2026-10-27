#pragma once
// Shared harness for native TU1.5 sound kernel differential tests: a sparse
// page memory, randomized inputs with special floats, caller FP environments,
// and one comparison step that requires the native kernel and the lifted
// routine to agree on every owed output (or to fault at the same address).
#include "halo2/native_tu15_sound_manager.hpp"
#include <bit>
#include <cmath>
#include <cstdio>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <string>

namespace halo2::sound_test {
struct Memory final : SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::shared_ptr<std::array<std::byte,4096>>> pages;
    Memory clone() const {
        Memory copy;for(const auto &[n,p]:pages)copy.pages[n]=std::make_shared<std::array<std::byte,4096>>(*p);
        return copy;
    }
    std::byte *at(std::uint32_t p) const {
        const auto found=pages.find(p>>12);
        if(found==pages.end()){char t[40];std::snprintf(t,sizeof t,"unmapped %08x",p);throw std::out_of_range(t);}
        return found->second->data()+(p&4095u);
    }
    void read(std::uint32_t p,std::span<std::byte> out) const override {for(auto &b:out)b=*at(p++);}
    void write(std::uint32_t p,std::span<const std::byte> in) override {for(auto b:in)*at(p++)=b;}
    void map(std::uint32_t p,std::size_t bytes) {
        for(auto n=p>>12;n<=(p+bytes-1)>>12;++n)
            if(!pages.contains(n))pages[n]=std::make_shared<std::array<std::byte,4096>>();
    }
    void put32(std::uint32_t p,std::uint32_t v){map(p,4);for(unsigned i=0;i<4;++i)*at(p+i)=std::byte(v>>(i*8));}
    void put16(std::uint32_t p,std::uint16_t v){map(p,2);for(unsigned i=0;i<2;++i)*at(p+i)=std::byte(v>>(i*8));}
    void put8(std::uint32_t p,std::uint8_t v){map(p,1);*at(p)=std::byte(v);}
    void putf(std::uint32_t p,float v){put32(p,std::bit_cast<std::uint32_t>(v));}
    std::uint32_t get32(std::uint32_t p) const {
        std::uint32_t v=0;for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(*at(p+i))<<(i*8);return v;
    }
    std::uint16_t get16(std::uint32_t p) const {
        return std::uint16_t(std::to_integer<unsigned>(*at(p))|(std::to_integer<unsigned>(*at(p+1))<<8));
    }
};
struct Random {
    std::mt19937 rng;
    explicit Random(std::uint32_t seed=0x5eed1234u):rng(seed){}
    std::uint32_t word(){return rng();}
    std::uint32_t below(std::uint32_t n){return rng()%n;}
    // Finite values mostly; NaNs, infinities, denormals and signed zeros too.
    float scalar() {
        switch(below(12)) {
        case 0:return std::bit_cast<float>(0x7fc00000u|(word()&0x3fffffu));  // quiet NaN
        case 1:return std::bit_cast<float>(0x7f800001u|(word()&0x3fffffu));  // signalling NaN
        case 2:return below(2)?INFINITY:-INFINITY;
        case 3:return std::bit_cast<float>(word()&0x807fffffu);              // denormal
        case 4:return below(2)?0.f:-0.f;
        case 5:return std::bit_cast<float>(word());                          // any bits
        default:return std::ldexp(float(std::int32_t(word()))/2147483648.f,int(below(40))-20);
        }
    }
    float finite(float scale=1000.f){return (float(std::int32_t(word()))/2147483648.f)*scale;}
};

// Caller stack: [stack] holds the return address, arguments follow.
inline constexpr std::uint32_t stack=0x60080000u;
inline constexpr std::array<std::uint16_t,6> controls{0x27f,0x7f,0x37f,0x67f,0xa7f,0xe7f};
inline constexpr std::array<std::uint32_t,5> csrs{0x1f80u,0x9f80u,0x1fc0u,0xdfc0u,0x3f80u};

struct Case {
    Memory memory;SourcePhysicsContactRuntimeAbi abi{};SourcePhysicsFactoryFloatingPoint fp{};
};
// Random registers, a mapped stack with a return address, random x87 stack,
// status (TOP, sticky flags) and one of the caller FP environments.
inline Case base_case(Random &r) {
    Case c;
    c.memory.map(stack-0x2000u,0x3000u);c.memory.put32(stack,0x00abcdefu);
    auto &a=c.abi;
    for(auto &g:a.gp)g=r.word();
    a.gp[4]=stack;
    for(auto &x:a.xmm)for(auto &l:x)l=r.word();
    a.eflags=0x202u;
    a.x87_depth=r.below(5);
    for(unsigned i=0;i<a.x87_depth;++i)a.x87[i]=static_cast<long double>(r.scalar());
    a.x87_status=std::uint16_t((r.below(8)<<11)|(r.word()&0x473fu));
    c.fp={controls[r.below(controls.size())],csrs[r.below(csrs.size())]|(r.word()&0x3fu)};
    a.floating_point=c.fp;
    return c;
}

using Reference=std::function<void(SourcePhysicsContactMemory &,std::uint32_t,SourcePhysicsContactRuntimeAbi &,
    SourcePhysicsFactoryFloatingPoint,const SourcePhysicsContactRecordObserver &)>;
// One comparison: the oracle (native prediction vs lifted routine), then a
// committed native run against a committed lifted run over identical copies.
// Returns false and prints the difference on failure. faults counts states
// where both faulted at the same address.
inline bool agree(const Case &c,std::uint32_t entry,const Reference &reference,unsigned &faults,
    const char *label,unsigned n) {
    auto oracle=c;oracle.memory=c.memory.clone();
    try {
        const auto why=native_tu15_sound_manager_compare(oracle.memory,entry,oracle.abi,oracle.fp,{},
            [&](auto &m,auto &a,const auto &o){reference(m,entry,a,oracle.fp,o);});
        if(!why.empty()){std::cerr<<label<<" case "<<n<<": "<<why<<'\n';return false;}
    } catch(const std::exception &) {
        // A fault counts only when both sides fault with the same message.
        auto lifted=c,native=c;lifted.memory=c.memory.clone();native.memory=c.memory.clone();
        std::string lifted_fault="returned",native_fault="returned";
        try{reference(lifted.memory,entry,lifted.abi,lifted.fp,{});}catch(const std::exception &e){lifted_fault=e.what();}
        try{native_tu15_sound_manager_run(native.memory,entry,native.abi,native.fp);}
        catch(const std::exception &e){native_fault=e.what();}
        if(lifted_fault!=native_fault || lifted_fault=="returned") {
            std::cerr<<label<<" case "<<n<<": native "<<native_fault<<", lifted "<<lifted_fault<<'\n';return false;
        }
        ++faults;return true;
    }
    auto native=c,lifted=c;native.memory=c.memory.clone();lifted.memory=c.memory.clone();
    std::string native_fault,lifted_fault;
    try{native_tu15_sound_manager_run(native.memory,entry,native.abi,native.fp);}catch(const std::exception &e){native_fault=e.what();}
    try{reference(lifted.memory,entry,lifted.abi,lifted.fp,{});}catch(const std::exception &e){lifted_fault=e.what();}
    if(native_fault!=lifted_fault) {
        std::cerr<<label<<" case "<<n<<": committed fault "<<native_fault<<" / "<<lifted_fault<<'\n';return false;
    }
    // Everything below the caller's stack pointer after the return is dead.
    const auto dead_end=lifted.abi.gp[4];
    for(const auto &[page,bytes]:lifted.memory.pages) {
        const auto &other=*native.memory.pages.at(page);
        for(unsigned i=0;i<4096;++i) {
            const auto p=(page<<12)+i;
            if(p<dead_end && p>=stack-0x1000u)continue;
            if((*bytes)[i]!=other[i]) {
                char t[80];std::snprintf(t,sizeof t,"%s case %u: committed store differs at %08x",label,n,p);
                std::cerr<<t<<'\n';return false;
            }
        }
    }
    return true;
}
} // namespace halo2::sound_test
