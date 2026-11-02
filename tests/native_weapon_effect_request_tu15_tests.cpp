#include "halo2/native_weapon_effect_dispatch_tu15.hpp"
#include "native_tu15_sound_test_support.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
using namespace halo2;
using Json=nlohmann::json;
namespace {
constexpr std::uint32_t stack=0x2010000;
void need(bool ok,const std::string &why){if(!ok)throw std::runtime_error(why);}
std::vector<std::byte> unhex(const std::string &s) {
    std::vector<std::byte> b;for(unsigned n=0;n<s.size();n+=2)b.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return b;
}
std::array<std::uint32_t,4> xmm(const Json &j) {
    auto b=unhex(j.get<std::string>());std::array<std::uint32_t,4> a{};
    need(b.size()==16,"Invalid XMM reference");
    for(unsigned n=0;n<16;++n)a[n/4]|=std::to_integer<std::uint32_t>(b[n])<<((n%4)*8);return a;
}
void compare_bytes(const SourcePhysicsContactMemory &m,std::uint32_t at,const Json &raw,const char *why) {
    auto expected=unhex(raw.get<std::string>());std::vector<std::byte> actual(expected.size());m.read(at,actual);
    need(actual==expected,why);
}
}
int main(int argc,char **argv)try {
    need(argc==2,"Supply unchanged-original request reference");std::ifstream in(argv[1]);auto fixture=Json::parse(in);
    need(fixture.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong original image");
    unsigned cases{},calls{};
    for(const auto &row:fixture.at("rows")) {
        const auto entry=row.at("entry").get<std::uint32_t>();
        const auto body=row.value("body_address",0x1002000u+3*0x70u);
        try {
            sound_test::Memory m;m.map(stack-4096,8192);m.map(body,12);
            for(const auto &v:row.at("memory")) {
                auto at=v[0].get<std::uint32_t>(),value=v[1].get<std::uint32_t>();auto width=v[2].get<unsigned>();
                if(width==1)m.put8(at,value);else if(width==2)m.put16(at,value);else m.put32(at,value);
            }
            m.write(stack-512,unhex(row.at("initial_stack")));
            SourcePhysicsWorldRuntimeAbi a{};a.gp=row.at("gp").get<decltype(a.gp)>();a.eflags=row.at("eflags");
            for(unsigned n=0;n<8;++n)a.xmm[n]=xmm(row.at("xmm")[n]);
            a.x87_depth=2;a.x87[0]=1.25L;a.x87[1]=-3.5L;a.x87_status=0x4311;
            a.floating_point=SourcePhysicsFactoryFloatingPoint{0xa7f,0xdfc0};
            auto before=m.clone();auto entry_abi=a;unsigned called{},stored{};
            SourcePhysicsContactRecordObserver observe=[&](const auto &event) {
                const auto &expected=row.at("stores").at(stored++);
                std::uint32_t value{};for(unsigned n=0;n<event.written.size();++n)value|=std::to_integer<std::uint32_t>(event.written[n])<<(8*n);
                need(event.instruction==expected[0] && event.address==expected[1] && event.written.size()==expected[2] && value==expected[3],
                    "Native instruction-tagged write differs at store "+std::to_string(stored));
            };
            SourcePhysicsWorldRuntimeServices services;
            services.required_owner=[&](auto &memory,const auto &c,auto &abi,const auto &) {
                const auto &expected=row.at("calls").at(called++);
                need(!c.tail && c.target==expected.at("target"),"Different lower request owner");
                need(abi.gp==expected.at("gp").template get<decltype(abi.gp)>(),"Required GP ABI differs");
                for(unsigned n=0;n<8;++n)need(abi.xmm[n]==xmm(expected.at("xmm")[n]),"Required XMM ABI differs");
                compare_bytes(memory,stack-512,expected.at("stack"),"Required request/stack bytes differ");
                need(m.get32(abi.gp[4])==c.instruction+5,"Stopped original return address differs");
                for(const auto &w:expected.at("writes"))memory.write(w[0],unhex(w[1]));
                for(unsigned n=0;n<3;++n)abi.gp[n]=row.at("clobber")[n];abi.xmm[0]=xmm(row.at("service_xmm0"));
                unsigned arguments{};
                switch(c.target) {
                case 0x17a1a0:case 0x178e30:case 0x18cdd0:case 0x18d540:arguments=1;break;
                case 0x17dce0:arguments=3;break;case 0x17b130:break;
                case 0x18c090:arguments=2;break;case 0x18c130:arguments=6;break;
                case 0xbbe00:case 0x1791c0:case 0x18be00:break;
                default:throw std::runtime_error("Undeclared service");
                }
                abi.gp[4]+=4+4*arguments;++calls;
            };
            need(native_weapon_effect_dispatch_tu15_owns_entry(entry),"Request kernel not registered");
            native_weapon_effect_dispatch_tu15_run(m,entry,a,services,observe);
            need(called==row.at("calls").size() && stored==row.at("stores").size(),"Omitted original calls/writes");
            need(a.gp==row.at("expected_gp").get<decltype(a.gp)>(),"Returned GP differs");
            for(unsigned n=0;n<8;++n)need(a.xmm[n]==xmm(row.at("expected_xmm")[n]),"Returned XMM differs");
            compare_bytes(m,stack-512,row.at("expected_stack"),"Returned stack bytes differ");
            compare_bytes(m,body,row.at("expected_body"),"Generation/refresh data differs");
            need(a.x87_depth==2 && a.x87[0]==1.25L && a.x87[1]==-3.5L && a.x87_status==0x4311 &&
                a.floating_point->x87_control_word==0xa7f && a.floating_point->mxcsr==0xdfc0,"Unrelated FP state changed");
            if(called) {
                auto unavailable=before.clone();auto abi=entry_abi;bool stopped{};
                try{native_weapon_effect_dispatch_tu15_run(unavailable,entry,abi,{});}catch(const std::runtime_error &){stopped=true;}
                need(stopped && abi.gp==row.at("calls")[0].at("gp").get<decltype(abi.gp)>(),"Missing service lost stopped ABI");
                compare_bytes(unavailable,stack-512,row.at("calls")[0].at("stack"),"Missing service lost original request");
            }
        } catch(const std::exception &e) {throw std::runtime_error("case "+std::to_string(cases)+" entry "+std::to_string(entry)+": "+e.what());}
        ++cases;
    }
    need((cases==144 && calls>100) || (cases==32 && calls>32),"Incomplete request reference");
    std::cout<<"PASS "<<cases<<" unchanged-original request cases, "<<calls<<" lower calls and all instruction-tagged writes\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
