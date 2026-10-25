#include "halo2/source_damage_delivery_runtime.hpp"
#include "halo2/source_damage_material_native_tu15.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
using Json=nlohmann::json;
void check(bool yes,const char *why){if(!yes)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string &hex) {
    check(hex.size()%2==0,"Odd original material byte count");std::vector<std::byte> result;
    for(std::size_t n=0;n<hex.size();n+=2)result.push_back(std::byte(std::stoul(hex.substr(n,2),nullptr,16)));
    return result;
}
struct Memory final:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    mutable Json accesses=Json::array();std::uint32_t pc{};
    auto at(std::uint32_t p)const {
        auto it=banks.upper_bound(p);check(it!=banks.begin(),"Native material escaped bank");--it;
        check(std::uint64_t(p)-it->first<it->second.size(),"Native material escaped extent");return it;
    }
    void read(std::uint32_t p,std::span<std::byte> out)const override {
        accesses.push_back({{"instruction",pc},{"kind","read"},{"address",p},{"size",out.size()}});
        for(auto &value:out){const auto it=at(p);value=it->second[p-it->first];++p;}
    }
    void write(std::uint32_t p,std::span<const std::byte> input)override {
        accesses.push_back({{"instruction",pc},{"kind","write"},{"address",p},{"size",input.size()}});
        for(const auto value:input){const auto it=at(p);banks.at(it->first)[p-it->first]=value;++p;}
    }
};
}
int main(int argc,char **argv)try {
    check(argc==4,"Supply original material fixture, observer route and report path");
    const std::string_view route=argv[2];check(route=="selected" || route=="canonical","Unknown material observer route");
    std::ifstream input(argv[1]);const auto fixture=Json::parse(input);
    check(fixture.at("schema")=="H2_DAMAGE_MATERIAL_NATIVE_ORIGINAL_V1","Wrong material authority");
    Json results=Json::array();std::size_t differences{},compared_bytes{},instructions_total{},accesses_total{};
    bool product_mutation_rejected=false,other_difference=false;
    for(const auto &row:fixture.at("cases")) {
        Memory memory;for(const auto &bank:row.at("before_banks"))
            check(memory.banks.emplace(bank.at("address").get<std::uint32_t>(),bytes(bank.at("hex"))).second,"Duplicate material bank");
        SourcePhysicsContactRuntimeAbi abi{};const auto &before=row.at("before"),&expected=row.at("after");
        abi.gp=before.at("gp").get<decltype(abi.gp)>();abi.xmm=before.at("xmm").get<decltype(abi.xmm)>();
        abi.eflags=before.at("eflags");abi.fs_base=0;abi.x87_depth=0;abi.x87_status=0;
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{before.at("x87_control_word"),before.at("mxcsr")};
        Json instructions=Json::array();SourcePhysicsContactRuntimeServices services{};
        services.instruction=[&](auto pc){
            const auto actual=route=="selected"?pc:fixture.at("canonical_observation_map").at(std::to_string(pc)).template get<std::uint32_t>();
            memory.pc=actual;instructions.push_back(actual);
        };
        services.required_owner=[](auto &,const auto &,auto &,const auto &){throw std::runtime_error("Material native requested external result");};
        const auto activity=source_damage_material_native_activity();
        (route=="selected"?source_run_damage_delivery_runtime_selected_observations:source_run_damage_delivery_runtime)(memory,
            0xd71c0u,abi,SourcePhysicsContactImage::TitleUpdate15,*abi.floating_point,services,{});
        const auto actual_activity=source_damage_material_native_activity();
        Json errors=Json::array();const auto verify=[&](bool yes,const char *why){if(!yes)errors.push_back(why);};
        verify(actual_activity.calls==activity.calls+1,"Native material route not selected");
        verify(instructions==row.at("instructions"),"Native material instruction boundaries differ");
        verify(memory.accesses==row.at("accesses"),"Native material ordered access addresses/sizes differ");
        verify(abi.gp==expected.at("gp").get<decltype(abi.gp)>(),"Native material GP state differs");
        verify((abi.eflags&0xfffffdffu)==(expected.at("eflags").get<std::uint32_t>()&0xfffffdffu),"Native material flags differ");
        const bool same_sse=abi.xmm==expected.at("xmm").get<decltype(abi.xmm)>();
        verify(same_sse,"Native material SSE state differs");
        verify(abi.fs_base==0 && abi.x87_depth==0 && abi.x87_status==expected.at("x87_status") && abi.floating_point &&
            abi.floating_point->x87_control_word==expected.at("x87_control_word") && abi.floating_point->mxcsr==expected.at("mxcsr"),"Native material FP/control state differs");
        bool same_banks=true;for(const auto &bank:row.at("after_banks")) {
            const auto &actual=memory.banks.at(bank.at("address").get<std::uint32_t>());
            same_banks &= actual==bytes(bank.at("hex"));compared_bytes+=actual.size();
        }
        verify(same_banks,"Native material full bank differs");
        if(!errors.empty()) {
            ++differences;
            const auto wanted=expected.at("xmm").at(0).at(0).get<std::uint32_t>(),got=abi.xmm[0][0];
            const auto nan=[](auto bits){return (bits&0x7f800000u)==0x7f800000u && (bits&0x7fffffu)!=0;};
            const bool nan_difference=nan(wanted)&&nan(got)&&wanted!=got;
            for(const auto &error:errors)if(error!="Native material SSE state differs" && error!="Native material full bank differs")other_difference=true;
            product_mutation_rejected |= nan_difference;
            other_difference |= !nan_difference;
        }
        instructions_total+=instructions.size();accesses_total+=memory.accesses.size();
        results.push_back({{"id",row.at("id")},{"name",row.at("name")},{"passed",errors.empty()},{"errors",errors}});
    }
    Json report{{"passed",differences==0},{"route",route},{"cases",results.size()},{"differences",differences},
        {"original_final_bank_bytes",compared_bytes},{"instructions",instructions_total},{"ordered_accesses",accesses_total},
        {"product_mutation_rejected",product_mutation_rejected && !other_difference},{"results",results}};
    std::ofstream(argv[3])<<report.dump(2)<<'\n';
    std::cout<<"Material owner route="<<route<<" cases="<<results.size()<<" differences="<<differences<<'\n';
    if(differences)std::cerr<<report.dump()<<'\n';return differences?1:0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
