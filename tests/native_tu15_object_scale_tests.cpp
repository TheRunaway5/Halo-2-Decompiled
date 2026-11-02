#include "halo2/native_tu15_object_scale.hpp"
#include <algorithm>
#include <bit>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
namespace {
using namespace halo2;using J=nlohmann::json;
std::vector<std::byte> decode(const std::string& s) {
    std::vector<std::byte> out;
    for(std::size_t n=0;n<s.size();n+=2)out.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));
    return out;
}
std::string hex(std::span<const std::byte> b) {
    constexpr char digits[]="0123456789abcdef";std::string out;
    for(auto v:b){const auto n=std::to_integer<unsigned>(v);out+=digits[n>>4];out+=digits[n&15];}return out;
}
struct Memory:SourcePhysicsContactMemory {
    struct Bank {std::uint32_t address;std::vector<std::byte> bytes;};
    std::vector<Bank> banks;mutable J reads=J::array();J writes=J::array();
    std::span<std::byte> bytes(std::uint32_t p,std::size_t n) {
        for(auto& b:banks)if(p>=b.address&&std::uint64_t(p-b.address)+n<=b.bytes.size())return std::span(b.bytes).subspan(p-b.address,n);
        throw std::runtime_error("Unmapped scale operand");
    }
    void read(std::uint32_t p,std::span<std::byte> out)const override {
        const auto b=const_cast<Memory*>(this)->bytes(p,out.size());std::ranges::copy(b,out.begin());reads.push_back(J::array({p,out.size()}));
    }
    void write(std::uint32_t p,std::span<const std::byte> in)override {
        std::ranges::copy(in,bytes(p,in.size()).begin());writes.push_back(J::array({p,hex(in)}));
    }
    void put(std::uint32_t at,std::uint32_t v,unsigned size=4) {
        auto out=bytes(at,size);for(unsigned n=0;n<size;++n)out[n]=std::byte(v>>(8*n));
    }
};
struct RefreshBoundary{};
void check(bool v,const char* message){if(!v)throw std::runtime_error(message);}
Memory hierarchy() {
    Memory m;m.banks={{0x46a000,std::vector<std::byte>(4096)},{0x4ec000,std::vector<std::byte>(4096)},
        {0x1000000,std::vector<std::byte>(0x4000)}};
    m.put(0x4ecbd0,0x1000000);m.put(0x1000044,0x1000100);m.put(0x46a17c,0x3d088889);
    for(unsigned n=0;n<4;++n){const auto body=0x1001000+0x400*n;
        m.put(0x1000108+12*n,body);m.put(body+0xa0,0x3f800000);m.put(body+0x112,0xffff,2);
        m.put(body+0x10,0xffffffff);m.put(body+0x0c,0xffffffff);}
    m.put(0x1001010,0xbeef0001);return m;
}
void controls() {
    NativeTu15PhysicsFrameFp fp;
    {auto m=hierarchy();std::vector<unsigned> order;NativeTu15ObjectScaleServices services;
        services.refresh=[&](auto object,auto&){order.push_back(object&65535u);
            if((object&65535u)==0)m.put(0x1001010,2); // parent's child was already pinned
            if((object&65535u)==1){m.put(0x1000114,0x1001c00);m.put(0x100140c,2);} // row relocates; old body's sibling wins
        };
        native_tu15_object_scale_hierarchy(m,0,0x3f000000,0xffff,fp,services);
        check(order==std::vector<unsigned>{0,1,2},"Scale hierarchy lost pinned child/body or post-recursion sibling");}
    {auto m=hierarchy();native_tu15_object_scale_hierarchy(m,0,0xbf800000,30,fp,{});
        check(m.writes.empty(),"Negative scale published a word");
        unsigned durations{};for(const auto& r:m.reads)if(r[0]==0x46a17c)++durations;
        check(durations==2,"Rejected parent scale skipped child traversal");}
    {auto m=hierarchy();native_tu15_object_scale_hierarchy(m,0xffffffff,0x7f812345,30,fp,{});
        check(m.reads.empty()&&m.writes.empty(),"NONE scale read memory or performed arithmetic");}
    for(bool interpolation:{false,true}) {
        auto m=hierarchy();if(interpolation)m.put(0x1001112,0x200,2);bool refused=false;
        try{native_tu15_object_scale(m,0,0x3f000000,0,fp,{});}catch(const std::runtime_error&){refused=true;}
        check(refused&&m.writes.size()==1&&m.writes[0][0]==0x10010a0,"Missing scale dependency lost preceding store or silently succeeded");
    }
}
J run(const J& row) {
    Memory m;for(const auto& b:row.at("banks"))m.banks.push_back({b.at("address"),decode(b.at("before"))});
    const auto& before=row.at("before_fp");NativeTu15PhysicsFrameFp fp{before[0],before[1],before[2]};
    const NativeTu15ObjectScalePublisher writer=row.value("publisher",false)?
        NativeTu15ObjectScalePublisher([&](auto at,auto data){m.write(at,data);}):NativeTu15ObjectScalePublisher{};
    std::uint32_t seconds{};NativeTu15ObjectScaleServices services;
    services.refresh=[](auto,auto&){throw RefreshBoundary{};};
    try {
        if(row.at("kind")=="pose")native_tu15_object_scale_pose(m,row.at("body"),row.at("old_scale"),row.at("scale"),fp,writer);
        else if(row.at("kind")=="seconds")seconds=native_tu15_object_scale_seconds(m,row.at("ticks"),fp);
        else native_tu15_object_scale(m,row.at("object"),row.at("scale"),0,fp,services,writer);
    }catch(const RefreshBoundary&){}
    J banks=J::array();for(const auto& b:m.banks)if(b.address!=0x62000000)banks.push_back({{"address",b.address},{"after",hex(b.bytes)}});
    return {{"reads",m.reads},{"writes",m.writes},{"banks",banks},{"fp",{fp.control_word,fp.status,fp.mxcsr}},{"seconds",seconds}};
}
}
int main()try {
    controls();
    for(std::string line;std::getline(std::cin,line);)if(!line.empty())std::cout<<run(J::parse(line)).dump()<<'\n'<<std::flush;
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
