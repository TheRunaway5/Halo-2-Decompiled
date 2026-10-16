#include "halo2/source_particle_constant_storage_tu15.hpp"
#include "halo2/source_effect_particle_draw_tu15.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>

using namespace halo2;
using Json=nlohmann::json;
namespace {
Json state(const SourcePhysicsWorldRuntimeAbi &a) {
    return Json{{"gp",a.gp},{"xmm",a.xmm},{"eflags",a.eflags},{"fs_base",a.fs_base},
        {"x87_depth",a.x87_depth},{"x87_status",a.x87_status},
        {"x87_control",a.floating_point->x87_control_word},{"mxcsr",a.floating_point->mxcsr},
        {"active_mmx",bool(a.active_mmx)}};
}
void export_bank(const std::filesystem::path &p,const SourceRuntimeMemory &source,
    const SourceParticleConstantStorageTu15 &storage) {
    std::ofstream out(p,std::ios::binary);
    const auto region=[&](std::uint32_t address,std::span<const std::byte> bytes) {
        const auto size=std::uint32_t(bytes.size());out.write(reinterpret_cast<const char*>(&address),4);
        out.write(reinterpret_cast<const char*>(&size),4);out.write(reinterpret_cast<const char*>(bytes.data()),size);
    };
    source.visit_committed(region);storage.visit_committed(region);
    if(!out)throw FormatError("Cannot export actual native constant storage");
}
}
int main(int argc,char **argv)try {
    if(argc>2)throw FormatError("Optional argument: new diagnostic output directory");
    const auto output=argc==2?std::filesystem::path(argv[1]):std::filesystem::path{};
    if(!output.empty()) {
        if(std::filesystem::exists(output))throw FormatError("Preserve earlier constant-storage evidence");
        std::filesystem::create_directories(output);
    }
    SourceRuntimeMemory memory;
    memory.mount(0x60000000,std::vector<std::byte>(65536),SourceRuntimeMemory::Access::ReadWrite,
        "declared native SDK constant-call stack and input");
    SourceParticleConstantStorageTu15 storage;
    std::array<ShaderVector,192> inherited{},payload{};
    for(unsigned n=0;n<192;++n)for(unsigned k=0;k<4;++k) {
        inherited[n][k]=std::bit_cast<float>(0x3e000000u+n*4+k);
        // Preserve raw signed-zero, infinity and NaN payloads. Neither SDK
        // emission nor native method consumption performs float arithmetic.
        constexpr std::array<std::uint32_t,8> special{0,0x80000000,0x7f800000,0xff800000,
            0x7fc12345,0xffc54321,0x3f800000,0x00000001};
        payload[n][k]=std::bit_cast<float>(special[(n*4+k)%special.size()]);
    }
    const std::array<std::array<unsigned,3>,8> cases{{
        {0x4012b0,0,4},{0x4012b0,15,4},{0x4012b0,191,4},
        {0x401410,15,4},{0x401410,174,12},{0x401410,0,16},
        {0x401410,7,20},{0x401410,0,768}}};
    for(unsigned n=0;n<cases.size();++n) {
        const auto [entry,first,count]=cases[n];storage.begin(memory,inherited);
        memory.write(0x60000100,std::as_bytes(std::span(payload)));
        const std::array<unsigned,2> stack{0x25d25,count};memory.write(0x60008000,std::as_bytes(std::span(stack)));
        SourcePhysicsWorldRuntimeAbi a{};a.gp={0x12345678,first,0x60000100,0x89abcdef,
            0x60008000,0x76543210,0x13572468,0x24681357};a.eflags=0x202;
        a.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
        const auto before=a;std::vector<unsigned> instructions;
        SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto p){instructions.push_back(p);};
        services.required_owner=[](auto &,const auto &,auto &,const auto &) {
            throw FormatError("Constant emission exhausted its native storage; no SDK return supplied");
        };
        const auto path=output/std::to_string(n);
        if(!output.empty()) {std::filesystem::create_directory(path);export_bank(path/"before.bin",memory,storage);}
        source_run_effect_particle_draw_tu15(storage,entry,a,*a.floating_point,services);
        std::array<ShaderVector,192> consumed{};storage.consume(consumed);
        auto expected=inherited;std::memcpy(expected.data()+first,payload.data(),count*4);
        if(std::memcmp(expected.data(),consumed.data(),sizeof(expected)))
            throw FormatError("Original constant packets did not reach their native consumer bit-exactly");
        auto shadow=inherited;
        if(entry==0x4012b0)std::memcpy(shadow.data()+first,payload.data(),16);
        std::array<ShaderVector,192> actual_shadow{};storage.read(0x411818,std::as_writable_bytes(std::span(actual_shadow)));
        if(std::memcmp(shadow.data(),actual_shadow.data(),sizeof(shadow)))
            throw FormatError("Single/bulk setter shadow ownership differs");
        if(a.gp[4]!=before.gp[4]+(entry==0x401410?8:4) || a.gp[3]!=before.gp[3] ||
            a.gp[5]!=before.gp[5] || a.gp[6]!=before.gp[6] || a.gp[7]!=before.gp[7])
            throw FormatError("Original SDK setter did not preserve its nonvolatile/stack contract");
        if(!output.empty()) {
            export_bank(path/"after.bin",memory,storage);
            std::ofstream(path/"case.json")<<Json{{"entry",entry},{"first",first},{"dwords",count},
                {"before",state(before)},{"after",state(a)},{"instructions",instructions},
                {"caller_history","Declared native SDK interface; no original boot history claimed"},
                {"gpu_returns_supplied",Json::array()},{"gpu_submission",false}}.dump(2)<<'\n';
        }
    }
    // Invalid packets must leave the GPU consumer's whole destination intact.
    const auto malformed=[&](unsigned address,unsigned value) {
        storage.begin(memory,inherited);
        const std::array<unsigned,2> end{0xf7700008,0xf770ffbc};
        storage.write(0x40f0c0,std::as_bytes(std::span(end)));
        const std::array<unsigned,2> packet{0x41ea4,15};storage.write(0xf7700000,std::as_bytes(std::span(packet)));
        storage.write(address,std::as_bytes(std::span(&value,1)));
        auto unchanged=payload;bool rejected=false;
        try {storage.consume(unchanged);}catch(const FormatError &){rejected=true;}
        if(!rejected || std::memcmp(unchanged.data(),payload.data(),sizeof(payload)))
            throw FormatError("Invalid constant packet was accepted or partially published");
    };
    malformed(0xf7700000,0x81ea4); // Truncated two-word payload.
    malformed(0xf7700004,192);     // Outside the admitted register bank.
    malformed(0xf7700000,0x41b00); // A texture method has another owner.
    malformed(0x40f0c0,0xf7710004);// Cursor outside the actual allocation.
    storage.begin(memory,inherited);
    std::array<std::byte,8> crossing{};bool rejected=false;
    try{storage.read(0x40f0c4,crossing);}catch(const FormatError &){rejected=true;}
    if(!rejected)throw FormatError("A partial constant-owner read escaped into the source bank");
    SourceRuntimeMemory other;rejected=false;
    try{storage.begin(other,inherited);}catch(const FormatError &){rejected=true;}
    if(!rejected)throw FormatError("A constant-owner lease crossed native worlds");
    std::cout<<"PASS8 complete original constant setters, raw-bit native consumption, shadow/ABI contracts,4 atomic packet rejections and ownership boundaries\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
