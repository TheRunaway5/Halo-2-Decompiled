#include "halo2/native_tu15_data_array_iteration.hpp"
#include "halo2/native_campaign_ai_cancel_support_tu15.hpp"
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
    const NativeTu15DataArrayIterator *local{};mutable std::vector<std::array<std::uint32_t,4>> states_at_reads;
    struct Write {std::uint32_t address;std::vector<std::byte> before,written;};
    std::vector<Write> writes;bool capture_reads=true;
    auto locate(std::uint32_t address,std::size_t size)const {
        auto found=banks.upper_bound(address);check(found!=banks.begin(),"Cancel input owner absent");--found;
        check(std::uint64_t(address-found->first)+size<=found->second.size(),"Cancel input extent absent");return found;
    }
    void read(std::uint32_t address,std::span<std::byte> result)const override {
        const auto bank=locate(address,result.size());std::copy_n(bank->second.begin()+address-bank->first,result.size(),result.begin());
        if(capture_reads){reads.push_back({address,{result.begin(),result.end()},writes.size()});if(local)states_at_reads.push_back({local->current,local->pool,local->datum,local->cursor});}
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
    const auto entry=row.at("entry").get<std::uint32_t>();
    if(entry==0x16ea60)return native_tu15_data_array_next(memory,row.at("pool"),row.at("first"));
    if(entry==0x6ca40)return native_tu15_data_array_advance(memory,row.at("iterator").get<std::uint32_t>(),observe);
    if(entry==0xc1e90)return native_tu15_bit_array_test(memory,row.at("words"),row.at("index"));
    if(entry==0x1d0430)return native_campaign_ai_actor_timer_tu15(memory,row.at("actor"));
    if(entry==0x275ab0){native_campaign_ai_group_members_begin_tu15(memory,row.at("group"),row.at("iterator"),observe);return 0;}
    throw std::runtime_error("Unknown actual cancel prerequisite");
}

