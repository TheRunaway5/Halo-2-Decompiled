#include "halo2/confined_file.hpp"
#include <chrono>
#include <fstream>
#include <iostream>

using namespace halo2;
template<class F> void rejects(F fn) {
    try {fn();}catch(const FormatError&){return;}
    throw FormatError("Unsafe resource path admitted");
}
int main()try {
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
    std::cout<<"Confined regular-file reads, link rejection and descriptor ownership passed\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
