#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_simplex_initialize_abi.hpp"
#include "physics/runtime/native_tu15_polygon_features_abi.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <random>
#include <set>
#include <vector>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
namespace reference {
static void child(WorldState&);
inline bool handoff_native_contact_features(WorldState&,std::uint32_t,std::uint32_t){return false;}
inline std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){
    if(p==0x30ae80u)return 0x30e970u;if(p==0x30ae98u)return 0x30e988u;
    throw std::runtime_error("Unknown initialization return mapping");
}
void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool);
#include "simplex_initialize_reference.inc"
}
namespace {
constexpr std::uint32_t base=0x61000000u,work=base+0x100u,stack=base+0x7000u;
constexpr std::uint32_t robase=0x462000u,vtable=0x4620d0u;
struct Memory:SourcePhysicsContactMemory {
    std::vector<std::byte>bank=std::vector<std::byte>(0x8000u);
    std::array<std::byte,0x1000>ro{};
    mutable std::vector<std::pair<std::uint32_t,std::size_t>>reads;
    mutable unsigned read_count{};unsigned write_count{},fail_read{},fail_write{};
    std::span<std::byte>view(std::uint32_t at,std::size_t size){
        if(at>=robase&&std::uint64_t(at-robase)+size<=ro.size())return std::span(ro).subspan(at-robase,size);
        if(at<base||std::uint64_t(at-base)+size>bank.size())throw std::runtime_error("Initialization DATA escaped bank");
        return std::span(bank).subspan(at-base,size);
    }
    void read(std::uint32_t at,std::span<std::byte>out)const override{
        reads.emplace_back(at,out.size());if(++read_count==fail_read)throw std::runtime_error("Declared initialization read stop");
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
    std::set<std::uint32_t>sites;std::uint64_t parent_callbacks{},child_callbacks{},child_entries{};
};
thread_local std::function<void(WorldState&,std::uint32_t,std::uint32_t)>dispatch_child;
Result run(Memory&m,const SourcePhysicsWorldRuntimeAbi&abi,bool native,unsigned feedback=0,unsigned fail_observer=0,unsigned stop_child=0,bool native_features=false,bool ordered_reference=false){
    Result out;
    SourcePhysicsContactRecordObserver observe=[&](const auto&e){
        out.stores.push_back(e);
        if(feedback==1u&&e.instruction==0x30e944u)m.put(e.address,2u);
        if(feedback==2u&&e.instruction==0x30e965u)m.put(abi.gp[4]+12u,base+0x5800u);
        if(feedback==3u&&e.instruction==0x30e9e9u)m.put(base+0x5000u,0x3f800000u);
        if(out.stores.size()==fail_observer)throw std::runtime_error("Declared initialization observer stop");
    };
    SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](auto pc){out.pc=pc;out.sites.insert(pc);if(pc>=0x30e930u&&pc<0x30ea00u)++out.parent_callbacks;else ++out.child_callbacks;};
    constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};auto&cursor=services.instruction_cursor;
    cursor.position=&out.pc;cursor.service_sites=service_sites;cursor.callback_type=&services.instruction.target_type();
    cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    physics_step_detail::FpScope environment(*abi.floating_point);WorldState s{m,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(abi);
    dispatch_child=[&](WorldState&state,std::uint32_t owner,std::uint32_t){
        if(owner!=0x1f6690u)throw std::runtime_error("Unexpected genuine feature owner");
        ++out.child_entries;out.packets.emplace_back(owner,state.save());
        if(out.child_entries==stop_child)throw std::runtime_error("Declared genuine feature entry stop");
        if(native_features&&native)native_polygon_features_detail::run_private_owner(state);
        else if(ordered_reference)reference::child_ordered(state);
        else reference::child(state);
    };
    try{
        if(native)native_simplex_initialize_detail::run_private_stage(s,dispatch_child,[&](auto&state,std::uint32_t pc){out.packets.emplace_back(pc,state.save());});
        else reference::run_stage(s);
    }catch(const std::runtime_error&){out.stopped=true;}
    dispatch_child={};out.abi=s.save();return out;
}
bool same_abi(const auto&a,const auto&b){
    if(a.gp!=b.gp||a.eflags!=b.eflags||a.xmm!=b.xmm||a.x87_depth!=b.x87_depth||a.x87_status!=b.x87_status||
        a.floating_point->mxcsr!=b.floating_point->mxcsr||a.floating_point->x87_control_word!=b.floating_point->x87_control_word)return false;
    for(unsigned n=0;n<a.x87_depth;++n)if(std::memcmp(&a.x87[n],&b.x87[n],10))return false;return true;
}
void compare(const Memory&a,const Memory&b,const Result&x,const Result&y,const std::string&name){
    if(!same_abi(x.abi,y.abi)||x.pc!=y.pc||x.stopped!=y.stopped||x.child_entries!=y.child_entries||a.bank!=b.bank||a.ro!=b.ro||a.reads!=b.reads||x.stores!=y.stores){
        std::cerr<<name<<" pc "<<std::hex<<x.pc<<"/"<<y.pc<<" flags "<<x.abi.eflags<<"/"<<y.abi.eflags<<std::dec<<" reads "<<a.reads.size()<<"/"<<b.reads.size()<<" stores "<<x.stores.size()<<"/"<<y.stores.size()<<"\n";
        for(unsigned n=0;n<8;++n){if(x.abi.gp[n]!=y.abi.gp[n])std::cerr<<"gp "<<n<<" differs\n";if(x.abi.xmm[n]!=y.abi.xmm[n])std::cerr<<"xmm "<<n<<" differs\n";}
        throw std::runtime_error("Native simplex initialization mismatch "+name);
    }
    if(y.parent_callbacks)throw std::runtime_error("Native initialization used parent literal callbacks");
}
void supplemental(bool native_features=false){
    std::mt19937 rng(0x30e930u);unsigned cases{},prefixes{};std::set<std::uint32_t>sites;
    for(unsigned k=0;k<96u;++k){
        Memory seed;for(auto&v:seed.bank)v=std::byte(rng());SourcePhysicsWorldRuntimeAbi abi{};for(auto&v:abi.gp)v=rng();
        abi.gp[1]=work;abi.gp[4]=stack+k%16u;
        const auto first=base+0x1000u,second=base+0x2000u,cache=base+0x4000u,relative=base+0x5000u;
        seed.put(first,vtable);seed.put(second,vtable);seed.put(vtable+0x34u,0x1f6690u);
        seed.put(abi.gp[4],0x63000000u);seed.put(abi.gp[4]+4u,first);seed.put(abi.gp[4]+8u,second);
        seed.put(abi.gp[4]+12u,relative);seed.put(abi.gp[4]+16u,cache);seed.put(abi.gp[4]+20u,base+0x6000u);
        seed.put(cache,0x00010000u);seed.put(cache+4u,0x00030002u);
        seed.put(cache+8u,(k%4u)|(((k/4u)%4u)<<8u));
        for(auto object:{first,second})for(unsigned row=0;row<4u;++row)for(unsigned lane=0;lane<4u;++lane)
            seed.put(object+(row+2u)*16u+lane*4u,std::bit_cast<std::uint32_t>(float(1+int(rng()%31))/8.0f));
        for(auto matrix:{relative,base+0x5800u})for(unsigned row=0;row<4u;++row)for(unsigned lane=0;lane<4u;++lane)
            seed.put(matrix+row*16u+lane*4u,std::bit_cast<std::uint32_t>(float(1+int(rng()%31))/8.0f));
        if(k%16u==7u){
            // The actual first feature MOVAPS overwrites its saved DI/SI.
            // Its true return therefore selects new parent work/count rows.
            const auto frame=((abi.gp[4]-4u)&~15u)-128u;
            abi.gp[1]=frame-0x40u;
            seed.put(cache+8u,0x101u);seed.put(first+0x28u,base+0x900u);seed.put(first+0x2cu,base+0x300u);
            seed.put(base+0x300u,1u);seed.put(base+0x900u,1u);
        }
        abi.eflags=0x202u|(rng()&0xcd5u);abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(0x37fu|((k%4)<<10u)),0x1f80u|((k%4)<<13u)|((k&4u)?0x8040u:0u)};
        abi.x87_depth=std::array{0u,1u,4u,8u}[k%4];abi.x87_status=0x4100u|((k%8)<<11u);
        for(unsigned n=0;n<abi.x87_depth;++n)abi.x87[n]=n%2?-(1.234567890123456789L+n):(9.876543210987654321L+n);
        for(auto&v:abi.xmm)for(auto&word:v)word=rng();
        Memory expected=seed,actual=seed;const auto x=run(expected,abi,false,k%4,0,0,native_features),y=run(actual,abi,true,k%4,0,0,native_features);compare(expected,actual,x,y,"normal-"+std::to_string(k));
        if(native_features&&y.child_callbacks)throw std::runtime_error("Native initialization closure used literal feature callbacks");
        if(x.stopped)throw std::runtime_error("Normal initialization case stopped");sites.insert(x.sites.begin(),x.sites.end());++cases;
        if(k<8u){
            Memory baseline=seed;const auto unchanged=run(baseline,abi,false);
            for(unsigned category=0;category<3u;++category){const auto limit=category==0?baseline.read_count:category==1?baseline.write_count:unsigned(unchanged.stores.size());
                for(unsigned n=1;n<=limit;++n){Memory a=seed,b=seed;if(category==0)a.fail_read=b.fail_read=n;if(category==1)a.fail_write=b.fail_write=n;
                    const auto r=run(a,abi,false,0,category==2?n:0,0,native_features,native_features),s=run(b,abi,true,0,category==2?n:0,0,native_features);compare(a,b,r,s,"prefix-"+std::to_string(k)+"-"+std::to_string(category)+"-"+std::to_string(n));
                    if(!r.stopped)throw std::runtime_error("Initialization prefix did not stop");++prefixes;
                }
            }
            for(unsigned stop=1;stop<=2;++stop){Memory a=seed,b=seed;const auto r=run(a,abi,false,0,0,stop,native_features),s=run(b,abi,true,0,0,stop,native_features);compare(a,b,r,s,"genuine-feature-entry-stop");if(!r.stopped)throw std::runtime_error("Feature entry did not stop");++prefixes;}
        }
    }
    unsigned parent_sites{};for(auto pc:sites)if(pc>=0x30e930u&&pc<0x30ea00u)++parent_sites;
    if(parent_sites!=67u)throw std::runtime_error("Initialization source coverage "+std::to_string(parent_sites));
    std::cerr<<"{\"canonical_cases\":"<<cases<<",\"retained_prefixes\":"<<prefixes<<",\"parent_source_sites\":"<<parent_sites<<",\"native_parent_literal_callbacks\":0,\"literal_feature_child_retained\":"<<(native_features?"false":"true")<<"}\n";
}
std::uint32_t input_word(){std::uint32_t value{};std::cin.read(reinterpret_cast<char*>(&value),4);if(!std::cin)throw std::runtime_error("Truncated initialization fixture");return value;}
void emit(std::uint32_t value){std::cout.write(reinterpret_cast<const char*>(&value),4);}
void emit_abi(const SourcePhysicsWorldRuntimeAbi&abi){
    for(auto value:abi.gp)emit(value);emit(abi.eflags);emit(abi.floating_point->mxcsr);emit(abi.x87_status);emit(abi.floating_point->x87_control_word);emit(abi.x87_depth);
    for(unsigned n=0;n<abi.x87_depth;++n)std::cout.write(reinterpret_cast<const char*>(&abi.x87[n]),10);
    for(auto&vector:abi.xmm)for(auto value:vector)emit(value);
}
void fixture(bool native_features=false){
    const unsigned count=input_word();for(unsigned k=0;k<count;++k){
        SourcePhysicsWorldRuntimeAbi abi{};for(auto&value:abi.gp)value=input_word();abi.eflags=input_word();
        const auto mxcsr=input_word(),cw=input_word();abi.x87_status=input_word();abi.x87_depth=input_word();abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(cw),mxcsr};
        for(unsigned n=0;n<abi.x87_depth;++n){std::array<std::byte,10>raw{};std::cin.read(reinterpret_cast<char*>(raw.data()),10);std::memcpy(&abi.x87[n],raw.data(),10);}
        for(auto&vector:abi.xmm)for(auto&value:vector)value=input_word();Memory memory;std::cin.read(reinterpret_cast<char*>(memory.bank.data()),memory.bank.size());std::cin.read(reinterpret_cast<char*>(memory.ro.data()),memory.ro.size());
        const auto got=run(memory,abi,true,0,0,0,native_features);if(got.stopped)throw std::runtime_error("Native physical initialization stopped");emit_abi(got.abi);
        std::cout.write(reinterpret_cast<const char*>(memory.bank.data()),memory.bank.size());std::cout.write(reinterpret_cast<const char*>(memory.ro.data()),memory.ro.size());emit(got.stores.size());
        for(auto&event:got.stores){emit(event.instruction);emit(event.address);emit(event.written.size());std::cout.write(reinterpret_cast<const char*>(event.before.data()),event.before.size());std::cout.write(reinterpret_cast<const char*>(event.written.data()),event.written.size());}
        emit(got.pc);emit(got.child_entries);emit(got.packets.size());for(auto&[pc,state]:got.packets){emit(pc);emit_abi(state);}
    }
}
}
void reference::dispatch(WorldState&s,std::uint32_t owner,std::uint32_t site,bool){dispatch_child(s,owner,site);}
int main(int argc,char**argv){try{if(argc==2&&std::string(argv[1])=="--fixture")fixture();else if(argc==2&&std::string(argv[1])=="--fixture-native-features")fixture(true);else supplemental(argc==2&&std::string(argv[1])=="--native-feature-closure");return 0;}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
