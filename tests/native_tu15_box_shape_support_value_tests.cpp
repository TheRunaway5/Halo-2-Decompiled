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
        if(p>=0x461960u&&std::uint64_t(p)+out.size()<=0x461970u){operations.push_back(J{{"kind","read"},{"address",p},{"bytes",out.size()}});for(unsigned n=0;n<out.size();++n)out[n]=std::byte((p+n)%4u==3u?0x80u:0u);return;}
        check(p>=base&&std::uint64_t(p-base)+out.size()<=bank.size(),"Box value unexpected read");
        operations.push_back(J{{"kind","read"},{"address",p},{"bytes",out.size()}});
        std::copy_n(bank.begin()+p-base,out.size(),out.begin());
    }
    void write(std::uint32_t p,std::span<const std::byte>data)override{
        if(fault==2)throw Boundary{};
        check(p>=base&&std::uint64_t(p-base)+data.size()<=bank.size(),"Box value unexpected write");
        operations.push_back(J{{"kind","write"},{"address",p},{"bytes",data.size()},{"written",hex(data)}});
        std::copy(data.begin(),data.end(),bank.begin()+p-base);
    }
};
J run(unsigned index,bool old,bool observed,int fault){
    Memory memory;memory.fault=fault;for(unsigned n=0;n<memory.bank.size();++n)memory.bank[n]=std::byte(n+index);
    J events=J::array();SourcePhysicsContactRecordObserver observe;
    if(observed)observe=[&](const auto&event){events.push_back(J{{"pc",event.instruction},{"address",event.address},{"before",hex(event.before)},{"written",hex(event.written)}});if(fault==3)throw Boundary{};};
    // Preserve the old numerical Environment admission and its existing
    // invalid-control/depth refusals before any box DATA access.
    NativeTu15PhysicsFrameFp fp{std::uint16_t(index%4u==3u?0u:0x37fu),std::uint16_t(0x4100u|((index%8u)<<11u)),0x1f80u|((index%4u)<<13u)};
    const auto original=fp;bool stopped{};std::string error;
    try{
        const auto function=old?polygon_support_reference::reference_contact_shape_support:native_tu15_contact_shape_support;
        function(memory,{Memory::base},{0x306790u},Memory::base+32u,index%8u==0u?Memory::base+32u:Memory::base+48u+(index%4u),{},fp,index%10u,observe);
    }catch(const Boundary&){stopped=true;}catch(const std::runtime_error&e){stopped=true;error=e.what();}
    check(fp.control_word==original.control_word&&fp.status==original.status&&fp.mxcsr==original.mxcsr,"Box value changed FP payload");
    return J{{"stopped",stopped},{"error",error},{"operations",memory.operations},{"events",events},{"bank",hex(memory.bank)},
        {"fp",{fp.control_word,fp.status,fp.mxcsr}}};
}
}
int main()try{
    unsigned comparisons{};for(unsigned n=0;n<128;++n)for(bool observed:{false,true})for(int fault=0;fault<4;++fault){
        check(run(n,false,observed,fault)==run(n,true,observed,fault),"Box public value protocol differs");++comparisons;
    }
    std::cout<<J{{"passed",true},{"comparisons",comparisons},{"backed_shape_direction_output_aliases",true},{"public_vector_event_bytes",16},{"raw_vector_protocol_qualified_here",false}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
