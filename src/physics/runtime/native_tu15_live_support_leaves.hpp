#pragma once
// Boundary adapters for the group L live-support systems
// (native_tu15_live_support_systems.hpp). Their callers are still translated
// TU1.5 code, so each adapter owns that caller's ABI and nothing else:
//   - arguments from registers and stack slots, results back into the
//     original's registers (including partial-register writes and the scratch
//     values the original leaves behind);
//   - the final EFLAGS, from the instruction that decides them in the original;
//   - the original's stack frame: register saves and restores, and the frame
//     slots whose writes are part of the original's store order;
//   - the diagnostic cursor: each memory access publishes its original
//     instruction (a per-system table of access role -> instruction), and the
//     RET publishes the exit.
// Delete an adapter when its callers go native.
//
// The leaf framework (native_tu15_matrix_leaves.hpp) runs the adapters:
// admission (TU1.5, literal cursor or none, no trace, census switch),
// HALO2_REFERENCE_MATRIX_LEAVES=1 (translated bodies) and
// HALO2_VERIFY_MATRIX_LEAVES=1 (each call checked against them). Bound by
// tools/native_live_support_leaves_replacements.py.
#include "physics/runtime/native_tu15_matrix_leaves.hpp"
#include "physics/runtime/native_tu15_live_support_systems.hpp"
#include "physics/runtime/native_tu15_portal_predicate_leaves.hpp"
#include "physics/runtime/native_tu15_leaf_children.hpp"

