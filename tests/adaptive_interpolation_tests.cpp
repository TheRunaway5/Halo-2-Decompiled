// Non-host adaptive interpolation buffer (NETCODE_ENHANCED_GOAL item 3a).
// A deterministic host/network/client model: host ticks at 30 Hz, a fixed
// 40 ms path plus an injected jitter profile, LanClient's stale-tick drop,
// and a 60 Hz client frame loop with small frame-time jitter. The runtime's
// release rule (release the next buffered snapshot once playback reaches the
// current endpoint) and blend are applied to every frame. Checks: the steady
// delay tracks the jitter (clean approaches one tick plus processing); the
// presented time never steps backwards and, outside underrun holds, advances
// at a rate within the slew bound while the delay grows and shrinks; no snap
// after the first placement; underruns stay rare; Original keeps the fixed
// one-tick delay whatever the jitter.
#include "halo2/adaptive_interpolation.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <deque>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

using namespace halo2;
namespace {
void check(bool yes,const std::string &message){if(!yes)throw std::runtime_error(message);}
constexpr double tick=1.0/30.0,latency=.040;
// Client frame cadences: 60 Hz vsync (+-0.5 ms) and 144 Hz (+-1 ms).
struct Cadence {const char *name;double period,jitter;};
constexpr Cadence hz60{"60 Hz",1.0/60.0,.001},hz144{"144 Hz",1.0/144.0,.002};
struct Rng {
  std::uint32_t state;
  double unit(){state=state*1664525u+1013904223u;return double(state>>8)/16777216.0;}
};
struct Profile {
  std::string name;
  // Extra one-way delay of the snapshot sent at host time `sent`.
  std::function<double(double sent,Rng &)> extra;
  double expected; // Jitter the steady delay should cover (seconds), for the range check.
};
struct Arrival {std::uint32_t tick;double at;};
std::vector<Arrival> arrivals(const Profile &profile,double seconds,std::uint32_t seed) {
  Rng rng{seed};std::vector<Arrival> out;
  for(std::uint32_t t=1;double(t)*tick<seconds;++t) {
    const double sent=double(t)*tick;
    out.push_back({t,sent+latency+profile.extra(sent,rng)});
  }
  std::stable_sort(out.begin(),out.end(),[](const auto &a,const auto &b){return a.at<b.at;});
  return out;
}
struct Result {
  double mean_target{},mean_delay{},mean_behind{},max_delay{};
  double underrun_fraction{};
  std::uint64_t frames{},holding_frames{},snaps{},backward{},rate_violations{},jumps{},underruns{};
  double min_rate=1e9,max_rate{};
  std::uint64_t dropped{},released{};
};
// Adaptive (Balanced/Full) presentation of one profile. `measure_from` skips
// the warm-up when averaging the steady delay.
Result adaptive(const Profile &profile,const Cadence &cadence,double seconds,double measure_from,std::uint32_t seed,
    std::vector<std::pair<double,double>> *trace=nullptr) {
  AdaptiveInterpolationClock clock;const auto slew=clock.settings().slew;
  const auto incoming=arrivals(profile,seconds,seed);
  Rng frame_rng{seed^0x9e3779b9u};
  std::size_t next=0;std::uint32_t delivered=0;
  std::deque<std::uint32_t> queue;
  std::uint32_t previous=0,current=0;bool bound=false;
  double last_shown=-1,last_at=0;bool have_last=false,last_holding=false;
  Result r;double sum_target=0,sum_delay=0,sum_behind=0;std::uint64_t measured=0;
  for(double at=cadence.period;at<seconds;at+=cadence.period+(frame_rng.unit()-.5)*cadence.jitter) {
    // LanClient: accept newer ticks polled this frame, drop stale ones.
    while(next<incoming.size() && incoming[next].at<=at) {
      const auto a=incoming[next++];
      if(a.tick<=delivered){++r.dropped;continue;}
      delivered=a.tick;clock.receive(a.tick,at);queue.push_back(a.tick);
    }
    if(!clock.active())continue;
    const double playback=clock.advance(at);
    // Runtime release rule.
    while(!queue.empty()) {
      const auto t=queue.front();
      if(bound && t<current){queue.pop_front();continue;}
      if(bound && t!=current && playback<double(current))break;
      queue.pop_front();++r.released;
      if(!bound){previous=current=t;bound=true;continue;}
      if(t!=current){previous=current;current=t;}
    }
    if(!bound)continue;
    const double span=double(current-previous);
    const double blend=span>0?std::clamp((playback-double(previous))/span,0.0,1.0):1.0;
    const double shown=double(previous)+blend*span;
    const auto stats=clock.statistics(at);
    ++r.frames;
    if(have_last) {
      const double dt=at-last_at,step=shown-last_shown;
      if(step<-1e-9)++r.backward;
      const double rate=step*tick/dt;
      // Outside holds the presented time moves at a slewed rate; leaving a
      // hold may only resume, never jump ahead.
      if(!stats.holding && !last_holding && at>=1.0) {
        r.min_rate=std::min(r.min_rate,rate);r.max_rate=std::max(r.max_rate,rate);
        if(rate<1-slew-1e-6 || rate>1+slew+1e-6)++r.rate_violations;
      }
      if(step*tick>dt*(1+slew)+1e-9)++r.jumps;
    }
    if(stats.holding)++r.holding_frames;
    if(at>=measure_from) {
      sum_target+=stats.target;sum_delay+=stats.delay;
      sum_behind+=(at-latency)/tick*tick-shown*tick;++measured;
      r.max_delay=std::max(r.max_delay,stats.delay);
    }
    if(trace)trace->push_back({at,stats.delay});
    last_shown=shown;last_at=at;have_last=true;last_holding=stats.holding;
  }
  const auto stats=clock.statistics(last_at);
  r.snaps=stats.snaps;r.underruns=stats.underruns;
  r.mean_target=sum_target/double(measured);r.mean_delay=sum_delay/double(measured);
  r.mean_behind=sum_behind/double(measured);
  r.underrun_fraction=double(r.holding_frames)/double(r.frames);
  return r;
}
// Original presentation: the existing runtime formula (blend from the
// receipt over one tick). Returns the presented delay behind the newest
// received tick at every receipt frame and counts forward jumps.
struct OriginalResult {double receipt_delay_min=1e9,receipt_delay_max=-1e9;std::uint64_t receipts{},jumps{};};
OriginalResult original(const Profile &profile,const Cadence &cadence,double seconds,std::uint32_t seed) {
  const auto incoming=arrivals(profile,seconds,seed);
  Rng frame_rng{seed^0x9e3779b9u};
  std::size_t next=0;std::uint32_t delivered=0,previous=0,current=0;bool bound=false;
  double snapshot_time=0,last_shown=-1,last_at=0;
  OriginalResult r;
  for(double at=cadence.period;at<seconds;at+=cadence.period+(frame_rng.unit()-.5)*cadence.jitter) {
    bool received=false;
    while(next<incoming.size() && incoming[next].at<=at) {
      const auto a=incoming[next++];
      if(a.tick<=delivered)continue;
      delivered=a.tick;
      if(!bound){previous=current=a.tick;bound=true;}
      else{previous=current;current=a.tick;}
      snapshot_time=at;received=true;
    }
    if(!bound)continue;
    const double blend=std::clamp((at-snapshot_time)/tick,0.0,1.0);
    const double shown=double(previous)+blend*double(current-previous);
    if(received && current==previous+1) {
      ++r.receipts;
      const double behind=double(current)-shown;
      r.receipt_delay_min=std::min(r.receipt_delay_min,behind);r.receipt_delay_max=std::max(r.receipt_delay_max,behind);
    }
    if(last_shown>=0 && (shown-last_shown)*tick>(at-last_at)*1.5+1e-9)++r.jumps;
    last_shown=shown;last_at=at;
  }
  return r;
}
double ms(double seconds){return seconds*1000;}
std::string describe(const std::string &name,const Result &r) {
  char text[512];
  std::snprintf(text,sizeof text,"%-10s target %6.1f ms, delay %6.1f ms (max %6.1f), behind host %6.1f ms; "
      "underrun frames %.3f%% (%llu events), snaps %llu, rate [%.3f, %.3f], backward %llu, jumps %llu, "
      "rate violations %llu, stale drops %llu",
      name.c_str(),ms(r.mean_target),ms(r.mean_delay),ms(r.max_delay),ms(r.mean_behind),r.underrun_fraction*100,
      (unsigned long long)r.underruns,(unsigned long long)r.snaps,r.min_rate,r.max_rate,
      (unsigned long long)r.backward,(unsigned long long)r.jumps,(unsigned long long)r.rate_violations,
      (unsigned long long)r.dropped);
  return text;
}
}

