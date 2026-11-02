#include "halo2/mcc_script_layout.hpp"
#include "halo2/cache.hpp"
#include <iostream>
#include <vector>
using namespace halo2;
namespace {
void put(std::span<std::byte> out,unsigned at,unsigned value,unsigned width=4) {
    for(unsigned n=0;n<width;++n)out[at+n]=std::byte(value>>(8*n));
}
}
int main()try {
    const std::string name="ai_force_active";
    std::vector<std::byte> strings;for(char c:name)strings.push_back(std::byte(c));strings.push_back(std::byte{});
    std::vector<std::byte> nodes(80);
    put(nodes,0,0x9000,2);put(nodes,2,319,2);put(nodes,4,4,2);put(nodes,6,8,2);
    put(nodes,8,0xffffffff);put(nodes,16,0x90010001);
    put(nodes,20,0x9001,2);put(nodes,22,2,2);put(nodes,24,2,2);put(nodes,26,9,2);
    put(nodes,28,0x90020002);
    put(nodes,40,0x9002,2);put(nodes,42,19,2);put(nodes,44,19,2);put(nodes,46,9,2);
    put(nodes,48,0x90030003);put(nodes,56,0xffffffff);
    put(nodes,60,0x9003,2);put(nodes,62,5,2);put(nodes,64,5,2);put(nodes,66,9,2);
    put(nodes,68,0xffffffff);put(nodes,76,1);
    const auto layout=project_mcc_script_layout(nodes,strings,{});
    layout.require_native_ready();
    if(Bytes(layout.nodes).u16(2)!=317)throw FormatError("AI activation selected another native signature");
    std::cout<<"PASS actual E3 AI activation command admission\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
