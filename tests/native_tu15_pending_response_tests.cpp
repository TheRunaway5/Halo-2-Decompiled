#include "gameplay/objects/source_pending_response_native_tu15_internal.hpp"
#include <nlohmann/json.hpp>
#include <cstring>
#include <fstream>
#include <iostream>

using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
using Json=nlohmann::json;
namespace {
void check(bool yes,const char* why){if(!yes)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string& text) {
    std::vector<std::byte> data;check(text.size()%2==0,"Odd reference hex");
    for(std::size_t n=0;n<text.size();n+=2)data.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));
    return data;
}
struct Memory final:SourcePhysicsContactMemory {
    std::uint32_t base{},pc{};std::vector<std::byte> data;mutable Json accesses=Json::array();
    void bounds(std::uint32_t at,std::size_t n)const {
        check(at>=base && std::uint64_t(at-base)+n<=data.size(),"Pending step escaped actual declared bank");
    }
    void read(std::uint32_t at,std::span<std::byte> out)const override {
        bounds(at,out.size());accesses.push_back({{"pc",pc},{"kind","read"},{"address",at},{"size",out.size()}});
        std::copy_n(data.begin()+std::size_t(at-base),out.size(),out.begin());
    }
    void write(std::uint32_t at,std::span<const std::byte> in)override {
        bounds(at,in.size());accesses.push_back({{"pc",pc},{"kind","write"},{"address",at},{"size",in.size()}});
        std::copy(in.begin(),in.end(),data.begin()+std::size_t(at-base));
    }
};
unsigned half(std::ifstream& input) {
    unsigned char value[2];check(bool(input.read(reinterpret_cast<char*>(value),2)),"Truncated original word reference");
    return value[0]|(unsigned(value[1])<<8);
}
}
int main(int argc,char** argv)try {
    check(argc==4,"Supply decompressed numeric vectors, original ABI reference and report path");
    std::ifstream input(argv[1],std::ios::binary);char header[12];check(bool(input.read(header,12)),"Missing vector header");
    check(std::memcmp(header,"PND15V01",8)==0 && static_cast<unsigned char>(header[10])==1 && header[8]==0 && header[9]==0 && header[11]==0,"Wrong complete vector domain");
    unsigned words{};std::array<unsigned,3> statuses{};
    for(unsigned word=0;word<65536;++word) {
        const auto expected=half(input),count=half(input);const auto response=input.get(),status=input.get();
        const auto result=native_tu15_pending_response_step(std::uint16_t(word));
        check(result.word==expected && result.countdown==count && result.response==response && int(result.status)==status,
            "Native pending word differs from unchanged original instructions");
        ++words;++statuses.at(unsigned(status));
    }
    check(input.peek()==std::char_traits<char>::eof(),"Trailing numeric vectors");
    std::ifstream reference(argv[2]);const auto fixture=Json::parse(reference);unsigned cases{};
    for(const auto& row:fixture.at("abi_cases"))for(bool selected:{false,true}) {
        Memory memory;memory.base=fixture.at("bank");memory.data=bytes(row.at("before_bank_hex"));
        SourcePhysicsContactRuntimeAbi abi{};abi.gp=row.at("before_gp").get<decltype(abi.gp)>();abi.eflags=row.at("before_eflags");
        for(unsigned j=0;j<abi.xmm.size();++j)for(unsigned k=0;k<4;++k)abi.xmm[j][k]=0xa5760000u+j*4+k;
        const auto xmm=abi.xmm;const SourcePhysicsWorldRuntimeServices services{};
        const SourcePhysicsContactRecordObserver observer{};
        WorldState state(memory,observer,services,SourcePhysicsContactImage::TitleUpdate15);state.load(abi);
        Json trace=Json::array();
        std::optional<SourcePendingResponseNativeObservationTu15> observation;
        const SourcePendingResponseObservationScopeTu15 observed(&memory,[&](const auto& value){
            check(!observation,"Native pending step published twice");observation=value;
            const auto at=std::size_t(value.address-memory.base);
            check(at+2<=memory.data.size(),"Native pending observation escaped its bank");
            const auto actual=std::to_integer<unsigned>(memory.data[at])|
                (std::to_integer<unsigned>(memory.data[at+1])<<8);
            check(actual==value.after,"Native pending observation preceded its actual publication");
        });
        const auto status=pending_response_step_native_tu15(state,[&](auto canonical,auto actual){
            check(canonical+0xf50u==actual,"Pending parent observation mapping changed");
            // Both parent APIs retain their own observation convention. The
            // independent reference always records actual TU1.5 addresses.
            const auto published=selected?actual:canonical;
            memory.pc=selected?published:published+0xf50u;trace.push_back(memory.pc);return published;
        });
        const auto after=state.save();
        check(observation && observation->address==fixture.at("pending").get<unsigned>() &&
            observation->before==row.at("word").get<unsigned>() && observation->status==status,
            "Native pending observation lost its actual word/continuation");
        check(int(status)==row.at("status").get<int>(),"Pending continuation selection differs");
        check(after.gp==row.at("after_gp").get<decltype(abi.gp)>(),"Pending volatile/preserved registers differ");
        check((after.eflags&0x8c5u)==(row.at("after_eflags").get<unsigned>()&0x8c5u),"Pending defined flags differ");
        check(after.xmm==xmm && after.x87_depth==0 && after.x87_status==0,"Pending step changed unused FP state");
        check(memory.data==bytes(row.at("after_bank_hex")),"Pending complete declared bank differs");
        check(trace==row.at("trace") && memory.accesses==row.at("accesses"),"Pending ordered observations/accesses differ");
        ++cases;
    }
    Json report={{"passed",true},{"word_cases",words},{"status_counts",statuses},{"adapter_route_cases",cases},
        {"original_instruction_sites",fixture.at("original_instruction_sites").size()},
        {"full_parent_or_live_event_delivery_verified",false}};
    std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';check(bool(out),"Pending report write failed");
    std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
