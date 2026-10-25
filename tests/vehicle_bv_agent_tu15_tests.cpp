#include "halo2/source_vehicle_bv_agent_tu15.hpp"
#include "halo2/physics_world_runtime.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
using namespace halo2;
namespace {
using Json=nlohmann::json;
void check(bool yes,const std::string &why){if(!yes)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string &hex) {
    check(hex.size()%2==0,"Invalid original BV input bytes");std::vector<std::byte> result;
    for(std::size_t n=0;n<hex.size();n+=2)result.push_back(std::byte(std::stoul(hex.substr(n,2),nullptr,16)));
    return result;
}
std::string hex(std::span<const std::byte> bytes) {
    constexpr char digits[]="0123456789abcdef";std::string result;
    for(auto byte:bytes){const auto value=std::to_integer<unsigned>(byte);result+=digits[value>>4];result+=digits[value&15];}
    return result;
}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    auto at(std::uint32_t address)const {
        auto row=banks.upper_bound(address);
        check(row!=banks.begin(),"BV query read has no original input bank");--row;
        check(std::uint64_t(address-row->first)<row->second.size(),"BV query read escapes its original input bank");return row;
    }
    void read(std::uint32_t address,std::span<std::byte> output)const override {
        for(auto &byte:output){const auto row=at(address);byte=row->second[address++-row->first];}
    }
    void write(std::uint32_t address,std::span<const std::byte> input)override {
        for(auto byte:input){const auto row=at(address);banks.at(row->first)[address++-row->first]=byte;}
    }
};
SourcePhysicsWorldRuntimeAbi abi(const Json &row) {
    SourcePhysicsWorldRuntimeAbi result;result.gp=row.at("gp").get<decltype(result.gp)>();
    result.eflags=row.at("eflags");result.fs_base=row.at("fs_base");result.x87_status=row.at("x87_status");
    result.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("x87_control_word"),row.at("mxcsr")};
    for(unsigned n=0;n<8;++n) {
        const auto value=row.at("xmm")[n].get<std::string>();check(value.size()==32,"Invalid original SSE input");
        for(unsigned word=0;word<4;++word)result.xmm[n][word]=std::stoul(value.substr(24-word*8,8),nullptr,16);
    }
    result.x87_depth=row.at("x87_values").size();check(result.x87_depth<=8,"Invalid original x87 input");
    for(unsigned n=0;n<result.x87_depth;++n) {
        const auto raw=bytes(row.at("x87_values")[n]);check(raw.size()==10,"Invalid original extended input");
        std::memcpy(&result.x87[n],raw.data(),10);
    }
    return result;
}
}
int main(int argc,char **argv)try {
    check(argc==3,"Supply ORIGINAL_FIXTURE REPORT");std::ifstream input(argv[1]);const auto fixture=Json::parse(input);
    check(fixture.at("image")=="1.5","BV-agent oracle requires English TU1.5");
    unsigned compared{},stores_compared{},instructions_compared{};std::size_t memory_compared{};
    for(const auto &case_:fixture.at("cases")) {
        check(case_.at("complete").get<bool>(),"Complete original BV-agent return required");
        Memory memory;
        for(const auto &bank:case_.at("initial_banks"))
            check(memory.banks.emplace(bank.at("address"),bytes(bank.at("hex"))).second,"Duplicate original BV bank");
        auto state=abi(case_.at("entry_abi"));const auto expected=abi(case_.at("exit"));
        SourcePhysicsWorldRuntimeServices services;std::vector<std::uint32_t> trace;
        std::vector<SourcePhysicsContactRecordStore> stores;unsigned callbacks{},timestamps{},estimates{};
        services.instruction=[&](auto pc){trace.push_back(pc);};
        services.timestamp_counter=[&] {
            check(timestamps<case_.at("timestamps").size(),"Unrecorded BV CPU timestamp");
            return case_.at("timestamps")[timestamps++].at("output").get<std::uint64_t>();
        };
        services.reciprocal_sqrt=[&](auto value) {
            check(estimates<case_.at("estimates").size(),"Unrecorded BV CPU estimate");
            const auto &row=case_.at("estimates")[estimates++];check(value==row.at("input"),"BV CPU estimate input differs");
            return row.at("output").get<std::uint32_t>();
        };
        services.required_owner=[&](auto &m,const auto &call,auto &a,const auto &observe) {
            const auto fp=*a.floating_point;
            if(source_vehicle_bv_agent_tu15_owns_entry(call.target)) {
                source_run_vehicle_bv_agent_tu15(m,call.target,a,fp,services,observe);return;
            }
            // These complete existing helpers are independently reached by the
            // original witness. No callback value or return is supplied.
            if(call.target==0x31ede0u || call.target==0x312620u) {
                ++callbacks;
                source_run_physics_world_runtime_selected_observations(m,
                    call.target==0x31ede0u?0x31b250u:0x30eb30u,a,
                    SourcePhysicsContactImage::TitleUpdate15,fp,services,observe);return;
            }
            throw std::runtime_error("Unbound original BV query callback "+std::to_string(call.target));
        };
        source_run_vehicle_bv_agent_tu15(memory,case_.at("entry"),state,*state.floating_point,services,
            [&](const auto &store){stores.push_back(store);});
        check(trace==case_.at("instructions").get<decltype(trace)>(),"Native BV query changed original instruction path");
        check(stores.size()==case_.at("writes").size(),"Native BV query changed original store count");
        for(std::size_t n=0;n<stores.size();++n) {
            const auto &actual=stores[n];const auto &original=case_.at("writes")[n];
            check(actual.instruction==original.at("instruction") && actual.address==original.at("address") &&
                actual.before==bytes(original.at("before")) && actual.written==bytes(original.at("written")),
                "Native BV query changed ordered original store "+std::to_string(n));
        }
        check(state.gp==expected.gp && state.xmm==expected.xmm && state.fs_base==expected.fs_base &&
            (state.eflags&0xcd5u)==(expected.eflags&0xcd5u),"Native BV query changed original integer/SSE/FS ABI");
        const bool fp_equal=state.x87_depth==expected.x87_depth && state.x87_status==expected.x87_status &&
            state.floating_point->x87_control_word==expected.floating_point->x87_control_word &&
            state.floating_point->mxcsr==expected.floating_point->mxcsr;
        if(!fp_equal)std::cerr<<Json{{"native_depth",state.x87_depth},{"original_depth",expected.x87_depth},
            {"native_status",state.x87_status},{"original_status",expected.x87_status},
            {"native_control",state.floating_point->x87_control_word},{"original_control",expected.floating_point->x87_control_word},
            {"native_mxcsr",state.floating_point->mxcsr},{"original_mxcsr",expected.floating_point->mxcsr}}.dump()<<'\n';
        check(fp_equal,"Native BV query changed original FP ABI");
        for(unsigned n=0;n<state.x87_depth;++n)
            check(std::memcmp(&state.x87[n],&expected.x87[n],10)==0,"Native BV query changed extended original value");
        for(const auto &bank:case_.at("final_banks")) {
            const auto &actual=memory.banks.at(bank.at("address").get<std::uint32_t>());
            check(hex(actual)==bank.at("hex").get<std::string>(),"Native BV query changed complete original input/output bank");
            memory_compared+=actual.size();
        }
        check(callbacks==4 && timestamps==case_.at("timestamps").size() && estimates==case_.at("estimates").size(),
            "Native BV query changed real callback or recorded CPU input count");
        ++compared;stores_compared+=stores.size();instructions_compared+=trace.size();
    }
    const Json report{{"passed",true},{"original_cases",compared},{"ordered_stores",stores_compared},
        {"instructions",instructions_compared},{"complete_bank_bytes",memory_compared},
        {"full_x87_status_compared",true},{"full_mxcsr_compared",true},
        {"subcall_results_supplied",false},{"production_opcode_execution",false},{"full_vehicle_parity",false}};
    std::ofstream(argv[2])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
