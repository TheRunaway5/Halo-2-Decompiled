#include "source_script_wake_fixture.hpp"
#include <algorithm>

using namespace halo2::script_wake_fixture;
namespace {
ScriptProgram original_program(const Cache &cache) {
    if(cache.is_mcc() || cache.is_vista())throw FormatError("Cue fixture requires the original Xbox map");
    const Bytes scenario(cache.tag_payload(cache.scenario_tag()));ScriptProgram p;
    const auto strings=cache.read_tag_bytes(scenario.u32(0x1b4),scenario.u32(0x1b0));
    p.strings.assign(reinterpret_cast<const char *>(strings.data()),strings.size());
    const auto count=scenario.u32(0x238),script_count=scenario.u32(0x1b8);
    const Bytes nodes(cache.read_tag_bytes(scenario.u32(0x23c),std::size_t(count)*20));
    const Bytes scripts(cache.read_tag_bytes(scenario.u32(0x1bc),std::size_t(script_count)*40));
    for(unsigned i=0;i<count;++i){const auto a=i*20;p.nodes.push_back({nodes.u16(a),nodes.u16(a+2),nodes.u16(a+4),nodes.u16(a+6),nodes.u32(a+8),nodes.u32(a+12),nodes.u32(a+16)});}
    for(unsigned i=0;i<script_count;++i){const auto a=i*40;p.scripts.push_back({scripts.string(a,32),scripts.u16(a+32),scripts.u16(a+34),scripts.u32(a+36)});}
    return p;
}
}
int main(int argc,char **argv) {
    try {
        if(argc!=3)throw std::runtime_error("expected fixture directory and original introduction map");
        if(!std::filesystem::exists(argv[2]))return 77;
        const std::filesystem::path root=argv[1];Cache cache(argv[2]);auto p=original_program(cache);
        std::vector<std::pair<unsigned,std::uint32_t>> observed,expected;unsigned tick{};
        ScriptVm vm(p,[&](const auto &name,const auto &args)->ScriptValue {
            if(name=="sound_impulse_start")observed.emplace_back(tick,args.at(0).bits);
            else if(name!="print" && name!="cinematic_subtitle" && name!="unit_set_emotional_state")
                throw std::runtime_error("undeclared non-timing boundary: "+name);
            return {};
        },[](int,unsigned){return null_datum;});
        std::ifstream names(root/"opening49.names");std::string name;unsigned selected{};
        while(names>>name){vm.start(name);++selected;}
        if(selected!=49)throw std::runtime_error("actual 49 cue selection differs");
        for(tick=0;tick<=750;++tick)vm.tick(tick);
        if(!vm.diagnostics().empty())throw std::runtime_error(vm.diagnostics().front());
        std::ifstream f(root/"opening49.events");unsigned t{};std::uint32_t datum{};
        while(f>>t>>datum)expected.emplace_back(t,datum);
        std::sort(observed.begin(),observed.end());
        if(observed!=expected || observed.size()!=49)throw std::runtime_error("actual opening cue wake-relative requests differ");
        std::cout<<"PASS actual 49 original cue programs match unchanged TU1.5 wake-relative ticks and sound datums; no voice/full-scene scheduling claim\n";
    }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
