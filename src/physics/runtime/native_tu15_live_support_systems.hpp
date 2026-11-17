#pragma once
// Native TU1.5 live-support systems (group L of
// docs/native_replacement/REMAINING_TRANSLATED_20261115.md):
//
//   unit_animation_mode::listed       C9550  is the unit's animation mode one of five?
//   flagged_datum_iterator::advance   1EC970 next in-use datum of a data array
//   animation_channel_iterator::next  1F7F60 next populated channel entry
//   animation_channel_iterator::begin 1F7E90 start an iteration (owner, selection)
//   bit_vector::test                  C1E90  one bit of a bit vector
//   unit_level_block::update          110350 ramp/decay of a unit's level block
//   vertex_position::read             243F20 one (de)compressed vertex position
//   float_to_int64::convert           32CC0C x87 value -> int64, toward zero (_ftol2)
//   data_array_reset::delete_all      16E600 empty a data array (data_delete_all)
//   bounded_string_copy::copy         32A4E0 the C runtime's strncpy
//   data_array_setup::initialize      16E450 lay out a data array in a block (data_initialize)
//   data_array_setup::initialize_header 16E4B0 its header
//   resource_references::any_busy     22A790 is any referenced hardware resource busy
//   collision_model_ray::cast         1F8200 a ray against every collision BSP of a posed model
//   bitmap_texel::address             138D90 the address of a texel in a mip level of a bitmap
//   effect_update::update             179CD0 one tick of every live effect
//   interval_table::find              171010 the record whose interval holds a key
//   triangle_point::test              1211C0 is a point on a triangle; its barycentrics
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores. Every access
// names its role (each system's Access enum), and the boundary adapter
// (native_tu15_live_support_leaves.hpp) maps that role to the original
// instruction for the diagnostic cursor. Registers, flags, stack slots and
// partial-register results belong to that adapter, not to this file.
//
// What stays exact, because translated neighbours observe it:
//   - memory accesses happen in the original order (the store observer sees
//     every write, and a faulting read must be the original's first fault);
//   - single-precision arithmetic runs the original SSE instructions in the
//     original operand order (NaN payloads, rounding, MXCSR sticky flags);
//   - the few values the original leaves behind in caller-visible scratch
//     registers are returned in each result's Residue, for the adapter only.
//
// Field names describe what the code does with each field; where the game
// meaning is not established the name says so.
#include <bit>
#include <cstdint>
#include <optional>
#include <xmmintrin.h>

namespace halo2::native_live_support {

// ------------------------------------------------------------------ helpers
inline std::uint32_t widen16(std::uint32_t word){return std::uint32_t(std::int32_t(std::int16_t(std::uint16_t(word))));}
inline std::int32_t widen8(std::uint32_t byte){return std::int32_t(std::int8_t(std::uint8_t(byte)));}
inline std::int32_t signed_value(std::uint32_t x){return std::int32_t(x);}

// Single-precision operations exactly as the original SSE instructions:
// the first operand is the destination, so it owns a NaN payload, and the
// operation is not reordered or fused. Each acts on the scalar lane.
namespace sse {
inline float bits_to_float(std::uint32_t bits){return _mm_cvtss_f32(_mm_castsi128_ps(_mm_cvtsi32_si128(int(bits))));}
inline std::uint32_t float_to_bits(float x){return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(_mm_set_ss(x))));}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
inline float add(float a,float b){__m128 x=_mm_set_ss(a),y=_mm_set_ss(b);asm volatile("addss %1,%0":"+x"(x):"x"(y));return _mm_cvtss_f32(x);}
inline float subtract(float a,float b){__m128 x=_mm_set_ss(a),y=_mm_set_ss(b);asm volatile("subss %1,%0":"+x"(x):"x"(y));return _mm_cvtss_f32(x);}
inline float multiply(float a,float b){__m128 x=_mm_set_ss(a),y=_mm_set_ss(b);asm volatile("mulss %1,%0":"+x"(x):"x"(y));return _mm_cvtss_f32(x);}
inline float divide(float a,float b){__m128 x=_mm_set_ss(a),y=_mm_set_ss(b);asm volatile("divss %1,%0":"+x"(x):"x"(y));return _mm_cvtss_f32(x);}
inline float from_int(std::int32_t v){__m128 x=_mm_setzero_ps();asm volatile("cvtsi2ssl %1,%0":"+x"(x):"r"(v));return _mm_cvtss_f32(x);}
inline std::int32_t truncate(float a){std::int32_t v;asm volatile("cvttss2si %1,%0":"=r"(v):"x"(_mm_set_ss(a)));return v;}
// COMISS (signals on any NaN): a > b, ordered.
inline bool ordered_greater(float a,float b) {
    unsigned char below,equal;asm volatile("comiss %3,%2\n\tsetb %0\n\tsete %1":"=qm"(below),"=qm"(equal):"x"(_mm_set_ss(a)),"x"(_mm_set_ss(b)):"cc");
    return !below&&!equal;
}
// COMISS: a >= b, ordered (not below).
inline bool ordered_not_below(float a,float b) {
    unsigned char below;asm volatile("comiss %2,%1\n\tsetb %0":"=qm"(below):"x"(_mm_set_ss(a)),"x"(_mm_set_ss(b)):"cc");
    return !below;
}
// UCOMISS (quiet): a == b and both ordered.
inline bool quiet_ordered_equal(float a,float b) {
    unsigned char equal,parity;asm volatile("ucomiss %3,%2\n\tsete %0\n\tsetp %1":"=qm"(equal),"=qm"(parity):"x"(_mm_set_ss(a)),"x"(_mm_set_ss(b)):"cc");
    return equal&&!parity;
}
#endif
} // namespace sse

// x87 operations exactly as the original instructions, on the extended
// value and under the live control word (precision and rounding). A store
// reports C1, which the hardware sets when the stored result was rounded up.
namespace x87 {
struct Stored {std::uint64_t bits;bool rounded_up;};
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
// FST/FSTP m32.
inline Stored store_single(long double x) {
    float result;std::uint16_t status;
    asm volatile("fldt %2\n\tfstps %0\n\tfnstsw %1":"=m"(result),"=m"(status):"m"(x):"st");
    return {std::bit_cast<std::uint32_t>(result),(status&0x200u)!=0};
}
// FISTP m64, rounded by the control word.
inline Stored store_int64(long double x) {
    std::int64_t result;std::uint16_t status;
    asm volatile("fldt %2\n\tfistpq %0\n\tfnstsw %1":"=m"(result),"=m"(status):"m"(x):"st");
    return {std::bit_cast<std::uint64_t>(result),(status&0x200u)!=0};
}
// FLD m32: a single to the extended format (an SNaN signals and is quieted).
inline long double load_single(std::uint32_t bits) {
    long double result;const auto value=std::bit_cast<float>(bits);
    asm volatile("flds %1\n\tfstpt %0":"=m"(result):"m"(value):"st");
    return result;
}
// FISTP m32, rounded by the control word.
inline Stored store_int32(long double x) {
    std::int32_t result;std::uint16_t status;
    asm volatile("fldt %2\n\tfistpl %0\n\tfnstsw %1":"=m"(result),"=m"(status):"m"(x):"st");
    return {std::uint64_t(std::bit_cast<std::uint32_t>(result)),(status&0x200u)!=0};
}
#endif
// FILD m64 (exact).
inline long double load_int64(std::uint64_t bits){return static_cast<long double>(std::bit_cast<std::int64_t>(bits));}
// FSUB: a - b. long double arithmetic is x87 arithmetic on this target.
inline long double subtract(long double a,long double b){volatile long double difference=a-b;return difference;}
} // namespace x87

// ------------------------------------------------------------- game layouts
// Globals at fixed TU1.5 addresses.
namespace globals {
inline constexpr std::uint32_t object_headers=0x4ecbd0u;    // data_array* of object header datums
inline constexpr std::uint32_t tag_instances=0x4f0414u;     // tag instance table (16-byte rows, +8 definition)
inline constexpr std::uint32_t game_random=0x4f3cd8u;       // -> random state; +4 is the LCG seed
inline constexpr std::uint32_t iterator_owner=0x5020a8u;    // owner of the 1EC970 iteration; +1 enable byte
}
// TU1.5 data_array header.
namespace data_array {
inline constexpr std::uint32_t name=0x0u,maximum_count=0x20u,datum_size=0x24u,alignment_bits=0x28u,valid=0x29u,flags=0x2au,
    signature=0x2cu,allocation=0x30u,next_index=0x34u,first_unallocated=0x38u,actual_count=0x3cu,next_identifier=0x40u,
    data=0x44u,in_use_bits=0x48u;
inline constexpr std::uint32_t header_size=0x4cu,name_length=32u,signature_value=0x64407440u;  // 'd@t@'
}
// Object header datum (12 bytes) in the object data array.
namespace object_header {
inline constexpr std::uint32_t size=12u,object=8u;
}
// The object of a handle (index in the low word; the salt is not checked).
// The three reads are the original's, in order.
template<class Memory,class Access>
std::uint32_t object_of(Memory &memory,std::uint32_t handle,Access table,Access headers,Access object) {
    const auto array=memory.load32(table,globals::object_headers);
    const auto rows=memory.load32(headers,array+data_array::data);
    return memory.load32(object,rows+(handle&0xffffu)*object_header::size+object_header::object);
}

// The data array's iteration as its callers inline it: the first in-use
// index at or after start, below the array's first unallocated index. A
// negative start finds nothing without reading the array; the bit vector's
// word is re-read for every index tested. The roles are the caller's
// accesses of the first unallocated index, the bit vector and its words.
namespace data_array {
struct Scan {
    std::optional<std::uint32_t> index;  // the in-use index found
    std::uint32_t limit;                 // the first unallocated index (when read)
    std::optional<std::uint32_t> bits;   // the bit vector (when read)
};
template<class Memory,class Access>
Scan first_in_use(Memory &memory,std::uint32_t array,std::uint32_t start,Access limit_role,Access bits_role,Access word_role) {
    Scan scan{};
    if(signed_value(start)<0)return {};
    scan.limit=memory.load32(limit_role,array+data_array::first_unallocated);
    if(signed_value(start)>=signed_value(scan.limit))return scan;
    const auto bits=memory.load32(bits_role,array+data_array::in_use_bits);
    scan.bits=bits;
    for(auto index=start;signed_value(index)<signed_value(scan.limit);++index) {
        const auto word=memory.load32(word_role,bits+std::uint32_t(signed_value(index)>>5)*4u);
        if(word&(1u<<(index&31u))){scan.index=index;return scan;}
    }
    return scan;
}
} // namespace data_array

// ------------------------------------------------------ unit animation mode
// C9550. A unit's animation state lies at a per-type offset in the object
// (word +12A); its +7C is the current mode string id. The original compares
// it against the five-entry table at 473990 and stops at the first match.
namespace unit_animation_mode {
inline constexpr std::uint32_t state_offset=0x12au,state_mode=0x7cu;
inline constexpr std::uint32_t listed_modes=0x473990u,listed_mode_count=5u;
enum class Access {object_table,object_headers,object,state_offset,mode,listed_mode};
struct Residue {
    std::uint32_t next_entry;  // table address after the last entry compared
};
struct Result {bool listed;std::uint32_t mode;Residue residue;};
template<class Memory> Result listed(Memory &memory,std::uint32_t unit) {
    const auto object=object_of(memory,unit,Access::object_table,Access::object_headers,Access::object);
    const auto state=object+widen16(memory.load16(Access::state_offset,object+state_offset));
    const auto mode=memory.load32(Access::mode,state+state_mode);
    const auto end=listed_modes+listed_mode_count*4u;
    std::uint32_t entry=listed_modes;bool found=false;
    while(entry<end && !found) {
        found=memory.load32(Access::listed_mode,entry)==mode;
        entry+=4u;
    }
    return {found,mode,{entry}};
}
} // namespace unit_animation_mode

