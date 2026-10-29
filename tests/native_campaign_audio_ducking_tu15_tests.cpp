#include "audio/native_campaign_audio_ducking_adapter.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
namespace {
using namespace halo2;using J=nlohmann::json;
void check(bool value,const std::string& reason){if(!value)throw std::runtime_error(reason);}
std::vector<std::byte> decode(const std::string& text){std::vector<std::byte> result;for(std::size_t i=0;i<text.size();i+=2)result.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));return result;}
std::string encode(std::span<const std::byte> bytes){constexpr char digits[]="0123456789abcdef";std::string result;for(auto byte:bytes){auto n=std::to_integer<unsigned>(byte);result+=digits[n>>4];result+=digits[n&15];}return result;}
struct Memory final:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    struct Read {std::uint32_t address;std::vector<std::byte> bytes;unsigned stores_before;};
    mutable std::vector<Read> reads;
    unsigned store_count{};
    auto locate(std::uint32_t address,std::size_t size)const {
        auto found=banks.upper_bound(address);check(found!=banks.begin(),"Ducking input owner absent");--found;
        check(std::uint64_t(address-found->first)+size<=found->second.size(),"Ducking input extent absent");return found;
    }
    void read(std::uint32_t address,std::span<std::byte> result)const override {
        const auto bank=locate(address,result.size());std::copy_n(bank->second.begin()+address-bank->first,result.size(),result.begin());
        if(address<0x2000000u)reads.push_back({address,{result.begin(),result.end()},store_count});
    }
    void write(std::uint32_t address,std::span<const std::byte> input)override {
        // The observer helper reads a store's before-image for diagnostics.
        // Remove exactly that diagnostic read, retaining the algorithm's reads.
        if(!reads.empty()&&reads.back().address==address&&reads.back().bytes.size()==input.size())reads.pop_back();
        const auto bank=locate(address,input.size());std::copy(input.begin(),input.end(),banks.at(bank->first).begin()+address-bank->first);
        ++store_count;
    }
};
std::uint16_t control(){std::uint16_t value;asm volatile("fnstcw %0":"=m"(value));return value;}
std::uint16_t status(){std::uint16_t value;asm volatile("fnstsw %0":"=a"(value));return value;}
void run(const char* path) {
    std::ifstream input(path);check(bool(input),"Physical original ducking fixture absent");const auto fixture=J::parse(input);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Ducking reference image changed");
    check(fixture.at("backend")=="unchanged-tu15-physical-i386-interleaved-inputs-stores-fp","Independent original ducking backend absent");
    unsigned cases{},reads{},stores{};
    for(const auto& row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();Memory memory;
        for(const auto& bank:row.at("banks"))memory.banks.emplace(bank.at("address"),decode(bank.at("before")));
        const auto stack=row.at("stack").get<std::uint32_t>();
        memory.banks.emplace(stack-0x1000u,std::vector<std::byte>(0x2000u));auto& data=memory.banks.at(stack-0x1000u);
        const auto stop=row.at("stop").get<std::uint32_t>();for(unsigned i=0;i<4;++i)data[0x1000u+i]=std::byte(stop>>(i*8));
        SourcePhysicsContactRuntimeAbi abi{};abi.gp=row.at("gp").get<decltype(abi.gp)>();abi.xmm=row.at("xmm").get<decltype(abi.xmm)>();
        const auto gp=abi.gp;const auto xmm=abi.xmm;abi.eflags=2;
        const auto before=row.at("before_fp");const SourcePhysicsFactoryFloatingPoint fp{before[0],before[2]};
        abi.floating_point=fp;abi.x87_status=before[1];std::vector<SourcePhysicsContactRecordStore> writes;
        SourcePhysicsContactRecordObserver observer=[&](const auto& event){writes.push_back(event);};
        SourcePhysicsContactRuntimeServices services;
        services.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("Ducking escaped its native dependency closure");};
        native_tu15_sound::CampaignContext context{memory,fp,observer,services};
        const auto host_cw=control(),host_sw=status();const auto host_mxcsr=_mm_getcsr();
        native_tu15_sound::campaign_ducking_adapter::run(context,abi);
        check(control()==host_cw&&status()==host_sw&&_mm_getcsr()==host_mxcsr,name+": leaked native ducking FP state");
        check(memory.reads.size()==row.at("reads").size(),name+": ordered ducking input count differs");
        for(unsigned i=0;i<memory.reads.size();++i) {
            const auto& actual=memory.reads[i];const auto& expected=row.at("reads")[i];
            check(actual.address==expected.at("address")&&actual.stores_before==expected.at("stores_before")&&encode(actual.bytes)==expected.at("value").get<std::string>(),
                name+": ordered ducking input differs at "+std::to_string(i));
        }
        check(writes.size()==row.at("writes").size(),name+": ordered ducking store count differs");
        for(unsigned i=0;i<writes.size();++i) {
            const auto& actual=writes[i];const auto& expected=row.at("writes")[i];
            check(actual.instruction==expected.at("instruction")&&actual.address==expected.at("address")&&
                encode(actual.before)==expected.at("before").get<std::string>()&&encode(actual.written)==expected.at("written").get<std::string>(),
                name+": ordered ducking store differs at "+std::to_string(i));
        }
        for(const auto& bank:row.at("banks"))check(encode(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+": retained ducking bank differs");
        const auto after=row.at("after_fp");const auto got=abi.floating_point.value();
        check(got.x87_control_word==after[0]&&abi.x87_status==after[1]&&got.mxcsr==after[2],name+": physical ducking FP differs "+J::array({got.x87_control_word,abi.x87_status,got.mxcsr}).dump()+"/"+after.dump());
        for(unsigned reg:{3u,5u,6u,7u})check(abi.gp[reg]==gp[reg],name+": callee-saved ducking GPR differs");
        for(unsigned reg:{4u,5u,6u,7u})check(abi.xmm[reg]==xmm[reg],name+": preserved ducking SIMD differs");
        check(abi.gp[4]==stack+4u,name+": ducking return stack differs");
        ++cases;reads+=memory.reads.size();stores+=writes.size();
    }
    check(cases>0,"Empty physical ducking corpus");std::cout<<cases<<" complete original campaign ducking calls, "<<reads<<" ordered inputs and "<<stores<<" ordered stores with exact physical FP passed\n";
}
}
int main(int argc,char** argv)try{check(argc==2,"Supply original physical ducking fixture");run(argv[1]);return 0;}
catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
