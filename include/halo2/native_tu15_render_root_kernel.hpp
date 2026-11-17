#pragma once

#include <cstdint>
#include <stdexcept>

namespace halo2::native_render_root_detail {
struct Object { std::uint32_t identity; };
struct Rows { std::uint32_t address; };
struct Slots { std::uint32_t index; };
struct Selection { Object object; Rows rows; };

inline constexpr Object missing{0xffffffffu};
inline constexpr auto object_pool_address = 0x4ecbd0u;
inline constexpr bool exists(Object object) { return object.identity != missing.identity; }
inline constexpr std::uint32_t object_index(Object object) { return object.identity & 0xffffu; }
inline constexpr Slots row_slots(std::uint32_t index) { return {index * 3u}; }
inline constexpr std::uint32_t body_address(Rows rows, Slots slot) { return rows.address + slot.index * 4u + 8u; }
inline constexpr std::uint32_t parent_address(Object body) { return body.identity + 0x14u; }

// The selected object is the last valid object reached. Acquire each body
// and parent separately so a storage owner can preserve their fault order.
template<class View> decltype(auto) find(View& view) {
    auto object = view.start();
    if (view.has_object(object)) {
        const auto rows = view.acquire_rows();
        do {
            const auto slot = view.select(object);
            const auto body = view.acquire_body(rows, slot);
            object = view.acquire_parent(body);
        } while (view.has_parent(object));
    }
    return view.finish();
}

// Private facade for comparison with the existing renderer value algorithm.
// Keep that public facade's bounded cycle diagnostic separate from the
// generated source controller's original loop/exception contract.
template<class Read> Selection value(Read read, Object initial) {
    struct View {
        Read& read;
        Object initial;
        Selection result{missing,{0}};
        unsigned depth{};
        Object start() const { return initial; }
        bool has_object(Object object) const { return exists(object); }
        Rows acquire_rows() { return result.rows = {read(read(object_pool_address) + 0x44u)}; }
        Slots select(Object object) {
            if (depth++ == 65536u) throw std::runtime_error("Native render owner has a cyclic object-parent chain");
            result.object = object;
            return row_slots(object_index(object));
        }
        Object acquire_body(Rows rows, Slots slot) { return {read(body_address(rows, slot))}; }
        Object acquire_parent(Object body) { return {read(parent_address(body))}; }
        bool has_parent(Object object) const { return exists(object); }
        Selection finish() const { return result; }
    } view{read, initial};
    return find(view);
}
} // namespace halo2::native_render_root_detail
