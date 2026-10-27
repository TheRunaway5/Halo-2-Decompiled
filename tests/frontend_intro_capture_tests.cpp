#include "halo2/frontend_intro_capture.hpp"
#include <iostream>
#include <stdexcept>

namespace {
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
template<class F>void rejects(F call,const char *message) {
  try{call();}catch(const std::exception&){return;}
  throw std::runtime_error(message);
}
}
int main()try {
  using halo2::FrontendIntroCapture;
  FrontendIntroCapture capture(4);
  // Thousands of successful black presents and arbitrary paint-only time
  // must not consume the four actual visible frames requested by validation.
  for(unsigned frame=0;frame<1000;++frame) {
    check(!capture.ready(true,frame*4),"Active map intro was admitted for capture");
    capture.presented(true,5000+frame,frame*4);
    check(!capture.done() && capture.qualified_frames()==0,"Black warm-up exhausted the visible capture budget");
  }
  check(capture.warmup_frames()==1000,"Warm-up presents were not accounted for");
  rejects([&]{capture.require_complete();},"Uncleared map intro certified a completed capture");
  const std::uint32_t first=0xfffffffeu;
  for(unsigned frame=0;frame<4;++frame) {
    check(capture.ready(false,4001+frame),"Cleared map intro was rejected");
    check(capture.next_is_final()==(frame==3),"Screenshot was not assigned to the fourth clear frame");
    capture.presented(false,first+frame,4001+frame);
    check(capture.done()==(frame==3),"Capture completed before all requested clear presents");
  }
  capture.require_complete();
  check(capture.first_clear_ui_clock()==first && capture.last_clear_ui_clock()==1 && capture.waited_ms()==4001,
        "Read-only source UI clock observations lost wraparound or first-clear timing");
  rejects([&]{capture.presented(false,2,4005);},"Capture accepted more than its finite visible budget");
  FrontendIntroCapture timeout(4);
  timeout.presented(true,100,29999);
  rejects([&]{timeout.ready(true,30000);},"An uncleared intro waited without a bound");
  rejects([&]{timeout.ready(false,30000);},"A late first observation escaped the finite wait budget");
  FrontendIntroCapture restarted(4);
  restarted.presented(false,0,1);
  rejects([&]{restarted.ready(true,2);},"A restarted map intro mixed black frames into a visible capture");
  rejects([&]{restarted.require_complete();},"Early exit certified an incomplete capture");
  FrontendIntroCapture::validate_request(4,false,false,false,true,true);
  FrontendIntroCapture::validate_request(4,false,false,false,false,false);
  rejects([]{FrontendIntroCapture value(0);},"Unbounded capture was admitted");
  for(unsigned invalid=0;invalid<5;++invalid)
    rejects([&]{FrontendIntroCapture::validate_request(invalid?4:0,invalid==1,invalid==2,invalid==3,true,invalid!=4);},
        "An incompatible diagnostic capture request was admitted");
  std::cout<<"PASS finite visible-frame capture, actual intro predicate, clock observations, timeout and incompatible modes\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