namespace halo2::native_tu15_leaves {
// A unit whose store events report its canonical (disc-build) PCs while its
// cursor follows TU1.5 (the physics pool startup unit's copies of 16E450,
// 16E4B0, 16E600 and 32A4E0): the kernel's stores are reported at
// site_of(TU1.5 store site), the binder's map of the unit's own sites (an
// unmapped site refuses the call). Admission and verify mode are run()'s: the
// overlay comparison holds no store PCs.
template<class State,class Sites> struct StoreSiteIo : RealIo<State> {
    Sites site_of;
    void st(std::uint32_t p,std::uint32_t a,unsigned n,std::uint64_t x) {
        this->at(p);physics_step_detail::w(this->s.memory,this->s.observe,site_of(p),a,n,x);
    }
};
template<class State,class Kernel,class Reference,class Sites>
bool run_with_store_sites(State &s,Kernel kernel,Reference reference,Sites site_of) {
    if(verify_mode())return run(s,kernel,reference,0u);
    std::uint32_t *pc=nullptr;
    if(!admitted(s,pc))return false;
    using Flags=std::remove_cvref_t<decltype(s.flags)>;
    Machine<Flags> m{s.gp,{},s.flags};
    if constexpr(requires{s.other_flags;})m.other_flags=s.other_flags;
    for(unsigned i=0;i<8;++i)m.x[i]=s.xmm[i];
    if constexpr(requires{s.fp.lanes;s.fp.count;s.fp.top;s.fp.status;})m.x87={s.fp.lanes,s.fp.count,s.fp.top,s.fp.status};
    StoreSiteIo<State,Sites> io{{s,pc},site_of};
    try {kernel(io,m);}
    catch(...){m.materialize_fault_flags();publish_machine(s,m);throw;}
    publish_machine(s,m);
    return true;
}

namespace live_support_boundary {
enum Register : unsigned {eax,ecx,edx,ebx,esp,ebp,esi,edi};
inline void set_low_byte(std::uint32_t &reg,std::uint32_t value){reg=(reg&~0xffu)|(value&0xffu);}
inline std::uint32_t float_bits(float x){return native_live_support::sse::float_to_bits(x);}

// The logic's Memory: typed accesses on the leaf framework's Io. Each access
// first publishes its original instruction (site(access)) to the cursor.
template<class Io,class Sites> struct Memory {
    Io &io;Sites site;
    template<class A> std::uint32_t load32(A a,std::uint32_t at){return io.rd(site(a),at,4);}
    template<class A> std::uint32_t load16(A a,std::uint32_t at){return io.rd(site(a),at,2);}
    template<class A> std::uint32_t load8(A a,std::uint32_t at){return io.rd(site(a),at,1);}
    // An 8-byte read is one instruction; its two dword reads report it.
    template<class A> std::uint64_t load64(A a,std::uint32_t at) {
        const std::uint64_t low=io.rd(site(a),at,4);return low|(std::uint64_t(io.rd(site(a),at+4u,4))<<32);
    }
    template<class A> void store64(A a,std::uint32_t at,std::uint64_t value){io.st(site(a),at,8,value);}
    template<class A> void store32(A a,std::uint32_t at,std::uint32_t value){io.st(site(a),at,4,value);}
    template<class A> void store16(A a,std::uint32_t at,std::uint32_t value){io.st(site(a),at,2,value&0xffffu);}
    template<class A> void store8(A a,std::uint32_t at,std::uint32_t value){io.st(site(a),at,1,value&0xffu);}
};
template<class Io,class Sites> Memory<Io,Sites> memory(Io &io,Sites sites){return {io,sites};}
// Memory with the original's block fill: REP STOSD of the whole dwords, then
// REP STOSB of the rest, each element reported at its string instruction
// (site for the dwords, tail_site for the bytes). EFLAGS.DF steps the
// destination down; the ABI keeps DF clear, but the fill honours it.
template<class Io,class Sites,class TailSites> struct StringMemory : Memory<Io,Sites> {
    TailSites tail_site;bool descending;
    template<class A> void fill(A a,std::uint32_t at,std::uint32_t bytes,std::uint32_t byte) {
        const auto pattern=(byte&0xffu)*0x01010101u;
        for(auto n=bytes>>2;n;--n){this->io.st(this->site(a),at,4,pattern);at+=descending?0xfffffffcu:4u;}
        for(auto n=bytes&3u;n;--n){this->io.st(tail_site(a),at,1,byte&0xffu);at+=descending?0xffffffffu:1u;}
    }
};

// Original instructions of each access role.
constexpr std::uint32_t c9550_site(native_live_support::unit_animation_mode::Access a) {
    using enum native_live_support::unit_animation_mode::Access;
    switch(a) {
    case object_table:return 0xc9550u;case object_headers:return 0xc9556u;case object:return 0xc9564u;
    case state_offset:return 0xc9568u;case mode:return 0xc956fu;case listed_mode:return 0xc9580u;
    }
    return 0;
}
constexpr std::uint32_t c1e90_site(native_live_support::bit_vector::Access){return 0xc1ea3u;}
constexpr std::uint32_t iterator_site(native_live_support::flagged_datum_iterator::Access a) {
    using enum native_live_support::flagged_datum_iterator::Access;
    switch(a) {
    case owner:return 0x1ec971u;case enabled:return 0x1ec977u;case index:return 0x1ec990u;case array:return 0x1ec994u;
    case first_unallocated:return 0x1ec999u;case in_use_bits:return 0x1ec9a0u;case in_use_word:return 0x1ec9b4u;
    case datum_size:return 0x1ec9f3u;case data:return 0x1ec9f6u;case index_store:return 0x1ec9feu;case salt:return 0x1eca01u;
    case handle_store:return 0x1eca09u;case maximum_count:return 0x1ec9beu;case reset_index_store:return 0x1ec9c1u;
    case reset_handle_store:return 0x1ec9c4u;case element_store:return 0x1ec9cfu;case flagged_only:return 0x1ec9d3u;
    case flag:return 0x1ec9dau;case handle:return 0x1ec9e1u;case published_store:return 0x1ec9e8u;
    case exhausted_handle:return 0x1eca0eu;case exhausted_published_store:return 0x1eca17u;
    }
    return 0;
}
constexpr std::uint32_t channel_site(native_live_support::animation_channel_iterator::Access a) {
    using enum native_live_support::animation_channel_iterator::Access;
    switch(a) {
    case position:return 0x1f7f60u;case group:return 0x1f7f6du;case subtable:return 0x1f7f71u;case owner:return 0x1f7f75u;
    case groups:return 0x1f7f77u;case entry:return 0x1f7f7au;case subtables:return 0x1f7f81u;case entry_count:return 0x1f7f8fu;
    case scan_owner:return 0x1f7f9bu;case group_count:return 0x1f7f9du;case scan_groups:return 0x1f7faeu;
    case selection:return 0x1f7fb7u;case subtable_count:return 0x1f7fc0u;case scan_subtables:return 0x1f7fc5u;
    case scan_entry_count:return 0x1f7fceu;case exhausted_record_store:return 0x1f7fdcu;case exhausted_record:return 0x1f7fe3u;
    case exhausted_position_store:return 0x1f7fecu;case entries:return 0x1f7ff8u;case record_id:return 0x1f800eu;
    case position_store:return 0x1f802au;case record_store:return 0x1f8034u;
    }
    return 0;
}
// 1F7E90: each selection dword store is its own instruction (index by the
// store's offset from the selection bytes).
struct ChannelBeginSites {
    std::uint32_t selection,source;
    constexpr std::uint32_t operator()(native_live_support::animation_channel_iterator::BeginAccess a,std::uint32_t at)const {
        using enum native_live_support::animation_channel_iterator::BeginAccess;
        const auto word=((at-selection)>>2)&3u;
        constexpr std::uint32_t zero[4]{0x1f7eabu,0x1f7eadu,0x1f7eb0u,0x1f7eb3u},copy[4]{0x1f7ec3u,0x1f7ec8u,0x1f7eceu,0x1f7ed4u},
            load[4]{0x1f7ebeu,0x1f7ec5u,0x1f7ecbu,0x1f7ed1u},none[4]{0x1f7ef5u,0x1f7ef7u,0x1f7efau,0x1f7efdu};
        switch(a) {
        case owner_store:return 0x1f7e9bu;case position_store:return 0x1f7e9du;
        case zero_store:return zero[word];case copy_store:return copy[word];case none_store:return none[word];
        case source_word:return load[((at-source)>>2)&3u];
        case record_count:return 0x1f7edbu;case records:return 0x1f7f10u;case record_group:return 0x1f7f13u;
        case source_choice:return 0x1f7f17u;case choice_count:return 0x1f7f26u;case choices:return 0x1f7f2bu;
        case choice_subtable:return 0x1f7f31u;case selection_store:return 0x1f7f3fu;
        }
        return 0;
    }
};
// Memory with the original's register saves and restores placed among its
// accesses: before(access) runs before an access and after(access) after it,
// so the whole access order (reads included) is the original's.
template<class Io,class Sites,class Before,class After> struct OrderedMemory : Memory<Io,Sites> {
    Before before;After after;
    template<class A> std::uint32_t load(A a,std::uint32_t at,unsigned width) {
        before(a);const auto value=this->io.rd(this->site(a),at,width);after(a);return value;
    }
    template<class A> std::uint32_t load32(A a,std::uint32_t at){return load(a,at,4);}
    template<class A> std::uint32_t load16(A a,std::uint32_t at){return load(a,at,2);}
    template<class A> std::uint32_t load8(A a,std::uint32_t at){return load(a,at,1);}
    template<class A> void store(A a,std::uint32_t at,unsigned width,std::uint32_t value) {
        before(a);this->io.st(this->site(a),at,width,value);after(a);
    }
    template<class A> void store32(A a,std::uint32_t at,std::uint32_t value){store(a,at,4,value);}
    template<class A> void store16(A a,std::uint32_t at,std::uint32_t value){store(a,at,2,value&0xffffu);}
    template<class A> void store8(A a,std::uint32_t at,std::uint32_t value){store(a,at,1,value&0xffu);}
};
// Memory whose site table also sees the access address (one role, several
// instructions).
template<class Io,class Sites> struct AddressedMemory {
    Io &io;Sites site;
    template<class A> std::uint32_t load32(A a,std::uint32_t at){return io.rd(site(a,at),at,4);}
    template<class A> std::uint32_t load8(A a,std::uint32_t at){return io.rd(site(a,at),at,1);}
    template<class A> void store32(A a,std::uint32_t at,std::uint32_t value){io.st(site(a,at),at,4,value);}
    template<class A> void store8(A a,std::uint32_t at,std::uint32_t value){io.st(site(a,at),at,1,value&0xffu);}
};
constexpr std::uint32_t level_block_site(native_live_support::unit_level_block::Access a) {
    using enum native_live_support::unit_level_block::Access;
    switch(a) {
    case object_table:return 0x110355u;case object_headers:return 0x110364u;case object:return 0x11036au;case tag:return 0x11036eu;
    case block_offset:return 0x110370u;case tag_instances:return 0x11037eu;case definition:return 0x110387u;
    case attachment_offset:return 0x11038bu;case attachment:return 0x110396u;case threshold:return 0x11039du;
    case level_compare:return 0x1103a5u;case level:return 0x1103a9u;case ramp_down:return 0x1103b0u;case floor_store:return 0x1103c6u;
    case ramp_up:return 0x1103d2u;case ceiling:return 0x1103dau;case ramp_store:return 0x1103edu;case decay_level:return 0x1103f9u;
    case decay_step:return 0x1103feu;case byte_scale:return 0x110411u;case default_scale:return 0x110419u;case output_scale:return 0x110421u;
    case decay_store:return 0x110429u;case definition_scale:return 0x110440u;case byte:return 0x110453u;case byte_store:return 0x11048eu;
    case owner_headers:return 0x1104a0u;case owner_object:return 0x1104a3u;case owner_offset:return 0x1104a7u;case owner_word:return 0x1104aeu;
    case owner_level:return 0x1104b6u;case countdown_store:return 0x1104c9u;case countdown:return 0x1104d2u;case random:return 0x1104d9u;
    case seed:return 0x1104dfu;case seed_store:return 0x1104edu;case reload_store:return 0x110500u;case step_store:return 0x11050du;
    }
    return 0;
}

// 243F20: original instructions per layout and role.
constexpr std::uint32_t vertex_site(native_live_support::vertex_position::Access a) {
    using namespace native_live_support::vertex_position;
    using enum Role;
    if(a.role==descriptor)return 0x243f20u;
    if(a.role==layout)return 0x243f27u;
    const auto axis=[&](std::uint32_t x,std::uint32_t y,std::uint32_t z){return a.axis==0?x:a.axis==1?y:z;};
    switch(a.layout) {
    case Layout::other:
        switch(a.role) {
        case default_point:return 0x244153u;
        case coordinate:return axis(0x244159u,0x24415du,0x244163u);
        case store:return axis(0x24415bu,0x244160u,0x244166u);
        case node_store:return 0x24416du;
        default:return 0;
        }
    case Layout::uncompressed:
        switch(a.role) {
        case data_offset:return 0x243f41u;case data_base:return 0x243f44u;
        case coordinate:return axis(0x243f4du,0x243f51u,0x243f57u);
        case store:return axis(0x243f4fu,0x243f54u,0x243f5au);
        case node_store:return 0x24416du;
        default:return 0;
        }
    case Layout::uncompressed_with_node:
        switch(a.role) {
        case data_offset:return 0x24403au;case data_base:return 0x24403du;
        case coordinate:return axis(0x24404bu,0x24404fu,0x244055u);
        case store:return axis(0x24404du,0x244052u,0x244058u);
        case node:return 0x244061u;case node_store:return 0x244067u;
        default:return 0;
        }
    case Layout::compressed:
        switch(a.role) {
        case data_offset:return 0x243f62u;case data_base:return 0x243f65u;
        case maximum:return axis(0x243f68u,0x243f91u,0x243f9bu);
        case minimum:return axis(0x243f6du,0x243f96u,0x243fa0u);
        case two:return 0x243f71u;case one:return 0x243f79u;case step:return 0x243f81u;case half:return 0x243f89u;
        case coordinate:return axis(0x243faau,0x243fb5u,0x243fb9u);
        case offset:return axis(0x243fd1u,0x243fedu,0x244014u);
        case store:return axis(0x244023u,0x24402fu,0x244032u);
        case node_store:return 0x24416du;
        default:return 0;
        }
    case Layout::compressed_with_node:
        switch(a.role) {
        case data_offset:return 0x244070u;case data_base:return 0x244073u;
        case maximum:return axis(0x244076u,0x24409fu,0x2440a9u);
        case minimum:return axis(0x24407bu,0x2440a4u,0x2440aeu);
        case two:return 0x24407fu;case one:return 0x244087u;case step:return 0x24408fu;case half:return 0x244097u;
        case coordinate:return axis(0x2440b7u,0x2440bfu,0x2440e0u);
        case offset:return axis(0x2440d8u,0x2440f9u,0x244120u);
        case store:return axis(0x24412fu,0x24413bu,0x24413eu);
        case node:return 0x244143u;case node_store:return 0x24414au;
        default:return 0;
        }
    }
    return 0;
}
// 243F20's coordinate spills: the stack slot of each axis (x, y, z), and the
// store and reload instructions of the two compressed layouts.
struct VertexSpill {std::uint32_t slot,store,reload;};
constexpr VertexSpill vertex_spill(native_live_support::vertex_position::Layout layout,unsigned axis) {
    constexpr std::uint32_t slot[3]{0xcu,0x10u,0x14u};
    constexpr std::uint32_t compressed[2][3]{{0x244019u,0x243ff2u,0x244025u},{0x24401fu,0x243ffcu,0x24402bu}};
    constexpr std::uint32_t with_node[2][3]{{0x244125u,0x2440feu,0x244131u},{0x24412bu,0x244108u,0x244137u}};
    const auto &table=layout==native_live_support::vertex_position::Layout::compressed?compressed:with_node;
    return {slot[axis],table[0][axis],table[1][axis]};
}
constexpr std::uint32_t ftol_site(native_live_support::float_to_int64::Access a) {
    using enum native_live_support::float_to_int64::Access;
    switch(a) {
    case sign_store:return 0x32cc17u;case integer_store:return 0x32cc1bu;case integer_reload:return 0x32cc1fu;
    case sign_load:return 0x32cc23u;case low_load:return 0x32cc27u;case zero_high_load:return 0x32cc6bu;
    case discard_rounded_store:return 0x32cc77u;case discard_value_store:return 0x32cc7bu;
    case negative_difference_store:return 0x32cc35u;case negative_difference_load:return 0x32cc38u;case negative_high_load:return 0x32cc4au;
    case positive_difference_store:return 0x32cc53u;case positive_difference_load:return 0x32cc56u;case positive_high_load:return 0x32cc62u;
    }
    return 0;
}
constexpr std::uint32_t reset_site(native_live_support::data_array_reset::Access a) {
    using enum native_live_support::data_array_reset::Access;
    switch(a) {
    case first_unallocated_store:return 0x16e608u;case actual_count_store:return 0x16e60fu;case next_index_store:return 0x16e616u;
    case flags:return 0x16e622u;case salt:case salt_store:return 0x16e625u;
    case fill_datum_size:return 0x16e634u;case fill_maximum_count:return 0x16e637u;case fill_data:return 0x16e63bu;
    case debug_fill:return 0x16e648u;
    case datum_count:return 0x16e651u;case datum_size:return 0x16e660u;case datum_data:return 0x16e663u;
    case datum_salt_store:return 0x16e669u;case datum_recount:return 0x16e66fu;
    case bit_count:return 0x16e677u;case bit_vector:return 0x16e67au;case bit_clear:return 0x16e68du;
    }
    return 0;
}
constexpr std::uint32_t strncpy_site(native_live_support::bounded_string_copy::Access a) {
    using enum native_live_support::bounded_string_copy::Access;
    switch(a) {
    case leading_byte:return 0x32a50cu;case leading_store:return 0x32a511u;case trailing_byte:return 0x32a533u;
    case trailing_store:return 0x32a538u;case word_test:return 0x32a594u;case word:return 0x32a59du;case word_store:return 0x32a585u;
    case terminated_store_0:return 0x32a5dbu;case terminated_store_1:return 0x32a5d5u;case terminated_store_2:return 0x32a5cbu;
    case terminated_store_3:return 0x32a5c1u;case leading_fill:return 0x32a556u;case word_fill:return 0x32a5e9u;
    case byte_fill:return 0x32a573u;
    }
    return 0;
}
constexpr std::uint32_t setup_site(native_live_support::data_array_setup::Access a) {
    using enum native_live_support::data_array_setup::Access;
    switch(a) {
    case header_clear:return 0x16e4bau;case flags:case flags_store:return 0x16e4d1u;case name_end_store:return 0x16e4d7u;
    case maximum_count_store:return 0x16e4dau;case datum_size_store:return 0x16e4e1u;case data_clear_store:return 0x16e4e8u;
    case valid_store:return 0x16e4ebu;case alignment_store:return 0x16e4f5u;case signature_store:return 0x16e4f8u;
    case in_use_bits_store:return 0x16e4ffu;case allocation_store:return 0x16e502u;
    case reset_flags:case reset_flags_store:return 0x16e480u;case data_store:return 0x16e48fu;case bit_vector:return 0x16e492u;
    case bit_clear:return 0x16e49au;
    }
    return 0;
}
constexpr std::uint32_t setup_tail_site(native_live_support::data_array_setup::Access a) {
    return a==native_live_support::data_array_setup::Access::bit_clear?0x16e4a1u:0x16e4bau;
}
constexpr std::uint32_t reset_tail_site(native_live_support::data_array_reset::Access a) {
    return a==native_live_support::data_array_reset::Access::debug_fill?0x16e64fu:0x16e694u;
}
constexpr std::uint32_t interval_site(native_live_support::interval_table::Access a) {
    using enum native_live_support::interval_table::Access;
    switch(a) {
    case clear_store:return 0x171012u;case count:return 0x171018u;case records:return 0x171022u;case first:return 0x171026u;
    case length:return 0x17102eu;case found_first:return 0x171042u;case offset_store:return 0x17104cu;
    case count_recheck:return 0x17104fu;case missing_store:return 0x171059u;case index_store:return 0x171066u;
    }
    return 0;
}
constexpr std::uint32_t triangle_site(native_live_support::triangle_point::Access a) {
    using enum native_live_support::triangle_point::Access;
    switch(a) {
    case point_x:return 0x1211c3u;case origin_x:return 0x1211c7u;case edge1_y_end:return 0x1211cbu;case origin_y:return 0x1211d0u;
    case edge1_z_end:return 0x1211d5u;case origin_z:return 0x1211dau;case edge2_y_end:return 0x1211dfu;case origin_y2:return 0x1211e4u;
    case edge2_x_end:return 0x1211e9u;case origin_x2:return 0x1211edu;case edge1_x_end:return 0x1211f1u;case origin_x3:return 0x1211f5u;
    case offset_x_store:return 0x1211f9u;case point_y:return 0x1211ffu;case origin_y3:return 0x121204u;case offset_y_store:return 0x121209u;
    case point_z:return 0x12120fu;case origin_z2:return 0x121214u;case offset_z_store:return 0x121219u;case edge2_z_end:return 0x12121fu;
    case origin_z3:return 0x121224u;case edge2_y_store:return 0x121229u;case edge1_z_store:return 0x12123au;case edge2_z_store:return 0x12124bu;
    case edge1_y_store:return 0x121251u;case offset_y:return 0x12126au;case offset_z:return 0x121273u;case offset_x:return 0x121284u;
    case normal_z_store:return 0x12129cu;case normal_y_store:return 0x1212a9u;case tolerance:return 0x1212beu;
    case edge2_x_store:return 0x1212c9u;case edge1_x_store:return 0x1212cfu;case normal_x_store:return 0x1212d5u;
    case dominant_component:return 0x1212ebu;case zero:return 0x1212f0u;case negative_tolerance:return 0x1212ffu;
    case projection_u:return 0x121312u;case projection_v:return 0x121319u;case offset_v:return 0x121323u;case edge2_v:return 0x121329u;
    case edge2_u:return 0x121332u;case offset_u:return 0x121338u;case edge2_u_store:return 0x12133eu;case edge1_v:return 0x12135eu;
    case edge1_u:return 0x121364u;case edge2_u_reload:return 0x12137eu;case degenerate_tolerance:return 0x121390u;
    case sum_tolerance:return 0x12139fu;case one:return 0x1213b0u;case u_store:return 0x1213cbu;case v_store:return 0x1213d3u;
    }
    return 0;
}
constexpr std::uint32_t busy_scan_site(native_live_support::resource_references::Access a) {
    using enum native_live_support::resource_references::Access;
    switch(a) {
    case count:return 0x22a790u;case references:return 0x22a7a7u;case id:return 0x22a7aau;case kind:return 0x22a7b5u;
    case element:return 0x22a7bau;case offset:return 0x22a7c5u;case element_offset:return 0x22a7cdu;case holders:return 0x22a7d4u;
    case resource:return 0x22a7ddu;case recount:return 0x22a7f4u;
    }
    return 0;
}
// 22A790's Memory: each load also writes the register the original loads
// into (ECX: the count, the reference id, its element word, the holder
// table; EDX: the element path's offset; EAX: the rest; the re-read count
// also steps EBX and EDI), so the child call and the exits see them.
template<class Io,class Flags> struct BusyScanMemory {
    using Access=native_live_support::resource_references::Access;
    Io &io;Machine<Flags> &m;
    std::uint32_t load(Access a,std::uint32_t at,unsigned width) {
        auto &gp=m.gp;const auto value=io.rd(busy_scan_site(a),at,width);
        switch(a) {
        case Access::count:case Access::id:case Access::holders:gp[ecx]=value;break;
        case Access::element:gp[ecx]=(gp[ecx]&~0xffffu)|value;break;
        case Access::element_offset:gp[edx]=value;break;
        case Access::kind:break;  // CMP BYTE PTR [EAX],2
        case Access::recount:gp[eax]=value;++gp[ebx];gp[edi]+=native_live_support::resource_references::reference::size;break;
        default:gp[eax]=value;break;
        }
        return value;
    }
    std::uint32_t load32(Access a,std::uint32_t at){return load(a,at,4);}
    std::uint32_t load16(Access a,std::uint32_t at){return load(a,at,2);}
    std::uint32_t load8(Access a,std::uint32_t at){return load(a,at,1);}
};
// Memory whose loads also write the register the original loads into
// (hook(access, value) after each load): a native with genuine children
// keeps the registers its children see.
template<class Io,class Sites,class Hook> struct LoadingMemory : Memory<Io,Sites> {
    Hook hook;
    template<class A> std::uint32_t load(A a,std::uint32_t at,unsigned width) {
        const auto value=this->io.rd(this->site(a),at,width);hook(a,value);return value;
    }
    template<class A> std::uint32_t load32(A a,std::uint32_t at){return load(a,at,4);}
    template<class A> std::uint32_t load16(A a,std::uint32_t at){return load(a,at,2);}
    template<class A> std::uint32_t load8(A a,std::uint32_t at){return load(a,at,1);}
};
constexpr std::uint32_t ray_site(native_live_support::collision_model_ray::Access a) {
    using enum native_live_support::collision_model_ray::Access;
    switch(a) {
    case initial_fraction:return 0x1f820du;case fraction_reset:return 0x1f8220u;case region_remap:return 0x1f8225u;
    case selection:return 0x1f8228u;case model:return 0x1f822bu;case position:return 0x1f8260u;case node_matrices:return 0x1f826eu;
    case scale:return 0x1f8290u;case origin_one:return 0x1f8296u;case origin_x:return 0x1f829du;case origin_y:return 0x1f82a1u;
    case origin_z:return 0x1f82a6u;case left_x:return 0x1f82cfu;case forward_x:return 0x1f82d5u;case left_y:return 0x1f82dbu;
    case vector_one:return 0x1f82ecu;case up_x:return 0x1f8301u;case position_x:return 0x1f830bu;case local_origin_x:return 0x1f8315u;
    case up_y:return 0x1f831bu;case forward_y:return 0x1f832cu;case position_y:return 0x1f833au;case local_origin_y:return 0x1f8340u;
    case up_z:return 0x1f8346u;case left_z:return 0x1f8353u;case forward_z:return 0x1f835du;case vector_z:return 0x1f8367u;
    case vector_x:return 0x1f8370u;case vector_y:return 0x1f8378u;case position_z:return 0x1f837du;case local_origin_z:return 0x1f8387u;
    case forward_x_again:return 0x1f839bu;case bsp:return 0x1f83a8u;case up_x_again:return 0x1f83beu;case local_vector_x:return 0x1f83c8u;
    case left_y_again:return 0x1f83ceu;case forward_y_again:return 0x1f83e0u;case left_z_again:return 0x1f83eeu;
    case forward_z_again:return 0x1f8400u;case fraction:return 0x1f840eu;case local_vector_y:return 0x1f8414u;
    case flags_store:return 0x1f8420u;case bsp_store:return 0x1f8424u;case local_vector_z:return 0x1f8430u;
    case bitmap_limit_store:return 0x1f8436u;case bitmap_store:return 0x1f843bu;case origin_store:return 0x1f843fu;
    case vector_store:return 0x1f8443u;case output_store:return 0x1f8447u;case floor_store:return 0x1f8459u;
    case record_count_store:return 0x1f845du;case previous_leaf_store:return 0x1f8460u;case previous_kind_store:return 0x1f8467u;
    case plane_store:return 0x1f846eu;case limit_zero_store:return 0x1f8477u;case limit_one:return 0x1f847fu;
    case limit_one_store:return 0x1f848au;case limit_store:return 0x1f8492u;case limit:return 0x1f8498u;case region:return 0x1f84adu;
    case hit_position_store:return 0x1f84b6u;case hit_node_store:return 0x1f84b9u;case hit_region_store:return 0x1f84bdu;
    }
    return 0;
}
constexpr std::uint32_t texel_site(native_live_support::bitmap_texel::Access a) {
    using enum native_live_support::bitmap_texel::Access;
    switch(a) {
    case pixels:return 0x138d96u;case one:return 0x138da1u;case mipmap_count:return 0x138db1u;case level_spill:return 0x138dc9u;
    case level_load:return 0x138dcfu;case level_store:return 0x138dd3u;case level_reload:return 0x138dd7u;case width:return 0x138de1u;
    case width_store:return 0x138df8u;case width_one_store:return 0x138dfeu;case width_one_reload:return 0x138e06u;
    case flags:return 0x138e0au;case width_round_store:return 0x138e1fu;case height:return 0x138e25u;case height_store:return 0x138e3au;
    case height_one_store:return 0x138e40u;case height_one_reload:return 0x138e48u;case height_round_store:return 0x138e5cu;
    case coordinate:return 0x138e60u;case half:return 0x138e64u;case u:return 0x138e78u;case u_spill:return 0x138e80u;
    case u_masked_load:return 0x138e88u;case u_masked_store:return 0x138e8cu;case u_masked:return 0x138e90u;
    case u_divided_load:return 0x138e99u;case u_divided_store:return 0x138e9du;case u_divided:return 0x138ea1u;
    case height_reload:return 0x138eafu;case v:return 0x138ec1u;case v_spill:return 0x138ecau;case v_masked_load:return 0x138ed2u;
    case v_masked_store:return 0x138ed6u;case v_masked:return 0x138eddu;case v_divided_load:return 0x138ee3u;
    case v_divided_store:return 0x138ee7u;case v_divided:return 0x138eebu;case type:return 0x138efbu;
    }
    return 0;
}
constexpr std::uint32_t effect_update_site(native_live_support::effect_update::Access a) {
    using enum native_live_support::effect_update::Access;
    switch(a) {
    case game_time:return 0x179cd1u;case time_step:return 0x179cd6u;case effects:return 0x179cddu;case first_size:return 0x179cfau;
    case first_data:return 0x179cfdu;case first_salt:return 0x179d03u;case array:return 0x179d2au;case data:return 0x179d30u;
    case flags:return 0x179d40u;case particle_systems:return 0x179d4eu;case array_reread:return 0x179d74u;
    case first_unallocated:return 0x179d93u;case in_use_bits:return 0x179d9au;case in_use_word:return 0x179db1u;
    case next_size:return 0x179dc7u;case next_data:return 0x179dcau;case next_salt:return 0x179dd0u;
    }
    return 0;
}
} // namespace live_support_boundary

// C9550 (fastcall: ECX = unit handle). AL = listed; ECX = mode; EDX = the
// table address after the last compare. Flags: TEST AL,AL after a match,
// otherwise the table-bound CMP. RET 0.
template<class Io,class Flags> void kernel_c9550(Io &io,Machine<Flags> &m) {
    namespace b=live_support_boundary;using namespace native_live_support::unit_animation_mode;
    auto memory=b::memory(io,b::c9550_site);
    const auto result=listed(memory,m.gp[b::ecx]);
    b::set_low_byte(m.gp[b::eax],result.listed);
    m.gp[b::ecx]=result.mode;m.gp[b::edx]=result.residue.next_entry;
    if(result.listed)m.flags.logic(1u,1);
    else m.flags.minus(result.residue.next_entry,listed_modes+listed_mode_count*4u,4);
    pop(io,m,0xc958du);
}

// C1E90 (EDX = bit index, [ESP+4] = vector). EAX = bit (NEG/SBB/NEG of the
// masked word; flags of the last NEG); ECX = vector; EDX = word index. RET 4.
template<class Io,class Flags> void kernel_c1e90(Io &io,Machine<Flags> &m) {
    namespace b=live_support_boundary;
    auto memory=b::memory(io,b::c1e90_site);
    const auto index=m.gp[b::edx];
    m.gp[b::ecx]=io.rd(0xc1e9cu,m.gp[b::esp]+4u,4);
    m.gp[b::edx]=std::uint32_t(std::int32_t(index)>>5);
    const bool set=native_live_support::bit_vector::test(memory,m.gp[b::ecx],index);
    m.gp[b::eax]=m.flags.minus(0u,set?0xffffffffu:0u,4);
    pop(io,m,0xc1eacu);m.gp[b::esp]+=4u;
}

// 1EC970 (ESI = iterator). PUSH ECX reserves a result slot, which is set to 0
// after the enable test and returned when there is nothing to return. EAX =
// datum or that slot; ECX = the slot; EDX = the published handle (or, when
// disabled, the enable byte in DL). Flags: TEST DL,DL (disabled), TEST ECX,ECX
// (exhausted) or the TEST AL,AL of the last flag byte. EBX/EBP/EDI restored.
template<class Io,class Flags> void kernel_1ec970(Io &io,Machine<Flags> &m) {
    namespace b=live_support_boundary;using namespace native_live_support::flagged_datum_iterator;
    auto &gp=m.gp;auto memory=b::memory(io,b::iterator_site);
    push(io,m,0x1ec970u,gp[b::ecx]);
    const auto slot=gp[b::esp];
    const auto on=enabled(memory);
    gp[b::eax]=0;b::set_low_byte(gp[b::edx],on);
    io.st(0x1ec97eu,slot,4,0u);
    if(!on) {
        m.flags.logic(0u,1);
        gp[b::ecx]=pop(io,m,0x1eca1bu);pop(io,m,0x1eca1cu);return;
    }
    push(io,m,0x1ec987u,gp[b::ebx]);push(io,m,0x1ec988u,gp[b::ebp]);push(io,m,0x1ec989u,gp[b::edi]);
    const auto step=advance(memory,gp[b::esi]);
    gp[b::edx]=step.handle;
    if(!step.element) {
        m.flags.logic(0u,4);
        gp[b::eax]=io.rd(0x1eca11u,slot,4);
        gp[b::edi]=pop(io,m,0x1eca15u);gp[b::ebp]=pop(io,m,0x1eca16u);gp[b::ebx]=pop(io,m,0x1eca1au);
        gp[b::ecx]=pop(io,m,0x1eca1bu);pop(io,m,0x1eca1cu);return;
    }
    m.flags.logic(step.last_flag_tested,1);
    gp[b::eax]=step.element;
    gp[b::edi]=pop(io,m,0x1ec9e4u);gp[b::ebp]=pop(io,m,0x1ec9e5u);gp[b::ebx]=pop(io,m,0x1ec9ebu);
    gp[b::ecx]=pop(io,m,0x1ec9ecu);pop(io,m,0x1ec9edu);
}

// 1F7F60 (ESI = iterator). EAX = record != 0; ECX = record; EDX = record, or
// when exhausted the scan's last examined value (else the caller's EDX).
// Flags: TEST ECX,ECX. EBX/EBP/EDI restored. RET 0. The saves follow the
// position read and the restores interleave with the exit stores, as in the
// original (a caller replaying this leaf inline sees its exact access order).
template<class Io,class Flags> void kernel_1f7f60(Io &io,Machine<Flags> &m) {
    namespace b=live_support_boundary;using namespace native_live_support::animation_channel_iterator;
    auto &gp=m.gp;
    const auto before=[&](Access a){if(a==Access::position_store)gp[b::edi]=pop(io,m,0x1f8021u);};
    const auto after=[&](Access a) {
        switch(a) {
        case Access::position:push(io,m,0x1f7f63u,gp[b::ebx]);push(io,m,0x1f7f69u,gp[b::ebp]);push(io,m,0x1f7f6au,gp[b::edi]);break;
        case Access::exhausted_record:gp[b::edi]=pop(io,m,0x1f7fe6u);gp[b::ebp]=pop(io,m,0x1f7febu);break;
        case Access::position_store:gp[b::ebp]=pop(io,m,0x1f8033u);break;
        default:break;
        }
    };
    b::OrderedMemory<Io,decltype(&b::channel_site),decltype(before),decltype(after)> memory{{io,&b::channel_site},before,after};
    const auto step=next(memory,gp[b::esi]);
    gp[b::ecx]=step.record;gp[b::eax]=step.record!=0u;
    m.flags.logic(step.record,4);
    if(!step.found) {
        if(step.residue.last_examined)gp[b::edx]=*step.residue.last_examined;
        gp[b::ebx]=pop(io,m,0x1f7ff6u);pop(io,m,0x1f7ff7u);return;
    }
    gp[b::edx]=step.record;
    gp[b::ebx]=pop(io,m,0x1f803au);pop(io,m,0x1f803bu);
}

// 1F7E90 (EAX = owner, EBX = source selection or 0; [ESP+4] = iterator,
// [ESP+8] = remap table or 0). The remap path keeps the clamped record count
// in the [ESP+8] slot. ECX = iterator + 4 (ADD ECX,4) unless the remap loop
// ran; EDX = 0 (zeroed) or the source's fourth dword (copied, EAX its third).
// After the remap loop EAX = the re-read count, ECX = the last source choice
// (or, when it was stored, the sign-extended group byte with EDX = the
// iterator), and DL = the last group byte. Flags: ADD ECX,4, TEST EAX,EAX
// (no records) or the loop's CMP ESI,EAX. EBP (and ESI/EDI) restored. RET 8.
template<class Io,class Flags> void kernel_1f7e90(Io &io,Machine<Flags> &m) {
    namespace b=live_support_boundary;using namespace native_live_support::animation_channel_iterator;
    auto &gp=m.gp;
    const auto it=io.rd(0x1f7e92u,gp[b::esp]+4u,4);
    push(io,m,0x1f7e96u,gp[b::ebp]);
    const auto slot=gp[b::esp]+0xcu;  // the remap table argument
    const auto remap=io.rd(0x1f7e97u,slot,4);
    struct Frame {
        Io &io;Machine<Flags> &m;std::uint32_t slot;
        void limit_records(std::uint32_t count) {
            io.st(0x1f7ee1u,slot,4,remap_table::maximum_records);
            if(std::int32_t(count)<=std::int32_t(remap_table::maximum_records))io.st(0x1f7eebu,slot,4,count);
        }
        std::uint32_t record_limit(){const auto limit=io.rd(0x1f7f00u,slot,4);push(io,m,0x1f7f04u,m.gp[b::esi]);return limit;}
        void begin_records(){push(io,m,0x1f7f0bu,m.gp[b::edi]);}
        std::uint32_t iterator(){return io.rd(0x1f7f3bu,slot-4u,4);}
        std::uint32_t reread_limit(){return io.rd(0x1f7f43u,slot,4);}
    } frame{io,m,slot};
    const auto source=gp[b::ebx];
    b::AddressedMemory<Io,b::ChannelBeginSites> memory{io,{it+iterator::selected_subtable,source}};
    const auto result=begin(memory,frame,it,gp[b::eax],source,remap);
    const auto &r=result.residue;
    gp[b::ecx]=it+4u;
    switch(result.selection) {
    case Selection::zeroed:gp[b::edx]=0;m.flags.plus(it,4u,4);break;
    case Selection::copied:gp[b::eax]=r.copied_words[0];gp[b::edx]=r.copied_words[1];m.flags.plus(it,4u,4);break;
    case Selection::remapped:
        gp[b::eax]=std::uint32_t(r.record_limit);
        if(!r.last) {
            m.flags.logic(gp[b::eax],4);
            gp[b::esi]=pop(io,m,0x1f7f50u);gp[b::ebp]=pop(io,m,0x1f7f51u);pop(io,m,0x1f7f52u);gp[b::esp]+=8u;return;
        }
        // EDX holds the last re-read iterator under the last group byte (DL).
        if(r.iterator)gp[b::edx]=*r.iterator;
        gp[b::ecx]=std::uint32_t(r.last->choice);b::set_low_byte(gp[b::edx],r.last->group);
        if(r.last->selected){gp[b::ecx]=std::uint32_t(native_live_support::widen8(r.last->group));gp[b::edx]=*r.iterator;}
        m.flags.minus(r.records_examined,gp[b::eax],4);
        gp[b::edi]=pop(io,m,0x1f7f4fu);gp[b::esi]=pop(io,m,0x1f7f50u);gp[b::ebp]=pop(io,m,0x1f7f51u);pop(io,m,0x1f7f52u);gp[b::esp]+=8u;
        return;
    }
    gp[b::ebp]=pop(io,m,result.selection==Selection::zeroed?0x1f7eb6u:0x1f7ed7u);
    pop(io,m,result.selection==Selection::zeroed?0x1f7eb7u:0x1f7ed8u);gp[b::esp]+=8u;
}

// 22A790 (ESI = owner, [ESP+4] = the block; a leaf with a child). The
// original keeps the count in ECX until the scan, the block + 8 in EBP, the
// index in EBX and index * 16 in EDI; the access hooks below keep the
// registers each load writes, so the 4057F0 call (ECX = EAX = the resource,
// the flags of TEST EAX,EAX, after the return address push) and the exits
// see the original's registers. AL = 1 when busy (TEST AL,AL), else 0 over
// the re-read count (XOR AL,AL) or the caller's EAX (empty: TEST ECX,ECX).
// EBX, EBP and EDI restored. RET 4.
template<class Io,class Flags,class Busy> void kernel_22a790(Io &io,Machine<Flags> &m,Busy &&busy_query) {
    namespace b=live_support_boundary;using namespace native_live_support::resource_references;
    auto &gp=m.gp;
    b::BusyScanMemory<Io,Flags> memory{io,m};
    struct Frame {
        Io &io;Machine<Flags> &m;Busy &query;
        std::uint32_t block() {
            auto &gp=m.gp;
            push(io,m,0x22a793u,gp[b::ebx]);push(io,m,0x22a794u,gp[b::ebp]);
            const auto block=io.rd(0x22a795u,gp[b::esp]+0xcu,4);
            b::set_low_byte(gp[b::eax],0u);gp[b::ebp]=block+block_skip;gp[b::ebx]=0;
            return block;
        }
        void begin_scan(){push(io,m,0x22a7a4u,m.gp[b::edi]);m.gp[b::edi]=0;}
        std::uint32_t busy(std::uint32_t resource) {
            auto &gp=m.gp;
            gp[b::eax]=gp[b::ecx]=resource;m.flags.logic(resource,4);
            push(io,m,0x22a7e6u,0x22a7ebu);io.call(m,0x4057f0u,query);
            return gp[b::eax];
        }
    } frame{io,m,busy_query};
    switch(any_busy(memory,frame,gp[b::esi])) {
    case Exit::empty:
        m.flags.logic(gp[b::ecx],4);
        gp[b::ebp]=pop(io,m,0x22a80au);gp[b::ebx]=pop(io,m,0x22a80bu);pop(io,m,0x22a80cu);break;
    case Exit::idle:
        gp[b::edi]=pop(io,m,0x22a7ffu);gp[b::ebp]=pop(io,m,0x22a800u);
        b::set_low_byte(gp[b::eax],0u);m.flags.logic(0u,1);
        gp[b::ebx]=pop(io,m,0x22a803u);pop(io,m,0x22a804u);break;
    case Exit::busy:
        b::set_low_byte(gp[b::eax],1u);m.flags.logic(1u,1);
        gp[b::edi]=pop(io,m,0x22a809u);gp[b::ebp]=pop(io,m,0x22a80au);gp[b::ebx]=pop(io,m,0x22a80bu);pop(io,m,0x22a80cu);break;
    }
    gp[b::esp]+=4u;
}

// 110350 (EAX = unit handle). SUB ESP,8 makes the byte loop's counter
// ([ESP+10] after the saves) and spill ([ESP+14]) slots. ECX = block. The
// attached paths return AL = 1 over the object address, EDX = the block
// offset; the detached paths return AL = any byte nonzero over the residue
// the original leaves in EAX/EDX. Flags: the final ADD ESP,8. RET 0.
template<class Io,class Flags> void kernel_110350(Io &io,Machine<Flags> &m) {
    namespace b=live_support_boundary;using namespace native_live_support::unit_level_block;
    auto &gp=m.gp;auto &x=m.x;auto memory=b::memory(io,b::level_block_site);
    gp[b::esp]-=8u;
    push(io,m,0x110353u,gp[b::ebx]);push(io,m,0x110354u,gp[b::ebp]);push(io,m,0x110360u,gp[b::esi]);push(io,m,0x11037du,gp[b::edi]);
    struct Frame {
        Io &io;std::uint32_t esp;
        void begin_bytes(std::uint32_t count){io.st(0x110431u,esp+0x10u,4,count);}
        float spill(float value) {
            io.st(0x110466u,esp+0x14u,4,b::float_bits(value));
            return native_live_support::sse::bits_to_float(io.rd(0x11046cu,esp+0x14u,4));
        }
        std::uint32_t count_down(){const auto left=io.rd(0x110494u,esp+0x10u,4)-1u;io.st(0x11049au,esp+0x10u,4,left);return left;}
    } frame{io,gp[b::esp]};
    const auto result=update(memory,frame,gp[b::eax]);
    const auto &r=result.residue;
    const auto lane=[](float value){return _mm_set_ss(value);};
    gp[b::ecx]=result.block;
    std::array<std::uint32_t,5> exit{};  // EDI, ESI, EBP, EBX and RET pops
    switch(result.path) {
    case Path::floored:case Path::ramped_down:case Path::ramped_up:
        gp[b::eax]=(r.object&~0xffu)|1u;gp[b::edx]=r.block_offset;
        x[0]=lane(r.level);
        x[1]=result.path==Path::ramped_up?lane(r.ceiling):_mm_setzero_ps();
        exit=result.path==Path::floored?std::array{0x1103c0u,0x1103c1u,0x1103c2u,0x1103cdu,0x1103d1u}:
                                         std::array{0x1103eau,0x1103ebu,0x1103ecu,0x1103f4u,0x1103f8u};
        break;
    default: {
        x[0]=lane(result.path==Path::no_owner?r.last_scaled:r.level);x[1]=_mm_setzero_ps();
        x[2]=lane(r.byte_scale);x[3]=lane(r.default_scale);x[4]=lane(r.output_scale);x[5]=lane(r.last_product);
        gp[b::edx]=r.rows;gp[b::eax]=r.owner_offset;
        if(result.path!=Path::no_owner) {
            // LAHF after the UCOMISS of the level with zero (re-executed: the
            // comparison's MXCSR effect is idempotent).
            m.flags.scalar_compare(x[0],_mm_setzero_ps(),true);
            gp[b::eax]=(gp[b::eax]&~0xff00u)|(((m.flags.bits()|2u)&0xffu)<<8);
        }
        if(result.path==Path::reloaded){gp[b::eax]=r.reload_product;gp[b::edx]=r.random;}
        b::set_low_byte(gp[b::eax],result.any_byte_nonzero);
        exit=result.path==Path::nonzero_level?std::array{0x1104c4u,0x1104c5u,0x1104c6u,0x1104cdu,0x1104d1u}:
             result.path==Path::reloaded?std::array{0x1104f9u,0x1104ffu,0x110503u,0x110506u,0x11050au}:
                                         std::array{0x110510u,0x110511u,0x110512u,0x110515u,0x110519u};
        break;
    }
    }
    gp[b::edi]=pop(io,m,exit[0]);gp[b::esi]=pop(io,m,exit[1]);gp[b::ebp]=pop(io,m,exit[2]);gp[b::ebx]=pop(io,m,exit[3]);
    gp[b::esp]=m.flags.plus(gp[b::esp],8u,4);
    pop(io,m,exit[4]);
}

// 243F20 (EDX = part, ECX = point output, EAX = bounds, EDI = node output or
// 0, [ESP+4] = vertex index). SUB ESP,C makes the three coordinate spill
// slots ([ESP+C], +10, +14 after the saves). Exit registers by layout:
//   other         EAX = y, EDX = z
//   uncompressed  EAX = z, EDX = y
//   with node     EAX = the node byte (node output) or the vertex address, EDX = z
//   compressed    EAX = z, EDX = y; XMM0 = z, XMM1/XMM2 = the y/z ranges,
//                 XMM3..XMM6 = the constants, XMM7 = x over the caller's
//                 upper lanes (CVTSI2SS); with node, ECX = the node byte
//                 when there is a node output.
// EBX/EBP/ESI restored. Flags: the final ADD ESP,C. RET 4.
template<class Io,class Flags> void kernel_243f20(Io &io,Machine<Flags> &m) {
    namespace b=live_support_boundary;using namespace native_live_support::vertex_position;
    auto &gp=m.gp;auto &x=m.x;auto memory=b::memory(io,b::vertex_site);
    gp[b::esp]-=0xcu;
    push(io,m,0x243f26u,gp[b::ebx]);push(io,m,0x243f2au,gp[b::ebp]);push(io,m,0x243f2fu,gp[b::esi]);
    const auto index=io.rd(0x243f30u,gp[b::esp]+0x1cu,4);
    struct Frame {
        Io &io;std::uint32_t esp;
        std::uint32_t spill(Layout layout,unsigned axis,float value) {
            const auto spill=b::vertex_spill(layout,axis);
            io.st(spill.store,esp+spill.slot,4,b::float_bits(value));
            return io.rd(spill.reload,esp+spill.slot,4);
        }
    } frame{io,gp[b::esp]};
    const auto result=read(memory,frame,gp[b::edx],index,gp[b::eax],gp[b::ecx],gp[b::edi]);
    const auto lane=[](std::uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int(bits)));};
    const auto &point=result.point;
    std::array<std::uint32_t,4> exit{0x244173u,0x244174u,0x244175u,0x244179u};  // ESI, EBP, EBX and RET pops
    switch(result.layout) {
    case Layout::other:gp[b::eax]=point[1];gp[b::edx]=point[2];break;
    case Layout::uncompressed:gp[b::eax]=point[2];gp[b::edx]=point[1];break;
    case Layout::uncompressed_with_node:
        gp[b::eax]=result.node_stored?result.node:result.residue.element;gp[b::edx]=point[2];
        if(result.node_stored)exit={0x244065u,0x244066u,0x244069u,0x24406du};
        break;
    default: {
        const auto &r=result.residue;
        gp[b::eax]=point[2];gp[b::edx]=point[1];
        x[0]=lane(point[2]);x[1]=_mm_set_ss(r.range[1]);x[2]=_mm_set_ss(r.range[2]);
        x[3]=_mm_set_ss(r.constants.two);x[4]=_mm_set_ss(r.constants.one);x[5]=_mm_set_ss(r.constants.step);x[6]=_mm_set_ss(r.constants.half);
        x[7]=_mm_move_ss(x[7],lane(point[0]));
        if(result.layout==Layout::compressed_with_node && result.node_stored) {
            gp[b::ecx]=result.node;exit={0x244148u,0x244149u,0x24414cu,0x244150u};
        }
        break;
    }
    }
    gp[b::esi]=pop(io,m,exit[0]);gp[b::ebp]=pop(io,m,exit[1]);gp[b::ebx]=pop(io,m,exit[2]);
    gp[b::esp]=m.flags.plus(gp[b::esp],0xcu,4);
    pop(io,m,exit[3]);gp[b::esp]+=4u;
}

