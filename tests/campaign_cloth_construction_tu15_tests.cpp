#include "halo2/source_campaign_cinematic_tu15.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_physics_entry_observation.hpp"
#include "helpers/live_physics_entry_capture_tu15.hpp"
#include <cstdlib>
#include <iostream>
#include <unistd.h>
using namespace halo2;

int main(int argc,char **argv)try {
    if(argc!=3)throw FormatError("Supply TU15_XBE OPENING_MAP");
    if(!std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    const auto placements=[&] {
        const auto content=cache.load_content(0,false);std::vector<ScenarioObject> out;
        for(unsigned index:{8u,9u}) {
            const auto &name=content.object_names.at(index);
            const auto at=std::ranges::find_if(content.objects,[&](const auto &p){return p.type==name.type && p.placement==name.placement;});
            if(at==content.objects.end())throw FormatError("Authored scene10 Brute guard placement unavailable");out.push_back(*at);
        }
        return out;
    }();
    auto path=(std::filesystem::temp_directory_path()/"halo2-source-cloth-XXXXXX").string();
    if(!::mkdtemp(path.data()))throw FormatError("Source cloth storage unavailable");
    struct Cleanup{std::string path;~Cleanup(){std::error_code e;std::filesystem::remove_all(path,e);}}cleanup{path};
    MultiplayerObjectRuntime objects(argv[1],cache,0,path);auto &w=objects.tu15_world();auto &m=w.memory();
    w.initialize_map();w.configure_campaign_session(1,1,30,0x12345678);source_campaign_cinematic_control_tu15(w,true);
    const auto word=[&](auto at){return Bytes(m.view(at,4)).u32(0);};
    const auto pool=word(0x4ecc08u);
    if(!pool || std::to_integer<unsigned>(m.view(pool+0x29u,1)[0])!=1)
        throw FormatError("Original registered cloth storage was not constructed and activated");
    struct ClothCall{std::uint32_t tag,owner,instance=null_datum;};
    std::vector<ClothCall> calls;
    std::unique_ptr<test::LivePhysicsEntryCaptureTu15> capture;
    const auto capture_directory=std::getenv("HALO2_OPENING_CLOTH_CAPTURE");
    SourcePhysicsEntryObserverBinding observation(m,[&](bool entering,std::uint32_t entry,const auto &state) {
        if(entry==0x119060u && capture_directory) {
            if(entering)capture=std::make_unique<test::LivePhysicsEntryCaptureTu15>(
                std::filesystem::path(capture_directory)/("guard-"+std::to_string(calls.size())));
            capture->observe(entering,state,m,entry);
        }
        if(entry==0x119060u) {
            if(entering)calls.push_back({word(state.gp[4]+4u),word(state.gp[4]+8u)});
            else calls.back().instance=state.gp[0];
        }
    });
    // Actual x01_10_problem_actors creates names8 and9 before scene09 ends.
    // Their original attachment walker calls119060 with the authored flag;
    // the test supplies no cloth allocation, owner or geometry result.
    std::vector<std::uint32_t> owners;
    for(const auto &placement:placements) {
        std::cout<<"PHASE authored Brute guard birth\n"<<std::flush;
        const auto owner=objects.create_scenario_object(placement,SourceScenarioBirth::runtime);
        if(owner==null_datum)throw FormatError("Actual Brute guard birth returned NONE");owners.push_back(owner);
        std::cout<<"BIRTH owner="<<std::hex<<owner<<" definition="<<placement.definition<<" variant="<<placement.variant<<std::dec<<'\n';
    }
    unsigned flags{};
    for(const auto &call:calls) {
        if(call.tag!=0xe56103ecu)continue;
        std::cout<<"CLOTH owner="<<std::hex<<call.owner<<" instance="<<call.instance<<std::dec<<'\n'<<std::flush;
        if(call.instance!=null_datum || std::ranges::find(owners,call.owner)==owners.end())
            throw FormatError("Declared default guard variant differs from its original no-marker/free branch");
        ++flags;
    }
    // Untouched original119060 replays establish NONE for these declared
    // seed/default-variant inputs. The allocator must release both attempts.
    if(flags!=2 || word(word(pool+0x48u))!=0)
        throw FormatError("Both original no-marker guard flag attempts did not release their slots");
    std::cout<<"PASS two actual guard births/flag callbacks and original no-marker/free branch\n";
    std::cout<<"LIMIT authored guard births; no full scene VM, cloth animation or rendered cloth pixels\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