// --------------------------------------------------- flagged datum iterator
// 1EC970. Advances an iterator over the in-use datums of a data array. When
// the iterator's flagged_only byte is set, datums whose flag byte (+9) is
// clear are skipped. The whole iteration is gated by the enable byte of the
// owner global at 5020A8 (the adapter asks enabled() first: the original
// writes its result slot between that test and the iteration).
namespace flagged_datum_iterator {
namespace iterator {
inline constexpr std::uint32_t element=0x0u,array=0x4u,handle=0x8u,index=0xcu,flagged_only=0x10u,published_handle=0x14u;
}
inline constexpr std::uint32_t datum_flag=0x9u;
enum class Access {
    owner,enabled,index,array,first_unallocated,in_use_bits,in_use_word,datum_size,data,index_store,salt,
    handle_store,maximum_count,reset_index_store,reset_handle_store,element_store,flagged_only,flag,
    handle,published_store,exhausted_handle,exhausted_published_store
};
template<class Memory> std::uint8_t enabled(Memory &memory) {
    const auto owner=memory.load32(Access::owner,globals::iterator_owner);
    return std::uint8_t(memory.load8(Access::enabled,owner+1u));
}
struct Result {
    std::uint32_t element;          // the datum, or 0 when the array is exhausted
    std::uint32_t handle;           // the handle published to +14
    std::uint8_t last_flag_tested;  // the flagged_only or flag byte that ended the step
};
template<class Memory> Result advance(Memory &memory,std::uint32_t it) {
    for(;;) {
        const auto start=memory.load32(Access::index,it+iterator::index)+1u;
        const auto array=memory.load32(Access::array,it+iterator::array);
        std::uint32_t element=0;
        if(const auto index=data_array::first_in_use(memory,array,start,Access::first_unallocated,Access::in_use_bits,
               Access::in_use_word).index) {
            const auto size=memory.load32(Access::datum_size,array+data_array::datum_size);
            element=size*(*index)+memory.load32(Access::data,array+data_array::data);
            memory.store32(Access::index_store,it+iterator::index,*index);
            // Datum handle: the datum's salt word above the index.
            const auto salt=memory.load16(Access::salt,element);
            memory.store32(Access::handle_store,it+iterator::handle,(salt<<16)|*index);
        } else {
            const auto reset=memory.load32(Access::maximum_count,array+data_array::maximum_count);
            memory.store32(Access::reset_index_store,it+iterator::index,reset);
            memory.store32(Access::reset_handle_store,it+iterator::handle,0xffffffffu);
        }
        memory.store32(Access::element_store,it+iterator::element,element);
        std::uint8_t tested=0;
        if(element) {
            tested=std::uint8_t(memory.load8(Access::flagged_only,it+iterator::flagged_only));
            if(tested) {
                tested=std::uint8_t(memory.load8(Access::flag,element+datum_flag));
                if(!tested)continue;  // flagged_only: skip datums without the flag
            }
        }
        // Publish the step's handle (the exhausted and found exits are
        // separate instructions in the original).
        const auto handle=memory.load32(element?Access::handle:Access::exhausted_handle,it+iterator::handle);
        memory.store32(element?Access::published_store:Access::exhausted_published_store,it+iterator::published_handle,handle);
        return {element,handle,tested};
    }
}
} // namespace flagged_datum_iterator

// ----------------------------------------------- animation channel iterator
// 1F7F60. Walks the populated entries of a channel table. The owner holds
// groups; the iterator selects one sub-table per group (a signed byte per
// group at +4); each sub-table lists 0x44-byte entries. The packed position
// (+14) holds the current entry's record id, entry, sub-table and group
// bytes, or -1 before the first step. The iterator first advances within
// the current sub-table, then scans the following groups for a selected,
// non-empty sub-table.
namespace animation_channel_iterator {
namespace iterator {
inline constexpr std::uint32_t owner=0x0u,selected_subtable=0x4u,position=0x14u,entry=0x15u,subtable=0x16u,group=0x17u,record=0x18u;
}
namespace owner {inline constexpr std::uint32_t group_count=0x1cu,groups=0x20u;}
namespace group {inline constexpr std::uint32_t size=12u,subtable_count=0x4u,subtables=0x8u;}
namespace subtable {inline constexpr std::uint32_t size=20u,entry_count=0x4u,entries=0x8u;}
namespace entry {inline constexpr std::uint32_t size=0x44u,record_id=0x0u,record_body=0x4u;}
enum class Access {
    position,group,subtable,owner,groups,entry,subtables,entry_count,scan_owner,group_count,
    scan_groups,selection,subtable_count,scan_subtables,scan_entry_count,exhausted_record_store,exhausted_record,
    exhausted_position_store,entries,record_id,position_store,record_store
};
struct Residue {
    // The last group descriptor address or entry count the step examined
    // (or the entry byte it advanced); the original leaves it in a
    // caller-visible scratch register when the table is exhausted.
    std::optional<std::uint32_t> last_examined;
};
struct Result {bool found;std::uint32_t record;Residue residue;};  // record: the entry's body, or the re-read +18 when exhausted
template<class Memory> Result next(Memory &memory,std::uint32_t it) {
    Residue residue;
    const auto position=memory.load32(Access::position,it+iterator::position);
    std::uint32_t current_group=0xffffffffu;  // -1: start the scan at group 0
    std::uint32_t found_group{},found_subtable{},found_entry{},subtable_address{};
    bool found=false;
    if(position!=0xffffffffu) {
        // Advance within the current sub-table.
        current_group=memory.load8(Access::group,it+iterator::group);
        found_subtable=memory.load8(Access::subtable,it+iterator::subtable);
        const auto owner_address=memory.load32(Access::owner,it+iterator::owner);
        const auto groups=memory.load32(Access::groups,owner_address+owner::groups);
        const auto entry_index=memory.load8(Access::entry,it+iterator::entry);
        residue.last_examined=entry_index;
        const auto subtables=memory.load32(Access::subtables,groups+current_group*group::size+group::subtables);
        subtable_address=subtables+found_subtable*subtable::size;
        if(subtable_address) {
            const auto count=memory.load32(Access::entry_count,subtable_address+subtable::entry_count);
            residue.last_examined=entry_index+1u;
            if(signed_value(entry_index+1u)<signed_value(count)) {
                found=true;found_group=current_group;found_entry=entry_index+1u;
            }
        }
    }
    if(!found) {
        // Scan the following groups for a selected sub-table with entries.
        const auto owner_address=memory.load32(Access::scan_owner,it+iterator::owner);
        const auto group_count=memory.load32(Access::group_count,owner_address+owner::group_count);
        for(auto g=current_group+1u;signed_value(g)<signed_value(group_count);++g) {
            const auto group_address=memory.load32(Access::scan_groups,owner_address+owner::groups)+g*group::size;
            residue.last_examined=group_address;
            const auto selected=widen8(memory.load8(Access::selection,g+it+iterator::selected_subtable));
            if(selected<0)continue;
            if(selected>=signed_value(memory.load32(Access::subtable_count,group_address+group::subtable_count)))continue;
            subtable_address=memory.load32(Access::scan_subtables,group_address+group::subtables)+std::uint32_t(selected)*subtable::size;
            const auto count=memory.load32(Access::scan_entry_count,subtable_address+subtable::entry_count);
            residue.last_examined=count;
            if(signed_value(count)<=0)continue;
            found=true;found_group=g;found_subtable=std::uint32_t(selected);found_entry=0;
            break;
        }
    }
    if(!found) {
        memory.store32(Access::exhausted_record_store,it+iterator::record,0u);
        const auto record=memory.load32(Access::exhausted_record,it+iterator::record);
        memory.store32(Access::exhausted_position_store,it+iterator::position,0xffffffffu);
        return {false,record,residue};
    }
    const auto entry_address=memory.load32(Access::entries,subtable_address+subtable::entries)+found_entry*entry::size;
    const auto record_id=memory.load8(Access::record_id,entry_address+entry::record_id);
    // Position bytes: record id, entry, sub-table, group (the group's upper
    // bits are shifted out, as in the original).
    const auto packed=(found_group<<24)|((found_subtable&0xffu)<<16)|((found_entry&0xffu)<<8)|record_id;
    memory.store32(Access::position_store,it+iterator::position,packed);
    const auto body=entry_address+entry::record_body;
    memory.store32(Access::record_store,it+iterator::record,body);
    return {true,body,residue};
}

// 1F7E90, the iteration's start: the owner, the position -1 (before the
// first step), and the sixteen selection bytes (+4..+13, one signed
// sub-table choice per owner group). The selection comes from a source
// selection, optionally through a remap table:
//   - no source: every group selects sub-table 0 (the bytes are zeroed);
//   - a source without a remap table: the source's sixteen bytes, as is;
//   - a source and a remap table: every group first selects none (-1); then
//     each of the table's first sixteen records maps source group i to an
//     owner group (its +4 byte, -1 for none). A source choice that is in
//     range of the record's choices (+8 count, +C list of 8-byte choices)
//     selects the sub-table that choice names (its +5 byte, -1 for none).
//     The selection byte is addressed by the signed group byte.
namespace remap_table {inline constexpr std::uint32_t count=0x70u,records=0x74u,maximum_records=16u;}
namespace remap_record {inline constexpr std::uint32_t size=0x10u,group=0x4u,choice_count=0x8u,choices=0xcu;}
namespace remap_choice {inline constexpr std::uint32_t size=8u,subtable=0x5u;}
inline constexpr std::uint32_t selection_bytes=16u;
enum class BeginAccess {
    owner_store,position_store,zero_store,source_word,copy_store,record_count,none_store,
    records,record_group,source_choice,choice_count,choices,choice_subtable,selection_store
};
enum class Selection {zeroed,copied,remapped};
struct BeginResidue {
    // Values the original leaves in caller-visible scratch registers.
    std::uint32_t copied_words[2];  // copied: the source's third and fourth dwords
    std::int32_t record_limit;      // remapped: the record count (at most 16), as last read
    struct Record {
        std::uint32_t group;         // the record's group byte
        std::int32_t choice;         // the source choice, sign-extended
        bool selected;               // the choice named a sub-table, which was stored
    };
    std::optional<Record> last;     // remapped: the last record examined
    std::uint32_t records_examined; // remapped: the loop's final index
    std::optional<std::uint32_t> iterator;  // remapped: the last re-read iterator pointer
};
struct BeginResult {Selection selection;BeginResidue residue;};
// Frame: the original keeps the clamped record count in its own argument
// slot and re-reads it, and the iterator pointer argument, inside the remap
// loop; those writes and reads belong to the boundary:
//   limit_records(count) stores 16, then the count when it is at most 16;
//   record_limit() reads the slot (the loop's first bound);
//   begin_records() is the loop's register save (only when it runs);
//   iterator() re-reads the iterator argument for a store;
//   reread_limit() reads the slot again after each record.
template<class Memory,class Frame>
BeginResult begin(Memory &memory,Frame &frame,std::uint32_t it,std::uint32_t owner_address,std::uint32_t source,std::uint32_t remap) {
    using Access=BeginAccess;
    BeginResult result{};auto &residue=result.residue;
    memory.store32(Access::owner_store,it+iterator::owner,owner_address);
    memory.store32(Access::position_store,it+iterator::position,0xffffffffu);
    const auto selection=it+iterator::selected_subtable;
    if(!source) {
        for(std::uint32_t word=0;word<selection_bytes/4u;++word)memory.store32(Access::zero_store,selection+word*4u,0u);
        result.selection=Selection::zeroed;return result;
    }
    if(!remap) {
        // A dword at a time, each load followed by its store.
        for(std::uint32_t word=0;word<selection_bytes/4u;++word) {
            const auto value=memory.load32(Access::source_word,source+word*4u);
            memory.store32(Access::copy_store,selection+word*4u,value);
            if(word>=2u)residue.copied_words[word-2u]=value;
        }
        result.selection=Selection::copied;return result;
    }
    result.selection=Selection::remapped;
    frame.limit_records(memory.load32(Access::record_count,remap+remap_table::count));
    for(std::uint32_t word=0;word<selection_bytes/4u;++word)memory.store32(Access::none_store,selection+word*4u,0xffffffffu);
    residue.record_limit=signed_value(frame.record_limit());
    if(residue.record_limit<=0)return result;
    frame.begin_records();
    for(std::uint32_t index=0;;) {
        const auto record=memory.load32(Access::records,remap+remap_table::records)+index*remap_record::size;
        BeginResidue::Record step{};
        step.group=memory.load8(Access::record_group,record+remap_record::group);
        // The source choice is read for every record, mapped or not.
        step.choice=widen8(memory.load8(Access::source_choice,source+index));
        if(step.group!=0xffu && step.choice>=0 &&
            step.choice<signed_value(memory.load32(Access::choice_count,record+remap_record::choice_count))) {
            const auto choice=memory.load32(Access::choices,record+remap_record::choices)+std::uint32_t(step.choice)*remap_choice::size;
            const auto subtable=memory.load8(Access::choice_subtable,choice+remap_choice::subtable);
            if(subtable!=0xffu) {
                step.selected=true;residue.iterator=frame.iterator();
                memory.store8(Access::selection_store,*residue.iterator+std::uint32_t(widen8(step.group))+iterator::selected_subtable,subtable);
            }
        }
        residue.last=step;
        residue.record_limit=signed_value(frame.reread_limit());
        residue.records_examined=++index;
        if(!(signed_value(index)<residue.record_limit))break;
    }
    return result;
}
} // namespace animation_channel_iterator

