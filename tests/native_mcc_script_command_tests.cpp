#include "halo2/native_mcc_script_builtin.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_campaign_runtime_tu15.hpp"
#include "halo2/source_ai_frame_tu15.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <set>
#include <nlohmann/json.hpp>

namespace {
using namespace halo2;
void require(bool value,const char *why){if(!value)throw std::runtime_error(why);}
void put(SourceRuntimeMemory &m,unsigned at,unsigned value,unsigned bytes=4) {
    std::array<std::byte,4> out{};for(unsigned n=0;n<bytes;++n)out[n]=std::byte(value>>(8*n));
    m.write(at,std::span(out).first(bytes));
}
unsigned get(const SourceRuntimeMemory&m,unsigned at){return Bytes(m.view(at,4)).u32(0);}
template<class F>void rejected(F &&f){bool bad=false;try{f();}catch(const FormatError&){bad=true;}
    require(bad,"Invalid MCC command receipt admitted");}
struct TrackedMemory:SourcePhysicsContactMemory {
    SourceRuntimeMemory bank;
    mutable std::vector<std::pair<unsigned,unsigned>> reads;
    void read(unsigned at,std::span<std::byte> out)const override {reads.emplace_back(at,out.size());bank.read(at,out);}
    void write(unsigned at,std::span<const std::byte> in)override {bank.write(at,in);}
};
struct Fixture {
    static constexpr unsigned scenario=0x90001000,pool=0x90002000,syntax=0x90002100,
        thread=0x90003000,nodes=0x90004000,strings=0x90005000,
        output=0x90007000,stack=0x60008000,frame=thread+0x40;
    TrackedMemory memory;
    std::string name="rasterizer_blur";
    MccScriptLayout layout;
    std::unique_ptr<NativeMccScriptBuiltinProfile> profile;
    Fixture(unsigned value,bool nested=false) {
        auto &m=memory.bank;
        for(const auto [at,size]:{std::pair(0x90000000u,0x10000u),std::pair(0x60000000u,0x10000u),
            std::pair(0x505e5cu,0x14u),std::pair(0x4ecc20u,4u),std::pair(0x4f3218u,4u),
            std::pair(0x502248u,0x3c10u)})
            m.mount(at,std::vector<std::byte>(size),SourceRuntimeMemory::Access::ReadWrite,"explicit original HS command fixture");
        put(m,0x4ecc20,scenario);put(m,0x505e5c,pool);put(m,0x505e60,1);put(m,0x505e6c,syntax);
        put(m,pool+0x44,thread);put(m,scenario+0x238,nested?5:3);put(m,scenario+0x23c,nodes);
        put(m,scenario+0x1b0,name.size()+1);put(m,scenario+0x1b4,strings);
        put(m,syntax+0x20,nested?5:3);put(m,syntax+0x24,20);put(m,syntax+0x44,nodes);
        const auto node=[&](unsigned index,unsigned op,unsigned type,unsigned flags,unsigned next,unsigned value) {
            const auto at=nodes+index*20;
            put(m,at,0x9000+index,2);put(m,at+2,op,2);put(m,at+4,type,2);put(m,at+6,flags,2);
            put(m,at+8,next);put(m,at+16,value);
        };
        node(0,0x2b3,4,8,0xffffffff,0x90010001);
        node(1,2,2,9,0x90020002,0);
        node(2,nested?0x38f:7,7,nested?8:9,0xffffffff,nested?0x90030003:value);
        if(nested) {
            node(3,2,2,9,0xffffffff,0);put(m,nodes+60+12,name.size()+1);
            // The actual no-argument clock getter is a deferred expression,
            // with original LONG-to-SHORT coercion, never a supplied value.
            const std::string clock_name="game_tick_rate";
            m.write(strings+name.size()+1,std::as_bytes(std::span(clock_name.c_str(),clock_name.size()+1)));
            put(m,scenario+0x1b0,name.size()+clock_name.size()+2);
            m.mount(0x51df54,std::vector<std::byte>(4),SourceRuntimeMemory::Access::ReadWrite,"clock root");
            put(m,0x51df54,0x90006000);put(m,0x90006002,value,2);
            // Unused complete node preserves the explicit bank count.
            node(4,7,7,9,0xffffffff,0);
        }
        m.write(strings,std::as_bytes(std::span(name.c_str(),name.size()+1)));
        const auto names=m.view(strings,get(m,scenario+0x1b0));
        layout=project_mcc_script_layout(m.view(nodes,(nested?5:3)*20),names,{});
        NativeMccScriptBuiltinProfile disabled(layout,names);
        require(!disabled.owns(layout.requirements.front()),"Unqualified command was enabled by default");
        profile=std::make_unique<NativeMccScriptBuiltinProfile>(layout,names,true);
        require(profile->owns(layout.requirements.front()) && profile->calls()==(nested?2u:1u),"Genuine command receipt not owned");
        auto abi=input();SourcePhysicsWorldRuntimeServices services;
        source_run_campaign_runtime_tu15(memory,0x212470,abi,{0x37f,0x1f80},services);
        put(m,thread+2,1,1);put(m,thread+3,1,1);put(m,thread+0x10,frame);
        put(m,frame,thread+0x18);put(m,frame+4,0x90000000);put(m,thread+0x20,output);
        put(m,output,0xdeadbeef);put(m,thread+0xc,0x100,2);
    }
    SourcePhysicsWorldRuntimeAbi input() {
        SourcePhysicsWorldRuntimeAbi abi{};
        abi.gp={0x12345678,0x23456789,0x3456789a,0x456789ab,stack,0x56789abc,0x6789abcd,0xa0010000};
        abi.eflags=0x247;abi.x87[0]=1.25L;abi.x87_depth=1;
        for(unsigned n=0;n<8;++n)for(unsigned j=0;j<4;++j)abi.xmm[n][j]=n*0x11223344u+j;
        put(memory.bank,stack,0xfeedcafe);return abi;
    }
    void run(bool ai,bool restore_after_defer,bool nested) {
        auto abi=input();const auto before=abi;
        SourcePhysicsWorldRuntimeServices services;std::set<unsigned> seen;
        std::optional<SourceRuntimeMemory::Checkpoint> deferred;
        unsigned parses=0,completions=0;
        services.instruction=[&](unsigned pc) {
            seen.insert(pc);if(pc==0x212010)++parses;if(pc==0x211da0)++completions;
            if(pc==0x211c30 && parses==1 && !deferred) {
                require(completions==0 && get(memory.bank,output)==0xdeadbeef,
                    "Deferred command completed before actual argument evaluation");
                deferred=memory.bank.checkpoint();
            }
        };
        services.required_owner=[&](auto &m,const auto &call,auto &state,const auto &observe) {
            if(call.target==0x212010)source_run_ai_frame_tu15(m,call.target,state,{0x27f,0x1f80},services,observe);
            else if(source_campaign_runtime_tu15_owns_entry(call.target))
                source_run_campaign_runtime_tu15(m,call.target,state,{0x27f,0x1f80},services,observe);
            else throw std::runtime_error("MCC command reached an unowned dependency");
        };
        const auto execute=[&]{
            if(ai)source_run_ai_frame_tu15(memory,0x211b10,abi,{0x27f,0x1f80},services);
            else source_run_campaign_runtime_tu15(memory,0x211b10,abi,{0x27f,0x1f80},services);
        };
        memory.reads.clear();execute();
        require(deferred && parses==2 && completions==(nested?2u:1u),"Actual parser/deferred child/completion path differs");
        const auto check=[&] {
            require(get(memory.bank,thread+0x10)==thread+0x18 && get(memory.bank,output)==0 && abi.gp[4]==stack+4,
                "Command completion/frame retirement/RET12 differs");
            for(unsigned n:{3u,5u,6u,7u})require(abi.gp[n]==before.gp[n],"Command changed nonvolatile state");
            require(abi.xmm==before.xmm && abi.x87_depth==before.x87_depth && abi.x87[0]==before.x87[0] &&
                abi.x87_status==before.x87_status,"Command changed FP/SIMD state");
            require(seen.contains(0x212010) && seen.contains(0x211da0) && !seen.contains(0x211c0d),
                "Command bypassed original parsing or read colliding Xbox descriptor");
            require(std::count(memory.reads.begin(),memory.reads.end(),std::pair<unsigned,unsigned>(frame+14,2))==1,
                "MCC blur did not read exactly its original two-byte SHORT argument");
        };
        check();
        if(restore_after_defer) {
            SourceRuntimeMemory replacement;replacement.restore(*deferred);memory.bank.swap_backing(replacement);
            abi=input();memory.reads.clear();execute();check();
        }
    }
};
void installed_calls(const char *path) {
    using Json=nlohmann::json;std::ifstream file(path);Json reference;file>>reference;
    require(reference.at("schema")=="H2_INSTALLED_MCC_BLUR_CALLS_V1","Installed blur reference schema differs");
    unsigned cases=0;
    for(const auto &map:reference.at("maps"))for(const auto &call:map.at("calls"))for(bool ai:{false,true}) {
        Fixture f(call.at("argument").at(6).get<unsigned>());
        auto &m=f.memory.bank;
        constexpr unsigned actual_nodes=0xa0000000,actual_strings=0xa1000000;
        const auto count=map.at("node_count").get<unsigned>(),size=map.at("strings_bytes").get<unsigned>();
        require(count && count<=65536 && size && size<=16u*1024u*1024u,"Installed reference bank extent differs");
        std::vector<std::byte> raw(count*20),names(size);
        for(const auto &row:call.at("rows")) {
            const auto index=row.at("index").get<unsigned>();const auto hex=row.at("raw").get<std::string>();
            require(index<count && hex.size()==40,"Installed reference record extent differs");
            for(unsigned n=0;n<20;++n)raw[index*20+n]=std::byte(std::stoul(hex.substr(n*2,2),nullptr,16));
        }
        const auto offset=call.at("function_text_offset").get<unsigned>();
        require(offset<size && f.name.size()+1<=size-offset,"Installed function-name reference escapes strings");
        std::copy_n(reinterpret_cast<const std::byte*>(f.name.c_str()),f.name.size()+1,names.begin()+offset);
        m.mount(actual_nodes,raw,SourceRuntimeMemory::Access::ReadWrite,"exact sparse installed syntax reference");
        m.mount(actual_strings,names,SourceRuntimeMemory::Access::ReadWrite,"exact installed command-name identity");
        put(m,Fixture::scenario+0x238,count);put(m,Fixture::scenario+0x23c,actual_nodes);
        put(m,Fixture::scenario+0x1b0,size);put(m,Fixture::scenario+0x1b4,actual_strings);
        put(m,Fixture::syntax+0x20,count);put(m,Fixture::syntax+0x44,actual_nodes);
        put(m,Fixture::frame+4,call.at("datum").get<unsigned>());
        f.layout=project_mcc_script_layout(raw,names,{});
        f.profile=std::make_unique<NativeMccScriptBuiltinProfile>(f.layout,names,true);
        require(f.profile->calls()==1 && f.profile->owns(f.layout.requirements.at(0)),"Installed authored command not owned");
        NativeMccScriptBuiltinBinding binding(f.memory,*f.profile);
        f.run(ai,true,false);
        require(std::ranges::equal(m.view(actual_nodes,raw.size()),raw),"HS execution changed installed syntax records");
        require(Bytes(m.view(Fixture::frame+14,2)).u16(0)==(call.at("argument").at(6).get<unsigned>()&65535u),
            "Installed SHORT literal lost its authored low word");
        ++cases;
    }
    require(cases==14,"Installed reference no longer covers all seven declared calls");
    std::cout<<"PASS "<<cases<<" exact installed Alpha Moon/E3 HS compositions and deferred checkpoint retries\n";
}
}
int main(int argc,char **argv)try {
    unsigned cases=0;
    for(bool nested:{false,true})for(bool ai:{false,true})
        for(unsigned value:{0u,1u,0x7fffu,0x8000u,0xffffu,0x1234ffffu,0xffffffffu}) {
            Fixture f(value,nested);NativeMccScriptBuiltinBinding binding(f.memory,*f.profile);
            require(!native_mcc_script_builtin_value(f.memory,Fixture::nodes),"Argumentful command leaked through getter seam");
            require(native_mcc_script_builtin_return_type(f.memory,Fixture::nodes)==4,"MCC command return declaration differs");
            f.run(ai,true,nested);++cases;
            put(f.memory.bank,Fixture::nodes+40,0xdead,2);
            rejected([&]{native_mcc_script_builtin_command(f.memory,Fixture::nodes);});
        }
    std::cout<<"PASS "<<cases<<" MCC command HS compositions and deferred checkpoint retries; real parser, nested getter/cast, exact SHORT reads, VOID completion, identity and FP/ABI\n";
    if(argc==2)installed_calls(argv[1]);else require(argc==1,"Supply at most the installed blur reference");
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
