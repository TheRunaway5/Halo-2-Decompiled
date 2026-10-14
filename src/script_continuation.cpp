#include "halo2/scripts.hpp"
#include "world_continuation_bindings.hpp"
#include <algorithm>
#include <set>

namespace halo2 {
std::array<bool,7> ScriptVm::authority_services()const {
    return {bool(host_),bool(object_resolver_),bool(external_globals_),bool(object_lists_.allocate),
        bool(object_lists_.append),bool(random_choice_),bool(command_ready_)};
}
void ScriptVm::validate_authority(const AuthorityCheckpoint &saved)const {
    if(saved.version!=1 || saved.program_binding!=world_continuation::script_program(program_) ||
        saved.local_services!=authority_services() || saved.globals.size()!=program_.globals.size() ||
        saved.threads.size()>320 || saved.actor)
        throw FormatError("Script continuation has incompatible program, services or dispatch state");
    for(unsigned i=0;i<saved.globals.size();++i)if(program_.globals[i].type!=3 && saved.globals[i].type!=program_.globals[i].type)
        throw FormatError("Script continuation changes an authored global type");
    for(const auto &thread:saved.threads) {
        if(thread.script>=program_.scripts.size() || thread.stack.size()>129 ||
            (thread.faulted && !thread.stopped) ||
            ((program_.scripts[thread.script].type==5)!=thread.actor.has_value()) ||
            (thread.actor && *thread.actor==null_datum) || (thread.command_wait && (!thread.actor || !command_ready_)))
            throw FormatError("Script continuation has an invalid thread or command actor");
        // Handled script faults retain their original partial stack and report.
        // They are stopped and cannot be awakened; do not erase that evidence
        // or require the malformed node that caused the fault to become valid.
        if(thread.faulted)continue;
        for(const auto &f:thread.stack) {
            const auto &n=node(f.id);
            if(f.children.size()>1025 || f.values.size()>std::max<std::size_t>(1,f.children.size()) ||
                f.next>f.children.size() || f.wait_state>3)
                throw FormatError("Script continuation has an invalid expression frontier");
            if(!f.entered) {
                if(!f.children.empty() || !f.values.empty() || !f.name.empty() || f.next || f.returned_call || f.wait_state || f.random_visited)
                    throw FormatError("Script continuation changes an unentered expression");
                continue;
            }
            if(n.flags&5)throw FormatError("Script continuation retains a completed scalar expression");
            const auto &fn=node(n.value);
            if(f.name!=string(fn.string_offset) || f.returned_call!=bool(n.flags&2))
                throw FormatError("Script continuation changes its compiled call binding");
            std::vector<std::uint32_t> children;std::set<std::uint32_t> seen;
            for(auto id=fn.next;id!=null_datum;id=node(id).next) {
                if(children.size()>1024 || !seen.insert(id).second)throw FormatError("Script continuation has cyclic compiled arguments");
                children.push_back(id);
            }
            if(children!=f.children)throw FormatError("Script continuation changes its compiled arguments");
            if(f.name=="begin_random" && (f.children.size()>32 ||
                (f.children.size()<32 && (f.random_visited>>f.children.size()))))
                throw FormatError("Script continuation has an invalid random-visited frontier");
        }
    }
}
struct ScriptVm::PreparedAuthority::Impl {
    ScriptVm *owner{};bool *staging{};AuthorityCheckpoint saved;
    ~Impl(){if(staging)*staging=false;}
};
ScriptVm::PreparedAuthority::PreparedAuthority()=default;
ScriptVm::PreparedAuthority::~PreparedAuthority()=default;
ScriptVm::PreparedAuthority::PreparedAuthority(PreparedAuthority &&) noexcept=default;
ScriptVm::PreparedAuthority &ScriptVm::PreparedAuthority::operator=(PreparedAuthority &&) noexcept=default;
bool ScriptVm::PreparedAuthority::for_owner(const ScriptVm &owner)const noexcept{return impl_ && impl_->owner==&owner;}
ScriptVm::AuthorityCheckpoint ScriptVm::authority_checkpoint()const {
    authority_.capture();if(updating_ || current_thread_)throw FormatError("Script continuation requested during dispatch");
    AuthorityCheckpoint saved;saved.program_binding=world_continuation::script_program(program_);saved.local_services=authority_services();
    saved.threads=threads_;saved.globals=globals_;saved.diagnostics=diagnostics_;saved.suspended=suspended_;saved.actor=current_actor_;
    validate_authority(saved);return saved;
}
ScriptVm::PreparedAuthority ScriptVm::prepare_authority(const AuthorityCheckpoint &saved) {
    authority_.prepare();if(updating_ || current_thread_)throw FormatError("Script continuation import requested during dispatch");
    PreparedAuthority result;result.impl_=std::make_unique<PreparedAuthority::Impl>();
    result.impl_->owner=this;result.impl_->staging=&authority_.staging;authority_.staging=true;
    validate_authority(saved);result.impl_->saved=saved;return result;
}
bool ScriptVm::commit_authority(PreparedAuthority &&prepared) noexcept {
    if(!prepared.for_owner(*this) || !authority_.staging || authority_.depth || updating_ || current_thread_)return false;
    auto &s=prepared.impl_->saved;threads_.swap(s.threads);globals_.swap(s.globals);diagnostics_.swap(s.diagnostics);
    suspended_=s.suspended;current_actor_=s.actor;authority_.fault=false;prepared.impl_.reset();return true;
}
}
