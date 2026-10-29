#pragma once
#include <algorithm>
#include <filesystem>
#include <vector>
#include "halo2/cache.hpp"
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#endif

namespace halo2 {
// Resolve every component without following links. Each next component is
// opened relative to the retained parent descriptor on POSIX. The final
// descriptor, rather than a checked pathname, owns every subsequent read.
#if !defined(_WIN32)
inline int open_confined_file(const std::filesystem::path& path,bool admit_regular=true) {
    const auto full=std::filesystem::absolute(path);
    int parent=::open("/",O_RDONLY|O_DIRECTORY|O_CLOEXEC);
    if(parent<0)throw FormatError("Cannot open resource root");
    try {
        std::vector<std::filesystem::path> parts;
        for(const auto& part:full.relative_path()) {
            if(part=="..")throw FormatError("Parent traversal in resource path");
            if(part!=".")parts.push_back(part);
        }
        if(parts.empty())throw FormatError("Missing resource filename");
        for(std::size_t i=0;i<parts.size();++i) {
            const bool last=i+1==parts.size();
            const int next=::openat(parent,parts[i].c_str(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK|
                (last?0:O_DIRECTORY));
            if(next<0)throw FormatError("Linked or unavailable resource path");
            ::close(parent);parent=next;
        }
        struct stat info{};
        if(admit_regular && (::fstat(parent,&info)!=0||!S_ISREG(info.st_mode)))throw FormatError("Nonregular resource file");
        return parent;
    } catch(...) {::close(parent);throw;}
}
#endif

inline std::vector<std::byte> read_confined_file(const std::filesystem::path& path,
    std::uint64_t offset=0,std::uint64_t length=0,bool whole=true) {
#if defined(_WIN32)
    // Deny deletion while walking, retaining all parent handles so a directory
    // cannot be replaced between admission and the final open.
    std::vector<HANDLE> handles;
    auto close=[&]{for(auto h:handles)CloseHandle(h);};
    try {
        const auto full=std::filesystem::absolute(path);
        auto current=full.root_path();
        for(const auto& part:full.relative_path()) {
            if(part==L"..")throw FormatError("Parent traversal in resource path");
            if(part==L".")continue;
            current/=part;
            HANDLE h=CreateFileW(current.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,
                FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_BACKUP_SEMANTICS,nullptr);
            if(h==INVALID_HANDLE_VALUE)throw FormatError("Cannot open resource component");
            handles.push_back(h);BY_HANDLE_FILE_INFORMATION info{};
            if(!GetFileInformationByHandle(h,&info)||(info.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT))
                throw FormatError("Linked resource path");
        }
        if(handles.empty())throw FormatError("Missing resource filename");
        const auto file=handles.back();BY_HANDLE_FILE_INFORMATION info{};LARGE_INTEGER size{};
        if(!GetFileInformationByHandle(file,&info)||(info.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)||
           GetFileType(file)!=FILE_TYPE_DISK||!GetFileSizeEx(file,&size))throw FormatError("Nonregular resource file");
        const auto extent=std::uint64_t(size.QuadPart);
        if(whole)length=extent;
        if(offset>extent||length>extent-offset||length>1024ull*1024*1024)throw FormatError("Resource range budget");
        LARGE_INTEGER at{};at.QuadPart=offset;
        if(!SetFilePointerEx(file,at,nullptr,FILE_BEGIN))throw FormatError("Resource seek failed");
        std::vector<std::byte> out(static_cast<std::size_t>(length));
        for(std::size_t done=0;done<out.size();) {
            DWORD got{};const auto count=DWORD(std::min<std::size_t>(out.size()-done,1024*1024));
            if(!ReadFile(file,out.data()+done,count,&got,nullptr)||!got)throw FormatError("Short resource read");
            done+=got;
        }
        close();return out;
    }catch(...){close();throw;}
#else
    const int fd=open_confined_file(path);
    try {
        struct stat info{};
        if(::fstat(fd,&info)!=0||info.st_size<0)throw FormatError("Cannot measure resource");
        const auto extent=std::uint64_t(info.st_size);
        if(whole)length=extent;
        if(offset>extent||length>extent-offset||length>1024ull*1024*1024)throw FormatError("Resource range budget");
        std::vector<std::byte> out(static_cast<std::size_t>(length));
        for(std::size_t done=0;done<out.size();) {
            const auto got=::pread(fd,out.data()+done,out.size()-done,off_t(offset+done));
            if(got<0&&errno==EINTR)continue;
            if(got<=0)throw FormatError("Short resource read");
            done+=std::size_t(got);
        }
        ::close(fd);return out;
    }catch(...){::close(fd);throw;}
#endif
}
}
