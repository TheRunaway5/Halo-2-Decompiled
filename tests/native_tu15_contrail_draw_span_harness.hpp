#pragma once

#include "native_tu15_frame_misc_harness.hpp"
#include "rendering/effects/native_tu15_contrail_vertex_emission.hpp"

#include <algorithm>

namespace contrail_draw_span_harness {
using namespace frame_misc_harness;

using Abi=SourcePhysicsWorldRuntimeAbi;

inline bool identical(const Abi &a,const Abi &b) {
    return same_abi(a,b) && a.fs_base==b.fs_base && a.floating_point && b.floating_point &&
        a.floating_point->mxcsr==b.floating_point->mxcsr && !a.active_mmx && !b.active_mmx;
}

struct Packet {
    char kind;
    std::uint32_t address;
    std::size_t width;
    Abi abi;

    bool operator==(const Packet &o) const {
        return kind==o.kind && address==o.address && width==o.width && identical(abi,o.abi);
    }
};

struct Provider : Memory {
    mutable std::vector<Packet> packets;
    std::vector<std::vector<std::byte>> writes;
    const WorldState *state{};
    unsigned stop;
    bool change_fp;

    Provider(const Memory &source,unsigned fail_at,bool vary_fp)
        : Memory(source.clone()),stop(fail_at),change_fp(vary_fp) {}

    void visit(char kind,std::uint32_t address,std::size_t width) const {
        packets.push_back(Packet{kind,address,width,state->save()});
        const auto step=packets.size();
        if (change_fp) {
            const auto cw=std::uint16_t(0x037fu | ((step&3u)<<10u));
            asm volatile("fldcw %0"::"m"(cw):"memory");
            source_load_x87_exception_flags(std::uint16_t(step&0x3fu));
            _mm_setcsr(0x1f80u | (unsigned(step&3u)<<13u) | unsigned(step&0x3fu));
        }
        if (step==stop) throw std::runtime_error("Injected contrail draw provider fault");
    }

    void read(std::uint32_t a,std::span<std::byte> out) const override {
        visit('r',a,out.size());
        Memory::read(a,out);
    }

    void write(std::uint32_t a,std::span<const std::byte> in) override {
        writes.emplace_back(in.begin(),in.end());
        visit('w',a,in.size());
        Memory::write(a,in);
    }
};

enum class Policy { draw, unscoped, trace, observer };
struct Outcome {
    std::vector<std::uint32_t> trace;
    std::vector<SourcePhysicsContactRecordStore> stores;
    std::string fault;
    Abi abi;
    bool native{};
};

template<auto Reference, auto Selected>
Outcome run(Provider &memory,const Case &input,bool raw,Policy policy) {
    Outcome out;
    SourcePhysicsWorldRuntimeServices services;
    if (raw || policy==Policy::trace) services.instruction=[&](auto pc) { out.trace.push_back(pc); };
    SourcePhysicsContactRecordObserver observe;
    if (policy==Policy::observer) observe=[&](const auto &record) { out.stores.push_back(record); };
    std::optional<native_contrail_vertex_emission_detail::ScopedDrawServices> scope;
    if (policy!=Policy::unscoped) scope.emplace(services);
    FpScope fp(*input.abi.floating_point);
    WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
    state.load(input.abi);
    memory.state=&state;
    try {
        if (raw) Reference(state);
        else out.native=Selected(state);
    } catch (const std::runtime_error &error) {
        out.fault=error.what();
    }
    out.abi=state.save();
    memory.state=nullptr;
    return out;
}

struct Stats {
    unsigned cases{},faults{},packets{},writes{},admissions{};
    std::map<std::uint32_t,unsigned> sites;
};

template<auto Reference, auto Selected>
unsigned compare(const Case &input,unsigned id,Stats &stats,bool forced,Policy policy=Policy::draw,unsigned cut=0) {
    const bool native=policy==Policy::draw && !forced;
    Provider original(input.memory,cut,id%5u==0u),selected(input.memory,cut,id%5u==0u);
    const auto a=run<Reference, Selected>(original,input,true,policy),b=run<Reference, Selected>(selected,input,false,policy);
    const auto mismatch=std::mismatch(original.packets.begin(),original.packets.end(),selected.packets.begin(),selected.packets.end());
    if (mismatch.first!=original.packets.end() || mismatch.second!=selected.packets.end() ||
            a.fault!=b.fault || a.fault.empty()!=(!cut) || (!cut && b.native!=native) ||
            !identical(a.abi,b.abi) || original.writes!=selected.writes || a.stores!=b.stores || !original.same(selected) ||
            (policy==Policy::trace ? a.trace!=b.trace : !b.trace.empty())) {
        std::fprintf(stderr,"Contrail draw case %u cut %u packets %zu/%zu mismatch %zu faults [%s]/[%s]\n",
            id,cut,original.packets.size(),selected.packets.size(),std::size_t(mismatch.first-original.packets.begin()),
            a.fault.c_str(),b.fault.c_str());
        if (mismatch.first!=original.packets.end() && mismatch.second!=selected.packets.end())
            std::fprintf(stderr,"first packet %c:%x / %c:%x\n",mismatch.first->kind,mismatch.first->address,
                mismatch.second->kind,mismatch.second->address);
        std::exit(1);
    }
    ++stats.cases;
    stats.faults+=bool(cut);
    stats.admissions+=b.native;
    stats.packets+=unsigned(original.packets.size());
    stats.writes+=unsigned(original.writes.size());
    if (!cut) for (auto pc : a.trace) ++stats.sites[pc];
    return unsigned(original.packets.size());
}
} // namespace contrail_draw_span_harness
