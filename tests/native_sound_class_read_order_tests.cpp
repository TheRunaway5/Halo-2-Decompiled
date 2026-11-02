// Replay the independent unchanged-original corpus through the actual registry.
// Unrelated inventories are omitted by the existing focused link fixture.
#include "halo2/native_tu15_sound_manager.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <bit>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>
using J=nlohmann::json;
namespace {
void need(bool ok,const std::string &why){if(!ok)throw std::runtime_error(why);}
std::vector<std::byte> raw_bytes(const std::string &s) {
    std::vector<std::byte> out;
    for(std::size_t n=0;n<s.size();n+=2)out.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));
    return out;
}
std::string raw_hex(std::span<const std::byte> input) {
    constexpr auto digits="0123456789abcdef";std::string out;
    for(const auto b:input){const auto v=std::to_integer<unsigned>(b);out+=digits[v>>4];out+=digits[v&15];}
    return out;
}
struct Boundary {};
class CheckedMemory final:public halo2::SourcePhysicsContactMemory {
    const J &input;std::map<std::uint32_t,std::byte> data;
    std::map<std::uint32_t,unsigned> occurrences;unsigned ordinal{};
public:
    J reads=J::array(),boundary=nullptr,applied=J::array();
    explicit CheckedMemory(const J &c):input(c) {
        for(const auto &bank:c.at("banks")) {
            const auto start=bank.at("address").get<std::uint32_t>();const auto raw=raw_bytes(bank.at("before"));
            for(unsigned n=0;n<raw.size();++n)need(data.emplace(start+n,raw[n]).second,"Overlapping input banks");
        }
    }
    std::vector<std::byte> peek(std::uint32_t at,unsigned count)const {
        std::vector<std::byte> out;for(unsigned n=0;n<count;++n)out.push_back(data.at(at+n));return out;
    }
    void read(std::uint32_t at,std::span<std::byte> out)const override {const_cast<CheckedMemory*>(this)->read_value(at,out);}
    void write(std::uint32_t,std::span<const std::byte>)override {throw std::runtime_error("Unexpected native store");}
    void read_value(std::uint32_t at,std::span<std::byte> out) {
        const auto fault=input.value("fault",J::object());
        if(fault.value("phase","")=="before_read" && fault.at("ordinal")==ordinal) {
            boundary={{"phase","before_read"},{"address",at},{"bytes",out.size()}};throw Boundary{};
        }
        for(unsigned n=0;n<out.size();++n)if(!data.contains(at+n)) {
            boundary={{"phase","memory_fault"},{"address",at},{"bytes",out.size()}};throw Boundary{};
        }
        const auto raw=peek(at,out.size());std::copy(raw.begin(),raw.end(),out.begin());
        reads.push_back({{"address",at},{"bytes",out.size()}});++ordinal;
        const auto occurrence=++occurrences[at];
        for(const auto &edit:input.value("input_timeline",J::array()))
            if(edit.at("on_read").at("address")==at && edit.at("on_read").at("occurrence")==occurrence) {
                for(const auto &write:edit.at("writes")) {
                    const auto start=write.at("address").get<std::uint32_t>();const auto raw=raw_bytes(write.at("bytes"));
                    for(unsigned n=0;n<raw.size();++n)data.at(start+n)=raw[n];
                }
                applied.push_back(edit);
            }
    }
};
}
int main(int argc,char **argv)try {
    need(std::getenv("HALO2_TEST_SCRATCH"),"Use tools/run_test.py");need(argc==2,"Supply original fixture");
    std::ifstream file(argv[1]);const auto fixture=J::parse(file);unsigned count{},faults{},reads{};
    for(const auto &c:fixture.at("cases")) {
        const auto name=c.at("name").get<std::string>();CheckedMemory memory(c);bool completed=false;
        halo2::SourcePhysicsContactRuntimeAbi abi{};
        abi.gp=c.at("gp").get<std::array<std::uint32_t,8>>();abi.x87_status=c.at("before_fp")[1];
        const halo2::SourcePhysicsFactoryFloatingPoint fp{c.at("before_fp")[0].get<std::uint16_t>(),c.at("before_fp")[2].get<std::uint32_t>()};
        abi.floating_point=fp;
        if(c.value("active_mmx",false)) {
            halo2::SourcePhysicsMmxAlias mmx{};mmx.known_mask=0xff;
            for(unsigned r=0;r<8;++r) {
                const auto raw=raw_bytes(c.at("live_x87")[r]);std::copy(raw.begin(),raw.end(),mmx.physical[r].begin());
            }
            abi.active_mmx=mmx;
        } else {
            abi.x87_depth=c.at("live_x87").size();
            for(unsigned r=0;r<abi.x87_depth;++r) {
                const auto raw=raw_bytes(c.at("live_x87")[r]);std::memcpy(&abi.x87[r],raw.data(),10);
            }
        }
        const auto original_extended=std::bit_cast<std::array<std::byte,sizeof(abi.x87)>>(abi.x87);
        const auto original_depth=abi.x87_depth;
        const auto original_mmx=abi.active_mmx.has_value();
        const auto mmx_before=abi.active_mmx.value_or(halo2::SourcePhysicsMmxAlias{});
        for(unsigned r=0;r<8;++r)for(unsigned n=0;n<4;++n)abi.xmm[r][n]=0x6b000000u+c.at("id").get<unsigned>()*37u+r*4u+n;
        const auto original_xmm=abi.xmm;
        try {halo2::native_tu15_sound_manager_run(memory,0x228220u,abi,fp,{},{});completed=true;}
        catch(const Boundary &){++faults;}
        auto original_reads=c.at("reads");for(auto &read:original_reads)read.erase("instruction");
        need(completed==c.at("completed").get<bool>(),name+": completion differs");
        need(memory.boundary==c.at("typed_boundary"),name+": fault boundary differs");
        need(memory.reads==original_reads,name+": ordered reads differ");
        need(c.at("writes").empty(),name+": original stores differ");
        need(memory.applied==c.at("applied_input_timeline"),name+": input timeline differs");
        need(c.at("before_fp")==c.at("after_fp") && c.at("entry_live_x87_preserved")==true &&
             c.at("original_xmm_preserved")==true,name+": original FP/SIMD state differs");
        need(abi.xmm==original_xmm && abi.x87_status==c.at("before_fp")[1].get<std::uint16_t>() &&
             abi.floating_point->x87_control_word==fp.x87_control_word && abi.floating_point->mxcsr==fp.mxcsr,
             name+": native retained FP/SIMD fields differ");
        need(std::bit_cast<std::array<std::byte,sizeof(abi.x87)>>(abi.x87)==original_extended && abi.x87_depth==original_depth &&
             abi.active_mmx.has_value()==original_mmx && (!original_mmx ||
              (abi.active_mmx->physical==mmx_before.physical && abi.active_mmx->known_mask==mmx_before.known_mask)),
              name+": native retained extended/MMX fields differ");
        if(completed)need(abi.gp==c.at("boundary_gp").get<std::array<std::uint32_t,8>>(),name+": completed GP result differs");
        for(const auto &bank:c.at("banks"))need(raw_hex(memory.peek(bank.at("address"),bank.at("before").get_ref<const std::string&>().size()/2))==bank.at("after").get_ref<const std::string&>(),name+": retained bytes differ");
        reads+=memory.reads.size();++count;
    }
    std::cout<<count<<" original228220 comparisons; "<<faults<<" prefixes/faults; "<<reads<<" ordered reads; all completed GP outputs exact\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
