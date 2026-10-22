#include "platform/native_startup_resources.hpp"
#include <array>
#include <condition_variable>
#include <cstdlib>
#include <fcntl.h>
#include <future>
#include <iostream>
#include <mutex>
#include <thread>
#include <unistd.h>

namespace host=halo2::native_startup_resources;
namespace {
unsigned checks{};
void check(bool value,const char* why) {++checks;if(!value)throw std::runtime_error(why);}
struct File {
    int descriptor{-1};
    explicit File(const std::string& path) {
        auto name=path;
        descriptor=::mkstemp(name.data());
        if(descriptor<0)throw std::system_error(errno,std::generic_category(),"Create owned storage fixture");
        if(::unlink(name.c_str())) {
            const auto error=errno;::close(descriptor);descriptor=-1;
            throw std::system_error(error,std::generic_category(),"Unlink owned storage fixture");
        }
    }
    ~File(){if(descriptor>=0)::close(descriptor);}
};
}
int main()try {
    const auto* scratch=std::getenv("HALO2_TEST_SCRATCH");
    check(scratch&&*scratch,"Storage host test requires owned scratch");
    const auto main_worker=host::current_worker();
    check(bool(main_worker),"Main thread identity is missing");
    bool rejected{};
    try {(void)host::raise_worker_priority({});}catch(const std::invalid_argument&){rejected=true;}
    check(rejected,"Empty worker identity reached a native priority operation");
    std::promise<host::Worker> ready;auto identity=ready.get_future();
    std::mutex mutex;std::condition_variable_any changed;
    std::jthread worker([&](std::stop_token stop){
        try {ready.set_value(host::current_worker());}
        catch(...) {ready.set_exception(std::current_exception());return;}
        std::unique_lock lock(mutex);changed.wait(lock,stop,[]{return false;});
    });
    const auto target=identity.get();
    check(bool(target)&&target.id!=main_worker.id,"Worker identity is not the actual separate thread");
#if defined(__APPLE__)
    check(!::pthread_equal(main_worker.thread,target.thread),"Worker pthread points at its parent");
    int main_policy{},policy{};sched_param main_before{},before{},after{},main_after{};
    check(!::pthread_getschedparam(main_worker.thread,&main_policy,&main_before),"Cannot observe main priority");
    check(!::pthread_getschedparam(target.thread,&policy,&before),"Cannot observe worker priority");
    const bool priority=host::raise_worker_priority(target);
    int after_policy{};
    check(!::pthread_getschedparam(target.thread,&after_policy,&after),"Cannot recheck worker priority");
    check(after_policy==policy,"Host policy changed the scheduling class");
    check(after.sched_priority==before.sched_priority+(priority?1:0),"Priority BOOL disagrees with the actual worker");
    int after_main_policy{};
    check(!::pthread_getschedparam(main_worker.thread,&after_main_policy,&main_after),"Cannot recheck main priority");
    check(after_main_policy==main_policy&&main_before.sched_priority==main_after.sched_priority,
        "Worker priority operation changed the parent thread");
    std::cout<<"Darwin worker priority: before="<<before.sched_priority<<", after="<<after.sched_priority
             <<", success="<<priority<<'\n';
#else
    errno=0;const auto main_before=::getpriority(PRIO_PROCESS,0);
    check(errno==0,"Cannot observe native main nice value");
    errno=0;const auto before=::getpriority(PRIO_PROCESS,id_t(target.id));
    check(errno==0,"Cannot observe native worker nice value");
    const bool priority=host::raise_worker_priority(target);
    errno=0;const auto after=::getpriority(PRIO_PROCESS,id_t(target.id));
    check(errno==0 && after==(priority?-1:before),"Linux priority BOOL differs from the actual worker nice value");
    errno=0;check(::getpriority(PRIO_PROCESS,0)==main_before&&errno==0,"Worker priority changed the main nice value");
    std::cout<<"Linux worker priority: before="<<before<<", after="<<after<<", success="<<priority<<'\n';
#endif
    worker.request_stop();worker.join();
    File file(std::string(scratch)+"/startup-storage-XXXXXX");
    const std::array<unsigned char,8> bytes{0,1,2,3,254,255,7,8};
    check(::write(file.descriptor,bytes.data(),bytes.size())==ssize_t(bytes.size()),"Storage fixture write failed");
    check(host::synchronize_file(file.descriptor)==0,"Native storage synchronization failed");
    struct stat before_file{},same{},later{};
    check(::fstat(file.descriptor,&before_file)==0 && ::fstat(file.descriptor,&same)==0,"Cannot stat owned storage file");
    check(host::same_file_extent_and_time(before_file,same),"Unchanged storage metadata differs");
    const std::array<timespec,2> stamp{{{0,UTIME_OMIT},{100,1}}};
    check(::futimens(file.descriptor,stamp.data())==0 && ::fstat(file.descriptor,&later)==0,"Cannot change owned file timestamp");
    check(!host::same_file_extent_and_time(before_file,later),"Changed mtime was ignored");
    const auto first=later;
    auto ns=stamp;ns[1].tv_nsec=2;
    check(::futimens(file.descriptor,ns.data())==0 && ::fstat(file.descriptor,&later)==0,"Cannot change nanosecond timestamp");
    check(!host::same_file_extent_and_time(first,later),"Nanosecond mtime was ignored");
    const auto extent=later;
    check(::ftruncate(file.descriptor,4096)==0 && ::fstat(file.descriptor,&later)==0,"Cannot resize owned file");
    check(!host::same_file_extent_and_time(extent,later),"Changed extent was ignored");
    check(host::synchronize_file(-1)!=0 && errno==EBADF,"Invalid storage sync returned success");
    std::array<unsigned char,8> read{};
    check(::pread(file.descriptor,read.data(),read.size(),0)==ssize_t(read.size())&&read==bytes,
        "Synchronization/metadata operations altered stored bytes");
    std::cout<<"PASS "<<checks<<" native startup host controls: real worker identity/priority, parent preservation, file timestamps/extents/sync\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
