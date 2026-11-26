#pragma once
#include <cstdint>

namespace halo2::native_camera_basis {
inline constexpr std::uint32_t x=0u,y=4u,z=8u;
inline constexpr std::uint32_t one=0x46a0d0u,epsilon=0x46a0ecu,default_basis=0x474cd8u;

struct Basis {
    std::uint32_t direction,fallback,forward,perpendicular;
};

enum class Access {
    direction_x, direction_y, forward_x, forward_y,
    direction_length_y, direction_length_x, unit_scale, direction_length_store, direction_epsilon,
    direction_length_reload, direction_scale_x, direction_normal_x, direction_scale_y, direction_normal_y,
    fallback_x, fallback_y, fallback_copy_x, fallback_copy_y, direction_z,
    flip_read_x, flip_store_x, flip_read_y, flip_store_y,
    fallback_length_y, fallback_length_x, fallback_length_store, fallback_epsilon,
    fallback_length_reload, fallback_scale_x, fallback_normal_x, fallback_scale_y, fallback_normal_y,
    default_pointer, default_x, default_store_x, default_y, default_store_y,
    perpendicular_read_y, perpendicular_store_x, perpendicular_read_x, perpendicular_store_y,
    count,
};

// The original rereads each component after publishing it. In particular a
// source and output may overlap, and the perpendicular X store may alias forward X.
template<bool Fallback,class Memory,class Frame>
bool normalize(Memory &memory,Frame &frame,std::uint32_t forward) {
    using enum Access;
    frame.load_extended(memory.load32(Fallback?fallback_length_y:direction_length_y,forward+y));
    frame.load_extended(memory.load32(Fallback?fallback_length_x:direction_length_x,forward+x));
    if constexpr(!Fallback)frame.set_unit_scale(memory.load32(unit_scale,one));
    const auto length=frame.horizontal_length();
    const auto slot=frame.length_slot();
    memory.store32(Fallback?fallback_length_store:direction_length_store,slot,length);
    frame.absolute_length();
    if(frame.length_below_threshold(memory.load32(Fallback?fallback_epsilon:direction_epsilon,epsilon)))
        return false;

    const auto size=memory.load32(Fallback?fallback_length_reload:direction_length_reload,slot);
    const auto first=memory.load32(Fallback?fallback_scale_x:direction_scale_x,forward+x);
    const auto first_normalized=frame.template scale_first<Fallback>(size,first);
    memory.store32(Fallback?fallback_normal_x:direction_normal_x,forward+x,first_normalized);
    const auto second=memory.load32(Fallback?fallback_scale_y:direction_scale_y,forward+y);
    const auto second_normalized=frame.template scale_second<Fallback>(second);
    memory.store32(Fallback?fallback_normal_y:direction_normal_y,forward+y,second_normalized);
    return frame.nonzero_length();
}

template<class Memory,class Frame>
void build(Memory &memory,Frame &frame,const Basis &basis) {
    using enum Access;
    const auto direction_x_word=memory.load32(direction_x,basis.direction+x);
    memory.store32(forward_x,basis.forward+x,direction_x_word);
    const auto direction_y_word=memory.load32(direction_y,basis.direction+y);
    memory.store32(forward_y,basis.forward+y,direction_y_word);
    if(normalize<false>(memory,frame,basis.forward)) {
        frame.direction_used();
    } else {
        const auto x_word=memory.load32(fallback_x,basis.fallback+x);
        memory.store32(fallback_copy_x,basis.forward+x,x_word);
        const auto y_word=memory.load32(fallback_y,basis.fallback+y);
        memory.store32(fallback_copy_y,basis.forward+y,y_word);
        if(frame.flip_fallback(memory.load32(direction_z,basis.direction+z))) {
            const auto neg_x=frame.negate_for_flip(memory.load32(flip_read_x,basis.forward+x));
            memory.store32(flip_store_x,basis.forward+x,neg_x);
            const auto neg_y=frame.negate_for_flip(memory.load32(flip_read_y,basis.forward+y));
            memory.store32(flip_store_y,basis.forward+y,neg_y);
        }
        if(normalize<true>(memory,frame,basis.forward)) {
            frame.fallback_used();
        } else {
            frame.default_used();
            const auto address=memory.load32(default_pointer,default_basis);
            const auto default_x_word=memory.load32(default_x,address+x);
            memory.store32(default_store_x,basis.forward+x,default_x_word);
            const auto default_y_word=memory.load32(default_y,address+y);
            memory.store32(default_store_y,basis.forward+y,default_y_word);
        }
    }
    const auto perpendicular_x=frame.perpendicular_x(memory.load32(perpendicular_read_y,basis.forward+y));
    memory.store32(perpendicular_store_x,basis.perpendicular+x,perpendicular_x);
    const auto perpendicular_y=memory.load32(perpendicular_read_x,basis.forward+x);
    memory.store32(perpendicular_store_y,basis.perpendicular+y,perpendicular_y);
}
} // namespace halo2::native_camera_basis