// 32CC0C (_ftol2: the value in ST(0), which it pops). PUSH EBP / MOV EBP,ESP
// / SUB ESP,20 / AND ESP,-10 make a 16-aligned frame: the sign slot at +18,
// the integer at +10, the difference at +0. EDX:EAX = the integer; ECX = the
// biased difference bits (unchanged for zero); EBP restored. Flags: the final
// ADC/SBB EDX,0, or TEST EDX,7FFFFFFF for zero. RET 0.
// x87: the value is popped (TOP + 1). C1 follows the caller unit's x87
// model: StoresSetC1 keeps the hardware's C1 of the last store; otherwise
// C1 stays as the FILD reload cleared it. A caller with no value on the
// stack, or no free register for the routine's FLD ST(0), stops with the
// caller's x87 stack-fault exception, as the translated body does.
template<bool StoresSetC1,class Io,class Flags> void kernel_32cc0c(Io &io,Machine<Flags> &m) {
    namespace b=live_support_boundary;using namespace native_live_support::float_to_int64;
    auto &gp=m.gp;auto &stack=m.x87;
    push(io,m,0x32cc0cu,gp[b::ebp]);
    gp[b::ebp]=gp[b::esp];
    gp[b::esp]=(gp[b::esp]-0x20u)&0xfffffff0u;
    io.at(0x32cc15u);
    if(stack.count==0 || stack.count==8)throw std::runtime_error("Required original solver x87 stack-fault owner unavailable");
    auto memory=b::memory(io,b::ftol_site);
    const auto result=convert(memory,Slots{gp[b::esp]+0x18u,gp[b::esp]+0x10u,gp[b::esp]},stack.lanes[0]);
    // Pop the value.
    for(unsigned i=1;i<stack.count;++i)stack.lanes[i-1]=stack.lanes[i];
    --stack.count;stack.top=(stack.top+1u)&7u;
    stack.status=std::uint16_t((stack.status&~0x200u)|(StoresSetC1 && result.last_store_rounded_up?0x200u:0u));
    m.x87_stack_owned=true;
    gp[b::eax]=std::uint32_t(result.value);gp[b::edx]=std::uint32_t(result.value>>32);
    // ADC/SBB EAX,0 then EDX,0 carry the step through the low dword; the
    // flags are those of the high dword's ADC/SBB, i.e. of high +/- carry.
    const auto low=std::uint32_t(result.value);
    switch(result.path) {
    case Path::zero_or_indefinite:m.flags.logic(result.high&0x7fffffffu,4);break;
    case Path::negative:gp[b::ecx]=result.biased_difference;m.flags.plus(result.high,result.step && low==0u,4);break;
    case Path::positive:gp[b::ecx]=result.biased_difference;m.flags.minus(result.high,result.step && low==0xffffffffu,4);break;
    }
    gp[b::esp]=gp[b::ebp];gp[b::ebp]=pop(io,m,0x32cc7fu);
    pop(io,m,0x32cc80u);
}

