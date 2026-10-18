#include "halo2/source_campaign_environment_frame_tu15.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/source_execution_inputs.hpp"
#include "halo2/cache.hpp"
#include "halo2/original_data.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unistd.h>
using namespace halo2;using Json=nlohmann::json;
namespace {
void check(bool v,const char *why){if(!v)throw FormatError(why);}
std::vector<std::byte> unhex(const std::string &s){std::vector<std::byte> out;check(s.size()%2==0,"Odd fixture hex");for(unsigned n=0;n<s.size();n+=2)out.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return out;}

}
int main(int argc,char** argv)try {
    if(argc!=2 && argc!=4 && argc!=5 && argc!=6)throw FormatError("Supply CALLER_FIXTURE [TU15_XBE MAP [BSP]] or explicit FRESH_OUTPUT BSP");
    if(!std::filesystem::exists(argv[1]) || (argc>2 && (!std::filesystem::exists(argv[2]) || !std::filesystem::exists(argv[3]))))return 77;
    Json fixture;{std::ifstream in(argv[1]);in>>fixture;}
    check(fixture.at("xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Caller fixture authority differs");
    unsigned count{};for(const auto &row:fixture.at("cases")) {
        const auto input=unhex(row.at("input")),rgb=unhex(row.at("rgb")),expected=unhex(row.at("output"));
        check(input.size()==0x120 && expected.size()==input.size() && rgb.size()==12,"Caller fixture extent differs");
        std::array<std::byte,0x120> actual;std::copy(input.begin(),input.end(),actual.begin());
        const auto far=detail::source_campaign_environment_caller_tu15(actual,row.at("clear").get<std::uint8_t>(),
            std::span<const std::byte,12>(rgb),std::bit_cast<float>(row.at("near_bits").get<std::uint32_t>()),
            std::bit_cast<float>(row.at("far_bits").get<std::uint32_t>()));
        check(std::equal(actual.begin(),actual.end(),expected.begin()),"Original caller whole environment mismatch");
        check(std::bit_cast<std::uint32_t>(far)==row.at("far_after_bits").get<std::uint32_t>(),"Original caller exact far-plane mismatch");++count;
    }
    std::cout<<"PASS "<<count<<" actual original caller full-bank/RGB/far controls\n"<<std::flush;
    if(argc==2)return 0;
    Cache cache(argv[3],std::filesystem::path(argv[3]).parent_path(),OriginalData::load_xbe(argv[2]));
    auto path=(std::filesystem::temp_directory_path()/"halo2-env-publication-XXXXXX").string();check(::mkdtemp(path.data()),"Probe storage unavailable");
    struct Cleanup{std::string path;~Cleanup(){std::error_code ec;std::filesystem::remove_all(path,ec);}}cleanup{path};
    const bool explicit_output=argc==6;
    const auto output=explicit_output?std::filesystem::path(argv[4]):std::filesystem::path(path)/"probe-output";
    check(!std::filesystem::exists(output),"Output must be fresh");std::filesystem::create_directories(output);
    const unsigned selected=argc==6?std::stoul(argv[5]):argc==5?std::stoul(argv[4]):0u;
    MultiplayerObjectRuntime objects(cache,selected,path);auto &world=objects.tu15_world();world.initialize_map();world.configure_campaign_session(1,1,30,0x12345678u);
    auto &m=world.memory();const auto word=[&](std::uint32_t p){return Bytes(m.view(p,4)).u32(0);};
    const auto bsp=word(0x4ecc18u),clusters=word(bsp+0x9cu),rows=word(bsp+0xa0u);
    Json report{{"authority","OG Xbox TU1.5"},{"caller_cases",count},{"world_xbe_path_supplied",false},{"bsp",selected},{"cases",Json::array()}};
    struct Case{std::string name;Vec3 eye,forward;std::int32_t cluster;std::string fixture_frame;};
    std::vector<Case> cases;
    if(std::filesystem::path(argv[3]).filename()=="00a_introduction.map" && !selected) {
        cases.push_back({"fleet",{349.011566162109375f,340.078338623046875f,52.2694091796875f},{.5321619510650635f,.8273915648460388f,.17951834201812744f},27,"fleet"});
        cases.push_back({"destruction",{353.634918212890625f,294.381256103515625f,24.961688995361328125f},{-.22934174537658691f,-.9558929204940796f,-.18349668383598328f},27,"destruction"});
    }
    cases.push_back({"none",{}, {1,0,0},-1,{}});
    for(unsigned n=0;n<clusters;++n) {
        const Bytes row(m.view(rows+n*176u,176u));
        cases.push_back({"cluster-"+std::to_string(n),{(row.f32(0x54)+row.f32(0x58))*.5f,(row.f32(0x5c)+row.f32(0x60))*.5f,(row.f32(0x64)+row.f32(0x68))*.5f},{1,0,0},std::int32_t(n),{}});
    }
    const auto unchanged=m.checkpoint();
    for(const auto bad:std::array<std::int32_t,2>{-2,std::int32_t(clusters)}) {
        bool rejected{};try{(void)source_campaign_environment_frame_tu15(world,{}, {1,0,0},bad,.015625f,1024.f);}catch(const FormatError &){rejected=true;}
        check(rejected && m.checkpoint()==unchanged,"Invalid environment cluster mutated source state");
    }
    const auto admitted=objects.authority_checkpoint();
    unsigned topology_rejections{};
    for(const auto required:std::array<std::uint32_t,6>{0x4f2d88u,0x4f2d98u,0x4f3008u,0x556554u,0x556e10u,0x4c6710u}) {
        for(unsigned mutation=0;mutation<6;++mutation) {
            auto invalid=admitted;
            const auto row=std::ranges::find(invalid.world.bank.regions,required,&SourceRuntimeMemory::CheckpointRegion::address);
            check(row!=invalid.world.bank.regions.end(),"Native environment cold destination lacks its complete owner");
            if(mutation==0)invalid.world.bank.regions.erase(row);
            else if(mutation==1)row->bytes.pop_back();
            else if(mutation==2)row->access=SourceRuntimeMemory::Access::ReadOnly;
            else if(mutation==3)row->owner+=" changed";
            else if(mutation==4)row->page_bytes=4096;
            else row->committed.push_back(1);
            bool rejected{};std::string diagnostic;
            try{(void)objects.prepare_authority(invalid);}catch(const FormatError &e){rejected=true;diagnostic=e.what();}
            check(m.checkpoint()==unchanged,"Rejected environment topology changed live source bytes");
            check(rejected,"Public authority restore accepted missing/wrong required environment storage");
            check(diagnostic=="TU1.5 authority lacks its required complete environment owner","Environment topology rejection occurred after its required-owner guard");
            ++topology_rejections;
        }
    }
    unsigned overrides{},clamps{},journal_cases{},sky_none{},sky_no_model{};
    for(const auto &c:cases) {
        const auto before=objects.authority_checkpoint();SourceExecutionInputs capture;capture.track_calls();
        SourceCampaignEnvironmentFrameTu15 frame;{SourceExecutionInputBinding binding(m,capture);frame=source_campaign_environment_frame_tu15(world,c.eye,c.forward,c.cluster,.015625f,1024.f);}
        std::vector<SourceExecutionInput> inputs;{const auto x=capture.finish();inputs.assign(x.begin(),x.end());}
        std::vector<SourceExecutionCall> calls;{const auto x=capture.calls();calls.assign(x.begin(),x.end());}
        check(std::equal(frame.bytes.begin(),frame.bytes.end(),m.view(0x4c6710u,0x120u).begin()),"Main caller omitted environment bytes");
        if(!c.fixture_frame.empty()) {const auto expected=unhex(fixture.at("genuine_main_frames").at(c.fixture_frame).at("bytes_hex"));check(expected.size()==0x120 && std::equal(expected.begin(),expected.end(),frame.bytes.begin()),"Genuine completed original full environment differs");}
        unsigned publications{};std::vector<std::uint32_t> entries;
        for(const auto &call:calls) {
            const Bytes args(call.arguments);
            if(call.kind==SourceExecutionCallKind::RunActualSourceCall)entries.push_back(args.u32(0));
            if(call.kind==SourceExecutionCallKind::WriteSourceCallInput && args.u32(0)==0x4c6710u) {++publications;check(args.u32(4)==0x120u,"Full publication journal extent differs");}
        }
        check(entries==std::vector<std::uint32_t>{0x14de90u,0x1310f0u},"Environment resolver/producer order differs");check(publications==1,"Full environment publication omitted or duplicated");
        const auto after=m.checkpoint();check(objects.commit_authority(objects.prepare_authority(before)),"Environment checkpoint restore failed");
        {SourceExecutionInputs replay(inputs,calls);{SourceExecutionInputBinding binding(m,replay);const auto repeated=source_campaign_environment_frame_tu15(world,c.eye,c.forward,c.cluster,.015625f,1024.f);check(repeated.bytes==frame.bytes && std::bit_cast<std::uint32_t>(repeated.far_distance)==std::bit_cast<std::uint32_t>(frame.far_distance),"Environment caller output changed in replay");}(void)replay.finish();}
        check(m.checkpoint()==after,"Environment publication journal changed complete native bank");++journal_cases;overrides+=bool(frame.clear_override);clamps+=frame.far_distance!=1024.f;sky_none+=frame.visible_sky==-1;sky_no_model+=!frame.sky_model_present;
        report["cases"].push_back({{"name",c.name},{"cluster",c.cluster},{"visible_sky",frame.visible_sky},{"clear_override",frame.clear_override},{"sky_model_present",frame.sky_model_present},{"far_bits",std::bit_cast<std::uint32_t>(frame.far_distance)},{"whole_bank_replay",true},{"genuine_original_bank",!c.fixture_frame.empty()}});
    }
    report["required_owner_topology_rejections_before_restore"]=topology_rejections;report["sky_none"]=sky_none;report["sky_no_model"]=sky_no_model;report["invalid_cluster_rejections_without_mutation"]=2;report["overrides"]=overrides;report["clamps"]=clamps;report["world_cases"]=journal_cases;
    std::ofstream(output/"report.json")<<report.dump(2)<<'\n';std::cout<<"PASS "<<journal_cases<<" ordinary native producer/publication complete-bank replay cases, overrides="<<overrides<<" clamps="<<clamps<<'\n';
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
