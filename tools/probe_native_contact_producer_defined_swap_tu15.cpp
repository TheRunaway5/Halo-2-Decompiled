// Includes the isolated production implementation to exercise its actual
// inline selection-swap seam. These are DATA-domain/compatibility controls;
// independent whole304 original comparison remains a separate requirement.
#include "native.cpp"
#include "halo2/native_tu15_contact_candidate_private_data.hpp"
#include <iostream>
namespace {
using namespace halo2;
constexpr std::uint32_t P=0x74000000u;
void check(bool v,const char*message){if(!v)throw std::runtime_error(message);}
struct Outer final:SourcePhysicsContactMemory {
    mutable unsigned reads{};unsigned writes{};
    void read(std::uint32_t,std::span<std::byte>)const override{++reads;throw std::runtime_error("Unexpected outer read");}
    void write(std::uint32_t,std::span<const std::byte>)override{++writes;throw std::runtime_error("Unexpected outer write");}
};
const std::array ranges{NativeTu15ContactPrivateRange{P,4096}};
void word(SourcePhysicsContactMemory&m,std::uint32_t p,std::uint32_t value){std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(8*n));m.write(p,b);}
void numeric(SourcePhysicsContactMemory&m,std::uint32_t p,unsigned seed){
    word(m,p+0x20u,0x3f800000u+seed);word(m,p+0x24u,0x40000000u+seed);
    for(unsigned n=0;n<3;++n)word(m,p+0x70u+n*4u,0x1000u+seed*16u+n);
    const std::array<std::byte,2>flags{std::byte(seed&1u),std::byte(0)};m.write(p+0x7cu,flags);
}
void invoke(SourcePhysicsContactMemory&m,NativeTu15PhysicsFrameFp&fp,std::uint32_t a,std::uint32_t b,std::uint32_t temp){
    const SourcePhysicsContactRecordObserver observer;halo2::Environment environment(fp,0);halo2::Memory memory(m,observer);
    halo2::swap_selected(memory,{a,b},temp);
}
}
int main()try{
    unsigned controls{};
    for(unsigned mode=0;mode<4;++mode)for(bool self:{false,true}){
        Outer x,y;NativeTu15ContactCandidatePrivateData a(x,ranges,{}),b(y,ranges,{});
        std::array<std::byte,256>input;for(unsigned n=0;n<input.size();++n)input[n]=std::byte(n*31u+7u);
        a.write(P,input);b.write(P,input);for(unsigned k=0;k<2;++k){numeric(a,P+k*128u,k);numeric(b,P+k*128u,k);}
        NativeTu15PhysicsFrameFp old{std::uint16_t(0x27fu|(mode<<10u)),0x20u,0x1fa0u|(mode<<13u)},fresh=old;
        invoke(a,old,P,self?P:P+128u,0);invoke(b,fresh,P,self?P:P+128u,P+0x400u);
        std::array<std::byte,256>expected,actual;a.read(P,expected);b.read(P,actual);
        check(expected==actual&&old.control_word==fresh.control_word&&old.status==fresh.status&&old.mxcsr==fresh.mxcsr,"Known private swap/default or FP differs");
        check(!x.reads&&!x.writes&&!y.reads&&!y.writes,"Known swap touched outer storage");++controls;
    }
    {
        Outer x;NativeTu15ContactCandidatePrivateData a(x,ranges,{});numeric(a,P,0);numeric(a,P+128u,1);
        std::array<std::byte,16>known;known.fill(std::byte(0x6b));a.write(P+128u,known);
        NativeTu15PhysicsFrameFp fp{0x27f,0x20,0x1fa0};invoke(a,fp,P,P+128u,P+0x400u);
        check(a.data_is_defined(P,16)&&!a.data_is_defined(P+128u,16),"Private vector mask did not move with its field");
        std::array<std::byte,16>value;a.read(P,value);check(value==known,"Defined private vector snapshot differs");
        bool refused=false;try{a.read(P+128u,value);}catch(const std::runtime_error&){refused=true;}
        check(refused&&!x.reads&&!x.writes&&fp.status==0x20&&fp.mxcsr==0x1fa0,"Unknown private vector acquired a value/escaped or changed FP");++controls;
    }
    for(unsigned kind=0;kind<5;++kind){
        Outer x;NativeTu15ContactCandidatePrivateData a(x,ranges,{});numeric(a,P,0);numeric(a,P+128u,1);
        auto first=P,second=P+128u,temp=P+0x400u;
        if(kind==0){const std::array<NativeTu15ContactPrivateRange,1>one{{{P+128u+0x24u,4u}}};(void)one;
            // Revoke only this genuine numeric input. Plain copies must not
            // turn an unavailable floating-point operand into a value.
            NativeTu15ContactCandidatePrivateData nested(a,ranges,{});numeric(nested,P,0);
            word(nested,P+128u+0x20u,0x3f800000u);for(unsigned n=0;n<3;++n)word(nested,P+128u+0x70u+n*4u,n);
            NativeTu15PhysicsFrameFp fp{0x27f,0x20,0x1fa0};bool refused=false;
            try{invoke(nested,fp,first,second,temp);}catch(const std::runtime_error&e){refused=std::string(e.what()).find("unprovided private DATA")!=std::string::npos;}
            check(refused&&!x.reads&&!x.writes,"Undefined numeric swap input did not refuse");++controls;continue;
        }
        if(kind==1)temp=P;if(kind==2)temp=0xfffffff0u;if(kind==3)first=0x10000000u;if(kind==4)temp=P+1u;
        NativeTu15PhysicsFrameFp fp{0x27f,0x20,0x1fa0};bool refused=false;
        try{invoke(a,fp,first,second,temp);}catch(const std::runtime_error&e){refused=std::string(e.what()).find("disjoint owned-private")!=std::string::npos;}
        check(refused&&!x.reads&&!x.writes,"Private swap admission touched storage before refusal");++controls;
    }
    std::cout<<"{\"passed\":true,\"controls\":"<<controls<<"}\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
