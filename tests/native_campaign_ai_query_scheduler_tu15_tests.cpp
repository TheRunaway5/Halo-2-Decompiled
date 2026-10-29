#include "halo2/native_campaign_ai_query_scheduler_tu15.hpp"
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
    mutable std::vector<Read> reads;
    struct Write {std::uint32_t address;std::vector<std::byte> before,written;};
    std::vector<Write> writes;bool capture_reads=true;
    auto locate(std::uint32_t address,std::size_t size)const {
        auto found=banks.upper_bound(address);check(found!=banks.begin(),"Scheduler input owner absent");--found;
        check(std::uint64_t(address-found->first)+size<=found->second.size(),"Scheduler input extent absent");return found;
    }
    void read(std::uint32_t address,std::span<std::byte> result)const override {
        const auto bank=locate(address,result.size());std::copy_n(bank->second.begin()+address-bank->first,result.size(),result.begin());
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
    const auto entry=row.at("entry").get<std::uint32_t>(),actor=row.at("actor").get<std::uint32_t>(),query=row.at("query").get<std::uint32_t>(),request=row.at("request").get<std::uint32_t>();
    const auto priority=row.at("priority").get<std::int16_t>();
    if(entry==0x2786d0)return std::uint16_t(native_campaign_ai_query_find_slot_tu15(memory,actor,query));
    if(entry==0x278720)return std::uint16_t(native_campaign_ai_query_select_slot_tu15(memory,actor,priority,observe));
    if(entry==0x278a50)native_campaign_ai_query_release_slot_tu15(memory,actor,row.at("slot"),observe);
    else if(entry==0x2787f0)return native_campaign_ai_query_reserve_member_tu15(memory,query,actor,priority,row.at("expiration_bits"),observe);
    else if(entry==0x278930)native_campaign_ai_query_broadcast_tu15(memory,request,row.at("arguments")[0],observe);
    else if(entry==0x278b50)native_campaign_ai_query_retire_tu15(memory,request,observe);
    else throw std::runtime_error("Unknown actual scheduler owner");return 0;
}

