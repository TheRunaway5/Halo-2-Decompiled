#pragma once
#include "halo2/native_tu15_object_body_kernel.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2::native_object_body_detail {
// VehicleEnvironment publishes raw instruction callbacks. They may change
// registers, flags, providers or the next callback at every authored site.
// Keep those source boundaries while the shared typed lookup owns its tree.
template<class State>class RawFrame {
    State&s_;
    void step(std::uint32_t pc){if(s_.services.instruction)s_.services.instruction(pc);}
    std::uint32_t read(std::uint32_t address,unsigned width=4){
        return physics_world_runtime_detail::r(s_.memory,address,width);
    }
public:
    explicit RawFrame(State&s):s_(s){}
    std::uint32_t begin(){
        step(0xbcdf0u);s_.push(0xbcdf0u,s_.reg(6,4,0));
        step(0xbcdf1u);s_.push(0xbcdf1u,s_.reg(7,4,0));
        step(0xbcdf2u);s_.set(6,s_.reg(0,4,0),4,0);
        step(0xbcdf4u);s_.set(7,s_.flags.logic(s_.reg(7,4,0)^s_.reg(7,4,0),4),4,0);
        return s_.gp[6];
    }
    bool present(std::uint32_t){
        step(0xbcdf6u);s_.flags.minus(s_.reg(6,4,0),0xffffffffu,4);
        step(0xbcdf9u);return !s_.flags.zf;
    }
    std::uint32_t objects(){
        step(0xbcdfbu);s_.set(2,read(0x4ecbd0u),4,0);return s_.gp[2];
    }
    std::uint32_t limit(std::uint32_t){
        step(0xbce01u);s_.set(0,read(0x38u+s_.gp[2]),4,0);return s_.gp[0];
    }
    bool in_bounds(std::uint32_t,std::uint32_t){
        step(0xbce04u);s_.set(1,s_.reg(6,4,0),4,0);
        step(0xbce06u);s_.set(1,s_.flags.logic(s_.reg(1,4,0)&0xffffu,4),4,0);
        step(0xbce0cu);s_.flags.minus(s_.reg(1,4,0),s_.reg(0,4,0),4);
        step(0xbce0eu);return s_.flags.sf!=s_.flags.of;
    }
    std::uint32_t record(std::uint32_t){
        step(0xbce10u);s_.set(0,read(0x24u+s_.gp[2]),4,0);
        step(0xbce13u);s_.set(0,s_.flags.product(s_.reg(0,4,0),s_.reg(1,4,0)),4,0);
        step(0xbce16u);s_.set(0,s_.flags.plus(s_.reg(0,4,0),read(0x44u+s_.gp[2]),4),4,0);
        return s_.gp[0];
    }
    std::uint16_t salt(std::uint32_t){
        step(0xbce19u);s_.set(1,read(s_.gp[0],2),2,0);return std::uint16_t(s_.reg(1,2,0));
    }
    bool live(std::uint16_t){
        step(0xbce1cu);s_.flags.logic(s_.reg(1,2,0)&s_.reg(1,2,0),2);
        step(0xbce1fu);return !s_.flags.zf;
    }
    bool matches(std::uint32_t,std::uint16_t){
        step(0xbce21u);s_.set(1,std::uint32_t(std::int32_t(std::int16_t(s_.reg(1,2,0)))),4,0);
        step(0xbce24u);s_.set(6,s_.flags.right(s_.reg(6,4,0),0x10u,4,true),4,0);
        step(0xbce27u);s_.flags.minus(s_.reg(1,4,0),s_.reg(6,4,0),4);
        step(0xbce29u);return s_.flags.zf;
    }
    void admit(std::uint32_t){step(0xbce2bu);s_.set(7,s_.reg(0,4,0),4,0);}
    std::uint32_t finish(){
        step(0xbce2du);s_.set(0,s_.flags.logic(s_.reg(0,4,0)^s_.reg(0,4,0),4),4,0);
        step(0xbce2fu);s_.flags.logic(s_.reg(7,4,0)&s_.reg(7,4,0),4);
        step(0xbce31u);
        if(!s_.flags.zf){
            step(0xbce33u);s_.set(1,read(0x3u+s_.gp[7],1),1,0);
            step(0xbce36u);s_.set(2,1u,4,0);
            step(0xbce3bu);s_.set(2,s_.flags.shift(s_.reg(2,4,0),s_.reg(1,1,0),4),4,0);
            step(0xbce3du);s_.flags.logic(read(0xcu+s_.gp[4])&s_.reg(2,4,0),4);
            step(0xbce41u);
            if(!s_.flags.zf){step(0xbce43u);s_.set(0,read(0x8u+s_.gp[7]),4,0);}
        }
        step(0xbce46u);s_.set(7,s_.pop(),4,0);
        step(0xbce47u);s_.set(6,s_.pop(),4,0);
        step(0xbce48u);s_.pop();s_.gp[4]+=4u;return s_.gp[0];
    }
};
template<class State>std::uint32_t run_raw_frame(State&s){RawFrame view{s};return lookup(view);}
} // namespace halo2::native_object_body_detail