// 32A4E0, strncpy (cdecl: [ESP+4] = destination, [ESP+8] = source,
// [ESP+C] = count; RET 0). EAX = the destination, re-read from the stack;
// ECX = 0; EDX = the last dword copied by the dword loop (else the caller's).
// EBX/ESI/EDI saved (EDI alone for a zero count) and restored. Flags: the
// SUB of the count that reached zero, or the TEST/AND that found it zero.
template<class Io,class Flags> void kernel_32a4e0(Io &io,Machine<Flags> &m) {
    namespace b=live_support_boundary;using namespace native_live_support::bounded_string_copy;
    auto &gp=m.gp;
    const auto count=io.rd(0x32a4e0u,gp[b::esp]+0xcu,4);
    push(io,m,0x32a4e4u,gp[b::edi]);
    if(!count) {
        m.flags.logic(0u,4);gp[b::ecx]=0;
        gp[b::eax]=io.rd(0x32a57fu,gp[b::esp]+8u,4);gp[b::edi]=pop(io,m,0x32a583u);pop(io,m,0x32a584u);return;
    }
    push(io,m,0x32a4edu,gp[b::esi]);push(io,m,0x32a4eeu,gp[b::ebx]);
    const auto source=io.rd(0x32a4f1u,gp[b::esp]+0x14u,4);
    const auto destination=io.rd(0x32a4fbu,gp[b::esp]+0x10u,4);
    auto memory=b::memory(io,b::strncpy_site);
    const auto result=copy(memory,destination,source,count);
    gp[b::ecx]=0;
    if(result.residue.last_word)gp[b::edx]=*result.residue.last_word;
    if(result.residue.counted_down)m.flags.minus(1u,1u,4);else m.flags.logic(0u,4);
    switch(result.exit) {
    case Exit::copied:
        gp[b::eax]=io.rd(0x32a546u,gp[b::esp]+0x10u,4);
        gp[b::ebx]=pop(io,m,0x32a54au);gp[b::esi]=pop(io,m,0x32a54bu);gp[b::edi]=pop(io,m,0x32a54cu);pop(io,m,0x32a54du);return;
    case Exit::filled:
        gp[b::eax]=io.rd(0x32a5fcu,gp[b::esp]+0x10u,4);
        gp[b::ebx]=pop(io,m,0x32a600u);gp[b::esi]=pop(io,m,0x32a601u);gp[b::edi]=pop(io,m,0x32a602u);pop(io,m,0x32a603u);return;
    default:
        gp[b::ebx]=pop(io,m,0x32a57du);gp[b::esi]=pop(io,m,0x32a57eu);
        gp[b::eax]=io.rd(0x32a57fu,gp[b::esp]+8u,4);gp[b::edi]=pop(io,m,0x32a583u);pop(io,m,0x32a584u);return;
    }
}

