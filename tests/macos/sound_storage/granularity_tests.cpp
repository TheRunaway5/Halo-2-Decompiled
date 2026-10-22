#include "halo2/native_sound_storage.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace halo2;
namespace {
void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
}
int main(int argc,char **argv) {
    std::size_t host_page=0;int status=0;std::string error;
    try {
        require(argc==2||(argc==4&&std::string_view(argv[2])=="--map-only"),"Expected a bounded report path and optional isolated alias");
        NativeSoundStorage storage({0x480d68,0x10000000,0x10100000});host_page=storage.page_size();
        if(argc==4) {
            storage.map_zeroed(std::uint32_t(std::stoul(argv[3],nullptr,16)),0x300000);
        } else {
        // The retained21A6E0 caller protects this exact physical alias/span.
        // It is4KiB aligned, with a4KiB residue on a16KiB host.
        storage.map_zeroed(0x82e45000,0x300000);
        auto services=storage.cache_services();
        services.raw_protect(0x82e45000,0x300000,4);
        storage.memory(0x82e45000,1)[0]=std::byte{0x39};
        require(storage.memory(0x82e45000,1)[0]==std::byte{0x39},"Original physical buffer lost actual writable backing");
        // Explicit private alias convention used by the original constructor
        // differential. These are not original Xbox allocator addresses.
        const auto first=services.raw_allocate(0x480d68,0x208c);
        const auto second=services.raw_allocate(0x480d68,0x30fc);
        require(first==0x10000000&&second==0x10003000,"Private alias allocation depends on host page size");
        services.raw_protect(first+0x1000,0x1000,4);
        }
    } catch(const std::exception &failure){status=1;error=failure.what();std::cerr<<error<<'\n';}
    if(argc>=2) {
        std::ofstream output(argv[1]);output<<"{\"passed\":"<<(status?"false":"true")<<",\"host_page_bytes\":"<<host_page
            <<",\"error\":\""<<error<<"\"}\n";if(!output)status=1;
    }
    return status;
}
