#include "halo2/game_voice.hpp"
#include "voice_queue_fixture.hpp"
#include <iostream>
#include <thread>

using namespace halo2;
using namespace std::chrono_literals;
namespace {
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
template<class F>void wait(F ready,const char *why){const auto end=std::chrono::steady_clock::now()+4s;
    while(!ready()){check(std::chrono::steady_clock::now()<end,why);std::this_thread::sleep_for(2ms);}}
GameVoiceFrame tone(unsigned sequence,std::uint64_t stamp){VoiceCodec encoder;VoicePcm pcm;
    for(unsigned i=0;i<pcm.size();++i)pcm[i]=.3f*std::sin(float(i)*6.2831853f*440/48000);
    return {1,sequence,false,{1,1},encoder.encode(pcm),stamp};}
}
int main()try{
    auto device=std::make_shared<test::VoiceQueueState>();GameVoice voice(std::make_unique<test::QueuedVoiceDevices>(device));
    const auto first=voice.fence();wait([&]{return voice.ready(first);},"Opt-out fence did not retire");
    check(device->snapshot().starts==0&&voice.resume(first),"Fence opened a microphone or could not resume");
    voice.mute(3,true);voice.blocks(1u<<4);voice.microphone_muted(true);voice.enable(true);
    device->block_playback();for(unsigned n=1;n<=5;++n)voice.receive(tone(n,first));
    wait([&]{return device->snapshot().playback_entered;},"Decoded PCM did not reach the held device call");
    const auto before=device->snapshot();const auto second=voice.fence();
    check(first!=second&&!voice.ready(second)&&!voice.resume(second),"New role resumed before in-flight PCM retired");
    check(device->snapshot().discards==before.discards,"Fence did device I/O on the requesting thread");
    voice.receive(tone(900,first));check(voice.outgoing().empty(),"Old microphone queue survived fence");
    device->unblock_playback();wait([&]{return voice.ready(second);},"Decoded/device queues did not retire");
    const auto state=voice.snapshot();check(state.enabled&&state.microphone_muted&&state.muted==(1u<<3)&&state.blocked==(1u<<4)&&
        !state.transmitting&&!state.radio&&!state.talking[1]&&device->snapshot().queued_audible==0,"Fence lost preferences or retained decoded/device audio");
    check(!voice.ready(first)&&!voice.resume(first)&&voice.resume(second),"Stale voice stamp resumed a newer route");
    auto audible=device->snapshot().audible;voice.receive(tone(999,first));std::this_thread::sleep_for(80ms);
    check(device->snapshot().audible==audible,"Held predecessor frame was decoded after resumption");
    voice.receive(tone(1,second));wait([&]{return device->snapshot().audible>audible;},"New speaker sequence/decoder did not restart");
    check(device->snapshot().starts==1&&device->snapshot().stops==0,"Migration reopened audio devices");
    voice.microphone_muted(false);device->block_capture();wait([&]{return device->snapshot().capture_entered;},"Microphone call did not reach the gate");
    const auto third=voice.fence();check(!voice.ready(third),"Old in-flight microphone was acknowledged early");
    device->unblock_capture();wait([&]{return voice.ready(third);},"Microphone fence did not retire");
    check(voice.outgoing().empty()&&voice.resume(third),"Old captured samples were relabeled for the next role");
    device->capture(true);std::vector<GameVoiceFrame> frames;
    wait([&]{frames=voice.outgoing();return !frames.empty();},"Fresh microphone speech did not recover");
    for(const auto &frame:frames)check(frame.delivery_generation==third,"Microphone frame lost its local route stamp");
    const auto fourth=voice.fence();wait([&]{return voice.ready(fourth);},"Repeated fence did not retire");
    check(!voice.resume(third)&&voice.outgoing().empty(),"Held old microphone batch reopened delivery");
    auto other_device=std::make_shared<test::VoiceQueueState>();GameVoice other(std::make_unique<test::QueuedVoiceDevices>(other_device));
    const auto foreign=other.fence();check(foreign!=fourth&&!voice.resume(foreign),"Separate audio owners reused a delivery stamp");
    voice.close();check(voice.ready(fourth)&&!voice.resume(fourth),"Closed devices blocked retirement or reopened");
    voice.enable(true);check(!voice.snapshot().enabled,"Closed voice reopened devices");
    std::cout<<"PASS actual Opus/PCM and queued synthetic devices: in-flight playback/capture fences, asynchronous retirement acknowledgment, old talker/predictor/sequence and device queue removal, late-frame rejection, fresh low-sequence speaker and microphone recovery, unchanged mute/block/opt-in/device ownership, repeated and foreign stamps, close\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