// --------------------------------------------------------------- bit vector
// C1E90. One bit of a dword bit vector; a negative index addresses the
// words before the vector (arithmetic shift), as in the original.
namespace bit_vector {
enum class Access {word};
template<class Memory> bool test(Memory &memory,std::uint32_t vector,std::uint32_t index) {
    const auto word=memory.load32(Access::word,vector+std::uint32_t(signed_value(index)>>5)*4u);
    return (word&(1u<<(index&31u)))!=0;
}
} // namespace bit_vector

// --------------------------------------------------------- unit level block
// 110350. A unit's level block lies at a per-type offset in the object (word
// +33E). Its level (+40) ramps toward fixed bounds while the block's
// attachment handle (+38) is set. Without one, the level decays, and the
// block's twelve signed bytes (+50) are rescaled by the level-independent
// factor chain (definition +1E8, or a default when that is not positive).
// While the block's owner word (+36) is set and the level is exactly zero,
// a countdown byte (+2) steps toward zero and is reloaded from the game
// LCG; any other level marks it FF. The game meaning of the block is not
// established; the names describe the arithmetic.
namespace unit_level_block {
namespace unit {inline constexpr std::uint32_t tag=0x0u,block_offset=0x33eu;}
namespace block {inline constexpr std::uint32_t countdown=0x2u,owner_word=0x36u,attachment=0x38u,level=0x40u,bytes=0x50u,byte_count=12u;}
inline constexpr std::uint32_t definition_scale=0x1e8u;
// XBE constants, read where the original reads them.
namespace constant {
inline constexpr std::uint32_t ramp_threshold=0x46a0ecu,ramp_down_step=0x46a5d0u,ramp_up_step=0x44a8a0u,ceiling=0x46a0d0u,
    decay_step=0x44a98cu,byte_scale=0x5564d8u,default_scale=0x46a194u,output_scale=0x46a270u;
}
enum class Access {
    object_table,object_headers,object,tag,block_offset,tag_instances,definition,attachment_offset,attachment,
    threshold,level_compare,level,ramp_down,floor_store,ramp_up,ceiling,ramp_store,
    decay_level,decay_step,byte_scale,default_scale,output_scale,decay_store,definition_scale,byte,byte_store,
    owner_headers,owner_object,owner_offset,owner_word,owner_level,countdown_store,countdown,random,seed,seed_store,reload_store,step_store
};
enum class Path {floored,ramped_down,ramped_up,no_owner,nonzero_level,stepped,reloaded};
struct Residue {
    // Values the original leaves in caller-visible scratch registers.
    std::uint32_t object,block_offset,owner_offset,rows,random,reload_product;
    float level,last_product,last_scaled,byte_scale,default_scale,output_scale,ceiling;
};
struct Result {Path path;bool any_byte_nonzero;std::uint32_t block;Residue residue;};
// Frame: the original keeps the byte loop's counter and each scaled value in
// slots of its own stack frame; those writes are part of its observable
// store order, so the boundary owns them: begin_bytes() before the loop,
// spill(value) per byte (returns the reloaded bits), and count_down() at the
// end of each byte (returns the remaining count).
template<class Memory,class Frame> Result update(Memory &memory,Frame &frame,std::uint32_t unit_handle) {
    using namespace sse;
    Result result{};auto &residue=result.residue;
    const auto load_float=[&](Access access,std::uint32_t address){return bits_to_float(memory.load32(access,address));};
    // The object table pointer is kept for the owner test below.
    const auto table=memory.load32(Access::object_table,globals::object_headers);
    const auto object=memory.load32(Access::object,
        memory.load32(Access::object_headers,table+data_array::data)+(unit_handle&0xffffu)*object_header::size+object_header::object);
    residue.object=object;
    const auto tag=memory.load32(Access::tag,object+unit::tag);
    const auto block=object+widen16(memory.load16(Access::block_offset,object+unit::block_offset));
    result.block=block;
    const auto instances=memory.load32(Access::tag_instances,globals::tag_instances);
    const auto definition=memory.load32(Access::definition,(tag&0xffffu)*16u+instances+8u);
    // The original reads the block offset again for the attachment test.
    residue.block_offset=widen16(memory.load16(Access::attachment_offset,object+unit::block_offset));
    if(memory.load32(Access::attachment,residue.block_offset+object+block::attachment)!=0xffffffffu) {
        // Attached: ramp the level toward 0 (below the threshold) or the ceiling.
        const auto threshold=load_float(Access::threshold,constant::ramp_threshold);
        const bool below=ordered_greater(threshold,load_float(Access::level_compare,block+block::level));
        auto level=load_float(Access::level,block+block::level);
        if(below) {
            level=add(level,load_float(Access::ramp_down,constant::ramp_down_step));
            if(!ordered_greater(level,0.0f)) {
                residue.level=0.0f;  // +0
                memory.store32(Access::floor_store,block+block::level,0u);
                result.path=Path::floored;return result;
            }
        } else {
            level=add(level,load_float(Access::ramp_up,constant::ramp_up_step));
            residue.ceiling=load_float(Access::ceiling,constant::ceiling);
            if(ordered_greater(level,residue.ceiling))level=residue.ceiling;
        }
        residue.level=level;
        memory.store32(Access::ramp_store,block+block::level,float_to_bits(level));
        result.path=below?Path::ramped_down:Path::ramped_up;return result;
    }
    // Detached: decay the level and rescale the twelve bytes.
    auto level=subtract(load_float(Access::decay_level,block+block::level),load_float(Access::decay_step,constant::decay_step));
    if(!ordered_greater(level,0.0f))level=0.0f;
    residue.byte_scale=load_float(Access::byte_scale,constant::byte_scale);
    residue.default_scale=load_float(Access::default_scale,constant::default_scale);
    residue.output_scale=load_float(Access::output_scale,constant::output_scale);
    memory.store32(Access::decay_store,block+block::level,float_to_bits(level));
    frame.begin_bytes(block::byte_count);
    auto byte_address=block+block::bytes;
    do {
        const auto scale=load_float(Access::definition_scale,definition+definition_scale);
        const auto factor=ordered_greater(scale,0.0f)?scale:residue.default_scale;
        const auto value=widen8(memory.load8(Access::byte,byte_address));
        // ((value * byte_scale) * factor) * output_scale, in this order.
        auto scaled=multiply(multiply(multiply(from_int(value),residue.byte_scale),factor),residue.output_scale);
        residue.last_product=scaled;
        scaled=frame.spill(scaled);
        auto clamped=truncate(scaled);
        if(clamped<-128)clamped=-128;else if(clamped>127)clamped=127;
        memory.store8(Access::byte_store,byte_address,std::uint32_t(clamped)&0xffu);
        if(clamped&0xff)result.any_byte_nonzero=true;
        residue.last_scaled=scaled;
        ++byte_address;
    } while(frame.count_down()!=0u);
    // The owner test re-reads the unit through the object table.
    residue.rows=memory.load32(Access::owner_headers,table+data_array::data);
    const auto owner_object=memory.load32(Access::owner_object,residue.rows+(unit_handle&0xffffu)*object_header::size+object_header::object);
    residue.owner_offset=widen16(memory.load16(Access::owner_offset,owner_object+unit::block_offset));
    if(memory.load16(Access::owner_word,residue.owner_offset+owner_object+block::owner_word)==0xffffu) {
        result.path=Path::no_owner;return result;
    }
    residue.level=load_float(Access::owner_level,block+block::level);
    if(!quiet_ordered_equal(residue.level,0.0f)) {
        memory.store8(Access::countdown_store,block+block::countdown,0xffu);
        result.path=Path::nonzero_level;return result;
    }
    const auto countdown=memory.load8(Access::countdown,block+block::countdown);
    if(countdown) {
        memory.store8(Access::step_store,block+block::countdown,(countdown-1u)&0xffu);
        result.path=Path::stepped;return result;
    }
    // Reload from the game LCG: 30 + (high word * 225 >> 16), in a byte.
    residue.random=memory.load32(Access::random,globals::game_random);
    const auto seed=memory.load32(Access::seed,residue.random+4u)*0x19660du+0x3c6ef35fu;
    memory.store32(Access::seed_store,residue.random+4u,seed);
    residue.reload_product=((seed>>16)*0xe1u)>>16;
    memory.store8(Access::reload_store,block+block::countdown,(residue.reload_product+0x1eu)&0xffu);
    result.path=Path::reloaded;return result;
}
} // namespace unit_level_block

// ---------------------------------------------------------- vertex position
// 243F20. One vertex position of a mesh part. The part's +3C points at its
// vertex descriptor: a layout byte (+0) and the vertex data at +8 (base)
// plus +C (offset). The layout byte selects the vertex format:
//   1  uncompressed             real_point3d, 12 bytes
//   2  compressed               three int16, 6 bytes
//   3  uncompressed, with node  real_point3d and a node byte at +C, 16 bytes
//   4  compressed, with node    three int16 and a node byte at +6, 8 bytes
// Any other layout yields the default point the global at 474CE8 points to.
// A compressed coordinate q is dequantized into the part's bounds (x, y and
// z minimum/maximum pairs):
//   ((q*2 + 1)*step + 1/2)*(maximum - minimum) + minimum,  step = 1/131070,
// which spreads [-32768, 32767] over the bounds. The constants 2, 1, step
// and 1/2 are XBE data, read where the original reads them. When the caller
// passes a node output, the vertex's node byte (layouts 3 and 4) or zero is
// stored there as a dword.
//
// The original dispatches the layout byte through a jump table in the XBE
// (24417C). The table is immutable code, so the switch below replaces it.
namespace vertex_position {
namespace part {inline constexpr std::uint32_t descriptor=0x3cu;}
namespace descriptor {inline constexpr std::uint32_t layout=0x0u,data_base=0x8u,data_offset=0xcu;}
namespace bounds {inline constexpr std::uint32_t minimum[3]{0x0u,0x8u,0x10u},maximum[3]{0x4u,0xcu,0x14u};}
namespace constant {inline constexpr std::uint32_t two=0x46a11cu,one=0x46a0d0u,step=0x46a2e0u,half=0x46a0ccu,default_point=0x474ce8u;}
enum class Layout {other,uncompressed,compressed,uncompressed_with_node,compressed_with_node};
inline Layout layout_of(std::uint32_t byte) {
    switch(byte) {
    case 1:return Layout::uncompressed;
    case 2:return Layout::compressed;
    case 3:return Layout::uncompressed_with_node;
    case 4:return Layout::compressed_with_node;
    default:return Layout::other;
    }
}
inline bool compressed(Layout l){return l==Layout::compressed || l==Layout::compressed_with_node;}
inline std::uint32_t stride(Layout l) {
    switch(l) {
    case Layout::uncompressed:return 12u;
    case Layout::compressed:return 6u;
    case Layout::uncompressed_with_node:return 16u;
    default:return 8u;
    }
}
// Offset of the node byte in a vertex, for the layouts that have one.
inline std::uint32_t node_offset(Layout l){return l==Layout::uncompressed_with_node?0xcu:6u;}
// Each access names its layout, its role and (for per-axis roles) the axis.
enum class Role {
    descriptor,layout,default_point,data_offset,data_base,maximum,minimum,two,one,step,half,
    coordinate,offset,store,node,node_store
};
struct Access {Layout layout;Role role;unsigned axis;};
struct Constants {float two,one,step,half;};
// ((q*2 + 1)*step + 1/2)*range, in the original's operation order.
inline float scaled(std::int32_t q,const Constants &k,float range) {
    using namespace sse;
    return multiply(add(multiply(add(multiply(from_int(q),k.two),k.one),k.step),k.half),range);
}
struct Residue {
    // Values the original leaves in caller-visible scratch registers.
    std::uint32_t element;  // the vertex address
    float range[3];         // compressed: the y and z ranges stay in XMM1/XMM2
    Constants constants;    // compressed: XMM3..XMM6
};
struct Result {Layout layout;std::uint32_t point[3];bool node_stored;std::uint32_t node;Residue residue;};
// Frame: the compressed layouts pass each dequantized coordinate through a
// slot of the original's stack frame and store the reloaded bits; those
// writes are part of its store order, so the boundary owns them:
// spill(layout, axis, value) stores the value and returns the reloaded bits.
template<class Memory,class Frame>
Result read(Memory &memory,Frame &frame,std::uint32_t part_address,std::uint32_t index,std::uint32_t bounds_address,
    std::uint32_t point,std::uint32_t node_output) {
    using namespace sse;
    Result result{};
    const auto descriptor_address=memory.load32(Access{Layout::other,Role::descriptor,0},part_address+part::descriptor);
    result.layout=layout_of(memory.load8(Access{Layout::other,Role::layout,0},descriptor_address+descriptor::layout));
    const auto layout=result.layout;
    const auto at=[layout](Role role,unsigned axis=0){return Access{layout,role,axis};};
    const auto load_float=[&](Access access,std::uint32_t address){return bits_to_float(memory.load32(access,address));};
    const auto vertex_data=[&] {
        const auto offset=memory.load32(at(Role::data_offset),descriptor_address+descriptor::data_offset);
        return offset+memory.load32(at(Role::data_base),descriptor_address+descriptor::data_base);
    };
    if(layout==Layout::other) {
        // The default point, copied a coordinate at a time.
        const auto source=memory.load32(at(Role::default_point),constant::default_point);
        for(unsigned axis=0;axis<3;++axis) {
            result.point[axis]=memory.load32(at(Role::coordinate,axis),source+axis*4u);
            memory.store32(at(Role::store,axis),point+axis*4u,result.point[axis]);
        }
    } else if(!compressed(layout)) {
        const auto element=vertex_data()+index*stride(layout);
        result.residue.element=element;
        for(unsigned axis=0;axis<3;++axis) {
            result.point[axis]=memory.load32(at(Role::coordinate,axis),element+axis*4u);
            memory.store32(at(Role::store,axis),point+axis*4u,result.point[axis]);
        }
    } else {
        const auto data=vertex_data();
        auto &range=result.residue.range;auto &k=result.residue.constants;
        // Read order: the x range, the constants, the y and z ranges.
        const auto range_of=[&](unsigned axis) {
            const auto maximum=load_float(at(Role::maximum,axis),bounds_address+bounds::maximum[axis]);
            return subtract(maximum,load_float(at(Role::minimum,axis),bounds_address+bounds::minimum[axis]));
        };
        range[0]=range_of(0);
        k.two=load_float(at(Role::two),constant::two);
        k.one=load_float(at(Role::one),constant::one);
        k.step=load_float(at(Role::step),constant::step);
        k.half=load_float(at(Role::half),constant::half);
        range[1]=range_of(1);
        range[2]=range_of(2);
        const auto element=data+index*stride(layout);
        result.residue.element=element;
        std::int32_t q[3];
        for(unsigned axis=0;axis<3;++axis)q[axis]=signed_value(widen16(memory.load16(at(Role::coordinate,axis),element+axis*2u)));
        // Each coordinate adds its minimum, re-read from the bounds.
        const auto coordinate=[&](unsigned axis) {
            return add(scaled(q[axis],k,range[axis]),load_float(at(Role::offset,axis),bounds_address+bounds::minimum[axis]));
        };
        const auto x=coordinate(0),y=coordinate(1);
        // Store order: the y and x spills, the point's x, the z spill, then
        // the point's y and z.
        result.point[1]=frame.spill(layout,1u,y);
        const auto z=coordinate(2);
        result.point[0]=frame.spill(layout,0u,x);
        memory.store32(at(Role::store,0),point,result.point[0]);
        result.point[2]=frame.spill(layout,2u,z);
        memory.store32(at(Role::store,1),point+4u,result.point[1]);
        memory.store32(at(Role::store,2),point+8u,result.point[2]);
    }
    if(node_output) {
        const bool has_node=layout==Layout::uncompressed_with_node || layout==Layout::compressed_with_node;
        result.node=has_node?memory.load8(at(Role::node),result.residue.element+node_offset(layout)):0u;
        memory.store32(at(Role::node_store),node_output,result.node);
        result.node_stored=true;
    }
    return result;
}
} // namespace vertex_position

