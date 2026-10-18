#include "halo2/source_campaign_environment_view_tu15.hpp"
#include "halo2/source_execution_inputs.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/original_data.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>
#include <unistd.h>
using namespace halo2;using Json=nlohmann::json;
namespace {
void check(bool value,const char *why){if(!value)throw FormatError(why);}
Vec3 vector(const Json &j){return {j[0],j[1],j[2]};}
std::vector<std::byte> unhex(const std::string &s){std::vector<std::byte> b;for(unsigned n=0;n<s.size();n+=2)b.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return b;}
}
int main(int argc,char **argv)try {
    if(argc!=5 && argc!=6)throw FormatError("Supply LOCATION_FIXTURE ENVIRONMENT_FIXTURE TU15_XBE OPENING_MAP [FRESH_OUTPUT]");
    for(unsigned n=1;n<=4;++n)if(!std::filesystem::is_regular_file(argv[n]))return 77;
    Json original,environment;{std::ifstream f(argv[1]);f>>original;}{std::ifstream f(argv[2]);f>>environment;}
    check(original.at("status")=="PASS_UNCHANGED_ORIGINAL_MAIN_LOCATION_QUERY_AND_FALLBACK" &&
        original.at("xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Original location authority differs");
    Cache cache(argv[4],std::filesystem::path(argv[4]).parent_path(),OriginalData::load_xbe(argv[3]));
    auto path=(std::filesystem::temp_directory_path()/"halo2-env-view-XXXXXX").string();check(::mkdtemp(path.data()),"Environment view storage unavailable");
    struct Cleanup{std::string path;~Cleanup(){std::error_code e;std::filesystem::remove_all(path,e);}}cleanup{path};
    const auto output=argc==6?std::filesystem::path(argv[5]):std::filesystem::path(path)/"evidence";
    check(!std::filesystem::exists(output),"Preserve environment view evidence");std::filesystem::create_directories(output);
    MultiplayerObjectRuntime objects(cache,0,path);auto &world=objects.tu15_world();
    world.initialize_map();world.configure_campaign_session(1,1,30,0x12345678u);
    const auto resources=cache.load_bsp_atmosphere(0);auto &memory=world.memory();
    const auto word=[&](std::uint32_t p){return Bytes(memory.view(p,4)).u32(0);};
    const auto put=[&](std::uint32_t p,std::int32_t value){world.write_source_call_input(p,std::as_bytes(std::span(&value,1)));};
    Json report{{"status","RUNNING"},{"world_constructor_xbe_path_supplied",false},{"cases",Json::array()}};
    for(const auto &c:original.at("cases")) {
        put(0x4c6874u,c.at("previous_cluster"));put(0x4c6884u,c.at("previous_leaf"));
        const auto before=objects.authority_checkpoint();SourceExecutionInputs capture;capture.track_calls();
        SourceCampaignEnvironmentViewTu15 frame;
        {SourceExecutionInputBinding binding(memory,capture);frame=source_campaign_environment_view_tu15(world,resources,vector(c.at("position")),{1,0,0},.015625f,1024.f);}
        std::vector<SourceExecutionInput> inputs;{const auto x=capture.finish();inputs.assign(x.begin(),x.end());}
        std::vector<SourceExecutionCall> calls;{const auto x=capture.calls();calls.assign(x.begin(),x.end());}
        check(frame.leaf==c.at("leaf") && frame.fog.cluster==c.at("cluster") && frame.location_found==c.at("location_found"),"Original completed main location differs");
        check(word(0x4c6874u)==std::bit_cast<std::uint32_t>(c.at("persistent_cluster").get<std::int32_t>()) &&
              word(0x4c6884u)==std::bit_cast<std::uint32_t>(c.at("persistent_leaf").get<std::int32_t>()),"Failed camera query changed persistent original history");
        check(std::ranges::equal(frame.source.bytes,memory.view(0x4c6710u,0x120u)),"Environment view lost its complete source publication");
        std::vector<std::uint32_t> entries;unsigned publications{};
        for(const auto &call:calls){const Bytes a(call.arguments);
            if(call.kind==SourceExecutionCallKind::RunActualSourceCall)entries.push_back(a.u32(0));
            if(call.kind==SourceExecutionCallKind::WriteSourceCallInput && a.u32(0)==0x4c6710u){++publications;check(a.u32(4)==0x120u,"Partial environment publication");}}
        check(entries==std::vector<std::uint32_t>{0x14cdb0u,0x14de90u,0x1310f0u} && publications==1,"Source location/resolver/producer/publication phase differs");
        // The two genuine bank captures use their actual submitting direction.
        // This test's explicit +X direction is a location-control input only.
        const auto after=memory.checkpoint();check(objects.commit_authority(objects.prepare_authority(before)),"Environment view checkpoint restore failed");
        {SourceExecutionInputs replay(inputs,calls);{SourceExecutionInputBinding binding(memory,replay);
            const auto repeated=source_campaign_environment_view_tu15(world,resources,vector(c.at("position")),{1,0,0},.015625f,1024.f);
            check(repeated.source.bytes==frame.source.bytes && repeated.leaf==frame.leaf && repeated.location_found==frame.location_found,"Environment view replay result differs");}(void)replay.finish();}
        check(memory.checkpoint()==after,"Environment view replay changed complete native memory");
        report["cases"].push_back({{"name",c.at("name")},{"leaf",frame.leaf},{"cluster",frame.fog.cluster},{"found",frame.location_found},{"complete_bank_replay",true}});
    }
    for(const auto &[name,eye,forward]:std::array<std::tuple<const char *,Vec3,Vec3>,2>{{
        {"fleet",{349.011566162109375f,340.078338623046875f,52.2694091796875f},{.5321619510650635f,.8273915648460388f,.17951834201812744f}},
        {"destruction",{353.634918212890625f,294.381256103515625f,24.961688995361328125f},{-.22934174537658691f,-.9558929204940796f,-.18349668383598328f}}}}) {
        const auto frame=source_campaign_environment_view_tu15(world,resources,eye,forward,.015625f,1024.f);
        const auto expected=unhex(environment.at("genuine_main_frames").at(name).at("bytes_hex"));
        check(std::ranges::equal(frame.source.bytes,expected),"Original genuine main environment differs after actual location query");
        const auto constants=bsp_fog_constants(frame.fog,eye,forward);const auto externs=bsp_fog_externs(frame.fog);
        std::ofstream(output/(std::string(name)+"-fog.json"))<<Json{{"constants",constants},{"scalars",externs.scalars},{"colors",externs.colors}}.dump(2)<<'\n';
    }
    report["status"]="PASS_ORIGINAL_LOCATION_FULL_SOURCE_ENVIRONMENT_AND_REPLAY";
    report["genuine_original_banks_after_actual_location"]=2;
    std::ofstream(output/"result.json")<<report.dump(2)<<'\n';
    std::cout<<"PASS "<<original.at("cases").size()<<" original location/history cases, full source publication and whole-bank replay; two genuine original banks after actual location\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
