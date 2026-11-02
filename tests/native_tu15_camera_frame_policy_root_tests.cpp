#include "halo2/native_tu15_camera_frame_policy_root.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
using J=nlohmann::json;
void require(bool v,const char *why){if(!v)throw std::runtime_error(why);}
std::vector<std::byte> raw(const std::string &s){std::vector<std::byte> b;for(unsigned n=0;n<s.size();n+=2)b.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return b;}
std::string hex(std::span<const std::byte> b){constexpr char d[]="0123456789abcdef";std::string s;for(auto v:b){auto n=std::to_integer<unsigned>(v);s+=d[n>>4];s+=d[n&15];}return s;}
struct Memory:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::vector<std::byte>> banks;J changes=J::array();unsigned published{};
    auto locate(std::uint32_t p,unsigned n)const {
        auto at=banks.upper_bound(p);if(at!=banks.begin()){--at;if(std::uint64_t(p-at->first)+n<=at->second.size())return at;}
        throw std::runtime_error("missing actual frame-policy root memory");
    }
    void read(std::uint32_t p,std::span<std::byte> out)const override{const auto at=locate(p,out.size());std::copy_n(at->second.begin()+p-at->first,out.size(),out.begin());}
    void write(std::uint32_t p,std::span<const std::byte> in)override {
        const auto at=locate(p,in.size());std::copy(in.begin(),in.end(),banks.at(at->first).begin()+p-at->first);
        for(const auto &c:changes)if(c.at("after_store")==published){const auto b=raw(c.at("bytes"));const auto p=c.at("address").get<std::uint32_t>();const auto at=locate(p,b.size());std::copy(b.begin(),b.end(),banks.at(at->first).begin()+p-at->first);}++published;
    }
};
void check(const J &row,unsigned return_mode) {
    Memory m;for(const auto &b:row.at("banks"))m.banks.emplace(b.at("address"),raw(b.at("before")));m.changes=row.at("input_changes");
    SourcePhysicsWorldRuntimeAbi a{};a.gp=row.at("gp").get<decltype(a.gp)>();
    // The direct renderer obtains this real continuation from prepare_source_call.
    // Canonical original inputs/outputs remain unchanged; no callee result is supplied.
    if(return_mode!=1)m.banks.emplace(a.gp[4],return_mode==2?raw("01003060"):raw("00003060"));
    a.eflags=0x602u;a.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};a.x87_status=0x4000;
    a.x87_depth=4;for(unsigned n=0;n<8;++n){a.x87[n]=n+0.25L;for(unsigned k=0;k<4;++k)a.xmm[n][k]=0x12340000u+n*4u+k;}
    const auto before=a;J stores=J::array();bool missing{},wrong{};
    try{native_tu15_camera_frame_policy_root_run(m,a,[&](const auto &s){stores.push_back({{"instruction",s.instruction},{"address",s.address},{"before",hex(s.before)},{"written",hex(s.written)}});});}
    catch(const NativeTu15FramePolicyRootReturnRequired &e){wrong=true;require(e.actual_return()==0x60300001u,"wrong root continuation value lost");}
    catch(const std::runtime_error &){missing=true;}
    require(missing==(return_mode==1)&&wrong==(return_mode==2),"root completion/late continuation rejection differs");
    require(a.gp[4]==before.gp[4]+(return_mode?0u:4u),"root consumed an absent/wrong continuation");
    for(unsigned n=0;n<8;++n)if(n!=4)require(a.gp[n]==before.gp[n],"root reconstructed opaque GP");
    require(a.eflags==before.eflags&&a.xmm==before.xmm&&a.x87==before.x87&&a.x87_depth==before.x87_depth&&a.x87_status==before.x87_status&&a.floating_point->mxcsr==before.floating_point->mxcsr&&a.floating_point->x87_control_word==before.floating_point->x87_control_word,"root changed retained ABI policy");
    const auto &writes=row.at("writes");require(stores.size()==writes.size(),"root lost original semantic stores");
    for(unsigned n=0;n<stores.size();++n){const auto &a=stores[n];const auto &b=writes[n];require(a.at("instruction")==b.at("instruction")&&a.at("address")==b.at("address")&&a.at("before")==b.at("before")&&a.at("written")==b.at("written"),"root original store sequence differs");}
    for(const auto &b:row.at("banks"))require(m.banks.at(b.at("address"))==raw(b.at("after")),"root lost committed original bytes");
}
}
int main(int argc,char **argv)try {
    require(argc==2,"supply unchanged original policy fixture");std::ifstream f(argv[1]);const auto data=J::parse(f);
    require(data.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&data.at("instruction_patches").empty()&&!data.at("supplied_original_outcomes").get<bool>(),"root original authority differs");
    unsigned cases{};for(const auto &row:data.at("cases"))if(row.at("owner")==0x2c470u&&row.at("completed").get<bool>())for(unsigned ret=0;ret<3;++ret){check(row,ret);++cases;}
    require(cases>=36,"root policy profiles missing");std::cout<<cases<<" original-data root transports; genuine late RET/missing/wrong continuation and opaque ABI passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