// 16E600 (ESI = data array). EAX = 0; ECX = 0; EDX = the bit vector's byte
// count; EBX/ESI/EBP kept, EDI restored. The strncpy call (salt, name, 2) is
// 32A4E0's kernel, entered after the original's argument pushes and CALL.
// Flags: the final AND ECX,3. RET 0.
template<class Io,class Flags> void kernel_16e600(Io &io,Machine<Flags> &m) {
    namespace b=live_support_boundary;using namespace native_live_support::data_array_reset;
    auto &gp=m.gp;const auto array=gp[b::esi];
    push(io,m,0x16e600u,gp[b::edi]);push(io,m,0x16e601u,salt_characters);
    const auto salt=array+native_live_support::data_array::next_identifier;
    push(io,m,0x16e606u,array);push(io,m,0x16e607u,salt);
    b::StringMemory<Io,decltype(&b::reset_site),decltype(&b::reset_tail_site)> memory{{io,&b::reset_site},&b::reset_tail_site,
        (m.other_flags&0x400u)!=0};
    struct Frame {
        Io &io;Machine<Flags> &m;
        // strncpy(salt, name, 2): its arguments are pushed at entry; EDI holds
        // the salt's address (LEA EDI,[ESI+40]) at the call.
        void copy_name(std::uint32_t destination,std::uint32_t,std::uint32_t) {
            m.gp[b::edi]=destination;push(io,m,0x16e61du,0x16e622u);kernel_32a4e0(io,m);
        }
    } frame{io,m};
    const auto result=delete_all(memory,frame,array);
    gp[b::esp]+=12u;
    gp[b::eax]=0;gp[b::ecx]=0;gp[b::edx]=result.bit_vector_bytes;
    m.flags.logic(result.bit_vector_bytes&3u,4);
    gp[b::edi]=pop(io,m,0x16e696u);
    pop(io,m,0x16e697u);
}

