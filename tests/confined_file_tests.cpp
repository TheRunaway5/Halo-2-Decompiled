#include "halo2/confined_file.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <cstring>

using namespace halo2;
template<class F> void rejects(F fn) {
    try {fn();}catch(const FormatError&){return;}
    throw FormatError("Unsafe resource path admitted");
}
int main(int argc,char** argv)try {
    const auto root=std::filesystem::temp_directory_path()/
        ("confined-resource-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Owner {std::filesystem::path path;~Owner(){std::filesystem::remove_all(path);}} owner{root};
    std::filesystem::create_directories(root/"assets");
    {std::ofstream out(root/"assets/shared.map");out<<"authored";}
    {std::ofstream out(root/"outside.map");out<<"private";}
    const auto regular=read_confined_file(root/"assets/shared.map",1,3,false);
    if(regular!=std::vector{std::byte{'u'},std::byte{'t'},std::byte{'h'}})throw FormatError("Regular range changed");
    rejects([&]{read_confined_file(root/"assets");});
    rejects([&]{read_confined_file(root/"assets/../outside.map");});
#if defined(_WIN32)
    // A real directory junction must stay rejected even on Wine, whose host
    // filesystem mount folders carry synthetic mount-point attributes.
    const auto link=root/"redirect";
    std::filesystem::create_directory(link);
    const auto target=std::filesystem::absolute(root/"assets").native();
    const auto substitute=std::wstring(L"\\??\\")+target;
    const auto sub_bytes=substitute.size()*sizeof(wchar_t),print_bytes=target.size()*sizeof(wchar_t);
    std::vector<std::byte> reparse(16+sub_bytes+sizeof(wchar_t)+print_bytes+sizeof(wchar_t));
    const auto word=[&](unsigned at,std::uint16_t value){std::memcpy(reparse.data()+at,&value,sizeof(value));};
    const DWORD tag=IO_REPARSE_TAG_MOUNT_POINT;std::memcpy(reparse.data(),&tag,sizeof(tag));
    word(4,std::uint16_t(reparse.size()-8));word(10,std::uint16_t(sub_bytes));
    word(12,std::uint16_t(sub_bytes+sizeof(wchar_t)));word(14,std::uint16_t(print_bytes));
    std::memcpy(reparse.data()+16,substitute.data(),sub_bytes);
    std::memcpy(reparse.data()+16+sub_bytes+sizeof(wchar_t),target.data(),print_bytes);
    const auto handle=CreateFileW(link.c_str(),GENERIC_WRITE,0,nullptr,OPEN_EXISTING,
        FILE_FLAG_OPEN_REPARSE_POINT|FILE_FLAG_BACKUP_SEMANTICS,nullptr);
    if(handle==INVALID_HANDLE_VALUE)throw FormatError("Cannot open junction fixture");
    DWORD returned{};const bool created=DeviceIoControl(handle,FSCTL_SET_REPARSE_POINT,reparse.data(),
        DWORD(reparse.size()),nullptr,0,&returned,nullptr)!=0;CloseHandle(handle);
    if(!created)throw FormatError("Cannot create junction fixture");
    rejects([&]{read_confined_file(link/"shared.map");});
    if(!RemoveDirectoryW(link.c_str()))throw FormatError("Cannot remove junction fixture");
#endif
#if !defined(_WIN32)
    std::filesystem::create_symlink(root/"outside.map",root/"assets/mainmenu.map");
    rejects([&]{read_confined_file(root/"assets/mainmenu.map");});
    std::filesystem::create_directory_symlink(root/"assets",root/"redirect");
    rejects([&]{read_confined_file(root/"redirect/shared.map");});
    const int fd=open_confined_file(root/"assets/shared.map");
    std::filesystem::rename(root/"assets/shared.map",root/"assets/original.map");
    std::filesystem::rename(root/"outside.map",root/"assets/shared.map");
    char bytes[8]{};const auto count=::pread(fd,bytes,8,0);::close(fd);
    if(count!=8||std::string(bytes,8)!="authored")throw FormatError("Opened identity was replaced");
#endif
    if(argc==2) {
        const auto actual=read_confined_file(argv[1]);
        const std::string text(reinterpret_cast<const char*>(actual.data()),actual.size());
        if(text!="authored")throw FormatError("Host-mount resource read changed");
    }else if(argc!=1)throw FormatError("Supply at most one owned host-mount fixture");
    std::cout<<"Confined regular-file reads, link rejection and descriptor ownership passed\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
