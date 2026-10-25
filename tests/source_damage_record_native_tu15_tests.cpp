#include "halo2/source_damage_receiver_tu15.hpp"
#include "halo2/source_damage_record_native_tu15.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
using Json=nlohmann::json;
void check(bool yes,const char *why){if(!yes)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string &hex) {
    check(hex.size()%2==0,"Odd native record fixture byte count");std::vector<std::byte> result;
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
    auto at(std::uint32_t p)const {
        auto it=banks.upper_bound(p);check(it!=banks.begin(),"Native record escaped its bank");--it;
        check(std::uint64_t(p)-it->first<it->second.size(),"Native record escaped its extent");return it;
    }
    void read(std::uint32_t p,std::span<std::byte> out)const override {
        for(auto &value:out){const auto it=at(p);value=it->second[p-it->first];++p;}
    }
    void write(std::uint32_t p,std::span<const std::byte> input)override {
        for(const auto value:input){const auto it=at(p);banks.at(it->first)[p-it->first]=value;++p;}
    }
};
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply untouched original indexed-record fixture");
    std::ifstream input(argv[1]);const auto fixture=Json::parse(input);
    check(fixture.at("schema")=="H2_DAMAGE_RECORD_NATIVE_ORIGINAL_V1","Wrong indexed-record authority");
    std::size_t cases{},compared_bytes{},stores{};
    for(const auto &row:fixture.at("cases")) {
        Memory memory;
        for(const auto &bank:row.at("before_banks"))
            check(memory.banks.emplace(bank.at("address").get<std::uint32_t>(),bytes(bank.at("hex"))).second,
                "Duplicate indexed-record bank");
        SourcePhysicsWorldRuntimeAbi abi{};
        const auto &before=row.at("before");abi.gp=before.at("gp").get<decltype(abi.gp)>();
        abi.xmm=before.at("xmm").get<decltype(abi.xmm)>();abi.eflags=before.at("eflags");abi.fs_base=0;
        abi.x87_depth=0;abi.x87_status=0;
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{before.at("x87_control_word"),before.at("mxcsr")};
        const auto initial=abi;Json instructions=Json::array(),writes=Json::array();
        SourcePhysicsWorldRuntimeServices services{};
        services.instruction=[&](auto pc){instructions.push_back(pc);};
        services.required_owner=[](auto &,const auto &,auto &,const auto &){
            throw std::runtime_error("Indexed-record native owner requested a fabricated external result");
        };
        const auto activity=source_damage_record_native_activity();
        source_run_damage_receiver_tu15(memory,0x105460u,abi,*abi.floating_point,services,[&](const auto &store){
            writes.push_back({{"instruction",store.instruction},{"address",store.address},
                {"before",hex(store.before)},{"written",hex(store.written)}});
        });
        const auto after=source_damage_record_native_activity();
        check(after.calls==activity.calls+1,"Indexed-record native kernel was not selected");
        check(writes==row.at("stores"),"Indexed-record native ordered stores differ from original");
        check(instructions==row.at("instructions"),"Indexed-record native instruction boundaries differ from original");
        check(abi.gp==row.at("after").at("gp").get<decltype(abi.gp)>(),"Indexed-record native GP state differs from original");
        check(abi.eflags==row.at("after").at("eflags"),"Indexed-record native integer flags differ from original");
        check(abi.xmm==row.at("after").at("xmm").get<decltype(abi.xmm)>(),"Indexed-record native SSE state differs from original");
        check(abi.fs_base==initial.fs_base && abi.x87_depth==0 && abi.x87_status==0 && abi.floating_point &&
            abi.floating_point->x87_control_word==row.at("after").at("x87_control_word") &&
            abi.floating_point->mxcsr==row.at("after").at("mxcsr"),"Indexed-record native FP/control state differs from original");
        for(const auto &bank:row.at("after_banks")) {
            const auto &actual=memory.banks.at(bank.at("address").get<std::uint32_t>());
            check(actual==bytes(bank.at("hex")),"Indexed-record native full memory bank differs from original");
            compared_bytes+=actual.size();
        }
        stores+=writes.size();++cases;
    }
    std::cout<<"PASS indexed-record native original cases="<<cases<<" ordered_stores="<<stores<<" full_bank_bytes="<<compared_bytes<<'\n';
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
