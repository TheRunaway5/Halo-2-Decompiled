#include "halo2/physics_platform_locks.hpp"
#include "halo2/native_synchronization.hpp"
#include <array>
#include <future>
#include <iostream>

using namespace halo2;
namespace {
const char *phase="setup";
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
template<class F>void rejects(F &&f){bool stopped=false;try{f();}catch(const std::exception &){stopped=true;}check(stopped,"Unsafe lock checkpoint accepted");}
}
int main() try {
    SourceRuntimeMemory source,target,unrelated;
    for(auto *bank:{&source,&target,&unrelated})for(auto address:{0x1000u,0x2000u})
        bank->mount(address,std::vector<std::byte>(0x1000),SourceRuntimeMemory::Access::ReadWrite,"declared critical-section storage");
    PhysicsPlatformLocks original,resumed;
    original.initialize(source,0x1ff0);
    resumed.initialize(target,0x1100);resumed.initialize(unrelated,0x1200);
    PhysicsPlatformLockServices one{.current_thread=[] {return 1u;}},two{.current_thread=[] {return 77u;}};
    original.enter(source,0x1ff0,one);original.enter(source,0x1ff0,one);
    rejects([&]{original.ownership_checkpoint(source);});
    original.leave(source,0x1ff0,one);rejects([&]{original.ownership_checkpoint(source);});
    original.leave(source,0x1ff0,one);
    phase="capture";
    const auto saved=original.ownership_checkpoint(source),old=resumed.ownership_checkpoint(target);
    const auto other=resumed.ownership_checkpoint(unrelated);const auto bank=source.checkpoint(),before=target.checkpoint();
    {
        phase="cancellation";
        auto stage=resumed.prepare_ownership(target,saved,bank);
        rejects([&]{resumed.ownership_checkpoint(target);});
        rejects([&]{resumed.initialize(target,0x1100);});
        rejects([&]{resumed.prepare_ownership(target,saved,bank);});
        check(!original.commit_ownership(std::move(stage)),"Foreign lock owner accepted a prepared registry");
    }
    check(target.checkpoint()==before && resumed.ownership_checkpoint(target)==old,"Cancelled lock staging changed state");
    auto bad=saved;bad.version=2;rejects([&]{resumed.prepare_ownership(target,bad,bank);});
    bad=saved;bad.critical_sections.push_back(bad.critical_sections.front());rejects([&]{resumed.prepare_ownership(target,bad,bank);});
    bad=saved;bad.critical_sections={0xfffffff0u};rejects([&]{resumed.prepare_ownership(target,bad,bank);});
    original.enter(source,0x1ff0,one);
    rejects([&]{resumed.prepare_ownership(target,saved,source.checkpoint());});
    original.leave(source,0x1ff0,one);
    resumed.enter(target,0x1100,two);rejects([&]{resumed.prepare_ownership(target,saved,bank);});resumed.leave(target,0x1100,two);
    phase="publication";
    auto stage=resumed.prepare_ownership(target,saved,bank);target.restore(bank);
    check(resumed.commit_ownership(std::move(stage)) && resumed.ownership_checkpoint(target)==saved &&
        resumed.ownership_checkpoint(unrelated)==other,"Lock import lost source/local-bank ownership");
    original.enter(source,0x1ff0,one);original.leave(source,0x1ff0,one);
    resumed.enter(target,0x1ff0,two);resumed.leave(target,0x1ff0,two);
    check(target.checkpoint()==source.checkpoint(),"Rebound thread equality tokens changed future released source state");
    phase="contention";
    NativeSynchronization native({0x40000001,0x40000010});const auto services=native.services();
    const auto semaphore=services.create_semaphore(0,1).handle;
    std::promise<void> entering;auto entered=entering.get_future();
    auto contended=two;
    contended.wait=[&](auto){entering.set_value();check(services.wait(semaphore,0xffffffffu).state==NativeSynchronizationWaitState::Acquired,"Real lock wait failed");};
    one.wake=[&](auto){check(!services.release_semaphore(semaphore,1).error,"Real lock wake failed");};
    resumed.enter(target,0x1ff0,one);
    auto waiter=std::async(std::launch::async,[&]{resumed.enter(target,0x1ff0,contended);resumed.leave(target,0x1ff0,contended);});
    entered.wait();rejects([&]{resumed.ownership_checkpoint(target);});
    rejects([&]{resumed.prepare_ownership(target,saved,bank);});
    resumed.leave(target,0x1ff0,one);waiter.get();
    check(resumed.ownership_checkpoint(target)==saved && target.checkpoint()==source.checkpoint(),"Contended lock did not finish after rejected snapshot");
    std::cout<<"PASS critical-section continuation, held/recursive/contended guards, atomic rejection/cancellation, cross-region records and local thread-token rebind\n";
    return 0;
} catch(const std::exception &e){std::cerr<<phase<<": "<<e.what()<<'\n';return 1;}
