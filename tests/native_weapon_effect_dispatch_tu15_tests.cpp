#include "halo2/native_weapon_effect_dispatch_tu15.hpp"
#include "halo2/native_weapon_audio_route_tu15.hpp"
#include "native_tu15_sound_test_support.hpp"
#include <fstream>
#include <nlohmann/json.hpp>

using namespace halo2;
using Json=nlohmann::json;
namespace {
void need(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
std::vector<std::byte> unhex(const std::string &s) {
    std::vector<std::byte> bytes;
    for(unsigned i=0;i<s.size();i+=2)bytes.push_back(std::byte(std::stoul(s.substr(i,2),nullptr,16)));
    return bytes;
}
std::array<std::uint32_t,4> xmm(const Json &raw) {
    const auto bytes=unhex(raw.get<std::string>());std::array<std::uint32_t,4> words{};
    need(bytes.size()==16,"Bad original XMM fixture");
    for(unsigned n=0;n<16;++n)words[n/4]|=std::to_integer<std::uint32_t>(bytes[n])<<((n%4)*8);
    return words;
}
}
int main(int argc,char **argv)try {
    need(argc==2,"Supply independent TU1.5 dispatch reference");
    std::ifstream in(argv[1]);const auto fixture=Json::parse(in);
    need(fixture.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
        "Unexpected original weapon reference");
    unsigned cases{},calls{};
    for(const auto &row:fixture.at("rows")) {
        sound_test::Memory memory;memory.map(0x2000000,0x20000);
        for(const auto &item:row.at("memory")) {
            const auto at=item[0].get<std::uint32_t>(),value=item[1].get<std::uint32_t>();
            const auto width=item[2].get<unsigned>();
            if(width==1)memory.put8(at,std::uint8_t(value));else memory.put32(at,value);
        }
        SourcePhysicsWorldRuntimeAbi a{};a.gp=row.at("gp").get<decltype(a.gp)>();
        for(unsigned n=0;n<8;++n)a.xmm[n]=xmm(row.at("xmm")[n]);
        // These integer/bit-copy owners must also preserve unrelated FP state.
        a.x87_depth=2;a.x87[0]=1.25L;a.x87[1]=-3.5L;a.x87_status=0x4311;
        a.floating_point=SourcePhysicsFactoryFloatingPoint{0x0a7f,0xdfc0};
        memory.put32(a.gp[4],0x3000000);
        for(unsigned n=0;n<3;++n)memory.put32(a.gp[4]+4+n*4,row.at("arguments")[n]);
        const auto before=memory.clone();
        const auto entry_abi=a;
        unsigned called{};
        SourcePhysicsWorldRuntimeServices services;
        services.required_owner=[&](auto &m,const auto &call,auto &abi,const auto &) {
            const auto &expected=row.at("calls").at(called++);
            need(call.target==expected.at("target"),"Native selected a different effect/sound owner");
            need(!call.tail && abi.gp==expected.at("gp").template get<decltype(abi.gp)>(),
                "Native callee GP/stack ABI differs from unchanged original");
            need(abi.xmm[0]==xmm(expected.at("xmm0")),"Native effect control bits differ");
            const auto stack=unhex(expected.at("stack"));std::vector<std::byte> actual(stack.size());
            m.read(abi.gp[4],actual);need(actual==stack,"Native callee arguments differ");
            need(memory.get32(abi.gp[4])==call.instruction+5,"Native call return site differs");
            for(unsigned n=0;n<3;++n)abi.gp[n]=row.at("clobber")[n];
            if(call.target==0x17dff0u)abi.gp[0]=row.at("refresh_result");
            else if(call.target==0x179610u)abi.gp[0]=row.at("create_result");
            abi.gp[4]+=unsigned(stack.size());++calls;
        };
        const auto entry=row.at("entry").get<std::uint32_t>();
        need(native_weapon_effect_dispatch_tu15_owns_entry(entry),"Native dispatch is not registered");
        native_weapon_effect_dispatch_tu15_run(memory,entry,a,services);
        need(called==row.at("calls").size(),"Native omitted an original required owner");
        need(a.gp==row.at("expected_gp").get<decltype(a.gp)>(),"Native return GP ABI differs");
        for(unsigned n=0;n<8;++n)need(a.xmm[n]==xmm(row.at("expected_xmm")[n]),"Native return XMM differs");
        need(a.x87_depth==2 && a.x87[0]==1.25L && a.x87[1]==-3.5L && a.x87_status==0x4311 &&
            a.floating_point->x87_control_word==0x0a7f && a.floating_point->mxcsr==0xdfc0,
            "Native bit-copy owner modified unrelated FP state");
        if(!row.at("calls").empty()) {
            auto unavailable_memory=before.clone();auto unavailable_abi=entry_abi;bool stopped{};
            try {native_weapon_effect_dispatch_tu15_run(unavailable_memory,entry,unavailable_abi,{});}
            catch(const std::runtime_error &){stopped=true;}
            need(stopped,"Missing required effect owner manufactured a successful return");
            const auto &expected=row.at("calls").front();
            need(unavailable_abi.gp==expected.at("gp").get<decltype(a.gp)>() &&
                unavailable_abi.xmm[0]==xmm(expected.at("xmm0")),
                "Missing required owner lost the original stopped call/return-address ABI");
        }
        for(const auto &[page,bytes]:before.pages) {
            if(page>=0x2000 && page<0x2020)continue; // dead stack scratch only
            need(memory.pages.contains(page) && *memory.pages.at(page)==*bytes,"Native effect dispatch wrote gameplay data");
        }
        ++cases;
    }
    need(cases==512 && calls>100,"Incomplete original dispatch coverage");
    // Full salted identities: identical low-word indices in another generation
    // cannot route zoom or firing sounds to this local player.
    const std::array views{NativeWeaponAudioViewTu15{0x12340001,0x12340002,0x12340003},
        NativeWeaponAudioViewTu15{0x43210004,0x43210005,0x43210006}};
    for(unsigned n=0;n<views.size();++n) {
        for(auto owner:{views[n].unit,views[n].primary,views[n].secondary}) {
            need(native_weapon_audio_view_tu15(owner,views,true)==n,"Unit/weapon audio lost its local view");
            need(!native_weapon_audio_view_tu15(owner^0x10000u,views,true),"Foreign generation stole a local audio view");
        }
        need(!native_weapon_audio_view_tu15(views[n].unit,views),"Ordinary unit audio acquired a duplicate weapon route");
    }
    need(!native_weapon_audio_view_tu15(0xffffffffu,views) &&
        !native_weapon_audio_view_tu15(0xffffffffu,std::array{NativeWeaponAudioViewTu15{}}),"NONE acquired an audio view");
    std::cout<<"PASS "<<cases<<" unchanged-original dispatch/selection cases, "<<calls
        <<" exact required-owner calls; salted zoom/primary/offhand routing\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
