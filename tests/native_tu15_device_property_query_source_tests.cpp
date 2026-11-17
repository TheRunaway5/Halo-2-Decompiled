#include "gameplay/objects/native_tu15_device_property_query_source_frame.hpp"
#include "native_tu15_object_property_source_test_harness.hpp"

using namespace halo2;
using namespace halo2::mopp_ray_frame_test;
namespace device_reference {
#include "device_property_query_reference.inc"
}
namespace {
struct DeviceProfile {
    using Current=device_reference::current::WorldState;
    using Fresh=device_reference::fresh::WorldState;
    static constexpr auto label="Device query";
    static constexpr auto sites=171u;
    static constexpr bool paired_nan=true;
    static bool prefix_row(std::size_t n) {return n<16||(n%8==6&&n<56);}
    static std::string fixture_name(std::uint32_t id) {return device_reference::fixture_name(id);}
    static std::uint32_t fixture_key(std::uint32_t id) {return device_reference::fixture_key(id);}
    static void run_native(auto&state,unsigned n) {device_reference::run_native(state,n);}
    static void run_reference(auto&state,unsigned n) {device_reference::run_reference(state,n);}
    static bool spill(std::uint32_t pc) {return pc==0x10bfe9u||pc==0x10c019u||pc==0x10c058u;}
    static void after_read(const auto&memory,std::uint32_t,std::span<std::byte>data) {
        if(memory.feedback==2&&(*memory.cursor==0x10c013u||*memory.cursor==0x10c052u)){
            const std::uint32_t denominator=0xc0000000u;std::memcpy(data.data(),&denominator,4);
            _mm_setcsr((_mm_getcsr()&~0x6000u)|0x4000u);
        }
    }
    static void after_write(auto&memory,std::uint32_t,std::span<const std::byte>) {
        if(memory.feedback==4){
            if(spill(*memory.cursor)||*memory.cursor==0x10c089u)*memory.observer=*memory.collector;
            if(*memory.cursor==0x10c098u||*memory.cursor==0x10c0a6u)*memory.observer={};
        }
    }
    static void after_observer(auto&memory,const auto&event) {
        if(memory.feedback==3){
            auto&state=*memory.state;
            if(spill(event.instruction)){
                state.fp.status^=0x200u;
                const auto bank=memory.at(event.address,4);const std::uint32_t value=0xbf000000u;
                auto&data=memory.banks.at(bank->first);
                std::memcpy(data.data()+event.address-bank->first,&value,4);
            }
            if(event.instruction==0x10c089u){
                state.flags.cf=false;state.flags.zf=!state.flags.zf;state.set(3,0x91u,1,0);
            }
        }
    }
    static void after_instruction(WorldState&state,std::uint32_t pc,int feedback) {
        if(feedback==1){
            if(pc==0x10bedbu)state.flags.cf=!state.flags.cf;
            if(pc==0x10c06fu||pc==0x10c08du){state.flags.cf=false;state.flags.zf=!state.flags.zf;}
            if(pc==0x10c09au||pc==0x10c0a8u)state.set(3,0xa5u,1,0);
        }
        if(feedback==5){
            if(pc==0x10bfcbu||pc==0x10bffdu||pc==0x10c011u||pc==0x10c050u||
               pc==0x10bfd7u||pc==0x10bff3u||pc==0x10c007u||pc==0x10c080u)state.fp.status|=0x200u;
            if(spill(pc)){state.fp.st(0)=0x1.000003p0L;state.fp.status&=~0x200u;}
        }
        if(feedback==6&&(pc==0x10beaeu||pc==0x10beb6u)){
            state.xmm[0]=_mm_set_ss(std::bit_cast<float>(0x7fc0a55au));
            state.xmm[3]=_mm_set_ss(std::bit_cast<float>(0x7fc0b66bu));
        }
    }
};
}
int main(int argc,char**argv) {
    return halo2::object_property_source_test::drive<DeviceProfile>(argc,argv);
}
