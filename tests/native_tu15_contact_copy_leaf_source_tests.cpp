#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_contact_copy_leaf_source_frame.hpp"
#include <algorithm>
#include <bit>
#include <iostream>
#include <map>
#include <string_view>

namespace {
using namespace halo2;
using namespace halo2::physics_contact_runtime_detail;
using native_contact_copy_leaf_detail::SourceFormat;
#include "contact_copy_source_reference.inc"

enum class Change { none, trace, clear, rebind, install_observer, clear_observer, live_registers, host_fp };
struct Stop { char kind; unsigned ordinal; friend bool operator==(const Stop&, const Stop&) = default; };
struct Packet {
    std::array<std::uint32_t,8> gp;
    std::array<std::array<std::uint32_t,4>,8> xmm;
    std::array<std::array<std::byte,10>,8> x87, mmx;
    std::uint32_t flags, fs, cursor, mxcsr;
    std::uint16_t cw, status;
    unsigned depth, mmx_mask;
    bool has_mmx;
    friend bool operator==(const Packet&, const Packet&) = default;
};
struct Event {
    char kind;
    std::uint32_t address, site;
    std::vector<std::byte> before, data;
    Packet state;
    friend bool operator==(const Event&, const Event&) = default;
};
struct Result {
    Packet state;
    std::vector<Event> events;
    std::map<std::uint32_t,std::byte> written;
    std::optional<Stop> stop;
};
struct Memory final : SourcePhysicsContactMemory {
    unsigned seed{};
    mutable std::map<std::uint32_t,std::byte> bytes;
    mutable std::vector<Event> events;
    mutable std::array<unsigned,256> counts{};
    std::optional<Stop> fail;
    std::function<Packet()> capture;
    std::function<void(char,unsigned,std::uint32_t)> feedback;

