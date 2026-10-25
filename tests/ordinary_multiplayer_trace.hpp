#pragma once
// Test-only synchronous trace of already computed ordinary inputs and poses.
// Hashes cover explicit little-endian words/raw source bytes, never C++ padding.
#include "halo2/multiplayer.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <openssl/evp.h>
#include <bit>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fcntl.h>
#include <limits>
#include <memory>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>
#if defined(__linux__) && defined(__x86_64__) && (defined(__GNUC__) || defined(__clang__))
#include <xmmintrin.h>
#else
#error The test trace FP-preservation component is qualified only on Linux x86-64
#endif

namespace halo2::ordinary_trace {
inline void require(bool value,const char *why){if(!value)throw std::runtime_error(why);}
// Preserve the complete x87 register/environment image and MXCSR around our
// diagnostic library work, including live logical x87 values and errno.
class Environment {
  alignas(16) std::array<std::byte,512> image_{};
  int error_=errno;
public:
  Environment()noexcept {
    asm volatile("fxsave64 %0":"=m"(image_)::"memory");
    unsigned short control{};
    asm volatile("fnstcw %0":"=m"(control));control|=0x3fu;
    asm volatile("fldcw %0\n\tfnclex"::"m"(control):"memory");
    _mm_setcsr((_mm_getcsr()|0x1f80u)&~0x3fu);
  }
  ~Environment()noexcept {
    asm volatile("fxrstor64 %0"::"m"(image_):"memory","st","st(1)","st(2)","st(3)",
        "st(4)","st(5)","st(6)","st(7)","xmm0","xmm1","xmm2","xmm3","xmm4","xmm5",
        "xmm6","xmm7","xmm8","xmm9","xmm10","xmm11","xmm12","xmm13","xmm14","xmm15");
    errno=error_;
  }
  Environment(const Environment&)=delete;
};
inline std::array<std::byte,8> little(std::uint64_t value)noexcept {
  std::array<std::byte,8> out{};for(unsigned i=0;i<8;++i)out[i]=std::byte(value>>(i*8));return out;
}
class Hash {
  std::unique_ptr<EVP_MD_CTX,decltype(&EVP_MD_CTX_free)> context_{EVP_MD_CTX_new(),EVP_MD_CTX_free};
public:
  Hash(){require(bool(context_) && EVP_DigestInit_ex(context_.get(),EVP_sha256(),nullptr)==1,"Trace SHA initialization failed");}
  void bytes(std::span<const std::byte> value){require(EVP_DigestUpdate(context_.get(),value.data(),value.size())==1,"Trace SHA update failed");}
  void word(std::uint64_t value){const auto out=little(value);bytes(out);}
  void real(float value){word(std::bit_cast<std::uint32_t>(value));}
  void vector(Vec3 value){real(value.x);real(value.y);real(value.z);}
  void transform(const Transform &value){real(value.scale);for(const auto &basis:value.basis)vector(basis);vector(value.position);}
  std::array<std::byte,32> finish(){std::array<std::byte,32> out{};unsigned n{};
    require(EVP_DigestFinal_ex(context_.get(),reinterpret_cast<unsigned char*>(out.data()),&n)==1 && n==out.size(),"Trace SHA finish failed");return out;}
};
struct Limits {
  std::uint64_t bytes=8u*1024u*1024u,records=20000,nodes=4096,bank_bytes=256u*1024u*1024u,
      total_bank_bytes=16ull*1024ull*1024ull*1024ull;
  unsigned ticks=120;
};
enum class BankCheckpoint : std::uint32_t { preparation=1,final=2 };
struct Counts {
  std::uint64_t sources{},inputs{},views{},rectangles{},preparations{},finals{};
  std::array<std::uint64_t,4> nodes{};
  bool operator==(const Counts&)const=default;
};
class Sink {
  int file_=-1;
  Limits limit_;
  std::uint64_t bytes_{},records_{},inputs_{},source_bytes_{},source_ranges_{},views_{},rectangles_{};
  std::array<std::uint64_t,4> nodes_{};
  const SourceRuntimeMemory *memory_{};
  std::uint64_t sources_{},preparations_{},finals_{};
  bool failed_{},finished_{};
  static constexpr std::uint64_t footer_bytes=24+12*8;
  void write(std::span<const std::byte> value) {
    auto rest=value;
    while(!rest.empty()){
      const auto n=::write(file_,rest.data(),rest.size());
      if(n<0 && errno==EINTR)continue;
      require(n>0,"Trace file write failed");rest=rest.subspan(std::size_t(n));
    }
    bytes_+=value.size();
  }
  void word(std::uint64_t value){const auto out=little(value);write(out);}
  void real(float value){word(std::bit_cast<std::uint32_t>(value));}
  void vector(Vec3 value){real(value.x);real(value.y);real(value.z);}
  void header(unsigned kind,std::uint64_t payload) {
    require(!finished_ && !failed_,"Trace is finished/incomplete");
    require(records_<limit_.records && payload<=limit_.bytes && bytes_<=limit_.bytes-footer_bytes &&
        24+payload<=limit_.bytes-footer_bytes-bytes_,"Trace byte/record cap exceeded");
    word(kind);word(payload);word(records_++);
  }
  template<class Fn> void record(Fn body) {
    Environment preserved;
    try{body();}catch(...){failed_=true;throw;}
  }
  std::array<std::byte,32> bank() {
    require(memory_!=nullptr,"Trace source owner is absent");Hash hash;
    std::uint64_t count{},bytes{},end{};
    memory_->visit_committed([&](std::uint32_t address,std::span<const std::byte> value){
      require(!value.empty() && address>=end && value.size()<=limit_.bank_bytes-bytes &&
          source_bytes_<=limit_.total_bank_bytes && value.size()<=limit_.total_bank_bytes-source_bytes_,
          "Trace committed-bank cap/order exceeded");
      end=std::uint64_t(address)+value.size();require(end<=0x100000000ull,"Trace source range wraps");
      hash.word(address);hash.word(value.size());hash.bytes(value);bytes+=value.size();source_bytes_+=value.size();++count;
    });
    source_ranges_+=count;hash.word(count);hash.word(bytes);return hash.finish();
  }
  void completed(const MultiplayerState &state,Hash &hash) {
    hash.word(state.tick);hash.word(state.source_body_poses_authoritative);hash.word(state.source_body_poses.size());
    for(const auto &pose:state.source_body_poses){hash.word(pose.object);hash.word(pose.nodes.size());
      require(pose.nodes.size()<=limit_.nodes,"Trace completed node cap exceeded");
      for(const auto &node:pose.nodes)hash.transform(node);hash.vector(pose.sphere.center);hash.real(pose.sphere.radius);}
    // The retained engine delivery bank preserves its own order/serial/raw
    // fields. This does not claim transient callbacks in other owners.
    hash.word(state.source_engine_events.size());
    for(const auto &event:state.source_engine_events){hash.word(event.serial);hash.word(event.tick);hash.word(event.random_seed);
      for(auto value:event.words)hash.word(value);hash.word(event.data);hash.word(event.mode);hash.word(event.game_over);
      for(const auto &player:event.players){hash.word(player.datum);hash.word(player.flags);hash.word(std::uint8_t(player.membership));}}
  }
public:
  explicit Sink(const std::filesystem::path &path,Limits limit={}) :limit_(limit) {
    Environment preserved;
    const auto *owned=std::getenv("HALO2_TEST_SCRATCH");
    require(owned && *owned && path.is_absolute() && path.lexically_normal()==path,"Trace requires absolute owned scratch output");
    const auto root=std::filesystem::canonical(owned),parent=std::filesystem::canonical(path.parent_path());
    const auto relative=parent.lexically_relative(root);
    require(!relative.is_absolute() && (relative.empty() || *relative.begin()!="..") &&
        limit_.bytes>=256 && limit_.bytes<=8u*1024u*1024u && limit_.records && limit_.nodes && limit_.ticks &&
        limit_.bank_bytes && limit_.total_bank_bytes,
        "Trace output escapes owner or limits invalid");
    file_=::open(path.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);
    require(file_>=0,"Trace output exists/is unsafe or cannot be created");
    try{const std::array<std::byte,8> magic{std::byte{'H'},std::byte{'2'},std::byte{'O'},std::byte{'T'},std::byte{'R'},std::byte{'0'},std::byte{'2'},std::byte{'\n'}};
      write(magic);word(limit_.bytes);word(limit_.records);word(limit_.nodes);word(limit_.ticks);
    }catch(...){::close(file_);file_=-1;throw;}
  }
  Sink(const Sink&)=delete;
  ~Sink()noexcept {
    Environment preserved;
    if(file_>=0){if(!finished_)try{finish(false);}catch(...){}::close(file_);}
  }
  void source(const SourceRuntimeMemory &memory){record([&]{require(!sources_ && !finished_,"Trace source owner rebound");memory_=&memory;
    const auto hash=bank();header(1,32);write(hash);++sources_;});}
  void input(unsigned step,std::span<const Controls> controls,const MultiplayerState &state){record([&]{
    require(step==inputs_ && inputs_<limit_.ticks && controls.size()<=4,"Trace input ordinal/count cap exceeded");
    Hash hash;hash.word(controls.size());
    for(const auto &c:controls){for(float v:{c.forward,c.right,c.yaw,c.pitch,c.look_stick})hash.real(v);
      for(bool v:{c.jump,c.crouch,c.fire,c.reload,c.skip_cinematic,c.grenade,c.pickup,c.dual_wield,c.switch_weapon,c.fire_secondary,
          c.switch_grenade,c.accept,c.cancel,c.melee,c.flashlight,c.start,c.back})hash.word(v);
      hash.word(std::uint32_t(c.zoom));hash.word(c.zoom_reset_serial);hash.word(c.pickup_milliseconds);hash.word(c.dual_wield_milliseconds);
      hash.word(c.vehicle_interaction_target.has_value());if(c.vehicle_interaction_target){hash.word(c.vehicle_interaction_target->action);
        hash.word(c.vehicle_interaction_target->index);hash.word(c.vehicle_interaction_target->object);}}
    completed(state,hash);const auto values=hash.finish(),memory=bank();header(2,16+64);word(step);word(state.tick);write(values);write(memory);++inputs_;
  });}
  void view(unsigned view,const MultiplayerState &state,const Camera &camera){record([&]{
    require(view<4,"Trace local view exceeds four");Hash state_hash;
    for(const auto &player:state.players){state_hash.word(player.connected);state_hash.word(player.source_unit);
      state_hash.vector(player.position);state_hash.real(player.yaw);state_hash.real(player.pitch);state_hash.real(player.crouch);}
    const auto hash=state_hash.finish();header(3,16+14*8+32);word(view);word(state.tick);
    vector(camera.position);real(camera.yaw);real(camera.pitch);real(camera.field_of_view);word(camera.horizontal_field_of_view);
    word(camera.oriented);vector(camera.forward);vector(camera.up);write(hash);++views_;
  });}
  void rectangle(unsigned view,const UiRect &value){record([&]{require(view<4,"Trace rectangle view exceeds four");
    header(4,5*8);word(view);real(value.top);real(value.left);real(value.bottom);real(value.right);++rectangles_;
  });}
  void nodes(unsigned view,std::uint32_t tick,const MultiplayerNodeObservation &value){record([&]{
    const auto stage=std::uint32_t(value.stage);require(view<4 && stage>=1 && stage<=4 && value.nodes.size()<=limit_.nodes,"Trace node/view/stage cap exceeded");
    Hash hash;for(const auto &node:value.nodes)hash.transform(node);const auto digest=hash.finish();
    header(5,11*8+32);word(view);word(tick);word(stage);word(value.owner);word(value.render_model);word(value.mode);
    real(value.fraction);word(value.nodes.size());word(value.parent_known);word(value.parent);word(value.parent_node);write(digest);++nodes_[stage-1];
  });}
  void checkpoint(BankCheckpoint stage,unsigned ordinal,std::uint32_t tick){record([&]{
    require(!finals_ && ((stage==BankCheckpoint::preparation && ordinal==preparations_ && preparations_<limit_.ticks) ||
        (stage==BankCheckpoint::final && ordinal==0)),"Trace bank checkpoint stage/order/cap differs");
    const auto digest=bank();header(6,3*8+32);word(std::uint32_t(stage));word(ordinal);word(tick);write(digest);
    if(stage==BankCheckpoint::preparation)++preparations_;else ++finals_;
  });}
  void finish(bool complete=true) {
    Environment preserved;
    require(!finished_,"Trace finished twice");
    // Reserved space makes cap failures explicit without silently truncating.
    word(0xffffffffu);word(12*8);word(records_);word(complete&&!failed_);word(inputs_);word(views_);word(rectangles_);
    for(auto n:nodes_)word(n);word(source_ranges_);word(source_bytes_);word(preparations_);word(finals_);finished_=true;memory_=nullptr;
    require(::fsync(file_)==0,"Trace final flush failed");
  }
  bool failed()const noexcept{return failed_;}
  std::uint64_t records()const noexcept{return records_;}
  Counts counts()const noexcept{return {sources_,inputs_,views_,rectangles_,preparations_,finals_,nodes_};}
};
inline void attach(MultiplayerOptions &options,Sink &sink) {
  require(!options.source_world_observer && !options.input_observer && !options.view_state_observer &&
      !options.view_rectangle_observer && !options.node_observer,"Trace attachment would replace another observer");
  options.source_world_observer=[&](const SourceRuntimeMemory &memory){sink.source(memory);};
  options.input_observer=[&](unsigned step,std::span<const Controls> controls,const MultiplayerState &state){sink.input(step,controls,state);};
  options.view_state_observer=[&](unsigned view,const MultiplayerState &state,const Camera &camera){sink.view(view,state,camera);};
  options.view_rectangle_observer=[&](unsigned view,const UiRect &rect){sink.rectangle(view,rect);};
  options.node_observer=[&](unsigned view,std::uint32_t tick,const MultiplayerNodeObservation &nodes){sink.nodes(view,tick,nodes);};
}
} // namespace halo2::ordinary_trace
