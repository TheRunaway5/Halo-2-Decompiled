#pragma once
// Boundary adapter for departure remap, 19AE70
// (native_tu15_departure_remap_systems.hpp). Its callers and the two direct
// children (stack probe 32A660 and 19DD10 with RET 4) keep the original ABI,
// which the adapter owns: 8-byte stack alignment (PUSH EBP; MOV EBP,ESP; AND
// ESP,-8; MOV EAX,4568; CALL 32A660), EBX, EBP, ESI and EDI saved below the
// frame, every REP CMPSD / REP MOVSD / REP STOSD / MOVSW / STOSW stepping
// ESI, EDI and ECX in the direction flag's direction, and epilogue restoring
// EDI, ESI, EBP, EBX, MOV ESP,EBP, POP EBP, RET 0. Each access publishes its
// original instruction.
// HALO2_REFERENCE_DEPARTURE_REMAP=1 restores the translated body.
// HALO2_VERIFY_DEPARTURE_REMAP=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/units/native_tu15_departure_remap_systems.hpp"

namespace halo2::native_departure_remap_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_departure_remap;
using Access=logic::departure_remap::Access;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_DEPARTURE_REMAP");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_DEPARTURE_REMAP");return v;}
enum Branch : unsigned {
    inactive, locked, match_past_limit, match_empty_player, roster_unused, matched_player, match_duplicate, match_not_found,
    rebuild_past_limit, rebuild_empty_player, new_player, kept_player,
    zero_stats, copy_stats, matrix_copy, matrix_zero, event_remapped, event_cleared, branch_count};
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Departure remap"};return v;}
inline std::uint32_t sx8(std::uint32_t v){return std::uint32_t(std::int32_t(std::int8_t(v)));}
inline constexpr std::uint32_t old_init_pcs[15]{
    0x19aea3u,0x19aea7u,0x19aeabu,0x19aeafu,0x19aeb3u,0x19aeb7u,0x19aebbu,0x19aebfu,
    0x19aec3u,0x19aec7u,0x19aecbu,0x19aed2u,0x19aed9u,0x19aee0u,0x19aee7u};
inline constexpr std::uint32_t new_init_pcs[16]{
    0x19aef0u,0x19aef4u,0x19aef8u,0x19aefcu,0x19af00u,0x19af04u,0x19af0eu,0x19af12u,
    0x19af16u,0x19af1au,0x19af1eu,0x19af22u,0x19af26u,0x19af2au,0x19af2eu,0x19af32u};
inline constexpr std::uint32_t sites[]{
    0x19ae80u,0x19ae91u,0u,0u,0x19af08u,0x19af38u,0x19af3fu,
    0x19af4cu,0x19af51u,0x19af54u,0x19af5cu,0x19af70u,0x19af78u,0x19af88u,0x19af88u,0x19af8au,
    0x19af9fu,0x19afa6u,0x19afaau,0x19afb2u,
    0x19afcbu,0x19afcbu,0x19afd2u,0x19afd6u,
    0x19afe7u,0x19afecu,0x19afefu,0x19aff7u,0x19affdu,
    0x19b009u,0x19b00du,0x19b012u,0x19b014u,0x19b017u,0x19b01au,0x19b01du,
    0x19b02au,0x19b02au,0x19b02cu,0x19b033u,
    0x19b04eu,0x19b04eu,0x19b050u,0x19b065u,
    0x19b080u,0x19b080u,0x19b090u,
    0x19b0a0u,0x19b0a2u,0x19b0b3u,0x19b0b3u,0x19b0b5u,0x19b0b5u,
    0x19b0d9u,0x19b0d9u,0x19b0e4u,0x19b0f0u,0x19b105u,0x19b10cu,0x19b110u,
    0x19b140u,0x19b151u,0x19b155u,0x19b159u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::event_clear_store)+1u);
