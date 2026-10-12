#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/cache.hpp"
#include "halo2/source_game_variant.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <unistd.h>
#include <xmmintrin.h>

namespace {
void check(bool ok,const char*message) {if(!ok)throw std::runtime_error(message);}
std::uint16_t control_word() {std::uint16_t v;asm volatile("fnstcw %0":"=m"(v));return v;}
std::uint16_t exception_flags() {std::uint16_t v;asm volatile("fnstsw %0":"=a"(v));return v&0x3f;}
void dump(halo2::SourceRuntimeMemory&m,const std::filesystem::path&p) {
    std::ofstream out(p,std::ios::binary);
    m.visit_committed([&](auto address,auto bytes) {
        const auto n=std::uint32_t(bytes.size());
        out.write(reinterpret_cast<const char*>(&address),4);
        out.write(reinterpret_cast<const char*>(&n),4);
        out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
    });
    check(bool(out),"Native frame bank export failed");
}
void dump_abi(const halo2::SourcePhysicsWorldRuntimeAbi&a,const std::filesystem::path&p) {
    std::ofstream out(p,std::ios::binary);
    const auto write=[&](const auto&v){out.write(reinterpret_cast<const char*>(&v),sizeof(v));};
    write(a.gp);write(a.xmm);write(a.eflags);write(a.fs_base);write(a.x87_depth);write(a.x87_status);
    const auto fp=a.floating_point.value();write(fp.x87_control_word);write(fp.mxcsr);
    check(bool(out),"Native frame ABI export failed");
}
}
int main(int argc,char**argv) {
    std::filesystem::path files;
    try {
        check(argc==3 || argc==4,"Supply owned TU1.5 and Xbox map, optionally a diagnostic directory");
        halo2::Cache cache(argv[2]);
        auto name=(std::filesystem::temp_directory_path()/"halo2-frame-tu15-XXXXXX").string();
        auto p=::mkdtemp(name.data());check(p!=nullptr,"Native frame temporary directory failed");files=p;
        std::size_t stores=0;
        {
            halo2::PhysicsLiveWorldTu15 world(cache,argv[1],files);
            bool rejected=false;
            try {world.step_physics();}catch(const halo2::FormatError&) {rejected=true;}
            check(rejected,"Physics frame admitted a missing source world/session");
            world.initialize_map();
            rejected=false;
            try {world.step_physics();}catch(const halo2::FormatError&) {rejected=true;}
            check(rejected,"Physics frame admitted a missing source session");
            world.configure_session(halo2::source_game_variant_default(0),1,30,0x12345678u);
            const auto source_world=world.world();
            check(source_world!=0,"Native frame lost the constructed source world");
            const auto cw=control_word(),exceptions=exception_flags();const auto mxcsr=_mm_getcsr();
            for(unsigned tick=0;tick<120;++tick) {
                std::ofstream trace;
                std::filesystem::path directory;
                if(argc==4 && tick<2) {
                    directory=std::filesystem::path(argv[3])/std::to_string(tick);
                    std::filesystem::create_directories(directory);
                    dump(world.memory(),directory/"before.bin");
                    trace.open(directory/"native-stores.bin",std::ios::binary);
                }
                const auto a=world.step_physics([&](const auto&s) {
                    ++stores;
                    if(trace.is_open()) {
                        const auto n=std::uint32_t(s.written.size());
                        trace.write(reinterpret_cast<const char*>(&s.instruction),4);
                        trace.write(reinterpret_cast<const char*>(&s.address),4);
                        trace.write(reinterpret_cast<const char*>(&n),4);
                        trace.write(reinterpret_cast<const char*>(s.before.data()),n);
                        trace.write(reinterpret_cast<const char*>(s.written.data()),n);
                    }
                });
                check(control_word()==cw && exception_flags()==exceptions && _mm_getcsr()==mxcsr,
                      "Original physics frame leaked its FP state into the native application");
                check(world.world()==source_world,"Original physics frame replaced its constructed world");
                if(trace.is_open()) {
                    check(bool(trace),"Native frame store export failed");trace.close();
                    dump(world.memory(),directory/"after.bin");dump_abi(a,directory/"native-exit.bin");
                }
            }
        }
        std::filesystem::remove_all(files);
        std::cout<<"PASS fresh native TU1.5 physics frame: 120 source frames, "<<stores
                 <<" ordered writes; world/session gates and FP restoration pass; no player or populated-world claim\n";
        return 0;
    }catch(const std::exception&e) {
        if(!files.empty())std::filesystem::remove_all(files);
        std::cerr<<"FAIL "<<e.what()<<'\n';return 1;
    }
}
