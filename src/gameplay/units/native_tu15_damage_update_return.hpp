#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"

namespace halo2::native_damage_update_return_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::run_owner;
using native_small_queries_detail::VerifySummary;

enum class Entry : unsigned { updated, notify, unchanged, count };
inline std::atomic<std::uint64_t> &coverage() {
    static std::atomic<std::uint64_t> value{};
    return value;
}

template<class State, class Notify>
struct Frame : Port<State> {
    Notify &notify;
    Entry entry;

    Frame(State &state, std::uint32_t *cursor, Trace<State> *trace, Notify &callback, Entry start)
        : Port<State>(state,cursor,trace,{}), notify(callback), entry(start) {}

    void finish(std::uint32_t restore_ebx, std::uint32_t return_pc) {
        auto &gp = this->gp;
        gp[ebx] = this->pop(restore_ebx);
        gp[esp] = this->flags.plus(gp[esp],0x70u,4);
        this->ret(return_pc,4);
        this->publish(return_pc);
    }

    void notify_return() {
        auto &gp = this->gp;
        auto &flags = this->flags;
        gp[edx] = this->read(0xd8b64u,gp[esp]+0x14u,4);
        const auto notify_bit = flags.logic(flags.right(gp[eax]&0xffu,3u,1,false)&1u,1);
        gp[eax] = (gp[eax]&~0xffu) | notify_bit;
        this->store(0xd8b6du,gp[esp]+0x24u,1,notify_bit);
        gp[eax] = this->read(0xd8b71u,gp[esp]+0x24u,4);
        gp[esi] = gp[ebp]+0x108u;
        gp[ebx] = gp[ebp]+0xfcu;
        gp[edi] = gp[ebp]+0xf4u;
        this->push(0xd8b87u,gp[eax]);
        this->call(0xd8b88u,0xd8b8du,0xdb0a0u,[&](State &state) {
            try {
                notify(state);
            } catch (...) {
                gp = state.gp;
                flags = state.flags;
                throw;
            }
        });
        gp[ecx] = (gp[ecx]&~0xffu) | this->read(0xd8b8du,gp[esp]+0x13u,1);
        gp[edi] = this->pop(0xd8b91u);
        gp[esi] = this->pop(0xd8b92u);
        gp[ecx] = (gp[ecx]&~0xffu) | flags.logic((gp[ecx]|gp[eax])&0xffu,1);
        gp[ebp] = this->pop(0xd8b95u);
        gp[eax] = (gp[eax]&~0xffu) | (gp[ecx]&0xffu);
        finish(0xd8b98u,0xd8b9cu);
    }

    void run() {
        auto &gp = this->gp;
        this->reached(static_cast<unsigned>(entry));
        try {
            switch (entry) {
            case Entry::updated:
                gp[edi] = this->pop(0xd8b58u);
                gp[esi] = this->pop(0xd8b59u);
                gp[ebp] = this->pop(0xd8b5au);
                gp[eax] = (gp[eax]&~0xffu) | 1u;
                finish(0xd8b5du,0xd8b61u);
                break;
            case Entry::notify:
                notify_return();
                break;
            case Entry::unchanged:
                gp[esi] = this->pop(0xd8b9fu);
                gp[ebp] = this->pop(0xd8ba0u);
                gp[eax] = (gp[eax]&~0xffu) | (gp[edx]&0xffu);
                finish(0xd8ba3u,0xd8ba7u);
                break;
            case Entry::count:
                throw std::logic_error("Invalid damage return entry");
            }
        } catch (...) {
            this->s.gp = gp;
            this->s.flags = this->flags;
            throw;
        }
    }
};

template<class State, class Reference, class Notify>
bool bridge(State &state, Entry entry, Reference reference, Notify notify) {
    if (state.image != SourcePhysicsContactImage::TitleUpdate15 || !state.selected_observation_pcs)
        return false;
    static const bool reference_mode = flag("HALO2_REFERENCE_DAMAGE_UPDATE_RETURN");
    static const bool verify_mode = flag("HALO2_VERIFY_DAMAGE_UPDATE_RETURN");
    static VerifySummary summary{"Damage update return"};
    constexpr std::uint32_t first[]{0xd8b58u,0xd8b64u,0xd8b9fu};
    const auto verify_reference = [&](State &input) {
        input.selected_observation_pcs = true;
        reference(input,entry);
    };
    return run_owner(state,first[static_cast<unsigned>(entry)],0xd8ba7u,reference_mode,verify_mode,
        coverage(),summary,verify_reference,[&](State &value,std::uint32_t *cursor,Trace<State> *trace) {
            return Frame<State,Notify>{value,cursor,trace,notify,entry};
        });
}
} // namespace halo2::native_damage_update_return_detail
