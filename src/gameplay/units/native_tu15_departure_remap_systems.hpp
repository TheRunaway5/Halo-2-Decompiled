#pragma once
// Native TU1.5 departure remap, entered at 19AE70 (player departure unit):
// reconcile the multiplayer departure roster, per-player stat blocks, 16x16
// interaction matrix and 1000 event records against the live player table
// ([4F54F4]), then refresh derived departure state via 19DD10(51DFB0).
//
//   departure_remap::run   19AE70() -> void; RET 0
//
// Frame is 8-byte aligned (AND ESP,-8) with 0x4568 bytes probed via 32A660.
// Returns early if departure tracking ([51DFA0]) is inactive or locked
// ([51DFB1]). Otherwise initializes old->new (ESP+0x58) and new->old
// (ESP+0x18) 16-entry index maps to -1, matches active players (salt != 0)
// against the 16 roster slots at 0x51E334 (0xA4 bytes each, 12-byte identity
// at +2 via REPE CMPSD), snapshots and rebuilds the roster (ESP+0x498),
// snapshots and remaps the 16 stat blocks at 0x51EEF4 (0x36A bytes each via
// ESP+0xED8), snapshots and remaps the 16x16 matrix at 0x522594 (ESP+0x98),
// remaps the two signed-byte player indices in each of the 1000 event records
// at 0x522F35 (stride 0x24), and calls 19DD10(0x51DFB0).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access); the boundary adapter
// (native_tu15_departure_remap.hpp) maps the role to the original instruction,
// owns the ABI of 32A660 and 19DD10, and preserves every REP string step in
// the direction flag's direction.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_departure_remap {
using native_object_update::at;
namespace globals {
inline constexpr std::uint32_t players=0x4f54f4u,active=0x51dfa0u,state_root=0x51dfb0u,locked=0x51dfb1u,
    roster=0x51e334u,roster_id_start=0x51e336u,roster_id_end=0x51ed76u,roster_body_start=0x51e344u,roster_body_end=0x51ed84u,
    stats_start=0x51eef4u,stats_end=0x522594u,matrix_start=0x522594u,matrix_end=0x522994u,events_start=0x522f35u;
}
inline constexpr std::uint32_t frame_size=0x4568u,stack_align_mask=0xfffffff8u,none=0xffffffffu,slot_count=16u,
    datum_stride=0x24u,datum_limit=0x38u,datum_data=0x44u,
    roster_stride=0xa4u,roster_words=0x290u,roster_entry_words=0x29u,profile_offset=0x44u,profile_words=0x24u,
    stats_stride=0x36au,stats_words=0xda8u,stats_entry_words=0xdau,
    matrix_words=0x100u,event_count=0x3e8u,event_stride=0x24u;
namespace local {
inline constexpr std::uint32_t cursor=0x14u,new_to_old=0x18u,old_to_new=0x58u,
    saved_matrix=0x98u,saved_roster=0x498u,saved_stats=0xed8u;
}

