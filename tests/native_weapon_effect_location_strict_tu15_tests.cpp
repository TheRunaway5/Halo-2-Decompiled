#include "halo2/native_weapon_effect_dispatch_tu15.hpp"
#include "native_tu15_sound_test_support.hpp"
#include "native_weapon_effect_compiled_route.hpp"
#include <cstring>
#include <fstream>
#include <set>
#include <immintrin.h>
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
    if(a.gp!=expected.at("gp").get<decltype(a.gp)>())std::cerr<<"actual GP="<<Json(a.gp)<<" expected="<<expected.at("gp")<<'\n';
    need(a.gp==expected.at("gp").get<decltype(a.gp)>(),"Different original GP");
    need(!((a.eflags^expected.at("eflags").get<std::uint32_t>())&mask),"Different defined original flags");check_fp(a,expected.at("fp"));
}
void check_stack(const SourcePhysicsContactMemory &m,const Json &raw) {
    const auto expected=unhex(raw);std::vector<std::byte> actual(expected.size());m.read(stack-64,actual);need(actual==expected,"Different original live/dead stack");
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

void seed(sound_test::Memory &memory, const Json &row) {
    memory.map(stack-4096,8192);
    memory.write(stack-64,unhex(row.at("initial_stack")));
    for(const auto &blob:row.at("blobs")){
        const auto at=blob[0].get<std::uint32_t>();const auto raw=unhex(blob[1]);
        memory.map(at,raw.size());memory.write(at,raw);
    }
    for(const auto &v:row.at("memory")){
        const auto at=v[0].get<std::uint32_t>(),value=v[1].get<std::uint32_t>();
        if(v[2]==1)memory.put8(at,value);else if(v[2]==2)memory.put16(at,value);else memory.put32(at,value);
    }
}

struct EffectOriginAccess {
    std::uint32_t pc, address, width;
    std::size_t instruction;
    bool write;
    std::optional<std::uint32_t> written;
};

struct EffectOriginMemory final:SourcePhysicsContactMemory {
    sound_test::Memory backing;
    const std::vector<EffectOriginAccess> &accesses;
    const std::vector<std::uint32_t> &trace;
    const std::size_t reopen;
    const std::uint32_t delta;
    SourcePhysicsWorldRuntimeServices &services;
    mutable std::size_t operation{}, instruction{};
    mutable std::uint32_t pc{};

    EffectOriginMemory(const std::vector<EffectOriginAccess> &accesses,
        const std::vector<std::uint32_t> &trace, std::size_t reopen, std::uint32_t delta,
        SourcePhysicsWorldRuntimeServices &services)
        : accesses(accesses), trace(trace), reopen(reopen), delta(delta), services(services) {}

    const EffectOriginAccess &access(std::uint32_t address, std::size_t width, bool write) const {
        need(operation<accesses.size(),"Unexpected effect-origin memory access");
        const auto &expected=accesses[operation++];
        need(expected.address==address && expected.width==width && expected.write==write,
             "Different effect-origin memory access order or width");
        if(services.instruction)need(pc==expected.pc-delta,"Wrong effect-origin memory callback PC");
        return expected;
    }

    void completed(const EffectOriginAccess &event) const {
        if(operation!=reopen)return;
        instruction=event.instruction+1;
        services.instruction=[this](auto actual){
            need(instruction<trace.size() && actual==trace[instruction]-delta,
                 "Effect-origin trace reopened at a wrong PC");
            pc=actual;
            ++instruction;
        };
    }

    void read(std::uint32_t address,std::span<std::byte> bytes) const override {
        const auto &event=access(address,bytes.size(),false);
        backing.read(address,bytes);
        completed(event);
    }

    void write(std::uint32_t address,std::span<const std::byte> bytes) override {
        const auto &event=access(address,bytes.size(),true);
        need(bytes.size()==4,"Unexpected effect-origin store width");
        std::uint32_t word{};
        for(unsigned i=0;i<4;++i)word|=std::to_integer<std::uint32_t>(bytes[i])<<(8u*i);
        if(event.written)need(word==*event.written,"Different effect-origin store bytes");
        backing.write(address,bytes);
        completed(event);
    }
};

void verify_native_origin(const Json &reference) {
    unsigned cases{}, reopens{};
    for(const auto &row:reference.at("rows"))try{
        constexpr std::uint32_t origin=0xbbe00u, canonical_offset=0x1340u;
        need(row.at("entry")==origin,"Quiet effect-origin test requires its original fixture");
        const auto offsets=unhex(row.at("trace_offsets"));
        const auto &faults=row.at("fault");
        std::vector<std::uint32_t> trace;
        std::vector<EffectOriginAccess> accesses;
        for(std::size_t i=0;i<offsets.size();i+=2){
            const auto pc=origin+std::to_integer<std::uint32_t>(offsets[i])+
                (std::to_integer<std::uint32_t>(offsets[i+1])<<8u);
            const auto at=trace.size();
            trace.push_back(pc);
            for(const auto &read:row.at("reads"))
                if(read[0]==pc)accesses.push_back({pc,read[1],read[2],at,false,{}});
            for(const auto &store:row.at("stores"))
                if(store[0]==pc)accesses.push_back({pc,store[1],store[2],at,true,store[3].get<std::uint32_t>()});
            if(!faults.empty() && faults[0].at("instruction")==pc && faults[0].at("access")=="write")
                accesses.push_back({pc,faults[0].at("address"),faults[0].at("size"),at,true,{}});
        }
        sound_test::Memory original_memory;
        seed(original_memory,row);
        auto expected_memory=original_memory.clone();
        for(const auto &event:accesses)
            if(event.written)expected_memory.put32(event.address,*event.written);
        SourcePhysicsWorldRuntimeAbi initial{};
        initial.gp=row.at("gp").get<decltype(initial.gp)>();
        initial.eflags=row.at("eflags");
        initial.fs_base=0x76543210;
        load_fp(initial,row.at("fp"));

        for(const auto delta:{0u,canonical_offset})
            for(std::size_t reopen=0;reopen<accesses.size();++reopen){
                auto abi=initial;
                SourcePhysicsWorldRuntimeServices services;
                services.required_owner=[](auto &,const auto &,auto &,const auto &){
                    throw std::runtime_error("Unexpected lower effect-origin owner");
                };
                EffectOriginMemory memory(accesses,trace,reopen,delta,services);
                memory.backing=original_memory.clone();
                const auto host_mxcsr=_mm_getcsr();
                bool stopped{};
                try {
                    need(native_weapon_effect_origin_tu15_bridge(memory,abi,services,{},delta!=0),
                         "Quiet effect-origin bridge declined reference inputs");
                }catch(const std::out_of_range &error){
                    need(!faults.empty() && std::string(error.what()).starts_with("unmapped ") &&
                         std::stoul(std::string(error.what()).substr(9),nullptr,16)==faults[0].at("address"),
                         "Wrong effect-origin fault address");
                    stopped=true;
                }
                need(stopped==!faults.empty() && memory.operation==accesses.size(),
                     "Wrong effect-origin execution or committed memory prefix");
                need(reopen ? services.instruction && memory.instruction==trace.size() : !services.instruction,
                     "Wrong effect-origin observed successor extent");
                check_abi(abi,row.at("expected"),row.at("flags_mask"));
                need(abi.fs_base==initial.fs_base && _mm_getcsr()==host_mxcsr,"Effect-origin host or FS state changed");
                for(const auto &[page,bytes]:memory.backing.pages)
                    need(*bytes==*expected_memory.pages.at(page),"Different effect-origin final memory");
                if(reopen)++reopens;
            }
        ++cases;
    }catch(const std::exception &error){
        throw std::runtime_error("Native effect-origin case "+std::to_string(cases)+": "+error.what());
    }
    need(cases==111 && reopens>3000,"Incomplete native effect-origin verification");
    std::cout<<"PASS "<<cases<<" native physical origin cases in both PC domains; "
             <<reopens<<" exact read and write trace resumes\n";
}
}

