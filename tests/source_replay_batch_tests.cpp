#include "halo2/source_replay_batch.hpp"
#include "halo2/source_execution_calls.hpp"
#include <iostream>
#include <stdexcept>
#include <tuple>

using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
template<class F>void rejects(F f){bool failed=false;try{f();}catch(const std::exception &){failed=true;}check(failed,"Invalid replay chain or budget accepted");}
void put(std::vector<std::byte> &bytes,std::size_t at,std::uint64_t value,unsigned n){for(unsigned i=0;i<n;++i)bytes.at(at+i)=std::byte(value>>(8*i));}
}
int main()try {
    SourceReplayBinding bound;bound.context.session[0]=std::byte{0x15};bound.context.host=4;bound.context.peers=(1u<<0)|(1u<<4);
    bound.context.epoch=3;bound.context.membership_revision=42;bound.base={7,123,1024,{std::byte{0x48}}};bound.previous=bound.base;
    SourceReplayBatch batch{bound,{}};batch.steps.push_back({123,123,{}});batch.steps.push_back({123,123,{}});
    batch.steps[0].phase.clocks={UINT32_MAX,UINT64_MAX,132000000000000000ull};batch.steps[0].phase.result_seconds=1234;
    const auto bytes=encode_source_replay_batch(batch);check(source_replay_batch_payload(bytes),"Replay payload not recognized");
    const auto result=decode_source_replay_batch(bytes,bound);check(result.binding==bound&&result.steps.size()==2&&
        result.steps[0].phase.clocks==batch.steps[0].phase.clocks&&result.steps[0].phase.result_seconds==1234,"Replay batch lost phase clock values");
    check(encode_source_replay_batch(result)==bytes,"Replay batch changed canonical values");
    for(std::size_t n=0;n<bytes.size();++n)rejects([&]{decode_source_replay_batch(std::span(bytes).first(n),bound);});
    for(unsigned n=0;n<8;++n){auto expected=bound;
        switch(n){case 0:++expected.context.epoch;break;case 1:++expected.context.membership_revision;break;
        case 2:expected.context.session[0]^=std::byte{1};break;case 3:expected.context.host=0;break;case 4:expected.context.peers|=2;break;
        case 5:expected.base.digest[0]^=std::byte{1};break;case 6:++expected.previous.id;break;case 7:++expected.first_sequence;break;}
        rejects([&]{decode_source_replay_batch(bytes,expected);});}
    for(auto [at,n,value]:std::array<std::tuple<unsigned,unsigned,std::uint64_t>,8>{{
        {8,4,2},{12,4,1},{63,1,1},{184,4,UINT32_MAX},{188,8,124},{196,8,122},{204,4,UINT32_MAX},{224,8,UINT64_MAX}}}){
        auto bad=bytes;put(bad,at,value,n);rejects([&]{decode_source_replay_batch(bad,bound);});}
    auto bad=bytes;bad.back()^=std::byte{1};rejects([&]{decode_source_replay_batch(bad,bound);});
    bad=bytes;bad.push_back(std::byte{});rejects([&]{decode_source_replay_batch(bad,bound);});
    auto next=batch;next.binding.previous.id=8;next.binding.first_sequence=3;
    check(decode_source_replay_batch(encode_source_replay_batch(next),next.binding).steps.size()==2,"Next validated replay link rejected");
    next.binding.first_sequence=UINT64_MAX;rejects([&]{encode_source_replay_batch(next);});
    auto wrong=batch;wrong.steps[1].before_tick=124;rejects([&]{encode_source_replay_batch(wrong);});
    wrong=batch;wrong.binding.first_sequence=2;rejects([&]{encode_source_replay_batch(wrong);});
    SourceReplayLimits limits;limits.phases=1;rejects([&]{decode_source_replay_batch(bytes,bound,limits);});rejects([&]{encode_source_replay_batch(batch,limits);});
    limits={};limits.encoded_bytes=bytes.size()-1;rejects([&]{decode_source_replay_batch(bytes,bound,limits);});rejects([&]{encode_source_replay_batch(batch,limits);});
    limits={};limits.decoded_bytes=63;rejects([&]{decode_source_replay_batch(bytes,bound,limits);});rejects([&]{encode_source_replay_batch(batch,limits);});
    limits={};limits.allocated_bytes=sizeof(SourceReplayStep)*2-1;rejects([&]{decode_source_replay_batch(bytes,bound,limits);});rejects([&]{encode_source_replay_batch(batch,limits);});
    limits={};limits.decoded_bytes=0;limits.allocated_bytes=0;auto mismatched=bound;++mismatched.context.epoch;
    rejects([&]{decode_source_replay_batch(bytes,mismatched,limits);});
    SourceReplayBatch dangerous{bound,{{123,123,{}}}};
    SourceExecutionCall raw_write;raw_write.kind=SourceExecutionCallKind::WriteSourceCallInput;
    const std::array<std::byte,1> value{std::byte{1}};
    raw_write.arguments=source_execution_arguments(0x400000u,std::span<const std::byte>(value));
    dangerous.steps[0].phase.calls.push_back(raw_write);
    rejects([&]{encode_source_replay_batch(dangerous);});
    // Construct the previously accepted network envelope using the trusted
    // local tape codec. Network decode must reject before a world is touched.
    const auto local=encode_source_execution_phase(dangerous.steps[0].phase);
    auto legacy=encode_source_replay_batch(SourceReplayBatch{bound,{{123,123,{}}}});
    legacy.resize(208);put(legacy,204,local.size(),4);legacy.insert(legacy.end(),local.begin(),local.end());
    rejects([&]{decode_source_replay_batch(legacy,bound);});
    for(unsigned kind=15;kind<=23;++kind){
      raw_write.kind=SourceExecutionCallKind(kind);
      rejects([&]{validate_source_network_replay_calls(std::span(&raw_write,1));});
    }
    SourcePlayerCameraCall camera;camera.player=0;camera.controller[0]=1;
    camera.controller[3]=0x400000u; // Caller-controlled native controller bank.
    raw_write.kind=SourceExecutionCallKind::PlayerCamera;
    raw_write.arguments=source_execution_arguments(camera);raw_write.result.assign(32,std::byte{});
    dangerous.steps[0].phase.calls={raw_write};
    rejects([&]{encode_source_replay_batch(dangerous);});
    const auto local_camera=encode_source_execution_phase(dangerous.steps[0].phase);
    legacy.resize(208);put(legacy,204,local_camera.size(),4);legacy.insert(legacy.end(),local_camera.begin(),local_camera.end());
    rejects([&]{decode_source_replay_batch(legacy,bound);});
    std::cout<<"PASS replay chain bounds and Rust network capability policy; legacy raw-write packet rejected before execution\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
