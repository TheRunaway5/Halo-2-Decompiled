#include "halo2/source_campaign_camera_tu15.hpp"
#include "halo2/campaign_source_runtime.hpp"
#include "halo2/simulation.hpp"
#include "halo2/cache.hpp"
#include "halo2/original_data.hpp"
#include <bit>
#include <iostream>
#include <map>
#include <fstream>
#include <cstdlib>
using namespace halo2;
namespace {
void check(bool value,const char *reason) {if(!value)throw FormatError(reason);}
struct Program {
    ScriptProgram p;
    std::uint32_t node(ScriptNode n) {
        const auto i=std::uint32_t(p.nodes.size());n.salt=std::uint16_t(0xe000+i);
        p.nodes.push_back(n);return (std::uint32_t(n.salt)<<16)|i;
    }
    std::uint32_t lit(std::uint32_t bits,unsigned type) {
        return node({0,std::uint16_t(type),std::uint16_t(type),9,null_datum,0,bits});
    }
    std::uint32_t global(unsigned i) {return node({0,7,7,13,null_datum,0,i});}
    std::uint32_t call(std::string name,std::vector<std::uint32_t> args={},unsigned type=4) {
        const auto at=std::uint32_t(p.strings.size());p.strings+=name;p.strings+='\0';
        for(unsigned i=0;i<args.size();++i)p.nodes[args[i]&65535u].next=i+1<args.size()?args[i+1]:null_datum;
        const auto function=node({0,0,2,9,args.empty()?null_datum:args[0],at,0});
        return node({0,0,std::uint16_t(type),8,null_datum,at,function});
    }
    Program() {
        p.globals.push_back({"zero_time",7,lit(0,7)});
        p.globals.push_back({"moving_time",7,lit(0,7)});
        // Actual03b c03_intra1_scene_02b point/duration operands. The program
        // deliberately isolates the host boundary; no full scene is claimed.
        const auto root=call("begin",{
            call("camera_control",{lit(1,5)}),
            call("camera_set",{lit(26,15),lit(0xffff0000u,7)}),
            call("set",{global(0),call("camera_time",{},7)},7),
            call("camera_set",{lit(27,15),lit(0xffff0105u,7)}),
            call("set",{global(1),call("camera_time",{},7)},7)});
        p.scripts.push_back({"point_adoption",0,4,root});
    }
};
std::vector<std::byte> copy(const SourceRuntimeMemory &m,std::uint32_t at,std::size_t n) {
    const auto b=m.view(at,n);return {b.begin(),b.end()};
}
template<class T>void write(std::ofstream &out,const T &v) {
    out.write(reinterpret_cast<const char *>(&v),sizeof(v));
}
void export_root(PhysicsLiveWorldTu15 &world,const std::filesystem::path &path) {
    check(!std::filesystem::exists(path),"preserve original point root evidence");
    std::filesystem::create_directories(path);auto &memory=world.memory();
    auto abi=world.prepare_source_call(std::array{120u,null_datum});abi.gp[2]=26;
    const auto dump=[&](const char *name) {
        std::ofstream out(path/name,std::ios::binary);
        memory.visit_committed([&](auto address,auto bytes){
            write(out,address);write(out,std::uint32_t(bytes.size()));
            out.write(reinterpret_cast<const char *>(bytes.data()),std::streamsize(bytes.size()));
        });check(bool(out),"point root source bank export failed");
    };
    const auto state=[&](const char *name) {
        std::ofstream out(path/name,std::ios::binary);
        write(out,abi.gp);write(out,abi.xmm);write(out,abi.eflags);write(out,abi.fs_base);
        write(out,abi.x87_depth);write(out,abi.x87_status);
        check(abi.floating_point.has_value()&&abi.x87_depth==0,"point root caller needs actual empty x87 frame");
        write(out,abi.floating_point->x87_control_word);write(out,abi.floating_point->mxcsr);
        check(bool(out),"point root ABI export failed");
    };
    dump("before.bin");state("input.bin");
    std::ofstream stores(path/"native-stores.bin",std::ios::binary);
    world.run_actual(0x16f150u,abi,[&](const SourcePhysicsContactRecordStore &s){
        if(s.address>=0x60000000u&&s.address<0x60200000u)return;
        write(stores,s.instruction);write(stores,s.address);write(stores,std::uint32_t(s.written.size()));
        stores.write(reinterpret_cast<const char *>(s.before.data()),std::streamsize(s.before.size()));
        stores.write(reinterpret_cast<const char *>(s.written.data()),std::streamsize(s.written.size()));
    });check(bool(stores),"point root ordered store export failed");stores.close();
    dump("native-after.bin");state("native-exit.bin");
}
}
int main(int argc,char **argv)try {
    if(argc!=3)throw FormatError("Supply TU15_XBE 03b_newmombasa_MAP");
    if(!std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    auto content=cache.load_content(0,false);
    check(content.camera_points.size()==28&&content.camera_points[26].name=="c03_intra1_01"&&
          content.camera_points[27].name=="c03_intra1_02","authored03b camera identity differs");
    content.objects.clear();content.scripts=Program{}.p;
    auto source=std::make_shared<CampaignSourceRuntime>(cache,std::filesystem::path(argv[1]),0,1,0x12345678);
    Simulation simulation(content,0,cache.spawns().front(),{},{},{},{},{},{},source);
    auto &world=source->objects().tu15_world();auto &m=world.memory();
    const auto word=[&](std::uint32_t at){return Bytes(m.view(at,4)).u32(0);};
    const auto users=word(0x4f54f0u);
    check(users&&word(users+12u)!=null_datum,"point test lacks an actual registered local source user");
    const auto real_player=source->hud_player(simulation).source_player;
    check(word(users+12u)==real_player,"point test active source user is not its actual player");
    const auto observer_before=copy(m,0x4f64a4u,0x358);
    std::map<std::uint32_t,unsigned> stores;
    world.observe_physics_stores([&](std::uint32_t,const SourcePhysicsContactRecordStore &s){
        if(s.instruction>=0x16f150u&&s.instruction<0x174c00u)++stores[s.instruction];
    });
    simulation.start_level();simulation.step({});
    world.observe_physics_stores({});
    for(const auto &d:simulation.scripts().diagnostics())std::cerr<<"SCRIPT "<<d<<'\n';
    check(simulation.scripts().diagnostics().empty(),"source point host faulted before complete observer flush");
    check(simulation.scripts().globals()[0].number()==0&&simulation.scripts().globals()[1].number()==261,
          "camera_set did not adopt source timer before the following actual camera_time host");
    const auto director=word(0x51df6cu);
    check(Bytes(m.view(director+2u,2)).u16(0)==1u&&Bytes(m.view(director+0x40u,2)).u16(0)==27u&&
          Bytes(m.view(director+0x42u,2)).u16(0)==0u,"source point mode/index/type were not adopted");
    check(word(director+0x3cu)==null_datum,"ordinary camera_set retained a relative object");
    const auto &point=content.camera_points[27];
    check(word(director+0x18u)==std::bit_cast<std::uint32_t>(point.position.x)&&
          word(director+0x1cu)==std::bit_cast<std::uint32_t>(point.position.y)&&
          word(director+0x20u)==std::bit_cast<std::uint32_t>(point.position.z),"source point position is not authored03b data");
    check(source_campaign_camera_time_tu15(world)==260,"source point did not retain one actual completed game tick");
    check(stores[0x16f188u]==2&&stores[0x172131u]>=2,
          "source point passed only an inactive-user observer early return");
    unsigned adopt{},output{};
    for(const auto [pc,n]:stores) {
        if(pc>=0x1723d0u&&pc<0x172cf0u)adopt+=n;
        if(pc>=0x173e30u&&pc<0x174690u)output+=n;
    }
    check(adopt&&output&&observer_before!=copy(m,0x4f64a4u,0x358),
          "source point did not complete active original observer adoption and output owners");
    const auto director_after=copy(m,director,0x78),observer_after=copy(m,0x4f64a4u,0x358);
    source_campaign_camera_point_tu15(world,-1,7);
    source_campaign_camera_point_tu15(world,28,7);
    check(director_after==copy(m,director,0x78)&&observer_after==copy(m,0x4f64a4u,0x358),
          "original invalid point admission changed camera/observer banks");
    if(const auto *path=std::getenv("HALO2_POINT_CAMERA_EXPORT_ROOT"))export_root(world,path);
    // Presentation rejects unsupported authored modes before source adoption.
    // Restart through the actual host; an early16F150 call would write its mode.
    unsigned unsupported_mode_stores{};
    content.camera_points[26].type=1;
    world.observe_physics_stores([&](std::uint32_t,const SourcePhysicsContactRecordStore &s){
        if(s.instruction==0x16f188u)++unsupported_mode_stores;
    });
    simulation.start_level();simulation.step({});world.observe_physics_stores({});
    check(unsupported_mode_stores==0,"unsupported host camera mode mutated the original source director");
    check(simulation.scripts().diagnostics().size()==1&&
          simulation.scripts().diagnostics().front()=="point_adoption: builtin 'camera_set': relative/dolly camera point unsupported",
          "unsupported camera mode did not fail at the existing Presentation boundary");
    std::cout<<"PASS real03b player source host timer0/261, mode/index/type/authored pose, one tick260; active observer stores="
             <<stores[0x172131u]<<" adopt="<<adopt<<" output="<<output<<" invalid-index unchanged; unsupported host mode rejected before source mutation\n";
    std::cout<<"LIMIT bounded normal-point host/adoption only; see separate original differential receipt; full scene/render pose and relative/dolly support remain open\n";
}catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
