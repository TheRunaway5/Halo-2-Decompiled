#pragma once
#include "halo2/physics_contact_record.hpp"

namespace halo2 {
// Lazy mathematical input, independent of retained/private storage layout.
// Native projections use Value; real tagged fields use Memory. Neither is a
// source CPU frame or an executable dependency/result provider.
class NativeCampaignAiJumpPointTu15 {
public:
    virtual ~NativeCampaignAiJumpPointTu15()=default;
    virtual std::uint32_t coordinate(unsigned axis)const=0;
};
class NativeCampaignAiJumpPointMemoryTu15 final:public NativeCampaignAiJumpPointTu15 {
    const SourcePhysicsContactMemory &memory_;
    std::uint32_t address_;
public:
    NativeCampaignAiJumpPointMemoryTu15(const SourcePhysicsContactMemory &memory,std::uint32_t address):
        memory_(memory),address_(address){}
    std::uint32_t coordinate(unsigned axis)const override {
        std::array<std::byte,4> raw{};memory_.read(address_+axis*4u,raw);std::uint32_t value{};
        for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(raw[n])<<(n*8u);
        return value;
    }
};
class NativeCampaignAiJumpPointValueTu15 final:public NativeCampaignAiJumpPointTu15 {
    std::array<std::uint32_t,3> value_;
public:
    explicit NativeCampaignAiJumpPointValueTu15(std::array<std::uint32_t,3> value):value_(value){}
    std::uint32_t coordinate(unsigned axis)const override{return value_.at(axis);}
};
}
