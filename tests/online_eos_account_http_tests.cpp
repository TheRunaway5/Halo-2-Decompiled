// Real Session/HTTP worker ownership. The fixture never supplies a valid
// external credential, so no account login or cloud user/session write occurs.
#include "halo2/online_match_session.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
#include "eos_sdk_worker_observer.hpp"
#include <eos_connect.h>
#include <eos_p2p.h>
#include <eos_sdk.h>
#include <nlohmann/json.hpp>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

using namespace halo2;
using namespace std::chrono_literals;
using Json = nlohmann::json;
namespace {
using namespace halo2::eos_worker_fixture;
class FixtureError : public std::runtime_error {public:using std::runtime_error::runtime_error;};
void check(bool value, const char *why) {if(!value)throw FixtureError(why);}
// These Linux/GNU-only ABI observers share numeric evidence, never SDK state.
// A local mutex plus flock serialize tiny POD updates across threads/processes;
// no SDK call runs while either observer lock is held.
class Observer {
public:
    Observer() {
      try {
        const auto *scratch=std::getenv("HALO2_TEST_SCRATCH");
        check(scratch&&*scratch,"SDK fixture requires owned scratch");
        const auto root=std::filesystem::canonical(scratch);
        check(root.filename().string().starts_with("halo2-test-"),"SDK fixture scratch is not owned");
        check(!std::getenv(observer_environment)&&!std::getenv(worker_environment)&&!std::getenv("LD_PRELOAD"),
            "Unexpected inherited SDK fixture observer");
        const auto worker=std::filesystem::canonical("/proc/self/exe").parent_path()/"eos_sdk_worker";
        const std::filesystem::path library=HALO2_EOS_WORKER_OBSERVER;
        check(std::filesystem::is_regular_file(std::filesystem::symlink_status(worker))&&
            std::filesystem::canonical(worker)==worker&&::access(worker.c_str(),X_OK)==0&&
            library.is_absolute()&&std::filesystem::is_regular_file(std::filesystem::symlink_status(library))&&
            std::filesystem::canonical(library)==library&&library.string().find_first_of(" :\t\r\n")==std::string::npos,
            "Fixed SDK worker or owned observer library is missing");
        path_=root/"eos-account-observer-XXXXXX";
        auto name=path_.string();
        fd_=::mkstemp(name.data());path_=name;created_=fd_>=0;
        check(fd_>=0&&::fchmod(fd_,0600)==0&&::fcntl(fd_,F_SETFD,FD_CLOEXEC)==0&&
            ::ftruncate(fd_,sizeof(ObserverData))==0,"SDK observer creation failed");
        struct stat metadata{};
        check(fd_>=0&&::fstat(fd_,&metadata)==0&&S_ISREG(metadata.st_mode)&&
            metadata.st_uid==::getuid()&&(metadata.st_mode&0777)==0600&&metadata.st_nlink==1&&
            metadata.st_size==static_cast<off_t>(sizeof(ObserverData)),"SDK observer metadata is invalid");
        auto *mapped=::mmap(nullptr,sizeof(ObserverData),PROT_READ|PROT_WRITE,MAP_SHARED,fd_,0);
        check(mapped!=MAP_FAILED,"SDK observer mapping failed");
        data_=static_cast<ObserverData *>(mapped);
        {
            Lock lock(fd_,local_);
            *data_=ObserverData{};data_->magic=observer_magic;data_->parent_pid=::getpid();
        }
        check(::setenv(observer_environment,path_.c_str(),1)==0,"SDK observer inheritance failed");
        inherited_=true;
        check(::setenv(worker_environment,worker.c_str(),1)==0&&::setenv("LD_PRELOAD",library.c_str(),1)==0,
            "SDK child observer inheritance failed");
        active=this;
      } catch(...) {close();throw;}
    }
    ~Observer(){if(active==this)active=nullptr;close();}
    Observer(const Observer &)=delete;
    Observer &operator=(const Observer &)=delete;
    ObserverData snapshot() const {Lock lock(fd_,local_);check(data_->magic==observer_magic,"SDK observer became invalid");return *data_;}
    static void record(SdkCall call) noexcept {
        try {
            check(active,"SDK entry lacks a genuine fixture observer");
            Lock lock(active->fd_,active->local_);auto &data=*active->data_;
            check(data.magic==observer_magic,"SDK observer became invalid");
            ++data.calls[call];
            if(static_cast<std::uint64_t>(::getpid())==data.parent_pid)++data.parent_calls;
            if(static_cast<std::uint64_t>(::getpid())!=data.worker_pid)++data.foreign_calls;
            if(::syscall(SYS_gettid)!=::getpid())++data.off_main_calls;
        } catch(...) {std::_Exit(71);}
    }
private:
    class Lock {
    public:
        Lock(int fd,std::mutex &local):local_(local),fd_(fd){int result;do{result=::flock(fd_,LOCK_EX);}while(result<0&&errno==EINTR);
            check(result==0,"SDK observer lock failed");}
        ~Lock(){::flock(fd_,LOCK_UN);}
    private:std::unique_lock<std::mutex> local_;int fd_;
    };
    void close() noexcept {
        if(data_){::munmap(data_,sizeof(ObserverData));data_=nullptr;}
        if(fd_>=0){::close(fd_);fd_=-1;}
        if(created_){::unlink(path_.c_str());created_=false;}
        if(inherited_){::unsetenv(observer_environment);::unsetenv(worker_environment);::unsetenv("LD_PRELOAD");inherited_=false;}
    }
    static Observer *active;
    mutable std::mutex local_;
    int fd_{-1};ObserverData *data_{};std::filesystem::path path_;bool created_{},inherited_{};
};
Observer *Observer::active{};
ObserverData actual_sdk_observation(const Observer &observer,bool allow_unstarted=false) {
    auto data=observer.snapshot();
    if(allow_unstarted&&data.worker_entries==0) {
        // A 401 can precede the peer worker's first lazy SDK tick. This is a
        // genuine zero-entry observation, never a manufactured child proof.
        check(data.worker_pid==0&&data.worker_main==0&&data.worker_image_verified==0&&data.worker_finished==0&&data.worker_result==0&&
            data.parent_calls==0&&data.foreign_calls==0&&data.off_main_calls==0&&
            data.calls==std::array<std::uint64_t,sdk_call_count>{},"Pre-SDK denial has inconsistent observer evidence");
        return data;
    }
    check(data.worker_entries==1&&data.worker_pid!=data.parent_pid&&data.worker_main==1&&data.worker_image_verified==1,
        "SDK child process-main observation is missing");
    check(data.parent_calls==0&&data.foreign_calls==0&&data.off_main_calls==0,
        "A real SDK entry escaped the registered child's process main");
    return data;
}
template<class F> void wait(F predicate, const char *why) {
    const auto until=std::chrono::steady_clock::now()+10s;
    while(!predicate()) {check(std::chrono::steady_clock::now()<until,why);std::this_thread::sleep_for(2ms);}
}
Json observed(const Observer &observer,bool allow_unstarted=false) {
    const auto data=actual_sdk_observation(observer,allow_unstarted);
    return {{"device_calls",data.calls[device_call]},{"login_calls",data.calls[login_call]},
        {"create_user_calls",data.calls[create_call]},{"socket_calls",data.calls[socket_call]},
        {"sdk_ticks",data.calls[tick_call]},{"sdk_worker_entries",data.worker_entries},
        {"sdk_worker_process_main",data.worker_main==1},{"sdk_parent_calls",data.parent_calls},
        {"sdk_foreign_calls",data.foreign_calls},{"sdk_off_main_calls",data.off_main_calls},
        {"sdk_worker_executable_verified",data.worker_image_verified==1},
        {"sdk_worker_exit_observed",data.worker_finished==1},{"observed_sdk_api_count",sdk_call_count}};
}
void no_unacknowledged_sdk_admission(const Observer &observer,bool allow_unstarted=false) {
    const auto data=actual_sdk_observation(observer,allow_unstarted);
    check(data.calls[device_call]==0,"Account transport called anonymous DeviceID creation");
    check(data.calls[login_call]==0,"Denied/unavailable issuer unexpectedly reached SDK Connect login");
    check(data.calls[create_call]==0,"Denied/unavailable issuer unexpectedly created a cloud user");
    check(data.calls[socket_call]==0,"Unacknowledged identity installed a P2P socket");
}
}