int main(int argc,char **argv)try {
    const NativeWeaponEffectTestRoute route(argc,argv);
    const bool native_origin=!route.compiled() && argc==3 && std::string(argv[2])=="--quiet-origin";
    need(route.compiled() || argc==2 || native_origin,"Supply strict original dispatcher reference");std::ifstream in(argv[1]);const auto reference=Json::parse(in);
    need(reference.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong original image");
    if(native_origin){verify_native_origin(reference);return 0;}
    unsigned cases{},read_stops{},write_stops{},mmx_cases{};
    const bool direction=reference.at("rows")[0].at("entry")==0x1791c0;
    for(const auto &compact:reference.at("rows"))try {
        if(!route.accepts(compact.at("entry")))continue;
        auto row=compact;
        const auto entry=row.at("entry").get<std::uint32_t>();Memory m;seed(m.backing,row);
        m.write_only_sites=reference.at("write_only_sites").get<decltype(m.write_only_sites)>();
        auto initial=m.backing.clone(),expected_memory=m.backing.clone();
        SourcePhysicsWorldRuntimeAbi a{};a.gp=row.at("gp").get<decltype(a.gp)>();a.eflags=row.at("eflags");a.fs_base=0x76543210;load_fp(a,row.at("fp"));const auto before=a;
        unsigned stored{},traced{};const auto host_mxcsr=_mm_getcsr();const auto trace=unhex(row.at("trace_offsets"));
        SourcePhysicsContactRecordObserver observe=[&](const auto &event) {
            need(stored<row.at("stores").size(),"Different strict location store count");const auto &expected=row.at("stores").at(stored++);
            std::uint32_t value{};for(unsigned n=0;n<event.written.size();++n)value|=std::to_integer<std::uint32_t>(event.written[n])<<(8*n);
            need(event.instruction==expected[0] && event.address==expected[1] && event.written.size()==expected[2] && value==expected[3],"Different strict location store");
            std::vector<std::byte> prior(event.written.size());expected_memory.read(event.address,prior);need(prior==event.before,"Different strict location preimage");expected_memory.write(event.address,event.written);
        };
        SourcePhysicsWorldRuntimeServices services;
        services.instruction=[&](auto pc) {
            m.pc=pc;need(traced+2<=trace.size(),"Different strict location trace extent");
            const auto bits=std::uint16_t(std::to_integer<unsigned>(trace[traced])|(std::to_integer<unsigned>(trace[traced+1])<<8u));traced+=2;
            if(pc!=entry+bits)std::cerr<<"actual PC="<<std::hex<<pc<<" expected="<<entry+bits<<std::dec<<'\n';need(pc==entry+bits,"Different strict location instruction trace");
        };
        services.required_owner=[](auto &,const auto &,auto &,const auto &){throw std::runtime_error("Unexpected lower location owner");};
        m.capture=true;bool stopped{};std::string error;
        try{route.run(m,entry,a,services,observe);}catch(const std::out_of_range &e){stopped=true;error=e.what();}
        m.capture=false;need(stopped==!row.at("fault").empty(),"Different strict location stopped-read outcome");
        if(stopped) {
            const auto &fault=row.at("fault")[0];need(error.starts_with("unmapped ") && std::stoul(error.substr(9),nullptr,16)==fault.at("address").get<std::uint32_t>() && m.pc==fault.at("instruction"),"Different strict location fault address/site");if(fault.at("access")=="read")++read_stops;else ++write_stops;
        }
        need(stored==row.at("stores").size() && traced==trace.size(),"Different final strict location store/trace/call count");
        if(Json(m.reads)!=row.at("reads"))std::cerr<<"actual reads="<<Json(m.reads)<<" expected="<<row.at("reads")<<'\n';
        need(Json(m.reads)==row.at("reads"),"Different strict location operand-read order");
        check_abi(a,row.at("expected"),row.at("flags_mask"));check_stack(m.backing,row.at("expected").at("stack"));need(a.fs_base==before.fs_base,"Source FS changed");
        for(const auto &[page,bytes]:m.backing.pages)need(*bytes==*expected_memory.pages.at(page),"Different strict location final memory page");
        need(_mm_getcsr()==host_mxcsr,"Host SSE environment leaked");
        mmx_cases+=row.at("kind")=="active-mmx";++cases;
    }catch(const std::exception &e){throw std::runtime_error("case "+std::to_string(cases)+": "+e.what());}
    need(route.scope(cases==111 && mmx_cases==4 && (direction?(read_stops==13 && write_stops==6):(read_stops==14 && write_stops==5)),cases),"Incomplete strict location scope");
    std::cout<<"PASS "<<cases<<" strict physical "<<(direction?"direction":"origin")<<" cases; complete GP/flags/FP/PC/read/store/fault state\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

