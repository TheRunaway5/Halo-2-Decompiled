#include "physics/runtime/native_tu15_arena_schedule_policy.hpp"
#include <iostream>
#include <type_traits>

using namespace halo2;
namespace {
void check(bool value,const char*message){if(!value)throw std::runtime_error(message);}
}
int main()try {
    using namespace native_arena_schedule_detail;
    static_assert(!std::is_copy_assignable_v<WorkerIdleBeforeWait>);
    unsigned checks{};
    SourcePhysicsGameArenaSchedule empty;
    check(preserves_parent_policy(empty),"Empty audited schedule refused");++checks;
    const auto idle=std::make_shared<WorkerIdleBarrier>();
    SourcePhysicsGameArenaSchedule owned{WorkerIdleBeforeWait{idle},{}};
    check(preserves_parent_policy(owned),"Exact owned schedule refused");++checks;
    auto copy=owned;auto moved=std::move(copy);
    check(preserves_parent_policy(moved),"Immutable owned schedule copy/move lost its contract");++checks;
    SourcePhysicsGameArenaSchedule wrapped{[original=owned.before_worker_wait](auto handle,auto stop){original(handle,stop);},{}};
    check(!preserves_parent_policy(wrapped),"Arbitrary schedule wrapper gained the contract");++checks;
    unsigned mutations{};owned.before_worker_wait=[&](auto,auto){++mutations;};
    check(!preserves_parent_policy(owned)&&mutations==0,"Rebound schedule accepted or invoked during check");++checks;
    check(preserves_parent_policy(moved),"Rebinding another copy changed the immutable capture");++checks;
    owned=moved;owned.before_main_yield=[]{};
    check(!preserves_parent_policy(owned),"Arbitrary main-yield callback accepted");++checks;
    owned.before_main_yield={};check(preserves_parent_policy(owned),"Cleared main-yield failed to restore the exact contract");++checks;
    bool foreign_accepted=true;
    std::jthread worker([&]{foreign_accepted=preserves_parent_policy(owned);for(unsigned n=0;n<5;++n)owned.before_worker_wait(0xf3000003u,{});});
    idle->finish_startup();worker.join();
    check(!foreign_accepted,"Foreign parent executor accepted");++checks;
    {std::lock_guard lock(idle->mutex);check(idle->waits==5,"Named schedule failed actual worker barrier behavior");++checks;}
    owned.before_worker_wait(0xf3000010u,{});
    {std::lock_guard lock(idle->mutex);check(idle->waits==5,"Foreign semaphore advanced the idle barrier");++checks;}
    bool refused_null{};try{(void)WorkerIdleBeforeWait{nullptr};}catch(const std::invalid_argument&){refused_null=true;}
    check(refused_null,"Absent barrier capture accepted");++checks;
    std::cout<<"{\"passed\":true,\"checks\":"<<checks<<",\"private_unbound\":true,\"production_policy_issued\":false}\n";
    return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
