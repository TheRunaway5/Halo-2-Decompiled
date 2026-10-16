#include "halo2/source_execution_codec.hpp"
#include "halo2/source_execution_calls.hpp"
#include <openssl/sha.h>
#include <zlib.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <tuple>

using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
template<class F>void rejects(F f){bool failed=false;try{f();}catch(const std::exception &){failed=true;}check(failed,"Malformed execution journal accepted");}
void put(std::vector<std::byte> &bytes,std::size_t at,std::uint64_t value,unsigned n){
    for(unsigned i=0;i<n;++i)bytes.at(at+i)=std::byte(value>>(i*8));}
std::uint64_t get(std::span<const std::byte> bytes,std::size_t at,unsigned n){std::uint64_t result{};
    for(unsigned i=0;i<n;++i)result|=std::uint64_t(std::to_integer<unsigned>(bytes[at+i]))<<(i*8);return result;}
std::vector<std::byte> unpack(const std::vector<std::byte> &encoded){
    std::vector<std::byte> result(get(encoded,16,8));uLongf size=result.size();
    check(uncompress(reinterpret_cast<Bytef *>(result.data()),&size,reinterpret_cast<const Bytef *>(encoded.data()+64),encoded.size()-64)==Z_OK,"Cannot unpack test journal");return result;}