// 16E4B0 (ESI = data array; [ESP+4] name, +8 maximum count, +C datum size,
// +10 alignment bits (a byte), +14 allocation, +18 in-use bit vector). The
// header clear is REP STOSD (EFLAGS.DF honoured); strncpy is 32A4E0's
// kernel, after the original's pushes (count 32, name, header) and CALL.
// EAX = allocation; ECX = maximum count with CL = alignment bits; EDX = the
// bit vector; EDI restored. Flags: ADD ESP,C (the strncpy arguments). RET 18.
template<class Io,class Flags> void kernel_16e4b0(Io &io,Machine<Flags> &m) {
    namespace b=live_support_boundary;using namespace native_live_support::data_array_setup;
    auto &gp=m.gp;const auto array=gp[b::esi];
    push(io,m,0x16e4b0u,gp[b::edi]);
    const auto saved=gp[b::esp];
    const bool descending=(m.other_flags&0x400u)!=0;
    b::StringMemory<Io,decltype(&b::setup_site),decltype(&b::setup_tail_site)> memory{{io,&b::setup_site},&b::setup_tail_site,descending};
    struct Frame {
        Io &io;Machine<Flags> &m;std::uint32_t saved;bool descending;
        std::uint32_t argument(Argument a) {
            switch(a) {
            case Argument::name:return io.rd(0x16e4bcu,saved+8u,4);
            case Argument::maximum_count:return io.rd(0x16e4c9u,saved+0xcu,4);
            case Argument::datum_size:return io.rd(0x16e4cdu,saved+0x10u,4);
            case Argument::alignment_bits:return io.rd(0x16e4ddu,saved+0x14u,1);
            case Argument::allocation:return io.rd(0x16e4eeu,saved+0x18u,4);
            case Argument::in_use_bits:return io.rd(0x16e4e4u,saved+0x1cu,4);
            }
            return 0;
        }
        void copy_name(std::uint32_t destination,std::uint32_t source,std::uint32_t count) {
            // After the clear: EDI past the header, ECX = 0, EAX = 0 then the name.
            m.gp[b::edi]=destination+(descending?0u-native_live_support::data_array::header_size:native_live_support::data_array::header_size);
            m.gp[b::ecx]=0;m.gp[b::eax]=source;
            push(io,m,0x16e4c0u,count);push(io,m,0x16e4c2u,source);push(io,m,0x16e4c3u,destination);
            push(io,m,0x16e4c4u,0x16e4c9u);kernel_32a4e0(io,m);
        }
    } frame{io,m,saved,descending};
    const auto header=initialize_header(memory,frame,array);
    gp[b::eax]=header.allocation;gp[b::ecx]=(header.maximum_count&~0xffu)|header.alignment_bits;gp[b::edx]=header.in_use_bits;
    gp[b::esp]=m.flags.plus(gp[b::esp],0xcu,4);
    gp[b::edi]=pop(io,m,0x16e505u);
    pop(io,m,0x16e506u);gp[b::esp]+=0x18u;
}

// 16E450 (CL = alignment bits, EBX = maximum count, EDX = datum size;
// [ESP+4] = the block (the data array), +8 name, +C allocation). The header
// call is 16E4B0's kernel after the original's six pushes and CALL. The bit
// vector clear is REP STOSD then REP STOSB (EFLAGS.DF honoured). EAX = 0,
// ECX = 0, EDX = the bit vector's byte count; ESI/EDI restored. Flags: the
// final AND ECX,3. RET C.
template<class Io,class Flags> void kernel_16e450(Io &io,Machine<Flags> &m) {
    namespace b=live_support_boundary;using namespace native_live_support::data_array_setup;
    auto &gp=m.gp;
    push(io,m,0x16e450u,gp[b::esi]);
    gp[b::esi]=io.rd(0x16e451u,gp[b::esp]+8u,4);
    push(io,m,0x16e455u,gp[b::edi]);
    b::StringMemory<Io,decltype(&b::setup_site),decltype(&b::setup_tail_site)> memory{{io,&b::setup_site},&b::setup_tail_site,
        (m.other_flags&0x400u)!=0};
    struct Frame {
        Io &io;Machine<Flags> &m;std::uint32_t mask;
        void initialize_header(std::uint32_t in_use_bits) {
            auto &gp=m.gp;
            // The registers at the call: EAX = the bit vector, EDI = the data.
            gp[b::edi]=(gp[b::esi]+native_live_support::data_array::header_size+mask)&~mask;
            gp[b::eax]=in_use_bits;
            push(io,m,0x16e46du,in_use_bits);
            gp[b::eax]=io.rd(0x16e46eu,gp[b::esp]+0x18u,4);
            push(io,m,0x16e472u,gp[b::eax]);push(io,m,0x16e473u,gp[b::ecx]);
            gp[b::ecx]=io.rd(0x16e474u,gp[b::esp]+0x1cu,4);
            push(io,m,0x16e478u,gp[b::edx]);push(io,m,0x16e479u,gp[b::ebx]);push(io,m,0x16e47au,gp[b::ecx]);
            push(io,m,0x16e47bu,0x16e480u);kernel_16e4b0(io,m);
        }
    } frame{io,m,(1u<<(gp[b::ecx]&31u))-1u};
    const auto placed=initialize(memory,frame,gp[b::esi],gp[b::ebx],gp[b::edx],gp[b::ecx]&0xffu);
    gp[b::eax]=0;gp[b::ecx]=0;gp[b::edx]=placed.bit_vector_bytes;
    m.flags.logic(placed.bit_vector_bytes&3u,4);
    gp[b::edi]=pop(io,m,0x16e4a3u);gp[b::esi]=pop(io,m,0x16e4a4u);
    pop(io,m,0x16e4a5u);gp[b::esp]+=0xcu;
}

// 171010 (EAX = offset to clear, EBX = table, ECX = key; [ESP+4] = index
// output, [ESP+8] = offset output). On the -1 exit ECX = the index pointer
// and EDX is as last loaded (a first value, or the caller's EDX); on the
// index exit EDX = the index pointer and ECX = the index. EAX = the offset
// pointer after a match, else the record reached (the caller's EAX for an
// empty table). ESI, EDI and (saved for a non-empty table) EBP restored.
// Flags: the final CMP of the index with the re-read count. RET 8.
template<class Io,class Flags> void kernel_171010(Io &io,Machine<Flags> &m) {
    namespace b=live_support_boundary;using namespace native_live_support::interval_table;
    auto &gp=m.gp;auto memory=b::memory(io,b::interval_site);
    const auto entry=gp[b::esp];
    push(io,m,0x171010u,gp[b::esi]);push(io,m,0x171011u,gp[b::edi]);
    struct Frame {
        Io &io;Machine<Flags> &m;std::uint32_t entry;bool saved{};std::uint32_t index_pointer{};
        void save_loop_register(){push(io,m,0x171025u,m.gp[b::ebp]);saved=true;}
        std::uint32_t offset_output(){return io.rd(0x171046u,entry+8u,4);}
        std::uint32_t index_output(bool missing){return index_pointer=io.rd(missing?0x171055u:0x171062u,entry+4u,4);}
    } frame{io,m,entry};
    const auto result=find(memory,frame,gp[b::ebx],gp[b::ecx],gp[b::eax]);
    const bool missing=result.index==result.count_recheck;
    if(result.matched)gp[b::eax]=result.residue.offset_output;
    else if(frame.saved)gp[b::eax]=result.residue.record;
    if(result.residue.last_first)gp[b::edx]=*result.residue.last_first;
    if(missing)gp[b::ecx]=frame.index_pointer;
    else {gp[b::edx]=frame.index_pointer;gp[b::ecx]=result.index;}
    m.flags.minus(result.index,result.count_recheck,4);
    if(frame.saved)gp[b::ebp]=pop(io,m,0x17104eu);
    gp[b::edi]=pop(io,m,0x171051u);gp[b::esi]=pop(io,m,0x171052u);
    pop(io,m,missing?0x17105fu:0x171068u);gp[b::esp]+=8u;
}

