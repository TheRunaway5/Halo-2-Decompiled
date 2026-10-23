#pragma once
#include "halo2/campaign_source_runtime.hpp"
#include "halo2/model_skin_submission.hpp"
#include "rendering/models/model_skin_cpu_materialization.hpp"
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <bit>
#include <cerrno>
#include <cfenv>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>

namespace halo2::campaign_reference_capture {
// A finite external-input fixture, not an alternative game clock/scheduler.
struct Options {
  float interval{},fraction{};
  std::uint32_t auxiliary_seed{};
  std::filesystem::path trace;
};
struct Invocation {
  bool campaign{},play{},multiplayer{},menu{},map_supplied{},mute{},incompatible{};
  unsigned frames{};
  std::filesystem::path screenshot;
};
inline float real(std::string_view text) {
  float result{};
  const auto parsed=std::from_chars(text.data(),text.data()+text.size(),result);
  if(text.empty() || parsed.ec!=std::errc{} || parsed.ptr!=text.data()+text.size() ||
      !std::isfinite(result))throw FormatError("Invalid campaign reference float input");
  return result;
}
inline std::uint32_t seed(std::string_view text) {
  std::uint32_t result{};
  const auto parsed=std::from_chars(text.data(),text.data()+text.size(),result);
  if(text.empty() || parsed.ec!=std::errc{} || parsed.ptr!=text.data()+text.size())
    throw FormatError("Invalid campaign reference auxiliary seed");
  return result;
}
inline void validate(const Options &options,const Invocation &invocation) {
  // The caller explicitly admits --campaign or --play --place-squad. The
  // receipt always distinguishes the latter from natural mission startup.
  if(!invocation.campaign || !invocation.play || invocation.multiplayer || invocation.menu ||
      !invocation.map_supplied || !invocation.mute || !invocation.frames || invocation.frames>600 ||
      invocation.screenshot.empty() || invocation.incompatible)
    throw FormatError("Campaign reference capture requires a campaign scene, --map, explicit --frames 1..600, --screenshot and --mute; incompatible modes are rejected");
  if(!std::isfinite(options.interval) || options.interval<=0 || options.interval>.05f ||
      !std::isfinite(options.fraction) || options.fraction<0 || options.fraction>1 || options.trace.empty())
    throw FormatError("Campaign reference capture requires interval (0,.05], fraction [0,1] and a trace path");
}
inline void owned_output(const std::filesystem::path &path) {
  const auto *value=std::getenv("HALO2_TEST_SCRATCH");
  if(!value || !*value || !path.is_absolute() || path.lexically_normal()!=path)
    throw FormatError("Campaign reference outputs require absolute owned HALO2_TEST_SCRATCH paths");
  const auto root=std::filesystem::canonical(value);
  const auto parent=std::filesystem::canonical(path.parent_path());
  const auto relative=parent.lexically_relative(root);
  if(relative.empty() || relative.is_absolute() || *relative.begin()==".." ||
      std::filesystem::exists(path) || std::filesystem::is_symlink(path))
    throw FormatError("Campaign reference output escapes its owner or would overwrite a file");
}
// Diagnostic allocation, hashing, stream formatting and read-only snapshots
// cannot own a source arithmetic effect, trap, rounding mode, DAZ/FTZ or errno.
class Environment {
  std::fenv_t saved_{};
  int error_=errno;
  bool saved_ok_=std::fegetenv(&saved_)==0;
public:
  Environment() {
    if(!saved_ok_) {errno=error_;throw FormatError("Campaign reference cannot save the FP environment");}
    std::fenv_t held{};
    if(std::feholdexcept(&held)!=0) {
      if(std::fesetenv(&saved_)!=0)std::terminate();
      errno=error_;
      throw FormatError("Campaign reference cannot mask diagnostic FP effects");
    }
  }
  Environment(const Environment&)=delete;
  Environment&operator=(const Environment&)=delete;
  ~Environment()noexcept {
    if(saved_ok_ && std::fesetenv(&saved_)!=0)std::terminate();
    errno=error_;
  }
};
inline std::string hex(std::span<const std::byte> bytes) {
  constexpr char alphabet[]="0123456789abcdef";
  std::string result;result.reserve(bytes.size()*2);
  for(const auto byte:bytes) {const auto b=std::to_integer<unsigned>(byte);result+=alphabet[b>>4];result+=alphabet[b&15];}
  return result;
}
inline std::string sha256(std::span<const std::byte> bytes) {
  std::array<unsigned char,EVP_MAX_MD_SIZE> digest{};unsigned count{};
  if(!EVP_Digest(bytes.data(),bytes.size(),digest.data(),&count,EVP_sha256(),nullptr) || count!=32)
    throw FormatError("Campaign reference geometry digest failed");
  return hex(std::as_bytes(std::span(digest.data(),count)));
}
inline nlohmann::json state(const CampaignSourceRuntime &source) {
  const auto value=source.reference_state();
  return {{"rng_address",value.random_address},{"rng_words",value.random_words},
      {"game_options_seed",value.game_options_seed},
      {"clock_address",value.clock_address},{"clock_24_hex",hex(value.clock)},
      // Actual retained TU1.5 clock+8, advanced by the original update owner;
      // also used by live-world receipt validation, not Simulation::ticks().
      {"source_tick",Bytes(value.clock).u32(8)}};
}
inline nlohmann::json nodes(std::span<const Transform> values) {
  auto result=nlohmann::json::array();
  for(const auto &value:values) {
    std::array<std::uint32_t,13> bits{};unsigned i=0;
    bits[i++]=std::bit_cast<std::uint32_t>(value.scale);
    for(const auto &v:value.basis)for(float f:{v.x,v.y,v.z})bits[i++]=std::bit_cast<std::uint32_t>(f);
    for(float f:{value.position.x,value.position.y,value.position.z})bits[i++]=std::bit_cast<std::uint32_t>(f);
    result.push_back(bits);
  }
  return result;
}
// CPU and deferred banks have different raw positions by design. Compare the
// complete selected/materialized Vertex bytes without editing the source bank.
// Actual GPU execution/readback is a separate mandatory renderer-counter gate.
inline std::string geometry(std::span<const Vertex> vertices,std::span<const ModelSkinSubmission> submissions) {
  if(submissions.empty())return sha256(std::as_bytes(vertices));
  std::vector<Vertex> copy(vertices.begin(),vertices.end());
  std::vector<bool> touched(vertices.size());
  struct Record {
    std::uint32_t destination{},palette{};
    std::array<std::uint32_t,4> bones{};
    std::array<float,4> weights{};
    std::uint32_t post_identity_placement{},palette_count{};
  };
  for(const auto &submission:submissions) {
    if(!submission.geometry)throw FormatError("Campaign reference has a null skin owner");
    const auto &g=*submission.geometry;
    if(g.vertices.size()!=g.bindings.size() || submission.first>copy.size() ||
        g.vertices.size()>copy.size()-submission.first || copy.size()>UINT32_MAX || g.palette.size()>UINT32_MAX)
      throw FormatError("Campaign reference skin range is invalid");
    std::vector<Record> records;records.reserve(g.vertices.size());
    for(std::size_t i=0;i<g.vertices.size();++i) {
      const auto destination=submission.first+i;
      if(touched[destination])throw FormatError("Campaign reference skin destinations overlap");
      touched[destination]=true;
      Record record;record.destination=std::uint32_t(destination);record.weights=g.bindings[i].weights;
      for(unsigned j=0;j<4;++j)record.bones[j]=g.bindings[i].bones[j];
      record.post_identity_placement=g.post_identity_placement;record.palette_count=std::uint32_t(g.palette.size());
      records.push_back(record);
    }
    model_skin_cpu_materialize(std::span(copy),std::span(records),std::span(g.palette));
  }
  return sha256(std::as_bytes(std::span(copy)));
}
class Trace {
  std::FILE *file_{};
  std::size_t bytes_{};
  std::uint64_t sequence_{};
public:
  explicit Trace(const std::filesystem::path &path) {
    Environment guard;owned_output(path);file_=std::fopen(path.c_str(),"wx");
    if(!file_)throw FormatError("Campaign reference trace could not be created");
  }
  ~Trace()noexcept {
    // Do not construct a throwing guard while unwinding a source failure.
    // Every record is already flushed. If the environment cannot be saved,
    // leave final descriptor retirement to process exit rather than run more
    // diagnostic library code under an unowned FP state.
    const int saved_error=errno;std::fenv_t saved{};
    if(file_ && std::fegetenv(&saved)==0) {
      std::fenv_t held{};
      if(std::feholdexcept(&held)==0)std::fclose(file_);
      if(std::fesetenv(&saved)!=0)std::terminate();
    }
    errno=saved_error;
  }
  Trace(const Trace&)=delete;
  Trace&operator=(const Trace&)=delete;
  template<class Fn> void record(Fn make) {
    Environment guard;
    auto value=make();value["sequence"]=sequence_++;
    auto text=value.dump();text+='\n';
    constexpr std::size_t limit=64u*1024u*1024u;
    if(text.size()>limit-bytes_)throw FormatError("Campaign reference trace exceeded its finite budget");
    if(std::fwrite(text.data(),1,text.size(),file_)!=text.size() || std::fflush(file_)!=0)
      throw FormatError("Campaign reference trace write failed");
    bytes_+=text.size();
  }
};
}
