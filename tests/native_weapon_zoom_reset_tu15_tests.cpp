#include "halo2/native_weapon_effect_dispatch_tu15.hpp"
#include "native_tu15_sound_test_support.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
using namespace halo2;
using Json=nlohmann::json;
namespace {
constexpr std::uint32_t stack=0x2010000;
void need(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
std::vector<std::byte> unhex(const std::string &s) {
    std::vector<std::byte> bytes;for(unsigned n=0;n<s.size();n+=2)bytes.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));
    return bytes;
}
std::array<std::uint32_t,4> lanes(const Json &s) {
    auto bytes=unhex(s);std::array<std::uint32_t,4> result{};need(bytes.size()==16,"Invalid XMM reference");
    for(unsigned n=0;n<16;++n)result[n/4]|=std::to_integer<std::uint32_t>(bytes[n])<<((n%4)*8);return result;
}
void compare(const SourcePhysicsContactMemory &m,std::uint32_t at,const Json &s) {
    auto expected=unhex(s);std::vector<std::byte> actual(expected.size());m.read(at,actual);need(actual==expected,"Different live/dead stack bytes");
}
}
int main(int argc,char **argv)try {
    need(argc==2,"Supply unchanged-original CABC0 reference");std::ifstream in(argv[1]);const auto reference=Json::parse(in);
    need(reference.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong original image");
    unsigned cases{},resets{},tail_calls{};
    for(const auto &row:reference.at("rows"))try {
        sound_test::Memory m;m.map(stack-4096,8192);m.write(stack-64,unhex(row.at("initial_stack")));
        for(const auto &v:row.at("memory")) {
            const auto at=v[0].get<std::uint32_t>(),value=v[1].get<std::uint32_t>();
            if(v[2]==1)m.put8(at,value);else if(v[2]==2)m.put16(at,value);else m.put32(at,value);
        }
        SourcePhysicsWorldRuntimeAbi a{};a.gp=row.at("gp").get<decltype(a.gp)>();a.eflags=0x202;
        for(unsigned n=0;n<8;++n)a.xmm[n]=lanes(row.at("xmm")[n]);
        a.x87_depth=2;a.x87[0]=1.25L;a.x87[1]=-3.5L;a.x87_status=0x4311;
        a.floating_point=SourcePhysicsFactoryFloatingPoint{0xa7f,0xdfc0};
        auto expected_memory=m.clone(),before=m.clone();const auto before_abi=a;unsigned stored{},called{},reset{};
        SourcePhysicsContactRecordObserver observe=[&](const auto &event) {
            need(stored<row.at("stores").size(),"Different store count");const auto &expected=row.at("stores").at(stored++);
            std::uint32_t value{};for(unsigned n=0;n<event.written.size();++n)value|=std::to_integer<std::uint32_t>(event.written[n])<<(8*n);
            need(event.instruction==expected[0] && event.address==expected[1] && event.written.size()==expected[2] && value==expected[3],"Different original store");
            std::vector<std::byte> prior(event.written.size());expected_memory.read(event.address,prior);need(prior==event.before,"Different store preimage");
            expected_memory.write(event.address,event.written);
        };
        SourcePhysicsWorldRuntimeServices services;
        services.required_owner=[&](auto &,const auto &c,auto &abi,const auto &) {
            need(called<row.at("calls").size(),"Different lower call count");const auto &expected=row.at("calls").at(called++);
            need(c.target==expected.at("target") && c.tail==expected.at("tail"),"Different lower/tail owner");
            const auto ret=m.get32(abi.gp[4]);need(c.instruction==(c.tail?0xcad2cu:ret-5u),"Different call instruction");
            need(abi.gp==expected.at("gp").template get<decltype(abi.gp)>(),"Different required GP ABI");
            for(unsigned n=0;n<8;++n)need(abi.xmm[n]==lanes(expected.at("xmm")[n]),"Different required XMM ABI");
            compare(m,stack-64,expected.at("stack"));
            for(unsigned n=0;n<3;++n)abi.gp[n]=expected.at("returned_gp")[n];abi.xmm[0]=lanes(expected.at("returned_xmm0"));
            abi.gp[4]+=c.target==0x106040?4:8;tail_calls+=c.tail;
        };
        NativeWeaponZoomResetObserverTu15 on_reset=[&](const auto &event) {
            need(reset<row.at("events").size(),"Different reset event count");const auto &e=row.at("events").at(reset++);
            need(stored==e.at("committed_stores"),"Reset published before/after its original commit boundary");
            need(event==NativeWeaponZoomResetTu15{e.at("unit"),e.at("player"),e.at("weapon"),e.at("previous_selector"),
                e.at("request"),e.at("weapon_scoped"),e.at("source_local_user")},"Different committed reset decision");++resets;
        };
        native_weapon_effect_dispatch_tu15_run(m,0xcabc0,a,services,observe,on_reset);
        need(stored==row.at("stores").size() && called==row.at("calls").size(),"Different final store/call count");
        need(reset==row.at("events").size(),"Different reset event count");
        need(a.gp==row.at("expected_gp").get<decltype(a.gp)>(),"Different final GP");
        for(unsigned n=0;n<8;++n)need(a.xmm[n]==lanes(row.at("expected_xmm")[n]),"Different final XMM");
        compare(m,stack-64,row.at("expected_stack"));
        need(m.pages.size()==expected_memory.pages.size(),"Different final memory extent");
        for(const auto &[page,bytes]:m.pages)need(*bytes==*expected_memory.pages.at(page),"Different final memory page");
        need(a.x87_depth==2 && a.x87[0]==1.25L && a.x87[1]==-3.5L && a.x87_status==0x4311 &&
            a.floating_point->x87_control_word==0xa7f && a.floating_point->mxcsr==0xdfc0,"Unrelated FP state changed");
        auto stopped=before_abi;bool unavailable{};
        try{native_weapon_effect_dispatch_tu15_run(before,0xcabc0,stopped,{});}catch(const std::runtime_error &){unavailable=true;}
        need(unavailable && stopped.gp==row.at("calls")[0].at("gp").get<decltype(stopped.gp)>(),"Missing lower owner lost stopped ABI");
        compare(before,stack-64,row.at("calls")[0].at("stack"));++cases;
    }catch(const std::exception &e){throw std::runtime_error("case "+std::to_string(cases)+": "+e.what());}
    need(cases==144 && resets==92 && tail_calls==44,"Incomplete original reset scope");
    std::cout<<"PASS "<<cases<<" unchanged CABC0 cases, "<<resets<<" committed resets, "<<tail_calls<<" original sound tail calls\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
