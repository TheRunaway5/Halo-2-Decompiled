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
    need(route.compiled() || argc==2,"Supply strict original dispatcher reference");std::ifstream in(argv[1]);const auto reference=Json::parse(in);
    need(reference.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong original image");
    unsigned cases{},read_stops{},mmx_cases{},called_total{},missing_stops{};
    const auto returned=[](auto &abi,const Json &state) {
        abi.gp=state.at("gp").template get<decltype(abi.gp)>();abi.eflags=state.at("eflags");load_fp(abi,state.at("fp"));
    };
    for(const auto &row:reference.at("rows"))try {
        if(!route.accepts(row.at("entry")))continue;
        constexpr auto entry=0x108a70u;Memory m;m.backing.map(stack-4096,8192);m.backing.write(stack-96,unhex(row.at("initial_stack")));
        m.write_only_sites=reference.at("write_only_sites").get<decltype(m.write_only_sites)>();
        for(const auto &blob:row.at("blobs")){const auto at=blob[0].get<std::uint32_t>();const auto raw=unhex(blob[1]);m.backing.map(at,raw.size());m.backing.write(at,raw);}
        for(const auto &v:row.at("memory")) {
            const auto at=v[0].get<std::uint32_t>(),value=v[1].get<std::uint32_t>();
            if(v[2]==1)m.backing.put8(at,value);else if(v[2]==2)m.backing.put16(at,value);else m.backing.put32(at,value);
        }
        auto initial=m.backing.clone(),expected_memory=m.backing.clone();
        SourcePhysicsWorldRuntimeAbi a{};a.gp=row.at("gp").get<decltype(a.gp)>();a.eflags=row.at("eflags");a.fs_base=0x76543210;load_fp(a,row.at("fp"));const auto before=a;
        unsigned stored{},traced{},called{};const auto trace=unhex(row.at("trace_offsets"));
        SourcePhysicsContactRecordObserver observe=[&](const auto &event) {
            need(stored<row.at("stores").size(),"Different strict dispatcher store count");const auto &expected=row.at("stores").at(stored++);
            std::uint32_t value{};for(unsigned n=0;n<event.written.size();++n)value|=std::to_integer<std::uint32_t>(event.written[n])<<(8*n);
            need(event.instruction==expected[0] && event.address==expected[1] && event.written.size()==expected[2] && value==expected[3],"Different strict dispatcher store");
            std::vector<std::byte> prior(event.written.size());expected_memory.read(event.address,prior);need(prior==event.before,"Different strict dispatcher preimage");expected_memory.write(event.address,event.written);
        };
        SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](auto pc) {
            m.pc=pc;need(traced+2<=trace.size(),"Different strict dispatcher trace extent");
            const auto bits=std::uint16_t(std::to_integer<unsigned>(trace[traced])|(std::to_integer<unsigned>(trace[traced+1])<<8u));traced+=2;
            const auto offset=std::int32_t(std::bit_cast<std::int16_t>(bits));need(pc==entry+std::uint32_t(offset),"Different strict dispatcher instruction trace");
        };
        services.required_owner=[&](auto &,const auto &call,auto &abi,const auto &) {
            need(called<row.at("calls").size(),"Different strict dispatcher callee count");const auto &expected=row.at("calls").at(called++);
            need(!call.tail && call.target==expected.at("target") && call.instruction==m.backing.get32(abi.gp[4])-5u,"Different strict dispatcher lower call/return ABI");
            check_abi(abi,expected.at("before"),expected.at("flags_mask"));check_stack(m.backing,expected.at("before").at("stack"));returned(abi,expected.at("returned"));++called_total;
        };
        m.capture=true;bool stopped{};std::string error;
        try{route.run(m,entry,a,services,observe);}catch(const std::out_of_range &e){stopped=true;error=e.what();}
        m.capture=false;need(stopped==!row.at("fault").empty(),"Different strict dispatcher stopped-read outcome");
        if(stopped) {
            const auto &fault=row.at("fault")[0];need(error.starts_with("unmapped ") && std::stoul(error.substr(9),nullptr,16)==fault.at("address").get<std::uint32_t>() && m.pc==fault.at("instruction"),"Different strict dispatcher fault address/site");++read_stops;
        }
        need(stored==row.at("stores").size() && traced==trace.size() && called==row.at("calls").size(),"Different final strict dispatcher store/trace/call count");
        if(Json(m.reads)!=row.at("reads"))std::cerr<<"actual reads="<<Json(m.reads)<<" expected="<<row.at("reads")<<'\n';
        need(Json(m.reads)==row.at("reads"),"Different strict dispatcher operand-read order");
        check_abi(a,row.at("expected"),row.at("flags_mask"));check_stack(m.backing,row.at("expected").at("stack"));need(a.fs_base==before.fs_base,"Source FS changed");
        for(const auto &[page,bytes]:m.backing.pages)need(*bytes==*expected_memory.pages.at(page),"Different strict dispatcher final memory page");
        struct MissingOwner {};
        for(unsigned unavailable=0;unavailable<row.at("calls").size();++unavailable) {
            auto memory=initial.clone();auto stopped_abi=before;unsigned entered{};SourcePhysicsWorldRuntimeServices partial;
            partial.required_owner=[&](auto &,const auto &call,auto &abi,const auto &) {
                const auto &expected=row.at("calls").at(entered);need(call.target==expected.at("target"),"Different partial dispatcher owner");
                if(entered++==unavailable)throw MissingOwner{};
                returned(abi,expected.at("returned"));
            };
            bool missing{};try{route.run(memory,entry,stopped_abi,partial);}catch(const MissingOwner &){missing=true;}
            need(missing && entered==unavailable+1,"Missing actual dispatcher owner was replaced");const auto &expected=row.at("calls").at(unavailable);
            check_abi(stopped_abi,expected.at("before"),expected.at("flags_mask"));check_stack(memory,expected.at("before").at("stack"));++missing_stops;
        }
        mmx_cases+=row.at("kind")=="active-mmx";++cases;
    }catch(const std::exception &e){throw std::runtime_error("case "+std::to_string(cases)+": "+e.what());}
    need(route.scope(cases==268 && read_stops==8 && mmx_cases==4 && called_total==140 && missing_stops==140,cases),"Incomplete strict dispatcher scope");
    std::cout<<"PASS "<<cases<<" strict physical dispatcher cases; "<<called_total<<" lower ABIs; "<<missing_stops<<" unavailable-owner boundaries\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

