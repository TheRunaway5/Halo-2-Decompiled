#include "halo2/source_effect_world_locations.hpp"
#include "halo2/source_effect_location_admission.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_combat_action_tu15.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/raw_datum_pool.hpp"
#include "halo2/cache.hpp"
#include <cstring>
#include <set>
namespace halo2 {
namespace {
// Original17BA50/17DD50 also resolve encoded first-person graph nodes through
//1693AB. Its caller-owned stack/output live here; every source bank is read-only.
// This prevents the presentation consumer from masking off a node flag and
// incorrectly indexing the weapon's much smaller WORLD node bank.
class LocationCallMemory final:public SourcePhysicsContactMemory {
public:
    static constexpr std::uint32_t base=0x6fff0000u;
    const SourceRuntimeMemory &source;
    std::array<std::byte,65536> scratch{};
    explicit LocationCallMemory(const SourceRuntimeMemory &m):source(m){}
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        if(address>=base && std::uint64_t(address)+out.size()<=std::uint64_t(base)+scratch.size())
            std::memcpy(out.data(),scratch.data()+address-base,out.size());
        else source.read(address,out);
    }
    void write(std::uint32_t address,std::span<const std::byte> in)override {
        if(address<base || std::uint64_t(address)+in.size()>std::uint64_t(base)+scratch.size())
            throw FormatError("Original effect location resolver attempted a source-state write");
        std::memcpy(scratch.data()+address-base,in.data(),in.size());
    }
};
}
std::vector<SourceEffectLocation> source_effect_world_locations_tu15(const SourceRuntimeMemory &memory,
    std::uint32_t instance,std::uint32_t expected_definition) {
    const auto word=[&](std::uint32_t at) {
        const auto bytes=memory.view(at,4);std::uint32_t value=0;
        for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);
        return value;
    };
    const auto effects=word(0x4f720cu),locations=word(0x4f7208u);
    if(word(effects+0x20u)!=256u || word(effects+0x24u)!=400u ||
        word(locations+0x20u)!=512u || word(locations+0x24u)!=60u)
        throw FormatError("Combat location resolver requires its actual effect/location pools");
    const SourceRawDatumPoolServices storage{.read=[&](auto p,auto n){return memory.view(p,n);}};
    const auto effect=source_raw_datum_lookup(effects,instance,storage);
    if(!effect || word(effect+4u)!=expected_definition)
        throw FormatError("Combat location resolver lost its actual source effect allocation");
    const auto row=word(0x4f0414u)+(expected_definition&65535u)*16u;
    if(word(row+4u)!=expected_definition)throw FormatError("Combat location resolver lost its full effect tag identity");
    const auto tag=word(row+8u);
    const auto count=word(tag+0xcu);if(count>32u)throw FormatError("Combat effect location bank exceeds32");
    constexpr SourcePhysicsFactoryFloatingPoint fp{0x27f,0x1f80};
    LocationCallMemory call(memory);
    SourcePhysicsWorldRuntimeServices services;
    services.required_owner=[&](auto &m,const auto &c,auto &a,const auto &o) {
        if(source_combat_action_tu15_owns_entry(c.target))
            source_run_combat_action_tu15(m,c.target,a,fp,services,o);
        else if(source_physics_live_support_tu15_owns_entry(c.target))
            source_run_physics_live_support_tu15(m,c.target,a,fp,services,o);
        else throw FormatError("Original combat location resolver requires an unbound callee");
    };
    std::vector<SourceEffectLocation> out;std::set<std::uint32_t> visited;
    for(unsigned n=0;n<count;++n)for(auto datum=word(effect+0x7cu+n*4u);datum!=0xffffffffu;) {
        if(!visited.insert(datum).second || visited.size()>512)
            throw FormatError("Combat source location chain cycles or exceeds its real pool");
        const auto location=source_raw_datum_lookup(locations,datum,storage);
        if(!location)throw FormatError("Combat location resolver lost a full source location");
        // Original17D210 dispatches WORLD parts through17DCE0(mode0).
        // A registered weapon can retain both WORLD and encoded FP records
        // under one marker name; resolving both as WORLD duplicates parts.
        const auto node=Bytes(memory.view(location+2,2)).u16(0);
        if(!source_effect_world_location_admitted_tu15(node)) {
            datum=word(location+4u);continue;
        }
        SourcePhysicsWorldRuntimeAbi a{};
        a.gp[3]=location;a.gp[6]=effect;a.gp[7]=LocationCallMemory::base+0x5000u;
        a.gp[4]=LocationCallMemory::base+0xf000u;a.eflags=0x202u;
        // Native world presentation asks for the original WORLD branch (0).
        std::array<std::byte,8> arguments{};call.write(a.gp[4],arguments);
        source_run_combat_action_tu15(call,0x17ba50u,a,fp,services);
        if(a.gp[4]!=LocationCallMemory::base+0xf008u)
            throw FormatError("Original effect location resolver changed its caller stack contract");
        Transform frame;call.read(a.gp[0],std::as_writable_bytes(std::span(&frame,1)));
        out.push_back({word(word(tag+0x10u)+n*4u),frame});
        datum=word(location+4u);
    }
    return out;
}
}