// 1211C0 (ESI = point, EAX = v0, ECX = v1, EDX = v2; [ESP+4] = u output,
// [ESP+8] = v output). SUB ESP,30 makes the local block at [ESP]. AL = on
// the triangle. The dominant axis is the 123210 leaf, called with EAX = the
// normal (its kernel leaves XMM0/XMM1, the flags and x87 C1 as the original).
// Afterwards EAX = u*4 and ECX = v*4 (the projection's byte offsets); a hit
// leaves EDX = the u pointer and EAX = the v pointer. XMM0..XMM7 hold the
// intermediates of the exit taken (upper lanes zero: every value starts as a
// scalar load). Flags: the final ADD ESP,30. RET 8.
template<class Io,class Flags> void kernel_1211c0(Io &io,Machine<Flags> &m) {
    namespace b=live_support_boundary;using namespace native_live_support::triangle_point;
    auto &gp=m.gp;auto &x=m.x;auto memory=b::memory(io,b::triangle_site);
    gp[b::esp]-=0x30u;
    struct Frame {
        Io &io;Machine<Flags> &m;
        std::uint32_t locals()const{return m.gp[b::esp];}
        std::uint32_t dominant_axis(std::uint32_t normal) {
            m.gp[b::eax]=normal;push(io,m,0x1212e3u,0x1212e8u);kernel_123210(io,m);
            return std::uint32_t(std::int32_t(std::int16_t(m.gp[b::eax])));
        }
        std::uint32_t u_output(){return io.rd(0x1213b8u,m.gp[b::esp]+0x34u,4);}
        std::uint32_t v_output(){return io.rd(0x1213bcu,m.gp[b::esp]+0x38u,4);}
    } frame{io,m};
    const auto r=test(memory,frame,gp[b::esi],gp[b::eax],gp[b::ecx],gp[b::edx]);
    const auto lane=[](float value){return _mm_set_ss(value);};
    if(r.path==Path::off_plane) {
        x[0]=lane(r.normal_x_squared);x[1]=lane(r.normal_y_squared);x[2]=lane(r.plane_limit);x[3]=lane(r.dot_squared);
        x[4]=lane(r.normal[0]);x[5]=lane(r.edge2_x);x[6]=lane(r.edge1_x);x[7]=lane(r.edge1_y_edge2_z);
    } else {
        gp[b::eax]=std::uint32_t(r.u)*4u;gp[b::ecx]=std::uint32_t(r.v)*4u;
        x[2]=lane(r.b2);x[6]=lane(r.offset_u);x[7]=lane(r.eps);
        switch(r.path) {
        case Path::outside_edge2:  // XMM1 as the dominant-axis call left it
            x[0]=lane(r.edge2_v_offset_u);x[3]=lane(r.offset_v);x[4]=lane(r.edge2_v);x[5]=lane(r.edge2_x);break;
        case Path::outside_edge1:
            x[0]=lane(r.edge1_v);x[1]=lane(r.b1);x[3]=lane(r.offset_v_edge1_u);x[4]=lane(r.edge2_v);x[5]=lane(r.edge1_u);break;
        case Path::degenerate:
            x[0]=lane(r.det);x[1]=lane(r.b1);x[3]=lane(r.offset_v_edge1_u);x[4]=lane(r.edge2_v_edge1_u);x[5]=lane(r.edge1_u);break;
        case Path::outside_sum:
            x[0]=lane(r.det);x[1]=lane(r.b1);x[3]=lane(r.sum);x[4]=lane(r.det_limit);x[5]=lane(r.edge1_u);break;
        default:
            x[0]=lane(r.u_coordinate);x[1]=lane(r.b1);x[3]=lane(r.v_coordinate);x[4]=lane(r.det_limit);x[5]=lane(r.edge1_u);
            gp[b::edx]=r.u_output;gp[b::eax]=r.v_output;
            break;
        }
    }
    b::set_low_byte(gp[b::eax],r.path==Path::inside);
    gp[b::esp]=m.flags.plus(gp[b::esp],0x30u,4);
    pop(io,m,r.path==Path::inside?0x1213dcu:0x1213e4u);gp[b::esp]+=8u;
}

// 1F8200 (stdcall, RET 14: [ESP+4] the model instance, +8 the BSP test's
// flags, +C the ray origin, +10 the ray vector, +14 the result; a leaf with
// children). SUB ESP,98 and the four saves (EBX, EBP, ESI, EDI) make the
// workspace at the final ESP. Through the walk EBX = the BSP's position,
// ESI = its node (then the iterator for each 1F7F60 call), EDI = the result's
// fraction slot and EBP = the result; the load hooks keep the registers the
// original loads that its children see (the initial fraction's MOVSS into
// XMM0, the position into EBX, the node matrices into EDX). 1F7E90 and
// 1F7F60 run inline (their kernels, after the original's pushes and CALL).
// The children:
//   144090: EAX = the local matrix, ECX = the node's matrix (flags of ADD
//     ECX,EDX), EDX = the node matrices;
//   1E6980: its four arguments pushed (query, root node, 0, limit), EAX = 0,
//     ECX = the limit's bits, EDX = the query; XMM0..XMM7 as TestRegisters,
//     the flags of the last COMISS.
// Exit: AL = the found byte, re-read from the frame (over 1F7F60's EAX);
// ECX/EDX as 1F7F60 left them; XMM as the last child left them (XMM0 = the
// initial fraction when there is no BSP). Flags: ADD ESP,98. RET 14.
template<class Io,class Flags,class Invert,class Test>
void kernel_1f8200(Io &io,Machine<Flags> &m,Invert &&invert_route,Test &&test_route) {
    namespace b=live_support_boundary;using namespace native_live_support::collision_model_ray;
    auto &gp=m.gp;
    gp[b::esp]=m.flags.minus(gp[b::esp],0x98u,4);
    const auto hook=[&m](Access a,std::uint32_t value) {
        switch(a) {
        case Access::initial_fraction:m.x[0]=_mm_castsi128_ps(_mm_cvtsi32_si128(int(value)));break;
        case Access::position:m.gp[b::ebx]=value;break;
        case Access::node_matrices:m.gp[b::edx]=value;break;
        default:break;
        }
    };
    b::LoadingMemory<Io,decltype(&b::ray_site),decltype(hook)> memory{{io,&b::ray_site},hook};
    struct Frame {
        Io &io;Machine<Flags> &m;Invert &invert_route;Test &test_route;bool stepped{};
        std::uint32_t instance(){return io.rd(0x1f8206u,m.gp[b::esp]+0x9cu,4);}
        std::uint32_t result() {
            auto &gp=m.gp;
            push(io,m,0x1f8215u,gp[b::ebx]);push(io,m,0x1f8216u,gp[b::ebp]);
            gp[b::ebp]=io.rd(0x1f8217u,gp[b::esp]+0xb4u,4);
            push(io,m,0x1f821eu,gp[b::esi]);push(io,m,0x1f821fu,gp[b::edi]);
            gp[b::edi]=gp[b::ebp]+result::bsp_output;
            return gp[b::ebp];
        }
        std::uint32_t workspace()const{return m.gp[b::esp];}
        std::uint32_t argument(Argument a) {
            const auto esp=m.gp[b::esp];
            switch(a) {
            case Argument::instance:return io.rd(0x1f8267u,esp+0xacu,4);
            case Argument::flags:return io.rd(0x1f83a1u,esp+0xb0u,4);
            case Argument::origin:return io.rd(0x1f8289u,esp+0xb4u,4);
            case Argument::vector:return io.rd(0x1f82e1u,esp+0xb8u,4);
            }
            return 0;
        }
        void begin_iteration(std::uint32_t iterator,std::uint32_t model,std::uint32_t selection,std::uint32_t remap) {
            auto &gp=m.gp;
            gp[b::ecx]=remap;gp[b::ebx]=selection;gp[b::eax]=model;
            push(io,m,0x1f8231u,remap);
            gp[b::edx]=iterator;push(io,m,0x1f8239u,iterator);
            io.st(0x1f823au,gp[b::esp]+0x1bu,1,0u);  // the found byte (workspace +13)
            push(io,m,0x1f823fu,0x1f8244u);kernel_1f7e90(io,m);
        }
        bool next(std::uint32_t iterator) {
            m.gp[b::esi]=iterator;
            push(io,m,stepped?0x1f84cdu:0x1f824bu,stepped?0x1f84d2u:0x1f8250u);stepped=true;
            kernel_1f7f60(io,m);
            return m.flags.logic(m.gp[b::eax],1)!=0;  // TEST AL,AL
        }
        void invert(std::uint32_t node,std::uint32_t matrices,std::uint32_t local) {
            auto &gp=m.gp;
            gp[b::esi]=node;gp[b::ecx]=m.flags.plus(node*matrix::size,matrices,4);gp[b::eax]=local;
            push(io,m,0x1f8284u,0x1f8289u);io.call(m,0x144090u,invert_route);
        }
        bool test(std::uint32_t query,std::uint32_t limit,const TestRegisters &r) {
            auto &gp=m.gp;auto &x=m.x;
            const auto lane=[](float value){return _mm_set_ss(value);};
            x[0]=lane(r.fraction);x[1]=lane(r.bound);x[2]=lane(r.floor);
            for(unsigned i=0;i<3;++i)x[3+i]=lane(r.vector[i]);
            x[6]=lane(r.vector_z_up_x);x[7]=lane(r.local_origin_z);
            // The last COMISS, re-executed (its MXCSR effect is idempotent).
            if(r.below_zero)m.flags.scalar_compare(x[1],x[0],false);else m.flags.scalar_compare(x[0],x[1],false);
            gp[b::eax]=0;gp[b::ecx]=limit;
            push(io,m,0x1f849cu,limit);push(io,m,0x1f849du,0u);push(io,m,0x1f849eu,0u);
            gp[b::edx]=query;push(io,m,0x1f84a3u,query);
            push(io,m,0x1f84a4u,0x1f84a9u);io.call(m,0x1e6980u,test_route);
            return m.flags.logic(gp[b::eax],1)!=0;  // TEST AL,AL
        }
        void mark_found(){io.st(0x1f84c1u,m.gp[b::esp]+0x13u,1,1u);}
    } frame{io,m,invert_route,test_route};
    cast(memory,frame);
    b::set_low_byte(gp[b::eax],io.rd(0x1f84dau,gp[b::esp]+0x13u,1));
    gp[b::edi]=pop(io,m,0x1f84deu);gp[b::esi]=pop(io,m,0x1f84dfu);gp[b::ebp]=pop(io,m,0x1f84e0u);gp[b::ebx]=pop(io,m,0x1f84e1u);
    gp[b::esp]=m.flags.plus(gp[b::esp],0x98u,4);
    pop(io,m,0x1f84e8u);gp[b::esp]+=0x14u;
}

