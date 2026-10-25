#include "gameplay/multiplayer/source_objective_native_tu15_internal.hpp"
#include "halo2/source_ctf_activation_tu15.hpp"
#include "halo2/source_multiplayer_engines_tu15.hpp"
#include "ctf_activation_test_support.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <map>

using namespace halo2;
using Json=nlohmann::json;
namespace {
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::byte> bytes;
    std::array<std::byte,0x10000> bank{};
    void set(std::uint32_t at,std::uint32_t value,unsigned width) {
        for(unsigned n=0;n<width;++n) {
            const auto address=at+n;
            if(address>=0x4000000u && address<0x4010000u)bank[address-0x4000000u]=std::byte(value>>(n*8));
            else bytes[address]=std::byte(value>>(n*8));
        }
    }
    void read(std::uint32_t at,std::span<std::byte> to)const override {
        for(unsigned n=0;n<to.size();++n) {
            const auto address=at+n;
            to[n]=address>=0x4000000u && address<0x4010000u?bank[address-0x4000000u]:bytes.at(address);
        }
    }
    void write(std::uint32_t at,std::span<const std::byte> from)override {
        for(unsigned n=0;n<from.size();++n) {
            const auto address=at+n;
            if(address>=0x4000000u && address<0x4010000u)bank[address-0x4000000u]=from[n];
            else bytes.at(address)=from[n];
        }
    }
};
void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
struct RequiredOwnerBoundary {std::uint32_t entry;};
void real_owner(SourcePhysicsContactMemory& memory,std::uint32_t entry,SourcePhysicsWorldRuntimeAbi& abi,
    const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observe) {
    if(source_ctf_activation_tu15_owns_entry(entry))source_run_ctf_activation_tu15(memory,entry,abi,{0x27f,0x1f80},services,observe);
    else if(source_ctf_activation_test_support_owns_entry(entry))source_run_ctf_activation_test_support(memory,entry,abi,{0x27f,0x1f80},services,observe);
    else if(source_multiplayer_engines_tu15_owns_entry(entry))source_run_multiplayer_engines_tu15(memory,entry,abi,{0x27f,0x1f80},services,observe);
    else throw std::runtime_error("Unprovided actual objective test owner: "+std::to_string(entry));
}
}
int main(int argc,char** argv)try {
    need(argc==2 || argc==3,"Usage: objective_native_owner_tu15_tests ORIGINAL_FIXTURE [REPORT]");
    Json fixture;std::ifstream(argv[1])>>fixture;
    need(fixture.at("schema")=="H2_NATIVE_OBJECTIVE_OWNERS_V1" &&
        fixture.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
        fixture.at("supplied_service_results").empty(),"Invalid whole-owner original reference");
    unsigned count{},emulator_status_differences{},required_boundaries{};
    for(const auto& row:fixture.at("cases")) {
        try {
            physics_world_runtime_detail::FpScope scope({0x27f,0x1f80});
            Memory memory;
            for(const auto& patch:row.at("inputs"))memory.set(patch[0],patch[2],patch[1]);
            Json writes=Json::array();
            SourcePhysicsContactRecordObserver observe=[&](const SourcePhysicsContactRecordStore& store) {
                std::uint32_t value{};
                for(unsigned n=0;n<store.written.size();++n)value|=std::to_integer<unsigned>(store.written[n])<<(8*n);
                writes.push_back({store.instruction,store.address,store.written.size(),value});
            };
            SourcePhysicsWorldRuntimeServices services;
            const auto required=row.value("boundary",0u);
            services.required_owner=[&](SourcePhysicsContactMemory& m,const SourcePhysicsContactRuntimeCall& call,
                SourcePhysicsWorldRuntimeAbi& abi,const SourcePhysicsContactRecordObserver& writes) {
                if(required && call.target==required)throw RequiredOwnerBoundary{required};
                real_owner(m,call.target,abi,services,writes);
            };
            physics_world_runtime_detail::WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
            SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("gp").get<decltype(abi.gp)>();abi.eflags=row.at("eflags");
            abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
            if(row.contains("xmm"))abi.xmm=row.at("xmm").get<decltype(abi.xmm)>();
            state.load(abi);
            std::function<void(physics_world_runtime_detail::WorldState&,std::uint32_t,std::uint32_t,bool)> dispatch;
            dispatch=[&](auto& s,std::uint32_t entry,std::uint32_t,bool) {
                if(required && entry==required)throw RequiredOwnerBoundary{entry};
                if(entry==0x24d920u) {source_objective_native_tu15_detail::draw_goal_marker(s,dispatch);return;}
                auto call=s.save();real_owner(s.memory,entry,call,s.services,s.observe);s.load(call);
            };
            using namespace source_objective_native_tu15_detail;
            bool ran{};
            try {switch(row.at("entry").get<std::uint32_t>()) {
            case 0x24b2f0:ran=default_relationship(state);break;
            case 0x24a180:ran=registered_carrier(state);break;
            case 0x24be50:ran=marker_owner(state);break;
            case 0x24b4c0:ran=home_marker(state,dispatch);break;
            case 0x24a810:ran=pickup_allowed(state,dispatch);break;
            case 0x24a620:ran=overtime(state);break;
            case 0x24d390:ran=append_region(state,dispatch);break;
            case 0x24d1c0:ran=initialize_region_style(state,dispatch);break;
            case 0x24d920:ran=draw_goal_marker(state,dispatch);break;
            case 0x24aa90:ran=draw_assault_goals(state,dispatch);break;
            default:throw std::runtime_error("Unknown native objective owner case");
            }} catch(const RequiredOwnerBoundary& boundary) {need(boundary.entry==required,"Wrong native required owner boundary");ran=true;++required_boundaries;}
            need(ran,"Native whole owner declined reference input");const auto actual=state.save();
            need(actual.gp==row.at("expected_gp").get<decltype(actual.gp)>(),"Whole objective owner changed original registers");
            need((actual.eflags&row.value("eflags_mask",0x8d5u))==row.at("expected_flags"),"Whole objective owner changed defined original flags");
            need(writes==row.at("writes"),"Whole objective owner changed ordered original stores");
            if(row.contains("expected_xmm")) {
                need(actual.xmm==row.at("expected_xmm").get<decltype(actual.xmm)>(),"Whole objective owner changed original XMM results");
                need((actual.x87_status&0xfdc0u)==row.at("expected_x87_status"),"Whole objective owner changed original x87 status");
                Memory control_memory;
                for(const auto& patch:row.at("inputs"))control_memory.set(patch[0],patch[2],patch[1]);
                auto control=abi;SourcePhysicsWorldRuntimeServices control_services;
                control_services.instruction=[required](std::uint32_t pc){if(required && pc==required)throw RequiredOwnerBoundary{pc};};
                control_services.required_owner=[&](SourcePhysicsContactMemory& m,const SourcePhysicsContactRuntimeCall& call,
                    SourcePhysicsWorldRuntimeAbi& abi,const SourcePhysicsContactRecordObserver& writes) {
                    real_owner(m,call.target,abi,control_services,writes);
                };
                bool stopped{};
                try {source_run_multiplayer_engines_tu15(control_memory,row.at("entry").get<std::uint32_t>(),control,{0x27f,0x1f80},control_services);}
                catch(const RequiredOwnerBoundary& boundary) {need(boundary.entry==required,"Wrong literal required owner boundary");stopped=true;}
                need(stopped==(required!=0),"Literal objective boundary/completion differs");
                need(actual.gp==control.gp && actual.xmm==control.xmm && actual.eflags==control.eflags &&
                    actual.x87_status==control.x87_status && actual.floating_point->mxcsr==control.floating_point->mxcsr &&
                    actual.floating_point->x87_control_word==control.floating_point->x87_control_word &&
                    memory.bank==control_memory.bank && memory.bytes==control_memory.bytes,
                    "Native objective owner differs from the same-binary literal control");
                // The offline emulator does not report the hardware precision
                // sticky bit for inexact scalar arithmetic. Check its control
                // bits against Xbox execution and every status bit against the
                // same-binary hardware control, instead of changing game FP.
                const auto reference=row.at("expected_mxcsr").get<std::uint32_t>();
                need((actual.floating_point->mxcsr&~0x3fu)==(reference&~0x3fu),"Native objective changed original FP controls");
                if(actual.floating_point->mxcsr!=reference)++emulator_status_differences;
            }
            ++count;
        } catch(const std::exception& error){std::cerr<<row.at("name")<<": "<<error.what()<<'\n';throw;}
    }
    need(count==fixture.value("expected_case_count",fixture.contains("original_window_constant")?480u:1015u),"Whole native objective owner matrix lost cases");
    need(required_boundaries==fixture.value("required_owner_boundaries",0u),"Native required owner boundary count differs");
    if(argc==3) {
        Json report={{"passed",true},{"cases",count},{"required_owner_boundaries",required_boundaries},{"emulator_mxcsr_status_differences",emulator_status_differences},
            {"native_and_literal_hardware_state_identical",true},{"original_xbox_registers_flags_stores_and_xmm_identical",true}};
        std::ofstream output(argv[2]);output<<report.dump(2)<<'\n';need(bool(output),"Cannot retain native owner report");
    }
    std::cout<<count<<" original objective cases match native registers, flags and ordered stores; "
        <<required_boundaries<<" stop at required owner boundaries.\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
