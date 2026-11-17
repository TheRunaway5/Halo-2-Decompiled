#include "halo2/physics_phantom_world_tu15.hpp"
#if defined(HALO2_EFFECT_RATIO_LINK_VEHICLE_UPDATE)
#include "halo2/physics_vehicle_update.hpp"
#endif

#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace {
using J=nlohmann::json;
void check(bool condition,const std::string& message) {
    if(!condition)throw std::runtime_error(message);
}
std::vector<std::byte> bytes(const std::string& value) {
    check(value.size()%2==0,"Invalid original bank byte length");
    std::vector<std::byte> data;
    for(std::size_t i=0;i<value.size();i+=2)data.push_back(std::byte(std::stoul(value.substr(i,2),nullptr,16)));
    return data;
}
struct OperandStop{};
struct Bank : halo2::SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> segments;
    std::uint32_t pc{};
    std::optional<unsigned> stop;
    mutable J reads=J::array();
    void read(std::uint32_t address,std::span<std::byte> data)const override {
        if(stop&&reads.size()==*stop)throw OperandStop{};
        auto bank=segments.upper_bound(address);
        check(bank!=segments.begin(),"Linked Vehicle effect-ratio read escaped its banks");
        --bank;
        check(std::uint64_t(address-bank->first)+data.size()<=bank->second.size(),"Linked Vehicle effect-ratio read extent differs");
        reads.push_back(J{{"instruction",pc},{"address",address},{"bytes",data.size()}});
        std::memcpy(data.data(),bank->second.data()+address-bank->first,data.size());
    }
    void write(std::uint32_t,std::span<const std::byte>)override {
        throw std::runtime_error("Unexpected linked D3750 write");
    }
};
void load_x87(halo2::SourcePhysicsContactRuntimeAbi& abi,const J& values) {
    check(values.size()<=abi.x87.size(),"Invalid original x87 depth");
    abi.x87_depth=values.size();
    for(unsigned i=0;i<abi.x87_depth;++i) {
        const auto data=bytes(values[i]);
        check(data.size()==10&&sizeof(abi.x87[i])>=10,"Invalid original extended value");
        std::memcpy(&abi.x87[i],data.data(),10);
    }
}
}

int main(int argc,char** argv) try {
    check(argc==2||argc==3,"Supply owned original D3750 fixture and optional small report");
    std::ifstream input(argv[1]);const auto fixture=J::parse(input);
    unsigned normal{},stops{},callbacks{},reads{};
#if defined(HALO2_EFFECT_RATIO_LINK_VEHICLE_UPDATE)
    constexpr std::uint32_t callback_displacement=0x1090u;
#else
    constexpr std::uint32_t callback_displacement=0u;
#endif
    for(const auto& row:fixture.at("cases")) {
        Bank bank;
        for(const auto& region:row.at("banks"))bank.segments.emplace(region.at("address"),bytes(region.at("before")));
        if(row.contains("fault"))bank.stop=row.at("fault").at("ordinal").get<unsigned>();
        halo2::SourcePhysicsContactRuntimeAbi abi{};abi.gp=row.at("gp");abi.xmm=row.at("xmm");abi.eflags=row.at("eflags");
        abi.x87_status=row.at("before_fp")[1];
        halo2::SourcePhysicsFactoryFloatingPoint fp{row.at("before_fp")[0],row.at("before_fp")[2]};
        abi.floating_point=fp;load_x87(abi,row.at("live_x87"));
        halo2::SourcePhysicsContactRuntimeServices services;
        std::vector<std::uint32_t> trace,expected_trace;
        services.instruction=[&](auto pc){bank.pc=pc;trace.push_back(pc);};
        services.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("D3750 unexpectedly reached another owner");};
        auto observe=[](const auto&){throw std::runtime_error("D3750 unexpectedly reached a store observer");};
        bool stopped{};
        try {
#if defined(HALO2_EFFECT_RATIO_LINK_VEHICLE_UPDATE)
            halo2::source_run_physics_vehicle_update_runtime(bank,0xd26c0u,abi,
                halo2::SourcePhysicsContactImage::TitleUpdate15,fp,services,observe);
#else
            halo2::source_run_physics_phantom_world_tu15_runtime(bank,0xd3750u,abi,fp,services,observe);
#endif
        }
        catch(const OperandStop&){stopped=true;}
        const auto name=row.at("name").get<std::string>();
        for(const auto& site:row.at("sites"))expected_trace.push_back(site.at("entry").get<std::uint32_t>()-callback_displacement);
        auto expected_reads=row.at("reads");
        for(auto& operand:expected_reads)operand["instruction"]=operand.at("instruction").get<std::uint32_t>()-callback_displacement;
        check(stopped==bank.stop.has_value()&&trace==expected_trace&&bank.reads==expected_reads,name+": actual Phantom/Vehicle trace, operands or stop differ");
        const auto& expected=row.at("expected");
        J difference=J::object();
        const auto compare=[&](const char* field,J actual,const J& wanted) {
            if(actual!=wanted)difference[field]=J{{"actual",actual},{"original",wanted}};
        };
        compare("gp",abi.gp,expected.at("gp"));compare("xmm",abi.xmm,expected.at("xmm"));
        compare("eflags",abi.eflags,expected.at("eflags"));compare("x87_depth",abi.x87_depth,expected.at("live_x87").size());
        compare("fp",abi.floating_point?J::array({abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr}):J(),expected.at("fp"));
        compare("active_mmx",abi.active_mmx.has_value(),false);
        check(difference.empty(),name+": linked Vehicle effect-ratio complete ABI differs "+difference.dump());
#if !defined(HALO2_EFFECT_RATIO_LINK_VEHICLE_UPDATE)
        check(fp.x87_control_word==expected.at("fp")[0]&&fp.mxcsr==expected.at("fp")[2],name+": linked Phantom FP transfer differs");
#endif
        for(unsigned i=0;i<abi.x87_depth;++i){
            const auto data=bytes(expected.at("live_x87")[i]);
            check(std::memcmp(data.data(),&abi.x87[i],10)==0,name+": linked Vehicle x87 value differs");
        }
        for(const auto& region:row.at("banks"))
            check(bank.segments.at(region.at("address"))==bytes(region.at("after")),name+": linked Vehicle bank differs");
        if(stopped)++stops;else ++normal;
        callbacks+=trace.size();reads+=bank.reads.size();
    }
    const J report{{"passed",true},{"real_phantom_export",callback_displacement==0u},
        {"real_vehicle_update_export",callback_displacement!=0u},{"normal",normal},{"operand_stops",stops},
        {"callback_sites",callbacks},{"ordered_reads",reads}};
    if(argc==3){std::ofstream out(argv[2]);out<<report.dump(2)<<'\n';check(bool(out),"Cannot write the owned D3750 linked receipt");}
    std::cout<<report.dump()<<'\n';
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
