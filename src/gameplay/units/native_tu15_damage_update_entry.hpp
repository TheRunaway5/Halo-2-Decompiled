#pragma once

#include "gameplay/native_tu15_small_queries_common.hpp"

namespace halo2::native_damage_update_entry_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::flag;
using native_small_queries_detail::run_owner;

enum class Resume : std::uint32_t {
    retained = 0xffffffffu, finish = 0xd8b9fu, response = 0xd83f0u, validate = 0xd84f4u,
};
enum Branch : unsigned { immunity_published, no_parameters, no_sections, pending_response, branch_count };
inline std::atomic<std::uint64_t> &coverage() {
    static std::atomic<std::uint64_t> value{};
    return value;
}
constexpr std::uint32_t object_array = 0x4e78d0u, game_options = 0x4edf18u, timer_option = 0x4fcdbcu;

template<class State, class Route, class Select>
struct Frame : Port<State> {
    Route &describe;
    Select &select;
    Resume &resume;

    Frame(State &state, std::uint32_t *cursor, Trace<State> *trace, Route &child, Select &map, Resume &next)
        : Port<State>(state,cursor,trace,{}), describe(child), select(map), resume(next) {}

    std::uint32_t mapped(std::uint32_t pc, std::uint32_t canonical, unsigned bytes) {
        return this->read(pc,select(canonical),bytes);
    }
    std::uint32_t signed_word(std::uint32_t pc, std::uint32_t address) {
        return static_cast<std::uint32_t>(static_cast<std::int16_t>(this->read(pc,address,2)));
    }

    void load_object() {
        auto &gp = this->gp;
        auto &flags = this->flags;
        gp[esp] = flags.minus(gp[esp],0x70u,4);
        gp[ecx] = this->read(0xd8333u,gp[esp]+0x74u,4);
        gp[edx] = mapped(0xd8337u,object_array,4);
        gp[edx] = this->read(0xd833du,gp[edx]+0x44u,4);
        gp[eax] = flags.logic(gp[ecx]&0xffffu,4);
        this->push(0xd8347u,gp[ebx]);
        this->push(0xd8348u,gp[ebp]);
        gp[eax] = flags.shift(3u*gp[eax],2u,4);
        gp[ebp] = this->read(0xd834fu,gp[edx]+gp[eax]+8u,4);
        this->push(0xd8353u,gp[esi]);
        this->store(0xd8354u,gp[esp]+0x1cu,4,gp[eax]);
        this->call(0xd8358u,0xd835du,select(0xd7160u),[&](State &state) {
            try {
                describe(state);
            } catch (...) {
                gp = state.gp;
                flags = state.flags;
                throw;
            }
        });
    }

    void update_immunity_timer() {
        auto &gp = this->gp;
        auto &flags = this->flags;
        set_low_byte(gp[ecx],this->read(0xd835du,gp[ebp]+0xaau,1));
        gp[ebx] = flags.shift(1u,low_byte(gp[ecx]),4);
        set_low_byte(gp[edx],flags.logic(0u,1));
        this->store(0xd836cu,gp[esp]+0x14u,4,gp[eax]);
        this->store(0xd8370u,gp[esp]+0xfu,1,low_byte(gp[edx]));
        flags.logic(gp[ebx]&3u,1);
        if (flags.zf) return;
        gp[ecx] = mapped(0xd8379u,game_options,4);
        this->compare32(this->read(0xd837fu,gp[ecx]+8u,4),1u);
        if (!flags.zf) return;
        set_low_byte(gp[ecx],mapped(0xd8385u,timer_option,1));
        this->test8(gp[ecx]);
        if (flags.zf) return;
        this->compare32(this->read(0xd838fu,gp[ebp]+0x13cu,4),0xffffffffu);
        if (flags.zf) return;
        this->store(0xd8398u,gp[ebp]+0x104u,2,10u);
        this->reached(immunity_published);
    }

    void finish(Branch branch, Resume next, std::uint32_t pc) {
        this->reached(branch);
        resume = next;
        this->publish(pc);
    }

    void select_sections() {
        auto &gp = this->gp;
        auto &flags = this->flags;
        this->test32(gp[eax]);
        if (flags.zf) {
            finish(no_parameters,Resume::finish,0xd83a3u);
            return;
        }
        gp[ecx] = this->read(0xd83a9u,gp[eax]+0xbcu,4);
        this->test32(gp[ecx]);
        this->push(0xd83b1u,gp[edi]);
        this->store(0xd83b2u,gp[esp]+0x14u,4,0u);
        if (flags.zf || flags.sf!=flags.of) {
            finish(no_sections,Resume::validate,0xd83bau);
            return;
        }
        gp[eax] = signed_word(0xd83c0u,gp[ebp]+0x120u);
        gp[ecx] = signed_word(0xd83c7u,gp[ebp]+0x122u);
        gp[eax] = flags.right(gp[eax],3u,4,false);
        gp[ecx] = flags.plus(gp[ecx],gp[ebp],4);
        gp[ebx] = flags.logic(0u,4);
        this->test32(gp[eax]);
        this->store(0xd83d7u,gp[esp]+0x24u,4,gp[eax]);
        if (flags.zf || flags.sf!=flags.of) {
            finish(no_sections,Resume::validate,0xd83dbu);
            return;
        }
        gp[ecx] = flags.plus(gp[ecx],4u,4);
        this->store(0xd83e4u,gp[esp]+0x1cu,4,gp[ecx]);
        finish(pending_response,Resume::response,0xd83e8u);
    }

    void run() {
        try {
            load_object();
            update_immunity_timer();
            select_sections();
        } catch (...) {
            this->s.gp = this->gp;
            this->s.flags = this->flags;
            throw;
        }
    }
};

template<class State, class Reference, class Route, class Select>
Resume bridge(State &state, Reference reference, Route describe, Select select) {
    if (state.image != SourcePhysicsContactImage::TitleUpdate15 || !state.selected_observation_pcs)
        return Resume::retained;
    static const bool reference_mode = flag("HALO2_REFERENCE_DAMAGE_UPDATE_ENTRY");
    static const bool verify_mode = flag("HALO2_VERIFY_DAMAGE_UPDATE_ENTRY");
    static VerifySummary summary{"Damage update entry"};
    auto next = Resume::retained;
    const auto verify_reference = [&](State &input) {
        input.selected_observation_pcs = true;
        if (reference(input) != next) throw std::runtime_error("Damage update continuation differed");
    };
    run_owner(state,0xd8330u,0xd83e8u,reference_mode,verify_mode,coverage(),summary,verify_reference,
        [&](State &value,std::uint32_t *cursor,Trace<State> *trace) {
            return Frame<State,Route,Select>{value,cursor,trace,describe,select,next};
        });
    return next;
}
} // namespace halo2::native_damage_update_entry_detail
