#pragma once
#include <cstdint>
#include <optional>
#include <stdexcept>

namespace halo2 {
// Diagnostic capture budget only. The caller observes the actual MenuState
// map-intro predicate; this counter never supplies UI time or navigation.
class FrontendIntroCapture {
public:
  static constexpr std::uint64_t wait_budget_ms=30000;
  explicit FrontendIntroCapture(unsigned frames):requested_(frames) {
    if(!frames)throw std::invalid_argument("Intro-clear capture requires finite --frames");
  }
  static void validate_request(unsigned frames,bool audit,bool sweep,bool movie,
                               bool initial,bool skip_intro) {
    if(!frames || audit || sweep || movie || (initial&&!skip_intro))
      throw std::invalid_argument("--menu-wait-intro-clear requires finite --frames and initial --skip-intro; audit, sweep and movies are unsupported");
  }
  bool ready(bool intro_active,std::uint64_t elapsed_ms) const {
    if(!first_clear_ui_clock_ && elapsed_ms>=wait_budget_ms)
      throw std::runtime_error("Frontend map intro did not clear within the capture wait budget");
    if(intro_active && first_clear_ui_clock_)
      throw std::runtime_error("Frontend map intro restarted during a visible-frame capture");
    return !intro_active;
  }
  void presented(bool intro_active,std::uint32_t ui_clock,std::uint64_t elapsed_ms) {
    if(done())throw std::logic_error("Frontend intro capture exceeded its visible-frame budget");
    if(!ready(intro_active,elapsed_ms)){++warmup_frames_;return;}
    if(!first_clear_ui_clock_){first_clear_ui_clock_=ui_clock;waited_ms_=elapsed_ms;}
    last_clear_ui_clock_=ui_clock;
    ++qualified_frames_;
  }
  bool next_is_final() const {return qualified_frames_+1==requested_;}
  bool done() const {return qualified_frames_==requested_;}
  void require_complete() const {
    if(!done() || !first_clear_ui_clock_)
      throw std::runtime_error("Frontend exited before completing its intro-clear capture");
  }
  unsigned warmup_frames() const {return warmup_frames_;}
  unsigned qualified_frames() const {return qualified_frames_;}
  std::uint32_t first_clear_ui_clock() const {return first_clear_ui_clock_.value();}
  std::uint32_t last_clear_ui_clock() const {return last_clear_ui_clock_.value();}
  std::uint64_t waited_ms() const {return waited_ms_;}
private:
  unsigned requested_{},warmup_frames_{},qualified_frames_{};
  std::optional<std::uint32_t> first_clear_ui_clock_,last_clear_ui_clock_;
  std::uint64_t waited_ms_{};
};
}
