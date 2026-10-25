// Linux/GNU fixture only. Observe five real SDK entries in the separately built
// Rust worker, forwarding their official ABI and result without SDK options or
// callback changes. This library is never part of a production package.
#include "eos_sdk_worker_observer.hpp"
#include <eos_connect.h>
#include <eos_p2p.h>
#include <eos_sdk.h>
#include <cerrno>
#include <cstdlib>
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <stdexcept>
#include <string_view>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace {
using namespace halo2::eos_worker_fixture;
void require(bool value) {if(!value)throw std::runtime_error("SDK fixture observer refused");}
class Lock {
public:
    Lock(int fd,std::mutex &mutex):local_(mutex),fd_(fd) {
        int result;do{result=::flock(fd_,LOCK_EX);}while(result<0&&errno==EINTR);
        require(result==0);
    }
    ~Lock(){::flock(fd_,LOCK_UN);}
private:
    std::unique_lock<std::mutex> local_;int fd_;
};
struct State {int fd{-1};ObserverData *data{};std::mutex mutex;};
State *active{};

void initialize() {
    const auto *scratch=std::getenv("HALO2_TEST_SCRATCH");
    const auto *value=std::getenv(observer_environment);
    const auto *expected=std::getenv(worker_environment);
    require(scratch&&*scratch&&value&&*value&&expected&&*expected);
    const auto root=std::filesystem::canonical(scratch);
    const std::filesystem::path path=value,worker=expected;
    require(root.filename().string().starts_with("halo2-test-")&&path.is_absolute()&&path.parent_path()==root&&
        path.filename().string().starts_with("eos-account-observer-")&&worker.is_absolute()&&
        worker.filename()=="eos_sdk_worker"&&std::filesystem::is_regular_file(std::filesystem::symlink_status(worker))&&
        std::filesystem::canonical(worker)==worker&&std::filesystem::canonical("/proc/self/exe")==worker);
    std::array<char,8192> bytes{};std::ifstream command("/proc/self/cmdline",std::ios::binary);
    command.read(bytes.data(),bytes.size());const auto count=command.gcount();
    require(command.eof()&&count>0&&count<static_cast<std::streamsize>(bytes.size()));
    const std::string_view arguments(bytes.data(),static_cast<std::size_t>(count));
    const auto end=arguments.find('\0');constexpr char flag[]="--eos-sdk-worker";
    require(end!=std::string_view::npos&&end>0&&arguments.substr(end+1)==std::string_view(flag,sizeof(flag)));
    // This process owns the small mapping until exit, including all exit hooks.
    // No SDK state or credential bytes enter it; the OS retires its FD/mapping.
    auto *state=new State;
    state->fd=::open(path.c_str(),O_RDWR|O_CLOEXEC|O_NOFOLLOW);
    struct stat metadata{};
    require(state->fd>=0&&::fstat(state->fd,&metadata)==0&&S_ISREG(metadata.st_mode)&&
        metadata.st_uid==::getuid()&&(metadata.st_mode&0777)==0600&&metadata.st_nlink==1&&
        metadata.st_size==static_cast<off_t>(sizeof(ObserverData)));
    auto *mapped=::mmap(nullptr,sizeof(ObserverData),PROT_READ|PROT_WRITE,MAP_SHARED,state->fd,0);
    require(mapped!=MAP_FAILED);state->data=static_cast<ObserverData *>(mapped);
    {
        Lock lock(state->fd,state->mutex);auto &data=*state->data;
        require(data.magic==observer_magic&&data.parent_pid==static_cast<std::uint64_t>(::getppid())&&
            data.worker_entries==0&&::syscall(SYS_gettid)==::getpid());
        data.worker_pid=::getpid();data.worker_entries=1;data.worker_main=1;data.worker_image_verified=1;
    }
    active=state;
}

void record(SdkCall call) noexcept {
    try {
        require(active);Lock lock(active->fd,active->mutex);auto &data=*active->data;
        require(data.magic==observer_magic);++data.calls[call];
        if(static_cast<std::uint64_t>(::getpid())==data.parent_pid)++data.parent_calls;
        if(static_cast<std::uint64_t>(::getpid())!=data.worker_pid)++data.foreign_calls;
        if(::syscall(SYS_gettid)!=::getpid())++data.off_main_calls;
    } catch(...) {std::_Exit(71);}
}

template<class F> F next(const char *name) noexcept {
    auto *symbol=::dlsym(RTLD_NEXT,name);if(!symbol)std::_Exit(72);
    return reinterpret_cast<F>(symbol);
}

__attribute__((constructor)) void attach() noexcept {
    try {initialize();}catch(...) {std::_Exit(71);}
}
}

extern "C" {
void EOS_CALL EOS_Connect_CreateDeviceId(EOS_HConnect handle,const EOS_Connect_CreateDeviceIdOptions *options,void *client,EOS_Connect_OnCreateDeviceIdCallback callback) {
    record(device_call);next<decltype(&EOS_Connect_CreateDeviceId)>("EOS_Connect_CreateDeviceId")(handle,options,client,callback);
}
void EOS_CALL EOS_Connect_Login(EOS_HConnect handle,const EOS_Connect_LoginOptions *options,void *client,EOS_Connect_OnLoginCallback callback) {
    record(login_call);next<decltype(&EOS_Connect_Login)>("EOS_Connect_Login")(handle,options,client,callback);
}
void EOS_CALL EOS_Connect_CreateUser(EOS_HConnect handle,const EOS_Connect_CreateUserOptions *options,void *client,EOS_Connect_OnCreateUserCallback callback) {
    record(create_call);next<decltype(&EOS_Connect_CreateUser)>("EOS_Connect_CreateUser")(handle,options,client,callback);
}
EOS_NotificationId EOS_CALL EOS_P2P_AddNotifyPeerConnectionRequest(EOS_HP2P handle,const EOS_P2P_AddNotifyPeerConnectionRequestOptions *options,void *client,EOS_P2P_OnIncomingConnectionRequestCallback callback) {
    record(socket_call);return next<decltype(&EOS_P2P_AddNotifyPeerConnectionRequest)>("EOS_P2P_AddNotifyPeerConnectionRequest")(handle,options,client,callback);
}
void EOS_CALL EOS_Platform_Tick(EOS_HPlatform platform) {
    record(tick_call);next<decltype(&EOS_Platform_Tick)>("EOS_Platform_Tick")(platform);
}

// The Rust bin uses process::exit(worker_main()). Record that real requested
// status, then forward libc exit unchanged; a destructor cannot invent status0.
[[noreturn]] void exit(int status) noexcept {
    try {
        if(active) {
            Lock lock(active->fd,active->mutex);auto &data=*active->data;
            require(data.magic==observer_magic&&data.worker_pid==static_cast<std::uint64_t>(::getpid())&&
                ::syscall(SYS_gettid)==::getpid());
            data.worker_finished=1;data.worker_result=static_cast<std::uint64_t>(static_cast<std::int64_t>(status));
        }
    } catch(...) {std::_Exit(71);}
    next<void(*)(int)>("exit")(status);std::_Exit(72);
}
}