// ------------------------------------------------------- float to int64
// 32CC0C, the C runtime's _ftol2: the x87 value on top of the stack as a
// 64-bit integer, truncated toward zero whatever the rounding mode. FISTP
// rounds the value by the control word; the routine then forms the
// difference value - rounded (FSUBP, stored as a float) and steps the integer
// one toward zero when the rounding went away from zero: down for a
// positive value whose difference is negative, up for a negative value whose
// difference is positive. The value's sign comes from its single-precision
// store. Zero, and the integer indefinite 8000000000000000 (FISTP's result
// for a NaN or an out-of-range value), are returned as rounded.
//
// Every intermediate passes through the routine's own stack slots (Slots,
// owned by the boundary), and the routine re-reads them; those stores are
// part of the store order, and the re-reads are kept.
namespace float_to_int64 {
struct Slots {std::uint32_t sign,integer,difference;};  // integer: 8 bytes, high dword at +4
enum class Access {
    sign_store,integer_store,integer_reload,sign_load,low_load,zero_high_load,discard_rounded_store,discard_value_store,
    negative_difference_store,negative_difference_load,negative_high_load,
    positive_difference_store,positive_difference_load,positive_high_load
};
enum class Path {zero_or_indefinite,negative,positive};
struct Result {
    std::uint64_t value;
    Path path;
    std::uint32_t high;               // the integer's high dword, as re-read
    bool step;                        // the one-step correction was taken
    std::uint32_t biased_difference;  // the difference's float bits plus the carry bias (ECX)
    bool last_store_rounded_up;       // C1 of the routine's last x87 store
};
template<class Memory> Result convert(Memory &memory,const Slots &slot,long double value) {
    memory.store32(Access::sign_store,slot.sign,std::uint32_t(x87::store_single(value).bits));
    memory.store64(Access::integer_store,slot.integer,x87::store_int64(value).bits);
    const auto rounded=x87::load_int64(memory.load64(Access::integer_reload,slot.integer));
    const auto sign=memory.load32(Access::sign_load,slot.sign);
    const auto low=memory.load32(Access::low_load,slot.integer);
    if(low==0) {
        const auto high=memory.load32(Access::zero_high_load,slot.integer+4u);
        if((high&0x7fffffffu)==0) {
            // Zero or the integer indefinite: both values are popped through
            // the sign slot, the rounded value first.
            memory.store32(Access::discard_rounded_store,slot.sign,std::uint32_t(x87::store_single(rounded).bits));
            const auto last=x87::store_single(value);
            memory.store32(Access::discard_value_store,slot.sign,std::uint32_t(last.bits));
            return {std::uint64_t(high)<<32,Path::zero_or_indefinite,high,false,0u,last.rounded_up};
        }
    }
    const auto difference=x87::store_single(x87::subtract(value,rounded));
    const bool negative=signed_value(sign)<0;
    memory.store32(negative?Access::negative_difference_store:Access::positive_difference_store,slot.difference,
        std::uint32_t(difference.bits));
    const auto bits=memory.load32(negative?Access::negative_difference_load:Access::positive_difference_load,slot.difference);
    // A negative value steps up when the difference is a positive nonzero
    // float; a positive value steps down when it is a negative nonzero one.
    // The original tests the float bits with a biased add (the carry out of
    // bits + 7FFFFFFF, after flipping the sign for a negative value).
    const auto tested=negative?bits^0x80000000u:bits;
    const bool step=tested>0x80000000u;
    const auto high=memory.load32(negative?Access::negative_high_load:Access::positive_high_load,slot.integer+4u);
    const auto integer=(std::uint64_t(high)<<32)|low;
    return {negative?integer+step:integer-step,negative?Path::negative:Path::positive,high,step,tested+0x7fffffffu,
        difference.rounded_up};
}
} // namespace float_to_int64

// ------------------------------------------------------ bounded string copy
// 32A4E0, the C runtime's strncpy(destination, source, count): copies the
// source up to and including its terminator, at most count bytes, and fills
// the rest of the count with zeros. The runtime's access pattern is kept:
//   1. bytes, until the source is dword aligned;
//   2. whole dwords while four or more bytes remain. Each source dword is
//      read twice: for the zero-byte test ((w + 7EFEFEFF) ^ ~w) & 81010100,
//      which can report a zero byte that is not there (each byte is then
//      checked), and for the copy. The dword that holds the terminator is
//      stored with zeros above it;
//   3. the remaining bytes.
// The zero fill after a terminator copied in step 1 stores bytes until the
// destination is aligned, then dwords, then bytes; after one in step 2,
// dwords then bytes; after one in step 3, bytes.
namespace bounded_string_copy {
enum class Access {
    leading_byte,leading_store,trailing_byte,trailing_store,word_test,word,word_store,
    // The dword holding the terminator, by the terminator's byte (low first).
    terminated_store_0,terminated_store_1,terminated_store_2,terminated_store_3,
    leading_fill,word_fill,byte_fill
};
inline constexpr std::uint32_t zero_test_bias=0x7efefeffu,zero_test_mask=0x81010100u;
enum class Exit {empty,copied,byte_filled,filled};
struct Residue {
    std::optional<std::uint32_t> last_word;  // the last dword stored by step 2 (zeros above a terminator)
    bool counted_down;                       // the copy ended on a count reaching zero, not on a zero test
};
struct Result {Exit exit;Residue residue;};
// True when the dword may hold a zero byte (the runtime's test).
inline bool may_hold_zero(std::uint32_t word){return (((word+zero_test_bias)^~word)&zero_test_mask)!=0;}
template<class Memory> Result copy(Memory &memory,std::uint32_t destination,std::uint32_t source,std::uint32_t count) {
    Result result{};auto &residue=result.residue;
    if(!count){result.exit=Exit::empty;return result;}
    const auto zero_bytes=[&](Access access,std::uint32_t bytes) {
        for(;bytes;--bytes)memory.store8(access,destination++,0u);
    };
    const auto zero_words=[&](std::uint32_t words) {
        for(;words;--words){memory.store32(Access::word_fill,destination,0u);destination+=4u;}
    };
    auto remaining=count;
    // Step 1: bytes until the source is aligned.
    if(source&3u) {
        for(;;) {
            const auto character=memory.load8(Access::leading_byte,source++);
            memory.store8(Access::leading_store,destination++,character);
            if(--remaining==0){result.exit=Exit::copied;residue.counted_down=true;return result;}
            if(!character) {
                // Fill: bytes to an aligned destination, dwords, bytes.
                while(destination&3u) {
                    memory.store8(Access::leading_fill,destination++,0u);
                    if(--remaining==0){result.exit=Exit::filled;residue.counted_down=true;return result;}
                }
                zero_words(remaining>>2);
                if(remaining>>2 && !(remaining&3u)){result.exit=Exit::filled;return result;}
                zero_bytes(Access::byte_fill,remaining&3u);
                result.exit=Exit::byte_filled;residue.counted_down=true;return result;
            }
            if(!(source&3u))break;
        }
    }
    // Step 2: dwords.
    for(auto words=remaining>>2;words;) {
        const auto test=memory.load32(Access::word_test,source);
        auto word=memory.load32(Access::word,source);
        source+=4u;
        unsigned terminator=4;
        if(may_hold_zero(test)) {
            for(unsigned byte=0;byte<4 && terminator==4;++byte)if(!((word>>(8*byte))&0xffu))terminator=byte;
        }
        if(terminator==4) {
            memory.store32(Access::word_store,destination,word);residue.last_word=word;
            destination+=4u;--words;
            continue;
        }
        // The terminator's dword, zeros above it, then the fill.
        word&=terminator?0xffffffffu>>(8*(4-terminator)):0u;
        constexpr Access terminated[4]{Access::terminated_store_0,Access::terminated_store_1,Access::terminated_store_2,
            Access::terminated_store_3};
        memory.store32(terminated[terminator],destination,word);residue.last_word=word;
        destination+=4u;
        zero_words(words-1u);
        if(!(remaining&3u)){result.exit=Exit::filled;return result;}
        zero_bytes(Access::byte_fill,remaining&3u);
        result.exit=Exit::byte_filled;residue.counted_down=true;return result;
    }
    // Step 3: the remaining bytes.
    auto tail=remaining&3u;
    if(!tail){result.exit=Exit::copied;return result;}
    for(;;) {
        const auto character=memory.load8(Access::trailing_byte,source++);
        memory.store8(Access::trailing_store,destination++,character);
        if(!character) {
            zero_bytes(Access::byte_fill,tail-1u);
            result.exit=Exit::byte_filled;residue.counted_down=true;return result;
        }
        if(--tail==0){result.exit=Exit::copied;residue.counted_down=true;return result;}
    }
}
} // namespace bounded_string_copy

