"""Strict private Arena producer candidate; never writes production sources."""
import hashlib

SERVICES_PIN='bb3dca1084ed413a3dad7cf741b47ad04cea02bc7da1a44d167a8c64ec05b34e'
HEADER_PIN='7021af839ce2b74b4ae64382909398b6b8c7d5cefc05a95a59590a073dd0f665'
RUN_PIN='b374488b58b84a9f523f855f47449502eca8b178233c2fc4e3894c600d0a1b8e'
INCLUDES=('#include "physics/runtime/native_tu15_arena_schedule_policy.hpp"\n'
          '#include "physics/runtime/native_tu15_arena_publication.hpp"\n'
          '#include "physics/runtime/native_tu15_required_owner_policy.hpp"\n'
          '#include <exception>\n')
OLD_CALL='''  source_run_physics_game_arena_runtime(impl_->shared,entry,abi,SourcePhysicsContactImage::TitleUpdate15,
      impl_->config.floating_point,impl_->services(),impl_->observe);
'''
PUBLISHED_CALL='''  const auto joined=impl_->services();
  native_arena_publication_detail::Borrow published{impl_->shared,joined,impl_.get(),&Impl::RequiredOwner::matches_publication};
  source_run_physics_game_arena_runtime(impl_->shared,entry,abi,SourcePhysicsContactImage::TitleUpdate15,
      impl_->config.floating_point,joined,impl_->observe);
'''
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def services(source):
    start=source.index('  SourcePhysicsWorldRuntimeServices services(')
    end=source.index('  bool platform(',start)
    return source[start:end]
def run(source):
    start=source.index('void SourcePhysicsGameArenaNative::run(')
    end=source.index('void SourcePhysicsGameArenaNative::stop(',start)
    return source[start:end]
def prototype_header(source):
    if digest(source)!=HEADER_PIN:raise ValueError('Complete actual Arena public declaration drift')
    return source
def prototype(source):
    old=services(source)
    if digest(old)!=SERVICES_PIN:raise ValueError('Complete original Arena services producer drift')
    if digest(run(source))!=RUN_PIN or source.count(OLD_CALL)!=1:raise ValueError('Complete original synchronous Arena call scope drift')
    opening='    joined.required_owner = [this,stop](auto &m,const auto &call,auto &abi,const auto &o) {\n'
    closing='    };\n    return joined;\n  }\n'
    if old.count(opening)!=1 or not old.endswith(closing):raise ValueError('Exact Arena required-body boundary differs')
    body=old.split(opening,1)[1][:-len(closing)]
    candidate='''  struct RequiredOwner {
    Impl *const owner;
    const std::stop_token stop;
    void operator()(SourcePhysicsContactMemory &m,const SourcePhysicsContactRuntimeCall &call,
                    SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsContactRecordObserver &o)const {
      owner->required_entry(m,call,abi,o,stop);
    }
    static bool matches(const SourcePhysicsWorldRuntimeServices::RequiredOwner &callback,
                        const void *expected)noexcept {
      const auto *actual=callback.target<RequiredOwner>();
      return actual && actual->owner==expected && actual->owner &&
          actual->owner->preserves_parent_instruction_policy();
    }
    static bool matches_publication(const SourcePhysicsWorldRuntimeServices &services,
                                    const void *expected)noexcept {
      const auto *actual=services.required_owner.target<RequiredOwner>();
      return actual && actual->owner==expected && actual->owner &&
          !services.instruction && !actual->owner->lower.instruction &&
          native_required_owner_policy_detail::preserves_instruction_policy(services);
    }
  };
  bool preserves_parent_instruction_policy()const noexcept {
    return parent_executor==std::this_thread::get_id() &&
        native_arena_schedule_detail::preserves_parent_policy(config.schedule) &&
        native_required_owner_policy_detail::preserves_instruction_policy(lower);
  }
  void required_entry(SourcePhysicsContactMemory &m,const SourcePhysicsContactRuntimeCall &call,
                      SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsContactRecordObserver &o,
                      std::stop_token stop) {
'''+body+'''  }
  SourcePhysicsWorldRuntimeServices services(std::stop_token stop = {}) {
    auto joined = lower;
    joined.required_owner=RequiredOwner{this,stop};
    joined.required_owner_policy={&joined.required_owner.target_type(),parent_executor,this,&RequiredOwner::matches};
    return joined;
  }

'''
    anchor='  SourcePhysicsGameArenaConfiguration config;\n'
    if source.count(anchor)!=1 or INCLUDES in source:raise ValueError('Arena producer field/include boundary differs')
    result=source.replace(old,candidate,1).replace(anchor,'  const std::thread::id parent_executor=std::this_thread::get_id();\n'+anchor,1).replace(OLD_CALL,PUBLISHED_CALL,1)
    return INCLUDES+result
