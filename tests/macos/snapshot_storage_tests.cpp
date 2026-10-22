#include "halo2/source_runtime_host_physical_memory.hpp"
#include "native_protection.hpp"
#include <array>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <unistd.h>
#ifdef __APPLE__
#include "macos/cache_file_snapshot_storage.hpp"
#include <sys/stat.h>
#endif

using namespace halo2;
namespace {
void check(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
template<class F>void rejects(F &&f) {
    bool rejected=false;try{f();}catch(const std::exception &){rejected=true;}
    check(rejected,"Read-only restored physical tail accepted a source write");
}
#ifdef __APPLE__
struct Environment {
    const char *key;
    std::optional<std::string> value;
    explicit Environment(const char *name):key(name) {
        if(const auto *previous=std::getenv(key))value=previous;
    }
    ~Environment(){if(value)::setenv(key,value->c_str(),1);else ::unsetenv(key);}
};
void darwin_file_policy() {
    using namespace halo2::cache_snapshot_detail;
    const auto *scratch=std::getenv("HALO2_TEST_SCRATCH");
    check(scratch && std::filesystem::path(scratch).is_absolute(),"Snapshot policy test needs owned disk scratch");
    {
        Environment cache("XDG_CACHE_HOME"),home("HOME");
        const auto fake_home=std::filesystem::path(scratch)/"home";
        check(!::setenv("HOME",fake_home.c_str(),1) && !::unsetenv("XDG_CACHE_HOME"),"Cannot select test cache home");
        check(backing_directory()==fake_home/"Library"/"Caches"/"halo2-native"/"file-snapshots",
              "Darwin default snapshots escaped Library/Caches");
        check(!::setenv("XDG_CACHE_HOME","relative-cache",1),"Cannot select relative cache path");
        check(backing_directory()==fake_home/"Library"/"Caches"/"halo2-native"/"file-snapshots",
              "Relative cache override escaped the absolute default");
    }
    struct File {int fd;~File(){if(fd>=0)::close(fd);}} file{snapshot_file()};
    struct stat info{};
    check(file.fd>=0 && !::fstat(file.fd,&info) && S_ISREG(info.st_mode) &&
          info.st_nlink==0 && (info.st_mode&0777)==0600 && (::fcntl(file.fd,F_GETFD)&FD_CLOEXEC),
          "Darwin private snapshot inode ownership/cleanup policy differs");
    check(disk_backed(file.fd),"Darwin snapshot scratch is not accepted disk storage");
    struct statfs filesystem{};check(!::fstatfs(file.fd,&filesystem),"Cannot query snapshot filesystem");
    std::cout<<"PASS Darwin Library/Caches default, absolute override, private unlinked0600/CLOEXEC inode; filesystem="
             <<filesystem.f_fstypename<<'\n';
}
#endif
void unaligned_native_tail() {
    SourceRuntimeMemory original,target,replacement;
    SourceRuntimeHostPhysicalMemory physical(original),resumed(target);
    constexpr std::uint32_t bytes=0x101000;
    const auto base=physical.allocate({bytes,0x61000,0x61000+bytes-1,4096,4});
    original.write(base,std::array{std::byte{0x19}});
    original.write(base+bytes-1,std::array{std::byte{0x73}});
    physical.protect(base+bytes-4096,4096,2);
    const auto saved=original.checkpoint();const auto ownership=physical.ownership_checkpoint();
    replacement.restore(saved);
    const auto prior=replacement.native_address(base,bytes);
    auto staged=resumed.prepare_ownership(ownership,saved,replacement);
    const auto native=replacement.native_address(base,bytes);
    check((native==prior)==(bytes % ::sysconf(_SC_PAGESIZE)==0),
          "Snapshot adoption did not distinguish complete native granules from source-sized tails");
    check(!macos_test::native_writable(reinterpret_cast<void*>(native.value),bytes-1),
          "Read-only physical tail lost its native protection");
    target.swap_backing(replacement);
    check(resumed.commit_ownership(std::move(staged)),"Physical tail ownership did not publish");
    check(target.view(base,1)[0]==std::byte{0x19} && target.view(base+bytes-1,1)[0]==std::byte{0x73},
          "Independent disk snapshot/fallback lost complete tail bytes");
    rejects([&]{target.write(base+bytes-1,std::array{std::byte{0x55}});});
    resumed.protect(base+bytes-4096,4096,4);
    target.write(base+bytes-1,std::array{std::byte{0x66}});
    check(saved.regions[0].bytes[bytes-1]==std::byte{0x73} && original.view(base+bytes-1,1)[0]==std::byte{0x73},
          "Restored tail write reached source bank or saved checkpoint");
    resumed.release(base);
    std::cout<<"PASS disk-backed source4KiB tail, native-granule adoption/fallback and two-way isolation\n";
}
}
int main()try {
#ifdef __APPLE__
    darwin_file_policy();
#endif
    unaligned_native_tail();return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