// ------------------------------------------------------- data array reset
// 16E600, data_delete_all: empties a data array. It clears the first
// unallocated index (+38), the active count (+3C) and the next index (+34);
// reseeds the next salt (+40) from the first two characters of the array's
// name (a strncpy call, 32A4E0) with the high bit set; fills the datums with
// BA bytes when flag bit 3 (+2A, the debug fill) is set; zeroes each
// datum's salt word; and clears the in-use bit vector (+48, one bit per
// datum rounded up to dwords). The salt loop re-reads the header fields on
// every datum, as the original does.
namespace data_array_reset {
inline constexpr std::uint32_t debug_fill_flag=0x8u,debug_fill_byte=0xbau,salt_high_bit=0x8000u;
enum class Access {
    first_unallocated_store,actual_count_store,next_index_store,
    flags,salt,salt_store,fill_datum_size,fill_maximum_count,fill_data,debug_fill,
    datum_count,datum_size,datum_data,datum_salt_store,datum_recount,bit_count,bit_vector,bit_clear
};
struct Result {std::uint32_t bit_vector_bytes;};
inline constexpr std::uint32_t salt_characters=2u;
// Frame: copy_name(destination, source, count) is the strncpy call
// (bounded_string_copy::copy through its call frame).
template<class Memory,class Frame> Result delete_all(Memory &memory,Frame &frame,std::uint32_t array) {
    memory.store32(Access::first_unallocated_store,array+data_array::first_unallocated,0u);
    memory.store32(Access::actual_count_store,array+data_array::actual_count,0u);
    memory.store32(Access::next_index_store,array+data_array::next_index,0u);
    const auto salt=array+data_array::next_identifier;
    frame.copy_name(salt,array+data_array::name,salt_characters);
    const auto flags=memory.load8(Access::flags,array+data_array::flags);
    memory.store16(Access::salt_store,salt,memory.load16(Access::salt,salt)|salt_high_bit);
    if(flags&debug_fill_flag) {
        const auto size=memory.load32(Access::fill_datum_size,array+data_array::datum_size);
        const auto bytes=size*memory.load32(Access::fill_maximum_count,array+data_array::maximum_count);
        memory.fill(Access::debug_fill,memory.load32(Access::fill_data,array+data_array::data),bytes,debug_fill_byte);
    }
    if(signed_value(memory.load32(Access::datum_count,array+data_array::maximum_count))>0) {
        for(std::uint32_t index=0;;) {
            const auto size=memory.load32(Access::datum_size,array+data_array::datum_size);
            const auto data=memory.load32(Access::datum_data,array+data_array::data);
            memory.store16(Access::datum_salt_store,size*index+data,0u);
            const auto count=memory.load32(Access::datum_recount,array+data_array::maximum_count);
            if(!(signed_value(++index)<signed_value(count)))break;
        }
    }
    // One bit per datum, in whole dwords: ((count + 31) >> 5) * 4 bytes, with
    // the original's signed shift.
    const auto count=memory.load32(Access::bit_count,array+data_array::maximum_count);
    const auto bytes=std::uint32_t(signed_value(count+0x1fu)>>5)<<2;
    memory.fill(Access::bit_clear,memory.load32(Access::bit_vector,array+data_array::in_use_bits),bytes,0u);
    return {bytes};
}
} // namespace data_array_reset

// ------------------------------------------------------- data array setup
// 16E450, data_initialize: lays out a data array in a caller's block. The
// datums start after the header, aligned to 2^alignment_bits bytes (the
// shift count is taken modulo 32, as SHL does); the in-use bit vector
// follows the datums. 16E4B0 initializes the header (below); then flag bits
// 0 and 1 are cleared again, the data pointer is stored, and the bit vector
// is cleared (one bit per datum, rounded up to dwords, the count shifted
// arithmetically).
namespace data_array_setup {
inline constexpr std::uint32_t setup_flags=0x3u;
enum class Access {
    header_clear,flags,flags_store,name_end_store,maximum_count_store,datum_size_store,data_clear_store,valid_store,
    alignment_store,signature_store,in_use_bits_store,allocation_store,
    reset_flags,reset_flags_store,data_store,bit_vector,bit_clear
};
struct Layout {std::uint32_t data,in_use_bits,bit_vector_bytes;};
inline Layout layout(std::uint32_t block,std::uint32_t maximum_count,std::uint32_t datum_size,std::uint32_t alignment_bits) {
    const auto mask=(1u<<(alignment_bits&31u))-1u;
    const auto data=(block+data_array::header_size+mask)&~mask;
    return {data,maximum_count*datum_size+data,std::uint32_t(signed_value(maximum_count+0x1fu)>>5)<<2};
}
// Frame: initialize_header(in_use_bits) is the 16E4B0 call, with the
// caller's other arguments passed through.
template<class Memory,class Frame>
Layout initialize(Memory &memory,Frame &frame,std::uint32_t array,std::uint32_t maximum_count,std::uint32_t datum_size,
    std::uint32_t alignment_bits) {
    const auto placed=layout(array,maximum_count,datum_size,alignment_bits);
    frame.initialize_header(placed.in_use_bits);
    memory.store8(Access::reset_flags_store,array+data_array::flags,memory.load8(Access::reset_flags,array+data_array::flags)&~setup_flags);
    memory.store32(Access::data_store,array+data_array::data,placed.data);
    memory.fill(Access::bit_clear,memory.load32(Access::bit_vector,array+data_array::in_use_bits),placed.bit_vector_bytes,0u);
    return placed;
}

// 16E4B0: the header. It is cleared; the name is copied (strncpy, 32
// characters) and its last character forced to zero; the maximum count,
// datum size, alignment bits, allocation and bit vector are stored with the
// 'd@t@' signature; the data pointer and the valid byte are cleared, and
// flag bits 0 and 1 set. The arguments are read from the caller's stack where
// the original uses them, between its stores.
enum class Argument {name,maximum_count,datum_size,alignment_bits,allocation,in_use_bits};
struct Header {std::uint32_t maximum_count,datum_size,alignment_bits,allocation,in_use_bits;};
// Frame: argument(a) reads one argument; copy_name(destination, source,
// count) is the strncpy call.
template<class Memory,class Frame> Header initialize_header(Memory &memory,Frame &frame,std::uint32_t array) {
    Header header{};
    memory.fill(Access::header_clear,array,data_array::header_size,0u);
    frame.copy_name(array+data_array::name,frame.argument(Argument::name),data_array::name_length);
    header.maximum_count=frame.argument(Argument::maximum_count);
    header.datum_size=frame.argument(Argument::datum_size);
    memory.store8(Access::flags_store,array+data_array::flags,memory.load8(Access::flags,array+data_array::flags)|setup_flags);
    memory.store8(Access::name_end_store,array+data_array::name+data_array::name_length-1u,0u);
    memory.store32(Access::maximum_count_store,array+data_array::maximum_count,header.maximum_count);
    header.alignment_bits=frame.argument(Argument::alignment_bits);
    memory.store32(Access::datum_size_store,array+data_array::datum_size,header.datum_size);
    header.in_use_bits=frame.argument(Argument::in_use_bits);
    memory.store32(Access::data_clear_store,array+data_array::data,0u);
    memory.store8(Access::valid_store,array+data_array::valid,0u);
    header.allocation=frame.argument(Argument::allocation);
    memory.store8(Access::alignment_store,array+data_array::alignment_bits,header.alignment_bits);
    memory.store32(Access::signature_store,array+data_array::signature,data_array::signature_value);
    memory.store32(Access::in_use_bits_store,array+data_array::in_use_bits,header.in_use_bits);
    memory.store32(Access::allocation_store,array+data_array::allocation,header.allocation);
    return header;
}
} // namespace data_array_setup

// ---------------------------------------------------------- interval table
// 171010. Finds the record of a table whose interval [first, first + count)
// holds a key. The table block holds the record count (+0) and the records
// (+4), 0x48 bytes each, with the interval's first value at +6 and its
// count at +8: unsigned words, compared with the signed key. The routine
// first clears the caller's offset output. When a record holds the key it
// stores key - first through the offset pointer argument. Finally it reads
// the table count again: an index equal to it means no record, and -1 is
// stored through the index pointer argument; otherwise the index is (for a
// negative count that is index 0, with no record tested).
namespace interval_table {
namespace table {inline constexpr std::uint32_t count=0x0u,records=0x4u;}
namespace record {inline constexpr std::uint32_t size=0x48u,first=0x6u,count=0x8u;}
enum class Access {clear_store,count,records,first,length,found_first,offset_store,count_recheck,index_store,missing_store};
struct Residue {
    // Values the original leaves in caller-visible scratch registers.
    std::uint32_t record;                     // the record address reached (after the last one when none matched)
    std::optional<std::uint32_t> last_first;  // the last first value read
    std::uint32_t offset_output;              // the offset pointer, when a record matched
};
struct Result {bool matched;std::uint32_t index,count_recheck;Residue residue;};
// Frame: the boundary's stack frame. The original saves a register for the
// record loop once the table is known to be non-empty (save_loop_register(),
// a store after the clear), and reads its two pointer arguments from the
// caller's stack where it uses them: offset_output() on a match, and
// index_output(missing) on each of the two exits.
template<class Memory,class Frame>
Result find(Memory &memory,Frame &frame,std::uint32_t table_address,std::uint32_t key,std::uint32_t cleared_offset) {
    Result result{};auto &residue=result.residue;
    memory.store32(Access::clear_store,cleared_offset,0u);
    const auto count=memory.load32(Access::count,table_address+table::count);
    if(signed_value(count)>0) {
        residue.record=memory.load32(Access::records,table_address+table::records);
        frame.save_loop_register();
        do {
            const auto first=memory.load16(Access::first,residue.record+record::first);
            residue.last_first=first;
            if(!(signed_value(first)>signed_value(key))) {
                const auto end=memory.load16(Access::length,residue.record+record::count)+first;
                if(signed_value(end)>signed_value(key)) {
                    // The first value is read again for the offset.
                    const auto found_first=memory.load16(Access::found_first,residue.record+record::first);
                    residue.last_first=found_first;
                    residue.offset_output=frame.offset_output();
                    memory.store32(Access::offset_store,residue.offset_output,key-found_first);
                    result.matched=true;
                    break;
                }
            }
            ++result.index;residue.record+=record::size;
        } while(signed_value(result.index)<signed_value(count));
    }
    result.count_recheck=memory.load32(Access::count_recheck,table_address+table::count);
    if(result.index==result.count_recheck)memory.store32(Access::missing_store,frame.index_output(true),0xffffffffu);
    else memory.store32(Access::index_store,frame.index_output(false),result.index);
    return result;
}
} // namespace interval_table

