#include "halo2/source_effect_particle_draw_tu15.hpp"
#include "halo2/source_effect_particle_frame_tu15.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/source_combat_action_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
template<class T>void write(std::ostream &out,const T &value){out.write(reinterpret_cast<const char*>(&value),sizeof(value));check(bool(out),"Cannot retain native particle replay");}
std::vector<std::byte> read_file(const std::filesystem::path &path){std::ifstream file(path,std::ios::binary|std::ios::ate);check(bool(file),"Particle comparison file absent");const auto size=file.tellg();check(size>=0,"Particle comparison size absent");std::vector<std::byte> bytes(static_cast<std::size_t>(size));file.seekg(0);file.read(reinterpret_cast<char*>(bytes.data()),size);check(bool(file),"Cannot read particle comparison");return bytes;}
}
int main(int argc,char **argv)try {
    check(argc==4,"Supply ORIGINAL_CASE ENTRY OUTPUT");const std::filesystem::path input=argv[1],out=argv[3];
    if(!std::filesystem::exists(input/"before.bin") || !std::filesystem::exists(input/"hardware/hardware-after.bin") || !std::filesystem::exists(input/"original-trace.bin"))return 77;
    std::filesystem::create_directories(out);
    SourceRuntimeMemory memory;std::ifstream bank(input/"before.bin",std::ios::binary);check(bool(bank),"Actual original particle input bank absent");unsigned regions{};
    for(std::uint32_t address,size;bank.read(reinterpret_cast<char*>(&address),4);){
        bank.read(reinterpret_cast<char*>(&size),4);check(bool(bank) && size && size<0x40000000u && ++regions<4096,"Invalid captured source range");
        std::vector<std::byte> bytes(size);bank.read(reinterpret_cast<char*>(bytes.data()),size);check(bool(bank),"Truncated captured source range");memory.mount(address,std::move(bytes),SourceRuntimeMemory::Access::ReadWrite,"Original source particle replay bank");
    }
    SourcePhysicsWorldRuntimeAbi abi{};std::ifstream caller(input/"native-input.bin",std::ios::binary);check(bool(caller),"Original particle caller ABI absent");
    caller.read(reinterpret_cast<char*>(abi.gp.data()),sizeof(abi.gp));caller.read(reinterpret_cast<char*>(abi.xmm.data()),sizeof(abi.xmm));caller.read(reinterpret_cast<char*>(&abi.eflags),4);caller.read(reinterpret_cast<char*>(&abi.fs_base),4);check(bool(caller),"Truncated original particle caller");
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};const auto entry=std::stoul(argv[2],nullptr,0);
    std::ofstream trace(out/"native-trace.bin",std::ios::binary),stores(out/"native-stores.bin",std::ios::binary);SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](auto p){write(trace,p);};
    services.required_owner=[&](auto &m,const auto &call,auto &a,const auto &o){if(source_effect_particle_frame_tu15_owns_entry(call.target)){source_run_effect_particle_frame_tu15(m,call.target,a,{0x27f,0x1f80},services,o);return;}if(source_combat_action_tu15_owns_entry(call.target)){source_run_combat_action_tu15(m,call.target,a,{0x27f,0x1f80},services,o);return;}if(source_physics_live_support_tu15_owns_entry(call.target)){source_run_physics_live_support_tu15(m,call.target,a,{0x27f,0x1f80},services,o);return;}std::ostringstream error;error<<"Unbound original particle owner "<<std::hex<<call.instruction<<" -> "<<call.target;throw std::runtime_error(error.str());};
    const auto dump=[&]{std::ofstream file(out/"native-after.bin",std::ios::binary);memory.visit_committed([&](auto p,auto bytes){write(file,p);write(file,std::uint32_t(bytes.size()));file.write(reinterpret_cast<const char*>(bytes.data()),std::streamsize(bytes.size()));});check(bool(file),"Cannot retain particle replay bank");};
    try {source_run_effect_particle_draw_tu15(memory,entry,abi,{0x27f,0x1f80},services,[&](const auto &s){if(s.address>=0x60000000u && s.address<0x60200000u)return;write(stores,s.instruction);write(stores,s.address);write(stores,std::uint32_t(s.written.size()));stores.write(reinterpret_cast<const char*>(s.before.data()),std::streamsize(s.before.size()));stores.write(reinterpret_cast<const char*>(s.written.data()),std::streamsize(s.written.size()));});}
    catch(const std::exception &e){dump();std::ofstream(out/"failure.txt")<<e.what()<<'\n';throw;}
    trace.close();stores.close();dump();std::ofstream exit(out/"native-exit.bin",std::ios::binary);write(exit,abi.gp);write(exit,abi.xmm);write(exit,abi.eflags);write(exit,abi.fs_base);write(exit,abi.x87_depth);write(exit,abi.x87_status);write(exit,abi.floating_point->x87_control_word);write(exit,abi.floating_point->mxcsr);exit.close();
    check(read_file(out/"native-trace.bin")==read_file(input/"original-trace.bin"),"Native particle instruction path differs");
    check(read_file(out/"native-stores.bin")==read_file(input/"original-stores.bin"),"Native particle ordered writes differ");
    auto expected=read_file(input/"hardware/hardware-exit.bin");check(expected.size()==180,"Hardware particle exit ABI size differs");
    expected[161]&=std::byte{0xfd};check(read_file(out/"native-exit.bin")==expected,"Native particle GP/XMM/FP exit differs from hardware");
    std::ifstream reference(input/"hardware/hardware-after.bin",std::ios::binary);std::size_t compared{};
    for(std::uint32_t address,size;reference.read(reinterpret_cast<char*>(&address),4);){reference.read(reinterpret_cast<char*>(&size),4);check(bool(reference),"Truncated hardware bank");std::vector<std::byte> bytes(size);reference.read(reinterpret_cast<char*>(bytes.data()),size);check(bool(reference),"Truncated hardware source range");if(address>=0x60000000u && address<0x60200000u)continue;check(std::ranges::equal(memory.view(address,size),bytes),"Native particle retained bank differs from hardware");compared+=size;}
    std::cout<<"PASS native complete source particle frame entry="<<std::hex<<entry<<std::dec<<"; exact trace, ordered writes, full hardware exit except IF and "<<compared<<" retained bytes\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
