#pragma once
#include "halo2/content.hpp"
#include "halo2/source_script.hpp"
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
  ScriptAudit audit() const;
  void start(const std::string &name);
  void start_command(unsigned script,std::uint32_t actor);
  void stop_command(std::uint32_t actor);
  bool command_running(unsigned script,std::uint32_t actor)const;
  std::vector<std::uint32_t> object_list_roots()const;
  std::optional<std::uint32_t> command_actor()const{return current_actor_;}
  void wait_for_command();
  void command_ready(std::function<bool(std::uint32_t)> ready){command_ready_=std::move(ready);}
  void start_level();
  void set_random_choice(RandomChoice);
  void tick(std::uint64_t tick);
  Snapshot snapshot() const;
  void restore(const Snapshot &);
  // The host may request a world restore during a call. Finish that call,
  // then stop dispatching until the simulation reaches a safe boundary.
  void suspend_tick() { suspended_ = true; }
  const std::vector<std::string> &diagnostics() const { return diagnostics_; }
  std::size_t running() const;
  const std::vector<ScriptValue> &globals() const { return globals_; }

private:
  struct Frame {
    std::uint32_t id{};
    bool entered{}, returned_call{};
    std::string name;
    std::vector<std::uint32_t> children;
    std::vector<ScriptValue> values;
    std::size_t next{};
    unsigned wait_state{};
    std::uint64_t wait_start{};
    std::int64_t wait_interval = 30, wait_timeout = -1;
    std::uint32_t random_visited{};
  };
  struct Thread {
    std::size_t script{};
    std::uint64_t wake{};
    bool stopped{}, dormant{}, faulted{};
    std::vector<Frame> stack;
    std::optional<std::uint32_t> actor;
    bool command_wait{};
  };
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
