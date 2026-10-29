#include "halo2/native_campaign_object_marker_request_tu15.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <xmmintrin.h>
namespace {
using namespace halo2;using J=nlohmann::json;
void check(bool value,const std::string &why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte> decode(const std::string &s){std::vector<std::byte>b;for(std::size_t n=0;n<s.size();n+=2)b.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return b;}
std::string encode(std::span<const std::byte>b){constexpr char d[]="0123456789abcdef";std::string s;for(auto v:b){const auto n=std::to_integer<unsigned>(v);s+=d[n>>4];s+=d[n&15];}return s;}
std::uint32_t integer(std::span<const std::byte> data,unsigned at,unsigned n=4){std::uint32_t v{};for(unsigned i=0;i<n;++i)v|=std::to_integer<unsigned>(data[at+i])<<(i*8u);return v;}
class Memory final:public SourcePhysicsContactMemory {
public:
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    mutable J reads=J::array();
    void read(std::uint32_t at,std::span<std::byte> output)const override {
        auto found=banks.upper_bound(at);check(found!=banks.begin(),"Private marker attempted unowned engine read");--found;
        check(std::uint64_t(at-found->first)+output.size()<=found->second.size(),"Private marker attempted private-output memory read");
        std::copy_n(found->second.begin()+at-found->first,output.size(),output.begin());
        reads.push_back(J{{"address",at},{"bytes",output.size()},{"before",encode(output)},{"stores_before",0}});
    }
    void write(std::uint32_t,std::span<const std::byte>)override {throw std::runtime_error("Native private marker wrote engine memory");}
};
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply independent original private-marker fixture");J fixture;std::ifstream(argv[1])>>fixture;
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&fixture.at("backend")=="physical-unchanged-i386-offline","Wrong private-marker authority");
    check(fixture.at("instruction_patches").empty()&&!fixture.at("supplied_original_outcomes").get<bool>()&&!fixture.at("whole_parent_executed").get<bool>(),"Supplied private marker outcome");
    check(fixture.at("private_value_profile")=="actual1EBCB0-BAD60-entrySP+1C-capacity1"&&fixture.at("bank_format")=="physical-complete-bank-intern-v1","Unqualified private marker layout");
    std::map<std::string,std::vector<std::byte>> payload;for(const auto &[digest,value]:fixture.at("bank_data").items())payload.emplace(digest,decode(value));
    unsigned cases{},reads{},private_reads{},stores{};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();check(row.at("completed").get<bool>()&&row.at("physical_verified").get<bool>(),"Original private marker did not complete");
        const auto first=row.at("private_output").at("address").get<std::uint32_t>(),last=first+0x70u;
        check(row.at("capacity")==1&&row.at("arguments")[2]==1&&row.at("gp")[3]==first&&first==row.at("gp")[4].get<std::uint32_t>()+0x1cu,"Original private marker capacity/layout differs");
        Memory memory;const std::vector<std::byte> *physical_output=nullptr;
        for(const auto &bank:row.at("banks")) {
            const auto at=bank.at("address").get<std::uint32_t>();
            if(at==first){physical_output=&payload.at(bank.at("after_ref"));check(physical_output->size()==0x70u,"Private marker record extent differs");}
            else {check(bank.at("before_ref")==bank.at("after_ref"),"Original private marker mutated engine memory");memory.banks.emplace(at,payload.at(bank.at("before_ref")));}
        }
        check(physical_output!=nullptr,"Original private marker value absent");const auto before=memory.banks;
        const auto mxcsr=_mm_getcsr();asm volatile("fld1; fldpi":::"st","st(1)");
        alignas(16)std::array<std::byte,512> ambient{},after{};asm volatile("fxsave %0":"=m"(ambient)::"memory");
        NativeTu15PhysicsFrameFp fp{row.at("before_fp")[0],row.at("before_fp")[1],row.at("before_fp")[2]};
        const auto value=native_campaign_object_marker_request_value_tu15(memory,row.at("object"),row.at("direct"),row.at("marker"),fp);
        asm volatile("fxsave %0":"=m"(after)::"memory");
        check(mxcsr==_mm_getcsr()&&std::equal(ambient.begin(),ambient.begin()+5,after.begin())&&std::equal(ambient.begin()+32,ambient.begin()+160,after.begin()+32),name+": private marker changed ambient FP");
        asm volatile("fstp %%st(0); fstp %%st(0)":::"st","st(1)");
        check(value.count==(row.at("boundary_gp")[0].get<std::uint32_t>()&0xffffu),name+": private marker count differs");
        check(value.value.node==integer(*physical_output,0,2),name+": private marker node differs");
        for(unsigned lane=0;lane<13;++lane) {
            check(value.value.local[lane]==integer(*physical_output,4u+lane*4u),name+": private marker local field differs at "+std::to_string(lane));
            check(value.value.world[lane]==integer(*physical_output,0x38u+lane*4u),name+": private marker world field differs at "+std::to_string(lane));
        }
        check(value.value.radius==integer(*physical_output,0x6cu),name+": private marker radius differs");
        check(J::array({fp.control_word,fp.status,fp.mxcsr})==row.at("after_fp")&&row.at("live_x87_unchanged").get<bool>(),name+": private marker FP differs");
        J expected=J::array();
        for(auto event:row.at("reads")) {
            const auto at=event.at("address").get<std::uint32_t>(),n=event.at("bytes").get<unsigned>();
            if(first<=at&&at+n<=last){++private_reads;continue;}
            check(std::uint64_t(at)+n<=first||at>=last,"Original read crosses private marker record");
            const auto preceding=event.at("stores_before").get<unsigned>();
            check(preceding<=row.at("writes").size(),"Original private marker interleaving differs");
            for(unsigned i=0;i<preceding;++i){const auto &write=row.at("writes")[i];const auto p=write.at("address").get<std::uint32_t>();check(first<=p&&p+write.at("written").get_ref<const std::string&>().size()/2<=last,"A preceding private marker store escaped its owner");}
            event.erase("instruction");event["stores_before"]=0;expected.push_back(event);
        }
        for(const auto &write:row.at("writes")){const auto at=write.at("address").get<std::uint32_t>();check(first<=at&&at+write.at("written").get_ref<const std::string&>().size()/2<=last,"Private marker store escaped its real output range");}
        check(memory.reads==expected,name+": private marker ordered engine reads differ");
        check(memory.banks==before,name+": private marker retained input differs");
        check(row.at("boundary_gp")[4].get<std::uint32_t>()==row.at("gp")[4].get<std::uint32_t>()+16u,"Original private marker return contract differs");
        ++cases;reads+=memory.reads.size();stores+=row.at("writes").size();
    }
    check(cases==113,"Private marker input matrix differs");std::cout<<cases<<" independent original private marker values, "<<reads<<" engine reads, "<<private_reads<<" private reads and "<<stores<<" private stores passed\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