namespace departure_remap {
enum class Access {
    active_load,locked_load,old_map_init,new_map_init,players_load,old_map_last_init,cursor_init,
    match_limit,match_stride,match_data,match_salt,roster_used,players_reload,cmp_player,cmp_roster,cursor_reload,
    old_map_check,old_map_store,new_map_store,cursor_store,
    snapshot_roster_read,snapshot_roster_write,offset_init,offset_check,
    rebuild_limit,rebuild_stride,rebuild_data,rebuild_salt,new_map_load,
    roster_mark_used,id_read_0,id_write_0,id_read_1,id_write_1,id_read_2,id_write_2,
    profile_read,profile_write,roster_tail_1,roster_tail_2,
    copy_roster_read,copy_roster_write,offset_reload,offset_store,
    snapshot_stats_read,snapshot_stats_write,stat_old_idx,
    zero_stat_dword,zero_stat_word,copy_stat_read,copy_stat_write,copy_stat_read_word,copy_stat_write_word,
    snapshot_matrix_read,snapshot_matrix_write,matrix_row_old,matrix_col_old,matrix_saved_load,matrix_copy_store,matrix_zero_store,
    event_index_load,event_new_load,event_new_store,event_clear_store};

// Frame: see native_tu15_departure_remap.hpp.
template<class Memory,class Frame> void run(Memory &memory,Frame &frame) {
    using enum Access;
    frame.prologue();
    const bool is_active=frame.test_active(memory.load8(active_load,globals::active));
    frame.save_registers();
    if(!is_active)return;
    if(frame.test_locked(memory.load8(locked_load,globals::locked)))return;

    // Initialize old->new (0..14), new->old (0..5), load players, new->old (6..15), old->new (15), cursor=0.
    frame.prepare_none();
    for(std::uint32_t i=0;i<15u;++i) {
        frame.set_init_index(i);
        memory.store32(old_map_init,frame.local(local::old_to_new+4u*i),none);
    }
    frame.prepare_new_none();
    for(std::uint32_t i=0;i<6u;++i) {
        frame.set_init_index(i);
        memory.store32(new_map_init,frame.local(local::new_to_old+4u*i),none);
    }
    std::uint32_t players=memory.load32(players_load,globals::players);
    for(std::uint32_t i=6u;i<slot_count;++i) {
        frame.set_init_index(i);
        memory.store32(new_map_init,frame.local(local::new_to_old+4u*i),none);
    }
    frame.clear_cursor();
    memory.store32(old_map_last_init,frame.local(local::old_to_new+4u*15u),none);
    memory.store32(cursor_init,frame.local(local::cursor),0u);

    // Phase 1: Match active players against the existing departure roster.
    std::uint32_t slot=0u;
    for(;;) {
        if(frame.valid_match_slot(slot)) {
            const auto limit=memory.load32(match_limit,at(players,datum_limit));
            if(frame.below_limit_match(slot,limit)) {
                const auto stride=memory.load32(match_stride,at(players,datum_stride));
                const auto data=memory.load32(match_data,at(players,datum_data));
                const auto entry=frame.player_address_match(stride,slot,data);
                const auto salt=memory.load16(match_salt,entry);
                if(frame.nonzero_salt_match(salt)) {
                    std::uint32_t old_idx=frame.start_roster_scan();
                    std::uint32_t id_ptr=globals::roster_id_start;
                    bool found=false;
                    for(;;) {
                        const auto used=memory.load8(roster_used,id_ptr-2u);
                        if(frame.roster_entry_used(used)) {
                            frame.clear_players_before_reload();
                            players=memory.load32(players_reload,globals::players);
                            frame.start_identity_compare(entry+4u,id_ptr);
                            const auto step=frame.step4();
                            std::uint32_t p=entry+4u,q=id_ptr;
                            bool eq=true;
                            for(std::uint32_t w=0;w<3u;++w,p+=step,q+=step) {
                                const auto a=memory.load32(cmp_player,p);
                                const auto b=memory.load32(cmp_roster,q);
                                eq=frame.compared_identity_word(a,b);
                                if(!eq)break;
                            }
                            slot=memory.load32(cursor_reload,frame.local(local::cursor));
                            if(eq){found=true;break;}
                        }
                        id_ptr=frame.next_roster_scan(id_ptr,old_idx);
                        if(!frame.more_roster_scan(id_ptr))break;
                    }
                    if(found) {
                        const auto existing=memory.load32(old_map_check,frame.local(local::old_to_new)+4u*old_idx);
                        if(frame.slot_unclaimed(existing)) {
                            memory.store32(old_map_store,frame.local(local::old_to_new)+4u*old_idx,slot);
                            memory.store32(new_map_store,frame.local(local::new_to_old)+4u*slot,old_idx);
                        }
                    } else {
                        frame.note_unmatched();
                    }
                }
            }
        }
        slot=frame.advance_match_slot(slot);
        memory.store32(cursor_store,frame.local(local::cursor),slot);
        if(!frame.more_match_slots(slot))break;
    }

    // Phase 2: Snapshot roster and rebuild entries for active players.
    auto copy_dwords=[&](Access rd,Access wr,std::uint32_t from,std::uint32_t to,std::uint32_t count) {
        const auto step=frame.step4();
        for(std::uint32_t k=0;k<count;++k,from+=step,to+=step)
            memory.store32(wr,to,memory.load32(rd,from));
    };
    const auto saved_roster_base=frame.local(local::saved_roster);
    frame.start_roster_snapshot(saved_roster_base);
    copy_dwords(snapshot_roster_read,snapshot_roster_write,globals::roster,saved_roster_base,roster_words);
    std::uint32_t body_ptr=frame.start_roster_rebuild();
    memory.store32(offset_init,frame.local(local::cursor),0u);
    std::uint32_t rebuild_idx=0u;
    for(;;) {
        const auto off_check=memory.load32(offset_check,frame.local(local::cursor));
        if(frame.valid_rebuild_offset(off_check,body_ptr)) {
            const auto limit=memory.load32(rebuild_limit,at(players,datum_limit));
            if(frame.below_limit_rebuild(rebuild_idx,limit)) {
                const auto stride=memory.load32(rebuild_stride,at(players,datum_stride));
                const auto data=memory.load32(rebuild_data,at(players,datum_data));
                const auto entry=frame.player_address_rebuild(stride,rebuild_idx,data);
                const auto salt=memory.load16(rebuild_salt,entry);
                if(frame.nonzero_salt_rebuild(salt)) {
                    const auto old_idx=memory.load32(new_map_load,frame.local(local::new_to_old)+4u*rebuild_idx);
                    if(frame.is_new_player(old_idx)) {
                        const auto id_src=frame.prepare_new_roster_entry(entry);
                        memory.store8(roster_mark_used,body_ptr-0x10u,1u);
                        const auto w0=memory.load32(id_read_0,id_src);
                        const auto id_dst=frame.roster_id_dest(body_ptr);
                        memory.store32(id_write_0,id_dst,w0);
                        const auto w1=memory.load32(id_read_1,at(id_src,4u));
                        memory.store32(id_write_1,at(id_dst,4u),w1);
                        const auto w2=memory.load32(id_read_2,at(id_src,8u));
                        memory.store32(id_write_2,at(id_dst,8u),w2);
                        frame.start_profile_copy(at(entry,profile_offset),body_ptr);
                        copy_dwords(profile_read,profile_write,at(entry,profile_offset),body_ptr,profile_words);
                        memory.store8(roster_tail_1,at(body_ptr,0x90u),0xffu);
                        memory.store8(roster_tail_2,body_ptr-0xfu,0xffu);
                    } else {
                        const auto saved_src=frame.start_kept_roster_copy(old_idx,body_ptr);
                        copy_dwords(copy_roster_read,copy_roster_write,saved_src,body_ptr-0x10u,roster_entry_words);
                    }
                }
            }
        }
        const auto cur_off=memory.load32(offset_reload,frame.local(local::cursor));
        const auto next_off=frame.advance_rebuild(cur_off,body_ptr,rebuild_idx);
        memory.store32(offset_store,frame.local(local::cursor),next_off);
        if(!frame.more_rebuild(body_ptr))break;
    }

    // Phase 3: Snapshot and remap the 16 stat blocks (0x36A bytes each).
    const auto saved_stats_base=frame.local(local::saved_stats);
    frame.start_stats_snapshot(saved_stats_base);
    copy_dwords(snapshot_stats_read,snapshot_stats_write,globals::stats_start,saved_stats_base,stats_words);
    std::uint32_t map_ptr=frame.start_stats_remap();
    std::uint32_t stat_dst=globals::stats_start;
    for(;;) {
        const auto old_idx=memory.load32(stat_old_idx,map_ptr);
        if(frame.stat_entry_absent(old_idx,stat_dst)) {
            frame.prepare_zero_stat();
            const auto step4=frame.step4();
            std::uint32_t dst=stat_dst;
            for(std::uint32_t k=0;k<stats_entry_words;++k,dst+=step4)
                memory.store32(zero_stat_dword,dst,0u);
            memory.store16(zero_stat_word,dst,0u);
        } else {
            const auto src_base=frame.prepare_copy_stat(old_idx);
            const auto step4=frame.step4();
            std::uint32_t src=src_base,dst=stat_dst;
            for(std::uint32_t k=0;k<stats_entry_words;++k,src+=step4,dst+=step4)
                memory.store32(copy_stat_write,dst,memory.load32(copy_stat_read,src));
            memory.store16(copy_stat_write_word,dst,memory.load16(copy_stat_read_word,src));
        }
        frame.advance_stats_remap(stat_dst,map_ptr);
        if(!frame.more_stats_remap(stat_dst))break;
    }

    // Phase 4: Snapshot and remap the 16x16 interaction matrix.
    const auto saved_matrix_base=frame.local(local::saved_matrix);
    frame.start_matrix_snapshot(saved_matrix_base);
    copy_dwords(snapshot_matrix_read,snapshot_matrix_write,globals::matrix_start,saved_matrix_base,matrix_words);
    std::uint32_t cell_ptr=globals::matrix_start;
    std::uint32_t row_map_ptr=frame.start_matrix_remap();
    for(;;) {
        const auto row_old=memory.load32(matrix_row_old,row_map_ptr);
        std::uint32_t col=frame.start_matrix_row();
        for(;;) {
            const auto col_old=memory.load32(matrix_col_old,frame.local(local::new_to_old)+4u*col);
            if(frame.matrix_cell_valid(row_old,col_old)) {
                const auto cell_idx=frame.matrix_saved_index(row_old,col_old);
                const auto val=memory.load32(matrix_saved_load,frame.local(local::saved_matrix)+4u*cell_idx);
                memory.store32(matrix_copy_store,cell_ptr,val);
            } else {
                memory.store32(matrix_zero_store,cell_ptr,0u);
            }
            frame.advance_matrix_col(col,cell_ptr);
            if(!frame.more_matrix_cols(col))break;
        }
        frame.advance_matrix_row(row_map_ptr,cell_ptr);
        if(!frame.more_matrix_rows(cell_ptr))break;
    }

    // Phase 5: Remap the two signed-byte player indices in all 1000 event records.
    std::uint32_t ev_ptr=frame.start_events();
    std::uint32_t ev_rem=event_count;
    for(;;) {
        std::uint32_t field_ptr=frame.start_event_record(ev_ptr);
        std::uint32_t fields_rem=2u;
        for(;;) {
            const auto raw=memory.load8(event_index_load,field_ptr);
            const auto idx=frame.event_index_signed(raw);
            if(frame.event_index_valid(idx)) {
                const auto mapped=memory.load8(event_new_load,frame.local(local::old_to_new)+4u*idx);
                memory.store8(event_new_store,field_ptr,mapped);
            } else {
                memory.store8(event_clear_store,field_ptr,0xffu);
            }
            if(!frame.advance_event_field(field_ptr,fields_rem))break;
        }
        if(!frame.advance_event_record(ev_ptr,ev_rem))break;
    }

    frame.finalize();
}
} // namespace departure_remap
} // namespace halo2::native_departure_remap
