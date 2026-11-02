#include "halo2/native_weapon_effect_dispatch_tu15.hpp"
#include "native_tu15_sound_test_support.hpp"
#include "native_weapon_effect_compiled_route.hpp"
#include <cstring>
#include <fstream>
#include <set>
#include <nlohmann/json.hpp>
using namespace halo2;using Json=nlohmann::json;
namespace {
constexpr std::uint32_t stack=0x2010000;
void need(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
std::vector<std::byte> unhex(const std::string &s) {
    std::vector<std::byte> bytes;for(unsigned n=0;n<s.size();n+=2)bytes.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return bytes;
}
std::array<std::uint32_t,4> lanes(const Json &s) {
    const auto bytes=unhex(s);std::array<std::uint32_t,4> result{};need(bytes.size()==16,"Invalid XMM reference");
    std::memcpy(result.data(),bytes.data(),16);return result;
}
void load_fp(SourcePhysicsWorldRuntimeAbi &a,const Json &fp) {
    a.floating_point=SourcePhysicsFactoryFloatingPoint{fp.at("cw"),fp.at("mxcsr")};a.x87_status=fp.at("status");a.x87_depth=fp.at("stack").size();
    a.active_mmx.reset();
    if(fp.contains("mmx")) {
        SourcePhysicsMmxAlias mmx{};mmx.known_mask=255;
        for(unsigned n=0;n<8;++n){const auto raw=unhex(fp.at("mmx")[n]);need(raw.size()==10,"Invalid MMX reference");std::memcpy(mmx.physical[n].data(),raw.data(),10);}
        a.active_mmx=mmx;
    }
    for(unsigned n=0;n<a.x87_depth;++n){const auto raw=unhex(fp.at("stack")[n]);need(raw.size()==10,"Invalid extended reference");std::memcpy(&a.x87[n],raw.data(),10);}
    for(unsigned n=0;n<8;++n)a.xmm[n]=lanes(fp.at("xmm")[n]);
}
void check_fp(const SourcePhysicsWorldRuntimeAbi &a,const Json &fp) {
    need(a.floating_point && a.floating_point->x87_control_word==fp.at("cw") && a.floating_point->mxcsr==fp.at("mxcsr"),"Different original FP controls/sticky flags");
    need(a.x87_depth==fp.at("stack").size() && a.x87_status==fp.at("status"),"Different original x87 depth/status");
    for(unsigned n=0;n<a.x87_depth;++n){const auto raw=unhex(fp.at("stack")[n]);need(!std::memcmp(&a.x87[n],raw.data(),10),"Different original live extended lane");}
    for(unsigned n=0;n<8;++n)need(a.xmm[n]==lanes(fp.at("xmm")[n]),"Different original XMM");
    need(a.active_mmx.has_value()==fp.contains("mmx"),"Different original active MMX state");
    if(a.active_mmx) {
        need(a.active_mmx->known_mask==255,"Different original MMX known mask");
        for(unsigned n=0;n<8;++n){const auto raw=unhex(fp.at("mmx")[n]);need(!std::memcmp(a.active_mmx->physical[n].data(),raw.data(),10),"Different original physical MMX lane");}
    }
}
void check_abi(const SourcePhysicsWorldRuntimeAbi &a,const Json &expected,std::uint32_t mask) {
    need(a.gp==expected.at("gp").get<decltype(a.gp)>(),"Different original GP");
    need(!((a.eflags^expected.at("eflags").get<std::uint32_t>())&mask),"Different defined original flags");check_fp(a,expected.at("fp"));
}
void check_stack(const SourcePhysicsContactMemory &m,const Json &raw) {
    const auto expected=unhex(raw);std::vector<std::byte> actual(expected.size());m.read(stack-96,actual);need(actual==expected,"Different original live/dead stack");
}
struct Memory final:SourcePhysicsContactMemory {
    sound_test::Memory backing;
    std::set<std::uint32_t> write_only_sites;
    std::uint32_t pc{};
    bool capture{};
    mutable std::vector<std::array<std::uint32_t,3>> reads;
    void read(std::uint32_t at,std::span<std::byte> bytes)const override {
        // Store observers read preimages at write-only source instructions.
        // These are instrumentation reads, not original operand reads.
        if(capture && !write_only_sites.contains(pc))reads.push_back({pc,at,std::uint32_t(bytes.size())});
        backing.read(at,bytes);
    }
    void write(std::uint32_t at,std::span<const std::byte> bytes)override {backing.write(at,bytes);}
};
}

int main(int argc,char **argv)try {
    const NativeWeaponEffectTestRoute route(argc,argv);
    need(route.compiled() || argc==2,"Supply original selector reference");
    std::ifstream in(argv[1]);const auto reference=Json::parse(in);
    need(reference.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong original image");
    unsigned cases{},read_stops{},write_stops{},aliases{},mmx_cases{},read_count{},store_count{};
    for(const auto &row:reference.at("rows"))try {
        if(!route.accepts(row.at("entry")))continue;
        const auto entry=row.at("entry").get<std::uint32_t>();need(entry==0x106f90u,"Unknown original selector owner");
        Memory m;m.backing.map(stack-4096,8192);m.backing.write(stack-96,unhex(row.at("initial_stack")));
        m.write_only_sites=reference.at("write_only_sites").get<decltype(m.write_only_sites)>();
        for(const auto &v:row.at("memory")) {
            const auto at=v[0].get<std::uint32_t>(),value=v[1].get<std::uint32_t>();
            if(v[2]==1)m.backing.put8(at,value);else if(v[2]==2)m.backing.put16(at,value);else m.backing.put32(at,value);
        }
        auto expected_memory=m.backing.clone();
        SourcePhysicsWorldRuntimeAbi a{};a.gp=row.at("gp").get<decltype(a.gp)>();a.eflags=row.at("eflags");a.fs_base=0x76543210;load_fp(a,row.at("fp"));const auto before=a;
        unsigned stored{},traced{};const auto trace=unhex(row.at("trace_offsets"));
        SourcePhysicsContactRecordObserver observe=[&](const auto &event) {
            need(stored<row.at("stores").size(),"Different original selector store count");const auto &expected=row.at("stores").at(stored++);
            std::uint32_t value{};for(unsigned n=0;n<event.written.size();++n)value|=std::to_integer<std::uint32_t>(event.written[n])<<(8*n);
            need(event.instruction==expected[0] && event.address==expected[1] && event.written.size()==expected[2] && value==expected[3],"Different original selector store");
            std::vector<std::byte> prior(event.written.size());expected_memory.read(event.address,prior);need(prior==event.before,"Different original selector preimage");expected_memory.write(event.address,event.written);
        };
        SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](auto pc) {
            m.pc=pc;need(traced+2<=trace.size(),"Different original selector trace extent");
            const auto offset=std::to_integer<unsigned>(trace[traced])|(std::to_integer<unsigned>(trace[traced+1])<<8u);traced+=2;
            need(pc==entry+offset,"Different original selector instruction trace");
        };
        services.required_owner=[](auto &,const auto &,auto &,const auto &){throw std::runtime_error("Original selector leaf acquired a lower call");};
        bool stopped{};std::string error;m.capture=true;
        try {
            route.run(m,entry,a,services,observe);
        }catch(const std::out_of_range &e){stopped=true;error=e.what();}
        m.capture=false;
        need(stopped==!row.at("fault").empty(),"Different original selector stopped-access outcome");
        if(stopped) {
            const auto &fault=row.at("fault")[0];
            need(error.starts_with("unmapped ") && std::stoul(error.substr(9),nullptr,16)==fault.at("address").get<std::uint32_t>() && m.pc==fault.at("instruction"),"Different original selector stop address/site");
            if(fault.at("access")=="read")++read_stops;else ++write_stops;
        }
        need(stored==row.at("stores").size() && traced==trace.size(),"Different final original selector store/trace count");
        if(Json(m.reads)!=row.at("reads"))std::cerr<<"actual reads="<<Json(m.reads)<<" expected="<<row.at("reads")<<'\n';
        need(Json(m.reads)==row.at("reads"),"Different original ordered selector operand reads");
        check_abi(a,row.at("expected"),row.at("flags_mask"));check_stack(m,row.at("expected").at("stack"));need(a.fs_base==before.fs_base,"Unrelated source FS changed");
        for(const auto &[page,bytes]:m.backing.pages)need(*bytes==*expected_memory.pages.at(page),"Different final original selector memory page");
        const auto kind=row.at("kind").get<std::string>();aliases+=kind.starts_with("alias-");mmx_cases+=kind=="active-mmx";
        read_count+=m.reads.size();store_count+=stored;++cases;
    }catch(const std::exception &e){throw std::runtime_error("case "+std::to_string(cases)+": "+e.what());}
    need(route.scope(cases==268 && read_stops==8 && write_stops==0 && aliases==0 && mmx_cases==4,cases),"Incomplete physical original selector scope");
    std::cout<<"PASS "<<cases<<" physical original selector cases; "<<read_count<<" operand reads; "<<store_count<<" ordered stores\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

