#include "halo2/source_damage_model_children_tu15.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
using Json=nlohmann::json;
void check(bool yes,const char *why){if(!yes)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string &hex) {
    check(hex.size()%2==0,"Odd original child byte count");std::vector<std::byte> result;
    for(std::size_t n=0;n<hex.size();n+=2)result.push_back(std::byte(std::stoul(hex.substr(n,2),nullptr,16)));
    return result;
}
struct Memory final:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    mutable Json accesses=Json::array();std::uint32_t pc{};
    auto at(std::uint32_t p)const {
        auto it=banks.upper_bound(p);check(it!=banks.begin(),"Native child escaped bank");--it;
        check(std::uint64_t(p)-it->first<it->second.size(),"Native child escaped extent");return it;
    }
    void read(std::uint32_t p,std::span<std::byte> out)const override {
        accesses.push_back({{"instruction",pc},{"kind","read"},{"address",p},{"size",out.size()}});
        for(auto &value:out){const auto it=at(p);value=it->second[p-it->first];++p;}
    }
    void write(std::uint32_t p,std::span<const std::byte> input)override {
        accesses.push_back({{"instruction",pc},{"kind","write"},{"address",p},{"size",input.size()}});
        for(const auto value:input){const auto it=at(p);banks.at(it->first)[p-it->first]=value;++p;}
    }
    // Diagnostic callback inspection is outside the owner's memory trace.
    std::uint32_t word(std::uint32_t p)const {
        std::uint32_t result{};for(unsigned n=0;n<4;++n){const auto b=at(p+n);result|=std::to_integer<std::uint32_t>(b->second[p+n-b->first])<<(8*n);}return result;
    }
};
}
int main(int argc,char **argv)try {
    check(argc==3,"Supply original child controls and report path");
    std::ifstream input(argv[1]);const auto fixture=Json::parse(input);
    check(fixture.at("schema")=="H2_DAMAGE_MODEL_CHILDREN_ORIGINAL_V1","Wrong child authority");
    Json results=Json::array();std::size_t differences{},compared_bytes{},instructions_total{},accesses_total{},calls_total{};
    for(const auto &row:fixture.at("cases")) {
        Memory memory;for(const auto &bank:row.at("before_banks"))
            check(memory.banks.emplace(bank.at("address").get<std::uint32_t>(),bytes(bank.at("hex"))).second,"Duplicate child bank");
        SourcePhysicsContactRuntimeAbi abi{};const auto &before=row.at("before"),&expected=row.at("after");
        abi.gp=before.at("gp").get<decltype(abi.gp)>();abi.xmm=before.at("xmm").get<decltype(abi.xmm)>();
        abi.eflags=before.at("eflags");abi.fs_base=0;abi.x87_depth=0;abi.x87_status=0;
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{before.at("x87_control_word"),before.at("mxcsr")};
        Json instructions=Json::array(),calls=Json::array();SourcePhysicsContactRuntimeServices services{};
        services.instruction=[&](auto pc){memory.pc=pc;instructions.push_back(pc);};
        services.required_owner=[&](auto &,const auto &call,auto &state,const auto &) {
            check(call.instruction==0xdc619u && call.target==0xd9110u && !call.tail,"Child requested a different source owner");
            calls.push_back({{"gp",state.gp},{"eflags",state.eflags},{"return",memory.word(state.gp[4])},
                {"child",memory.word(state.gp[4]+4u)}});
            // Declared component callback clobbers; no gameplay result or
            // original D9110 behavior is asserted by this boundary fixture.
            state.gp[0]=0x10325476u;state.gp[1]=0x89abcdefu;state.gp[2]=0x76543210u;
            state.gp[4]+=8u;state.eflags=0x293u;
        };
        const auto activity=source_damage_model_children_native_activity();
        source_run_damage_model_children_tu15(memory,abi,*abi.floating_point,services,{});
        Json errors=Json::array();const auto verify=[&](bool yes,const char *why){if(!yes)errors.push_back(why);};
        verify(source_damage_model_children_native_activity()==activity+1,"Native child route not selected");
        verify(instructions==row.at("instructions"),"Child instruction boundaries differ");
        verify(memory.accesses==row.at("accesses"),"Child ordered memory access differs");
        verify(calls==row.at("calls"),"Child callee ABI/order differs");
        verify(abi.gp==expected.at("gp").get<decltype(abi.gp)>(),"Child GP state differs");
        verify((abi.eflags&0xfffffdffu)==(expected.at("eflags").get<std::uint32_t>()&0xfffffdffu),"Child flags differ");
        verify(abi.xmm==expected.at("xmm").get<decltype(abi.xmm)>(),"Child SSE state differs");
        verify(abi.fs_base==0 && abi.x87_depth==0 && abi.x87_status==expected.at("x87_status") && abi.floating_point &&
            abi.floating_point->x87_control_word==expected.at("x87_control_word") && abi.floating_point->mxcsr==expected.at("mxcsr"),"Child FP/control state differs");
        bool same_banks=true;for(const auto &bank:row.at("after_banks")) {
            const auto &actual=memory.banks.at(bank.at("address").get<std::uint32_t>());
            same_banks &= actual==bytes(bank.at("hex"));compared_bytes+=actual.size();
        }
        verify(same_banks,"Child final bank differs");if(!errors.empty())++differences;
        instructions_total+=instructions.size();accesses_total+=memory.accesses.size();calls_total+=calls.size();
        results.push_back({{"id",row.at("id")},{"name",row.at("name")},{"passed",errors.empty()},{"errors",errors}});
    }
    Json report{{"passed",differences==0},{"cases",results.size()},{"differences",differences},
        {"original_final_bank_bytes",compared_bytes},{"instructions",instructions_total},{"ordered_accesses",accesses_total},
        {"declared_callback_invocations",calls_total},{"results",results},{"full_gameplay_acceptance",false}};
    std::ofstream(argv[2])<<report.dump(2)<<'\n';
    std::cout<<"Child owner cases="<<results.size()<<" differences="<<differences<<'\n';
    if(differences)std::cerr<<report.dump()<<'\n';return differences?1:0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
