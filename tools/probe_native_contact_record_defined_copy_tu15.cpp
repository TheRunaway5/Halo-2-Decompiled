#include "halo2/native_tu15_contact_candidate_private_data.hpp"
#include "halo2/native_tu15_contact_buffers.hpp"
#include <iostream>
namespace {
using namespace halo2;
constexpr std::uint32_t PRIVATE=0x74000000u,PUBLIC=0x10000000u;
void check(bool v,const char*why){if(!v)throw std::runtime_error(why);}
struct Outer final:SourcePhysicsContactMemory {
    std::array<std::byte,4096>bytes;unsigned reads{},writes{};
    Outer(){bytes.fill(std::byte(0xa5));}
    void read(std::uint32_t p,std::span<std::byte>b)const override{
        check(p>=PUBLIC&&std::uint64_t(p-PUBLIC)+b.size()<=bytes.size(),"Unbacked retained read");
        ++const_cast<Outer*>(this)->reads;std::copy_n(bytes.begin()+p-PUBLIC,b.size(),b.begin());
    }
    void write(std::uint32_t p,std::span<const std::byte>b)override{
        check(p>=PUBLIC&&std::uint64_t(p-PUBLIC)+b.size()<=bytes.size(),"Unbacked retained write");
        ++writes;std::copy(b.begin(),b.end(),bytes.begin()+p-PUBLIC);
    }
};
const std::array owned{NativeTu15ContactPrivateRange{PRIVATE,4096}};
const std::array retained{NativeTu15ContactPrivateRange{PUBLIC,4096}};
void word(SourcePhysicsContactMemory&m,std::uint32_t p,std::uint32_t n){std::array<std::byte,4>b{};for(unsigned k=0;k<4;++k)b[k]=std::byte(n>>(8*k));m.write(p,b);}
std::uint32_t word(SourcePhysicsContactMemory&m,std::uint32_t p){std::array<std::byte,4>b{};m.read(p,b);std::uint32_t n{};for(unsigned k=0;k<4;++k)n|=std::to_integer<unsigned>(b[k])<<(8*k);return n;}
struct Stop{};
}
int main()try{
    unsigned controls{};
    {
        Outer outer;NativeTu15ContactCandidatePrivateData memory(outer,owned,retained);
        const std::array<std::byte,1>zero{std::byte(0)};memory.write(PRIVATE+0x7cu,zero);memory.write(PRIVATE+0x7du,zero);
        unsigned events{};NativeTu15ContactRecordMemory records(memory,[&](const auto&){++events;});
        native_tu15_copy_contact_record(records,{PRIVATE+0x200u},{PRIVATE});
        check(!memory.data_is_defined(PRIVATE+0x200u,16)&&memory.data_is_defined(PRIVATE+0x27cu,2),"Private record definedness differs");
        bool refused=false;try{std::array<std::byte,1>b;memory.read(PRIVATE+0x200u,b);}catch(const std::runtime_error&){refused=true;}
        check(refused&&events==0&&outer.reads==0&&outer.writes==0,"Undefined private record acquired a value or escaped");++controls;
        refused=false;try{native_tu15_copy_contact_record(records,{PUBLIC},{PRIVATE});}catch(const std::runtime_error&){refused=true;}
        check(refused&&outer.writes==0,"Undefined record escaped before first definition");++controls;
    }
    {
        Outer outer;NativeTu15ContactCandidatePrivateData memory(outer,owned,retained);
        std::array<std::byte,128>source;for(unsigned n=0;n<source.size();++n)source[n]=std::byte(n+1u);memory.write(PRIVATE,source);
        std::vector<SourcePhysicsContactRecordStore>events;NativeTu15ContactRecordMemory records(memory,[&](const auto&e){events.push_back(e);});
        native_tu15_copy_contact_record(records,{PUBLIC},{PRIVATE});
        check(events.size()==13&&events.front().instruction==0x303ba9u&&events.back().instruction==0x303bfbu,"Defined record SourcePC events differ");
        for(unsigned n=0;n<128;++n){const bool gap=(n>=0x28u&&n<0x30u)||n>=0x7eu;check(outer.bytes[n]==(gap?std::byte(0xa5):source[n]),"Defined record fields/gaps differ");}
        ++controls;
        outer.bytes.fill(std::byte(0xa5));unsigned seen{};NativeTu15ContactRecordMemory interrupted(memory,[&](const auto&){++seen;throw Stop{};});bool stopped=false;
        try{native_tu15_copy_contact_record(interrupted,{PUBLIC},{PRIVATE});}catch(const Stop&){stopped=true;}
        check(stopped&&seen==1&&std::equal(source.begin(),source.begin()+16,outer.bytes.begin())&&outer.bytes[16]==std::byte(0xa5),"Defined-copy poststore observer commit differs");++controls;
    }
    {
        Outer outer;NativeTu15ContactCandidatePrivateData memory(outer,owned,retained);
        constexpr auto header=PRIVATE+0x600u,constructor=PRIVATE+0x400u,destination=PRIVATE+0x200u;
        word(memory,header,destination);word(memory,header+4u,0);word(memory,header+8u,0x80000004u);
        NativeTu15PhysicsFrameFp fp{0x27f,0x20,0x1fa0};unsigned events{};
        const auto result=native_tu15_append_contact_record(memory,{header},{constructor},fp,{},[&](const auto&){++events;});
        check(result.identity==destination&&word(memory,header+4u)==1&&memory.data_is_defined(destination+0x7cu,2)&&!memory.data_is_defined(destination,16),"Native303EE0 private constructor/count differs");
        check(events==0&&outer.reads==0&&outer.writes==0&&fp.control_word==0x27f&&fp.status==0x20&&fp.mxcsr==0x1fa0,"Native303EE0 private copy changed retained data/FP");++controls;
    }
    std::cout<<"{\"passed\":true,\"controls\":"<<controls<<",\"manufactured_unknown_values\":false}\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
