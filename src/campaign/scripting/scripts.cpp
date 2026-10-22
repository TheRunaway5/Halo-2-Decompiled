#include "halo2/scripts.hpp"
#include "halo2/kernels.hpp"
#include "halo2/mcc_script_layout.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <set>
namespace halo2 {
struct ScriptVm::SavedState {
  const ScriptProgram *program{};
  std::deque<Thread> threads;
  std::vector<ScriptValue> globals;
};
ScriptVm::Snapshot ScriptVm::snapshot() const {
  if (updating_)
    throw FormatError("script checkpoint requested during dispatch");
  Snapshot result;
  result.state_ = std::make_shared<SavedState>(
      SavedState{&program_, threads_, globals_});
  return result;
}
void ScriptVm::restore(const Snapshot &snapshot) {
  NativeAuthorityBoundary::Call call(authority_);
  if (updating_ || !snapshot.state_ || snapshot.state_->program != &program_)
    throw FormatError("invalid script checkpoint restore");
  ++authority_.mutations;threads_ = snapshot.state_->threads;
  globals_ = snapshot.state_->globals;
  // Fault reports describe observed execution, including discarded timelines.
  // Keep this history outside the checkpoint so a revert cannot hide a fault.
  suspended_ = false;
}
namespace {
ScriptValue scalar_convert(ScriptValue value, unsigned target) {
  if (value.type == target || target == 3)
    return value;
  if (target == 4)
    return {};
  if (target == 5 && value.type == 9) {
    value.bits = value.text.empty();
    value.text.clear();
  } else if (value.type >= 50 && value.type <= 55 && target >= 50 &&
             target <= 55) { /* object subtype conversion preserves handle */
  } else
    value.bits = scalar_cast_bits(value.bits, value.type, target);
  value.type = std::uint16_t(target);
  return value;
}
std::size_t global_index(const ScriptNode &node) {
  return node.value & 65535u;
}
std::int32_t wait_ticks(std::int32_t duration) {
  // 0x2089B0 converts authored 30 Hz delays through float seconds and the
  // current game frequency, then uses x87 FISTP with nearest-even rounding.
  const float seconds = float(duration) * (1.f / 30.f);
  const float ticks = seconds * 30.f;
  const float rounded = std::nearbyint(ticks);
  if (!std::isfinite(rounded) || double(rounded) >= 2147483648. ||
      double(rounded) < -2147483648.)
    return INT32_MIN;
  return std::int32_t(rounded);
}
} // namespace
double ScriptValue::number() const {
  if (type == 6)
    return std::bit_cast<float>(bits);
  if (type == 5)
    return (bits & 255u) != 0;
  if (type == 7)
    return static_cast<std::int16_t>(bits);
  return static_cast<std::int32_t>(bits);
}
ScriptValue ScriptValue::numeric(double number, std::uint16_t type) {
  if (!std::isfinite(number) || number > INT32_MAX || number < INT32_MIN)
    throw FormatError("script numeric result outside supported range");
  ScriptValue v;
  v.type = type;
  v.bits = type == 6 ? std::bit_cast<std::uint32_t>(float(number))
                     : std::uint32_t(std::int32_t(number));
  return v;
}
const ScriptNode &ScriptVm::node(std::uint32_t id) const {
  const auto index = id & 0xffffu;
  if (id == null_datum || index >= program_.nodes.size())
    throw FormatError("script node index outside syntax array");
  const auto &n = program_.nodes[index];
  if (n.salt == 0 || n.salt != id >> 16 || n.flags == 0xbaba)
    throw FormatError("stale/poison script datum");
  return n;
}
std::string ScriptVm::string(std::uint32_t at) const {
  if (at >= program_.strings.size())
    throw FormatError("script string offset outside data");
  const auto end = program_.strings.find('\0', at);
  if (end == std::string::npos)
    throw FormatError("unterminated script string");
  return program_.strings.substr(at, end - at);
}
ScriptValue ScriptVm::convert(ScriptValue value, unsigned target) const {
  if(target==31 && value.type>=50 && value.type<=61) {
    if(value.type>=56) {
      const int name=std::int16_t(value.bits);
      if(name<0 || name>=640)value.bits=null_datum;
      else if(object_resolver_)value.bits=object_resolver_(name,50);
      else throw FormatError("object-name list conversion requires a live scene");
    }
    value.bits=source_hs_object_list(value.bits,object_lists_);
    value.type=31;
    return value;
  }
  if (value.type >= 56 && value.type <= 61 && target >= 50 && target <= 55) {
    const int name = std::int16_t(value.bits);
    if (name == -1)
      value.bits = null_datum;
    else if (object_resolver_)
      value.bits = object_resolver_(name, target);
    else
      throw FormatError("object-name conversion requires a live scene");
    value.type = std::uint16_t(target);
    return value;
  }
  return scalar_convert(std::move(value), target);
}
ScriptValue ScriptVm::literal(const ScriptNode &n) const {
  ScriptValue v;
  v.type = n.type == 2 ? 2 : n.opcode;
  v.bits = n.value;
  if (v.type == 5)
    v.bits = n.value & 255u;
  if (v.type == 10 || (v.type >= 44 && v.type <= 49))
    v.bits = n.value & 65535u;
  if (v.type == 2 || v.type == 9)
    v.text = string(n.string_offset);
  return convert(v, n.type);
}
ScriptValue ScriptVm::read_global(const ScriptNode &n) const {
  const auto index = global_index(n);
  if (index & 0x8000) {
    if (!external_globals_)
      throw FormatError("external global reference unsupported");
    return convert(external_globals_(index & 0x7fff, std::nullopt), n.type);
  }
  if (index >= globals_.size())
    throw FormatError("invalid global reference");
  return convert(globals_[index], n.type);
}
ScriptValue ScriptVm::write_global(const ScriptNode &n, ScriptValue value) {
  const auto index = global_index(n);
  if (index & 0x8000) {
    if (!external_globals_)
      throw FormatError("external global assignment unsupported");
    value = convert(std::move(value), n.type);
    value=convert(external_globals_(index & 0x7fff, value),n.type);
  } else {
    if (index >= globals_.size())
      throw FormatError("invalid global assignment");
    value = convert(std::move(value), program_.globals[index].type);
    globals_[index] = value;
  }
  return value;
}
ScriptVm::ScriptVm(const ScriptProgram &p, Host host, ObjectResolver resolver,
                   ExternalGlobals external)
    : ScriptVm(p,std::move(host),std::move(resolver),std::move(external),{}) {}
ScriptVm::ScriptVm(const ScriptProgram &p, Host host, ObjectResolver resolver,
                   ExternalGlobals external,SourceHsObjectListServices object_lists)
    : ScriptVm(p,std::move(host),std::move(resolver),std::move(external),std::move(object_lists),{}) {}
ScriptVm::ScriptVm(const ScriptProgram &p, Host host, ObjectResolver resolver,
                   ExternalGlobals external,SourceHsObjectListServices object_lists,InitialBuiltin initial_builtin)
    : program_(p), host_(std::move(host)),
      object_resolver_(std::move(resolver)), external_globals_(std::move(external)),
      object_lists_(std::move(object_lists)) {
  globals_.resize(p.globals.size());
  std::vector<unsigned> state(p.globals.size());
  std::function<ScriptValue(std::uint32_t, unsigned)> evaluate;
  std::function<ScriptValue(std::size_t, unsigned)> global =
      [&](std::size_t i, unsigned depth) {
        if (i >= globals_.size() || depth > 128 || state[i] == 1)
          throw FormatError("invalid/cyclic global initializer");
        if (state[i] == 2)
          return globals_[i];
        state[i] = 1;
        globals_[i] =
            convert(evaluate(p.globals[i].root, depth + 1), p.globals[i].type);
        state[i] = 2;
        return globals_[i];
      };
  evaluate = [&](std::uint32_t id, unsigned depth) -> ScriptValue {
    if (depth > 128)
      throw FormatError("global initializer expression depth");
    const auto &n = node(id);
    if (n.flags & 4) {
      if (global_index(n) & 0x8000)
        return read_global(n);
      return convert(global(global_index(n), depth + 1), n.type);
    }
    if (n.flags & 1)
      return literal(n);
    if (n.flags & 2)
      throw FormatError("static script global initializer unsupported");
    const auto &fn = node(n.value);
    if(!(fn.flags&1) || fn.type!=2)
      throw FormatError("global initializer call lacks a function-name node");
    const auto name = string(fn.string_offset);
    const bool arithmetic=name=="+" || name=="-" || name=="*" || name=="/" || name=="min" || name=="max";
    const bool clock=(name=="game_tick_rate" && n.opcode==0x38f) ||
        (name=="game_tick_rate_scalar" && n.opcode==0x390);
    if(!arithmetic && !clock)
      throw FormatError("unsupported global initializer builtin: " + name);
    std::vector<ScriptValue> args;
    std::set<std::uint32_t> seen;
    for (auto child = fn.next; child != null_datum; child = node(child).next) {
      if (!seen.insert(child).second || args.size() > 1024)
        throw FormatError("cyclic global initializer arguments");
      args.push_back(evaluate(child, depth + 1));
    }
    if(clock) {
      if(!args.empty())throw FormatError("invalid MCC clock getter initializer arity");
      const auto value=initial_builtin?initial_builtin(name,args):std::optional<ScriptValue>{};
      if(!value)throw FormatError("MCC clock getter initializer requires a live game-clock owner");
      if(value->type!=8)throw FormatError("MCC clock getter initializer did not return its declared LONG");
      return convert(*value,n.type);
    }
    if (args.empty())
      throw FormatError("empty global initializer expression");
    double result = args[0].number();
    for (std::size_t i = 1; i < args.size(); ++i) {
      const double b = args[i].number();
      if (name == "+")
        result += b;
      else if (name == "-")
        result -= b;
      else if (name == "*")
        result *= b;
      else if (name == "/") {
        if (!b)
          throw FormatError("global initializer divide by zero");
        result /= b;
      } else if (name == "min")
        result = std::min(result, b);
      else if (name == "max")
        result = std::max(result, b);
      else
        throw FormatError("unsupported global initializer builtin: " + name);
    }
    return ScriptValue::numeric(result, n.type);
  };
  for (std::size_t i = 0; i < p.globals.size(); ++i)
    global(i, 0);
  audit();
}
ScriptAudit ScriptVm::audit() const {
  ScriptAudit result;
  for (const auto &n : program_.nodes)
    //A zero datum salt is a free pool record, even when its retained payload
    //still contains an old link (16B7A0/16BA40). Vista caches retain these
    //records rather than filling the entire payload with BABA.
    if (n.salt != 0 && n.flags != 0xbaba) {
      ++result.active;
      if (n.next != null_datum)
        node(n.next);
      if (!(n.flags & 1)) {
        const auto &fn = node(n.value);
        if (!(fn.flags & 1) || fn.type != 2)
          throw FormatError("call lacks function-name node");
        const auto name = string(fn.string_offset);
        if (n.flags & 2) {
          ++result.script_calls;
          if (n.opcode >= program_.scripts.size() ||
              program_.scripts[n.opcode].name != name)
            throw FormatError("script-call name/index mismatch");
        } else {
          ++result.calls;
          ++result.builtins[name];
        }
      }
    }
  for (const auto &s : program_.scripts)
    node(s.root);
  return result;
}
void ScriptVm::start(const std::string &name) {
  NativeAuthorityBoundary::Call call(authority_);
  const auto found =
      std::find_if(program_.scripts.begin(), program_.scripts.end(),
                   [&](const auto &s) { return s.name == name; });
  if (found == program_.scripts.end())
    throw FormatError("script not found: " + name);
  activate(std::size_t(found - program_.scripts.begin()), 0);
}
ScriptVm::Thread &ScriptVm::activate(std::size_t index, std::uint64_t wake) {
  if (index >= program_.scripts.size())
    throw FormatError("script index outside table");
  const auto &script = program_.scripts[index];
  if (script.type == 5)
    throw FormatError("command scripts require an actor context: " +
                      script.name);
  for (auto &t : threads_)
    if (t.script == index) {
      if (t.faulted)
        throw FormatError("cannot wake faulted script: " + script.name);
      ++authority_.mutations;t.wake = wake;
      t.dormant = false;
      t.stopped = false;
      t.external_sleep.reset(); //Explicit diagnostic restart, separate from HS wake.
      if (t.stack.empty())
        t.stack.push_back(Frame{script.root});
      return t;
    }
  if (threads_.size() >= 320)
    throw FormatError("script thread capacity exhausted");
  Thread t;
  ++authority_.mutations;
  t.script = index;
  t.wake = wake;
  t.stack.push_back(Frame{script.root});
  threads_.push_back(std::move(t));
  return threads_.back();
}
void ScriptVm::start_level() {
  NativeAuthorityBoundary::Call call(authority_);
  for (std::size_t i = 0; i < program_.scripts.size(); ++i) {
    const auto type = program_.scripts[i].type;
    if (type <= 2) {
      auto &t = activate(i, 0);
      t.dormant = type == 1;
    }
  }
}
void ScriptVm::start_command(unsigned script,std::uint32_t actor) {
  NativeAuthorityBoundary::Call call(authority_);
  if(script>=program_.scripts.size() || program_.scripts[script].type!=5 || actor==null_datum)
    throw FormatError("Command script requires a live actor and command definition");
  Thread next;next.script=script;next.actor=actor;next.stack.push_back(Frame{program_.scripts[script].root});
  ++authority_.mutations;
  for(auto &thread:threads_)if(thread.actor==actor){thread=std::move(next);return;}
  if(threads_.size()>=320)throw FormatError("Script thread capacity exhausted");threads_.push_back(std::move(next));
}
void ScriptVm::stop_command(std::uint32_t actor) {
  NativeAuthorityBoundary::Call call(authority_);++authority_.mutations;
  for(auto &thread:threads_)if(thread.actor==actor){thread.stopped=true;thread.command_wait=false;}
}
bool ScriptVm::command_running(unsigned script,std::uint32_t actor)const {
  return std::any_of(threads_.begin(),threads_.end(),[&](const auto &thread){return (script==null_datum || thread.script==script) && thread.actor==actor && !thread.stopped && !thread.faulted;});
}
std::vector<std::uint32_t> ScriptVm::object_list_roots()const {
  std::vector<std::uint32_t> result;
  const auto keep=[&](const ScriptValue &value){if(value.type==31 && value.bits!=null_datum)result.push_back(value.bits);};
  for(const auto &value:globals_)keep(value);
  for(const auto &thread:threads_)for(const auto &frame:thread.stack)for(const auto &value:frame.values)keep(value);
  return result;
}
void ScriptVm::wait_for_command() {
  NativeAuthorityBoundary::Call call(authority_);
  if(!current_thread_ || !current_thread_->actor || !command_ready_)throw FormatError("Command wait has no actor completion owner");
  ++authority_.mutations;current_thread_->command_wait=true;
}
void ScriptVm::set_random_choice(RandomChoice choice) {
  NativeAuthorityBoundary::Call call(authority_);
  if(updating_)throw FormatError("script random source changed during dispatch");
  ++authority_.mutations;random_choice_=std::move(choice);
}
std::size_t ScriptVm::running() const {
  return std::count_if(threads_.begin(), threads_.end(),
                       [](const auto &t) { return !t.stopped && !t.dormant; });
}
void ScriptVm::update(Thread &thread, std::uint64_t tick) {
  thread.wake = 0; //TU1.5 211B60: active-thread wake before any evaluator.
  auto finish = [&](ScriptValue value) {
    value = convert(std::move(value), node(thread.stack.back().id).type);
    thread.stack.pop_back();
    if (!thread.stack.empty())
      thread.stack.back().values.push_back(std::move(value));
  };
  for (unsigned budget = 0; budget < 10000 && !thread.stopped &&
                            !thread.dormant && thread.wake <= tick;
       ++budget) {
    if (thread.stack.size() > 128)
      throw FormatError("script expression depth limit");
    if (thread.stack.empty()) {
      if (program_.scripts[thread.script].type == 2) {
        thread.wake = tick + 1;
        thread.stack.push_back(Frame{program_.scripts[thread.script].root});
      } else
        thread.stopped = true;
      return;
    }
    auto &f = thread.stack.back();
    const auto &n = node(f.id);
    if (!f.entered) {
      f.entered = true;
      if (n.flags & 4) {
        finish(read_global(n));
        continue;
      }
      if (n.flags & 1) {
        finish(literal(n));
        continue;
      }
      const auto &fn = node(n.value);
      f.name = string(fn.string_offset);
      std::set<std::uint32_t> seen;
      for (auto arg = fn.next; arg != null_datum; arg = node(arg).next) {
        if (f.children.size() > 1024 || !seen.insert(arg).second)
          throw FormatError("cyclic/excessive script arguments");
        f.children.push_back(arg);
      }
      if (n.flags & 2) {
        if (!f.children.empty())
          throw FormatError("parameterized static scripts are not supported");
        if (thread.stack.size() >= 128)
          throw FormatError("script recursion limit");
        f.returned_call = true;
        // Optional observation of authored static-call entry, before its
        // frame is pushed. No source clock, wake or script value is changed.
        if(std::getenv("HALO2_SCRIPT_CALL_TRACE"))
          std::cout<<"Script call: tick="<<tick<<" name="<<f.name<<'\n';
        thread.stack.push_back(Frame{program_.scripts[n.opcode].root});
        continue;
      }
    }
    if (f.returned_call) {
      if (f.values.empty())
        throw FormatError("static call has no result");
      finish(f.values.back());
      continue;
    }
    if(f.name=="begin_random") {
      if(f.children.size()>32 || !random_choice_)
        throw FormatError("begin_random requires a supported source frame and primary RNG");
      const auto next=random_choice_(unsigned(f.children.size()),f.random_visited);
      if(next) {
        if(*next>=f.children.size() || (f.random_visited&(1u<<*next)))
          throw FormatError("begin_random primary callback returned an invalid child");
        //2099F0 queues nonliteral expressions, then207CF0 marks this bit
        //before the nested call executes. Inner RNG/sleeps remain interleaved.
        f.random_visited|=1u<<*next;
        thread.stack.push_back(Frame{f.children[*next]});
        continue;
      }
      //207EDD reads the child-result slot, separate from the visited word.
      //The builtin declares type3, so209AE0 preserves those raw result bits.
      finish(ScriptValue{n.type,f.values.empty()?0:f.values.back().bits,
                         f.values.empty()?std::string{}:f.values.back().text});
      continue;
    }
    if (f.name == "sleep_until") {
      if (f.children.empty() || f.children.size() > 3)
        throw FormatError("invalid sleep_until arity");
      if (f.wait_state == 0) {
        f.wait_start = tick;
        f.next = 1; // interval and timeout are evaluated once, before the test.
        f.wait_state = 1;
      }
      if (f.wait_state == 1) {
        if (f.next < f.children.size()) {
          thread.stack.push_back(Frame{f.children[f.next++]});
          continue;
        }
        if (!f.values.empty())
          f.wait_interval = wait_ticks(std::int16_t(convert(f.values[0], 7).bits));
        if (f.values.size() > 1) {
          const auto timeout = std::int32_t(convert(f.values[1], 8).bits);
          f.wait_timeout = timeout == -1 ? -1 : wait_ticks(timeout);
        }
        f.values.clear();
        f.wait_state = 2;
      }
      const auto deadline =
          f.wait_timeout == -1
              ? UINT64_MAX
              : f.wait_start +
                    std::uint64_t(std::max<std::int64_t>(0, f.wait_timeout));
      if (f.wait_state == 2 && tick < deadline) {
        // 0x2089B0 queues a nonliteral predicate through 0x2099F0 before
        // sleeping. Its host effects occur after the interval. Primitive
        // literals/globals are evaluated immediately by 0x2099F0 instead.
        f.wait_state = 3;
        thread.wake = std::min(
            tick + std::uint64_t(std::max<std::int64_t>(1, f.wait_interval)),
            deadline);
        const auto &predicate = node(f.children[0]);
        if (predicate.flags & 4) {
          f.values.push_back(read_global(predicate));
        } else if (predicate.flags & 1)
          f.values.push_back(literal(predicate));
        else
          thread.stack.push_back(Frame{f.children[0]});
        return;
      }
      if (tick >= deadline || (!f.values.empty() && f.values.back().truth())) {
        finish({});
        continue;
      }
      if (f.values.empty())
        throw FormatError("sleep_until lacks condition result");
      f.values.clear();
      f.wait_state = 2;
      continue;
    }
    if (f.name == "if") {
      if (f.children.size() < 2 || f.children.size() > 3)
        throw FormatError("invalid if arity");
      if (f.values.empty()) {
        thread.stack.push_back(Frame{f.children[0]});
        continue;
      }
      if (f.values.size() == 1) {
        const auto branch = f.values[0].truth() ? 1u : 2u;
        if (branch < f.children.size()) {
          thread.stack.push_back(Frame{f.children[branch]});
          continue;
        }
        finish({});
        continue;
      }
      finish(f.values.back());
      continue;
    }
    if ((f.name == "and" || f.name == "or") && !f.values.empty()) {
      const auto truth = f.values.back().truth();
      if ((f.name == "and" && !truth) || (f.name == "or" && truth)) {
        finish(ScriptValue::numeric(truth, 5));
        continue;
      }
    }
    if (f.next < f.children.size()) {
      const auto id = f.children[f.next++];
      if (thread.stack.size() >= 128)
        throw FormatError("script expression depth limit");
      thread.stack.push_back(Frame{id});
      continue;
    }
    const auto args = f.values;
    const auto name = f.name;
    auto arity = [&](std::size_t count) {
      if (args.size() != count)
        throw FormatError("wrong argument count for " + name);
    };
    ScriptValue value;
    if (name == "begin") {
      if (!args.empty())
        value = args.back();
    } else if (name == "and" || name == "or")
      value = ScriptValue::numeric(name == "and", 5);
    else if (name == "not") {
      arity(1);
      value = ScriptValue::numeric(!args[0].truth(), 5);
    } else if (name == "set") {
      arity(2);
      const auto &target = node(f.children[0]);
      if (!(target.flags & 4))
        throw FormatError("set target is not a supported global");
      value = write_global(target, args[1]);
    } else if (name == "+" || name == "-" || name == "*" || name == "/" ||
               name == "min" || name == "max") {
      if (args.empty())
        throw FormatError("empty arithmetic arguments");
      double result = args[0].number();
      for (std::size_t i = 1; i < args.size(); ++i) {
        const auto b = args[i].number();
        if (name == "+")
          result += b;
        else if (name == "-")
          result -= b;
        else if (name == "*")
          result *= b;
        else if (name == "/") {
          if (!b)
            throw FormatError("script divide by zero");
          result /= b;
        } else if (name == "min")
          result = std::min(result, b);
        else
          result = std::max(result, b);
      }
      value = ScriptValue::numeric(result, n.type);
    } else if (name == "=" || name == "!=" || name == ">" || name == "<" ||
               name == ">=" || name == "<=") {
      arity(2);
      const auto a = args[0].number(), b = args[1].number();
      const bool result = name == "="    ? a == b
                          : name == "!=" ? a != b
                          : name == ">"  ? a > b
                          : name == "<"  ? a < b
                          : name == ">=" ? a >= b
                                         : a <= b;
      value = ScriptValue::numeric(result, 5);
    } else if (name == "sleep") {
      if (args.empty() || args.size() > 2)
        throw FormatError("invalid sleep arity");
      // TU1.5 210BE9..210C22 ignores negative short delays and nonpositive
      // converted tick counts. 210C44..210C54 saves the first externally
      // interrupted wake, including dormant -2, until 211F7B restores it.
      const auto delay = std::int16_t(scalar_cast_bits(args[0].bits,args[0].type,7));
      const auto duration = delay < 0 ? 0 : wait_ticks(delay);
      if (args.size() == 2) {
        const auto target = std::int16_t(args[1].bits);
        //210BD5 treats script NONE (-1) as this thread, without lookup.
        if (duration > 0 && target == -1)
          thread.wake = tick + std::uint64_t(duration);
        else if (duration > 0)
          for (auto &t : threads_)
            if (target >= 0 && t.script == std::size_t(target)) {
              if (!t.stopped) {
                if (&t != &thread && !t.external_sleep)
                  t.external_sleep = AuthorityWakeState{t.dormant ? 0 : t.wake, t.dormant};
                t.wake = tick + std::uint64_t(duration);
                t.dormant = false;
              }
              break; //212130 returns the first existing matching thread.
            }
      } else if (duration > 0)
        thread.wake = tick + std::uint64_t(duration);
      finish({});
      if (thread.wake > tick)
        return;
      continue;
    } else if (name == "sleep_forever") {
      if (args.size() > 1)
        throw FormatError("invalid sleep_forever arity");
      if (args.empty())
        thread.dormant = true;
      else {
        const auto target = args[0].bits & 65535u;
        if (target >= program_.scripts.size())
          throw FormatError("sleep_forever target outside scripts");
        for (auto &t : threads_)
          if (t.script == target && !t.stopped)
            t.dormant = true;
      }
      finish({});
      if (thread.dormant)
        return;
      continue;
    } else if (name == "wake") {
      arity(1);
      const auto target = std::int16_t(args[0].bits);
      // TU1.5 210F53/211F40 never allocates an absent thread or restarts a
      // completed (-1) thread. The ordinal dispatcher reads a later target's
      // new wake immediately; already visited targets wait for the next pass.
      for (auto &t : threads_)
        if (target >= 0 && t.script == std::size_t(target)) {
          if (!t.stopped) {
            if (t.external_sleep) {
              t.wake = t.external_sleep->wake;
              t.dormant = t.external_sleep->dormant;
              t.external_sleep.reset();
            } else {
              t.wake = 0;
              t.dormant = false;
              //211FAB..211FFB aborts the current sleep_until frame, or its
              //queued predicate plus that immediate parent. Other enclosing
              //continuations retain their already evaluated arguments.
              auto count = t.stack.size();
              if (count && t.stack[count-1].entered && t.stack[count-1].name == "sleep_until")
                --count;
              else if (count > 1 && t.stack[count-2].entered && t.stack[count-2].name == "sleep_until")
                count -= 2;
              if (count != t.stack.size()) {
                const auto result = convert({}, node(t.stack[count].id).type);
                t.stack.resize(count);
                if (!t.stack.empty())t.stack.back().values.push_back(result);
              }
            }
          }
          break;
        }
    } else {
      current_thread_=&thread;current_actor_=thread.actor;
      try {
        const auto projected=mcc_script_projected_builtin_name(n.opcode,name);
        value=host_(projected?std::string(*projected):name,args);
      } catch(const std::exception &error) {
        current_thread_=nullptr;
        current_actor_.reset();
        throw FormatError("builtin '"+name+"': "+error.what());
      } catch(...) {
        current_thread_=nullptr;
        current_actor_.reset();
        throw;
      }
      current_thread_=nullptr;current_actor_.reset();
    }
    finish(std::move(value));
    if (suspended_ || thread.command_wait)
      return;
  }
  if (!thread.stopped && !thread.dormant && thread.wake <= tick)
    thread.wake = tick + 1; // cooperative budget, preserving the stack
}
void ScriptVm::tick(std::uint64_t tick) {
  NativeAuthorityBoundary::Call call(authority_);
  if (updating_)
    throw FormatError("recursive script dispatch");
  ++authority_.mutations;updating_ = true;
  struct DispatchExit {
    bool &updating;Thread *&thread;std::optional<std::uint32_t> &actor;
    ~DispatchExit(){updating=false;thread=nullptr;actor.reset();}
  } dispatch_exit{updating_,current_thread_,current_actor_};
  suspended_ = false;
  const auto count = threads_.size();
  for (std::size_t i = 0; i < count; ++i) {
    auto &t = threads_[i];
    if (!t.stopped && !t.dormant && t.wake <= tick) {
      try {
        if(t.command_wait) {
          if(!t.actor || !command_ready_)throw FormatError("Command wait lost its actor completion owner");
          if(!command_ready_(*t.actor))continue;
          t.command_wait=false;
        }
        update(t, tick);
      } catch (const std::exception &e) {
        diagnostics_.push_back(program_.scripts[t.script].name + ": " +
                               e.what());
        t.stopped = true;
        t.faulted = true;
      }
    }
    if (suspended_)
      break;
  }
  updating_ = false;
}
} // namespace halo2
