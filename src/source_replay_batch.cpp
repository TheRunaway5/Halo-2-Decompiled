#include "halo2/source_replay_batch.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <limits>

namespace halo2 {
namespace {
constexpr std::array<std::byte,8> magic{std::byte{'H'},std::byte{'2'},std::byte{'R'},std::byte{'P'},
    std::byte{'L'},std::byte{'Y'},std::byte{'0'},std::byte{'1'}};
void need(bool ok){if(!ok)throw FormatError("Invalid source replay batch binding, order or budget");}
struct Writer {
    std::uint64_t limit;std::vector<std::byte> bytes;
    void raw(std::span<const std::byte> value){need(value.size()<=limit-bytes.size());bytes.insert(bytes.end(),value.begin(),value.end());}
    void put(std::uint64_t value,unsigned n){std::array<std::byte,8> b{};for(unsigned i=0;i<n;++i)b[i]=std::byte(value>>(8*i));raw(std::span(b).first(n));}
};
struct Reader {
    std::span<const std::byte> bytes;std::size_t at{};
    std::span<const std::byte> raw(std::size_t n){need(n<=bytes.size()-at);const auto out=bytes.subspan(at,n);at+=n;return out;}
    std::uint64_t get(unsigned n){std::uint64_t value{};const auto b=raw(n);for(unsigned i=0;i<n;++i)value|=std::uint64_t(std::to_integer<unsigned>(b[i]))<<(8*i);return value;}
};
void seal(Writer &out,const AuthorityCutSeal &s){out.put(s.id,8);out.put(s.tick,8);out.put(s.bytes,8);out.raw(s.digest);}
AuthorityCutSeal seal(Reader &in){AuthorityCutSeal s;s.id=in.get(8);s.tick=in.get(8);s.bytes=in.get(8);
    std::ranges::copy(in.raw(s.digest.size()),s.digest.begin());return s;}
void binding(Writer &out,const SourceReplayBinding &b){out.raw(b.context.session);out.put(b.context.epoch,4);
    out.put(b.context.membership_revision,8);out.put(b.context.peers,2);out.put(b.context.host,1);out.put(0,1);
    seal(out,b.base);seal(out,b.previous);out.put(b.first_sequence,8);}
SourceReplayBinding binding(Reader &in){SourceReplayBinding b;std::ranges::copy(in.raw(b.context.session.size()),b.context.session.begin());
    b.context.epoch=std::uint32_t(in.get(4));b.context.membership_revision=in.get(8);b.context.peers=std::uint16_t(in.get(2));
    b.context.host=unsigned(in.get(1));need(in.get(1)==0);b.base=seal(in);b.previous=seal(in);b.first_sequence=in.get(8);return b;}
void validate(const SourceReplayBinding &b,std::size_t phases,SourceReplayLimits limits){
    need(limits.phases&&limits.phases<=1024&&phases&&phases<=limits.phases&&b.context.host<16&&
        (b.context.peers&(1u<<b.context.host))&&b.base.id&&b.base.bytes&&b.previous.id>=b.base.id&&b.previous.bytes&&
        b.previous.tick>=b.base.tick&&b.first_sequence&&phases<=UINT64_MAX-b.first_sequence);
    if(b.base.id==b.previous.id)need(b.base==b.previous&&b.first_sequence==1);
    else need(b.first_sequence>1);
}
std::uint64_t raw_size(std::span<const std::byte> phase){need(phase.size()>=64);Reader in{phase.subspan(16,8)};return in.get(8);}
void consume(std::uint64_t &budget,std::uint64_t amount){need(amount<=budget);budget-=amount;}
}
bool source_replay_batch_payload(std::span<const std::byte> bytes)noexcept{
    return bytes.size()>=magic.size()&&std::ranges::equal(bytes.first(magic.size()),magic);
}
std::uint64_t source_replay_phase_allocation(const SourceExecutionPhase &phase){
    need(phase.inputs.size()<=SourceExecutionInputs::maximum_entries&&phase.calls.size()<=SourceExecutionInputs::maximum_calls);
    std::uint64_t value=phase.inputs.size()*sizeof(SourceExecutionInput)+phase.calls.size()*sizeof(SourceExecutionCall);
    std::uint64_t payload=SourceExecutionInputs::maximum_call_bytes;
    for(const auto &call:phase.calls){consume(payload,call.arguments.size());consume(payload,call.result.size());value+=call.arguments.size()+call.result.size();}
    return value;
}
std::vector<std::byte> encode_source_replay_batch(const SourceReplayBatch &batch,SourceReplayLimits limits){
    validate(batch.binding,batch.steps.size(),limits);Writer out{limits.encoded_bytes,{}};out.raw(magic);out.put(1,4);out.put(0,4);
    binding(out,batch.binding);out.put(batch.steps.size(),4);
    auto decoded=limits.decoded_bytes,allocated=limits.allocated_bytes;consume(allocated,batch.steps.size()*sizeof(SourceReplayStep));
    auto tick=batch.binding.previous.tick;
    for(const auto &step:batch.steps){need(step.before_tick==tick&&step.after_tick>=tick);tick=step.after_tick;
        consume(allocated,source_replay_phase_allocation(step.phase));
        SourceExecutionCodecLimits phase_limits;phase_limits.encoded_bytes=limits.encoded_bytes-out.bytes.size();phase_limits.decoded_bytes=decoded;
        const auto bytes=encode_source_execution_phase(step.phase,phase_limits);consume(decoded,raw_size(bytes));
        out.put(step.before_tick,8);out.put(step.after_tick,8);out.put(bytes.size(),4);out.raw(bytes);
    }
    return std::move(out.bytes);
}
SourceReplayBatch decode_source_replay_batch(std::span<const std::byte> bytes,const SourceReplayBinding &expected,SourceReplayLimits limits){
    need(bytes.size()<=limits.encoded_bytes);Reader in{bytes};need(std::ranges::equal(in.raw(8),magic)&&in.get(4)==1&&in.get(4)==0);
    SourceReplayBatch batch;batch.binding=binding(in);need(batch.binding==expected);const auto count=in.get(4);validate(batch.binding,count,limits);
    need(count<=(bytes.size()-in.at)/20);auto decoded=limits.decoded_bytes,allocated=limits.allocated_bytes;
    consume(allocated,count*sizeof(SourceReplayStep));batch.steps.reserve(count);auto tick=batch.binding.previous.tick;
    for(std::uint64_t i=0;i<count;++i){SourceReplayStep step;step.before_tick=in.get(8);step.after_tick=in.get(8);need(step.before_tick==tick&&step.after_tick>=tick);tick=step.after_tick;
        const auto encoded=in.raw(in.get(4));SourceExecutionCodecLimits phase_limits;phase_limits.encoded_bytes=limits.encoded_bytes;
        phase_limits.decoded_bytes=decoded;phase_limits.allocated_bytes=allocated;
        step.phase=decode_source_execution_phase(encoded,phase_limits);consume(decoded,raw_size(encoded));
        consume(allocated,source_replay_phase_allocation(step.phase));batch.steps.push_back(std::move(step));
    }
    need(in.at==bytes.size());return batch;
}
}