std::vector<std::byte> repack(const std::vector<std::byte> &header,std::span<const std::byte> raw,int level=Z_BEST_SPEED){
    auto result=header;result.resize(64+compressBound(raw.size()));uLongf size=result.size()-64;
    check(compress2(reinterpret_cast<Bytef *>(result.data()+64),&size,reinterpret_cast<const Bytef *>(raw.data()),raw.size(),level)==Z_OK,"Cannot repack test journal");
    put(result,16,raw.size(),8);put(result,24,size,8);result.resize(64+size);
    SHA256(reinterpret_cast<const unsigned char *>(raw.data()),raw.size(),reinterpret_cast<unsigned char *>(result.data()+32));return result;}
}
int main()try {
    using I=SourceExecutionInputKind;using C=SourceExecutionCallKind;
    SourceExecutionPhase phase;phase.clocks={UINT32_MAX,UINT64_MAX,132000000000000000ull};phase.result_seconds=UINT32_MAX;
    phase.inputs={{I::Milliseconds,0,UINT32_MAX},{I::TimestampCounter,0,UINT64_MAX},
        {I::ReciprocalSquareRoot,0x7fc00001u,0x80000000u},{I::UtcFiletime,0,phase.clocks.utc_filetime},{I::ResultWallSeconds,0,UINT32_MAX}};
    phase.calls={{C::BeginModelFrame,{},0,5,{}},{C::CallActualSource,
        source_execution_arguments(0x9d8e0u,std::span<const std::uint32_t>{}),5,5,source_execution_result(8u)}};
    const auto bytes=encode_source_execution_phase(phase);auto decoded=decode_source_execution_phase(bytes);
    check(decoded.inputs==phase.inputs&&decoded.calls==phase.calls&&decoded.clocks==phase.clocks&&decoded.result_seconds==phase.result_seconds,
        "Execution journal normalized or omitted owning values");
    check(encode_source_execution_phase(decoded)==bytes,"Execution journal encoding is not canonical");
    for(std::size_t n=0;n<bytes.size();++n)rejects([&]{decode_source_execution_phase(std::span(bytes).first(n));});
    for(auto at:{0u,8u,12u,16u,24u,32u,64u}){auto bad=bytes;bad[at]^=std::byte{0x80};rejects([&]{decode_source_execution_phase(bad);});}
    auto trailing=bytes;trailing.push_back(std::byte{});rejects([&]{decode_source_execution_phase(trailing);});
    auto compressed_tail=bytes;compressed_tail.push_back(std::byte{});put(compressed_tail,24,compressed_tail.size()-64,8);
    rejects([&]{decode_source_execution_phase(compressed_tail);});
    auto raw=unpack(bytes);
    for(auto level:{Z_BEST_SPEED,3,6,9}){
        const auto compatible=decode_source_execution_phase(repack(bytes,raw,level));
        check(compatible.inputs==phase.inputs&&compatible.calls==phase.calls&&compatible.clocks==phase.clocks&&compatible.result_seconds==phase.result_seconds,
            "Compatible compression changed an owning phase value");
    }
    auto bad_raw=raw;bad_raw.push_back(std::byte{});rejects([&]{decode_source_execution_phase(repack(bytes,bad_raw));});
    for(auto [offset,width,value]:std::array<std::tuple<unsigned,unsigned,std::uint64_t>,8>{{
        {0,4,UINT32_MAX},{4,4,UINT32_MAX},{32,1,255},{33,4,1},{37,8,UINT64_MAX},
        {97,1,255},{98,4,1},{106,4,UINT32_MAX}}}){
        bad_raw=raw;put(bad_raw,offset,value,width);rejects([&]{decode_source_execution_phase(repack(bytes,bad_raw));});}
    auto bad_phase=phase;bad_phase.calls[1].result.pop_back();rejects([&]{encode_source_execution_phase(bad_phase);});
    bad_phase=phase;bad_phase.calls[1].input_begin=4;rejects([&]{encode_source_execution_phase(bad_phase);});
    bad_phase=phase;bad_phase.inputs[0].operand=1;rejects([&]{encode_source_execution_phase(bad_phase);});
    SourceExecutionCodecLimits limits;limits.encoded_bytes=bytes.size()-1;rejects([&]{encode_source_execution_phase(phase,limits);});rejects([&]{decode_source_execution_phase(bytes,limits);});
    limits={};limits.decoded_bytes=raw.size()-1;rejects([&]{encode_source_execution_phase(phase,limits);});rejects([&]{decode_source_execution_phase(bytes,limits);});
    limits={};limits.allocated_bytes=sizeof(SourceExecutionInput)*phase.inputs.size()+sizeof(SourceExecutionCall)*phase.calls.size();
    rejects([&]{encode_source_execution_phase(phase,limits);});rejects([&]{decode_source_execution_phase(bytes,limits);});
    SourceExecutionPhase empty;const auto empty_bytes=encode_source_execution_phase(empty);
    check(decode_source_execution_phase(empty_bytes).calls.empty(),"Empty phase did not survive its clock-only envelope");
    SourceExecutionPhase maximum;maximum.inputs.resize(SourceExecutionInputs::maximum_entries,{I::Milliseconds,0,1});
    maximum.calls.push_back({C::BeginModelFrame,{},0,std::uint32_t(maximum.inputs.size()),{}});
    check(decode_source_execution_phase(encode_source_execution_phase(maximum)).inputs==maximum.inputs,"Maximum admitted input count did not round trip");
    maximum.inputs.push_back({I::Milliseconds,0,1});rejects([&]{encode_source_execution_phase(maximum);});
    SourceExecutionPhase many_calls;many_calls.calls.resize(SourceExecutionInputs::maximum_calls,{C::BeginModelFrame,{},0,0,{}});
    check(decode_source_execution_phase(encode_source_execution_phase(many_calls)).calls==many_calls.calls,"Maximum admitted call count did not round trip");
    many_calls.calls.push_back({C::BeginModelFrame,{},0,0,{}});rejects([&]{encode_source_execution_phase(many_calls);});
    SourceExecutionPhase full_payload;const std::vector<std::byte> scratch(SourceExecutionInputs::maximum_call_bytes-8,std::byte{0x15});
    full_payload.calls.push_back({C::WriteSourceCallInput,source_execution_arguments(0x1000u,std::span<const std::byte>(scratch)),0,0,{}});
    check(decode_source_execution_phase(encode_source_execution_phase(full_payload)).calls==full_payload.calls,"Maximum admitted call payload did not round trip");
    full_payload.calls.front().arguments.push_back(std::byte{});rejects([&]{encode_source_execution_phase(full_payload);});
    std::cout<<"PASS owning compressed execution journal, exact values, canonical bytes, count/extent/allocation/payload/typed admission and corruption/trailing rejection\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
