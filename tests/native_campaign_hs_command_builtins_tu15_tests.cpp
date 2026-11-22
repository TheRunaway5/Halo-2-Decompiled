#include "native_campaign_hs_command_test_owners.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <sstream>
#include <xmmintrin.h>

namespace {
using namespace halo2;using Json=nlohmann::json;using u32=std::uint32_t;
constexpr u32 stack_low=0x2000000u,stack_size=0x40000u,stop=0x3000000u;
void check(bool value,const std::string &why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string &text){std::vector<std::byte> result;for(std::size_t i=0;i<text.size();i+=2)result.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));return result;}
std::string hex(std::span<const std::byte> data){const char digits[]="0123456789abcdef";std::string out;for(auto byte:data){auto n=std::to_integer<unsigned>(byte);out+=digits[n>>4];out+=digits[n&15];}return out;}
std::vector<std::byte> bank_bytes(const Json &bank,bool after=false){std::vector<std::byte> out(bank.at("size").get<std::size_t>());for(const auto &run:bank.at("before_runs")){const auto data=bytes(run[1]);std::ranges::copy(data,out.begin()+run[0].get<std::size_t>());}if(after)for(const auto &run:bank.at("after_runs")){const auto data=bytes(run[1]);std::ranges::copy(data,out.begin()+run[0].get<std::size_t>());}return out;}
struct Memory final:SourcePhysicsContactMemory {
    mutable std::map<u32,std::vector<std::byte>> banks;
    mutable std::vector<std::byte> stack=std::vector<std::byte>(stack_size);
    std::vector<SourcePhysicsContactRecordStore> stores;
    bool recording{};
    std::pair<std::vector<std::byte>*,u32> locate(u32 at,std::size_t size)const {
        auto it=banks.upper_bound(at);if(it!=banks.begin()){--it;if(std::uint64_t(at-it->first)+size<=it->second.size())return {&it->second,at-it->first};}
        if(at>=stack_low && std::uint64_t(at)+size<=stack_low+stack_size)return {&stack,at-stack_low};
        std::ostringstream why;why<<"missing native command storage "<<std::hex<<at<<" width "<<size;throw std::runtime_error(why.str());
    }
    void read(u32 at,std::span<std::byte> out)const override{const auto [data,offset]=locate(at,out.size());std::copy_n(data->begin()+offset,out.size(),out.begin());}
    void write(u32 at,std::span<const std::byte> value)override{const auto [data,offset]=locate(at,value.size());if(recording && data!=&stack)stores.push_back({0,at,{data->begin()+offset,data->begin()+offset+value.size()},{value.begin(),value.end()}});std::copy(value.begin(),value.end(),data->begin()+offset);}
    void word(u32 at,u32 value){std::array<std::byte,4> data{};for(unsigned i=0;i<4;++i)data[i]=std::byte(value>>(i*8));write(at,data);}
    explicit Memory(const Json &row){for(const auto &b:row.at("banks"))banks.emplace(b.at("address"),bank_bytes(b));}
};
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply physical HS command fixture");std::ifstream input(argv[1]);const auto fixture=Json::parse(input);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" && fixture.at("backend")=="physical-unchanged-i386-offline","Original authority changed");
    check(fixture.at("instruction_patches").empty() && !fixture.at("supplied_original_outcomes").get<bool>(),"Original result supplied");
    check(fixture.at("fixture_kind")=="native-hs-command-builtins-v1" || fixture.at("fixture_kind")=="native-hs-command-lifecycle-parents-v1","HS fixture kind differs");
    std::size_t count{},stores{},tagged{},refused{},producer{},checks{};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();check(row.at("physical_verified"),name+": original incomplete");Memory memory(row);SourcePhysicsWorldRuntimeAbi abi{};
        for(unsigned i=0;i<8;++i){abi.gp[i]=row.at("gp")[i];for(unsigned k=0;k<4;++k)abi.xmm[i][k]=row.at("xmm")[i][k];}
        abi.eflags=0x202u;abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};abi.x87_status=row.at("before_fp")[1];
        std::vector<u32> words{stop,row.at("matrix"),row.at("source")};for(const auto &a:row.at("stack_arguments"))words.push_back(a);
        for(std::size_t i=0;i<words.size();++i)memory.word(abi.gp[4]+u32(i)*4u,words[i]);memory.recording=true;
        SourcePhysicsContactRecordObserver observe=[&](const auto &event){const auto [data,offset]=memory.locate(event.address,event.written.size());(void)offset;if(data==&memory.stack)return;check(!memory.stores.empty() && memory.stores.back().address==event.address && memory.stores.back().written==event.written,name+": store observer mismatch");memory.stores.back().instruction=event.instruction;};
        const auto mxcsr=_mm_getcsr();std::uint16_t cw{},sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(cw),"=m"(sw));
        const auto entry=row.at("entry").get<u32>();u32 refusal{},refusal_site{};
        try{native_hs_command_test_call(memory,entry,abi,observe);}
        catch(const NativeCampaignAiPerformRequiredOwnerTu15 &error){refusal=error.entry();refusal_site=error.site();}
        catch(const std::exception &error){throw std::runtime_error(name+": "+error.what());}
        std::uint16_t after_cw{},after_sw{};asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(after_cw),"=m"(after_sw));check(cw==after_cw && sw==after_sw && mxcsr==_mm_getcsr(),name+": host FP leaked");
        const auto end=row.value("stopped_at",stop);
        if(end!=stop){check(refusal==end,name+": native refusal differs actual="+std::to_string(refusal)+" expected="+std::to_string(end)+"");
            if(end==0x211b10u)check(abi.gp[7]==row.at("boundary_gp")[7],name+": evaluator HS datum differs");
            check(abi.gp[4]==row.at("boundary_gp")[4],name+": native refusal ESP differs");
            std::array<std::byte,4> ret{};memory.read(abi.gp[4],ret);u32 value{};for(unsigned i=0;i<4;++i)value|=std::to_integer<u32>(ret[i])<<(i*8u);
            check(value==refusal_site+5u,name+": refusal return word differs");++refused;}
        else {
            check(!refusal,name+": unexpected native refusal "+std::to_string(refusal));const auto &gp=row.at("boundary_gp");const bool lifecycle=fixture.at("fixture_kind")=="native-hs-command-lifecycle-parents-v1";const u32 mask=lifecycle && entry!=0x211840u?0xffffu:0xffffffffu;
            check((abi.gp[0]&mask)==(gp[0].get<u32>()&mask),name+": result differs native="+std::to_string(abi.gp[0])+" original="+gp[0].dump());
            for(unsigned i:{1u,2u,3u,4u,5u,6u,7u})if(!lifecycle || i>=3u)check(abi.gp[i]==gp[i].get<u32>(),name+": GP differs index="+std::to_string(i)+" native="+std::to_string(abi.gp[i])+" original="+gp[i].dump());checks+=8;
            if(row.value("owned_entry",0u)==0x2b2340u) {
                constexpr u32 arithmetic_flags=0x8d5u;
                check((abi.eflags&arithmetic_flags)==(row.at("boundary_eflags").get<u32>()&arithmetic_flags),name+": arithmetic flags differ");
                ++checks;
            }
        }
        for(unsigned i=0;i<8;++i)for(unsigned k=0;k<4;++k)check(abi.xmm[i][k]==row.at("boundary_xmm")[i][k],name+": XMM differs");
        check(abi.floating_point->x87_control_word==row.at("after_fp")[0] && abi.x87_status==row.at("after_fp")[1] && abi.floating_point->mxcsr==row.at("after_fp")[2],name+": FP differs");++checks;
        for(const auto &b:row.at("banks")) {
            const auto expected=bank_bytes(b,true);const auto &actual=memory.banks.at(b.at("address"));
            if(expected!=actual){std::size_t n{};while(n<expected.size() && actual[n]==expected[n])++n;std::ostringstream why;why<<name<<": DATA mismatch "<<std::hex<<b.at("address").get<u32>()+n<<" native="<<std::to_integer<unsigned>(actual[n])<<" original="<<std::to_integer<unsigned>(expected[n]);throw std::runtime_error(why.str());}
        }
        const auto &writes=row.at("writes");check(memory.stores.size()==writes.size(),name+": store count differs native="+std::to_string(memory.stores.size())+" original="+std::to_string(writes.size()));
        for(std::size_t n=0;n<writes.size();++n){const auto &a=memory.stores[n];const auto &b=writes[n];const u32 original_pc=b.at("instruction");
            const bool pc=a.instruction==original_pc;
            check(pc && a.address==b.at("address") && hex(a.before)==b.at("before").get<std::string>() && hex(a.written)==b.at("written").get<std::string>(),name+": ordered store differs "+std::to_string(n)+" native pc="+std::to_string(a.instruction)+" address="+std::to_string(a.address)+" before="+hex(a.before)+" written="+hex(a.written)+" original "+b.dump());tagged+=a.instruction!=0;}
        stores+=writes.size();++count;
    }
    std::cout<<Json{{"passed",true},{"cases",count},{"producer_regions",producer},{"named_refusals",refused},{"ordered_stores",stores},{"tagged_store_pcs",tagged},{"gp_fp_assertions",checks},{"ordered_reads_claimed",false},{"private_child_frames_claimed",false}}.dump()<<'\n';return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
