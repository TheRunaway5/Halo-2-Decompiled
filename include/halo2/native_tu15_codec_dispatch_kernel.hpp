#pragma once
#include <cstdint>

namespace halo2::native_codec_dispatch_detail {

// This part of the codec owns only the selection. A tail result names the
// genuine next owner; that owner still owns the live caller and its return.
struct Tail {
    std::uint32_t instruction{},target{};
};

template<class View>Tail format(View& view) {
    view.load_format();
    for(unsigned value=0;value<5;++value)
        if(view.format_matches(value))return view.tail_format(value);
    return view.return_format();
}

template<class View>Tail field(View& view) {
    view.load_field();
    for(unsigned kind=0;kind<2;++kind) {
        if(!view.field_matches(kind))continue;
        view.load_variants(kind);
        // The wide TEST precedes the byte read. The byte load retains its
        // flags, then the low-byte TEST publishes the final tail flags.
        const bool wide=view.wide_nonzero();
        const bool byte=view.test_low_byte();
        return view.tail_field(kind*4u+(wide?0u:2u)+(byte?0u:1u));
    }
    if(view.field_matches(2u))return view.tail_field(8u);
    return view.return_field();
}

} // namespace halo2::native_codec_dispatch_detail
