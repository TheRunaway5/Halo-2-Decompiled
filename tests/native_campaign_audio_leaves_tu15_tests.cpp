#include "halo2/native_campaign_audio_leaves.hpp"
#include "audio/native_tu15_sound_campaign_audio.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>

namespace {
using namespace halo2;
using J=nlohmann::json;
void check(bool ok,const std::string& reason){if(!ok)throw std::runtime_error(reason);}
std::vector<std::byte> decode(const std::string& text) {
    check(text.size()%2==0,"Malformed original bank");std::vector<std::byte> result;
    for(std::size_t i=0;i<text.size();i+=2)result.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
    return result;
}
std::string encode(std::span<const std::byte> data) {
    constexpr char digits[]="0123456789abcdef";std::string text;
    for(auto byte:data){const auto value=std::to_integer<unsigned>(byte);text+=digits[value>>4];text+=digits[value&15];}
    return text;
}
struct Memory final:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    auto locate(std::uint32_t p,std::size_t n)const {
        auto found=banks.upper_bound(p);check(found!=banks.begin(),"Original audio backing unavailable");--found;
        check(std::uint64_t(p-found->first)+n<=found->second.size(),"Original audio extent unavailable");return found;
    }
    void read(std::uint32_t p,std::span<std::byte> out)const override {
        const auto found=locate(p,out.size());std::copy_n(found->second.begin()+p-found->first,out.size(),out.begin());
    }
    void write(std::uint32_t p,std::span<const std::byte> in)override {
        const auto found=locate(p,in.size());std::copy(in.begin(),in.end(),banks.at(found->first).begin()+p-found->first);
    }
};
std::uint16_t status(){std::uint16_t value;asm volatile("fnstsw %0":"=a"(value));return value;}
std::uint16_t control(){std::uint16_t value;asm volatile("fnstcw %0":"=m"(value));return value;}
void run(const std::filesystem::path& file) {
    std::ifstream input(file);check(bool(input),"Required original campaign audio fixture absent");const auto fixture=J::parse(input);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","TU1.5 audio image changed");
    check(fixture.at("backend")=="unchanged-tu15-physical-i386-stores-banks-fp","Independent physical original authority absent");
    unsigned cases{},writes{};std::array<unsigned,2> owners{};
    for(const auto& row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();Memory memory;
        for(const auto& bank:row.at("banks"))memory.banks.emplace(bank.at("address"),decode(bank.at("before")));
        const auto stack=row.at("stack").get<std::uint32_t>();
        memory.banks.emplace(stack-0x1000u,std::vector<std::byte>(0x2000u));
        auto& stack_bytes=memory.banks.at(stack-0x1000u);
        const auto return_address=row.at("stop").get<std::uint32_t>();
        const auto output=row.at("output").get<std::uint32_t>();
        for(unsigned i=0;i<4;++i){stack_bytes[0x1000u+i]=std::byte(return_address>>(8*i));stack_bytes[0x1004u+i]=std::byte(output>>(8*i));}
        SourcePhysicsContactRuntimeAbi abi{};abi.gp=row.at("gp").get<decltype(abi.gp)>();
        abi.xmm=row.at("xmm").get<decltype(abi.xmm)>();abi.eflags=2;
        const auto initial_gp=abi.gp;
        const auto initial_xmm=abi.xmm;
        const auto before=row.at("before_fp");
        const SourcePhysicsFactoryFloatingPoint fp{before[0],before[2]};
        abi.floating_point=fp;abi.x87_status=before[1];
        std::vector<SourcePhysicsContactRecordStore> stores;
        SourcePhysicsContactRecordObserver observer=[&](const auto& store){stores.push_back(store);};
        SourcePhysicsContactRuntimeServices services;
        services.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("Native audio leaf escaped to a residual owner");};
        native_tu15_sound::CampaignContext context{memory,fp,observer,services};
        const auto entry=row.at("entry").get<std::uint32_t>();
        auto routine=std::ranges::find_if(native_tu15_sound::campaign_channel_routines(),[&](auto routine){return routine.address==entry;});
        check(routine!=native_tu15_sound::campaign_channel_routines().end(),name+": native audio routine absent");
        const auto host_cw=control(),host_sw=status();const auto host_mxcsr=_mm_getcsr();
        routine->run(context,abi);
        check(control()==host_cw&&status()==host_sw&&_mm_getcsr()==host_mxcsr,name+": leaked host FP state");
        check(stores.size()==row.at("writes").size(),name+": ordered audio store count differs");
        for(unsigned i=0;i<stores.size();++i) {
            const auto& got=stores[i];const auto& expected=row.at("writes")[i];
            check(got.instruction==expected.at("instruction")&&got.address==expected.at("address")&&
                encode(got.before)==expected.at("before").get<std::string>()&&encode(got.written)==expected.at("written").get<std::string>(),
                name+": ordered audio store differs at "+std::to_string(i));
        }
        for(const auto& bank:row.at("banks"))
            check(encode(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+": retained audio bank differs");
        const auto after=row.at("after_fp");const auto actual=abi.floating_point.value_or(fp);
        check(actual.x87_control_word==after[0]&&abi.x87_status==after[1]&&actual.mxcsr==after[2],
            name+": physical audio FP differs "+J::array({actual.x87_control_word,abi.x87_status,actual.mxcsr}).dump()+"/"+after.dump());
        // Scratch EAX/ECX/EDX, XMM0/2/3/4 and dead stack are not outputs of the
        // original compiled function. Preserve every callee-saved value and
        // the delta input; no scratch register-machine reconstruction is used.
        for(unsigned reg:{3u,5u,6u,7u})check(abi.gp[reg]==initial_gp[reg],name+": native callee-saved GPR differs");
        for(unsigned reg:{1u,5u,6u,7u})check(abi.xmm[reg]==initial_xmm[reg],name+": native preserved SIMD differs");
        check(abi.gp[4]==stack+(entry==0x225d70u?8u:4u),name+": native return stack differs");
        ++owners[entry==0x225d70u];++cases;writes+=stores.size();
    }
    check(owners[0]>0&&owners[1]>0,"Both original campaign audio owners are required");
    std::cout<<cases<<" complete original campaign audio calls, "<<writes<<" ordered persistent stores and physical FP passed; whole campaign audio registration remains closed\n";
}
}
int main(int argc,char** argv)try {check(argc==2,"Supply physical original campaign audio fixture");run(argv[1]);return 0;}
catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
