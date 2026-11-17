#pragma once
// Native TU1.5 geometry cache system: the count of geometry reads still in
// flight, entered at 1306B0 (geometry cache frame unit).
//
//   pending_reads::count     1306B0  how many geometry cache reads have not
//                                    completed yet
//
// The geometry cache keeps one datum per geometry block read that it has
// issued ("xbox geometry cache", [4F2D5C]); the asynchronous read sets the
// datum's `ready` byte when the block is in memory (130980 and 16D390 read
// and write the same datums). The count is taken only while the predicted
// geometry array ("xbox predicted geometry", [4F2D60]) is valid, that is,
// while a map's geometry caches exist.
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores. Every access
// names its role (the Access enum); the boundary adapter
// (native_tu15_geometry_cache_count.hpp) maps the role to the original
// instruction for the diagnostic cursor and keeps the original's registers
// and flags. Registers, flags and stack slots belong to the adapter, not to
// this file.
//
// What stays exact, because translated neighbours observe it: the memory
// accesses happen in the original order and width (a faulting read must be
// the original's first fault, and verification replays every read in
// order), and address arithmetic is 32-bit and wraps as the original's does.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstddef>
#include <cstdint>

namespace halo2::native_geometry_cache {
// The game's data_array and its inlined in-use iteration are shared with the
// object update systems.
using native_object_update::DataArray;
using native_object_update::Search;
using native_object_update::SearchEnd;
using native_object_update::at;
using native_object_update::datum_of;
using native_object_update::next_in_use;
using native_object_update::none;

// ------------------------------------------------------------- game layouts
// Globals at fixed TU1.5 addresses.
namespace globals {
inline constexpr std::uint32_t geometry_cache=0x4f2d5cu;      // DataArray* of GeometryRead
inline constexpr std::uint32_t predicted_geometry=0x4f2d60u;  // DataArray* of predicted geometry requests
}

// A geometry cache datum (12 bytes): one geometry block read in flight.
struct GeometryRead {
    std::uint16_t identifier;     // the datum's salt
    std::uint8_t ready;           // +2: set by the asynchronous read when it completes
    std::uint8_t priority;        // +3: the read's priority (raised by 130980)
    std::uint32_t job;            // +4: the asynchronous read job
    std::uint32_t block_header;   // +8: the geometry block's resource header
};
static_assert(sizeof(GeometryRead)==12 && offsetof(GeometryRead,ready)==2 && offsetof(GeometryRead,block_header)==8);

// ----------------------------------------------------------- pending reads
// 1306B0. While the predicted geometry array is valid, counts the in-use
// datums of the geometry cache whose read is not ready yet. The datums are
// visited in data array order; a datum whose address works out to zero ends
// the count (the original treats it as the end of the array). The result is
// the count, and how the walk ended, for the adapter.
namespace pending_reads {
enum class Access {
    predicted_array,predicted_valid,cache_array,
    scan_limit,scan_bits,scan_word,datum_size,datum_data,ready
};
enum class Exit {
    no_caches,      // the predicted geometry array is not valid: nothing is counted
    scan_ended,     // every in-use datum was visited (see `scan`)
    null_datum      // a datum's address was zero
};
struct Result {
    Exit exit;
    std::uint32_t pending;  // reads not ready yet
    Search scan;            // the last search: its end, when the scan ended
};
template<class Memory> Result count(Memory &memory) {
    using enum Access;
    const auto predicted=memory.load32(predicted_array,globals::predicted_geometry);
    if(!memory.load8(predicted_valid,at(predicted,offsetof(DataArray,valid))))return {Exit::no_caches,0,{}};
    const auto cache=memory.load32(cache_array,globals::geometry_cache);
    std::uint32_t pending=0;
    for(auto index=none;;) {
        const auto search=next_in_use(memory,cache,index,scan_limit,scan_bits,scan_word);
        if(search.end)return {Exit::scan_ended,pending,search};
        index=search.index;
        const auto read=datum_of(memory,cache,index,datum_size,datum_data);
        if(!read)return {Exit::null_datum,pending,search};
        if(memory.load8(ready,at(read,offsetof(GeometryRead,ready))))continue;
        ++pending;
    }
}
} // namespace pending_reads
} // namespace halo2::native_geometry_cache
