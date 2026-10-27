#include "halo2/source_first_person_action_tu15.hpp"
#include <bit>
#include "halo2/cache.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include "halo2/raw_datum_pool.hpp"
namespace halo2 {
namespace {thread_local SourceFirstPersonActionObserverTu15 *active;
thread_local SourceFirstPersonCompositionObserverTu15 *active_composition;
class BorrowedInputGuard {
    const SourceRuntimeMemory *memory_{};
public:
    explicit BorrowedInputGuard(const SourcePhysicsContactMemory &memory):
        memory_(dynamic_cast<const SourceRuntimeMemory*>(&memory)) {
        if(!memory_)return;
        source_runtime_register_write_guard(*memory_,this,[](auto,auto){
            throw FormatError("Native FP producer cannot write its borrowed authority input");});
        try {source_runtime_register_topology_guard(*memory_,this,[](auto,auto){
            throw FormatError("Native FP producer cannot change borrowed authority topology");});}
        catch(...) {source_runtime_unregister_write_guard(*memory_,this);throw;}
    }
    ~BorrowedInputGuard() {
        if(!memory_)return;
        source_runtime_unregister_topology_guard(*memory_,this);
        source_runtime_unregister_write_guard(*memory_,this);
    }
};
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t at,unsigned n=4) {
    std::array<std::byte,4>b{};m.read(at,std::span(b).first(n));return Bytes(b).u32(0);
}
std::uint32_t body(const SourcePhysicsContactMemory &memory,std::uint32_t pool,std::uint32_t datum) {
    std::array<std::byte,4> storage;
    SourceRawDatumPoolServices services;
    services.read=[&](std::uint32_t at,std::size_t count)->std::span<const std::byte> {
        if(count>storage.size())throw FormatError("Native FP action datum lookup exceeded its source operand");
        memory.read(at,std::span(storage).first(count));return std::span(storage).first(count);
    };
    if(read(memory,pool+0x24u)!=12u)throw FormatError("Native FP action object-pool stride differs");
    const auto row=source_raw_datum_lookup(pool,datum,services);
    if(!row)throw FormatError("Native FP action requires its live full-salt object datum");
    const auto result=read(memory,row+8u);
    if(!result)throw FormatError("Native FP action object has no native body owner");
    return result;
}
std::uint32_t caller(std::uint32_t returned) {
    switch(returned) {
    case 0xf0c5fu:return 0xf0c5au;
    case 0xf0c84u:return 0xf0c7fu;
    case 0xf0cddu:return 0xf0cd8u;
    case 0xf0d1eu:return 0xf0d19u;
    case 0xf0df6u:return 0xf0df1u;
    default:return 0;
    }
}}
SourceFirstPersonActionObserverTu15::SourceFirstPersonActionObserverTu15(
    const SourcePhysicsContactMemory&m,Observer observer,Phase phase):memory_(m),observer_(std::move(observer)),
    phase_(std::move(phase)),previous_(active) {active=this;}
