#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "platform/native_tu15_millisecond_clock.hpp"
#include "halo2/source_execution_inputs.hpp"
#include "halo2/native_tu15_rng_startup.hpp"
#include <algorithm>
#include <iostream>
#include <vector>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
namespace copies {
#include "millisecond_clock_bodies.inc"
}
namespace {
constexpr std::array bases{0x41b000u,0x61000000u,0x62000000u,0x80000000u};
constexpr std::array sizes{4096u,8192u,8192u,4096u};
struct Memory:SourcePhysicsContactMemory {
    std::array<std::vector<std::byte>,4>banks{std::vector<std::byte>(sizes[0]),std::vector<std::byte>(sizes[1]),std::vector<std::byte>(sizes[2]),std::vector<std::byte>(sizes[3])};
    mutable std::vector<std::pair<std::uint32_t,std::size_t>>reads;
    std::function<void(Memory&,std::uint32_t,std::size_t)>after_read;
    std::function<void(Memory&,const SourcePhysicsContactRecordStore&)>after_store;
    std::span<std::byte>view(std::uint32_t at,std::size_t count) {
        for(unsigned n=0;n<4;++n)if(at>=bases[n]&&std::uint64_t(at-bases[n])+count<=sizes[n])return std::span(banks[n]).subspan(at-bases[n],count);
        throw std::runtime_error("Millisecond clock escaped declared source bank");
    }
    void read(std::uint32_t at,std::span<std::byte>out)const override{
        auto&self=*const_cast<Memory*>(this);reads.emplace_back(at,out.size());std::ranges::copy(self.view(at,out.size()),out.begin());
        if(after_read)self.after_read(self,at,out.size());
    }
    void write(std::uint32_t at,std::span<const std::byte>in)override{std::ranges::copy(in,view(at,in.size()).begin());}
    void put(std::uint32_t at,std::uint32_t value){auto bytes=view(at,4);for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(8*n));}
    std::uint32_t raw(std::uint32_t at){auto bytes=view(at,4);std::uint32_t value{};for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8*n);return value;}
};
std::uint32_t read_word(){unsigned char b[4];std::cin.read(reinterpret_cast<char*>(b),4);if(!std::cin)throw std::runtime_error("Truncated millisecond clock input");return b[0]|std::uint32_t(b[1])<<8|std::uint32_t(b[2])<<16|std::uint32_t(b[3])<<24;}
void emit(std::uint32_t value){for(unsigned n=0;n<4;++n)std::cout.put(char(value>>(8*n)));}
void supplemental() {
    constexpr std::uint32_t pointer=0x41b774,cell=0x61000100,other=0x61000200,stack=0x62001000;
    for(unsigned mode=0;mode<6;++mode) {
        Memory seed;seed.put(pointer,cell);seed.put(cell,0xaabbccdd);seed.put(other,0x87654321);seed.put(stack,0x63000000);
        seed.after_read=[mode,done=false](auto&m,std::uint32_t at,std::size_t)mutable {
            if(done)return;
            if(mode==0&&at==pointer){m.put(pointer,other);done=true;}
            if(mode==1&&at==cell){m.put(cell,0);done=true;}
            if(mode==2&&at==pointer){m.put(cell,0xffffffff);done=true;}
        };
        const auto run=[&](Memory&m,bool native){
            SourcePhysicsWorldRuntimeServices services;std::uint32_t pc{};WorldState*current{};
            services.instruction=[&](auto at){pc=at;
                if(at==0x33b595){if(mode==3)m.put(pointer,other);if(mode==4)current->gp[0]=other;if(mode==5)throw std::runtime_error("Declared observer boundary");}
            };
            const SourcePhysicsContactRecordObserver observe;WorldState state{m,observe,services,SourcePhysicsContactImage::TitleUpdate15};current=&state;
            SourcePhysicsWorldRuntimeAbi abi{};abi.gp={1,2,3,4,stack,6,7,8};abi.eflags=0xad7;state.load(abi);bool stopped{};
            try{if(native){if(!native_millisecond_clock_detail::bridge(state))throw std::logic_error("Native clock refused");}else copies::reference::run_33b590(state);}catch(const std::runtime_error&){stopped=true;}
            return std::pair{state.save(),stopped};
        };
        Memory a=seed,b=seed;auto x=run(a,false),y=run(b,true);
        if(x.first.gp!=y.first.gp||x.first.eflags!=y.first.eflags||x.second!=y.second||a.banks!=b.banks||a.reads!=b.reads)
            throw std::runtime_error("Canonical clock alias/publication mismatch mode="+std::to_string(mode));
    }
    Memory memory;SourcePhysicsWorldRuntimeServices services;const SourcePhysicsContactRecordObserver observe;
    WorldState legacy{memory,observe,services,SourcePhysicsContactImage::MapPack11};
    if(native_millisecond_clock_detail::bridge(legacy)||!memory.reads.empty())throw std::runtime_error("Legacy clock admission changed source state");
}
bool same_abi(const SourcePhysicsWorldRuntimeAbi&a,const SourcePhysicsWorldRuntimeAbi&b) {
    if(a.gp!=b.gp||a.eflags!=b.eflags||a.xmm!=b.xmm||a.x87_status!=b.x87_status||a.x87_depth!=b.x87_depth||
        a.floating_point->mxcsr!=b.floating_point->mxcsr||a.floating_point->x87_control_word!=b.floating_point->x87_control_word)return false;
    for(unsigned n=0;n<a.x87_depth;++n)if(std::memcmp(&a.x87[n],&b.x87[n],10))return false;return true;
}
struct LiveResult {SourcePhysicsWorldRuntimeAbi before_refresh,after;std::array<std::vector<std::byte>,4>banks;std::vector<std::pair<std::uint32_t,std::size_t>>reads;unsigned callbacks{},refreshes{},backend_reads{};bool rejected{};};
LiveResult live_run(unsigned mode,int copy,SourceExecutionInputs&inputs) {
    constexpr std::uint32_t pointer=0x41b774,stack=0x62001000;
    const auto cell=mode==3?pointer:0x61000100u;
    Memory memory;memory.put(pointer,cell);if(cell!=pointer)memory.put(cell,0);memory.put(stack,0x63000000);
    SourcePhysicsWorldRuntimeServices services;WorldState*current{};std::uint32_t pc{};bool trace=mode==1;LiveResult result;
    auto backend=native_tu15_rng_clock();
    services.instruction=[&](auto site){pc=site;++result.callbacks;if(site==0x33b595) {
        ++result.refreshes;result.before_refresh=current->save();
        // Use the existing production backend and production input journal.
        // Replay supplies captured external inputs, never a function outcome.
        inputs.require_executor();const auto value=inputs.sample(SourceExecutionInputKind::Milliseconds,0,[&]{++result.backend_reads;return backend.monotonic_milliseconds();});
        memory.put(cell,std::uint32_t(value));
    }};
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
    if(mode!=2)services.instruction_cursor={&pc,&trace,nullptr,sites,std::this_thread::get_id(),&services.instruction.target_type(),true,true};
    const SourcePhysicsContactRecordObserver observe;WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};current=&state;
    SourcePhysicsWorldRuntimeAbi incoming{};incoming.gp={0xabcdef12,2,3,4,stack,6,7,8};incoming.eflags=0xad7;
    for(unsigned n=0;n<8;++n)incoming.xmm[n]={0x12345678+n,0xffffffff-n,0x80000000+n,0x7f800001+n};
    incoming.floating_point=SourcePhysicsFactoryFloatingPoint{0x37f,0x9fc0};incoming.x87_status=0x3d21;incoming.x87_depth=1;incoming.x87[0]=1.25L;state.load(incoming);
    try {
        if(copy<0)copies::reference::run_33b590(state);else if(copy==0)copies::primary::run_33b590(state);
        else if(copy==1)copies::input::run_33b590(state);else copies::combat::run_33b590(state);
    }catch(const std::logic_error&error){
        if(std::string(error.what())!="Native execution inputs belong to another executor")throw;result.rejected=true;
    }
    result.after=state.save();result.banks=memory.banks;result.reads=memory.reads;
    if(result.refreshes!=1||result.before_refresh.gp[0]!=cell)throw std::runtime_error("Genuine clock refresh lost pointer-before-service ABI");
    if(result.rejected) {
        if(pc!=0x33b595u||!same_abi(result.after,result.before_refresh))throw std::runtime_error("Clock executor rejection lost partial ABI");
    }else {
        if(pc!=0x33b597u||result.after.gp[0]!=memory.raw(cell)||result.after.gp[4]!=stack+4)throw std::runtime_error("Clock result was not read from its real refreshed bank");
    }
    return result;
}
int live() {
    unsigned comparisons{},backend_reads{},replayed_inputs{},rejections{};
    for(unsigned mode=0;mode<4;++mode) {
        SourceExecutionInputs recording;const auto reference=live_run(mode,-1,recording);const auto recorded=recording.finish();
        const std::vector<SourceExecutionInput>inputs(recorded.begin(),recorded.end());
        if(inputs.size()!=1||reference.backend_reads!=1)throw std::runtime_error("Real clock backend was not sampled exactly once");backend_reads+=reference.backend_reads;
        for(int copy=0;copy<3;++copy) {
            SourceExecutionInputs replay{std::span<const SourceExecutionInput>(inputs)};const auto actual=live_run(mode,copy,replay);(void)replay.finish();
            if(!same_abi(reference.before_refresh,actual.before_refresh)||!same_abi(reference.after,actual.after)||reference.banks!=actual.banks||reference.reads!=actual.reads||actual.backend_reads)
                throw std::runtime_error("Actual backend/input-replay clock ABI or banks differ");
            const auto callbacks=(mode==1||mode==2)?3u:1u;
            if(actual.callbacks!=callbacks)throw std::runtime_error("Clock native service observation delivery differs");++comparisons;++replayed_inputs;
        }
        SourceExecutionInputs wrong_executor;LiveResult left,right;
        std::thread first([&]{left=live_run(mode,-1,wrong_executor);});first.join();
        std::thread second([&]{right=live_run(mode,0,wrong_executor);});second.join();(void)wrong_executor.finish();
        if(!left.rejected||!right.rejected||!same_abi(left.after,right.after)||left.banks!=right.banks||left.reads!=right.reads||left.backend_reads||right.backend_reads)
            throw std::runtime_error("Actual input-owner executor rejection differs");rejections+=2;
    }
    std::cout<<"{\"passed\":true,\"live_comparisons\":"<<comparisons<<",\"actual_backend_reads\":"<<backend_reads
        <<",\"replayed_external_inputs\":"<<replayed_inputs<<",\"actual_executor_rejections\":"<<rejections<<"}\n";return 0;
}
}
int main(int argc,char**argv)try {
    if(argc==2&&std::string(argv[1])=="--live")return live();
    if(std::getenv("HALO2_MILLISECOND_CLOCK_REFUSAL_TEST")) {
        Memory memory;SourcePhysicsWorldRuntimeServices services;
        WorldState state{memory,{},services,SourcePhysicsContactImage::TitleUpdate15};
        if(native_millisecond_clock_detail::bridge(state))throw std::runtime_error("Reference admission did not refuse native search");
        return 0;
    }
    supplemental();
    while(std::cin.peek()!=std::char_traits<char>::eof()) {
        const auto copy=read_word();SourcePhysicsWorldRuntimeAbi abi{};
        for(auto&v:abi.gp)v=read_word();abi.eflags=read_word();const auto mxcsr=read_word(),cw=read_word();
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(cw),mxcsr};abi.x87_status=std::uint16_t(read_word());abi.x87_depth=read_word();
        if(abi.x87_depth>4)throw std::runtime_error("Invalid live resource-record-search FP depth");
        for(unsigned n=0;n<abi.x87_depth;++n)std::cin.read(reinterpret_cast<char*>(&abi.x87[n]),10);
        for(auto&lane:abi.xmm)for(auto&v:lane)v=read_word();Memory memory;
        for(auto&bank:memory.banks)std::cin.read(reinterpret_cast<char*>(bank.data()),bank.size());
        std::vector<SourcePhysicsContactRecordStore>stores;SourcePhysicsContactRecordObserver observe=[&](const auto&e){stores.push_back(e);};
        unsigned instructions{};SourcePhysicsWorldRuntimeServices services;std::uint32_t pc{};
        services.instruction=[&](auto){++instructions;};
        constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};auto&cursor=services.instruction_cursor;
        cursor.position=&pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();
        cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
        WorldState state{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
        if(copy==0)copies::primary::run_33b590(state);
        else if(copy==1)copies::input::run_33b590(state);
        else if(copy==2)copies::combat::run_33b590(state);
        else throw std::runtime_error("Invalid clock production copy");
        if(pc!=0x33b597u)throw std::runtime_error("Native search final cursor differs");
        if(instructions!=1)throw std::runtime_error("Native clock must preserve the genuine595 observation callback");
        const auto result=state.save();if(result.x87_depth!=abi.x87_depth)throw std::runtime_error("Millisecond clock changed live FP depth");
        for(unsigned n=0;n<abi.x87_depth;++n)if(std::memcmp(&result.x87[n],&abi.x87[n],10))throw std::runtime_error("Millisecond clock changed live80bit caller value");
        for(auto v:result.gp)emit(v);emit(result.eflags);emit(result.floating_point->mxcsr);emit(result.x87_status);emit(result.floating_point->x87_control_word);emit(pc);
        for(const auto&lane:result.xmm)for(auto v:lane)emit(v);for(const auto&bank:memory.banks)std::cout.write(reinterpret_cast<const char*>(bank.data()),bank.size());
        emit(stores.size());for(const auto&e:stores){emit(e.instruction);emit(e.address);emit(e.written.size());
            for(unsigned n=0;n<4;++n)std::cout.put(n<e.before.size()?char(e.before[n]):'\0');
            for(unsigned n=0;n<4;++n)std::cout.put(n<e.written.size()?char(e.written[n]):'\0');}
    }
    return std::cout?0:1;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
