#pragma once
#include <cstdint>

namespace halo2::native_arena_initialize_detail {
// Actual TU1.5 126640 (the retained arena owner calls it canonical125280).
// The physical reservation child adds these two authored size components.
inline constexpr std::uint32_t extra_reservation_bytes=0x40000u;
inline constexpr std::uint32_t usable_reservation_bytes=0x3be000u;
inline constexpr std::uint32_t backing_words=(extra_reservation_bytes+usable_reservation_bytes)/4u;
inline constexpr std::uint32_t metadata_bytes=0x1288u;
inline constexpr std::uint32_t root_bytes=4u;
enum class Allocation { metadata,root };

struct Initialization {
    bool initialized_now{};
    std::uint32_t metadata{},root{};
};

template<class View>void clear_words(View&view,std::uint32_t count){
    for(std::uint32_t remaining=count;remaining;--remaining)view.clear_word();
}

template<class View>Initialization finish_root(View&view,Initialization result={}){
    result.root=view.resumed_pointer();
    view.publish_root(result.root);
    if(result.root)view.initialize_root(result.root);
    view.end_storage();view.finish();result.initialized_now=true;
    return result;
}

template<class View>Initialization finish_initialization(View&view){
    Initialization result;
    const auto next_offset=view.offset(Allocation::root);
    const auto next_base=view.base(Allocation::root);
    result.metadata=view.resumed_pointer();
    view.publish_metadata(result.metadata);
    result.root=next_base+next_offset;
    view.root_ready(result.root);
    const auto charge=view.root_charge_bytes();
    view.reserve_offset(Allocation::root,next_offset+charge);
    view.account_allocation(Allocation::root,charge);
    return finish_root(view,result);
}

template<class View>void initialize_metadata(View&view){
    const auto offset=view.offset(Allocation::metadata);
    const auto base=view.base(Allocation::metadata);
    view.metadata_ready(base+offset);
    view.reserve_offset(Allocation::metadata,offset+metadata_bytes);
    view.account_allocation(Allocation::metadata,metadata_bytes);
}

// A view owns storage, actual child calls and their observable boundaries.
// Allocation accounting consumes the real four-byte size input. Child-resumed
// metadata/root pointers are reacquired because their saved slots can alias.
template<class View>Initialization initialize(View&view){
    Initialization result;
    if(view.needs_initialization()){
        view.begin_storage();
        view.reserve_backing(usable_reservation_bytes,extra_reservation_bytes,0xffffffffu);
        clear_words(view,backing_words);
        view.initialize_savegames();

        initialize_metadata(view);
        return finish_initialization(view);
    }
    view.finish();
    return result;
}
}