// Parent link-time observers remain to prove that no direct SDK call escaped
// to this process. The separate worker DSO forwards the same five real APIs.
// Tick progress detects an SDK/transport lock held across HTTP.
extern "C" {
void EOS_CALL __real_EOS_Connect_CreateDeviceId(EOS_HConnect, const EOS_Connect_CreateDeviceIdOptions *, void *, EOS_Connect_OnCreateDeviceIdCallback);
void EOS_CALL __wrap_EOS_Connect_CreateDeviceId(EOS_HConnect handle, const EOS_Connect_CreateDeviceIdOptions *options, void *client, EOS_Connect_OnCreateDeviceIdCallback callback) {
    Observer::record(device_call);__real_EOS_Connect_CreateDeviceId(handle,options,client,callback);
}
void EOS_CALL __real_EOS_Connect_Login(EOS_HConnect, const EOS_Connect_LoginOptions *, void *, EOS_Connect_OnLoginCallback);
void EOS_CALL __wrap_EOS_Connect_Login(EOS_HConnect handle, const EOS_Connect_LoginOptions *options, void *client, EOS_Connect_OnLoginCallback callback) {
    Observer::record(login_call);__real_EOS_Connect_Login(handle,options,client,callback);
}
void EOS_CALL __real_EOS_Connect_CreateUser(EOS_HConnect, const EOS_Connect_CreateUserOptions *, void *, EOS_Connect_OnCreateUserCallback);
void EOS_CALL __wrap_EOS_Connect_CreateUser(EOS_HConnect handle, const EOS_Connect_CreateUserOptions *options, void *client, EOS_Connect_OnCreateUserCallback callback) {
    Observer::record(create_call);__real_EOS_Connect_CreateUser(handle,options,client,callback);
}
EOS_NotificationId EOS_CALL __real_EOS_P2P_AddNotifyPeerConnectionRequest(EOS_HP2P, const EOS_P2P_AddNotifyPeerConnectionRequestOptions *, void *, EOS_P2P_OnIncomingConnectionRequestCallback);
EOS_NotificationId EOS_CALL __wrap_EOS_P2P_AddNotifyPeerConnectionRequest(EOS_HP2P handle, const EOS_P2P_AddNotifyPeerConnectionRequestOptions *options, void *client, EOS_P2P_OnIncomingConnectionRequestCallback callback) {
    Observer::record(socket_call);return __real_EOS_P2P_AddNotifyPeerConnectionRequest(handle,options,client,callback);
}
void EOS_CALL __real_EOS_Platform_Tick(EOS_HPlatform);
void EOS_CALL __wrap_EOS_Platform_Tick(EOS_HPlatform platform) {
    Observer::record(tick_call);__real_EOS_Platform_Tick(platform);
}
}

