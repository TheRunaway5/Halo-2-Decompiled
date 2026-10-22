#pragma once
#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <string_view>
#include <fcntl.h>
#include <sys/mount.h>
#include <unistd.h>

namespace halo2::cache_snapshot_detail {
inline std::filesystem::path backing_directory() {
    const char *cache=std::getenv("XDG_CACHE_HOME");
    if(cache && std::filesystem::path(cache).is_absolute())
        return std::filesystem::path(cache)/"halo2-native"/"file-snapshots";
    const char *home=std::getenv("HOME");
    if(!home || !std::filesystem::path(home).is_absolute())return {};
    return std::filesystem::path(home)/"Library"/"Caches"/"halo2-native"/"file-snapshots";
}
inline int snapshot_file() {
    const auto directory=backing_directory();
    if(directory.empty())return -1;
    std::error_code error;
    std::filesystem::create_directories(directory,error);
    if(error)return -1;
    auto name=(directory/"cache-XXXXXX").string();
    const int descriptor=::mkstemp(name.data());
    if(descriptor<0)return -1;
    // Unlink before any bytes are written. Closing the last mapping releases
    // this private inode after ordinary exit, cancellation or process death.
    if(::unlink(name.c_str()) || ::fcntl(descriptor,F_SETFD,FD_CLOEXEC)<0) {
        ::close(descriptor);return -1;
    }
    return descriptor;
}
inline bool disk_backed(int descriptor) {
    struct statfs filesystem{};
    if(::fstatfs(descriptor,&filesystem))return false;
    const std::string_view kind(filesystem.f_fstypename);
    return kind!="tmpfs" && kind!="ramfs" && kind!="devfs";
}
inline int snapshot_sync(int descriptor) {
    int result;
    do {result=::fsync(descriptor);}while(result && errno==EINTR);
    return result;
}
inline bool copy_contents(int source,int destination,std::size_t size) {
    // A new unlinked inode is independent of source edits/truncation. Stream
    // through bounded storage and pread so the admitted source cursor stays
    // untouched. This also works across volumes without APFS clone support.
    std::array<std::byte,65536> buffer;
    std::size_t offset=0,pending=0;
    while(offset<size) {
        const auto count=::pread(source,buffer.data(),std::min(buffer.size(),size-offset),off_t(offset));
        if(count<0 && errno==EINTR)continue;
        if(count<=0)return false;
        for(std::size_t written=0;written<std::size_t(count);) {
            const auto n=::pwrite(destination,buffer.data()+written,count-written,off_t(offset+written));
            if(n<0 && errno==EINTR)continue;
            if(n<=0)return false;
            written+=std::size_t(n);
        }
        offset+=std::size_t(count);pending+=std::size_t(count);
        if(pending>=1024*1024) {
            if(snapshot_sync(destination))return false;
            pending=0;
        }
    }
    return snapshot_sync(destination)==0;
}
}
