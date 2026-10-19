#include "halo2/campaign_capture_boundary.hpp"
#include "halo2/cinematic_crossfade.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
#include <string_view>
using namespace halo2;
void check(bool b,const char*m){if(!b)throw std::runtime_error(m);}
int main(int argc,char**argv)try {
  const bool unbounded=argc==2 && std::string_view(argv[1])=="--old-unbounded";
  for(int due:{1,2,3,5,30,150})for(int request_tick=1;request_tick<=due;++request_tick) {
    CampaignCaptureCarry carry;CinematicCrossfadeState fade;fade.screen_start(true);
    const CampaignFrameTime admitted{due,float(due)*(1.f/30),.375f};
    auto frame=carry.admit(admitted,false);int remaining=frame.ticks,completed{},tick{},camera{},captured=-1;
    // The opening uses two seconds. The150-tick stress bank uses an eight
    // second synthetic effect so its original timer has not already expired.
    const auto step=[&]{++tick;++completed;if(tick==request_tick)fade.set(due>30?8.f:2.f,0);if(tick==request_tick+1)camera=1;};
    campaign_frame_ticks(remaining,[]{return true;},step,[&]{return !unbounded && fade.pending;});
    fade.update(accepted_campaign_frame_elapsed(frame.elapsed,frame.ticks,completed));
    if(fade.frame().capture){captured=camera;fade.capture_complete();}
    check(captured==0,"Crossfade captured the incoming council camera instead of the outgoing space scene");
    carry.defer(frame,completed,remaining);
    // An immediate follow-up render consumes the previously admitted ticks,
    // without borrowing a second wall-time interval or changing its fraction.
    frame=carry.admit({0,0,.375f},false);remaining=frame.ticks;completed=0;
    campaign_frame_ticks(remaining,[]{return true;},step,[]{return false;});
    carry.defer(frame,completed,remaining);
    check(tick==due && carry.pending_ticks()==0 && frame.fraction==.375f,
        "Capture boundary dropped/doubled admitted ticks or changed the source fraction");
  }
  CampaignCaptureCarry carry;carry.defer({5,.2f,.25f},2,3);
  check(carry.admit({0,0,0},true).ticks==0 && carry.pending_ticks()==3,
      "Pause consumed an unrendered capture tick budget");
  const auto resumed=carry.admit({2,.05f,.75f},false);
  check(resumed.ticks==5 && resumed.fraction==.75f && std::abs(resumed.elapsed-.15f)<1e-7f,
      "Resume discarded the accepted elapsed remainder or current source fraction");
  carry.defer({4,.15f,.25f},1,3);carry.clear();
  check(carry.admit({1,1.f/30,.5f},false).ticks==1,"Restore retained ticks from the discarded timeline");
  bool rejected=false;try{carry.defer({3,.1f,0},1,1);}catch(const std::invalid_argument&){rejected=true;}
  check(rejected,"Inconsistent capture budget was silently admitted");
  check(std::signbit(CampaignCaptureCarry{}.admit({0,-0.f,.125f},false).elapsed),
      "An empty carry changed the original signed-zero elapsed value");
  std::cout<<"PASS 1..150 tick capture boundaries, outgoing scene, exact tick conservation, fractional clock, pause/resume and timeline discard\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
