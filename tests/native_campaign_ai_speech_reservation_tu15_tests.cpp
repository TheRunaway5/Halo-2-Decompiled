#include "halo2/native_campaign_ai_speech_reservation_tu15.hpp"
#include <algorithm>
#include <cstring>
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
    for(std::size_t i=0;i<text.size();i+=2)result.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));return result;
}
std::string encode(std::span<const std::byte> bytes) {
    constexpr char digits[]="0123456789abcdef";std::string result;result.reserve(bytes.size()*2);
    for(auto byte:bytes){auto n=std::to_integer<unsigned>(byte);result+=digits[n>>4];result+=digits[n&15];}return result;
}
struct Memory final:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    struct Read {std::uint32_t address;std::vector<std::byte> value;std::size_t stores_before;};
    mutable std::vector<Read> reads;mutable std::uint32_t failed_read{};
    struct Write {std::uint32_t address;std::vector<std::byte> before,written;};
    std::vector<Write> writes;bool capture_reads=true;
    const NativeCampaignAiSpeechResolvedSound *local{};const std::uint32_t *local_prior{};
    mutable std::vector<std::array<std::uint32_t,2>> fields_at_reads;
    auto locate(std::uint32_t address,std::size_t size)const {
        auto found=banks.upper_bound(address);check(found!=banks.begin(),"Speech input owner absent");--found;
        check(std::uint64_t(address-found->first)+size<=found->second.size(),"Speech input extent absent");return found;
    }
    void read(std::uint32_t address,std::span<std::byte> result)const override {
        if(address==4u){failed_read=address;throw std::runtime_error("Original sound owner is uninitialized");}
        const auto bank=locate(address,result.size());std::copy_n(bank->second.begin()+address-bank->first,result.size(),result.begin());
        if(capture_reads) {
            reads.push_back({address,{result.begin(),result.end()},writes.size()});
            if(local)fields_at_reads.push_back({local->value,local_prior?*local_prior:0u});

        }
    }
    void write(std::uint32_t address,std::span<const std::byte> input)override {
        const auto bank=locate(address,input.size());auto &owner=banks.at(bank->first);const auto offset=address-bank->first;
        writes.push_back({address,{owner.begin()+offset,owner.begin()+offset+input.size()},{input.begin(),input.end()}});
        std::copy(input.begin(),input.end(),owner.begin()+offset);
    }
    std::uint32_t word(std::uint32_t address)const {
        const auto bank=locate(address,4);const auto &bytes=bank->second;std::uint32_t result{};
        for(unsigned n=0;n<4;++n)result|=std::to_integer<unsigned>(bytes[address-bank->first+n])<<(n*8u);return result;
    }
};
std::uint16_t control(){std::uint16_t value;asm volatile("fnstcw %0":"=m"(value));return value;}
std::uint16_t status(){std::uint16_t value;asm volatile("fnstsw %0":"=a"(value));return value;}
void run(const char *path) {
    std::ifstream input(path);check(bool(input),"Physical original speech fixture absent");const auto fixture=J::parse(input);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Speech original pin changed");
    check(fixture.at("backend")=="unchanged-tu15-physical-i386-interleaved-inputs-stores-fp","Physical original speech backend absent");
    unsigned cases{},reads{},stores{};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();Memory memory,diagnostic;
        for(const auto &bank:row.at("banks"))memory.banks.emplace(bank.at("address"),decode(bank.at("before")));
        diagnostic.banks=memory.banks;diagnostic.capture_reads=false;
        const auto cw=control(),sw=status();const auto mxcsr=_mm_getcsr();
        const auto kind=row.at("kind").get<std::string>();std::uint32_t value{};bool boolean=true;
        bool failed=false;try {
        if(kind=="width"){value=native_campaign_ai_sound_history_width_tu15(row.at("count"));boolean=false;}
        else if(kind=="prefix"){value=native_campaign_ai_sound_history_prefix_tu15(memory,row.at("range"),row.at("definition"));boolean=false;}
        else if(kind=="blocked")value=native_campaign_ai_sound_permutation_blocked_tu15(memory,row.at("sound"));
        else if(kind=="source")value=native_campaign_ai_speech_source_tu15(memory,row.at("unit"),row.at("action"),row.at("output").get<std::uint32_t>());
        else value=native_campaign_ai_speech_reserve_tu15(memory,row.at("unit"),row.at("action"),row.at("priority"),row.at("prior").get<std::uint32_t>(),row.at("output").get<std::uint32_t>());
        }catch(const std::runtime_error &){if(!row.value("fault",false))throw;failed=true;}
        check(failed==row.value("fault",false)&&(!failed||memory.failed_read==row.at("fault_address").get<std::uint32_t>()),name+": original fault prefix differs");
        check(control()==cw&&status()==sw&&_mm_getcsr()==mxcsr,name+": native speech FP host leak");
        check(failed||value==(row.at("after_gp")[0].get<std::uint32_t>()&(boolean?255u:0xffffffffu)),name+": speech result differs");
        check(row.at("before_fp")==row.at("after_fp")&&row.at("xmm")==row.at("after_xmm"),name+": original speech FP differs");
        check(memory.reads.size()==row.at("reads").size(),name+": ordered speech input count differs "+std::to_string(memory.reads.size())+"/"+std::to_string(row.at("reads").size()));
        for(unsigned i=0;i<memory.reads.size();++i) {
            const auto &actual=memory.reads[i];const auto &expected=row.at("reads")[i];
            check(actual.address==expected.at("address")&&actual.stores_before==expected.at("stores_before")&&encode(actual.value)==expected.at("value").get<std::string>(),name+": ordered speech input differs at "+std::to_string(i)+" actual="+J::array({actual.address,actual.stores_before,encode(actual.value)}).dump()+" wanted="+expected.dump());
        }
        check(memory.writes.size()==row.at("writes").size(),name+": ordered speech store count differs");
        for(unsigned i=0;i<memory.writes.size();++i) {
            const auto &actual=memory.writes[i];const auto &expected=row.at("writes")[i];
            check(actual.address==expected.at("address")&&encode(actual.before)==expected.at("before").get<std::string>()&&encode(actual.written)==expected.at("written").get<std::string>(),name+": ordered speech store differs at "+std::to_string(i));
        }
        for(const auto &bank:row.at("banks"))check(encode(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+": speech result differs for retained bank");
        if(row.contains("native_local_output")) {
            Memory local;local.banks=diagnostic.banks;
            const auto output=row.at("output").get<std::uint32_t>(),prior=row.at("prior").get<std::uint32_t>();
            NativeCampaignAiSpeechResolvedSound field{local.word(output)};auto reference=prior?local.word(prior):0u;
            local.local=&field;local.local_prior=prior?&reference:nullptr;
            const auto actual=kind=="source"?native_campaign_ai_speech_source_tu15(local,row.at("unit"),row.at("action"),field):native_campaign_ai_speech_reserve_tu15(local,row.at("unit"),row.at("action"),row.at("priority"),field,prior?&reference:nullptr);
            check(actual==bool(value),name+": speech result differs for native local output");
            const auto is_field=[&](std::uint32_t p){return p==output||(kind=="reserve"&&prior&&p==prior);};
            std::vector<J> engine;for(const auto &read:row.at("reads"))if(!is_field(read.at("address")))engine.push_back(read);
            check(local.reads.size()==engine.size(),name+": ordered speech input count differs for native local output");
            for(unsigned i=0;i<engine.size();++i) {
                auto expected_output=diagnostic.word(output),expected_prior=prior?diagnostic.word(prior):0u;
                for(unsigned n=0;n<engine[i].at("stores_before").get<unsigned>();++n) {
                    const auto &event=row.at("writes")[n];const auto p=event.at("address").get<std::uint32_t>();const auto bytes=decode(event.at("written"));std::uint32_t v{};std::memcpy(&v,bytes.data(),4);
                    if(p==output)expected_output=v;if(prior&&p==prior)expected_prior=v;
                }
                const auto &actual=local.reads[i];check(actual.address==engine[i].at("address")&&actual.stores_before==0&&encode(actual.value)==engine[i].at("value").get<std::string>()&&local.fields_at_reads[i]==std::array<std::uint32_t,2>{expected_output,prior?expected_prior:0u},name+": ordered speech input differs for local field publication");
            }
            check(field.value==memory.word(output)&&(kind=="source"||!prior||reference==memory.word(prior))&&local.writes.empty()&&local.banks==diagnostic.banks,name+": speech result differs for local field state");
        }
        check(row.at("before_x87")==row.at("after_x87"),name+": original speech occupied x87 differs");
        for(unsigned reg:{3u,5u,6u,7u})check(row.at("gp")[reg]==row.at("after_gp")[reg],name+": original speech preserved register differs");
        check(row.at("after_gp")[4].get<std::uint32_t>()==row.at("stack").get<std::uint32_t>()+(failed?0u:4u+std::uint32_t(row.at("arguments").size())*4u),name+": original speech return stack differs");
        ++cases;reads+=memory.reads.size();stores+=memory.writes.size();
    }
    check(cases>0,"Empty original speech corpus");std::cout<<cases<<" original speech choices, "<<reads<<" ordered reads, "<<stores<<" stores passed\n";
}
}
int main(int argc,char **argv)try{check(argc==2,"Supply original physical speech fixture");run(argv[1]);return 0;}
catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
