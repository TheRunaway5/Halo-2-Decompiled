#include "halo2/source_damage_delivery_runtime.hpp"
#include "halo2/source_damage_model_native_tu15.hpp"
#include "halo2/source_direct_damage_runtime.hpp"
#include "halo2/source_vitality_runtime.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
using Json=nlohmann::json;
void check(bool yes,const char *why){if(!yes)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string &hex) {
    check(hex.size()%2==0,"Odd native model fixture byte count");std::vector<std::byte> result;
    for(std::size_t n=0;n<hex.size();n+=2)result.push_back(std::byte(std::stoul(hex.substr(n,2),nullptr,16)));
    return result;
}
std::string hex(std::span<const std::byte> data) {
    constexpr char digits[]="0123456789abcdef";std::string result;
    for(const auto value:data){const auto b=std::to_integer<unsigned>(value);result+=digits[b>>4];result+=digits[b&15];}
    return result;
}
struct Memory final:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    mutable Json reads=Json::array();
    std::uint32_t pc{};
    auto at(std::uint32_t p)const {
        auto it=banks.upper_bound(p);check(it!=banks.begin(),"Native model escaped its bank");--it;
        check(std::uint64_t(p)-it->first<it->second.size(),"Native model escaped its extent");return it;
    }
    void read(std::uint32_t p,std::span<std::byte> out)const override {
        const auto address=p;
        for(auto &value:out){const auto it=at(p);value=it->second[p-it->first];++p;}
        reads.push_back({{"instruction",pc},{"address",address},{"bytes",hex(out)}});
    }
    void write(std::uint32_t p,std::span<const std::byte> input)override {
        for(const auto value:input){const auto it=at(p);banks.at(it->first)[p-it->first]=value;++p;}
    }
};
}
int main(int argc,char **argv)try {
    check(argc==2 || argc==3,"Supply untouched original model-damage fixture and optional route");
    const std::string_view route=argc==3?argv[2]:"delivery";
    check(route=="delivery" || route=="direct" || route=="vitality","Unknown native model route");
    std::ifstream input(argv[1]);const auto fixture=Json::parse(input);
    check(fixture.at("schema")=="H2_DAMAGE_MODEL_NATIVE_ORIGINAL_V1","Wrong model-damage authority");
    std::size_t cases{},compared_bytes{},reads{};
    for(const auto &row:fixture.at("cases")) {
        Memory memory;
        for(const auto &bank:row.at("before_banks"))
            check(memory.banks.emplace(bank.at("address").get<std::uint32_t>(),bytes(bank.at("hex"))).second,
                "Duplicate model-damage bank");
        SourcePhysicsContactRuntimeAbi abi{};
        const auto &before=row.at("before");abi.gp=before.at("gp").get<decltype(abi.gp)>();
        abi.xmm=before.at("xmm").get<decltype(abi.xmm)>();abi.eflags=before.at("eflags");abi.fs_base=0;
        abi.x87_depth=0;abi.x87_status=0;
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{before.at("x87_control_word"),before.at("mxcsr")};
        const auto initial=abi;Json instructions=Json::array();
        SourcePhysicsContactRuntimeServices services{};
        services.instruction=[&](auto pc){
            const auto actual=route=="delivery"?pc:fixture.at("canonical_observation_map").at(std::to_string(pc)).template get<std::uint32_t>();
            memory.pc=actual;instructions.push_back(actual);
        };
        services.required_owner=[](auto &,const auto &,auto &,const auto &){
            throw std::runtime_error("Model-damage native owner requested a fabricated external result");
        };
        const auto activity=source_damage_model_native_activity();
        const auto observe=[](const auto &){
                throw std::runtime_error("Read-only original model lookup emitted a store");
            };
        if(route=="delivery")source_run_damage_delivery_runtime_selected_observations(memory,0xd7160u,abi,
            SourcePhysicsContactImage::TitleUpdate15,*abi.floating_point,services,observe);
#ifdef HALO2_DAMAGE_MODEL_PAIRED_ROUTES
        else (route=="direct"?source_run_direct_damage_runtime:source_run_vitality_runtime)(memory,0xd7160u,abi,
            SourcePhysicsContactImage::TitleUpdate15,*abi.floating_point,services,observe);
#else
        else throw std::runtime_error("Paired routes were not compiled in this private qualification");
#endif
        const auto after=source_damage_model_native_activity();
        check(after.calls==activity.calls+1,"Model-damage native kernel was not selected");
        check(instructions==row.at("instructions"),"Model-damage native instruction boundaries differ from original");
        check(memory.reads==row.at("reads"),"Model-damage native ordered reads differ from original");
        check(abi.gp==row.at("after").at("gp").get<decltype(abi.gp)>(),"Model-damage native GP state differs from original");
        check(abi.eflags==row.at("after").at("eflags"),"Model-damage native integer flags differ from original");
        check(abi.xmm==row.at("after").at("xmm").get<decltype(abi.xmm)>(),"Model-damage native SSE state differs from original");
        check(abi.fs_base==initial.fs_base && abi.x87_depth==0 && abi.x87_status==0 && abi.floating_point &&
            abi.floating_point->x87_control_word==row.at("after").at("x87_control_word") &&
            abi.floating_point->mxcsr==row.at("after").at("mxcsr"),"Model-damage native FP/control state differs from original");
        for(const auto &bank:row.at("after_banks")) {
            const auto &actual=memory.banks.at(bank.at("address").get<std::uint32_t>());
            check(actual==bytes(bank.at("hex")),"Model-damage native full memory bank differs from original");
            compared_bytes+=actual.size();
        }
        reads+=memory.reads.size();++cases;
    }
    std::cout<<"PASS model-damage native route="<<route<<" original cases="<<cases<<" ordered_reads="<<reads<<" full_bank_bytes="<<compared_bytes<<'\n';
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
