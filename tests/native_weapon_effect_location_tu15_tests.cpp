#include "halo2/native_weapon_effect_dispatch_tu15.hpp"
#include "native_tu15_sound_test_support.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
#include <immintrin.h>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void need(bool ok,const std::string &why){if(!ok)throw std::runtime_error(why);}
std::vector<std::byte> unhex(const std::string &s) {
    std::vector<std::byte> bytes;
    for(unsigned n=0;n<s.size();n+=2)bytes.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));
    return bytes;
}
std::array<std::uint32_t,4> lanes(const Json &raw) {
    auto bytes=unhex(raw.get<std::string>());std::array<std::uint32_t,4> result{};
    need(bytes.size()==16,"Invalid XMM reference");
    for(unsigned n=0;n<16;++n)result[n/4]|=std::to_integer<std::uint32_t>(bytes[n])<<(8*(n%4));
    return result;
}
}
int main(int argc,char **argv)try {
    need(argc==2,"Supply unchanged-original location reference");
    std::ifstream in(argv[1]);const auto fixture=Json::parse(in);
    need(fixture.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong original image");
    unsigned cases{},faults{},writes{};
    for(const auto &row:fixture.at("rows")) {
        try {
            sound_test::Memory memory;
            for(const auto &bank:row.at("banks")) {
                auto bytes=unhex(bank[1]);memory.map(bank[0],bytes.size());memory.write(bank[0],bytes);
            }
            for(const auto &v:row.at("memory")) {
                const auto at=v[0].get<std::uint32_t>(),value=v[1].get<std::uint32_t>();
                switch(v[2].get<unsigned>()) {
                case 1:memory.put8(at,value);break;case 2:memory.put16(at,value);break;case 4:memory.put32(at,value);break;
                default:throw std::runtime_error("Invalid reference width");
                }
            }
            SourcePhysicsWorldRuntimeAbi a{};a.gp=row.at("gp").get<decltype(a.gp)>();a.eflags=0x202;
            for(unsigned n=0;n<8;++n)a.xmm[n]=lanes(row.at("xmm")[n]);
            a.floating_point=SourcePhysicsFactoryFloatingPoint{0xa7f,row.at("mxcsr")};
            a.x87_depth=2;a.x87[0]=1.25L;a.x87[1]=-3.5L;a.x87_status=0x4311;
            unsigned stored{};
            SourcePhysicsContactRecordObserver observe=[&](const auto &event) {
                const auto &expected=row.at("stores").at(stored++);
                std::uint32_t value{};
                for(unsigned n=0;n<event.written.size();++n)value|=std::to_integer<std::uint32_t>(event.written[n])<<(n*8);
                need(event.instruction==expected[0] && event.address==expected[1] && event.written.size()==expected[2] && value==expected[3],
                    "Different store "+std::to_string(stored));++writes;
            };
            bool faulted{};const auto host=_mm_getcsr();
            try{native_weapon_effect_dispatch_tu15_run(memory,row.at("entry"),a,{},observe);}
            catch(const std::out_of_range &){faulted=true;++faults;}
            need(faulted!=row.at("completed").get<bool>(),"Different completion/fault");
            need(stored==row.at("stores").size(),"Omitted original stores");
            need(a.gp==row.at("expected_gp").get<decltype(a.gp)>(),"Different final/stopped GP");
            for(unsigned n=0;n<8;++n)need(a.xmm[n]==lanes(row.at("expected_xmm")[n]),"Different XMM"+std::to_string(n));
            need(a.floating_point->mxcsr==row.at("expected_mxcsr"),"Different MXCSR: "+std::to_string(a.floating_point->mxcsr)+" expected "+row.at("expected_mxcsr").dump());
            need(_mm_getcsr()==host,"Host SSE environment leaked");
            need(a.x87_depth==2 && a.x87[0]==1.25L && a.x87[1]==-3.5L && a.x87_status==0x4311 &&
                a.floating_point->x87_control_word==0xa7f,"Unrelated x87 state changed");
            for(const auto &bank:row.at("expected_banks")) {
                auto expected=unhex(bank[1]);std::vector<std::byte> actual(expected.size());memory.read(bank[0],actual);
                need(actual==expected,"Different output memory");
            }
        }catch(const std::exception &error){throw std::runtime_error("case "+std::to_string(cases)+": "+error.what());}
        ++cases;
    }
    need(cases==192 && faults==16,"Incomplete original location scope");
    std::cout<<"PASS "<<cases<<" unchanged-original location cases, "<<faults<<" stopped prefixes, "<<writes<<" exact stores\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
