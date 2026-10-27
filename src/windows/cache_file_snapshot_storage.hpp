#pragma once
#include "platform/windows_api.hpp"
#include <atomic>
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>

namespace halo2::cache_snapshot_windows {
struct File {
    int fd;
    explicit File(int descriptor):fd(descriptor) {}
    ~File(){if(fd>=0)::_close(fd);}
    File(const File&)=delete;
    File& operator=(const File&)=delete;
};
struct Handle {
    HANDLE value;
    ~Handle(){if(value && value!=INVALID_HANDLE_VALUE)CloseHandle(value);}
};
inline HANDLE handle(int descriptor) {
    return reinterpret_cast<HANDLE>(::_get_osfhandle(descriptor));
}
inline std::filesystem::path backing_directory() {
    if(const auto *scratch=std::getenv("HALO2_TEST_SCRATCH")) {
        const auto directory=std::filesystem::path(reinterpret_cast<const char8_t*>(scratch));
        if(directory.is_absolute())return directory/"file-snapshots";
    }
    if(const auto *local=::_wgetenv(L"LOCALAPPDATA")) {
        const std::filesystem::path directory(local);
        if(directory.is_absolute())return directory/L"Halo2Native/file-snapshots";
    }
    std::error_code error;
    const auto directory=std::filesystem::temp_directory_path(error);
    return error?std::filesystem::path{}:directory/L"Halo2Native/file-snapshots";
}
inline bool disk_directory(const std::filesystem::path& directory) {
    wchar_t volume[MAX_PATH];
    if(!GetVolumePathNameW(directory.c_str(),volume,MAX_PATH))return false;
    const auto type=GetDriveTypeW(volume);
    return type==DRIVE_FIXED || type==DRIVE_REMOVABLE || type==DRIVE_REMOTE;
}
inline int snapshot_file() {
    const auto directory=backing_directory();
    if(directory.empty())return -1;
    std::error_code error;
    std::filesystem::create_directories(directory,error);
    if(error || !disk_directory(directory))return -1;
    static std::atomic<unsigned long> next{};
    for(unsigned attempt=0;attempt<32;++attempt) {
        const auto name=directory/(L"snapshot-"+std::to_wstring(GetCurrentProcessId())+L"-"+
            std::to_wstring(GetTickCount64())+L"-"+std::to_wstring(next.fetch_add(1))+L".tmp");
        const auto file=CreateFileW(name.c_str(),GENERIC_READ|GENERIC_WRITE,
            FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL|FILE_FLAG_DELETE_ON_CLOSE,nullptr);
        if(file==INVALID_HANDLE_VALUE) {
            if(GetLastError()==ERROR_FILE_EXISTS || GetLastError()==ERROR_ALREADY_EXISTS)continue;
            return -1;
        }
        const auto descriptor=::_open_osfhandle(reinterpret_cast<std::intptr_t>(file),
            _O_BINARY|_O_RDWR|_O_NOINHERIT);
        if(descriptor<0)CloseHandle(file);
        return descriptor;
    }
    return -1;
}
inline bool copy_contents(int descriptor,int destination,std::size_t size) {
    // ReOpenFile gives this reader its own cursor. Duplicated handles would
    // share the caller's cursor and cannot satisfy descriptor capture.
    const Handle source{ReOpenFile(handle(descriptor),GENERIC_READ,
        FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,FILE_FLAG_SEQUENTIAL_SCAN)};
    if(source.value==INVALID_HANDLE_VALUE)return false;
    std::array<std::byte,65536> bytes;
    std::size_t offset=0;
    while(offset<size) {
        DWORD read{};
        const auto wanted=DWORD(std::min(bytes.size(),size-offset));
        if(!ReadFile(source.value,bytes.data(),wanted,&read,nullptr) || !read)return false;
        for(DWORD written=0;written<read;) {
            const auto count=::_write(destination,bytes.data()+written,read-written);
            if(count<=0)return false;
            written+=DWORD(count);
        }
        offset+=read;
        if(offset%(1024*1024)==0 && ::_commit(destination))return false;
    }
    return ::_commit(destination)==0;
}
}