void run(const char *path) {
    std::ifstream input(path);check(bool(input),"Physical original cancel fixture absent");const auto fixture=J::parse(input);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Cancel original pin changed");
    check(fixture.at("backend")=="unchanged-tu15-physical-i386-interleaved-inputs-stores-fp","Physical original cancel backend absent");
    unsigned cases{},reads{},stores{},native_iterators{},native_heads{};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();Memory memory,diagnostic;
        for(const auto &bank:row.at("banks"))memory.banks.emplace(bank.at("address"),decode(bank.at("before")));
        diagnostic.banks=memory.banks;diagnostic.capture_reads=false;
        const auto cw=control(),sw=status();const auto mxcsr=_mm_getcsr();const auto result=invoke(row,memory);
        check(control()==cw&&status()==sw&&_mm_getcsr()==mxcsr,name+": native cancel changed caller FP environment");
        check(memory.reads.size()==row.at("reads").size(),name+": ordered cancel input count differs "+std::to_string(memory.reads.size())+"/"+std::to_string(row.at("reads").size()));
        for(unsigned i=0;i<memory.reads.size();++i) {
            const auto &actual=memory.reads[i];const auto &expected=row.at("reads")[i];
            check(actual.address==expected.at("address")&&actual.stores_before==expected.at("stores_before")&&encode(actual.value)==expected.at("value").get<std::string>(),name+": ordered cancel input differs at "+std::to_string(i));
        }
        check(memory.writes.size()==row.at("writes").size(),name+": ordered cancel store count differs");
        for(unsigned i=0;i<memory.writes.size();++i) {
            const auto &actual=memory.writes[i];const auto &expected=row.at("writes")[i];
            check(actual.address==expected.at("address")&&encode(actual.before)==expected.at("before").get<std::string>()&&encode(actual.written)==expected.at("written").get<std::string>(),name+": ordered cancel store differs at "+std::to_string(i));
        }
        for(const auto &bank:row.at("banks"))check(encode(memory.banks.at(bank.at("address")))==bank.at("after").get<std::string>(),name+": cancel result differs");
        if(row.contains("native_local_iterator")) {
            const auto fields=row.at("native_local_iterator").get<std::array<std::uint32_t,4>>();NativeTu15DataArrayIterator state{fields[0],fields[1],fields[2],fields[3]};
            Memory local;local.banks=diagnostic.banks;local.local=&state;
            const auto advanced=native_tu15_data_array_advance(local,state);const auto address=row.at("iterator").get<std::uint32_t>();
            J engine_reads=J::array();for(const auto &input:row.at("reads"))if(input.at("address").get<std::uint32_t>()<address||input.at("address").get<std::uint32_t>()>=address+16u)engine_reads.push_back(input);
            const auto apply=[&](auto &values,unsigned count) {
                for(unsigned index=0;index<count;++index) {
                    const auto &store=row.at("writes")[index];const auto at=store.at("address").get<std::uint32_t>();
                    check(at>=address&&at+4u<=address+16u,"cancel result differs: native iterator is not disjoint");
                    const auto bytes=decode(store.at("written"));std::uint32_t word{};for(unsigned byte=0;byte<4;++byte)word|=std::to_integer<unsigned>(bytes[byte])<<(byte*8u);values[(at-address)/4u]=word;
                }
            };
            check(local.reads.size()==engine_reads.size(),name+": ordered cancel input count differs for native iterator");
            for(unsigned i=0;i<local.reads.size();++i) {
                const auto &actual=local.reads[i];const auto &expected=engine_reads[i];auto snapshot=fields;apply(snapshot,expected.at("stores_before"));
                check(actual.address==expected.at("address")&&actual.stores_before==0&&encode(actual.value)==expected.at("value").get<std::string>()&&local.states_at_reads[i]==snapshot,name+": ordered cancel input differs for native iterator");
            }
            auto final=fields;apply(final,row.at("writes").size());
            check(advanced==(result!=0)&&std::array{state.current,state.pool,state.datum,state.cursor}==final&&local.writes.empty()&&local.banks==diagnostic.banks,name+": cancel result differs for native iterator");++native_iterators;
        }
        if(row.at("entry")==0x275ab0) {
            Memory local;local.banks=diagnostic.banks;const auto head=native_campaign_ai_group_first_member_tu15(local,row.at("group"));
            check(head==row.at("after_gp")[0]&&local.reads.size()==memory.reads.size()&&local.writes.empty()&&local.banks==diagnostic.banks,name+": cancel result differs for native group head");
            for(unsigned i=0;i<local.reads.size();++i)check(local.reads[i].address==memory.reads[i].address&&local.reads[i].value==memory.reads[i].value,name+": ordered cancel input differs for native group head");++native_heads;
        }
        std::vector<SourcePhysicsContactRecordStore> observed;invoke(row,diagnostic,[&](const auto &event){observed.push_back(event);});
        check(observed.size()==row.at("writes").size(),name+": ordered cancel store observer count differs");
        for(unsigned i=0;i<observed.size();++i) {
            const auto &actual=observed[i];const auto &expected=row.at("writes")[i];
            check(actual.instruction==expected.at("instruction")&&actual.address==expected.at("address")&&encode(actual.before)==expected.at("before").get<std::string>()&&encode(actual.written)==expected.at("written").get<std::string>(),name+": ordered cancel store observer differs");
        }
        check(row.at("before_fp")==row.at("after_fp"),name+": original cancel FP environment differs");
        check(row.at("xmm")==row.at("after_xmm"),name+": original cancel live SIMD differs");
        check(row.at("before_x87")==row.at("after_x87"),name+": original cancel occupied x87 differs");
        if(row.at("entry")!=0x275ab0)check(result==row.at("after_gp")[0],name+": cancel result differs");
        for(unsigned reg:{3u,5u,6u,7u})check(row.at("gp")[reg]==row.at("after_gp")[reg],name+": original cancel preserved register differs");
        check(row.at("after_gp")[4].get<std::uint32_t>()==row.at("stack").get<std::uint32_t>()+4u+row.at("arguments").size()*4u,name+": original cancel return stack differs");
        ++cases;reads+=memory.reads.size();stores+=memory.writes.size();
    }
    check(cases>0,"Empty original cancel corpus");std::cout<<cases<<" original path cancel calls, "<<reads<<" ordered inputs and "<<stores<<" ordered stores, "<<native_iterators<<" native iterators and "<<native_heads<<" native group heads passed\n";
}
}
int main(int argc,char **argv)try{check(argc==2,"Supply original physical cancel fixture");run(argv[1]);return 0;}
catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
