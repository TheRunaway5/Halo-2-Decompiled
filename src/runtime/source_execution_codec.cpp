#include "halo2/source_execution_codec.hpp"
#include "halo2/source_execution_calls.hpp"
#include "halo2/cache.hpp"
#include <openssl/sha.h>
#include <zlib.h>
#include <algorithm>
#include <limits>

namespace halo2 {
namespace {
constexpr std::array<std::byte,8> magic{std::byte{'H'},std::byte{'2'},std::byte{'E'},std::byte{'X'},
    std::byte{'E'},std::byte{'C'},std::byte{'0'},std::byte{'1'}};
constexpr std::size_t envelope_bytes=64,input_bytes=13,call_header_bytes=17;
// Measured on complete rendered host frames. Compression runs on the
// publisher worker; level6 reduces transfer time with modest worker CPU cost.
constexpr int phase_compression_level=6;
constexpr std::uint64_t maximum_body_bytes=32+input_bytes*SourceExecutionInputs::maximum_entries+
    call_header_bytes*SourceExecutionInputs::maximum_calls+SourceExecutionInputs::maximum_call_bytes;
auto digest(std::span<const std::byte> bytes){std::array<std::byte,32> result;
    SHA256(reinterpret_cast<const unsigned char *>(bytes.data()),bytes.size(),reinterpret_cast<unsigned char *>(result.data()));return result;}
void claim(std::uint64_t &remaining,std::uint64_t count,std::uint64_t stride){
    if(stride&&count>remaining/stride)throw FormatError("Execution journal exceeds allocation budget");remaining-=count*stride;
}
void validate(const SourceExecutionPhase &phase,SourceExecutionCodecLimits limits){
    if(phase.inputs.size()>SourceExecutionInputs::maximum_entries||phase.calls.size()>SourceExecutionInputs::maximum_calls)
        throw FormatError("Execution journal exceeds its phase bounds");
    auto remaining=limits.allocated_bytes;claim(remaining,phase.inputs.size(),sizeof(SourceExecutionInput));
    claim(remaining,phase.calls.size(),sizeof(SourceExecutionCall));
    for(const auto &call:phase.calls){claim(remaining,call.arguments.size(),1);claim(remaining,call.result.size(),1);}
    // Use the same width, operand, ordering and consumption-boundary admission
    // as the actual native replay owner, plus typed call payload admission.
    SourceExecutionInputs admitted(phase.inputs,phase.calls);
    validate_source_execution_calls(phase.calls);
}
struct Writer {
    std::uint64_t limit;std::vector<std::byte> bytes;
    void raw(std::span<const std::byte> value){
        if(value.size()>limit-bytes.size())throw FormatError("Execution journal exceeds decoded byte budget");
        bytes.insert(bytes.end(),value.begin(),value.end());
    }
    void put(std::uint64_t value,unsigned count){std::array<std::byte,8> b{};
        for(unsigned i=0;i<count;++i)b[i]=std::byte(value>>(8*i));raw(std::span(b).first(count));}
};
struct Reader {
    std::span<const std::byte> bytes;std::size_t at{};
    std::span<const std::byte> raw(std::size_t count){
        if(count>bytes.size()-at)throw FormatError("Truncated execution journal");
        const auto result=bytes.subspan(at,count);at+=count;return result;
    }
    std::uint64_t get(unsigned count){const auto b=raw(count);std::uint64_t value{};
        for(unsigned i=0;i<count;++i)value|=std::uint64_t(std::to_integer<unsigned>(b[i]))<<(8*i);return value;}
};
}
std::vector<std::byte> encode_source_execution_phase(const SourceExecutionPhase &phase,SourceExecutionCodecLimits limits){
    validate(phase,limits);Writer body{limits.decoded_bytes,{}};
    body.put(phase.inputs.size(),4);body.put(phase.calls.size(),4);
    body.put(phase.clocks.milliseconds,4);body.put(phase.clocks.cycles,8);body.put(phase.clocks.utc_filetime,8);body.put(phase.result_seconds,4);
    for(const auto &input:phase.inputs){body.put(std::uint8_t(input.kind),1);body.put(input.operand,4);body.put(input.value,8);}
    for(const auto &call:phase.calls){body.put(std::uint8_t(call.kind),1);body.put(call.input_begin,4);body.put(call.input_end,4);
        body.put(call.arguments.size(),4);body.put(call.result.size(),4);body.raw(call.arguments);body.raw(call.result);}
    if(limits.encoded_bytes<envelope_bytes||body.bytes.size()>std::numeric_limits<uLong>::max())
        throw FormatError("Execution journal exceeds encoded byte budget");
    const auto capacity=std::min<std::uint64_t>(compressBound(uLong(body.bytes.size())),limits.encoded_bytes-envelope_bytes);
    std::vector<std::byte> compressed(static_cast<std::size_t>(capacity));uLongf size=capacity;
    if(compress2(reinterpret_cast<Bytef *>(compressed.data()),&size,reinterpret_cast<const Bytef *>(body.bytes.data()),uLong(body.bytes.size()),phase_compression_level)!=Z_OK)
        throw FormatError("Cannot compress execution journal");compressed.resize(size);
    Writer envelope{limits.encoded_bytes,{}};envelope.raw(magic);envelope.put(1,4);envelope.put(0,4);
    envelope.put(body.bytes.size(),8);envelope.put(compressed.size(),8);envelope.raw(digest(body.bytes));envelope.raw(compressed);
    return std::move(envelope.bytes);
}
SourceExecutionPhase decode_source_execution_phase(std::span<const std::byte> bytes,SourceExecutionCodecLimits limits){
    if(bytes.size()<envelope_bytes||bytes.size()>limits.encoded_bytes)throw FormatError("Execution journal envelope exceeds admitted extent");
    Reader envelope{bytes};if(!std::ranges::equal(envelope.raw(8),magic)||envelope.get(4)!=1||envelope.get(4)!=0)
        throw FormatError("Unknown execution journal envelope");
    const auto raw_size=envelope.get(8),compressed_size=envelope.get(8);const auto expected=envelope.raw(32);
    if(raw_size<32||raw_size>maximum_body_bytes||raw_size>limits.decoded_bytes||raw_size>std::numeric_limits<uLongf>::max()||
       compressed_size!=bytes.size()-envelope_bytes||compressed_size>std::numeric_limits<uLong>::max())
        throw FormatError("Execution journal compression extent exceeds admitted budget");
    std::vector<std::byte> raw(static_cast<std::size_t>(raw_size));uLongf size=raw_size;uLong consumed=compressed_size;
    if(uncompress2(reinterpret_cast<Bytef *>(raw.data()),&size,reinterpret_cast<const Bytef *>(bytes.data()+envelope_bytes),&consumed)!=Z_OK||
        size!=raw_size||consumed!=compressed_size||!std::ranges::equal(digest(raw),expected))
        throw FormatError("Execution journal has corrupt or trailing compressed data");
    Reader body{raw};const auto inputs=body.get(4),calls=body.get(4);
    if(inputs>SourceExecutionInputs::maximum_entries||calls>SourceExecutionInputs::maximum_calls||
        input_bytes*inputs+call_header_bytes*calls>raw.size()-32)
        throw FormatError("Execution journal has impossible phase counts");
    auto remaining=limits.allocated_bytes;claim(remaining,inputs,sizeof(SourceExecutionInput));claim(remaining,calls,sizeof(SourceExecutionCall));
    SourceExecutionPhase phase;phase.clocks.milliseconds=std::uint32_t(body.get(4));phase.clocks.cycles=body.get(8);
    phase.clocks.utc_filetime=body.get(8);phase.result_seconds=std::uint32_t(body.get(4));
    phase.inputs.reserve(inputs);phase.calls.reserve(calls);
    for(std::uint64_t i=0;i<inputs;++i){SourceExecutionInput input;input.kind=static_cast<SourceExecutionInputKind>(body.get(1));
        input.operand=std::uint32_t(body.get(4));input.value=body.get(8);phase.inputs.push_back(input);}
    std::size_t payload_budget=SourceExecutionInputs::maximum_call_bytes;
    for(std::uint64_t i=0;i<calls;++i){SourceExecutionCall call;call.kind=static_cast<SourceExecutionCallKind>(body.get(1));
        call.input_begin=std::uint32_t(body.get(4));call.input_end=std::uint32_t(body.get(4));const auto arguments=body.get(4),result=body.get(4);
        if(arguments+result>payload_budget)throw FormatError("Execution journal exceeds call payload budget");
        payload_budget-=arguments+result;claim(remaining,arguments+result,1);
        const auto args=body.raw(arguments),returned=body.raw(result);call.arguments.assign(args.begin(),args.end());call.result.assign(returned.begin(),returned.end());
        phase.calls.push_back(std::move(call));
    }
    if(body.at!=body.bytes.size())throw FormatError("Execution journal has trailing state");
    validate(phase,limits);return phase;
}
}
