#pragma once
#include <cstdint>

namespace halo2::native_diagnostic_stream {

// The stream policy preserves the genuine formatter, output owner and flush
// owner. Views supply their lazy source acquisition and ABI transport.
template<class View>std::uint32_t print(View&view) {
    view.reserve_message();
    view.format_message();
    view.capture_output();
    const auto bytes=view.measure_message();
    view.append_message(bytes);
    return view.release_message();
}

template<class View>void flush(View&view) {
    view.flush_output();
}

template<class View>std::uint32_t measure(View&view) {
    view.open_text();
    std::uint32_t bytes{};
    while(view.next_byte()!=0)++bytes;
    view.finish_text(bytes);
    return bytes;
}

} // namespace halo2::native_diagnostic_stream