// ---------------------------------------------------------- triangle point
// 1211C0. Whether a point lies on a triangle, and its barycentric
// coordinates when it does. With the edges e1 = v1 - v0 and e2 = v2 - v0,
// the point's offset d = p - v0 and the normal n = e2 x e1:
//   - the point must be near the plane: (n.d)^2 < |n|^2 * tolerance;
//   - the triangle is projected onto the two axes (u, v) other than the
//     normal's dominant one (123210), in the order the table at 44AC8C gives
//     for that axis and the normal's sign, so the projected winding is fixed;
//   - in that plane  b2 = d[v]*e2[u] - e2[v]*d[u],
//                    b1 = e1[v]*d[u] - d[v]*e1[u],
//                    det = e1[v]*e2[u] - e2[v]*e1[u];
//     b2 and b1 must exceed -tolerance, det must lie outside
//     [-tolerance, tolerance], and b1 + b2 must not exceed det + tolerance.
// The coordinates b1/det and b2/det are then stored through the two output
// pointers. The tolerance is 1e-4 (XBE constants, read where the original
// reads them). Every comparison is ordered: a NaN rejects.
//
// The original keeps its vectors in a local block of its stack frame (the
// normal, d, e2 and e1, three floats each), which the projection indexes by
// axis and the dominant-axis call reads; the boundary gives its address.
// Those stores, and the reuse of the normal's x slot for e2[u], are part of
// the original's store order.
namespace triangle_point {
namespace locals {inline constexpr std::uint32_t normal=0x0u,offset=0xcu,edge2=0x18u,edge1=0x24u;}
namespace constant {inline constexpr std::uint32_t tolerance=0x46a0ecu,zero=0x46a0e8u,negative_tolerance=0x46a30cu,one=0x46a0d0u,
    projection=0x44ac8cu;}  // projection: rows of three words (u, v, dominant) per (axis, sign)
enum class Access {
    point_x,origin_x,edge1_y_end,origin_y,edge1_z_end,origin_z,edge2_y_end,origin_y2,edge2_x_end,origin_x2,edge1_x_end,origin_x3,
    offset_x_store,point_y,origin_y3,offset_y_store,point_z,origin_z2,offset_z_store,edge2_z_end,origin_z3,
    edge2_y_store,edge1_z_store,edge2_z_store,edge1_y_store,offset_y,offset_z,offset_x,normal_z_store,normal_y_store,
    tolerance,edge2_x_store,edge1_x_store,normal_x_store,
    dominant_component,zero,negative_tolerance,projection_u,projection_v,offset_v,edge2_v,edge2_u,offset_u,edge2_u_store,
    edge1_v,edge1_u,edge2_u_reload,degenerate_tolerance,sum_tolerance,one,u_store,v_store
};
enum class Path {off_plane,outside_edge2,outside_edge1,degenerate,outside_sum,inside};
struct Result {
    Path path;
    // The values computed, in the order the original computes them (the
    // boundary maps them onto the registers the original leaves them in).
    float normal[3],normal_x_squared,normal_y_squared,plane_limit,dot_squared,edge1_x,edge2_x,edge1_y_edge2_z;
    std::uint32_t axis;std::int32_t u,v;
    float eps,offset_v,edge2_v,offset_u,edge2_v_offset_u,b2,edge1_v,edge1_u,offset_v_edge1_u,b1,edge2_v_edge1_u,det,
        det_limit,sum,u_coordinate,v_coordinate;
    std::uint32_t u_output,v_output;
};
// Frame: locals() is the address of the local block; dominant_axis(normal)
// calls 123210 on the normal there (0 x, 1 y, 2 z); u_output() and
// v_output() read the caller's two pointer arguments.
template<class Memory,class Frame>
Result test(Memory &memory,Frame &frame,std::uint32_t point,std::uint32_t v0,std::uint32_t v1,std::uint32_t v2) {
    using namespace sse;
    Result r{};
    const auto block=frame.locals();
    const auto load=[&](Access a,std::uint32_t address){return bits_to_float(memory.load32(a,address));};
    const auto store=[&](Access a,std::uint32_t address,float value){memory.store32(a,address,float_to_bits(value));};
    const auto difference=[&](Access end,std::uint32_t end_address,Access start,std::uint32_t start_address) {
        const auto a=load(end,end_address);return subtract(a,load(start,start_address));
    };
    // The offset, the two edges and their spills, in the original's order.
    const auto dx=difference(Access::point_x,point,Access::origin_x,v0);
    const auto e1y=difference(Access::edge1_y_end,v1+4u,Access::origin_y,v0+4u);
    const auto e1z=difference(Access::edge1_z_end,v1+8u,Access::origin_z,v0+8u);
    const auto e2y=difference(Access::edge2_y_end,v2+4u,Access::origin_y2,v0+4u);
    const auto e2x=difference(Access::edge2_x_end,v2,Access::origin_x2,v0);
    const auto e1x=difference(Access::edge1_x_end,v1,Access::origin_x3,v0);
    store(Access::offset_x_store,block+locals::offset,dx);
    store(Access::offset_y_store,block+locals::offset+4u,difference(Access::point_y,point+4u,Access::origin_y3,v0+4u));
    store(Access::offset_z_store,block+locals::offset+8u,difference(Access::point_z,point+8u,Access::origin_z2,v0+8u));
    const auto e2z=difference(Access::edge2_z_end,v2+8u,Access::origin_z3,v0+8u);
    store(Access::edge2_y_store,block+locals::edge2+4u,e2y);
    const auto e1z_e2y=multiply(e1z,e2y),e2y_e1x=multiply(e2y,e1x);
    store(Access::edge1_z_store,block+locals::edge1+8u,e1z);
    r.edge1_y_edge2_z=multiply(e1y,e2z);
    const auto e1z_e2x=multiply(e1z,e2x);
    store(Access::edge2_z_store,block+locals::edge2+8u,e2z);
    store(Access::edge1_y_store,block+locals::edge1+4u,e1y);
    // n = e2 x e1, and the plane test against the re-read offset.
    auto &n=r.normal;
    n[1]=subtract(multiply(e2z,e1x),e1z_e2x);
    n[2]=subtract(multiply(e1y,e2x),e2y_e1x);
    const auto ny_dy=multiply(n[1],load(Access::offset_y,block+locals::offset+4u));
    const auto nz_dz=multiply(n[2],load(Access::offset_z,block+locals::offset+8u));
    const auto dot_yz=add(ny_dy,nz_dz);
    n[0]=subtract(e1z_e2y,r.edge1_y_edge2_z);
    const auto dot=add(dot_yz,multiply(n[0],load(Access::offset_x,block+locals::offset)));
    r.dot_squared=multiply(dot,dot);
    const auto nz2=multiply(n[2],n[2]);
    store(Access::normal_z_store,block+locals::normal+8u,n[2]);
    r.normal_y_squared=multiply(n[1],n[1]);
    store(Access::normal_y_store,block+locals::normal+4u,n[1]);
    r.normal_x_squared=multiply(n[0],n[0]);
    // |n|^2 = (z^2 + y^2) + x^2, in this order.
    r.plane_limit=multiply(add(add(nz2,r.normal_y_squared),r.normal_x_squared),load(Access::tolerance,constant::tolerance));
    const bool near_plane=ordered_greater(r.plane_limit,r.dot_squared);
    r.edge2_x=e2x;r.edge1_x=e1x;
    store(Access::edge2_x_store,block+locals::edge2,e2x);
    store(Access::edge1_x_store,block+locals::edge1,e1x);
    store(Access::normal_x_store,block+locals::normal,n[0]);
    if(!near_plane){r.path=Path::off_plane;return r;}
    // Project onto the axes other than the normal's dominant one.
    r.axis=frame.dominant_axis(block+locals::normal);
    const auto dominant=load(Access::dominant_component,block+locals::normal+r.axis*4u);
    const bool positive=ordered_greater(dominant,load(Access::zero,constant::zero));
    r.eps=load(Access::negative_tolerance,constant::negative_tolerance);
    const auto row=constant::projection+(r.axis*2u+(positive?1u:0u))*6u;
    r.u=signed_value(widen16(memory.load16(Access::projection_u,row)));
    r.v=signed_value(widen16(memory.load16(Access::projection_v,row+2u)));
    const auto u=std::uint32_t(r.u)*4u,v=std::uint32_t(r.v)*4u;
    r.offset_v=load(Access::offset_v,block+locals::offset+v);
    r.edge2_v=load(Access::edge2_v,block+locals::edge2+v);
    const auto e2u=load(Access::edge2_u,block+locals::edge2+u);
    r.offset_u=load(Access::offset_u,block+locals::offset+u);
    store(Access::edge2_u_store,block+locals::normal,e2u);
    r.edge2_v_offset_u=multiply(r.edge2_v,r.offset_u);
    r.b2=subtract(multiply(r.offset_v,e2u),r.edge2_v_offset_u);
    if(!ordered_greater(r.b2,r.eps)){r.path=Path::outside_edge2;return r;}
    r.edge1_v=load(Access::edge1_v,block+locals::edge1+v);
    r.edge1_u=load(Access::edge1_u,block+locals::edge1+u);
    r.offset_v_edge1_u=multiply(r.offset_v,r.edge1_u);
    r.b1=subtract(multiply(r.edge1_v,r.offset_u),r.offset_v_edge1_u);
    if(!ordered_greater(r.b1,r.eps)){r.path=Path::outside_edge1;return r;}
    // det, with e2[u] re-read from the normal's x slot.
    const auto edge1_v_edge2_u=multiply(r.edge1_v,load(Access::edge2_u_reload,block+locals::normal));
    r.edge2_v_edge1_u=multiply(r.edge2_v,r.edge1_u);
    r.det=subtract(edge1_v_edge2_u,r.edge2_v_edge1_u);
    if(!ordered_greater(r.eps,r.det) && !ordered_greater(r.det,load(Access::degenerate_tolerance,constant::tolerance))) {
        r.path=Path::degenerate;return r;
    }
    r.det_limit=add(r.det,load(Access::sum_tolerance,constant::tolerance));
    r.sum=add(r.b1,r.b2);
    if(!ordered_not_below(r.det_limit,r.sum)){r.path=Path::outside_sum;return r;}
    const auto inverse=divide(load(Access::one,constant::one),r.det);
    r.u_output=frame.u_output();r.v_output=frame.v_output();
    r.u_coordinate=multiply(inverse,r.b1);
    store(Access::u_store,r.u_output,r.u_coordinate);
    r.v_coordinate=multiply(inverse,r.b2);
    store(Access::v_store,r.v_output,r.v_coordinate);
    r.path=Path::inside;return r;
}
} // namespace triangle_point
// ---------------------------------------------------- resource references
// 22A790. Whether any hardware resource an owner references is still busy.
// The owner lists 16-byte references (+10 count, +14 list). A reference of
// the hardware kind (+0 byte 2) with an id (+C, -1 for none) locates its
// resource holder in the caller's block, at the reference's byte offset (+4
// word) past the block's first 8 bytes: either the holder itself, when the
// element word (+6) is FFFF, or element number <element> of the 32-byte
// holders that the structure there points to (its +4). A holder's +10 is the
// hardware resource; 4057F0, the resource busy query (a native of the
// resource busy framework, entered as a genuine child), returns its busy
// count. The scan stops at the first resource with a positive count. The
// owner's count is re-read after each reference.
namespace resource_references {
namespace owner {inline constexpr std::uint32_t count=0x10u,references=0x14u;}
namespace reference {inline constexpr std::uint32_t size=0x10u,kind=0x0u,offset=0x4u,element=0x6u,id=0xcu;}
namespace holder_table {inline constexpr std::uint32_t holders=0x4u;}
namespace holder {inline constexpr std::uint32_t size=0x20u,resource=0x10u;}
inline constexpr std::uint32_t hardware_kind=2u,whole_structure=0xffffu,block_skip=8u;
enum class Access {count,references,id,kind,element,offset,element_offset,holders,resource,recount};
enum class Exit {empty,idle,busy};
// Frame: block() reads the block argument; begin_scan() is the loop's
// register save; busy(resource) is the 4057F0 call.
template<class Memory,class Frame> Exit any_busy(Memory &memory,Frame &frame,std::uint32_t owner_address) {
    auto count=memory.load32(Access::count,owner_address+owner::count);
    const auto structures=frame.block()+block_skip;
    if(signed_value(count)<=0)return Exit::empty;
    frame.begin_scan();
    for(std::uint32_t index=0;signed_value(index)<signed_value(count);++index) {
        const auto entry=memory.load32(Access::references,owner_address+owner::references)+index*reference::size;
        if(memory.load32(Access::id,entry+reference::id)!=0xffffffffu &&
            memory.load8(Access::kind,entry+reference::kind)==hardware_kind) {
            const auto element=memory.load16(Access::element,entry+reference::element);
            std::uint32_t holder_address;
            if(element==whole_structure)holder_address=memory.load16(Access::offset,entry+reference::offset)+structures;
            else {
                const auto table=memory.load16(Access::element_offset,entry+reference::offset)+structures;
                holder_address=(element<<5)+memory.load32(Access::holders,table+holder_table::holders);
            }
            if(const auto resource=memory.load32(Access::resource,holder_address+holder::resource))
                if(signed_value(frame.busy(resource))>0)return Exit::busy;
        }
        count=memory.load32(Access::recount,owner_address+owner::count);
    }
    return Exit::idle;
}
} // namespace resource_references

