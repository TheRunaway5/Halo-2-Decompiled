#pragma once
// Native TU1.5 page scan, entered at 1F33C0 (physics live support unit): a
// cursor's first nonzero entry over its pages.
//
//   page_scan::scan   1F33C0(EDI = cursor)
//
// The cursor (+0 its handle, +4 its page, +6 its index) starts at page
// zero; on each page its index restarts at zero and 1F3460 (EAX the handle,
// ECX the frame's stride word, EDX the cursor's page) answers the page's
// table (+0 its count, +4 its base). An index within the count picks the
// entry at the index times the stride past the base (kept in the frame);
// a nonzero entry ends the scan, as does the last page (5).
//
// This is the game logic only. The game's memory is the retained TU1.5 bank,
// reached through a Memory object with typed loads and stores; every access
// names its role (Access), and the boundary adapter
// (native_tu15_page_scan.hpp) maps the role to the original instruction and
// owns the ABI of the child; the cursor and its page's address, which the
// original keeps in EDI and ESI across it, are a Kept, refreshed from the
// registers it returned.
#include "physics/runtime/native_tu15_object_tick_systems.hpp"
#include <cstdint>

namespace halo2::native_page_scan {
using native_object_update::at;
using native_object_update::signed_value;
using native_object_update::widen16;
namespace cursor {inline constexpr std::uint32_t handle=0u,page=4u,index=6u;}
namespace table {inline constexpr std::uint32_t count=0u,base=4u;}
inline constexpr std::uint32_t last_page=5u;

namespace page_scan {
enum class Access {page_reset,handle,index_reset,index,count,stride,base,entry_store,page,page_store};
enum class Outcome {found,exhausted};
struct Kept {std::uint32_t cursor,page;};   // EDI, ESI (the cursor's page word)
// Frame: local(k) the frame's words now; table(kept, handle) -> EAX 1F3460, refreshes kept.
template<class Memory,class Frame> Outcome scan(Memory &memory,Frame &frame,Kept &kept) {
    using enum Access;
    memory.store16(page_reset,kept.page,0u);
    for(;;) {
        const auto handle_value=memory.load32(handle,at(kept.cursor,cursor::handle));
        memory.store16(index_reset,at(kept.cursor,cursor::index),0u);
        const auto table_address=frame.table(kept,handle_value);
        const auto index_value=widen16(memory.load16(index,at(kept.page,cursor::index-cursor::page)));
        if(signed_value(index_value)<signed_value(memory.load32(count,at(table_address,table::count)))) {
            const auto offset=index_value*memory.load32(stride,frame.local(0u));
            const auto entry=offset+memory.load32(base,at(table_address,table::base));
            memory.store32(entry_store,frame.local(1u),entry);
            if(entry)return Outcome::found;
        }
        const auto page_value=memory.load16(page,kept.page);
        if(std::int16_t(page_value)>=std::int16_t(last_page))return Outcome::exhausted;
        memory.store16(page_store,kept.page,page_value+1u);
    }
}
} // namespace page_scan
} // namespace halo2::native_page_scan
