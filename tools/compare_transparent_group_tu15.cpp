// Streaming offline comparison peer. External callees are declared fixtures,
// never production replacements for scene copy, collection or layer drawing.
#include "halo2/source_transparent_group_tu15.hpp"
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string_view>
using namespace halo2;
namespace {
constexpr std::array<std::uint32_t,6> bases{0x473000,0x492000,0x4c6000,0x4ce000,0x2000000,0x2100000};
constexpr std::uint32_t stack=0x2000800,payload=0x2100100;
struct Bank : SourcePhysicsContactMemory {
    std::array<std::array<std::byte,4096>,6> bytes;
    std::pair<unsigned,unsigned> locate(std::uint32_t p,std::size_t n)const {
        for(unsigned i=0;i<bases.size();++i)if(p>=bases[i]&&p-bases[i]+std::uint64_t(n)<=4096)return {i,p-bases[i]};
        throw std::runtime_error("Undeclared group comparison memory");
    }
    void read(std::uint32_t p,std::span<std::byte> out)const override {auto [i,o]=locate(p,out.size());std::memcpy(out.data(),bytes[i].data()+o,out.size());}
    void write(std::uint32_t p,std::span<const std::byte> in)override {auto [i,o]=locate(p,in.size());std::memcpy(bytes[i].data()+o,in.data(),in.size());}
    void put(std::uint32_t p,std::uint32_t x,unsigned n=4){write(p,std::as_bytes(std::span{&x,1}).first(n));}
    std::uint32_t word(std::uint32_t p)const {std::uint32_t x;read(p,std::as_writable_bytes(std::span{&x,1}));return x;}
};
template<class T>void append(std::vector<std::byte>&out,const T&v){auto b=std::as_bytes(std::span{&v,1});out.insert(out.end(),b.begin(),b.end());}
unsigned arguments(std::uint32_t target){switch(target){case 0x352d0:return 2;case 0x44c80:return 8;case 0x14ba0:return 2;case 0x14b40:return 3;case 0x15660:case 0x15350:case 0x44540:case 0x445c0:case 0x44360:return 0;default:throw std::runtime_error("Undeclared group fixture callee");}}
constexpr std::array<std::uint32_t,9> cells{0x4ce268,0x4ce26c,0x4ce270,0x4ce274,0x4ce278,0x473690,0x473694,0x473698,0x4920d4};
}
int main(int argc,char **argv)try {
    const bool absent_owner=argc==2&&std::string_view(argv[1])=="--reject-missing-owner";
    if(argc!=1&&!absent_owner)throw std::runtime_error("Unknown group comparison option");
    std::array<std::uint32_t,10> input;
    while(std::cin.read(reinterpret_cast<char*>(input.data()),sizeof(input))) {
        Bank m;const auto seed=input[9];
        for(unsigned r=0;r<bases.size();++r)for(unsigned i=0;i<4096;++i)m.bytes[r][i]=std::byte(i*13u+r*17u+seed);
        m.put(stack,0x2200000);m.put(stack+4,payload);m.put(payload,input[4]);m.put(payload+4,input[5]);
        m.put(0x4ce270,input[6]);m.put(0x4ce26c,input[7]);m.put(0x4920d8,input[8]);m.put(0x4920b8,input[2]);
        m.put(0x4c6842,(input[1]&1)?0x81:0,1);m.put(0x4c6843,(input[1]&2)?0x42:0,1);
        m.put(0x492295,(input[1]&4)?0xff:0,1);m.put(0x492296,(input[1]&8)?0x80:0,1);
        SourcePhysicsWorldRuntimeAbi a{};
        for(unsigned i=0;i<8;++i){a.gp[i]=0xa5000000u+i*0x1020304u+seed;for(unsigned j=0;j<4;++j)a.xmm[i][j]=0x87654321u+i*101u+j*0x10203u+seed;}
        a.gp[4]=stack;a.eflags=0x202|(seed&0x8d5);a.fs_base=0x2300000;a.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
        std::vector<std::uint32_t> trace;std::vector<std::byte> stores,calls;
        SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](auto pc){trace.push_back(pc);};
        services.required_owner=[&](auto &,const auto &c,auto &abi,const auto &) {
            const auto count=arguments(c.target);append(calls,c.instruction);append(calls,c.target);append(calls,std::uint32_t(c.tail));
            append(calls,abi.gp);append(calls,abi.eflags&~0x10u);
            for(unsigned i=0;i<=count;++i)append(calls,m.word(abi.gp[4]+i*4));
            for(auto p:cells)append(calls,m.word(p));
            if(c.target==0x44c80)m.put(0x4ce270,input[0]);
            if(c.target==0x44360) {
                const auto layer=abi.gp[7];if(layer!=15&&layer!=21)throw std::runtime_error("Unknown group selector layer");
                m.put(0x473690,layer);m.put(0x473694,0);m.put(0x473698,0x4ce268);
                abi.gp[0]=(abi.gp[0]&0xffffff00u)|((input[3]>>(layer==15?0:1))&1u);
            }
            abi.gp[4]+=4u+count*4u;
        };
        if(absent_owner)services.required_owner={};
        bool rejected=false;
        try {source_run_transparent_group_tu15(m,0x423b0,a,{0x27f,0x1f80},services,[&](const auto &s){
            append(stores,s.instruction);append(stores,s.address);append(stores,std::uint32_t(s.written.size()));
            stores.insert(stores.end(),s.before.begin(),s.before.end());stores.insert(stores.end(),s.written.begin(),s.written.end());
        });}catch(const std::runtime_error &e){
            if(!absent_owner)throw;
            rejected=std::string_view(e.what())=="Required original contact runtime callee unavailable";
        }
        if(absent_owner) {
            if(!rejected||trace.back()!=0x423e1||!calls.empty()||a.gp[4]!=stack-24||m.word(a.gp[4])!=0x423e6||
               m.word(0x4ce270)!=0||m.word(0x4ce278)!=input[6]||m.word(0x4ce274)!=input[7])
                throw std::runtime_error("Missing graphics owner did not stop at the original live boundary");
            std::cout<<"PASS missing scene-copy owner rejects at423E1 with retained caller and preceding stores\n";
            continue;
        }
        std::vector<std::byte> out;append(out,std::uint32_t(trace.size()));for(auto p:trace)append(out,p);
        append(out,std::uint32_t(stores.size()));out.insert(out.end(),stores.begin(),stores.end());
        append(out,std::uint32_t(calls.size()));out.insert(out.end(),calls.begin(),calls.end());
        append(out,a.gp);append(out,a.eflags&~0x10u);append(out,a.fs_base);append(out,a.xmm);
        append(out,std::uint32_t(a.x87_depth));append(out,std::uint32_t(a.x87_status));
        append(out,std::uint32_t(a.floating_point->x87_control_word));append(out,a.floating_point->mxcsr);
        for(const auto &region:m.bytes)append(out,region);
        const auto size=std::uint32_t(out.size());std::cout.write(reinterpret_cast<const char*>(&size),4);
        std::cout.write(reinterpret_cast<const char*>(out.data()),out.size());std::cout.flush();
        if(!std::cout)throw std::runtime_error("Cannot write group comparison result");
    }
    if(!std::cin.eof())throw std::runtime_error("Truncated group comparison input");
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