// ------------------------------------------------------ collision model ray
// 1F8200. Tests a ray against every collision BSP of a posed collision
// model. The model instance (the first argument) holds the collision model
// definition (+8), its region/permutation selection (+C), an optional
// selection remap (+4), and the posed node matrices (+10: real_matrix4x3,
// 0x34 bytes per node). The BSPs are walked with the region iterator of
// animation_channel_iterator (1F7E90 begins it, 1F7F60 steps it): its
// groups are the model's regions, its sub-tables their permutations and its
// entries their BSPs, each entry's record id being the BSP's node.
// For each BSP:
//   - the node's matrix is inverted into the routine's local matrix (144090,
//     a native of the affine-inverse framework, entered as a genuine child);
//   - the ray's origin and vector are brought into the node's space by that
//     inverse: each is first scaled by the inverse's scale unless the scale
//     is exactly 1.0; the origin is rotated and translated, the vector only
//     rotated;
//   - the query block is filled for the BSP test (1E6980, the BSP segment
//     traversal, a genuine child): the caller's test flags, the BSP, the
//     local origin and vector, the result's BSP output (from +8), and the
//     traversal's leaf state (-1, 0, -1) and surface bitmap (0, 0);
//   - the result's fraction is floored at 0 and its record count cleared,
//     and the traversal runs from the root node over [0, limit], the limit
//     being the fraction clamped to [0, 1];
//   - a hit records the BSP's packed iterator position, its node and its
//     region in the result.
// The result's fraction starts as the XBE constant at 45695C; the BSP test
// lowers it on each nearer hit. The routine returns whether any BSP was hit.
//
// The original keeps its local matrix, the local ray, the query block and the
// iterator in its own stack frame (the workspace, whose base the boundary
// gives); those stores, and the constant and matrix re-reads, are part of the
// original's access order. The ray arithmetic keeps the original's operand
// order (each component sums its products in its own order, below).
namespace collision_model_ray {
namespace instance {inline constexpr std::uint32_t region_remap=0x4u,model=0x8u,selection=0xcu,node_matrices=0x10u;}
// real_matrix4x3: scale, then the forward, left and up rows, then the position.
namespace matrix {
inline constexpr std::uint32_t size=0x34u,scale=0x0u,forward=0x4u,left=0x10u,up=0x1cu,position=0x28u;
}
// The result: the hit BSP's packed iterator position (+0), its node (+4)
// and region (+6) words, then the BSP test's output block (+8, its fraction
// first; its +18 is the record count).
namespace result {inline constexpr std::uint32_t position=0x0u,node=0x4u,region=0x6u,bsp_output=0x8u,record_count=0x20u;}
// The BSP test's 0x28-byte query block.
namespace query {
inline constexpr std::uint32_t flags=0x0u,bsp=0x4u,bitmap_limit=0x8u,bitmap=0xcu,origin=0x10u,vector=0x14u,output=0x18u,
    previous_leaf=0x1cu,previous_kind=0x20u,plane=0x24u;
}
// The routine's local block, from the frame base.
namespace workspace {
inline constexpr std::uint32_t limit=0x14u,matrix=0x18u,vector=0x4cu,origin=0x58u,query=0x64u,iterator=0x8cu;
}
namespace constant {inline constexpr std::uint32_t initial_fraction=0x45695cu,one=0x46a0d0u;}
inline constexpr std::uint32_t none=0xffffffffu,root_node=0u,node_mask=0xffu;
enum class Argument {instance,flags,origin,vector};
enum class Access {
    initial_fraction,fraction_reset,region_remap,selection,model,
    position,node_matrices,scale,origin_one,origin_x,origin_y,origin_z,
    left_x,forward_x,left_y,vector_one,up_x,position_x,local_origin_x,up_y,forward_y,position_y,local_origin_y,
    up_z,left_z,forward_z,vector_z,vector_x,vector_y,position_z,local_origin_z,
    forward_x_again,bsp,up_x_again,local_vector_x,left_y_again,forward_y_again,left_z_again,forward_z_again,
    fraction,local_vector_y,flags_store,bsp_store,local_vector_z,bitmap_limit_store,bitmap_store,
    origin_store,vector_store,output_store,floor_store,record_count_store,previous_leaf_store,previous_kind_store,plane_store,
    limit_zero_store,limit_one,limit_one_store,limit_store,limit,
    region,hit_position_store,hit_node_store,hit_region_store
};
// The values the original holds in XMM0..XMM7 at the BSP test call; each is
// a scalar over zeroed upper lanes. The boundary hands them to the child.
struct TestRegisters {
    float fraction;      // XMM0, as read from the result
    float bound;         // XMM1: 0 when the fraction is below zero, else the 1.0 compared
    float floor;         // XMM2: the fraction floored at 0
    float vector[3];     // XMM3..XMM5: the ray vector (scaled)
    float vector_z_up_x; // XMM6: the last product of the local vector's x
    float local_origin_z;// XMM7
    bool below_zero;     // the flags are the last COMISS: 0 against the fraction, or the fraction against 1.0
};
struct Result {bool found;};
// Frame: the stack arguments, the frame slots and the children.
//   instance() reads the first argument at entry; result() reads the fifth
//   between the original's register saves; argument(a) re-reads one inside
//   the loop; workspace() is the local block's base;
//   begin_iteration(iterator, model, selection, remap) is the 1F7E90 call
//   (the found flag, a frame byte, is cleared between its argument pushes
//   and the call); next(iterator) is 1F7F60; invert(node, matrices, local)
//   is 144090 on matrices + node * 0x34; test(query, limit, registers) is
//   1E6980 (query, root node, 0, limit); mark_found() sets the found flag.
template<class Memory,class Frame> Result cast(Memory &memory,Frame &frame) {
    using namespace sse;
    const auto load=[&](Access a,std::uint32_t address){return bits_to_float(memory.load32(a,address));};
    const auto store=[&](Access a,std::uint32_t address,float value){memory.store32(a,address,float_to_bits(value));};
    Result outcome{};
    const auto model_instance=frame.instance();
    const auto initial=load(Access::initial_fraction,constant::initial_fraction);
    const auto output=frame.result();
    const auto fraction_slot=output+result::bsp_output;
    store(Access::fraction_reset,fraction_slot,initial);
    const auto remap=memory.load32(Access::region_remap,model_instance+instance::region_remap);
    const auto selection=memory.load32(Access::selection,model_instance+instance::selection);
    const auto model=memory.load32(Access::model,model_instance+instance::model);
    const auto base=frame.workspace();
    const auto local=base+workspace::matrix,local_origin=base+workspace::origin,local_vector=base+workspace::vector;
    const auto block=base+workspace::query,iterator=base+workspace::iterator,limit_slot=base+workspace::limit;
    frame.begin_iteration(iterator,model,selection,remap);
    for(bool more=frame.next(iterator);more;more=frame.next(iterator)) {
        namespace channel=animation_channel_iterator::iterator;
        const auto position=memory.load32(Access::position,iterator+channel::position);
        const auto matrices=memory.load32(Access::node_matrices,frame.argument(Argument::instance)+instance::node_matrices);
        const auto node=position&node_mask;
        frame.invert(node,matrices,local);
        // The origin, scaled unless the inverse's scale is exactly 1.0
        // (UCOMISS: equal and ordered); scale * coordinate.
        const auto origin=frame.argument(Argument::origin);
        const auto scale=load(Access::scale,local+matrix::scale);
        const bool origin_unscaled=quiet_ordered_equal(scale,load(Access::origin_one,constant::one));
        auto px=load(Access::origin_x,origin),py=load(Access::origin_y,origin+4u),pz=load(Access::origin_z,origin+8u);
        if(!origin_unscaled){px=multiply(scale,px);py=multiply(scale,py);pz=multiply(scale,pz);}
        // local origin.x = ((left.x*py + forward.x*px) + pz*up.x) + position.x
        const auto left_x=load(Access::left_x,local+matrix::left);
        const auto forward_x_px=multiply(load(Access::forward_x,local+matrix::forward),px);
        const auto left_y=load(Access::left_y,local+matrix::left+4u);
        const auto vector=frame.argument(Argument::vector);
        // The vector's scale test, made separately (the constant re-read).
        const bool vector_unscaled=quiet_ordered_equal(scale,load(Access::vector_one,constant::one));
        auto x=add(multiply(left_x,py),forward_x_px);
        x=add(x,multiply(pz,load(Access::up_x,local+matrix::up)));
        x=add(x,load(Access::position_x,local+matrix::position));
        const auto left_y_py=multiply(left_y,py);
        store(Access::local_origin_x,local_origin,x);
        // local origin.y = ((up.y*pz + left.y*py) + forward.y*px) + position.y
        const auto up_y=load(Access::up_y,local+matrix::up+4u);
        auto y=add(multiply(up_y,pz),left_y_py);
        y=add(y,multiply(load(Access::forward_y,local+matrix::forward+4u),px));
        y=add(y,load(Access::position_y,local+matrix::position+4u));
        store(Access::local_origin_y,local_origin+4u,y);
        // local origin.z = ((up.z*pz + left.z*py) + forward.z*px) + position.z;
        // the vector is read in between.
        const auto up_z=load(Access::up_z,local+matrix::up+8u);
        auto z=multiply(up_z,pz);
        const auto left_z_py=multiply(load(Access::left_z,local+matrix::left+8u),py);
        const auto forward_z=load(Access::forward_z,local+matrix::forward+8u);
        z=add(z,left_z_py);
        auto vz=load(Access::vector_z,vector+8u);
        const auto forward_z_px=multiply(forward_z,px);
        auto vx=load(Access::vector_x,vector);
        z=add(z,forward_z_px);
        auto vy=load(Access::vector_y,vector+4u);
        z=add(z,load(Access::position_z,local+matrix::position+8u));
        store(Access::local_origin_z,local_origin+8u,z);
        // The vector, scaled unless the scale is exactly 1.0; coordinate * scale.
        if(!vector_unscaled){vx=multiply(vx,scale);vy=multiply(vy,scale);vz=multiply(vz,scale);}
        // local vector.x = (left.x*vy + forward.x*vx) + vz*up.x
        const auto forward_x=load(Access::forward_x_again,local+matrix::forward);
        const auto test_flags=frame.argument(Argument::flags);
        const auto bsp=memory.load32(Access::bsp,iterator+channel::record);
        auto dx=multiply(left_x,vy);
        dx=add(dx,multiply(forward_x,vx));
        const auto vz_up_x=multiply(vz,load(Access::up_x_again,local+matrix::up));
        dx=add(dx,vz_up_x);
        store(Access::local_vector_x,local_vector,dx);
        // local vector.y = (up.y*vz + left.y*vy) + forward.y*vx
        const auto left_y_vy=multiply(load(Access::left_y_again,local+matrix::left+4u),vy);
        auto dy=add(multiply(up_y,vz),left_y_vy);
        dy=add(dy,multiply(load(Access::forward_y_again,local+matrix::forward+4u),vx));
        // local vector.z = (up.z*vz + left.z*vy) + forward.z*vx
        const auto left_z_vy=multiply(load(Access::left_z_again,local+matrix::left+8u),vy);
        auto dz=add(multiply(up_z,vz),left_z_vy);
        dz=add(dz,multiply(load(Access::forward_z_again,local+matrix::forward+8u),vx));
        // The query block, interleaved with the fraction's floor as in the original.
        const auto fraction=load(Access::fraction,fraction_slot);
        store(Access::local_vector_y,local_vector+4u,dy);
        memory.store32(Access::flags_store,block+query::flags,test_flags);
        memory.store32(Access::bsp_store,block+query::bsp,bsp);
        store(Access::local_vector_z,local_vector+8u,dz);
        memory.store16(Access::bitmap_limit_store,block+query::bitmap_limit,0u);
        memory.store32(Access::bitmap_store,block+query::bitmap,0u);
        memory.store32(Access::origin_store,block+query::origin,local_origin);
        memory.store32(Access::vector_store,block+query::vector,local_vector);
        memory.store32(Access::output_store,block+query::output,fraction_slot);
        // COMISS 0, fraction: a NaN fraction is kept, unfloored.
        const bool below_zero=ordered_greater(0.0f,fraction);
        const auto floor=below_zero?0.0f:fraction;
        store(Access::floor_store,fraction_slot,floor);
        memory.store32(Access::record_count_store,output+result::record_count,0u);
        memory.store32(Access::previous_leaf_store,block+query::previous_leaf,none);
        memory.store8(Access::previous_kind_store,block+query::previous_kind,0u);
        memory.store32(Access::plane_store,block+query::plane,none);
        // The limit: 0 below zero, else 1.0 unless the fraction is not above
        // it (a NaN fraction passes through).
        float bound=0.0f;
        if(below_zero)store(Access::limit_zero_store,limit_slot,0.0f);
        else {
            bound=load(Access::limit_one,constant::one);
            store(Access::limit_one_store,limit_slot,bound);
            if(!ordered_greater(fraction,bound))store(Access::limit_store,limit_slot,fraction);
        }
        const auto limit=memory.load32(Access::limit,limit_slot);
        const TestRegisters registers{fraction,bound,floor,{vx,vy,vz},vz_up_x,z,below_zero};
        if(!frame.test(block,limit,registers))continue;
        // A hit: the BSP's position, node and region (the iterator's group byte).
        const auto region=memory.load8(Access::region,iterator+channel::group);
        memory.store32(Access::hit_position_store,output+result::position,position);
        memory.store16(Access::hit_node_store,output+result::node,node);
        memory.store16(Access::hit_region_store,output+result::region,region);
        frame.mark_found();
        outcome.found=true;
    }
    return outcome;
}
} // namespace collision_model_ray

