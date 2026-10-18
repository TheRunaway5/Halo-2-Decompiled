#pragma once
#include "halo2/online_voice.hpp"
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <stdexcept>

namespace halo2::test {
struct VoiceQueueState {
    struct Snapshot {unsigned starts{},stops{},discards{},captures{},audible{},queued_audible{};bool capture_entered{},playback_entered{};};
    mutable std::mutex mutex;std::condition_variable condition;
    Snapshot state;bool enabled_capture{},transmitting{},hold_capture{},hold_playback{},release_capture{},release_playback{};
    unsigned phase{},noise=12345;
    std::deque<std::pair<VoiceStereo,bool>> output;
    Snapshot snapshot()const{std::lock_guard lock(mutex);return state;}
    void capture(bool value){std::lock_guard lock(mutex);enabled_capture=value;}
    void block_capture(){std::lock_guard lock(mutex);hold_capture=enabled_capture=true;release_capture=state.capture_entered=false;}
    void block_playback(){std::lock_guard lock(mutex);hold_playback=true;release_playback=state.playback_entered=false;}
    void unblock_capture(){std::lock_guard lock(mutex);release_capture=true;condition.notify_all();}
    void unblock_playback(){std::lock_guard lock(mutex);release_playback=true;condition.notify_all();}
};
// Real PCM/Opus callers see a queued synthetic device. Gates hold an actual
// capture/playback call in flight without exposing any microphone or speaker.
class QueuedVoiceDevices final:public VoiceDevices {
    const std::shared_ptr<VoiceQueueState> s;
public:
    explicit QueuedVoiceDevices(std::shared_ptr<VoiceQueueState> state):s(std::move(state)){}
    void start()override{std::lock_guard lock(s->mutex);++s->state.starts;}
    void transmit(bool value)override{std::lock_guard lock(s->mutex);s->transmitting=value;}
    bool capture(VoicePcm &pcm)override{
        std::unique_lock lock(s->mutex);if(!s->enabled_capture||!s->transmitting)return false;
        for(auto &sample:pcm){s->noise=s->noise*1664525u+1013904223u;const float t=float(s->phase++)/48000.f;
            sample=(.5f+.5f*std::sin(t*6.2831853f*4.3f))*(.15f*std::sin(t*6.2831853f*(140+30*std::sin(t*3)))+
                .12f*(float(s->noise>>8)/16777216.f*2-1));}
        ++s->state.captures;
        if(s->hold_capture){s->state.capture_entered=true;s->condition.notify_all();
            if(!s->condition.wait_for(lock,std::chrono::seconds(10),[&]{return s->release_capture;}))throw std::runtime_error("Synthetic capture gate timed out");
            s->hold_capture=s->enabled_capture=false;}
        return true;
    }
    void playback(const VoicePcm &pcm)override{VoiceStereo stereo{};for(unsigned i=0;i<pcm.size();++i)stereo[i*2]=stereo[i*2+1]=pcm[i];playback_stereo(stereo);}
    void playback_stereo(const VoiceStereo &pcm)override{
        float energy{};for(auto value:pcm)energy+=value*value;const bool audible=energy>.001f;
        std::unique_lock lock(s->mutex);
        if(audible&&s->hold_playback){s->state.playback_entered=true;s->condition.notify_all();
            if(!s->condition.wait_for(lock,std::chrono::seconds(10),[&]{return s->release_playback;}))throw std::runtime_error("Synthetic playback gate timed out");
            s->hold_playback=false;}
        if(s->output.size()==16){s->state.queued_audible-=s->output.front().second;s->output.pop_front();}
        s->output.push_back({pcm,audible});s->state.queued_audible+=audible;s->state.audible+=audible;
    }
    void discard()override{std::lock_guard lock(s->mutex);s->output.clear();s->state.queued_audible=0;++s->state.discards;}
    void stop()override{std::lock_guard lock(s->mutex);s->transmitting=false;s->output.clear();s->state.queued_audible=0;++s->state.stops;}
};
}