    std::byte initial(std::uint32_t address) const {
        return std::byte((address * 33u ^ (address >> 8u) ^ (address >> 16u) ^ seed * 19u) & 255u);
    }
    std::byte get(std::uint32_t address) const {
        const auto found = bytes.find(address);
        return found == bytes.end() ? initial(address) : found->second;
    }
    void put(std::uint32_t address, std::uint32_t value) {
        for (unsigned n = 0; n < 4; ++n) bytes[address+n] = std::byte(value >> (n*8u));
    }
    unsigned event(char kind, std::uint32_t address, std::uint32_t site = 0,
        std::span<const std::byte> before = {}, std::span<const std::byte> data = {}) const {
        const auto ordinal = counts[static_cast<unsigned char>(kind)]++;
        events.push_back({kind,address,site,{before.begin(),before.end()},{data.begin(),data.end()},capture()});
        if (fail && *fail == Stop{kind,ordinal}) throw *fail;
        return ordinal;
    }
    void read(std::uint32_t address, std::span<std::byte> out) const override {
        for (unsigned n = 0; n < out.size(); ++n) out[n] = get(address+n);
        const auto ordinal = event('r',address,0,{},out);
        if (feedback) feedback('r',ordinal,address);
    }
    void write(std::uint32_t address, std::span<const std::byte> in) override {
        const auto ordinal = event('w',address,0,{},in);
        for (unsigned n = 0; n < in.size(); ++n) bytes[address+n] = in[n];
        if (feedback) feedback('w',ordinal,address);
    }
};

template<class State> Packet packet(const State& s, std::uint32_t cursor) {
    const auto abi = s.save();
    Packet result{abi.gp,abi.xmm,{},{},abi.eflags,abi.fs_base,cursor,
        abi.floating_point->mxcsr,abi.floating_point->x87_control_word,abi.x87_status,
        abi.x87_depth,0,abi.active_mmx.has_value()};
    for (unsigned n = 0; n < abi.x87_depth; ++n)
        std::memcpy(result.x87[n].data(), &abi.x87[n], 10);
    if (abi.active_mmx) {
        result.mmx_mask = abi.active_mmx->known_mask;
        result.mmx = abi.active_mmx->physical;
    }
    return result;
}

struct Case {
    bool invalid{}, selected{}, callback{}, cursor{}, observer{};
    unsigned seed{};
    Change change{};
    std::optional<Stop> fail{};
};

template<class State, class Run> Result execute(Case input, Run run) {
    physics_step_detail::FpScope host{{0x037fu,0x1f80u}};
    const auto sp = 0x61000120u;
    const auto source = std::array{0x61002000u,0x61002003u,0x61001000u,0xffffffd0u,sp-0x24u}[input.seed%5];
    const auto destination = std::array{0x61003000u,source+3u,source-7u,0xfffffff0u,sp}[input.seed%5];
    Memory m; m.seed=input.seed; m.fail=input.fail;
    m.put(sp,0x63000000u); m.put(sp+4u,source);
    SourcePhysicsContactRuntimeServices services;
    SourcePhysicsContactRecordObserver observer;
    std::uint32_t cursor=0x1234u;
    bool trace{};
    std::function<void(std::uint32_t)> touch_registers;
    const auto callback=[&](auto pc) {
        m.event('c',pc); cursor=pc;
        if (touch_registers) touch_registers(pc);
    };
    const auto replacement=[&](auto pc) { m.event('b',pc); cursor=pc; };
    if (input.callback) services.instruction=callback;
    constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};
    if (input.cursor)
        services.instruction_cursor={&cursor,&trace,nullptr,service_sites,std::this_thread::get_id(),
            &services.instruction.target_type(),true,true};
    const auto observe=[&](const SourcePhysicsContactRecordStore& store) {
        m.event('o',store.address,store.instruction,store.before,store.written);
    };
    if (input.observer) observer=observe;
    State s{m,observer,services,SourcePhysicsContactImage::TitleUpdate15};
    if constexpr (requires { s.selected_observation_pcs; }) s.selected_observation_pcs=input.selected;
    SourcePhysicsContactRuntimeAbi abi{};
    abi.gp={0xabcdef99u,destination,0xfedcba45u,7u,sp,0x12345678u,42u,0xfffffff9u};
    abi.eflags=0x202u | (input.seed%2 ? 0xcd5u : 0x10u); abi.fs_base=0x2a000000u;
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x037fu,0x1f80u | (input.seed%4u)<<13u};
    abi.x87_depth=input.seed%5; abi.x87_status=(input.seed%8u)<<11u;
    for (unsigned n=0;n<abi.x87_depth;++n) abi.x87[n]=n+0.125L;
    for (unsigned n=0;n<8;++n) for (unsigned lane=0;lane<4;++lane)
        abi.xmm[n][lane]=0xfff28000u+input.seed*37u+n*8u+lane;
    if constexpr (requires { s.active_mmx; }) if (input.seed%3==2) {
        abi.active_mmx.emplace(); abi.x87_depth=abi.x87_status=0;
        abi.active_mmx->known_mask=static_cast<std::uint8_t>(input.seed*19u);
        for (unsigned n=0;n<8;++n) for (unsigned byte=0;byte<10;++byte)
            abi.active_mmx->physical[n][byte]=std::byte(input.seed*11u+n*7u+byte);
    }
    s.load(abi);
    m.capture=[&]{return packet(s,cursor);};
    if (input.change==Change::live_registers) touch_registers=[&](auto pc) {
        if (pc==0x300109u || pc==0x303ba9u) s.xmm[0]=_mm_castsi128_ps(_mm_set_epi32(4,3,2,1));
        if (pc==0x300117u || pc==0x303bb7u) s.gp[2]=0x88776655u;
    };
    m.feedback=[&](char kind,unsigned ordinal,std::uint32_t address) {
        if (kind=='r' && ordinal==0) {
            if (input.change==Change::trace) trace=true;
            if (input.change==Change::clear) services.instruction={};
            if (input.change==Change::rebind) services.instruction=replacement;
        }
        if (input.change==Change::install_observer && kind=='w' && ordinal==0) observer=observe;
        if (input.change==Change::clear_observer && kind=='r' && address==destination) observer={};
        if (input.change==Change::host_fp) _mm_setcsr(0x1f80u | ((ordinal%4u)<<13u) | (ordinal%64u));
        if (input.change==Change::live_registers) s.gp[5]^=0x01010202u;
    };
    std::optional<Stop> stop;
    try { run(s,input.invalid); } catch (const Stop& error) { stop=error; }
    return {packet(s,cursor),std::move(m.events),std::move(m.bytes),stop};
}