// 138D90 (EAX = the bitmap, XMM1 = the level of detail, [ESP+4] = the
// coordinate; RET 4; a leaf with children). SUB ESP,C makes the frame slots:
// from the entry ESP, -C the height, -8 the width (first the level's spill),
// -4 the x87 conversions' integer; the coordinate argument's slot (+4) takes
// the coordinates' spills. EDI = the bitmap. EDI is saved at entry, EBX, EBP
// and ESI after the constant 1.0 is read (the access hook places them).
// At the level-offset child: EAX = the level (2D), 0 (3D, with ECX = the
// bitmap) or the type - 1 (cube), otherwise ECX = the height sign-extended;
// EBX = x, ESI = y, EBP = the level, EDX = the y wrap's last value; the
// flags of SUB EAX,0 or DEC EAX; XMM0 = 0.5 and XMM1's low lane the v spill
// over the caller's upper lanes. The texel-address child receives its seven
// arguments as the original reads and pushes them, ECX = y and EDX = the
// width slot, the flags of XOR EDX,EDX. x87: each FLD/FISTP pair clears C1
// and, under StoresSetC1 (the caller unit's model), leaves the FISTP's C1; a
// full x87 stack faults at the FLD, as the translated body.
// Exit: EAX (ECX, EDX) = the address child's, or EAX = -1 without pixels;
// EBX/EBP/ESI/EDI restored; flags of ADD ESP,C. RET 4.
template<bool StoresSetC1,class Io,class Flags,class Offset2d,class Offset3d,class OffsetCube,class Address>
void kernel_138d90(Io &io,Machine<Flags> &m,Offset2d &&offset_2d,Offset3d &&offset_3d,OffsetCube &&offset_cube,Address &&address_route) {
    namespace b=live_support_boundary;using namespace native_live_support::bitmap_texel;
    auto &gp=m.gp;
    const auto entry=gp[b::esp];
    gp[b::esp]=m.flags.minus(gp[b::esp],0xcu,4);
    push(io,m,0x138d93u,gp[b::edi]);
    gp[b::edi]=gp[b::eax];
    const auto before=[](Access){};
    const auto after=[&](Access a) {
        if(a==Access::one){push(io,m,0x138dacu,gp[b::ebx]);push(io,m,0x138dadu,gp[b::ebp]);push(io,m,0x138daeu,gp[b::esi]);}
    };
    b::OrderedMemory<Io,decltype(&b::texel_site),decltype(before),decltype(after)> memory{{io,&b::texel_site},before,after};
    struct Frame {
        Io &io;Machine<Flags> &m;std::uint32_t entry;
        Offset2d &offset_2d;Offset3d &offset_3d;OffsetCube &offset_cube;Address &address_route;
        Slots slots()const{return {entry-0xcu,entry-8u,entry-4u,entry+4u};}
        void x87_push() {
            if(m.x87.count==8)throw std::runtime_error("Required original solver x87 stack-fault owner unavailable");
            m.x87.status=std::uint16_t(m.x87.status&~0x200u);m.x87_stack_owned=true;
        }
        void x87_pop(bool rounded_up) {
            if constexpr(StoresSetC1)m.x87.status=std::uint16_t((m.x87.status&~0x200u)|(rounded_up?0x200u:0u));
        }
        std::uint32_t level_offset(std::int32_t type,std::uint32_t level,std::uint32_t x,std::uint32_t y,const Residue &r) {
            auto &gp=m.gp;
            gp[b::ebp]=level;gp[b::ebx]=x;gp[b::esi]=y;gp[b::ecx]=r.y_divisor;gp[b::edx]=r.y_wrap;
            m.x[0]=_mm_castsi128_ps(_mm_cvtsi32_si128(int(r.half)));
            m.x[1]=_mm_move_ss(m.x[1],_mm_castsi128_ps(_mm_cvtsi32_si128(int(r.v_spill))));
            gp[b::eax]=m.flags.minus(std::uint32_t(type),0u,4);  // SUB EAX,0
            if(type==std::int32_t(Type::texture_2d)) {
                push(io,m,0x138f1eu,0u);push(io,m,0x138f20u,0u);gp[b::eax]=level;
                push(io,m,0x138f24u,0x138f29u);io.call(m,0x138510u,offset_2d);
                return gp[b::eax];
            }
            gp[b::eax]=m.flags.increment(gp[b::eax],4,true);  // DEC EAX
            push(io,m,0x138f05u,level);push(io,m,0x138f06u,0u);push(io,m,0x138f08u,0u);push(io,m,0x138f0au,0u);
            if(type==std::int32_t(Type::texture_3d)){gp[b::ecx]=gp[b::edi];push(io,m,0x138f17u,0x138f1cu);io.call(m,0x1385d0u,offset_3d);}
            else {push(io,m,0x138f0eu,0x138f13u);io.call(m,0x1386e0u,offset_cube);}
            return gp[b::eax];
        }
        std::uint32_t texel_address(std::uint32_t bitmap_address,std::uint32_t offset,std::uint32_t x,std::uint32_t y) {
            auto &gp=m.gp;
            m.flags.logic(0u,4);gp[b::ecx]=io.rd(0x138f2bu,bitmap_address+bitmap::flags,2);
            m.flags.logic(0u,4);gp[b::edx]=io.rd(0x138f31u,bitmap_address+bitmap::format,2);
            push(io,m,0x138f35u,gp[b::ecx]);
            gp[b::ecx]=io.rd(0x138f36u,entry-0xcu,4);
            push(io,m,0x138f3au,gp[b::edx]);
            gp[b::edx]=io.rd(0x138f3bu,bitmap_address+bitmap::partition_size,4);
            push(io,m,0x138f3eu,gp[b::ecx]);push(io,m,0x138f3fu,x);push(io,m,0x138f40u,offset);
            gp[b::eax]=io.rd(0x138f41u,bitmap_address+bitmap::pixels,4);
            push(io,m,0x138f44u,gp[b::edx]);
            gp[b::edx]=io.rd(0x138f45u,entry-8u,4);
            push(io,m,0x138f49u,gp[b::eax]);
            gp[b::ecx]=y;
            push(io,m,0x138f4cu,0x138f51u);io.call(m,0x138c20u,address_route);
            return gp[b::eax];
        }
    } frame{io,m,entry,offset_2d,offset_3d,offset_cube,address_route};
    if(!address(memory,frame,gp[b::edi],_mm_cvtss_f32(m.x[1])).resident) {
        gp[b::eax]=m.flags.logic(0xffffffffu,4);  // OR EAX,-1
        gp[b::edi]=pop(io,m,0x138f5eu);
        gp[b::esp]=m.flags.plus(gp[b::esp],0xcu,4);
        pop(io,m,0x138f62u);gp[b::esp]+=4u;return;
    }
    gp[b::esi]=pop(io,m,0x138f51u);gp[b::ebp]=pop(io,m,0x138f52u);gp[b::ebx]=pop(io,m,0x138f53u);gp[b::edi]=pop(io,m,0x138f54u);
    gp[b::esp]=m.flags.plus(gp[b::esp],0xcu,4);
    pop(io,m,0x138f58u);gp[b::esp]+=4u;
}

// 179CD0 (no arguments; RET 0; a leaf with children). PUSH ECX reserves the
// time step's slot ([entry-4]); EBX and ESI are saved after the time step is
// read (the load hook places them, and keeps EAX = the game time globals,
// XMM0 = the time step and ESI = the effects for the slot scan), EBP and EDI
// when the walk begins. The children:
//   16EA60: ECX = ESI = the effects, EDX = 0 (flags of XOR EDX,EDX);
//   17C370: [ESP+4] = the handle, EBX = the handle and the scratch registers
//     of Found (place), flags of CMP EBX,-1;
//   17C5C0: as 17C370 left them (EBX = the handle), flags of TEST AL,AL;
//   17A380: EAX = the handle, CL = the flags byte >> 6, EDI = the data, EBP =
//     the array, flags of the equal CMP of +FC with -1;
//   17C140: [ESP+4] = the handle, [ESP+8] = the time step (EDX, re-read from
//     its slot), EAX = the datum over AL = 0 (SETE), CL, EDI and EBP as for
//     17A380, flags of TEST AL,AL.
// Exit: EAX/EDX as the last search left them (place), ECX = the time step's
// slot, popped; EBX/ESI (and EBP/EDI) restored; flags of CMP EBX,-1. RET 0.
template<class Io,class Flags,class SlotScan,class Attachments,class Visibility,class Destroy,class Advance>
void kernel_179cd0(Io &io,Machine<Flags> &m,SlotScan &&slot_scan,Attachments &&attachments_route,Visibility &&visibility_route,
    Destroy &&destroy_route,Advance &&advance_route) {
    namespace b=live_support_boundary;using namespace native_live_support::effect_update;
    auto &gp=m.gp;
    const auto entry=gp[b::esp];
    push(io,m,0x179cd0u,gp[b::ecx]);
    const auto hook=[&](Access a,std::uint32_t value) {
        switch(a) {
        case Access::game_time:gp[b::eax]=value;break;
        case Access::time_step:
            m.x[0]=_mm_castsi128_ps(_mm_cvtsi32_si128(int(value)));
            push(io,m,0x179cdbu,gp[b::ebx]);push(io,m,0x179cdcu,gp[b::esi]);break;
        case Access::effects:gp[b::esi]=value;break;
        default:break;
        }
    };
    b::LoadingMemory<Io,decltype(&b::effect_update_site),decltype(hook)> memory{{io,&b::effect_update_site},hook};
    struct Frame {
        Io &io;Machine<Flags> &m;std::uint32_t entry;bool walked{};
        SlotScan &slot_scan;Attachments &attachments_route;Visibility &visibility_route;Destroy &destroy_route;Advance &advance_route;
        void keep_time_step(std::uint32_t step){io.st(0x179ce7u,entry-4u,4,step);}
        std::uint32_t first_in_use(std::uint32_t effects) {
            auto &gp=m.gp;
            gp[b::edx]=m.flags.logic(0u,4);gp[b::ecx]=effects;
            push(io,m,0x179cedu,0x179cf2u);io.call(m,0x16ea60u,slot_scan);
            return gp[b::eax];
        }
        void begin_walk(){push(io,m,0x179d15u,m.gp[b::ebp]);push(io,m,0x179d16u,m.gp[b::edi]);walked=true;}
        // The scratch registers of a search (see Found).
        void place(const Found &f) {
            auto &gp=m.gp;
            if(f.inline_scan) {
                gp[b::esi]=f.scan.index.value_or(none);gp[b::edi]=f.scan.limit;gp[b::ebp]=f.array;gp[b::eax]=f.index;
                if(f.scan.bits)gp[b::edx]=*f.scan.bits;
                if(f.scan.index)gp[b::ecx]=std::uint32_t(std::int32_t(*f.scan.index)>>5);
                if(f.datum_offset){gp[b::eax]=*f.data;gp[b::edx]=*f.datum_offset;}
            } else if(f.datum_offset){gp[b::ecx]=*f.datum_offset;gp[b::edx]=*f.data;}
            gp[b::ebx]=f.handle;
            m.flags.minus(f.handle,none,4);
        }
        bool attachments(const Found &f) {
            place(f);
            push(io,m,0x179d17u,f.handle);push(io,m,0x179d18u,0x179d1du);io.call(m,0x17c370u,attachments_route);
            return m.flags.logic(m.gp[b::eax],1)!=0;  // TEST AL,AL
        }
        bool visibility(std::uint32_t) {
            push(io,m,0x179d21u,0x179d26u);io.call(m,0x17c5c0u,visibility_route);
            return m.flags.logic(m.gp[b::eax],1)!=0;
        }
        void checked_registers(const Checked &c) {
            auto &gp=m.gp;
            gp[b::ebp]=c.array;gp[b::edi]=c.data;gp[b::ecx]=(gp[b::ecx]&~0xffu)|(c.flags>>6);
        }
        void destroy(std::uint32_t handle,const Checked &c) {
            checked_registers(c);m.gp[b::eax]=handle;m.flags.minus(none,none,4);
            push(io,m,0x179d59u,0x179d5eu);io.call(m,0x17a380u,destroy_route);
        }
        void advance(std::uint32_t handle,const Checked &c) {
            auto &gp=m.gp;
            checked_registers(c);gp[b::eax]=c.datum&~0xffu;m.flags.logic(0u,1);
            gp[b::edx]=io.rd(0x179d69u,entry-4u,4);
            push(io,m,0x179d6du,gp[b::edx]);push(io,m,0x179d6eu,handle);
            push(io,m,0x179d6fu,0x179d74u);io.call(m,0x17c140u,advance_route);
        }
        void end(const Found &f) {
            auto &gp=m.gp;
            place(f);
            if(walked){gp[b::edi]=pop(io,m,0x179de2u);gp[b::ebp]=pop(io,m,0x179de3u);}
            gp[b::esi]=pop(io,m,0x179de4u);gp[b::ebx]=pop(io,m,0x179de5u);gp[b::ecx]=pop(io,m,0x179de6u);
            pop(io,m,0x179de7u);
        }
    } frame{io,m,entry,false,slot_scan,attachments_route,visibility_route,destroy_route,advance_route};
    update(memory,frame);
}
} // namespace halo2::native_tu15_leaves
