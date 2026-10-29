#include "halo2/native_campaign_ai_speech_choice_tu15.hpp"
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
    mutable std::vector<Read> reads;
    struct Write {std::uint32_t address;std::vector<std::byte> before,written;};
    std::vector<Write> writes;bool capture_reads=true;
    const std::vector<NativeCampaignAiSpeechCandidate> *local{};
    mutable std::vector<std::vector<std::uint32_t>> weights_at_reads;
    auto locate(std::uint32_t address,std::size_t size)const {
        auto found=banks.upper_bound(address);check(found!=banks.begin(),"Speech input owner absent");--found;
        check(std::uint64_t(address-found->first)+size<=found->second.size(),"Speech input extent absent");return found;
    }
    void read(std::uint32_t address,std::span<std::byte> result)const override {
        const auto bank=locate(address,result.size());std::copy_n(bank->second.begin()+address-bank->first,result.size(),result.begin());
        if(capture_reads) {
            reads.push_back({address,{result.begin(),result.end()},writes.size()});
            if(local){std::vector<std::uint32_t> weights;for(const auto &candidate:*local)weights.push_back(candidate.weight_bits);weights_at_reads.push_back(std::move(weights));}
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
NativeCampaignAudioFp before_fp(const J &row){return {row.at("before_fp")[0],row.at("before_fp")[1],row.at("before_fp")[2]};}
void verify_fp(const J &row,const NativeCampaignAudioFp &fp,const NativeCampaignAiSpeechChoice &result) {
    const auto name=row.at("name").get<std::string>();const auto &expected=row.at("after_fp");
    check(J::array({fp.control_word,fp.status,fp.mxcsr})==expected,name+": speech FP differs "+J::array({fp.control_word,fp.status,fp.mxcsr}).dump()+"/"+expected.dump());
    check(std::uint16_t(result.selected)==(row.at("after_gp")[0].get<std::uint32_t>()&0xffffu),name+": speech result differs");
    auto simd=row.at("xmm");
    if(result.fp_touched) {
        simd[0]=J::array({result.cumulative_bits,0,0,0});simd[1]=J::array({0,0,0,0});simd[2]=J::array({result.target_bits,0,0,0});
        if(result.inspected_score)simd[3]=J::array({result.last_term_bits,0,0,0});
    }
    check(simd==row.at("after_xmm"),name+": speech result differs for actual SIMD outcomes");
}
void run(const char *path) {
    std::ifstream input(path);check(bool(input),"Physical original speech fixture absent");const auto fixture=J::parse(input);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Speech original pin changed");
    check(fixture.at("backend")=="unchanged-tu15-physical-i386-interleaved-inputs-stores-fp","Physical original speech backend absent");
    unsigned cases{},reads{},stores{},locals{};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();Memory memory,diagnostic;
        for(const auto &bank:row.at("banks"))memory.banks.emplace(bank.at("address"),decode(bank.at("before")));
        diagnostic.banks=memory.banks;diagnostic.capture_reads=false;
        const auto address=row.at("candidates").get<std::uint32_t>();const auto count=row.at("count").get<std::int16_t>();
        auto fp=before_fp(row);const auto cw=control(),sw=status();const auto mxcsr=_mm_getcsr();
        const auto result=native_campaign_ai_speech_choice_tu15(memory,address,count,fp,row.at("capacity"));
        check(control()==cw&&status()==sw&&_mm_getcsr()==mxcsr,name+": native speech FP host leak");verify_fp(row,fp,result);
        check(memory.reads.size()==row.at("reads").size(),name+": ordered speech input count differs "+std::to_string(memory.reads.size())+"/"+std::to_string(row.at("reads").size()));
        for(unsigned i=0;i<memory.reads.size();++i) {
            const auto &actual=memory.reads[i];const auto &expected=row.at("reads")[i];
            check(actual.address==expected.at("address")&&actual.stores_before==expected.at("stores_before")&&encode(actual.value)==expected.at("value").get<std::string>(),name+": ordered speech input differs at "+std::to_string(i));
        }
        check(memory.writes.size()==row.at("writes").size(),name+": ordered speech store count differs");
        for(unsigned i=0;i<memory.writes.size();++i) {
            const auto &actual=memory.writes[i];const auto &expected=row.at("writes")[i];
            check(actual.address==expected.at("address")&&encode(actual.before)==expected.at("before").get<std::string>()&&encode(actual.written)==expected.at("written").get<std::string>(),name+": ordered speech store differs at "+std::to_string(i));
        }
        for(const auto &bank:row.at("banks"))check(encode(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+": speech result differs for retained bank");
        if(row.contains("native_local_candidates")) {
            const auto size=count>0?std::uint16_t(count):0u;std::vector<NativeCampaignAiSpeechCandidate> candidates(size);
            const auto bank=diagnostic.locate(address,size*20u);if(size)std::memcpy(candidates.data(),bank->second.data()+address-bank->first,size*20u);
            Memory local,expected;local.banks=expected.banks=diagnostic.banks;local.local=&candidates;auto local_fp=before_fp(row);
            const auto selected=native_campaign_ai_speech_choice_tu15(local,std::span(candidates),count,local_fp,row.at("capacity"));verify_fp(row,local_fp,selected);
            std::vector<NativeCampaignAiSpeechCandidate> staged(size);if(size)std::memcpy(staged.data(),bank->second.data()+address-bank->first,size*20u);
            const auto is_private=[&](std::uint32_t p){return p>=address&&std::uint64_t(p-address)<std::uint64_t(size)*20u;};
            J engine_reads=J::array();for(const auto &read:row.at("reads"))if(!is_private(read.at("address")))engine_reads.push_back(read);
            check(local.reads.size()==engine_reads.size(),name+": ordered speech input count differs for local candidates");
            for(unsigned n=0;n<engine_reads.size();++n) {
                auto expected_weights=staged;unsigned engine_stores{};const auto before=engine_reads[n].at("stores_before").get<unsigned>();
                for(unsigned i=0;i<before;++i) {
                    const auto &store=row.at("writes")[i];const auto at=store.at("address").get<std::uint32_t>();
                    if(is_private(at)) {const auto bytes=decode(store.at("written"));std::memcpy(reinterpret_cast<std::byte*>(expected_weights.data())+at-address,bytes.data(),bytes.size());}
                    else ++engine_stores;
                }
                std::vector<std::uint32_t> weights;for(const auto &candidate:expected_weights)weights.push_back(candidate.weight_bits);
                const auto &actual=local.reads[n];const auto &wanted=engine_reads[n];
                check(actual.address==wanted.at("address")&&actual.stores_before==engine_stores&&encode(actual.value)==wanted.at("value").get<std::string>()&&local.weights_at_reads[n]==weights,name+": ordered speech input differs for local candidates");
            }
            for(const auto &store:row.at("writes")) {
                const auto at=store.at("address").get<std::uint32_t>();const auto bytes=decode(store.at("written"));
                if(is_private(at))std::memcpy(reinterpret_cast<std::byte*>(staged.data())+at-address,bytes.data(),bytes.size());else expected.write(at,bytes);
            }
            check(candidates.size()==staged.size()&&(size==0||std::memcmp(candidates.data(),staged.data(),size*20u)==0)&&local.banks==expected.banks&&local.writes.size()==expected.writes.size(),name+": speech result differs for local candidates");++locals;
        }
        auto diagnostic_fp=before_fp(row);std::vector<SourcePhysicsContactRecordStore> observed;
        (void)native_campaign_ai_speech_choice_tu15(diagnostic,address,count,diagnostic_fp,row.at("capacity"),[&](const auto &event){observed.push_back(event);});
        check(observed.size()==row.at("writes").size(),name+": ordered speech store observer count differs");
        for(unsigned i=0;i<observed.size();++i) {
            const auto &actual=observed[i];const auto &expected=row.at("writes")[i];
            check(actual.instruction==expected.at("instruction")&&actual.address==expected.at("address")&&encode(actual.before)==expected.at("before").get<std::string>()&&encode(actual.written)==expected.at("written").get<std::string>(),name+": ordered speech store observer differs");
        }
        check(row.at("before_x87")==row.at("after_x87"),name+": original speech occupied x87 differs");
        for(unsigned reg:{3u,5u,6u,7u})check(row.at("gp")[reg]==row.at("after_gp")[reg],name+": original speech preserved register differs");
        check(row.at("after_gp")[4].get<std::uint32_t>()==row.at("stack").get<std::uint32_t>()+4u,name+": original speech return stack differs");
        ++cases;reads+=memory.reads.size();stores+=memory.writes.size();
    }
    check(cases>0,"Empty original speech corpus");std::cout<<cases<<" original speech choices, "<<reads<<" ordered reads, "<<stores<<" stores and "<<locals<<" local candidate profiles passed\n";
}
}
int main(int argc,char **argv)try{check(argc==2,"Supply original physical speech fixture");run(argv[1]);return 0;}
catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
