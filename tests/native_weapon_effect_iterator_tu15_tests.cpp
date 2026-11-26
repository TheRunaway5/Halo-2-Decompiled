#include "halo2/native_weapon_effect_dispatch_tu15.hpp"
#include "native_tu15_sound_test_support.hpp"
#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>
#ifdef HALO2_NATIVE_ITERATOR_COMPILED_CALLERS
#include "halo2/source_effect_particle_frame_tu15.hpp"
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/death_cycle_runtime_tu15.hpp"
#endif
using namespace halo2;
using Json=nlohmann::json;
namespace {
constexpr std::uint32_t stack=0x2010000;
void need(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
std::vector<std::byte> unhex(const std::string &s) {
    std::vector<std::byte> bytes;for(unsigned n=0;n<s.size();n+=2)bytes.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return bytes;
}
std::array<std::uint32_t,4> lanes(const Json &s) {
    const auto bytes=unhex(s);std::array<std::uint32_t,4> result{};need(bytes.size()==16,"Invalid original XMM reference");
    for(unsigned n=0;n<16;++n)result[n/4]|=std::to_integer<std::uint32_t>(bytes[n])<<((n%4)*8);return result;
}
void compare_stack(const SourcePhysicsContactMemory &m,const Json &raw) {
    const auto expected=unhex(raw);std::vector<std::byte> actual(expected.size());m.read(stack-512,actual);
    need(actual==expected,"Different original live/dead recursive stack bytes");
}
#ifndef HALO2_NATIVE_ITERATOR_COMPILED_CALLERS
class LateTraceMemory final : public SourcePhysicsContactMemory {
    sound_test::Memory &backing;
    SourcePhysicsWorldRuntimeServices &services;
    std::vector<std::uint32_t> &trace;
    unsigned writes_until_trace;

public:
    LateTraceMemory(sound_test::Memory &backing, SourcePhysicsWorldRuntimeServices &services,
                    std::vector<std::uint32_t> &trace, unsigned writes_until_trace)
        : backing(backing), services(services), trace(trace), writes_until_trace(writes_until_trace) {}

    void read(std::uint32_t address, std::span<std::byte> bytes) const override {
        backing.read(address, bytes);
    }
    void write(std::uint32_t address, std::span<const std::byte> bytes) override {
        backing.write(address, bytes);
        if (writes_until_trace && --writes_until_trace == 0)
            services.instruction = [this](std::uint32_t pc) { trace.push_back(pc); };
    }
};
#endif
}
int main(int argc,char **argv)try {
#ifdef HALO2_NATIVE_ITERATOR_COMPILED_CALLERS
    need(argc==3,"Supply unchanged-original 17DCE0 reference and compiled caller module");
    const std::string module=argv[2];need(module=="frame" || module=="support" || module=="death","Unknown compiled iterator caller");
#else
    need(argc==2,"Supply unchanged-original 17DCE0 reference");
#endif
    std::ifstream in(argv[1]);const auto reference=Json::parse(in);
    need(reference.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Wrong original image");
    need(native_weapon_effect_dispatch_tu15_owns_entry(0x17dce0),"Native iterator is not registered");
    const auto run=[&](auto &m,auto &a,const SourcePhysicsWorldRuntimeServices &services,const SourcePhysicsContactRecordObserver &observe) {
#ifdef HALO2_NATIVE_ITERATOR_COMPILED_CALLERS
        const auto fp=a.floating_point.value_or(SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80});
        if(module=="frame")source_run_effect_particle_frame_tu15(m,0x17dce0,a,fp,services,observe);
        else if(module=="support")source_run_physics_live_support_tu15(m,0x17dce0,a,fp,services,observe);
        else source_run_death_cycle_runtime_tu15(m,0x17dce0,a,fp,services,observe);
#else
        native_weapon_effect_dispatch_tu15_run(m,0x17dce0,a,services,observe);
#endif
    };
    unsigned cases{},recursive{},stops{},aliases{};
#ifndef HALO2_NATIVE_ITERATOR_COMPILED_CALLERS
    bool late_trace_checked{};
#endif
    for(const auto &row:reference.at("rows"))try {
        sound_test::Memory m;m.map(stack-4096,8192);m.write(stack-512,unhex(row.at("initial_stack")));
        for(const auto &v:row.at("memory")) {
            const auto at=v[0].get<std::uint32_t>(),value=v[1].get<std::uint32_t>();
            if(v[2]==2)m.put16(at,value);else m.put32(at,value);
        }
        SourcePhysicsWorldRuntimeAbi a{};a.gp=row.at("gp").get<decltype(a.gp)>();a.eflags=row.at("eflags");a.fs_base=0x76543210;
        for(unsigned n=0;n<8;++n)a.xmm[n]=lanes(row.at("xmm")[n]);
        a.floating_point=SourcePhysicsFactoryFloatingPoint{0xa7f,0xdfc0};
        if(cases&1u) {
            a.active_mmx.emplace();a.active_mmx->known_mask=0xa5;
            for(unsigned n=0;n<8;++n)for(unsigned j=0;j<10;++j)a.active_mmx->physical[n][j]=std::byte(n*19+j);
        }else {a.x87_depth=2;a.x87[0]=1.25L;a.x87[1]=-3.5L;a.x87_status=0x4311;}
        const auto before=a;auto expected_memory=m.clone();unsigned stored{},called{},traced{};
#ifndef HALO2_NATIVE_ITERATOR_COMPILED_CALLERS
        auto unobserved_memory=m.clone();
#endif
        const auto trace=unhex(row.at("trace_offsets"));
        SourcePhysicsContactRecordObserver observe=[&](const auto &event) {
            need(stored<row.at("stores").size(),"Different original iterator store count");const auto &expected=row.at("stores").at(stored++);
            std::uint32_t value{};for(unsigned n=0;n<event.written.size();++n)value|=std::to_integer<std::uint32_t>(event.written[n])<<(8*n);
            need(event.instruction==expected[0] && event.address==expected[1] && event.written.size()==expected[2] && value==expected[3],
                "Different original iterator store");
            std::vector<std::byte> prior(event.written.size());expected_memory.read(event.address,prior);
            need(prior==event.before,"Different original iterator store preimage");expected_memory.write(event.address,event.written);
        };
        SourcePhysicsWorldRuntimeServices services;
        services.required_owner=[](auto &,const auto &,auto &,const auto &){throw std::runtime_error("Iterator acquired an external service");};
        services.instruction=[&](auto pc) {
            need(traced<trace.size() && pc==0x17dce0u+std::to_integer<unsigned>(trace[traced++]),"Different original iterator instruction trace");
            if(pc!=0x17dd3c)return;
            need(called<row.at("recursive_calls").size(),"Different original recursive call count");
            const auto &expected=row.at("recursive_calls").at(called++);
#ifndef HALO2_NATIVE_ITERATOR_COMPILED_CALLERS
            // Compiled module APIs publish their retained ABI on return.
            // The direct owner exposes its actual recursive GP boundary here;
            // both routes expose the same live/dead stack and final full ABI.
            need(a.gp==expected.at("gp").template get<decltype(a.gp)>(),"Different original recursive GP boundary");
#endif
            compare_stack(m,expected.at("stack"));++recursive;
        };
        bool stopped{};std::string error;
        try{run(m,a,services,observe);}
        catch(const std::out_of_range &e){stopped=true;error=e.what();}
        need(stopped==!row.at("fault").empty(),"Different original iterator read-stop outcome");
        if(stopped) {
            const auto at=row.at("fault")[0].at("address").get<std::uint32_t>();
            need(error.starts_with("unmapped ") && std::stoul(error.substr(9),nullptr,16)==at,"Different original iterator read-stop address");++stops;
        }
        need(stored==row.at("stores").size() && called==row.at("recursive_calls").size(),"Different final iterator store/call count");
        need(traced==trace.size(),"Different final original iterator instruction trace extent");
        need(a.gp==row.at("expected_gp").get<decltype(a.gp)>(),"Different original final iterator GP");
        const auto mask=row.at("eflags_mask").get<std::uint32_t>();
        need((a.eflags&mask)==(row.at("expected_eflags").get<std::uint32_t>()&mask),"Different defined original iterator flags");
        for(unsigned n=0;n<8;++n)need(a.xmm[n]==lanes(row.at("expected_xmm")[n]),"Different original iterator XMM");
        compare_stack(m,row.at("expected_stack"));
        need(m.pages.size()==expected_memory.pages.size(),"Different original iterator memory extent");
        for(const auto &[page,bytes]:m.pages)need(*bytes==*expected_memory.pages.at(page),"Different original iterator final memory page");
        need(a.fs_base==before.fs_base && a.x87_depth==before.x87_depth && a.x87==before.x87 && a.x87_status==before.x87_status &&
            a.floating_point->x87_control_word==before.floating_point->x87_control_word && a.floating_point->mxcsr==before.floating_point->mxcsr &&
            bool(a.active_mmx)==bool(before.active_mmx),"Unrelated iterator FS/FP/MMX state changed");
        if(a.active_mmx)need(a.active_mmx->known_mask==before.active_mmx->known_mask && a.active_mmx->physical==before.active_mmx->physical,
            "Unrelated iterator physical MMX bytes changed");
#ifndef HALO2_NATIVE_ITERATOR_COMPILED_CALLERS
        auto unobserved=before;
        if(!late_trace_checked && !stopped && !row.at("recursive_calls").empty()) {
            constexpr auto publish_cursor=0x17dd06u;
            const auto &stores=row.at("stores");
            const auto first_publish=std::find_if(stores.begin(),stores.end(),[](const auto &event){
                return event[0]==publish_cursor;
            });
            need(first_publish!=stores.end(),"Missing original cursor store");
            auto backing=unobserved_memory.clone();
            auto late_abi=before;
            SourcePhysicsWorldRuntimeServices late_services;
            std::vector<std::uint32_t> late_trace;
            LateTraceMemory late_memory(backing,late_services,late_trace,unsigned(first_publish-stores.begin()+1));
            run(late_memory,late_abi,late_services,SourcePhysicsContactRecordObserver{});
            const auto cut=std::find(trace.begin(),trace.end(),std::byte(publish_cursor-0x17dce0u));
            need(cut!=trace.end() && late_trace.size()==std::size_t(trace.end()-cut-1),"Different late iterator trace extent");
            need(std::equal(cut+1,trace.end(),late_trace.begin(),[](auto offset,auto pc){
                return pc==0x17dce0u+std::to_integer<unsigned>(offset);
            }),"Different late iterator trace PCs");
            need(late_abi.gp==a.gp && late_abi.eflags==a.eflags,"Different late iterator register state");
            for(const auto &[page,bytes]:m.pages)
                need(*bytes==*backing.pages.at(page),"Different late iterator memory page");
            late_trace_checked=true;
        }
        std::string unobserved_error;
        try { run(unobserved_memory,unobserved,SourcePhysicsWorldRuntimeServices{},SourcePhysicsContactRecordObserver{}); }
        catch(const std::out_of_range &e) { unobserved_error=e.what(); }
        need(unobserved_error==error && unobserved.gp==a.gp && unobserved.eflags==a.eflags,
             "Different unobserved iterator fault or register state");
        need(unobserved_memory.pages.size()==m.pages.size(),"Different unobserved iterator memory extent");
        for(const auto &[page,bytes]:m.pages)
            need(*bytes==*unobserved_memory.pages.at(page),"Different unobserved iterator memory page");
#endif
        aliases+=row.at("recipe").at("kind").get<std::string>().find("alias")!=std::string::npos;++cases;
    }catch(const std::exception &e){throw std::runtime_error("case "+std::to_string(cases)+": "+e.what());}
    need(cases==156 && recursive==62 && stops==8 && aliases==20,"Incomplete original iterator scope");
#ifndef HALO2_NATIVE_ITERATOR_COMPILED_CALLERS
    need(late_trace_checked,"Incomplete late iterator trace coverage");
#endif
    unsigned exhaustive{};sound_test::Memory m;m.map(stack-4096,8192);
    constexpr std::uint32_t pool=0x1000000,rows=0x1004000,cursor=0x1002000;
    m.put32(0x4f7208,pool);m.put32(pool+0x44,rows);m.put32(rows+4,0xffffffffu);
    for(const auto mode:{0u,1u,3u}) {
        const auto expected=unhex(reference.at("node_modes").at(std::to_string(mode)));need(expected.size()==65536,"Incomplete original node domain");
        for(unsigned node=0;node<65536;++node) {
            m.put16(rows+2,node);m.put32(cursor,0x80010000);m.put32(stack,0x3000000);m.put32(stack+4,0x12345678);
            m.put32(stack+8,cursor);m.put32(stack+12,mode);SourcePhysicsWorldRuntimeAbi a{};a.gp[4]=stack;a.eflags=0x202;
            run(m,a,SourcePhysicsWorldRuntimeServices{},SourcePhysicsContactRecordObserver{});
            need(a.gp[0]==(expected[node]==std::byte{1}?rows:0) && m.get32(cursor)==0xffffffffu && a.gp[4]==stack+16,
                "Different exhaustive original WORLD/FP/loop node admission");++exhaustive;
        }
    }
    need(exhaustive==196608,"Incomplete original node/mode domain");
    std::cout<<"PASS "<<cases<<" unchanged 17DCE0 cases, "<<recursive<<" real recursive calls, "<<stops<<" read stops, "<<aliases<<" cursor aliases\n";
    std::cout<<"PASS "<<exhaustive<<" unchanged original node/mode admissions\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
