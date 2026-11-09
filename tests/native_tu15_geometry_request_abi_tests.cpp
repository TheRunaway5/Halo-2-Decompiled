#include "rendering/native_tu15_geometry_request_abi.hpp"
#include "rendering/native_tu15_geometry_publish_abi.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <algorithm>
#include <iostream>
#include <random>
#include <vector>
using namespace halo2;
using State=physics_world_runtime_detail::WorldState;
static void declared_service(State&,std::uint32_t);
#include "geometry_request_reference.inc"
namespace {
constexpr std::array bases{0x475000u,0x4f0000u,0x4f2000u,0x1000000u};
constexpr std::array sizes{4096u,4096u,8192u,65536u};
struct Access {std::uint32_t address,size;bool write;bool operator==(const Access&)const=default;};
struct Memory final:SourcePhysicsContactMemory {
    std::array<std::vector<std::byte>,4> banks;
    mutable std::vector<Access> accesses;
    Memory(){for(unsigned i=0;i<4;++i)banks[i].resize(sizes[i]);}
    std::span<std::byte> view(std::uint32_t p,std::size_t n){
        for(unsigned i=0;i<4;++i)if(p>=bases[i]&&p-bases[i]<=sizes[i]&&n<=sizes[i]-(p-bases[i]))
            return std::span(banks[i]).subspan(p-bases[i],n);
        throw std::runtime_error("Geometry request escaped declared DATA bank at "+std::to_string(p));
    }
    void read(std::uint32_t p,std::span<std::byte> out)const override {
        accesses.push_back({p,std::uint32_t(out.size()),false});
        auto in=const_cast<Memory*>(this)->view(p,out.size());std::ranges::copy(in,out.begin());
    }
    void write(std::uint32_t p,std::span<const std::byte> in)override {
        accesses.push_back({p,std::uint32_t(in.size()),true});std::ranges::copy(in,view(p,in.size()).begin());
    }
    void put(std::uint32_t p,std::uint32_t value,unsigned n=4){auto out=view(p,n);for(unsigned i=0;i<n;++i)out[i]=std::byte(value>>(8*i));}
};
struct Store {std::uint32_t pc,address;std::vector<std::byte> before,after;bool operator==(const Store&)const=default;};
struct Call {std::uint32_t target;SourcePhysicsWorldRuntimeAbi abi;};
struct Boundary{};
struct Context {std::vector<Call> calls;unsigned stop{},pumps{};bool native_publication{};};
thread_local Context *context;
void equal_abi(const auto&a,const auto&b,const char*where){
    for(unsigned i=0;i<8;++i)if(a.gp[i]!=b.gp[i])throw std::runtime_error(std::string(where)+" GP"+std::to_string(i)+" "+std::to_string(a.gp[i])+" != "+std::to_string(b.gp[i]));
    if(a.eflags!=b.eflags)throw std::runtime_error(std::string(where)+" flags "+std::to_string(a.eflags)+" != "+std::to_string(b.eflags));
    if(a.xmm!=b.xmm||a.x87_status!=b.x87_status||a.floating_point->mxcsr!=b.floating_point->mxcsr||
        a.floating_point->x87_control_word!=b.floating_point->x87_control_word)throw std::runtime_error(std::string(where)+" FP");
}
}
static void declared_service(State&s,std::uint32_t target){
    using namespace physics_step_detail;
    auto &c=*context;c.calls.push_back({target,s.save()});
    if(c.stop&&c.calls.size()==c.stop)throw Boundary{};
    // These are independent caller-owned service contracts, used only to
    // compose the parent. Actual production child outcomes remain unqualified.
    const auto store=[&](std::uint32_t p,unsigned n,std::uint32_t v){w(s.memory,s.observe,0x70000000u+target,p,n,v);};
    unsigned cleanup=0;
    if(target==0x130eb0u){store(r(s.memory,s.gp[4]+4,4)+0x20u,4,2);cleanup=12;}
    else if(target==0x1236a0u){cleanup=4;}
    else if(target==0x2dbf74u){}
    else if(target==0x128870u){if(++c.pumps==2)store(s.gp[5]+2,1,1);}
    else if(target==0x22a680u){
        if(c.native_publication){if(!native_geometry_publish_detail::run(s))throw std::runtime_error("Native publication declined");}
        else geometry_reference::publish(s);
        return;
    }
    else throw std::runtime_error("Unexpected geometry request owner");
    for(unsigned i=0;i<3;++i)s.gp[i]=target^(0xabcdef01u+i*0x1234567u+std::uint32_t(c.calls.size()));
    for(unsigned i=0;i<8;++i){const auto v=target+i*17+std::uint32_t(c.calls.size());s.xmm[i]=_mm_castsi128_ps(_mm_setr_epi32(v,v+31,v+62,v+93));}
    s.flags.minus(target,std::uint32_t(c.calls.size()),4);s.pop();s.gp[4]+=cleanup;
}
static int physical_probe(bool publish,bool reference=false,bool accesses_only=false){
    const auto word=[](){std::uint32_t value{};std::cin.read(reinterpret_cast<char*>(&value),4);if(!std::cin)throw std::runtime_error("Truncated physical request");return value;};
    const auto emit=[](std::uint32_t value){std::cout.write(reinterpret_cast<const char*>(&value),4);};
    while(std::cin.peek()!=std::char_traits<char>::eof()){
        SourcePhysicsWorldRuntimeAbi input{};
        for(auto&v:input.gp)v=word();input.eflags=word();const auto mxcsr=word();
        input.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(word()),mxcsr};input.x87_status=std::uint16_t(word());
        for(auto&lane:input.xmm)for(auto&v:lane)v=word();
        Memory memory;for(auto&bank:memory.banks){std::cin.read(reinterpret_cast<char*>(bank.data()),bank.size());if(!std::cin)throw std::runtime_error("Truncated physical bank");}
        std::vector<Store> stores;SourcePhysicsContactRecordObserver observer=[&](const auto&e){stores.push_back({e.instruction,e.address,e.before,e.written});};
        SourcePhysicsWorldRuntimeServices services;State s{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};s.load(input);
        Context c{{},1};context=&c;
        try{bool admitted=true;
            if(reference)geometry_reference::publish(s);
            else admitted=publish?native_geometry_publish_detail::run(s):native_geometry_request_detail::run(s,[](auto&v,auto target,auto){declared_service(v,target);});
            if(!admitted)throw std::runtime_error("Native physical request declined");}catch(Boundary&){}
        if(accesses_only){emit(memory.accesses.size());for(auto&a:memory.accesses){emit(a.address);emit(a.size);emit(a.write);}continue;}
        const auto result=s.save();emit(c.calls.empty()?0:c.calls.back().target);
        for(auto v:result.gp)emit(v);emit(result.eflags);emit(result.floating_point->mxcsr);emit(result.x87_status);emit(result.floating_point->x87_control_word);
        for(auto&lane:result.xmm)for(auto v:lane)emit(v);
        for(auto&bank:memory.banks)std::cout.write(reinterpret_cast<const char*>(bank.data()),bank.size());
        emit(stores.size());for(auto&e:stores){emit(e.pc);emit(e.address);emit(e.after.size());
            for(unsigned i=0;i<4;++i)std::cout.put(i<e.before.size()?char(e.before[i]):'\0');
            for(unsigned i=0;i<4;++i)std::cout.put(i<e.after.size()?char(e.after[i]):'\0');}
    }
    return 0;
}
int main(int argc,char**argv)try{
    if(argc==2&&std::string_view(argv[1])=="--physical")return physical_probe(false);
    if(argc==2&&std::string_view(argv[1])=="--publish-physical")return physical_probe(true);
    if(argc==2&&std::string_view(argv[1])=="--publish-accesses")return physical_probe(true,false,true);
    if(argc==2&&std::string_view(argv[1])=="--publish-reference-accesses")return physical_probe(true,true,true);
    std::mt19937 random(0x130980);unsigned normal{},boundaries{},calls{};std::array<unsigned,5> owners{};
    for(unsigned ordinal=0;ordinal<3072;++ordinal){
        Memory initial;for(auto&bank:initial.banks)for(auto&v:bank)v=std::byte(random());
        constexpr auto header=0x1000100u,physical=0x1004000u,pool=0x1004100u,records=0x1005000u;
        constexpr auto inflight=0x1006000u,rows=0x1006100u;
        const unsigned kind=ordinal%12,flags=(ordinal/12)%16;
        const auto stack=0x100e000u+(ordinal%16),age=records+2*24+20;
        initial.put(0x4f0414,0x1002100);initial.put(header+0x18,(ordinal/7)%3?2u:0xffffu,2);
        const auto group=std::array{0x6c746d70u,0x73627370u,0x6d6f6465u}[(ordinal/11)%3];
        initial.put(0x1002100+2*16,group);initial.put(0x1002100-16,group);
        initial.put(header+8,kind==1?0:1);initial.put(header+12,kind==2?0xffffffffu:0);
        initial.put(header+0x10,0);initial.put(header+0x1c,32,2);
        initial.put(header+0x1e,kind==0?1:0,1);
        initial.put(header+0x20,kind==3||kind==4||kind==5?0xffffffffu:2);
        initial.put(0x4f2d6c,physical);initial.put(physical+0x64,pool);initial.put(pool+0x44,records);
        initial.put(physical+0x38,0x10203040u+(ordinal%17));initial.put(physical+0x34,ordinal%32);
        initial.put(0x4f2d5c,inflight);initial.put(inflight+0x44,rows);initial.put(0x4f2d64,0x1008000);
        initial.put(rows+2*12+2,kind==6||kind==7?1:0,1);initial.put(rows+2*12+3,kind==8?0:255,1);
        initial.put(rows+2*12+4,0x12345678);initial.put(records+2*24+8,3);
        initial.put(0x4751ac,(ordinal/23)%2,1);initial.put(0x4f2d70,(ordinal/3)%4);
        initial.put(0x4f2d74,(ordinal/5)%3);initial.put(0x4f2d78,0xfffffff0);
        initial.put(stack,0x63000000);
        // Live aliases include saved words and policy bytes touched by age.
        if(kind==0&&ordinal%3==0)initial.put(pool+0x44,stack-16-2*24-20);
        if(kind==9)initial.put(pool+0x44,stack-12-2*24-20);
        if(kind==10)initial.put(pool+0x44,stack-4-2*24-20);
        if(kind==11)initial.put(pool+0x44,stack-11-2*24-20);
        SourcePhysicsWorldRuntimeAbi input{};for(auto&v:input.gp)v=random();input.gp[0]=(random()&0xffffff00u)|flags;
        input.gp[1]=header;input.gp[4]=stack;input.eflags=0x202|(random()&0xcd5u);
        input.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
        for(auto&lane:input.xmm)for(auto&v:lane)v=random();
        for(unsigned stop:std::array{0u,1u,2u,3u}){
            Memory left=initial,right=initial;std::vector<Store> ls,rs;Context lc{{},stop,0,true},rc{{},stop};
            const auto observer=[](auto&out){return [&out](const auto&e){out.push_back({e.instruction,e.address,e.before,e.written});};};
            unsigned native_instructions{},reference_instructions{};std::uint32_t cursor{},reference_cursor{};
            SourcePhysicsWorldRuntimeServices nservices,rservices;
            nservices.instruction=[&](auto){++native_instructions;};rservices.instruction=[&](auto pc){++reference_instructions;reference_cursor=pc;};
            constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};
            auto &policy=nservices.instruction_cursor;policy.position=&cursor;policy.observation_only_stores=true;
            policy.literal_service_sites=true;policy.service_sites=sites;policy.executor=std::this_thread::get_id();
            policy.callback_type=&nservices.instruction.target_type();
            SourcePhysicsContactRecordObserver lobserver=observer(ls),robserver=observer(rs);
            State ns{left,lobserver,nservices,SourcePhysicsContactImage::TitleUpdate15};
            State ref{right,robserver,rservices,SourcePhysicsContactImage::TitleUpdate15};
            ns.load(input);ref.load(input);
            bool ne=false,re=false;
            context=&lc;try{if(!native_geometry_request_detail::run(ns,[](auto&s,auto target,auto){declared_service(s,target);}))throw std::runtime_error("Native request declined");}catch(Boundary&){ne=true;}
            context=&rc;try{geometry_reference::run_130980(ref);}catch(Boundary&){re=true;}
            try{
                if(ne!=re||lc.calls.size()!=rc.calls.size())throw std::runtime_error("Service call shape");
                equal_abi(ns.save(),ref.save(),"return");
                if(left.banks!=right.banks||ls!=rs)throw std::runtime_error("DATA/stores");
                if(left.accesses!=right.accesses){unsigned i=0;while(i<std::min(left.accesses.size(),right.accesses.size())&&left.accesses[i]==right.accesses[i])++i;throw std::runtime_error("Access order at "+std::to_string(i));}
                for(unsigned i=0;i<lc.calls.size();++i){if(lc.calls[i].target!=rc.calls[i].target)throw std::runtime_error("Service target");equal_abi(lc.calls[i].abi,rc.calls[i].abi,"callee");}
                if(native_instructions)throw std::runtime_error("Translated native callback");
                if(cursor!=reference_cursor)throw std::runtime_error("Observation cursor");
            }catch(const std::exception&e){throw std::runtime_error("case "+std::to_string(ordinal)+" stop "+std::to_string(stop)+": "+e.what());}
            if(stop==0){++normal;calls+=lc.calls.size();for(auto&call:lc.calls){auto index=call.target==0x130eb0u?0:call.target==0x1236a0u?1:call.target==0x2dbf74u?2:call.target==0x128870u?3:4;++owners[index];}}
            else if(ne)++boundaries;
        }
    }
    for(auto count:owners)if(!count)throw std::runtime_error("Missing owner coverage");
    std::cout<<"{\"normal_cases\":"<<normal<<",\"owner_boundaries\":"<<boundaries<<",\"owner_calls\":"<<calls<<",\"native_instruction_callbacks\":0,\"owner_counts\":[";
    for(unsigned i=0;i<5;++i)std::cout<<(i?",":"")<<owners[i];std::cout<<"]}\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
