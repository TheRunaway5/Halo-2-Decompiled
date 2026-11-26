#pragma once
#include "halo2/physics_contact_runtime.hpp"
#include "physics/runtime/native_tu15_raw_x87_scope.hpp"
#include <array>
#include <cstdint>
#include <utility>

namespace halo2::native_mopp_ray_caller_detail {
struct Entry {
    std::uint32_t cache{},program{},opcode{},variables{},bounds{};
    bool valid_program_class{};
};
struct DirectIo {
    template<class Callback>void operator()(Callback&&callback)const{std::forward<Callback>(callback)();}
};

// The hosted caller uses the same provider and completed-store observer as
// retained production. The independent physical oracle uses its original
// access widths and alignment checks.
template<class Before> struct HostedMemory {
    static constexpr bool silent_hook = native_raw_x87_detail::silent_hook<Before>;
    Before &before;

    template<class State> void operator()(State &state, std::uint32_t pc) const { before(state, pc); }
};

template<class Before> inline constexpr bool hosted_memory = false;
template<class Before> inline constexpr bool hosted_memory<HostedMemory<Before>> = true;

// Private TU1.5 caller-frame foundation. It owns physical caller storage and
// observable ABI publication, not geometry decisions or instruction execution.
// No production entry uses it yet; admission and the complete ray controller
// must be qualified together before installing a default prefix.
template<class State,class Before,class Io=DirectIo>class Frame {
protected:
    State&s_;Before&before_;Io io_;
    template<class Callback>void io(Callback&&callback){io_(std::forward<Callback>(callback));}
    void at(std::uint32_t pc){
        if(s_.instruction_cursor&&s_.instruction_cursor->position)
            *s_.instruction_cursor->position=pc;
        if(!native_raw_x87_detail::skip_hook<Before>())io([&]{before_(s_,pc);});
    }
    std::uint32_t word(std::uint32_t address,unsigned width=4){
        std::array<std::byte,4>bytes{};
        if(!native_raw_x87_detail::direct_read(s_.memory,address,std::span(bytes).first(width)))
            io([&]{s_.memory.read(address,std::span(bytes).first(width));});
        std::uint32_t value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8u*n);
        return value;
    }
    template<class Commit>
    void hosted_store(std::uint32_t pc, std::uint32_t address, std::span<const std::byte> bytes, Commit commit) {
        // A store that starts without an observer publishes no event even if
        // the provider installs one. Every half of a vector checks separately.
        if (!s_.observe) {
            if (native_raw_x87_detail::scoped_access_reference() || !s_.memory.try_callback_free_write(address, bytes))
                io([&]{ s_.memory.write(address, bytes); });
            commit();
            return;
        }

        SourcePhysicsContactRecordStore event{pc, address, {}, {bytes.begin(), bytes.end()}};
        event.before.resize(bytes.size());
        io([&]{ s_.memory.read(address, event.before); });
        io([&]{ s_.memory.write(address, event.written); });
        commit();
        if (s_.observe) io([&]{ s_.observe(event); });
    }
    template<class Commit>
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value,Commit commit,bool positioned=false){
        if(!positioned)at(pc);
        std::array<std::byte,4>bytes{};
        for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(8u*n));
        if constexpr (hosted_memory<Before>) {
            hosted_store(pc, address, bytes, commit);
        } else {
            SourcePhysicsContactRecordStore event{pc,address,{},{}};
            if(s_.observe){event.before.resize(4);io([&]{s_.memory.read(address,event.before);});}
            io([&]{s_.memory.write(address,bytes);});commit();
            if(s_.observe){event.written.assign(bytes.begin(),bytes.end());io([&]{s_.observe(event);});}
        }
    }
    void push(std::uint32_t pc,std::uint32_t value,bool positioned=false){
        if constexpr (hosted_memory<Before>) {
            if (!positioned) at(pc);
            s_.gp[4] -= 4u;
            store(pc,s_.gp[4],value,[]{},true);
        } else {
            const auto address=s_.gp[4]-4u;
            store(pc,address,value,[&]{s_.gp[4]=address;},positioned);
        }
    }
    std::uint32_t spill_single(std::uint32_t pc,std::uint32_t address,bool pop,
                               native_raw_x87_detail::Scope<State>&scope){
        if constexpr (hosted_memory<Before>) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
            // Retained FST(P) publishes conversion status before the provider
            // sees the store. The logical pop follows any completed observer.
            std::uint32_t bits;
            asm volatile("fsts %0":"=m"(bits)::"memory");
            store(pc,address,bits,[]{},true);
            if (pop) {
                // Pop by changing TOP and the old top's tag. FINCSTP or FSTP
                // ST(0) would also clear the conversion's C1 rounding bit.
                std::array<std::uint32_t,7> environment;
                asm volatile("fnstenv %0":"=m"(environment)::"memory");
                constexpr auto top_mask = 0x3800u;
                const auto old_top = (environment[1] & top_mask) >> 11u;
                environment[1] = (environment[1] & ~top_mask) | (((old_top + 1u) & 7u) << 11u);
                environment[2] |= 3u << (old_top * 2u);
                asm volatile("fldenv %0"::"m"(environment):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
            }
            return bits;
#else
            throw std::logic_error("Hosted MOPP x87 spill requires an x86 backend");
#endif
        } else {
            const auto pending=scope.stage_single(pop);
            store(pc,address,pending.bits,[&]{scope.commit(pending);},true);
            return pending.bits;
        }
    }
    std::uint32_t pop(std::uint32_t pc){
        at(pc);const auto value=word(s_.gp[4]);s_.gp[4]+=4u;return value;
    }
public:
    Frame(State&s,Before&before,Io owner={}):s_(s),before_(before),io_(std::move(owner)){}
    Entry begin(){
        auto&g=s_.gp;
        push(0x30aa70u,g[5]);g[5]=g[4];
        g[4]=s_.flags.logic(g[4]&0xfffffff0u,4);
        g[4]=s_.flags.minus(g[4],0x194u,4);
        push(0x30aa7cu,g[3]);push(0x30aa7du,g[6]);
        at(0x30aa7eu);g[6]=word(g[5]+12u);
        at(0x30aa81u);g[0]=word(g[6],1);
        s_.flags.minus(g[0],0x6bu,4);
        Entry entry{};entry.program=g[6];entry.opcode=g[0];
        push(0x30aa87u,g[7]);
        entry.cache=g[1];store(0x30aa88u,g[4]+0x28u,g[1],[]{});
        store(0x30aa8cu,g[4]+0x3cu,999u,[]{});
        entry.valid_program_class=entry.opcode<=0x6bu;
        if(entry.valid_program_class){
            at(0x30aa9au);g[7]=word(g[5]+16u);entry.bounds=g[7];
            at(0x30aa9du);g[3]=word(g[5]+8u);entry.variables=g[3];
        }
        return entry;
    }
    // The typed DATA classifier owns this value. The original's two immutable
    // code-table fetches are replaced by that compiled format classification.
    void publish_program_class(std::uint32_t value){s_.gp[1]=value;}
    void finish(){
        auto&g=s_.gp;
        g[7]=pop(0x30b6afu);g[6]=pop(0x30b6b0u);g[3]=pop(0x30b6b1u);
        g[4]=g[5];g[5]=pop(0x30b6b4u);pop(0x30b6b5u);g[4]+=12u;
    }
};
} // namespace halo2::native_mopp_ray_caller_detail
