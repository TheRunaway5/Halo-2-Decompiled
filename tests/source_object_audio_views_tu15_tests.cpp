#include "halo2/source_object_loop_audio_tu15.hpp"
#include "halo2/source_object_loop_emitter_tu15.hpp"
#include "halo2/multiplayer_audio.hpp"
#include "native_tu15_sound_test_support.hpp"
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
void cluster_selector() {
    // Declared component inputs to the complete compiled253070 kernel;
    // these allocations are test operands, never a production source World.
    sound_test::Memory memory;
    constexpr std::uint32_t structure=0x1000000,table=0x1100000,cells=0x1200000,stack=0x1300100;
    memory.map(structure,0x230);memory.map(table,0x30);memory.map(cells,4096);memory.map(stack-8,16);
    memory.put32(structure+0x9c,4);memory.put32(structure+0xe8,cells);memory.put32(structure+0x228,table);
    memory.put32(table+0x28,cells);memory.putf(table+4,-3.25f);memory.putf(table+8,817.5f);
    MultiplayerAudioBsp bsp;bsp.clusters.resize(4);bsp.pair_distances.resize(6);
    bsp.distance_lower=-3.25f;bsp.distance_upper=817.5f;
    for(unsigned encoded=0;encoded<2;++encoded)for(unsigned cell=0;cell<256;++cell) {
        bsp.encoded_distances=bool(encoded);memory.put32(structure+0x224,encoded);
        std::fill(bsp.pair_distances.begin(),bsp.pair_distances.end(),std::uint8_t(cell));
        for(unsigned index=0;index<6;++index)memory.put8(cells+index,std::uint8_t(cell));
        for(unsigned first=0;first<4;++first)for(unsigned second=0;second<4;++second) {
            SourcePhysicsContactRuntimeAbi abi{};abi.gp[0]=first;abi.gp[1]=second;abi.gp[2]=structure;abi.gp[4]=stack;
            memory.put32(stack,0x60300000);
            native_tu15_sound_manager_run(memory,0x253070,abi,{0x27f,0x1f80});
            need(abi.gp[4]==stack+4,"Native253070 comparison changed its return ABI");
            need(abi.xmm[0][0]==std::bit_cast<std::uint32_t>(native_object_loop_cluster_scale_tu15(bsp,first,second)),
                "Replicated native PAS scale differs from complete253070");
        }
    }
    SourceObjectLoopEmitterTu15 emitter;emitter.loop=0x10003;emitter.object=0x20010;emitter.definition=0x30020;
    emitter.cluster=1;emitter.maximum_distance=2;emitter.function_active=true;
    emitter.location[3]=std::byte{1};Memory::store(emitter.location,4,std::bit_cast<std::uint32_t>(1.f));
    emitter.location[0x2c]=std::byte{1};
    SourceObjectAudioViewsTu15 views;views[0]=SourceObjectAudioViewTu15{{2,0,0},1};
    const std::array bank{emitter};
    auto requests=native_object_loop_requests_tu15(bank,bsp,views);
    need(requests.size()==1 && requests[0].arguments[2]==emitter.loop && requests[0].emitter_owner==emitter.object,
        "Native client loop lost exact-boundary audibility or full source identity");
    views[0]->position.x=std::nextafter(2.f,3.f);
    need(native_object_loop_requests_tu15(bank,bsp,views).empty(),"Native client distance admitted outside its actual boundary");
    views[0]->position={0,0,0};views[0]->cluster=0;bsp.pair_distances.assign(6,255);
    need(native_object_loop_requests_tu15(bank,bsp,views).empty(),"Native client PAS admitted the full256 cutoff");
    views[0]->cluster=1;emitter.function_active=false;emitter.stop_duration=.01f;
    requests=native_object_loop_requests_tu15(std::span(&emitter,1),bsp,views);
    need(requests.size()==1 && requests[0].arguments[5]==2 &&
        requests[0].arguments[7]==std::bit_cast<std::uint32_t>(.01f),"Native client inactive function lost its exact stop duration");
    need(native_object_loop_requests_tu15({},bsp,views).empty(),"Empty source emitter publication retained a request");
    auto duplicate=std::array{emitter,emitter};bool rejected{};
    try{validate_source_object_loop_emitters_tu15(duplicate);}catch(const FormatError&){rejected=true;}
    need(rejected,"Emitter publication accepted a duplicate source slot");
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
    Memory memory;std::array<std::vector<unsigned char>,3> results;
    for(unsigned lane=0;lane<3;++lane)for(unsigned variation=0;variation<8;++variation)
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
            if(lane==2)results[lane].push_back(native_object_loop_within_distance_tu15(
                {std::bit_cast<float>(point[0]),std::bit_cast<float>(point[1]),std::bit_cast<float>(point[2])},
                std::bit_cast<float>(radius),views));
            else {
                SourcePhysicsContactRuntimeAbi abi{};abi.gp[4]=Memory::stack_address;
                abi.gp[6]=Memory::point_address;abi.xmm[3][0]=radius;abi.eflags=2;
                source_run_object_loop_audio_tu15(memory,0x190460,abi,{0x27f,0x1f80},{},{},lane?&views:nullptr);
                results[lane].push_back(abi.gp[0]&255u);
            }
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
    cluster_selector();
    std::cout<<"PASS "<<results[0].size()<<" original audibility cases in source, native-camera and native-replica lanes;8192 complete253070 comparisons\n";
    return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