int main(int argc, char **argv) try {
    check(argc==4,"Supply ORIGIN PRIVATE_FIXTURE_FILE CASE");
    Observer observer;
    struct Shutdown {~Shutdown(){online_protocol_bridge::eos_process_shutdown();}} shutdown;
    online_protocol_bridge::eos_sdk_register_host();
    const std::string mode=argv[3];
    check(mode=="denied"||mode=="unavailable"||mode=="stalled","Unknown account HTTP fixture case");
    Json descriptor;{std::ifstream input(argv[2]);input>>descriptor;}
    std::vector<OnlineAdmission> roster(2);
    for(unsigned slot=0;slot<roster.size();++slot) {
        auto &player=roster[slot];player.slot=slot;player.name="HTTP Fixture";
        player.team=0;player.peer=slot;player.identity.front()=std::byte(slot+1);
    }
    // A real two-seat/one-physical-owner roster makes complete_measurement
    // vacuous. Without the auth gate it would announce peer_ready immediately,
    // so the pending test does not need to fake a clock or wait 45 seconds.
    if(mode!="denied") {roster[1].peer=0;roster[1].guest=true;}
    OnlineEosOptions eos;
    eos.socket=descriptor.at("socket");eos.account_login=true;eos.force_relay=true;
    if(mode=="denied")eos.peers.push_back({1,descriptor.at("peer_key")});
    OnlineMatchTransportOptions options;options.direct_udp=true;options.eos=std::move(eos);
    auto session=std::make_unique<OnlineMatchSession>(argv[1],descriptor.at("room"),descriptor.at("bearer"),
        0,roster,true,false,false,options);
    std::cout<<"H2EOSAUTHFIXTURE "<<Json{{"constructed",true}}.dump()<<std::endl;
    std::string line;
    while(std::getline(std::cin,line)) {
        const auto command=Json::parse(line).at("action").get<std::string>();
        if(command=="snapshot") {
            wait([&]{return observer.snapshot().calls[tick_call]>0&&session->master_connected();},"Native SDK/control workers did not become ready");
            no_unacknowledged_sdk_admission(observer);
            check(!session->source_players()&&session->validated_authority().empty(),"HTTP setup manufactured a prepared native authority");
            auto result=observed(observer);
            result["failed"]=session->phase()==OnlineMatchSession::Phase::Failed;
            result["connecting"]=session->phase()==OnlineMatchSession::Phase::Connecting;
            result["master_connected"]=session->master_connected();
            result["host_candidates"]=session->host_candidates().size();
            std::cout<<"H2EOSAUTHFIXTURE "<<result.dump()<<std::endl;
        } else if(command=="denial") {
            wait([&]{return session->phase()==OnlineMatchSession::Phase::Failed;},"Issuer denial did not fail the actual Session");
            const auto error=session->error();
            check(error.find("ServerDenied")!=std::string::npos,"Rust typed account denial was discarded");
            check(error.find("Matchmaking service unavailable")==std::string::npos,"Account denial became the generic matchmaking error");
            no_unacknowledged_sdk_admission(observer,true);
            auto result=observed(observer,true);result["typed_server_denial"]=true;
            std::cout<<"H2EOSAUTHFIXTURE "<<result.dump()<<std::endl;
        } else if(command=="stop") {
            const auto started=std::chrono::steady_clock::now();
            session.reset();
            const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-started).count();
            check(elapsed<2000,"Pending HTTP prevented prompt Session/transport teardown");
            no_unacknowledged_sdk_admission(observer,mode=="denied");
            check(online_protocol_bridge::eos_process_shutdown(),"SDK process finalization found a retained platform");
            const auto final=actual_sdk_observation(observer,mode=="denied");
            check(final.worker_entries==0||(final.worker_finished==1&&final.worker_result==0),"SDK child did not request successful process-main exit");
            auto result=observed(observer,mode=="denied");result["stopped"]=true;result["teardown_ms"]=elapsed;
            result["sdk_shutdown"]=true;
            std::cout<<"H2EOSAUTHFIXTURE "<<result.dump()<<std::endl;
            return 0;
        } else throw FixtureError("Unknown native fixture command");
    }
    throw FixtureError("Fixture command stream ended before explicit teardown");
} catch(const FixtureError &error) {
    std::cerr<<error.what()<<'\n';return 1;
} catch(...) {
    // Do not echo a JSON parsing exception or SDK/configuration payload.
    std::cerr<<"Native EOS account HTTP fixture failed unexpectedly.\n";return 1;
}
