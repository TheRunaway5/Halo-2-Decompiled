#include "halo2/native_tu15_contact_shape.hpp"
#include "reference/native_tu15_contact_shape_support_before.inc"
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
using J=nlohmann::json;
namespace {
struct Boundary {};
void check(bool value,const char*message){if(!value)throw std::runtime_error(message);}
std::string hex(std::span<const std::byte>data){constexpr char chars[]="0123456789abcdef";std::string out;for(auto b:data){const auto v=std::to_integer<unsigned>(b);out+=chars[v>>4];out+=chars[v&15];}return out;}
struct Memory:SourcePhysicsContactMemory {
    static constexpr std::uint32_t base=0x61000000;
    std::array<std::byte,64>bank{};
    mutable J operations=J::array();
    int fault{};
    void read(std::uint32_t p,std::span<std::byte>out)const override{
        if(fault==1)throw Boundary{};
        check(p>=base&&std::uint64_t(p-base)+out.size()<=bank.size(),"Zero value unexpected read");
        operations.push_back(J{{"kind","read"},{"address",p},{"bytes",out.size()}});
        std::copy_n(bank.begin()+p-base,out.size(),out.begin());
    }
    void write(std::uint32_t p,std::span<const std::byte>data)override{
        if(fault==2)throw Boundary{};
        check(p>=base&&std::uint64_t(p-base)+data.size()<=bank.size(),"Zero value unexpected write");
        operations.push_back(J{{"kind","write"},{"address",p},{"bytes",data.size()},{"written",hex(data)}});
        std::copy(data.begin(),data.end(),bank.begin()+p-base);
    }
};
J run(unsigned index,bool old,bool observed,int fault){
    Memory memory;memory.fault=fault;for(unsigned n=0;n<memory.bank.size();++n)memory.bank[n]=std::byte(n+index);
    J events=J::array();SourcePhysicsContactRecordObserver observe;
    if(observed)observe=[&](const auto&event){events.push_back(J{{"pc",event.instruction},{"address",event.address},{"before",hex(event.before)},{"written",hex(event.written)}});if(fault==3)throw Boundary{};};
    // The zero-owner branch precedes the old numerical Environment. Preserve
    // its broader value-only acceptance of these opaque FP/depth fields.
    NativeTu15PhysicsFrameFp fp{std::uint16_t(index*513u),std::uint16_t(index*257u),index*0x01010101u};
    const auto original=fp;bool stopped{};std::string error;
    try{
        const auto function=old?polygon_support_reference::reference_contact_shape_support:native_tu15_contact_shape_support;
        function(memory,{0xdead0000u+index},{0x2e8d20u},0xf1000000u+index,Memory::base+16u+(index%16u),{},fp,index%2u?~0u:0u,observe);
    }catch(const Boundary&){stopped=true;}catch(const std::runtime_error&e){stopped=true;error=e.what();}
    check(fp.control_word==original.control_word&&fp.status==original.status&&fp.mxcsr==original.mxcsr,"Zero value changed FP payload");
    return J{{"stopped",stopped},{"error",error},{"operations",memory.operations},{"events",events},{"bank",hex(memory.bank)},
        {"fp",{fp.control_word,fp.status,fp.mxcsr}}};
}
}
int main()try{
    unsigned comparisons{};for(unsigned n=0;n<128;++n)for(bool observed:{false,true})for(int fault=0;fault<4;++fault){
        check(run(n,false,observed,fault)==run(n,true,observed,fault),"Zero public value protocol differs");++comparisons;
    }
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"unbacked_shape_and_direction",true},{"public_vector_event_bytes",16},{"raw_vector_protocol_qualified_here",false}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
