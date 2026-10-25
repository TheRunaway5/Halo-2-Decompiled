#include "halo2/source_object_loop_audio_tu15.hpp"
#include <bit>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <openssl/sha.h>
#include <sstream>

using namespace halo2;
namespace {
struct Memory final:SourcePhysicsContactMemory {
    static constexpr std::uint32_t sound_address=0x1001000,point_address=0x1003000,
        stack_address=0x100e000;
    std::array<std::byte,4> pointer{};
    std::array<std::byte,0x20c> sound{};
    std::array<std::byte,12> point{};
    std::array<std::byte,0x2000> stack{};
    Memory(){store(pointer,0,sound_address);}
    static void store(std::span<std::byte> bytes,unsigned offset,std::uint32_t value) {
        for(unsigned n=0;n<4;++n)bytes[offset+n]=std::byte(value>>(n*8));
    }
    std::span<std::byte> region(std::uint32_t address,std::size_t size) {
        for(auto [base,bytes]:{std::pair{0x4f2c50u,std::span<std::byte>(pointer)},
            {sound_address,sound},{point_address,point},{stack_address-0x1000,stack}})
            if(address>=base && std::uint64_t(address)+size<=std::uint64_t(base)+bytes.size())
                return bytes.subspan(address-base,size);
        throw std::runtime_error("Audibility accessed an undeclared component operand");
    }
    void read(std::uint32_t address,std::span<std::byte> out)const override {
        const auto bytes=const_cast<Memory*>(this)->region(address,out.size());
        std::copy(bytes.begin(),bytes.end(),out.begin());
    }
    void write(std::uint32_t address,std::span<const std::byte> bytes)override {
        auto out=region(address,bytes.size());std::copy(bytes.begin(),bytes.end(),out.begin());
    }
};
void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
std::string digest(const std::vector<unsigned char>&bytes) {
    unsigned char hash[SHA256_DIGEST_LENGTH];SHA256(bytes.data(),bytes.size(),hash);
    std::ostringstream out;out<<std::hex<<std::setfill('0');
    for(auto value:hash)out<<std::setw(2)<<unsigned(value);return out.str();
}
}
int main(int argc,char**argv)try {
    need(argc==2,"Supply the independent original audibility fixture");
    std::ifstream file(argv[1]);const auto fixture=nlohmann::json::parse(file);
    need(fixture.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
        "Audibility fixture has a different source authority");
    const auto positions=fixture.at("positions_bits").get<std::vector<std::array<std::uint32_t,3>>>();
    const auto points=fixture.at("points_bits").get<std::vector<std::array<std::uint32_t,3>>>();
    const auto radii=fixture.at("radii_bits").get<std::vector<std::uint32_t>>();
    Memory memory;std::array<std::vector<unsigned char>,2> results;
    for(unsigned lane=0;lane<2;++lane)for(unsigned variation=0;variation<8;++variation)
      for(unsigned mask=0;mask<16;++mask) {
        memory.sound.fill(std::byte{});SourceObjectAudioViewsTu15 views;
        for(unsigned slot=0;slot<4;++slot) {
            auto selected=positions.at((slot+variation)%4);
            if(variation&4)for(auto &value:selected)value^=0x80000000u;
            if(mask&(1u<<slot))views[slot]=SourceObjectAudioViewTu15{{std::bit_cast<float>(selected[0]),
                std::bit_cast<float>(selected[1]),std::bit_cast<float>(selected[2])},-1};
            if(lane==0) {
                memory.sound[0x8eu+slot*0x48u]=std::byte((mask>>slot)&1);
                for(unsigned axis=0;axis<3;++axis)Memory::store(memory.sound,0xb8+slot*0x48+axis*4,selected[axis]);
            }
        }
        const auto before=memory.sound;
        for(auto radius:radii)for(const auto &point:points) {
            for(unsigned axis=0;axis<3;++axis)Memory::store(memory.point,axis*4,point[axis]);
            SourcePhysicsContactRuntimeAbi abi{};abi.gp[4]=Memory::stack_address;
            abi.gp[6]=Memory::point_address;abi.xmm[3][0]=radius;abi.eflags=2;
            source_run_object_loop_audio_tu15(memory,0x190460,abi,{0x27f,0x1f80},{},{},lane?&views:nullptr);
            results[lane].push_back(abi.gp[0]&255u);
            need(memory.sound==before,"Native views changed the legacy sound/listener/readiness bank");
        }
    }
    for(const auto &lane:results)need(lane.size()==fixture.at("cases").get<unsigned>() &&
        digest(lane)==fixture.at("results_sha256").get<std::string>(),
        "Native camera audibility differs from the complete original190460 CPU owner");
    const SourceObjectAudioViewsTu15 empty;
    need(!source_object_audio_view_operand_tu15(&empty,Memory::sound_address,0x190475,
        Memory::sound_address+0x78,1),"Camera binding admitted a source readiness operand");
    need(!source_object_audio_view_operand_tu15(&empty,Memory::sound_address,0x220c94,
        Memory::sound_address+0x8e,1),"Camera binding escaped its seven original input loads");
    need(!source_object_audio_view_operand_tu15(nullptr,Memory::sound_address,0x190475,
        Memory::sound_address+0x8e,1),"Unbound source bank acquired a native view");
    std::cout<<"PASS "<<results[0].size()<<" original audibility cases in original and native camera lanes\n";
    return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
