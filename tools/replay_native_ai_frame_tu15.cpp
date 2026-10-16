#ifdef HALO2_REPLAY_AI_ACTOR_TU15
#include "halo2/source_ai_actor_tu15.hpp"
#elif defined(HALO2_REPLAY_OBJECT_MATERIAL_TU15)
#include "halo2/source_object_material_tu15.hpp"
#elif defined(HALO2_REPLAY_RECORDED_ANIMATION_TU15)
#include "halo2/source_recorded_animation_tu15.hpp"
#else
#include "halo2/source_ai_frame_tu15.hpp"
#endif
#include "halo2/source_runtime_memory.hpp"
#include <algorithm>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <xmmintrin.h>
using namespace halo2;
namespace {
#ifdef HALO2_REPLAY_AI_ACTOR_TU15
constexpr auto run_ai_consumer=source_run_ai_actor_tu15;
#elif defined(HALO2_REPLAY_OBJECT_MATERIAL_TU15)
constexpr auto run_ai_consumer=source_run_object_material_tu15;
#elif defined(HALO2_REPLAY_RECORDED_ANIMATION_TU15)
constexpr auto run_ai_consumer=source_run_recorded_animation_tu15;
#else
constexpr auto run_ai_consumer=source_run_ai_frame_tu15;
#endif
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
template<class T>void write(std::ostream &out,const T &value){out.write(reinterpret_cast<const char*>(&value),sizeof(value));check(bool(out),"Cannot retain native AI frame replay");}
std::vector<std::byte> read_file(const std::filesystem::path &path){std::ifstream file(path,std::ios::binary|std::ios::ate);check(bool(file),"AI frame comparison file absent");const auto size=file.tellg();check(size>=0,"AI frame comparison size absent");std::vector<std::byte> bytes(static_cast<std::size_t>(size));file.seekg(0);file.read(reinterpret_cast<char*>(bytes.data()),size);check(bool(file),"Cannot read AI frame comparison");return bytes;}
}
int main(int argc,char **argv)try {
    check(argc==4 || argc==5,"Supply ORIGINAL_CASE ENTRY OUTPUT [OBSERVABLE_WORD_ADDRESS]");const std::filesystem::path input=argv[1],out=argv[3];
    if(!std::filesystem::exists(input/"before.bin") || !std::filesystem::exists(input/"hardware/hardware-after.bin") || !std::filesystem::exists(input/"original-trace.bin"))return 77;
    std::filesystem::create_directories(out);
    SourceRuntimeMemory memory;std::ifstream bank(input/"before.bin",std::ios::binary);check(bool(bank),"Actual original AI frame input bank absent");unsigned regions{};
    for(std::uint32_t address,size;bank.read(reinterpret_cast<char*>(&address),4);){
        bank.read(reinterpret_cast<char*>(&size),4);check(bool(bank) && size && size<0x40000000u && ++regions<4096,"Invalid captured source range");
        std::vector<std::byte> bytes(size);bank.read(reinterpret_cast<char*>(bytes.data()),size);check(bool(bank),"Truncated captured source range");memory.mount(address,std::move(bytes),SourceRuntimeMemory::Access::ReadWrite,"Original source AI frame replay bank");
    }
    SourcePhysicsWorldRuntimeAbi abi{};std::ifstream caller(input/"native-input.bin",std::ios::binary);check(bool(caller),"Original AI frame caller ABI absent");
    caller.read(reinterpret_cast<char*>(abi.gp.data()),sizeof(abi.gp));caller.read(reinterpret_cast<char*>(abi.xmm.data()),sizeof(abi.xmm));caller.read(reinterpret_cast<char*>(&abi.eflags),4);caller.read(reinterpret_cast<char*>(&abi.fs_base),4);check(bool(caller),"Truncated original AI frame caller");
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};const auto entry=std::stoul(argv[2],nullptr,0);
    if(std::filesystem::exists(input/"native-fp-input.bin")) {
        std::ifstream fp_input(input/"native-fp-input.bin",std::ios::binary);
        SourcePhysicsFactoryFloatingPoint captured{};
        fp_input.read(reinterpret_cast<char*>(&abi.x87_depth),4);
        fp_input.read(reinterpret_cast<char*>(&abi.x87_status),2);
        fp_input.read(reinterpret_cast<char*>(&captured.x87_control_word),2);
        fp_input.read(reinterpret_cast<char*>(&captured.mxcsr),4);
        check(bool(fp_input) && abi.x87_depth==0,"Captured source caller requires an empty x87 stack");
        abi.floating_point=captured;
    }
    std::ofstream trace(out/"native-trace.bin",std::ios::binary),stores(out/"native-stores.bin",std::ios::binary);SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](auto p){write(trace,p);};
    // Same explicit CPU primitive policy as the real source world. This is
    // an actual SSE operation over the current operand, never a recorded
    // answer or a game-function return. Xbox lookup bits remain unverified.
    services.reciprocal_sqrt=[](std::uint32_t bits){return std::bit_cast<std::uint32_t>(
        _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(std::bit_cast<float>(bits)))));};
    services.required_owner=[&](auto &,const auto &call,auto &,const auto &){std::ostringstream error;error<<"Unbound original AI frame owner "<<std::hex<<call.instruction<<" -> "<<call.target;throw std::runtime_error(error.str());};
    const auto dump=[&]{std::ofstream file(out/"native-after.bin",std::ios::binary);memory.visit_committed([&](auto p,auto bytes){write(file,p);write(file,std::uint32_t(bytes.size()));file.write(reinterpret_cast<const char*>(bytes.data()),std::streamsize(bytes.size()));});check(bool(file),"Cannot retain AI frame replay bank");};
    try {run_ai_consumer(memory,entry,abi,*abi.floating_point,services,[&](const auto &s){if(s.address>=0x60000000u && s.address<0x60200000u)return;write(stores,s.instruction);write(stores,s.address);write(stores,std::uint32_t(s.written.size()));stores.write(reinterpret_cast<const char*>(s.before.data()),std::streamsize(s.before.size()));stores.write(reinterpret_cast<const char*>(s.written.data()),std::streamsize(s.written.size()));});}
    catch(const std::exception &e){dump();std::ofstream(out/"failure.txt")<<e.what()<<'\n';throw;}
    trace.close();stores.close();dump();std::ofstream exit(out/"native-exit.bin",std::ios::binary);write(exit,abi.gp);write(exit,abi.xmm);write(exit,abi.eflags);write(exit,abi.fs_base);write(exit,abi.x87_depth);write(exit,abi.x87_status);write(exit,abi.floating_point->x87_control_word);write(exit,abi.floating_point->mxcsr);exit.close();
    const bool stepped=std::filesystem::exists(input/"hardware-step-reference.json");
    const auto trace_reference=stepped?input/"hardware/hardware-trace.bin":input/"original-trace.bin";
    const auto stores_reference=stepped?input/"hardware/hardware-stores.bin":input/"original-stores.bin";
    check(read_file(out/"native-trace.bin")==read_file(trace_reference),"Native AI frame instruction path differs");
    check(read_file(out/"native-stores.bin")==read_file(stores_reference),"Native AI frame ordered writes differ");
    auto expected=read_file(input/"hardware/hardware-exit.bin");check(expected.size()==180,"Hardware AI frame exit ABI size differs");
    expected[161]&=std::byte{0xfd};check(read_file(out/"native-exit.bin")==expected,"Native AI frame GP/XMM/FP exit differs from hardware");
    std::ifstream reference(input/"hardware/hardware-after.bin",std::ios::binary);std::size_t compared{};
    for(std::uint32_t address,size;reference.read(reinterpret_cast<char*>(&address),4);){reference.read(reinterpret_cast<char*>(&size),4);check(bool(reference),"Truncated hardware bank");std::vector<std::byte> bytes(size);reference.read(reinterpret_cast<char*>(bytes.data()),size);check(bool(reference),"Truncated hardware source range");if(address>=0x60000000u && address<0x60200000u)continue;check(std::ranges::equal(memory.view(address,size),bytes),"Native AI frame retained bank differs from hardware");compared+=size;}
    if(argc==5) {
        const auto address=std::uint32_t(std::stoul(argv[4],nullptr,0));
        std::ifstream word_reference(input/"hardware/hardware-after.bin",std::ios::binary);bool found=false;
        for(std::uint32_t base,size;word_reference.read(reinterpret_cast<char*>(&base),4);) {
            word_reference.read(reinterpret_cast<char*>(&size),4);check(bool(word_reference),"Truncated observable-word region");
            std::vector<std::byte> bytes(size);word_reference.read(reinterpret_cast<char*>(bytes.data()),size);check(bool(word_reference),"Truncated observable-word data");
            if(base<=address && std::uint64_t(address)+2<=std::uint64_t(base)+size) {
                check(std::ranges::equal(memory.view(address,2),std::span(bytes).subspan(address-base,2)),"Native observable caller WORD differs from hardware");
                std::ofstream word(out/"observable-word.bin",std::ios::binary);word.write(reinterpret_cast<const char*>(bytes.data()+address-base),2);check(bool(word),"Cannot retain observable caller WORD");found=true;break;
            }
        }
        check(found,"Observable caller WORD absent from hardware bank");
    }
    std::cout<<"PASS native complete source replay entry="<<std::hex<<entry<<std::dec<<"; exact trace, ordered writes, full hardware exit except IF and "<<compared<<" retained bytes; observable WORD checked="<<(argc==5)<<'\n';return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