void run(const char *path) {
    std::ifstream input(path);check(bool(input),"Physical original scheduler fixture absent");const auto fixture=J::parse(input);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Scheduler original pin changed");
    check(fixture.at("backend")=="unchanged-tu15-physical-i386-interleaved-inputs-stores-fp","Physical original scheduler backend absent");
    unsigned cases{},reads{},stores{},typed_requests{};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();Memory memory,diagnostic;
        for(const auto &bank:row.at("banks"))memory.banks.emplace(bank.at("address"),decode(bank.at("before")));
        if(row.contains("stack_request"))memory.banks.emplace(row.at("request"),decode(row.at("stack_request")));
        diagnostic.banks=memory.banks;diagnostic.capture_reads=false;
        const auto cw=control(),sw=status();const auto mxcsr=_mm_getcsr();const auto result=invoke(row,memory);
        check(control()==cw&&status()==sw&&_mm_getcsr()==mxcsr,name+": native scheduler changed caller FP environment");
        check(memory.reads.size()==row.at("reads").size(),name+": ordered scheduler input count differs "+std::to_string(memory.reads.size())+"/"+std::to_string(row.at("reads").size()));
        for(unsigned i=0;i<memory.reads.size();++i) {
            const auto &actual=memory.reads[i];const auto &expected=row.at("reads")[i];
            check(actual.address==expected.at("address")&&actual.stores_before==expected.at("stores_before")&&encode(actual.value)==expected.at("value").get<std::string>(),name+": ordered scheduler input differs at "+std::to_string(i));
        }
        check(memory.writes.size()==row.at("writes").size(),name+": ordered scheduler store count differs");
        for(unsigned i=0;i<memory.writes.size();++i) {
            const auto &actual=memory.writes[i];const auto &expected=row.at("writes")[i];
            check(actual.address==expected.at("address")&&encode(actual.before)==expected.at("before").get<std::string>()&&encode(actual.written)==expected.at("written").get<std::string>(),name+": ordered scheduler store differs at "+std::to_string(i));
        }
        for(const auto &bank:row.at("banks"))check(encode(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+": scheduler result differs");
        if(row.contains("typed_request")) {
            Memory typed;typed.banks=diagnostic.banks;
            const auto &source=row.at("typed_request");const NativeCampaignAiQueryBroadcastRequest request{source.at("kind"),source.at("query"),source.at("expiration_bits")};
            native_campaign_ai_query_broadcast_tu15(typed,request,row.at("arguments")[0]);
            const auto address=row.at("request").get<std::uint32_t>();J expected_reads=J::array();
            for(const auto &read:row.at("reads"))if(read.at("address").get<std::uint32_t>()<address||read.at("address").get<std::uint32_t>()>=address+12u)expected_reads.push_back(read);
            for(const auto &store:row.at("writes")) {
                const auto at=store.at("address").get<std::uint32_t>();const auto width=store.at("written").get<std::string>().size()/2u;
                check(at+width<=address||at>=address+12u,name+": scheduler typed request is not disjoint");
            }
            check(typed.reads.size()==expected_reads.size(),name+": ordered scheduler input count differs for typed request");
            for(unsigned i=0;i<typed.reads.size();++i) {
                const auto &actual=typed.reads[i];const auto &expected=expected_reads[i];
                check(actual.address==expected.at("address")&&actual.stores_before==expected.at("stores_before")&&encode(actual.value)==expected.at("value").get<std::string>(),name+": ordered scheduler input differs for typed request");
            }
            check(typed.banks==memory.banks&&typed.writes.size()==memory.writes.size(),name+": scheduler result differs for typed request");
            for(unsigned i=0;i<typed.writes.size();++i) {
                const auto &actual=typed.writes[i];const auto &expected=memory.writes[i];
                check(actual.address==expected.address&&actual.before==expected.before&&actual.written==expected.written,name+": ordered scheduler store differs for typed request");
            }
            ++typed_requests;
        }
        const auto expected_writes=row.at("writes");J own_writes=J::array();for(const auto &store:expected_writes)if(store.at("instruction").get<std::uint32_t>()>=0x278000u)own_writes.push_back(store);
        std::vector<SourcePhysicsContactRecordStore> observed;invoke(row,diagnostic,[&](const auto &event){observed.push_back(event);});
        check(observed.size()==own_writes.size(),name+": ordered scheduler store observer count differs");
        for(unsigned i=0;i<observed.size();++i) {
            const auto &actual=observed[i];const auto &expected=own_writes[i];
            check(actual.instruction==expected.at("instruction")&&actual.address==expected.at("address")&&encode(actual.before)==expected.at("before").get<std::string>()&&encode(actual.written)==expected.at("written").get<std::string>(),name+": ordered scheduler store observer differs");
        }
        check(row.at("before_fp")==row.at("after_fp"),name+": original scheduler FP environment differs");
        const auto entry=row.at("entry").get<std::uint32_t>();
        if(entry==0x2786d0||entry==0x278720)check(result==(row.at("after_gp")[0].get<std::uint32_t>()&0xffffu),name+": scheduler result differs");
        if(entry==0x2787f0)check(result==(row.at("after_gp")[0].get<std::uint32_t>()&0xffu),name+": scheduler result differs");
        auto expected_simd=row.at("xmm");if(entry==0x2787f0&&result)expected_simd[0]=J::array({row.at("expiration_bits"),0,0,0});
        check(expected_simd==row.at("after_xmm"),name+": original scheduler live SIMD differs");
        check(row.at("before_x87")==row.at("after_x87"),name+": original scheduler occupied x87 differs");
        for(unsigned reg:{3u,5u,6u,7u})check(row.at("gp")[reg]==row.at("after_gp")[reg],name+": original scheduler required preserved register differs");
        const auto arguments=row.at("arguments").size();
        check(row.at("after_gp")[4].get<std::uint32_t>()==row.at("stack").get<std::uint32_t>()+4u+arguments*4u,name+": original scheduler return stack differs");
        ++cases;reads+=memory.reads.size();stores+=memory.writes.size();
    }
    check(cases>0,"Empty original scheduler corpus");std::cout<<cases<<" original path scheduler calls, "<<reads<<" ordered inputs and "<<stores<<" ordered stores and "<<typed_requests<<" disjoint native requests passed\n";
}
}
int main(int argc,char **argv)try{check(argc==2,"Supply original physical scheduler fixture");run(argv[1]);return 0;}
catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
