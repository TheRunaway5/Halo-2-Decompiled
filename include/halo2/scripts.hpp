#pragma once
#include "halo2/content.hpp"
#include "halo2/source_script.hpp"
#include "halo2/native_authority_boundary.hpp"
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <optional>
namespace halo2 {
struct ScriptValue {
  std::uint16_t type = 4;
  std::uint32_t bits{};
  std::string text;
  double number() const;
  bool truth() const { return number() != 0; }
  static ScriptValue numeric(double, std::uint16_t type = 6);
  bool operator==(const ScriptValue &)const=default;
};
struct ScriptAudit {
  std::size_t active{}, calls{}, script_calls{};
  std::map<std::string, std::size_t> builtins;
};
class ScriptVm {
  struct SavedState;
public:
  class Snapshot {
    friend class ScriptVm;
    std::shared_ptr<const SavedState> state_;
  };
  using Host = std::function<ScriptValue(const std::string &,
                                         const std::vector<ScriptValue> &)>;
  // Used only during global construction; never invokes the general host on
  // a partially constructed Simulation. The two MCC clock getters declare
  // LONG(), independently of their expression's requested coercion.
  using InitialBuiltin = std::function<std::optional<ScriptValue>(
      const std::string &,const std::vector<ScriptValue> &)>;
  using ObjectResolver = std::function<std::uint32_t(int, unsigned)>;
  // Writes return the value converted to the declaration's actual type.
  // Compiled expression types can differ from external declaration types.
  using ExternalGlobals = std::function<ScriptValue(
      unsigned, const std::optional<ScriptValue> &)>;
  //207CF0 consumes the same caller-owned primary stream as host random_range.
  //The callback runs for completion too and returns the next unvisitedchild.
  using RandomChoice = std::function<std::optional<unsigned>(unsigned,std::uint32_t)>;
  explicit ScriptVm(const ScriptProgram &, Host, ObjectResolver = {},
                    ExternalGlobals = {});
  ScriptVm(const ScriptProgram &, Host, ObjectResolver, ExternalGlobals,
           SourceHsObjectListServices);
  ScriptVm(const ScriptProgram &, Host, ObjectResolver, ExternalGlobals,
           SourceHsObjectListServices,InitialBuiltin);
  ScriptAudit audit() const;
  void start(const std::string &name);
  void start_command(unsigned script,std::uint32_t actor);
  void stop_command(std::uint32_t actor);
  bool command_running(unsigned script,std::uint32_t actor)const;
  std::vector<std::uint32_t> object_list_roots()const;
  std::optional<std::uint32_t> command_actor()const{return current_actor_;}
  void wait_for_command();
  void command_ready(std::function<bool(std::uint32_t)> ready){NativeAuthorityBoundary::Call call(authority_);++authority_.mutations;command_ready_=std::move(ready);}
  void start_level();
  void set_random_choice(RandomChoice);
  void tick(std::uint64_t tick);
  Snapshot snapshot() const;
  void restore(const Snapshot &);
  // The host may request a world restore during a call. Finish that call,
  // then stop dispatching until the simulation reaches a safe boundary.
  void suspend_tick() {NativeAuthorityBoundary::Call call(authority_);++authority_.mutations;suspended_ = true;}
  const std::vector<std::string> &diagnostics() const { return diagnostics_; }
  std::size_t running() const;
  const std::vector<ScriptValue> &globals() const { return globals_; }
  struct AuthorityFrame {
    std::uint32_t id{};
    bool entered{}, returned_call{};
    std::string name;
    std::vector<std::uint32_t> children;
    std::vector<ScriptValue> values;
    std::size_t next{};
    unsigned wait_state{};
    std::uint64_t wait_start{};
    std::int64_t wait_interval=30,wait_timeout=-1;
    std::uint32_t random_visited{};
    bool operator==(const AuthorityFrame &)const=default;
  };
  struct AuthorityWakeState {
    std::uint64_t wake{};
    bool dormant{};
    bool operator==(const AuthorityWakeState &)const=default;
  };
  struct AuthorityThread {
    std::size_t script{};
    std::uint64_t wake{};
    bool stopped{},dormant{},faulted{};
    std::vector<AuthorityFrame> stack;
    std::optional<std::uint32_t> actor;
    bool command_wait{};
    //TU1.5 thread flag2/+0xC: first schedule interrupted by external sleep.
    std::optional<AuthorityWakeState> external_sleep;
    bool operator==(const AuthorityThread &)const=default;
  };
  struct AuthorityCheckpoint {
    std::uint32_t version=1;
    std::array<std::byte,32> program_binding{};
    std::array<bool,7> local_services{};
    std::deque<AuthorityThread> threads;
    std::vector<ScriptValue> globals;
    std::vector<std::string> diagnostics;
    bool suspended{};
    std::optional<std::uint32_t> actor;
    bool operator==(const AuthorityCheckpoint &)const=default;
  };
  class PreparedAuthority {
  public:
    ~PreparedAuthority();
    PreparedAuthority(PreparedAuthority &&) noexcept;
    PreparedAuthority &operator=(PreparedAuthority &&) noexcept;
    bool for_owner(const ScriptVm &)const noexcept;
  private:
    friend class ScriptVm;
    PreparedAuthority();
    struct Impl;
    std::unique_ptr<Impl> impl_;
  };
  // Separate from campaign revert: retain diagnostics/suspension and accept
  // an equivalent independently bound program. Services stay local. The root
  // restores referenced list/object banks and RNG under the same barrier.
  AuthorityCheckpoint authority_checkpoint()const;
  PreparedAuthority prepare_authority(const AuthorityCheckpoint &);
  bool commit_authority(PreparedAuthority &&) noexcept;

private:
  using Frame=AuthorityFrame;
  using Thread=AuthorityThread;
  NativeAuthorityBoundary authority_;
  std::array<bool,7> authority_services()const;
  void validate_authority(const AuthorityCheckpoint &)const;
  const ScriptProgram &program_;
  Host host_;
  ObjectResolver object_resolver_;
  ExternalGlobals external_globals_;
  //Snapshot restoration also requires the caller's list/reference pool bank,
  //just as random callbacks require restoration of their primary RNG owner.
  SourceHsObjectListServices object_lists_;
  RandomChoice random_choice_;
  std::deque<Thread> threads_;
  std::vector<ScriptValue> globals_;
  std::vector<std::string> diagnostics_;
  bool updating_{}, suspended_{};
  std::optional<std::uint32_t> current_actor_;
  Thread *current_thread_{};
  std::function<bool(std::uint32_t)> command_ready_;
  const ScriptNode &node(std::uint32_t) const;
  std::string string(std::uint32_t) const;
  ScriptValue literal(const ScriptNode &) const;
  ScriptValue convert(ScriptValue, unsigned target) const;
  ScriptValue read_global(const ScriptNode &) const;
  ScriptValue write_global(const ScriptNode &, ScriptValue);
  Thread &activate(std::size_t script, std::uint64_t wake);
  void update(Thread &, std::uint64_t);
};
} // namespace halo2
