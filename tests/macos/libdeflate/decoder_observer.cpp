#include "decoder_observer.hpp"
#include <atomic>
#include <libdeflate.h>

namespace {
std::atomic<std::uint64_t> calls{},successes{},input_bytes{},output_bytes{};
}
extern "C" libdeflate_result halo2_test_libdeflate_decompress(libdeflate_decompressor* decoder,
    const void* input,std::size_t input_size,void* output,std::size_t output_size,
    std::size_t* consumed,std::size_t* written) {
    ++calls;
    const auto result=libdeflate_zlib_decompress_ex(decoder,input,input_size,output,output_size,consumed,written);
    if(result==LIBDEFLATE_SUCCESS) {
        ++successes;
        if(consumed)input_bytes+=*consumed;
        if(written)output_bytes+=*written;
    }
    return result;
}
DecoderObservation decoder_observation() {
    return {calls.load(),successes.load(),input_bytes.load(),output_bytes.load()};
}