SourceFirstPersonActionObserverTu15::~SourceFirstPersonActionObserverTu15(){active=previous_;}
bool SourceFirstPersonActionObserverTu15::phase_bound(const SourcePhysicsContactMemory&m)noexcept {
    return active && &active->memory_==&m && bool(active->phase_);
}
void SourceFirstPersonActionObserverTu15::phase(const SourcePhysicsContactMemory&m,
    const SourcePhysicsWorldRuntimeAbi&abi,const SourcePhysicsWorldRuntimeServices&cpu) {
    if(active && &active->memory_==&m && active->phase_) {
        // Real native CPU inputs remain part of the enclosing MatchStep tape.
        // This is a private producer borrow, not a diagnostic observer scope.
        BorrowedInputGuard guard(m);active->phase_(m,abi,cpu);
    }
}
void SourceFirstPersonActionObserverTu15::entry(const SourcePhysicsContactMemory&m,
    const SourcePhysicsWorldRuntimeAbi&abi,const SourcePhysicsWorldRuntimeServices&cpu) {
    if(!active || &active->memory_!=&m || !active->observer_)return;
    const auto instruction=caller(read(m,abi.gp[4]));
    if(instruction)observe(m,{instruction,0x16b736u,false},abi,cpu);
}
void SourceFirstPersonActionObserverTu15::observe(const SourcePhysicsContactMemory&m,
    const SourcePhysicsContactRuntimeCall&call,const SourcePhysicsWorldRuntimeAbi&abi,
    const SourcePhysicsWorldRuntimeServices&cpu) {
    if(!active || &active->memory_!=&m || !active->observer_ || call.target!=0x16b736u || call.tail)return;
    const auto sp=abi.gp[4];
    if(!caller(read(m,sp)) || caller(read(m,sp))!=call.instruction)
        throw FormatError("Native first-person request differs from its original caller");
    const auto pool=read(m,0x4ecbd0u),hand=read(m,sp+12u),weapon=read(m,sp+8u),view=read(m,sp+4u);
    if(hand>=2u)throw FormatError("Native first-person request hand escaped its original capacity");
    std::uint32_t unit;
    if(call.instruction==0xf0df1u) {
        const auto index=abi.gp[6],count=read(m,pool+0x20u),stride=read(m,pool+0x24u);
        if(index>0xffffu || index>=count || index>=read(m,pool+0x38u) || stride!=12u)
            throw FormatError("Native first-person request unit pool differs");
        unit=(read(m,read(m,pool+0x44u)+index*stride,2)<<16u)|index;
    } else {
        // F0BE0 preserves ESI as the hand, not F0D50's unit index. Its
        // actual weapon object retains the full native unit owner at+154.
        if(weapon==0xffffffffu)return;
        unit=read(m,body(m,pool,weapon)+0x154u);
        if(unit==0xffffffffu)return;
    }
    const auto unit_body=body(m,pool,unit);
    if(weapon!=0xffffffffu && read(m,body(m,pool,weapon)+0x154u)!=unit)
        throw FormatError("Native first-person request weapon differs from its full unit owner");
    const auto inventory_slot=std::bit_cast<std::int8_t>(std::uint8_t(read(m,unit_body+0x212u+hand,1)));
    if(inventory_slot < -1 || inventory_slot>=4)
        throw FormatError("Native first-person request inventory slot escaped its source capacity");
    const auto expected=inventory_slot==-1?0xffffffffu:read(m,unit_body+0x218u+std::uint32_t(inventory_slot)*4u);
    if(weapon!=expected && (call.instruction==0xf0df1u || view!=0xffffffffu))
        throw FormatError("Native first-person request differs from its unit inventory owner");
    const SourceFirstPersonNativeActionTu15 action{
        read(m,read(m,0x51df54u)+8u),unit,weapon,view,abi.gp[3],hand,call.instruction};
    BorrowedInputGuard guard(m);active->observer_(m,abi,cpu,action);
}
SourceFirstPersonCompositionObserverTu15::SourceFirstPersonCompositionObserverTu15(
    const SourcePhysicsContactMemory &memory,Observer observer):memory_(memory),observer_(std::move(observer)),
    previous_(active_composition) {active_composition=this;}
SourceFirstPersonCompositionObserverTu15::~SourceFirstPersonCompositionObserverTu15() {
    active_composition=previous_;
}
void SourceFirstPersonCompositionObserverTu15::entry(const SourcePhysicsContactMemory &memory,
    const SourcePhysicsWorldRuntimeAbi &caller) {
    if(!active_composition || &active_composition->memory_!=&memory || !active_composition->observer_)return;
    const auto sp=caller.gp[4];
    // Explicit sampling, render-cache construction and role installation also
    // call16A4DD. Only this original temporal-owner caller publishes inputs.
    if(read(memory,sp)!=0x16a47eu)return;
    const auto view=read(memory,sp+4u),hand=read(memory,sp+8u);
    if(view>=4u || hand>=2u)throw FormatError("Native FP composition escaped its original view/hand capacity");
    BorrowedInputGuard guard(memory);active_composition->observer_(memory,caller,view,hand);
}
}
