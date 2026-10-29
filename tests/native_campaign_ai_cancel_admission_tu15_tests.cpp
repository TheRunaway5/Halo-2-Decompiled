#include "halo2/native_campaign_ai_cancel_admission_tu15.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <xmmintrin.h>
namespace {
using namespace halo2;using J=nlohmann::json;
void check(bool value,const std::string &why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte> decode(const std::string &text) {
    std::vector<std::byte> result;result.reserve(text.size()/2);
    for(std::size_t i=0;i<text.size();i+=2)result.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
    return result;
}
std::string encode(std::span<const std::byte> bytes) {
    constexpr char digits[]="0123456789abcdef";std::string result;result.reserve(bytes.size()*2);
    for(auto byte:bytes){auto n=std::to_integer<unsigned>(byte);result+=digits[n>>4];result+=digits[n&15];}return result;
}
struct Memory final:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    struct Read {std::uint32_t address;std::vector<std::byte> value;std::size_t stores_before;};
    mutable std::vector<Read> reads;bool flip_ai{};mutable unsigned enable_reads{};
    struct Write {std::uint32_t address;std::vector<std::byte> before,written;};
    std::vector<Write> writes;bool capture_reads=true;
    auto locate(std::uint32_t address,std::size_t size)const {
        auto found=banks.upper_bound(address);check(found!=banks.begin(),"Admission input owner absent");--found;
        check(std::uint64_t(address-found->first)+size<=found->second.size(),"Admission input extent absent");return found;
    }
    void read(std::uint32_t address,std::span<std::byte> result)const override {
        const auto bank=locate(address,result.size());std::copy_n(bank->second.begin()+address-bank->first,result.size(),result.begin());
        if(flip_ai&&address==0x1000401u&&++enable_reads==2)result[0]=std::byte{1};
        if(capture_reads)reads.push_back({address,{result.begin(),result.end()},writes.size()});
    }
    void write(std::uint32_t address,std::span<const std::byte> input)override {
        const auto bank=locate(address,input.size());auto &owner=banks.at(bank->first);const auto offset=address-bank->first;
        writes.push_back({address,{owner.begin()+offset,owner.begin()+offset+input.size()},{input.begin(),input.end()}});
        std::copy(input.begin(),input.end(),owner.begin()+offset);
    }
};
std::uint16_t control(){std::uint16_t value;asm volatile("fnstcw %0":"=m"(value));return value;}
std::uint16_t status(){std::uint16_t value;asm volatile("fnstsw %0":"=a"(value));return value;}
std::uint32_t invoke(const J &row,Memory &memory,const SourcePhysicsContactRecordObserver &observe={}) {
    return native_campaign_ai_cancel_property_admission_campaign_tu15(memory,row.at("property"),row.at("actor"),row.at("group"));
}

void run(const char *path) {
    std::ifstream input(path);check(bool(input),"Physical original admission fixture absent");const auto fixture=J::parse(input);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Admission original pin changed");
    check(fixture.at("backend")=="unchanged-tu15-physical-i386-interleaved-inputs-stores-fp","Physical original admission backend absent");
    unsigned cases{},reads{},stores{};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();Memory memory,diagnostic;
        for(const auto &bank:row.at("banks"))memory.banks.emplace(bank.at("address"),decode(bank.at("before")));
        diagnostic.banks=memory.banks;diagnostic.capture_reads=false;
        const auto cw=control(),sw=status();const auto mxcsr=_mm_getcsr();const auto result=invoke(row,memory);
        check(control()==cw&&status()==sw&&_mm_getcsr()==mxcsr,name+": native admission changed caller FP environment");
        check(memory.reads.size()==row.at("reads").size(),name+": ordered admission input count differs "+std::to_string(memory.reads.size())+"/"+std::to_string(row.at("reads").size()));
        for(unsigned i=0;i<memory.reads.size();++i) {
            const auto &actual=memory.reads[i];const auto &expected=row.at("reads")[i];
            check(actual.address==expected.at("address")&&actual.stores_before==expected.at("stores_before")&&encode(actual.value)==expected.at("value").get<std::string>(),name+": ordered admission input differs at "+std::to_string(i));
        }
        check(memory.writes.size()==row.at("writes").size(),name+": ordered admission store count differs");
        for(unsigned i=0;i<memory.writes.size();++i) {
            const auto &actual=memory.writes[i];const auto &expected=row.at("writes")[i];
            check(actual.address==expected.at("address")&&encode(actual.before)==expected.at("before").get<std::string>()&&encode(actual.written)==expected.at("written").get<std::string>(),name+": ordered admission store differs at "+std::to_string(i));
        }
        for(const auto &bank:row.at("banks"))check(encode(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+": admission result differs");
        std::vector<SourcePhysicsContactRecordStore> observed;invoke(row,diagnostic,[&](const auto &event){observed.push_back(event);});
        check(observed.size()==row.at("writes").size(),name+": ordered admission store observer count differs");
        for(unsigned i=0;i<observed.size();++i) {
            const auto &actual=observed[i];const auto &expected=row.at("writes")[i];
            check(actual.instruction==expected.at("instruction")&&actual.address==expected.at("address")&&encode(actual.before)==expected.at("before").get<std::string>()&&encode(actual.written)==expected.at("written").get<std::string>(),name+": ordered admission store observer differs");
        }
        check(row.at("before_fp")==row.at("after_fp"),name+": original admission FP environment differs");
        check(row.at("xmm")==row.at("after_xmm"),name+": original admission live SIMD differs");
        check(row.at("before_x87")==row.at("after_x87"),name+": original admission occupied x87 differs");
        const auto mask=0xffu;
        check(result==(row.at("after_gp")[0].get<std::uint32_t>()&mask),name+": admission result differs");
        for(unsigned reg:{3u,5u,6u,7u})check(row.at("gp")[reg]==row.at("after_gp")[reg],name+": original admission preserved register differs");
        check(row.at("after_gp")[4].get<std::uint32_t>()==row.at("stack").get<std::uint32_t>()+4u+row.at("arguments").size()*4u,name+": original admission return stack differs");
        ++cases;reads+=memory.reads.size();stores+=memory.writes.size();
    }
    for(const auto &row:fixture.at("cases"))if(row.at("name")=="fallback-conflict-inactive") {
        Memory mode2,transition;for(const auto &bank:row.at("banks"))mode2.banks.emplace(bank.at("address"),decode(bank.at("before")));transition.banks=mode2.banks;
        auto &main=mode2.banks.at(0x1000000u);main[0x508]=std::byte{2};main[0x509]=main[0x50a]=main[0x50b]=std::byte{0};bool rejected=false;
        try{(void)invoke(row,mode2);}catch(const std::runtime_error &error){rejected=std::string(error.what()).find("mode2")!=std::string::npos;}check(rejected,"admission result differs: unknown mode2 policy was supplied");
        transition.flip_ai=true;rejected=false;
        try{(void)invoke(row,transition);}catch(const std::runtime_error &error){rejected=std::string(error.what()).find("inactive-to-active")!=std::string::npos;}check(rejected,"admission result differs: undefined iterator was supplied");
    }
    check(cases>0,"Empty original admission corpus");std::cout<<cases<<" original path admission calls, "<<reads<<" ordered inputs and "<<stores<<" ordered stores passed\n";
}
}
int main(int argc,char **argv)try{check(argc==2,"Supply original physical admission fixture");run(argv[1]);return 0;}
catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
