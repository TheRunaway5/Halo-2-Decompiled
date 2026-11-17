#include "halo2/native_campaign_ai_biped_refresh_tu15.hpp"
#include "halo2/native_campaign_ai_object_navigation_tu15.hpp"
#include "halo2/native_tu15_camera_projection.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <xmmintrin.h>
namespace {
using namespace halo2;using J=nlohmann::json;
void check(bool value,const std::string &why){if(!value)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::string &s){std::vector<std::byte> out;for(std::size_t n=0;n<s.size();n+=2)out.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return out;}
std::string hex(std::span<const std::byte> values){const char*d="0123456789abcdef";std::string out;for(auto b:values){const auto n=std::to_integer<unsigned>(b);out+=d[n>>4];out+=d[n&15];}return out;}
std::vector<std::byte> bank_bytes(const J &bank,bool after){std::vector<std::byte> out(bank.at("size").get<std::size_t>());for(const auto &run:bank.at("before_runs")){auto raw=bytes(run[1]);std::copy(raw.begin(),raw.end(),out.begin()+run[0].get<unsigned>());}if(after)for(const auto &run:bank.at("after_runs")){auto raw=bytes(run[1]);std::copy(raw.begin(),raw.end(),out.begin()+run[0].get<unsigned>());}return out;}
struct Prefix{};
struct Memory final:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    mutable std::vector<std::byte> stack=std::vector<std::byte>(0x40000);
    bool retained(std::uint32_t p,std::size_t n)const {for(const auto &[base,data]:banks)if(p>=base&&std::uint64_t(p-base)+n<=data.size())return true;return false;}
    void read(std::uint32_t p,std::span<std::byte> out)const override {
        for(const auto &[base,data]:banks)if(p>=base&&std::uint64_t(p-base)+out.size()<=data.size()){std::copy_n(data.begin()+p-base,out.size(),out.begin());return;}
        if(p>=0x2000000u&&std::uint64_t(p)+out.size()<=0x2040000u){std::copy_n(stack.begin()+p-0x2000000u,out.size(),out.begin());return;}
        throw std::runtime_error("missing read "+std::to_string(p)+" bytes "+std::to_string(out.size()));
    }
    void write(std::uint32_t p,std::span<const std::byte> in)override {
        for(auto &[base,data]:banks)if(p>=base&&std::uint64_t(p-base)+in.size()<=data.size()){std::copy(in.begin(),in.end(),data.begin()+p-base);return;}
        if(p>=0x2000000u&&std::uint64_t(p)+in.size()<=0x2040000u){std::copy(in.begin(),in.end(),stack.begin()+p-0x2000000u);return;}
        throw std::runtime_error("missing write "+std::to_string(p));
    }
};
void private_first_write() {
    Memory memory;auto &transform=memory.banks[0x1000u];transform.resize(52);
    const auto put=[&](unsigned offset,std::uint32_t value){for(unsigned n=0;n<4;++n)transform[offset+n]=std::byte(value>>(8u*n));};
    for(auto offset:{0u,4u,20u,36u})put(offset,0x3f800000u);
    memory.banks[0x46a0d0u]=bytes("0000803f");memory.banks[0x2000u]=std::vector<std::byte>(52,std::byte{0x55});
    const std::array retained{NativeTu15ContactPrivateRange{0x1000u,52},NativeTu15ContactPrivateRange{0x2000u,52},NativeTu15ContactPrivateRange{0x46a0d0u,4}};
    const std::array owned{NativeTu15ContactPrivateRange{0x6000000u,64}};
    NativeTu15ContactCandidatePrivateData guarded(memory,owned,retained);unsigned events{};
    const SourcePhysicsContactRecordObserver observer=[&](const auto &e){++events;check(e.address>=0x2000u&&e.address+e.written.size()<=0x2034u,"Private inverse leaked its first write");check(std::all_of(e.before.begin(),e.before.end(),[](auto v){return v==std::byte{0x55};}),"Retained inverse preimage changed");};
    NativeTu15PhysicsFrameFp fp{0x27f,0,0x1f80};
    native_tu15_camera_inverse_frame(guarded,0x1000u,0x6000000u,fp,observer);
    check(events==0,"Private inverse became retained observation");
    std::array<std::byte,52> result{};guarded.read(0x6000000u,result);check(std::equal(result.begin(),result.end(),transform.begin()),"Private inverse DATA differs");
    native_tu15_camera_inverse_frame(guarded,0x1000u,0x2000u,fp,observer);check(events>0,"Retained inverse lost its observer");
}
}
int main(int argc,char **argv)try {
    private_first_write();
    check(argc==2,"Supply independent original biped refresh fixture");std::ifstream input(argv[1]);const auto fixture=J::parse(input);
    check(fixture.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" && fixture.at("backend")=="physical-unchanged-i386-biped-refresh-retained-v1","Original authority differs");
    check(fixture.at("instruction_patches").empty()&&!fixture.at("supplied_original_outcomes").get<bool>(),"Reference supplies outcomes");
    unsigned cases{},stores{},prefixes{};
    for(const auto &row:fixture.at("cases")) {
        const auto name=row.at("name").get<std::string>();Memory memory;std::vector<NativeTu15ContactPrivateRange> retained;
        for(const auto &bank:row.at("banks")){const auto p=bank.at("address").get<std::uint32_t>();auto data=bank_bytes(bank,false);retained.push_back({p,std::uint32_t(data.size())});memory.banks.emplace(p,std::move(data));}
        retained.push_back({0x2000000u,0x40000u});
        SourcePhysicsWorldRuntimeAbi abi{};for(unsigned n=0;n<8;++n)abi.gp[n]=row.at("gp")[n];
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{row.at("before_fp")[0],row.at("before_fp")[2]};abi.x87_status=row.at("before_fp")[1];
        std::vector<std::uint32_t> args{0x3000000u,row.at("matrix"),row.at("source")};for(const auto &v:row.at("stack_arguments"))args.push_back(v);
        for(unsigned n=0;n<args.size();++n){std::array<std::byte,4>b{};for(unsigned k=0;k<4;++k)b[k]=std::byte(args[n]>>(8u*k));memory.write(abi.gp[4]+n*4u,b);}
        const auto fault=row.value("fault",J::object());J actual=J::array();
        SourcePhysicsContactRecordObserver observer=[&](const auto &e){
            check(e.address<0x6000000u||e.address>=0x6010000u,name+": private DATA escaped observation");
            if(!memory.retained(e.address,e.written.size()))return;
            actual.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});
            if(!fault.empty()&&actual.size()==fault.at("ordinal").get<unsigned>()+1u)throw Prefix{};
        };
        const auto host_csr=_mm_getcsr();std::uint16_t host_cw{},host_sw{};asm volatile("fnstcw %0;fnstsw %1":"=m"(host_cw),"=a"(host_sw));
        bool stopped=false;
        try {NativeCampaignAiBipedRefreshScratchBindingTu15 binding(memory,0x6000000u,retained);const auto entry=row.at("entry").get<std::uint32_t>();if(entry==0xe2300u||entry==0x275cb0u)native_campaign_ai_object_navigation_call_tu15(memory,entry,abi,observer);else native_campaign_ai_biped_refresh_call_tu15(memory,entry,abi,observer);}
        catch(const Prefix&){stopped=true;}
        catch(const std::exception &e){throw std::runtime_error(name+": "+e.what());}
        check(stopped==!fault.empty(),name+": checkpoint differs");
        if(!stopped){check((abi.gp[0]&255u)==(row.at("boundary_gp")[0].get<unsigned>()&255u),name+": AL differs");for(auto n:{3u,4u,5u,6u,7u})check(abi.gp[n]==row.at("boundary_gp")[n],name+": saved GP differs "+std::to_string(n));}
        for(const auto &bank:row.at("banks"))check(memory.banks.at(bank.at("address"))==bank_bytes(bank,true),name+": retained bank differs at "+std::to_string(bank.at("address").get<unsigned>()));
        J expected=J::array();for(const auto&e:row.at("writes"))expected.push_back(J{{"instruction",e.at("instruction")},{"address",e.at("address")},{"before",e.at("before")},{"written",e.at("written")}});
        if(actual!=expected){std::size_t n=0;while(n<actual.size()&&n<expected.size()&&actual[n]==expected[n])++n;throw std::runtime_error(name+": store "+std::to_string(n)+" differs native="+(n<actual.size()?actual[n].dump():"end")+" original="+(n<expected.size()?expected[n].dump():"end"));}
        check(J::array({abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr})==row.at("after_fp"),name+": FP differs");
        std::uint16_t cw{},sw{};asm volatile("fnstcw %0;fnstsw %1":"=m"(cw),"=a"(sw));check(cw==host_cw&&sw==host_sw&&_mm_getcsr()==host_csr,name+": host FP leaked");
        stores+=actual.size();++cases;if(stopped)++prefixes;
    }
    check(cases>100&&prefixes>0,"Incomplete biped refresh cohort");std::cout<<"Native biped refresh passed "<<cases<<" cases, "<<stores<<" ordered retained stores, "<<prefixes<<" publication checkpoints\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