template<class State,class Probe,class Finalize> struct Adapter : Port<State> {
    Probe &probe_child;          // 32A660 (direct)
    Finalize &finalize_child;    // 19DD10 (direct, RET 4)
    std::uint32_t init_index{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Probe &p,Finalize &f):Port<State>(s,c,t,{}),probe_child(p),finalize_child(f){}
    std::uint32_t local(std::uint32_t offset)const{return this->gp[esp]+offset;}
    std::uint32_t step4()const{return (this->s.other_flags&0x400u)?0u-4u:4u;}
    std::uint32_t step2()const{return (this->s.other_flags&0x400u)?0u-2u:2u;}
    void set_init_index(std::uint32_t i){init_index=i;}
    std::uint32_t instruction(Access a,std::uint32_t) {
        if(a==Access::old_map_init)return old_init_pcs[init_index];
        if(a==Access::new_map_init)return new_init_pcs[init_index];
        return sites[unsigned(a)];
    }
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case active_load:case locked_load:case event_new_load:set_low_byte(gp[eax],v);break;
        case players_load:case players_reload:gp[ebx]=v;break;
        case match_stride:case new_map_load:case id_read_2:case matrix_col_old:case matrix_saved_load:gp[edx]=v;break;
        case match_data:case rebuild_data:case offset_reload:case matrix_row_old:gp[esi]=v;break;
        case cursor_reload:case rebuild_stride:gp[ecx]=v;break;
        case id_read_0:case id_read_1:gp[edi]=v;break;
        case stat_old_idx:gp[eax]=v;break;
        case event_index_load:gp[eax]=sx8(v);break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case snapshot_roster_write:case profile_write:case copy_roster_write:
        case snapshot_stats_write:case copy_stat_write:case snapshot_matrix_write: {
            const auto d=step4();gp[esi]+=d;gp[edi]+=d;--gp[ecx];break;
        }
        case zero_stat_dword: {
            gp[edi]+=step4();--gp[ecx];break;
        }
        case zero_stat_word: {
            gp[edi]+=step2();break;
        }
        case copy_stat_write_word: {
            const auto d=step2();gp[esi]+=d;gp[edi]+=d;break;
        }
        default:break;
        }
    }
    // Prologue: PUSH EBP; MOV EBP,ESP; AND ESP,-8; MOV EAX,4568; CALL 32A660.
    void prologue() {
        auto &gp=this->gp;
        this->push(0x19ae70u,gp[ebp]);
        gp[ebp]=gp[esp];
        gp[esp]=this->flags.logic(gp[esp]&logic::stack_align_mask,4);
        gp[eax]=logic::frame_size;
        this->call(0x19ae7bu,0x19ae80u,0x32a660u,probe_child);
    }
    bool test_active(std::uint32_t v) {
        this->test8(v);
        if(this->flags.zf){this->reached(Branch::inactive);return false;}
        return true;
    }
    void save_registers() {
        auto &gp=this->gp;
        this->push(0x19ae87u,gp[ebx]);
        this->push(0x19ae88u,gp[ebp]);
        this->push(0x19ae89u,gp[esi]);
        this->push(0x19ae8au,gp[edi]);
    }
    bool test_locked(std::uint32_t v) {
        this->test8(v);
        if(!this->flags.zf){this->reached(Branch::locked);return true;}
        return false;
    }
    void prepare_none(){this->gp[eax]=logic::none;}
    void prepare_new_none(){this->gp[ecx]=this->flags.logic(this->gp[ecx]|logic::none,4);}
    void clear_cursor(){this->gp[ecx]=this->flags.logic(0u,4);}
    // Phase 1: Match active players.
    bool valid_match_slot(std::uint32_t slot) {
        this->compare32(slot,logic::none);
        if(this->flags.zf)return false;
        this->test32(slot);
        return std::int32_t(slot)>=0;
    }
    bool below_limit_match(std::uint32_t slot,std::uint32_t limit) {
        this->compare32(slot,limit);
        if(std::int32_t(slot)>=std::int32_t(limit)){this->reached(Branch::match_past_limit);return false;}
        return true;
    }
    std::uint32_t player_address_match(std::uint32_t stride,std::uint32_t slot,std::uint32_t data) {
        auto &gp=this->gp;
        gp[edx]=this->flags.product(stride,slot);
        return gp[edx]=this->flags.plus(gp[edx],data,4);
    }
    bool nonzero_salt_match(std::uint32_t salt) {
        this->flags.minus(salt&0xffffu,0u,2);
        if(this->flags.zf){this->reached(Branch::match_empty_player);return false;}
        return true;
    }
    std::uint32_t start_roster_scan() {
        auto &gp=this->gp;
        gp[ebp]=this->flags.logic(0u,4);
        gp[eax]=logic::globals::roster_id_start;
        return 0u;
    }
    bool roster_entry_used(std::uint32_t used) {
        this->flags.minus(used&0xffu,0u,1);
        if(this->flags.zf){this->reached(Branch::roster_unused);return false;}
        return true;
    }
    void clear_players_before_reload(){this->gp[ebx]=this->flags.logic(0u,4);}
    void start_identity_compare(std::uint32_t player_id,std::uint32_t roster_id) {
        auto &gp=this->gp;
        gp[ecx]=3u;gp[edi]=roster_id;gp[esi]=player_id;
    }
    bool compared_identity_word(std::uint32_t a,std::uint32_t b) {
        auto &gp=this->gp;
        this->compare32(a,b);
        const auto d=step4();gp[esi]+=d;gp[edi]+=d;--gp[ecx];
        return this->flags.zf;
    }
    std::uint32_t next_roster_scan(std::uint32_t id_ptr,std::uint32_t &old_idx) {
        auto &gp=this->gp;
        gp[eax]=this->flags.plus(id_ptr,logic::roster_stride,4);
        old_idx=gp[ebp]=this->flags.increment(old_idx,4,false);
        return gp[eax];
    }
    bool more_roster_scan(std::uint32_t id_ptr) {
        this->compare32(id_ptr,logic::globals::roster_id_end);
        return std::int32_t(id_ptr)<std::int32_t(logic::globals::roster_id_end);
    }
    bool slot_unclaimed(std::uint32_t existing) {
        this->compare32(existing,logic::none);
        if(!this->flags.zf){this->reached(Branch::match_duplicate);return false;}
        this->reached(Branch::matched_player);
        return true;
    }
    void note_unmatched(){this->reached(Branch::match_not_found);}
    std::uint32_t advance_match_slot(std::uint32_t slot) {
        auto &gp=this->gp;
        gp[ecx]=this->flags.increment(slot,4,false);
        this->compare32(gp[ecx],logic::slot_count);
        return gp[ecx];
    }
    bool more_match_slots(std::uint32_t slot)const{return std::int32_t(slot)<std::int32_t(logic::slot_count);}
    // Phase 2: Snapshot & rebuild roster.
    void start_roster_snapshot(std::uint32_t dst) {
        auto &gp=this->gp;
        gp[ecx]=logic::roster_words;gp[esi]=logic::globals::roster;gp[edi]=dst;
        gp[ebp]=this->flags.logic(0u,4);
    }
    std::uint32_t start_roster_rebuild(){return this->gp[eax]=logic::globals::roster_body_start;}
    bool valid_rebuild_offset(std::uint32_t off,std::uint32_t body_ptr) {
        this->compare32(off,0xffffff5cu);
        if(this->flags.zf)return false;
        this->compare32(body_ptr,logic::globals::roster_body_start);
        return std::int32_t(body_ptr)>=std::int32_t(logic::globals::roster_body_start);
    }
    bool below_limit_rebuild(std::uint32_t idx,std::uint32_t limit) {
        this->compare32(idx,limit);
        if(std::int32_t(idx)>=std::int32_t(limit)){this->reached(Branch::rebuild_past_limit);return false;}
        return true;
    }
    std::uint32_t player_address_rebuild(std::uint32_t stride,std::uint32_t idx,std::uint32_t data) {
        auto &gp=this->gp;
        gp[ecx]=this->flags.product(stride,idx);
        return gp[ecx]=this->flags.plus(gp[ecx],data,4);
    }
    bool nonzero_salt_rebuild(std::uint32_t salt) {
        this->flags.minus(salt&0xffffu,0u,2);
        if(this->flags.zf){this->reached(Branch::rebuild_empty_player);return false;}
        return true;
    }
    bool is_new_player(std::uint32_t old_idx) {
        this->compare32(old_idx,logic::none);
        if(this->flags.zf){this->reached(Branch::new_player);return true;}
        this->reached(Branch::kept_player);
        return false;
    }
    std::uint32_t prepare_new_roster_entry(std::uint32_t entry){return this->gp[edx]=entry+4u;}
    std::uint32_t roster_id_dest(std::uint32_t body_ptr){return this->gp[esi]=body_ptr-0xeu;}
    void start_profile_copy(std::uint32_t src,std::uint32_t dst) {
        auto &gp=this->gp;gp[esi]=src;gp[ecx]=logic::profile_words;gp[edi]=dst;
    }
    std::uint32_t start_kept_roster_copy(std::uint32_t old_idx,std::uint32_t body_ptr) {
        auto &gp=this->gp;
        gp[edx]=this->flags.product(old_idx,logic::roster_stride);
        gp[esi]=local(logic::local::saved_roster)+gp[edx];
        gp[edi]=body_ptr-0x10u;
        gp[ecx]=logic::roster_entry_words;
        return gp[esi];
    }
    std::uint32_t advance_rebuild(std::uint32_t cur_off,std::uint32_t &body_ptr,std::uint32_t &idx) {
        auto &gp=this->gp;
        gp[esi]=this->flags.plus(cur_off,logic::roster_stride,4);
        body_ptr=gp[eax]=this->flags.plus(body_ptr,logic::roster_stride,4);
        idx=gp[ebp]=this->flags.increment(idx,4,false);
        this->compare32(body_ptr,logic::globals::roster_body_end);
        return gp[esi];
    }
    bool more_rebuild(std::uint32_t body_ptr)const{return std::int32_t(body_ptr)<std::int32_t(logic::globals::roster_body_end);}
    // Phase 3: Snapshot & remap stats.
    void start_stats_snapshot(std::uint32_t dst) {
        auto &gp=this->gp;gp[ecx]=logic::stats_words;gp[esi]=logic::globals::stats_start;gp[edi]=dst;
    }
    std::uint32_t start_stats_remap() {
        auto &gp=this->gp;gp[ebx]=local(logic::local::new_to_old);gp[edx]=logic::globals::stats_start;
        return gp[ebx];
    }
    bool stat_entry_absent(std::uint32_t old_idx,std::uint32_t stat_dst) {
        auto &gp=this->gp;
        this->compare32(old_idx,logic::none);
        gp[edi]=stat_dst;gp[ecx]=logic::stats_entry_words;
        if(this->flags.zf){this->reached(Branch::zero_stats);return true;}
        this->reached(Branch::copy_stats);
        return false;
    }
    void prepare_zero_stat(){this->gp[eax]=this->flags.logic(0u,4);}
    std::uint32_t prepare_copy_stat(std::uint32_t old_idx) {
        auto &gp=this->gp;
        gp[eax]=this->flags.product(old_idx,logic::stats_stride);
        return gp[esi]=local(logic::local::saved_stats)+gp[eax];
    }
    void advance_stats_remap(std::uint32_t &stat_dst,std::uint32_t &map_ptr) {
        auto &gp=this->gp;
        stat_dst=gp[edx]=this->flags.plus(stat_dst,logic::stats_stride,4);
        map_ptr=gp[ebx]=this->flags.plus(map_ptr,4u,4);
        this->compare32(stat_dst,logic::globals::stats_end);
    }
    bool more_stats_remap(std::uint32_t stat_dst)const{return std::int32_t(stat_dst)<std::int32_t(logic::globals::stats_end);}
    // Phase 4: Snapshot & remap 16x16 interaction matrix.
    void start_matrix_snapshot(std::uint32_t dst) {
        auto &gp=this->gp;gp[ecx]=logic::matrix_words;gp[esi]=logic::globals::matrix_start;gp[edi]=dst;
    }
    std::uint32_t start_matrix_remap() {
        auto &gp=this->gp;gp[eax]=logic::globals::matrix_start;return gp[edi]=local(logic::local::new_to_old);
    }
    std::uint32_t start_matrix_row(){return this->gp[ecx]=this->flags.logic(0u,4);}
    bool matrix_cell_valid(std::uint32_t row_old,std::uint32_t col_old) {
        this->compare32(col_old,logic::none);
        if(this->flags.zf){this->reached(Branch::matrix_zero);return false;}
        this->compare32(row_old,logic::none);
        if(this->flags.zf){this->reached(Branch::matrix_zero);return false;}
        this->reached(Branch::matrix_copy);
        return true;
    }
    std::uint32_t matrix_saved_index(std::uint32_t row_old,std::uint32_t col_old) {
        auto &gp=this->gp;
        gp[ebx]=this->flags.shift(row_old,4u,4);
        return gp[ebx]=this->flags.plus(gp[ebx],col_old,4);
    }
    void advance_matrix_col(std::uint32_t &col,std::uint32_t &cell_ptr) {
        auto &gp=this->gp;
        col=gp[ecx]=this->flags.increment(col,4,false);
        cell_ptr=gp[eax]=this->flags.plus(cell_ptr,4u,4);
        this->compare32(col,logic::slot_count);
    }
    bool more_matrix_cols(std::uint32_t col)const{return std::int32_t(col)<std::int32_t(logic::slot_count);}
    void advance_matrix_row(std::uint32_t &row_map_ptr,std::uint32_t cell_ptr) {
        auto &gp=this->gp;
        row_map_ptr=gp[edi]=this->flags.plus(row_map_ptr,4u,4);
        this->compare32(cell_ptr,logic::globals::matrix_end);
    }
    bool more_matrix_rows(std::uint32_t cell_ptr)const{return std::int32_t(cell_ptr)<std::int32_t(logic::globals::matrix_end);}
    // Phase 5: Remap 1000 event records.
    std::uint32_t start_events() {
        auto &gp=this->gp;gp[esi]=logic::globals::events_start;gp[edi]=logic::event_count;return gp[esi];
    }
    std::uint32_t start_event_record(std::uint32_t ev_ptr) {
        auto &gp=this->gp;gp[ecx]=ev_ptr;gp[edx]=2u;return gp[ecx];
    }
    std::uint32_t event_index_signed(std::uint32_t)const{return this->gp[eax];}
    bool event_index_valid(std::uint32_t idx) {
        this->compare32(idx,logic::none);
        if(this->flags.zf){this->reached(Branch::event_cleared);return false;}
        this->test32(idx);
        if(std::int32_t(idx)<0){this->reached(Branch::event_cleared);return false;}
        this->compare32(idx,logic::slot_count);
        if(std::int32_t(idx)>=std::int32_t(logic::slot_count)){this->reached(Branch::event_cleared);return false;}
        this->reached(Branch::event_remapped);
        return true;
    }
    bool advance_event_field(std::uint32_t &field_ptr,std::uint32_t &fields_rem) {
        auto &gp=this->gp;
        field_ptr=gp[ecx]=this->flags.increment(field_ptr,4,false);
        fields_rem=gp[edx]=this->flags.increment(fields_rem,4,true);
        return !this->flags.zf;
    }
    bool advance_event_record(std::uint32_t &ev_ptr,std::uint32_t &ev_rem) {
        auto &gp=this->gp;
        ev_ptr=gp[esi]=this->flags.plus(ev_ptr,logic::event_stride,4);
        ev_rem=gp[edi]=this->flags.increment(ev_rem,4,true);
        return !this->flags.zf;
    }
    void finalize() {
        this->push(0x19b166u,logic::globals::state_root);
        this->call(0x19b16bu,0x19b170u,0x19dd10u,finalize_child);
    }
    void finish() {
        auto &gp=this->gp;
        gp[edi]=this->pop(0x19b170u);
        gp[esi]=this->pop(0x19b171u);
        gp[ebp]=this->pop(0x19b172u);
        gp[ebx]=this->pop(0x19b173u);
        gp[esp]=gp[ebp];
        gp[ebp]=this->pop(0x19b176u);
        this->ret(0x19b177u,0u);
        this->publish(0x19b177u);
    }
    void run() {
        Memory<Adapter> memory{*this};
        logic::departure_remap::run(memory,*this);
        finish();
    }
};
template<class State,class Reference,class Probe,class Finalize>
bool bridge(State &s,Reference reference,Probe probe_child,Finalize finalize_child) {
    return run_owner(s,0x19ae70u,0x19b177u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Probe,Finalize>(v,c,t,probe_child,finalize_child);});
}
} // namespace halo2::native_departure_remap_detail