// ------------------------------------------------------------ bitmap texel
// 138D90. The address of the texel at a texture coordinate (u, v) in one
// mip level of a bitmap, chosen by a level of detail (1.0 the finest).
//   - A bitmap without resident pixels (+54 zero) has no address (-1).
//   - The level: below a detail of 1.0, with mip levels (+14, a positive
//     word), it is (1.0 - detail) * mip count converted by the x87 (FISTP,
//     rounded by the control word); otherwise level 0.
//   - Each extent (width +4, height +6) is the dimension word shifted right
//     by the level as a signed word (SAR r16, the count masked to 5 bits),
//     at least 1; a block-compressed bitmap (flags +E bit 1) rounds each up
//     to a multiple of 4 (an extent of 0x7FFD..0x7FFF becomes 0x8000).
//   - The texel: extent * coordinate - 0.5, converted by the x87, wrapped
//     into the extent (taken as a signed word: 0x8000 is -32768): a power
//     of two masks, any other extent takes IDIV's remainder twice,
//     ((t % extent) + extent) % extent. No extent is zero (at least 1,
//     rounded at most to 0x8000), so the divisions cannot fault.
//   - The level's offset comes from the child for the bitmap type (+A): 2D
//     textures 138510, 3D textures 1385D0, cube maps (any other) 1386E0.
//   - The address is the child 138C20's, from the pixels, the first
//     partition's size (+34), the level offset, the texel, the level's
//     extents (re-read from their frame slots), the format (+C) and flags.
// The four children are genuine children (the routine's callers' unit
// routes). Every intermediate passes through the routine's frame slots (the
// boundary gives their addresses): the level spill shares the width slot,
// the coordinate spills share the coordinate argument's slot. Those stores
// and their re-reads are part of the access order.
namespace bitmap_texel {
namespace bitmap {
inline constexpr std::uint32_t width=0x4u,height=0x6u,type=0xau,format=0xcu,flags=0xeu,mipmap_count=0x14u,
    partition_size=0x34u,pixels=0x54u;
}
inline constexpr std::uint32_t compressed=0x2u,no_address=0xffffffffu;
enum class Type : std::int32_t {texture_2d=0,texture_3d=1};  // any other: a cube map
namespace constant {inline constexpr std::uint32_t one=0x46a0d0u,half=0x46a0ccu;}
enum class Access {
    pixels,one,mipmap_count,level_spill,level_load,level_store,level_reload,
    width,width_store,width_one_store,width_one_reload,flags,width_round_store,
    height,height_store,height_one_store,height_one_reload,height_round_store,
    coordinate,half,
    u,u_spill,u_masked_load,u_masked_store,u_masked,u_divided_load,u_divided_store,u_divided,height_reload,
    v,v_spill,v_masked_load,v_masked_store,v_masked,v_divided_load,v_divided_store,v_divided,type
};
// One coordinate's roles: the coordinate, its spill, and the conversion's
// load, store and read on the masked (power of two) and divided paths.
struct CoordinateRoles {Access coordinate,spill,masked_load,masked_store,masked,divided_load,divided_store,divided;};
// The routine's frame slots: the height and the width (which first hold the
// level's FISTP and its spill), the x87 conversions' integer, and the
// coordinate argument (which the coordinates' spills overwrite).
struct Slots {std::uint32_t height,width,integer,coordinate;};
// The values the original leaves in registers at the level-offset child
// call (the boundary gives them to the child).
struct Residue {
    std::uint32_t y_divisor;  // the height, sign-extended from its word
    std::uint32_t y_wrap;     // the y wrap's last value: the mask (power of two) or IDIV's remainder
    std::uint32_t v_spill;    // the v product's float bits
    std::uint32_t half;       // the 0.5 constant's bits
};
struct Result {bool resident;std::uint32_t address;};
// Frame: slots(); x87_push() after each FLD's read (a full x87 stack faults
// there, as the original) and x87_pop(rounded_up) after each FISTP's store
// (the caller's x87 status model); level_offset(type, level, x, y, residue)
// and texel_address(bitmap, offset, x, y) are the children.
template<class Memory,class Frame> Result address(Memory &memory,Frame &frame,std::uint32_t bitmap_address,float detail) {
    using namespace sse;
    const auto load_float=[&](Access a,std::uint32_t at){return bits_to_float(memory.load32(a,at));};
    if(!memory.load32(Access::pixels,bitmap_address+bitmap::pixels))return {false,no_address};
    const auto slots=frame.slots();
    // FLD m32 then FISTP m32: a spilled float to an integer slot.
    const auto through_x87=[&](Access load,std::uint32_t from,Access store,std::uint32_t to) {
        const auto value=x87::load_single(memory.load32(load,from));
        frame.x87_push();
        const auto stored=x87::store_int32(value);
        memory.store32(store,to,std::uint32_t(stored.bits));
        frame.x87_pop(stored.rounded_up);
    };
    // The level.
    std::uint32_t level=0;
    const auto one=load_float(Access::one,constant::one);
    if(ordered_greater(one,detail)) {
        const auto levels=memory.load16(Access::mipmap_count,bitmap_address+bitmap::mipmap_count);
        if(std::int16_t(levels)>0) {
            const auto product=multiply(subtract(one,detail),from_int(signed_value(widen16(levels))));
            memory.store32(Access::level_spill,slots.width,float_to_bits(product));
            through_x87(Access::level_load,slots.width,Access::level_store,slots.height);
            level=memory.load32(Access::level_reload,slots.height);
        }
    }
    // The level's extents, each kept in its frame slot.
    const auto shifted=[&](std::uint32_t dimension) {
        return std::int16_t(std::int32_t(std::int16_t(dimension))>>(level&31u));
    };
    const auto round_up=[](std::uint32_t extent){return extent+((0u-extent)&3u);};
    std::uint32_t width;
    if(const auto w=shifted(memory.load16(Access::width,bitmap_address+bitmap::width));w>1) {
        width=std::uint16_t(w);memory.store32(Access::width_store,slots.width,width);
    } else {
        memory.store32(Access::width_one_store,slots.width,1u);width=memory.load32(Access::width_one_reload,slots.width);
    }
    const bool block_compressed=(memory.load16(Access::flags,bitmap_address+bitmap::flags)&compressed)!=0;
    if(block_compressed){width=round_up(width);memory.store32(Access::width_round_store,slots.width,width);}
    std::uint32_t height;
    if(const auto h=shifted(memory.load16(Access::height,bitmap_address+bitmap::height));h>1) {
        height=std::uint16_t(h);memory.store32(Access::height_store,slots.height,height);
    } else {
        memory.store32(Access::height_one_store,slots.height,1u);height=memory.load32(Access::height_one_reload,slots.height);
    }
    if(block_compressed){height=round_up(height);memory.store32(Access::height_round_store,slots.height,height);}
    // The texel: extent * coordinate - 0.5 through the x87, wrapped.
    const auto coordinate=memory.load32(Access::coordinate,slots.coordinate);
    const auto half=memory.load32(Access::half,constant::half);
    // last: the wrap's final EDX (the mask, or the second remainder).
    struct Wrapped {std::uint32_t texel,last,spill;bool power_of_two;};
    const auto wrap=[&](std::uint32_t extent,std::uint32_t c_address,const CoordinateRoles &role) {
        const auto divisor=widen16(extent);
        const bool power_of_two=(divisor&(divisor-1u))==0;
        const auto spill=float_to_bits(subtract(multiply(from_int(signed_value(divisor)),load_float(role.coordinate,c_address)),
            bits_to_float(half)));
        memory.store32(role.spill,slots.coordinate,spill);
        if(power_of_two) {
            through_x87(role.masked_load,slots.coordinate,role.masked_store,slots.integer);
            return Wrapped{(extent-1u)&memory.load32(role.masked,slots.integer),divisor-1u,spill,true};
        }
        // IDIV twice (truncating): the remainder, then the remainder of it
        // plus the extent.
        through_x87(role.divided_load,slots.coordinate,role.divided_store,slots.integer);
        const auto t=std::int64_t(signed_value(memory.load32(role.divided,slots.integer))),e=std::int64_t(signed_value(divisor));
        const auto first=std::uint32_t(std::int32_t(t%e));
        const auto second=std::uint32_t(std::int32_t(std::int64_t(signed_value(first+divisor))%e));
        return Wrapped{second,second,spill,false};
    };
    using A=Access;
    const auto x=wrap(width,coordinate,{A::u,A::u_spill,A::u_masked_load,A::u_masked_store,A::u_masked,A::u_divided_load,
        A::u_divided_store,A::u_divided});
    // A divided x re-reads the height from its slot (the division used it).
    if(!x.power_of_two)height=memory.load32(A::height_reload,slots.height);
    const auto y=wrap(height,coordinate+4u,{A::v,A::v_spill,A::v_masked_load,A::v_masked_store,A::v_masked,A::v_divided_load,
        A::v_divided_store,A::v_divided});
    // The level's offset, then the texel's address.
    const auto type=std::int32_t(widen16(memory.load16(Access::type,bitmap_address+bitmap::type)));
    const auto offset=frame.level_offset(type,level,x.texel,y.texel,Residue{widen16(height),y.last,y.spill,half});
    return {true,frame.texel_address(bitmap_address,offset,x.texel,y.texel)};
}
} // namespace bitmap_texel

// ------------------------------------------------------------ effect update
// 179CD0. Updates every live effect for one tick. The effects are the
// datums (0x190 bytes) of the data array at 4F720C; the tick's time step is
// the float at +4 of the game time globals (51DF54), kept in a slot of the
// routine's frame and re-read for each effect. For each effect, in index
// order:
//   - its attachment update (17C370) runs, then its visibility update
//     (17C5C0); either returns true when the effect is gone, ending its tick;
//   - otherwise an effect marked retiring (flags +2, bit 6) whose particle
//     systems are all gone (+FC, -1) is deleted (17A380), and any other
//     effect advances by the time step (17C140). The original tests the
//     datum's address after that check: a null address (which no real array
//     has) skips the advance as well.
// The first effect is found by the data array's iteration (16EA60, the
// effect framework's native slot scan, from index 0); each next one by the
// inline scan (data_array::first_in_use) from the effect's index + 1, in the
// array as last read from 4F720C (re-read after a tick that did not delete
// or skip its effect). A handle is the datum's salt word over its index;
// the walk ends at the handle -1. The five children are genuine children:
// 16EA60, group E's 17C370, 17C5C0 and 17C140, and 17A380.
namespace effect_update {
namespace globals {inline constexpr std::uint32_t game_time=0x51df54u,effects=0x4f720cu;}
namespace game_time {inline constexpr std::uint32_t time_step=0x4u;}
namespace effect {inline constexpr std::uint32_t size=0x190u,flags=0x2u,particle_systems=0xfcu;}
inline constexpr std::uint32_t retiring=0x40u,none=0xffffffffu;
enum class Access {
    game_time,time_step,effects,first_size,first_data,first_salt,
    array,data,flags,particle_systems,array_reread,
    first_unallocated,in_use_bits,in_use_word,next_size,next_data,next_salt
};
// How the walk found its handle. The original leaves these in scratch
// registers, which the next child call and the exit see.
struct Found {
    std::uint32_t handle;                     // the effect's handle, or -1
    bool inline_scan;                         // else the first, from the slot scan (16EA60)
    std::uint32_t index;                      // the slot scan's index; or the inline scan's stop (the index found, the limit, or its start)
    std::uint32_t array;                      // the inline scan's array
    data_array::Scan scan;                    // the inline scan
    std::optional<std::uint32_t> datum_offset,data;  // a handle composed: size * index, and the array's data
};
// The effect as the retirement check read it (the original's registers at
// the delete or advance call).
struct Checked {std::uint32_t array,data,datum,flags;};
// Frame: keep_time_step(step) stores the step in the frame slot;
// first_in_use(effects) is 16EA60 from index 0; begin_walk() saves the
// loop's registers; attachments(found), visibility(handle), destroy(handle,
// checked) and advance(handle, checked) are 17C370, 17C5C0, 17A380 and
// 17C140 (the time step re-read from its slot); end(found) is the exit.
template<class Memory,class Frame> void update(Memory &memory,Frame &frame) {
    const auto game_time_globals=memory.load32(Access::game_time,globals::game_time);
    const auto time_step=memory.load32(Access::time_step,game_time_globals+game_time::time_step);
    const auto effects=memory.load32(Access::effects,globals::effects);
    frame.keep_time_step(time_step);
    // A handle: the datum's salt word (sign-extended, its upper half shifted
    // out) over the index.
    const auto compose=[&](Found &f,std::uint32_t array,std::uint32_t index,Access size,Access data,Access salt) {
        const auto offset=memory.load32(size,array+data_array::datum_size)*index;
        f.datum_offset=offset;
        f.data=memory.load32(data,array+data_array::data);
        f.handle=(widen16(memory.load16(salt,offset+*f.data))<<16)|index;
    };
    Found found{none,false,frame.first_in_use(effects),0u,{},{},{}};
    if(found.index!=none)compose(found,effects,found.index,Access::first_size,Access::first_data,Access::first_salt);
    if(found.handle==none){frame.end(found);return;}
    frame.begin_walk();
    for(;;) {
        const auto handle=found.handle;
        auto array=found.array;
        bool reread=true;
        if(!frame.attachments(found) && !frame.visibility(handle)) {
            Checked checked{};
            checked.array=array=memory.load32(Access::array,globals::effects);
            checked.data=memory.load32(Access::data,array+data_array::data);
            checked.datum=(handle&0xffffu)*effect::size+checked.data;
            checked.flags=memory.load8(Access::flags,checked.datum+effect::flags);
            if((checked.flags&retiring) &&
                memory.load32(Access::particle_systems,checked.datum+effect::particle_systems)==none) {
                frame.destroy(handle,checked);reread=false;
            } else if(!checked.datum)reread=false;
            else frame.advance(handle,checked);
        }
        if(reread)array=memory.load32(Access::array_reread,globals::effects);
        // The next effect: the inline scan from this one's index + 1.
        Found next{none,true,(handle&0xffffu)+1u,array,{},{},{}};
        next.scan=data_array::first_in_use(memory,array,next.index,Access::first_unallocated,Access::in_use_bits,Access::in_use_word);
        if(next.scan.index){next.index=*next.scan.index;compose(next,array,next.index,Access::next_size,Access::next_data,Access::next_salt);}
        else if(next.scan.bits)next.index=next.scan.limit;
        found=next;
        if(found.handle==none){frame.end(found);return;}
    }
}
} // namespace effect_update
} // namespace halo2::native_live_support
