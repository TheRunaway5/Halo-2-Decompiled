#include "audio/native_campaign_audio_retirement_adapter.hpp"
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
    struct Written {std::uint32_t address;std::vector<std::byte> before,written;};
    std::vector<Written> writes;unsigned store_count{};
    auto locate(std::uint32_t address,std::size_t size)const {
        auto found=banks.upper_bound(address);check(found!=banks.begin(),"Retirement input owner absent");--found;
        check(std::uint64_t(address-found->first)+size<=found->second.size(),"Retirement input extent absent");return found;
    }
    void read(std::uint32_t address,std::span<std::byte> result)const override {
        const auto bank=locate(address,result.size());std::copy_n(bank->second.begin()+address-bank->first,result.size(),result.begin());
        if(address<0x2000000u)reads.push_back({address,{result.begin(),result.end()},store_count});
    }
    void write(std::uint32_t address,std::span<const std::byte> input)override {
        const auto bank=locate(address,input.size());auto& owner=banks.at(bank->first);const auto offset=address-bank->first;
        writes.push_back({address,{owner.begin()+offset,owner.begin()+offset+input.size()},{input.begin(),input.end()}});
        std::copy(input.begin(),input.end(),owner.begin()+offset);
        ++store_count;
    }
};
std::uint16_t control(){std::uint16_t value;asm volatile("fnstcw %0":"=m"(value));return value;}
std::uint16_t status(){std::uint16_t value;asm volatile("fnstsw %0":"=a"(value));return value;}
void run(const char* path) {
    std::ifstream input(path);check(bool(input),"Physical original retirement fixture absent");const auto fixture=J::parse(input);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Retirement reference image changed");
    check(fixture.at("backend")=="unchanged-tu15-physical-i386-interleaved-inputs-stores-fp","Independent original retirement backend absent");
    unsigned cases{},reads{},stores{};
    for(const auto& row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();Memory memory;
        for(const auto& bank:row.at("banks"))memory.banks.emplace(bank.at("address"),decode(bank.at("before")));
        const auto stack=row.at("stack").get<std::uint32_t>();
        memory.banks.emplace(stack-0x1000u,std::vector<std::byte>(0x2000u));auto& data=memory.banks.at(stack-0x1000u);
        const auto stop=row.at("stop").get<std::uint32_t>();for(unsigned i=0;i<4;++i)data[0x1000u+i]=std::byte(stop>>(i*8));
        SourcePhysicsContactRuntimeAbi abi{};abi.gp=row.at("gp").get<decltype(abi.gp)>();abi.xmm=row.at("xmm").get<decltype(abi.xmm)>();
        abi.x87_depth=row.at("before_x87").size();
        for(unsigned index=0;index<abi.x87_depth;++index){const auto value=decode(row.at("before_x87")[index]);std::memcpy(&abi.x87[index],value.data(),10);}
        const auto gp=abi.gp;const auto xmm=abi.xmm;abi.eflags=2;
        const auto before=row.at("before_fp");const SourcePhysicsFactoryFloatingPoint fp{before[0],before[2]};
        abi.floating_point=fp;abi.x87_status=before[1];std::vector<SourcePhysicsContactRecordStore> writes;
        SourcePhysicsContactRecordObserver observer;
        SourcePhysicsContactRuntimeServices services;
        services.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("Retirement escaped its native dependency closure");};
        native_tu15_sound::CampaignContext context{memory,fp,observer,services};
        const auto host_cw=control(),host_sw=status();const auto host_mxcsr=_mm_getcsr();
        const auto entry=row.at("entry").get<std::uint32_t>();
        const auto arguments=row.at("arguments").get<std::vector<std::uint32_t>>();
        for(unsigned index=0;index<arguments.size();++index)for(unsigned byte=0;byte<4;++byte)data[0x1004u+index*4u+byte]=std::byte(arguments[index]>>(byte*8));
        native_tu15_sound::campaign_retirement_adapter::forward_direction(abi);
        if(entry==0x16e8a0u)native_tu15_sound::campaign_retirement_adapter::erase(context,abi);
        else {
            const auto entries=native_tu15_sound::campaign_retirement_routines();
            const auto selected=std::find_if(entries.begin(),entries.end(),[&](const auto& candidate){return candidate.address==entry;});
            check(selected!=entries.end()&&selected->semantic_original_proven,"Missing qualified actual retirement candidate");selected->run(context,abi);
        }
        const auto& actual_writes=memory.writes;
        check(control()==host_cw&&status()==host_sw&&_mm_getcsr()==host_mxcsr,name+": leaked native retirement FP state");
        check(memory.reads.size()==row.at("reads").size(),name+": ordered retirement input count differs "+std::to_string(memory.reads.size())+"/"+std::to_string(row.at("reads").size()));
        for(unsigned i=0;i<memory.reads.size();++i) {
            const auto& actual=memory.reads[i];const auto& expected=row.at("reads")[i];
            check(actual.address==expected.at("address")&&actual.stores_before==expected.at("stores_before")&&encode(actual.bytes)==expected.at("value").get<std::string>(),
                name+": ordered retirement input differs at "+std::to_string(i)+" actual "+std::to_string(actual.address)+"/"+std::to_string(actual.stores_before)+" expected "+expected.dump());
        }
        check(actual_writes.size()==row.at("writes").size(),name+": ordered retirement store count differs");
        for(unsigned i=0;i<actual_writes.size();++i) {
            const auto& actual=actual_writes[i];const auto& expected=row.at("writes")[i];
            check(actual.address==expected.at("address")&&
                encode(actual.before)==expected.at("before").get<std::string>()&&encode(actual.written)==expected.at("written").get<std::string>(),
                name+": ordered retirement store differs at "+std::to_string(i));
        }
        for(const auto& bank:row.at("banks"))check(encode(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+": retained retirement bank differs");
        const auto after=row.at("after_fp");const auto got=abi.floating_point.value();
        check(got.x87_control_word==after[0]&&abi.x87_status==after[1]&&got.mxcsr==after[2],name+": physical retirement FP differs "+J::array({got.x87_control_word,abi.x87_status,got.mxcsr}).dump()+"/"+after.dump());
        for(unsigned reg:{3u,5u,6u,7u})check(abi.gp[reg]==gp[reg],name+": callee-saved retirement GPR differs");
        check(abi.gp[4]==row.at("after_gp")[4],name+": retirement return stack differs");
        for(unsigned reg:{3u,5u,6u,7u})check(abi.gp[reg]==row.at("after_gp")[reg],name+": actual original nonvolatile differs");
        check(abi.x87_depth==row.at("after_x87").size(),name+": original occupied x87 depth differs");
        for(unsigned index=0;index<abi.x87_depth;++index)check(encode(std::span(reinterpret_cast<const std::byte*>(&abi.x87[index]),10))==row.at("after_x87")[index].get<std::string>(),name+": original occupied x87 operand differs");
        for(unsigned reg=0;reg<8;++reg)check(abi.xmm[reg]==row.at("after_xmm")[reg].get<std::array<std::uint32_t,4>>(),name+": actual SIMD contract differs");
        if(entry==0x16e8a0u)check(abi.gp[2]==row.at("after_gp")[2],name+": original pool owner EDX differs");
        ++cases;reads+=memory.reads.size();stores+=actual_writes.size();
    }
    check(cases>0,"Empty physical retirement corpus");std::cout<<cases<<" complete original campaign retirement calls, "<<reads<<" ordered inputs and "<<stores<<" ordered stores with exact physical FP passed\n";
}
}
int main(int argc,char** argv)try{check(argc==2,"Supply original physical retirement fixture");run(argv[1]);return 0;}
catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