void check(bool ok, std::string_view context, const char* what) {
    if (!ok) throw std::runtime_error(std::string(context)+": "+what);
}
struct Counts { unsigned paths{}, packets{}, prefixes{}, changed{}; };

template<class State, SourceFormat Format, class Run, class Resolve>
Counts compare(std::string_view context, Run retained, Resolve resolve, bool observed_world) {
    Counts counts;
    const auto candidate=[&](State& s,bool invalid) {
        if (invalid) native_contact_copy_leaf_detail::run_invalid_source_private<Format>(s,resolve);
        else native_contact_copy_leaf_detail::run_copy_source_private<Format>(s);
    };
    const auto pair=[&](Case input) {
        auto expected=execute<State>(input,retained),actual=execute<State>(input,candidate);
        check(actual.stop==expected.stop,context,"source-copy exception differs");
        check(actual.state==expected.state,context,"source-copy final ABI/FP/cursor differs");
        check(actual.events==expected.events,context,"source-copy ordered callback/provider/observer ABI differs");
        check(actual.written==expected.written,context,"source-copy committed bytes differ");
        check(actual.stop==input.fail,context,"declared source fault did not occur");
        ++counts.paths; counts.packets+=actual.events.size(); counts.prefixes+=bool(actual.stop);
        counts.changed+=input.change!=Change::none;
        return actual;
    };
    for (const bool selected : {false,true}) {
        if (selected && !observed_world) continue;
        for (const bool invalid : {false,true}) {
            for (unsigned n=0;n<20;++n) for (const bool callbacks : {false,true})
                for (const bool observer : {false,true}) pair({invalid,selected,callbacks,callbacks,observer,n});
            for (const auto change : {Change::trace,Change::clear,Change::rebind,Change::install_observer,
                Change::clear_observer,Change::live_registers,Change::host_fp}) {
                const bool start_observer=change!=Change::install_observer;
                for (unsigned n=0;n<5;++n) pair({invalid,selected,true,change!=Change::live_registers,start_observer,n,change});
            }
            const auto ordinary=pair({invalid,selected,true,false,true,0});
            std::map<char,unsigned> seen;
            for (const auto& event : ordinary.events) {
                const auto ordinal=seen[event.kind]++;
                pair({invalid,selected,true,false,true,0,Change::none,Stop{event.kind,ordinal}});
            }
            if (!invalid) {
                const auto installed=pair({invalid,selected,true,false,false,0,Change::install_observer});
                const auto found=std::find_if(installed.events.begin(),installed.events.end(),[](const auto& e){return e.kind=='o';});
                check(found!=installed.events.end() && found->address==0x61003008u && found->data.size()==8,
                    context,"observer install must begin at the second source vector half");
            }
        }
    }
    return counts;
}

template<class State> auto route(void(*copy)(State&),void(*invalid)(State&)) {
    return [=](State& s,bool sentinel){ (sentinel ? invalid : copy)(s); };
}
}

int main() try {
    const auto a=compare<State,SourceFormat::direct>("Contact raw",route(contact_raw::run_300100,contact_raw::run_307fe0),contact_raw::selected,false);
    const auto b=compare<State,SourceFormat::literal>("Contact prepared",route(contact_prepared::run_300100,contact_prepared::run_307fe0),contact_prepared::selected,false);
    const auto c=compare<world_raw::ObservedWorldState,SourceFormat::direct>("World raw",route(world_raw::run_300100,world_raw::run_307fe0),world_raw::selected,true);
    const auto d=compare<world_prepared::ObservedWorldState,SourceFormat::direct>("World prepared (direct)",route(world_prepared::run_300100,world_prepared::run_307fe0),world_prepared::selected,true);
    std::cout<<"{\"passed\":true,\"cases\":"<<a.paths+b.paths+c.paths+d.paths
        <<",\"full_pre_event_packets\":"<<a.packets+b.packets+c.packets+d.packets
        <<",\"provider_callback_observer_stops\":"<<a.prefixes+b.prefixes+c.prefixes+d.prefixes
        <<",\"changed_policy_or_register_cases\":"<<a.changed+b.changed+c.changed+d.changed<<"}\n";
    return 0;
} catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