int run_adaptive_interpolation_tests() {
  try {
    check(!adaptive_interpolation_enabled(NetcodeMode::original),"Original adapts its interpolation delay");
    check(adaptive_interpolation_enabled(NetcodeMode::balanced) && adaptive_interpolation_enabled(NetcodeMode::full),
        "Balanced/Full keep the fixed delay");
    const AdaptiveInterpolationSettings defaults;
    const double minimum=defaults.tick_seconds+defaults.processing_seconds;
    const auto uniform=[](double width){return [width](double,Rng &rng){return rng.unit()*width;};};
    const std::vector<Profile> profiles{
      {"clean",[](double,Rng &){return 0.0;},0},
      {"jitter20",uniform(.020),.020},
      {"jitter60",uniform(.060),.060},
      // Every 2 s the path stalls for 150 ms and then delivers the held
      // snapshots together (Wi-Fi scans, congested uplinks).
      {"bursts",[](double sent,Rng &rng){
        const double phase=std::fmod(sent,2.0);
        return (phase>=1.0 && phase<1.15?1.15-phase:0.0)+rng.unit()*.002;},.150},
    };
    constexpr double seconds=60,measure_from=20;
    for(const auto *cadence:{&hz144,&hz60}) {
      std::vector<Result> results;
      for(const auto &profile:profiles) {
        const auto r=adaptive(profile,*cadence,seconds,measure_from,0x5eed1234u);
        std::cout<<cadence->name<<' '<<describe(profile.name,r)<<'\n';
        results.push_back(r);
        check(r.backward==0,profile.name+": presented time stepped backwards");
        check(r.jumps==0,profile.name+": presented time jumped forward");
        check(r.rate_violations==0,profile.name+": playback rate left the slew bound outside an underrun hold");
        check(r.snaps==0,profile.name+": playback clock snapped after its first placement");
        check(r.mean_target<=defaults.maximum_seconds+1e-9,profile.name+": delay above its bound");
      }
      const auto &clean=results[0],&j20=results[1],&j60=results[2],&bursts=results[3];
      // Steady delay: clean approaches one tick plus processing; jittery
      // links add about their jitter width (frame polling quantizes it at
      // 60 Hz, so the width check uses the 144 Hz cadence); bursts are
      // absorbed after the first.
      check(clean.mean_target<=minimum+.002 && clean.mean_delay<=minimum+.006,
          "Clean link does not approach the one-tick minimum");
      check(clean.mean_target<j20.mean_target && j20.mean_target<j60.mean_target,"Delay does not grow with jitter");
      for(const auto &[r,width]:{std::pair{&j20,.020},{&j60,.060}}) {
        const double added=r->mean_target-minimum;
        if(cadence==&hz144)check(added>=.6*width && added<=1.4*width,"Steady delay does not track the injected jitter width");
        check(std::abs(r->mean_delay-r->mean_target)<.006,"Presented delay does not settle on its target");
      }
      check(bursts.mean_target>=.150-.010,"Recurring bursts are not absorbed");
      check(clean.underrun_fraction<.002 && j20.underrun_fraction<.005 && j60.underrun_fraction<.01,
          "Underruns exceed their bound on a uniformly jittery link");
      check(bursts.underrun_fraction<.02 && bursts.underruns<=3,"Recurring bursts keep underrunning");
    }

    // A link that turns jittery and clean again: the delay grows and shrinks
    // smoothly, without a snap or a jump.
    {
      const Profile changing{"changing",[](double sent,Rng &rng){
        return sent>=20 && sent<40?rng.unit()*.060:0.0;},.060};
      std::vector<std::pair<double,double>> trace;
      const auto r=adaptive(changing,hz60,seconds,0,0x0badf00du,&trace);
      std::cout<<"60 Hz "<<describe(changing.name,r)<<'\n';
      check(r.backward==0 && r.jumps==0 && r.rate_violations==0 && r.snaps==0,
          "Delay change moved the presented time discontinuously");
      const auto at=[&](double time){
        const auto found=std::lower_bound(trace.begin(),trace.end(),std::pair{time,0.0});
        return found==trace.end()?trace.back().second:found->second;
      };
      std::cout<<"changing   delay at 15 s "<<ms(at(15))<<" ms, 38 s "<<ms(at(38))<<" ms, 58 s "<<ms(at(58))<<" ms\n";
      check(at(15)<=minimum+.006,"Initial clean phase did not reach the minimum delay");
      check(at(38)>=minimum+.040,"Jittery phase did not grow the delay");
      check(at(58)<=minimum+.006,"Delay did not shrink back after the link cleaned up");
      check(r.underrun_fraction<.01,"Changing link underran too often");
    }

    // Original: the existing fixed one-tick delay at every receipt, whatever
    // the jitter (and the jumps it shows under jitter are unchanged).
    for(const auto &profile:profiles) {
      const auto r=original(profile,hz60,seconds,0x5eed1234u);
      std::cout<<"original "<<profile.name<<": delay behind the newest tick at receipt ["<<r.receipt_delay_min<<", "
          <<r.receipt_delay_max<<"] ticks over "<<r.receipts<<" receipts, forward jumps "<<r.jumps<<'\n';
      check(r.receipts>0 && std::abs(r.receipt_delay_min-1)<1e-9 && std::abs(r.receipt_delay_max-1)<1e-9,
          "Original no longer presents the fixed one-tick delay");
    }

    // Settings and degenerate inputs.
    {
      AdaptiveInterpolationClock clock;
      check(clock.advance(1)==0 && !clock.active(),"Inactive clock advanced");
      clock.receive(100,10);clock.receive(100,10.5);clock.receive(99,10.6);
      check(clock.newest()==100 && clock.statistics(10.6).arrivals==1,"Same-tick or stale arrivals were measured");
      const double first=clock.advance(10);
      check(first<=100 && std::abs((100-first)*tick-clock.target_delay())<1e-9,"First placement is not at the target delay");
      check(clock.advance(9.9)==first,"Playback ran backwards with an earlier frame time");
      bool rejected=false;
      try{AdaptiveInterpolationSettings bad;bad.maximum_seconds=.01;AdaptiveInterpolationClock{bad};}
      catch(const FormatError &){rejected=true;}
      check(rejected,"Maximum delay below one tick admitted");
    }
    std::cout<<"Adaptive interpolation tracks jitter, stays continuous and keeps Original fixed\n";
    return 0;
  } catch(const std::exception &error) {
    std::cerr<<"adaptive interpolation: "<<error.what()<<'\n';
    return 1;
  }
}
