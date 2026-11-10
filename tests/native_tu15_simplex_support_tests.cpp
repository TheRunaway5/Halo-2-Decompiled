#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_simplex_support_abi.hpp"
#include "physics/runtime/native_tu15_polygon_support_frame.hpp"
#include <algorithm>
#include <bit>
#include <iostream>
#include <random>
#include <set>
#include <vector>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
namespace reference {
using E=NativeTu15SimplexSupportEntry;using N=NativeTu15SimplexSupportNext;
void dispatch(WorldState&,std::uint32_t,std::uint32_t,bool);
#include "simplex_support_reference.inc"
}
namespace {
using E=NativeTu15SimplexSupportEntry;using N=NativeTu15SimplexSupportNext;
constexpr std::uint32_t base=0x61000000u,work=base+0x100u,stack=base+0x7000u;
constexpr std::uint32_t robase=0x462000u,vtable=0x4620d0u;
struct Memory:SourcePhysicsContactMemory {
    std::vector<std::byte>bank=std::vector<std::byte>(0x8000u);
    std::array<std::byte,0x1000>ro{},bound{},sign{};
    mutable std::vector<std::pair<std::uint32_t,std::size_t>>reads;
    std::function<void(std::uint32_t)>on_read;
    mutable unsigned read_count{};unsigned write_count{},fail_read{},fail_write{};
    std::span<std::byte>view(std::uint32_t at,std::size_t size){
        if(at>=0x44a000u&&std::uint64_t(at-0x44a000u)+size<=bound.size())return std::span(bound).subspan(at-0x44a000u,size);
        if(at>=0x461000u&&std::uint64_t(at-0x461000u)+size<=sign.size())return std::span(sign).subspan(at-0x461000u,size);
        if(at>=robase&&std::uint64_t(at-robase)+size<=ro.size())return std::span(ro).subspan(at-robase,size);
        if(at<base||std::uint64_t(at-base)+size>bank.size())throw std::runtime_error("Support DATA escaped bank");
        return std::span(bank).subspan(at-base,size);
    }
    void read(std::uint32_t at,std::span<std::byte>out)const override{
        reads.emplace_back(at,out.size());if(++read_count==fail_read)throw std::runtime_error("Declared initialization read stop");
        if(on_read)on_read(at);
        std::ranges::copy(const_cast<Memory*>(this)->view(at,out.size()),out.begin());
    }
    void write(std::uint32_t at,std::span<const std::byte>in)override{
        if(++write_count==fail_write)throw std::runtime_error("Declared initialization write stop");
        std::ranges::copy(in,view(at,in.size()).begin());
    }
    void put(std::uint32_t at,std::uint32_t value){std::memcpy(view(at,4).data(),&value,4);}
};
struct Result {
    SourcePhysicsWorldRuntimeAbi abi;std::uint32_t pc{};bool stopped{};
    std::vector<SourcePhysicsContactRecordStore>stores;
    std::vector<std::pair<std::uint32_t,SourcePhysicsWorldRuntimeAbi>>packets;
    std::set<std::uint32_t>sites;NativeTu15SimplexSupportNext next{};std::uint64_t parent_callbacks{},child_callbacks{},child_entries{};
};
thread_local std::function<void(WorldState&,std::uint32_t,std::uint32_t)>dispatch_child;
Result run(Memory&m,const SourcePhysicsWorldRuntimeAbi&abi,E entry,bool native,unsigned feedback=0,unsigned fail_observer=0,bool ordered_reference=true){
    Result out;
    SourcePhysicsContactRecordObserver observe=[&](const auto&e){
        out.stores.push_back(e);
        if(feedback==1u&&e.instruction==0x30eeacu)m.put(abi.gp[5]+8u,base+0x2000u);
        if(feedback==2u&&e.instruction==0x30f30du)m.put(abi.gp[6]+0x10u,1u);
        if(feedback==3u&&e.instruction==0x30f44du)m.put(abi.gp[7],7u);
        if(out.stores.size()==fail_observer)throw std::runtime_error("Declared support observer stop");
    };
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](auto pc){out.pc=pc;out.sites.insert(pc);++out.parent_callbacks;};
    constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};auto&cursor=services.instruction_cursor;
    cursor.position=&out.pc;cursor.service_sites=service_sites;cursor.callback_type=&services.instruction.target_type();
    cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    physics_step_detail::FpScope environment(*abi.floating_point);WorldState s{m,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(abi);
    dispatch_child=[&](WorldState&state,std::uint32_t owner,std::uint32_t){
        if(owner!=0x1f6610u)throw std::runtime_error("Unexpected genuine support owner");
        ++out.child_entries;out.packets.emplace_back(owner,state.save());
        native_polygon_support_detail::Noop before;
        native_polygon_support_detail::support_retained<true>(state,before);
    };
    m.on_read=[&](std::uint32_t at){if(feedback==4u&&out.pc==0x30f2b5u&&at==base+0x5010u)m.put(at,0x3f800000u);};
    try{
        if(native)out.next=native_simplex_support_detail::run_private_stage(s,entry,dispatch_child,[&](auto&state,std::uint32_t pc){out.packets.emplace_back(pc,state.save());});
        else out.next=ordered_reference?reference::run_stage(s,entry):reference::run_stage_unmodified(s,entry);
    }catch(const std::runtime_error&){out.stopped=true;}
    dispatch_child={};m.on_read={};out.abi=s.save();return out;
}
bool same_abi(const auto&a,const auto&b){
    if(a.gp!=b.gp||a.eflags!=b.eflags||a.xmm!=b.xmm||a.x87_depth!=b.x87_depth||a.x87_status!=b.x87_status||
        a.floating_point->mxcsr!=b.floating_point->mxcsr||a.floating_point->x87_control_word!=b.floating_point->x87_control_word)return false;
    for(unsigned n=0;n<a.x87_depth;++n)if(std::memcmp(&a.x87[n],&b.x87[n],10))return false;return true;
}
void compare(const Memory&a,const Memory&b,const Result&x,const Result&y,const std::string&name){
    if(!same_abi(x.abi,y.abi)||x.pc!=y.pc||x.stopped!=y.stopped||x.child_entries!=y.child_entries||x.next!=y.next||a.bank!=b.bank||a.ro!=b.ro||a.bound!=b.bound||a.sign!=b.sign||a.reads!=b.reads||x.stores!=y.stores){
        std::cerr<<name<<" pc "<<std::hex<<x.pc<<"/"<<y.pc<<" flags "<<x.abi.eflags<<"/"<<y.abi.eflags<<std::dec<<" reads "<<a.reads.size()<<"/"<<b.reads.size()<<" stores "<<x.stores.size()<<"/"<<y.stores.size()<<"\n";
        for(unsigned n=0;n<8;++n){if(x.abi.gp[n]!=y.abi.gp[n])std::cerr<<"gp "<<n<<" differs\n";if(x.abi.xmm[n]!=y.abi.xmm[n])std::cerr<<"xmm "<<n<<" differs\n";}
        throw std::runtime_error("Native simplex support mismatch "+name);
    }
    if(y.parent_callbacks)throw std::runtime_error("Native initialization used parent literal callbacks");
}
void supplemental(){
    std::mt19937 rng(0x30f274u);unsigned cases{},prefixes{};
    for(unsigned k=0;k<160u;++k){
        const auto entry=E(k%4u);const auto profile=k/4u;
        const auto current_work=k%32u==8u||k%32u==14u?stack-0x20u-((k%3u)+2u)*16u:
            k%32u==9u?stack+0x70u-(((k/3u)%3u)+14u)*16u:work;Memory seed;for(auto&v:seed.bank)v=std::byte(rng());
        SourcePhysicsWorldRuntimeAbi abi{};for(auto&v:abi.gp)v=rng();
        abi.gp[3]=current_work+0x120u;abi.gp[4]=stack;abi.gp[5]=base+0x6000u;abi.gp[6]=current_work;abi.gp[7]=current_work+4u;abi.gp[2]=current_work+0x100u;
        const auto first=base+0x1000u,second=base+0x2000u,cache=base+0x4000u,matrix=base+0x5000u;
        seed.put(first,vtable);seed.put(second,vtable);seed.put(first+0x14u,4u);seed.put(second+0x14u,4u);
        seed.put(vtable+0x30u,0x1f6610u);seed.put(0x44aac8u,0xff7fffffu);
        for(unsigned lane=0;lane<4u;++lane)seed.put(0x461960u+lane*4u,0x80000000u);
        seed.put(abi.gp[5]+8u,first);seed.put(abi.gp[5]+12u,second);seed.put(abi.gp[5]+16u,matrix);seed.put(abi.gp[5]+20u,cache);
        seed.put(cache+8u,0x04040000u);seed.put(current_work,k%3u);seed.put(current_work+4u,(k/3u)%3u);seed.put(current_work+0x10u,0u);seed.put(stack+0x4cu,0u);
        if(k%20u==0u)seed.put(current_work,4u);if(k%20u==1u)seed.put(current_work+4u,4u);
        if(k%20u==2u)seed.put(current_work+0x10u,1u);if(k%20u==5u)seed.put(current_work+0x10u,0xffffffffu);
        if(k%20u==4u)seed.put(current_work+0x10u,1u);
        if(k%20u==10u)seed.put(current_work,4u);
        if(k%20u==6u)seed.put(stack+0x4cu,1u);
        for(auto object:{first,second,matrix,current_work})for(unsigned row=0;row<20u;++row)for(unsigned lane=0;lane<4u;++lane){
            if(object==current_work&&row<2u)continue;
            seed.put(object+row*16u+lane*4u,std::bit_cast<std::uint32_t>(float(1+int(rng()%31))/8.0f));
        }
        seed.put(first,vtable);seed.put(second,vtable);seed.put(first+0x14u,4u);seed.put(second+0x14u,4u);
        abi.gp[1]=(k/4u)%4u;
        abi.eflags=0x202u|(rng()&0xcd5u);abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(0x37fu|((profile%4u)<<10u)),0x1f80u|((profile%4u)<<13u)|((profile&4u)?0x8040u:0u)};
        abi.x87_depth=std::array{0u,1u,4u,8u}[profile%4u];abi.x87_status=0x4100u|((profile%8u)<<11u);
        for(unsigned n=0;n<abi.x87_depth;++n)abi.x87[n]=n%2u?-1.234567890123456789L:9.876543210987654321L;
        for(auto&v:abi.xmm)for(auto&word:v)word=rng();
        Memory a=seed,b=seed;auto x=run(a,abi,entry,false,k%5u,0,false),y=run(b,abi,entry,true,k%5u);compare(a,b,x,y,"normal-"+std::to_string(k));
        if(x.stopped)throw std::runtime_error("Normal support fixture stopped");++cases;
        if(k<12u){
            for(unsigned category=0;category<2u;++category){const auto limit=category==0?a.read_count:unsigned(x.stores.size());
                for(unsigned n=1;n<=limit;++n){Memory c=seed,d=seed;if(category==0)c.fail_read=d.fail_read=n;
                    auto u=run(c,abi,entry,false,0,category==1?n:0),v=run(d,abi,entry,true,0,category==1?n:0);
                    compare(c,d,u,v,"prefix-"+std::to_string(k)+"-"+std::to_string(category)+"-"+std::to_string(n));
                    if(!u.stopped)throw std::runtime_error("Declared support prefix did not stop");++prefixes;
                }
            }
        }
    }
    std::cerr<<"{\"canonical_cases\":"<<cases<<",\"read_observer_prefixes\":"<<prefixes<<",\"native_literal_callbacks\":0}\n";
}
std::uint32_t input_word(){std::uint32_t value{};std::cin.read(reinterpret_cast<char*>(&value),4);if(!std::cin)throw std::runtime_error("Truncated initialization fixture");return value;}
void emit(std::uint32_t value){std::cout.write(reinterpret_cast<const char*>(&value),4);}
void emit_abi(const SourcePhysicsWorldRuntimeAbi&abi){
    for(auto value:abi.gp)emit(value);emit(abi.eflags);emit(abi.floating_point->mxcsr);emit(abi.x87_status);emit(abi.floating_point->x87_control_word);emit(abi.x87_depth);
    for(unsigned n=0;n<abi.x87_depth;++n)std::cout.write(reinterpret_cast<const char*>(&abi.x87[n]),10);
    for(auto&vector:abi.xmm)for(auto value:vector)emit(value);
}
void fixture(){
    const unsigned count=input_word();for(unsigned k=0;k<count;++k){
        const auto entry=E(input_word());SourcePhysicsWorldRuntimeAbi abi{};for(auto&value:abi.gp)value=input_word();abi.eflags=input_word();
        const auto mxcsr=input_word(),cw=input_word();abi.x87_status=input_word();abi.x87_depth=input_word();abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(cw),mxcsr};
        for(unsigned n=0;n<abi.x87_depth;++n){std::array<std::byte,10>raw{};std::cin.read(reinterpret_cast<char*>(raw.data()),10);std::memcpy(&abi.x87[n],raw.data(),10);}
        for(auto&vector:abi.xmm)for(auto&value:vector)value=input_word();Memory memory;
        for(auto data:{std::span<std::byte>(memory.bank),std::span<std::byte>(memory.bound),std::span<std::byte>(memory.sign),std::span<std::byte>(memory.ro)})std::cin.read(reinterpret_cast<char*>(data.data()),data.size());
        const auto got=run(memory,abi,entry,true);if(got.stopped)throw std::runtime_error("Native physical support stopped");emit_abi(got.abi);
        for(auto data:{std::span<std::byte>(memory.bank),std::span<std::byte>(memory.bound),std::span<std::byte>(memory.sign),std::span<std::byte>(memory.ro)})std::cout.write(reinterpret_cast<const char*>(data.data()),data.size());
        emit(got.stores.size());for(auto&event:got.stores){emit(event.instruction);emit(event.address);emit(event.written.size());std::cout.write(reinterpret_cast<const char*>(event.before.data()),event.before.size());std::cout.write(reinterpret_cast<const char*>(event.written.data()),event.written.size());}
        emit(got.pc);emit(got.child_entries);emit(got.packets.size());for(auto&[pc,state]:got.packets){emit(pc);emit_abi(state);}
    }
}
}
void reference::dispatch(WorldState&s,std::uint32_t owner,std::uint32_t site,bool){dispatch_child(s,owner,site);}
int main(int argc,char**argv){try{if(argc==2&&std::string(argv[1])=="--fixture")fixture();else supplemental();return 0;}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
