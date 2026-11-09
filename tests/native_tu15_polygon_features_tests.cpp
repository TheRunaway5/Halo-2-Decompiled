#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_polygon_features_abi.hpp"
#include "halo2/native_tu15_polygon_features_value.hpp"
#include "halo2/native_tu15_contact_features.hpp"
#include <algorithm>
#include <bit>
#include <iostream>
#include <random>
#include <set>
#include <vector>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
struct ObservedWorldState:WorldState{using WorldState::WorldState;bool selected_observation_pcs{true};};
namespace reference {
#include "polygon_features_reference.inc"
}
namespace {
constexpr std::uint32_t base=0x61000000u,shape=base+0x1000u,stack=base+0x7000u;
struct Memory:SourcePhysicsContactMemory {
    std::vector<std::byte>bank=std::vector<std::byte>(0x8000u);
    mutable std::vector<std::pair<std::uint32_t,std::size_t>>reads;
    mutable unsigned read_count{};unsigned write_count{},fail_read{},fail_write{};
    std::span<std::byte>view(std::uint32_t at,std::size_t size){
        if(at<base||std::uint64_t(at-base)+size>bank.size())throw std::runtime_error("Polygon feature DATA escaped bank");
        return std::span(bank).subspan(at-base,size);
    }
    void read(std::uint32_t at,std::span<std::byte>out)const override{
        reads.emplace_back(at,out.size());if(++read_count==fail_read)throw std::runtime_error("Declared feature read stop");
        std::ranges::copy(const_cast<Memory*>(this)->view(at,out.size()),out.begin());
    }
    void write(std::uint32_t at,std::span<const std::byte>in)override{
        if(++write_count==fail_write)throw std::runtime_error("Declared feature write stop");
        std::ranges::copy(in,view(at,in.size()).begin());
    }
    void put(std::uint32_t at,std::uint32_t value){std::memcpy(view(at,4).data(),&value,4);}
};
struct Result {
    SourcePhysicsWorldRuntimeAbi abi;std::uint32_t pc{};bool stopped{};
    std::vector<SourcePhysicsContactRecordStore>stores;
    std::vector<std::pair<std::uint32_t,SourcePhysicsWorldRuntimeAbi>>packets;
    std::set<std::uint32_t>sites;std::uint64_t callbacks{},native_routes{};
};
Result run(Memory&m,const SourcePhysicsWorldRuntimeAbi&abi,bool native,bool ordered=false,unsigned feedback=0,unsigned fail_observer=0,unsigned route=0){
    Result out;SourcePhysicsContactRecordObserver observe=[&](const auto&e){
        out.stores.push_back(e);
        if(feedback==1u&&e.instruction==0x1f66beu)m.put(shape+0x20u,0x3f800000u);
        if(out.stores.size()==fail_observer)throw std::runtime_error("Declared polygon observer stop");
    };
    SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto pc){out.pc=pc;out.sites.insert(pc);++out.callbacks;};
    constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};auto&cursor=services.instruction_cursor;
    cursor.position=&out.pc;cursor.service_sites=service_sites;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    physics_step_detail::FpScope environment(*abi.floating_point);ObservedWorldState s{m,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(abi);
    const auto prior=native_polygon_features_detail::activity();
    try{
        if(native&&route==1u)reference::run_bound_world(s);
        else if(native&&route==2u)reference::run_bound_contact(s);
        else if(native)native_polygon_features_detail::run_private_owner(s,[&](auto&state,std::uint32_t pc){out.packets.emplace_back(pc,state.save());});
        else if(route==2u)reference::run_canonical(s);
        else if(ordered)reference::run_ordered(s);else reference::run_reference(s);
    }catch(const std::runtime_error&){out.stopped=true;}
    const auto after=native_polygon_features_detail::activity();out.native_routes=after.actual_calls-prior.actual_calls+after.canonical_calls-prior.canonical_calls;
    out.abi=s.save();return out;
}
bool same_abi(const auto&a,const auto&b){
    if(a.gp!=b.gp||a.eflags!=b.eflags||a.xmm!=b.xmm||a.x87_depth!=b.x87_depth||a.x87_status!=b.x87_status||a.floating_point->mxcsr!=b.floating_point->mxcsr||a.floating_point->x87_control_word!=b.floating_point->x87_control_word)return false;
    for(unsigned n=0;n<a.x87_depth;++n)if(std::memcmp(&a.x87[n],&b.x87[n],10))return false;return true;
}
void compare(const Memory&a,const Memory&b,const Result&x,const Result&y,const std::string&name,bool forced=false){
    if(!same_abi(x.abi,y.abi)||x.pc!=y.pc||x.stopped!=y.stopped||a.bank!=b.bank||a.reads!=b.reads||x.stores!=y.stores){
        std::cerr<<name<<" pc "<<std::hex<<x.pc<<"/"<<y.pc<<" flags "<<x.abi.eflags<<"/"<<y.abi.eflags<<std::dec<<" reads "<<a.reads.size()<<"/"<<b.reads.size()<<" stores "<<x.stores.size()<<"/"<<y.stores.size()<<"\n";
        for(unsigned n=0;n<8;++n){if(x.abi.gp[n]!=y.abi.gp[n])std::cerr<<"gp "<<n<<" differs\n";if(x.abi.xmm[n]!=y.abi.xmm[n])std::cerr<<"xmm "<<n<<" differs\n";}
        throw std::runtime_error("Native polygon feature mismatch "+name);
    }
    if(y.callbacks&&!forced)throw std::runtime_error("Native polygon features used literal callbacks");
}
void admission(){
    for(unsigned mode=0;mode<11u;++mode){
        Memory memory;SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observe=[](const auto&){};
        std::uint32_t pc=0xabcdef12u;services.instruction=[](std::uint32_t){};constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u},wrong{0x38846fu,0x21f590u,0x33b595u};
        auto&cursor=services.instruction_cursor;cursor.position=&pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
        SourcePhysicsWorldRuntimeAbi abi{};abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x37fu,0x1f80u};
        physics_step_detail::FpScope environment(*abi.floating_point);ObservedWorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(abi);bool trace=true;
        switch(mode){
        case 0:s.image=SourcePhysicsContactImage::MapPack11;break;
        case 1:cursor.first_trace=&trace;break;
        case 2:services.instruction=[tag=2u](std::uint32_t){(void)tag;};break;
        case 3:cursor.executor=std::thread::id{};break;
        case 4:cursor.literal_service_sites=false;break;
        case 5:cursor.service_sites=wrong;break;
        case 6:cursor.observation_only_stores=false;break;
        case 7:s.selected_observation_pcs=false;break;
        case 8:s.active_mmx=SourcePhysicsMmxAlias{};break;
        case 9:s.fp.count=9u;break;
        case 10:cursor.position=nullptr;break;
        }
        const auto gp=s.gp;const auto bank=memory.bank;const auto flags=s.flags.bits();
        if(native_polygon_features_detail::bridge(s)||s.gp!=gp||s.flags.bits()!=flags||memory.bank!=bank||memory.read_count||memory.write_count||pc!=0xabcdef12u)throw std::runtime_error("Polygon feature admission changed refused caller");
    }
}
struct ValueMemory {
    Memory&memory;std::vector<SourcePhysicsContactRecordStore>stores;bool observed{},feedback{};
    std::uint32_t word(std::uint32_t at,unsigned width){return physics_step_detail::r(memory,at,width);}
    std::array<std::byte,16>vector_bytes(std::uint32_t at){
        if(at&15u)throw std::runtime_error("Value feature load alignment");std::array<std::byte,16>bytes{};memory.read_vector(at,bytes);return bytes;
    }
    void vector_bytes(std::uint32_t pc,std::uint32_t at,const std::array<std::byte,16>&bytes){
        if(at&15u)throw std::runtime_error("Value feature store alignment");SourcePhysicsContactRecordStore e{pc,at,{},{}};
        if(observed){e.before.resize(16u);memory.read(at,e.before);e.written.assign(bytes.begin(),bytes.end());}
        memory.write(at,bytes);
        if(observed){stores.push_back(e);if(feedback)memory.put(shape+0x20u,0x3f800000u);}
    }
};
bool nonnegative(std::uint32_t value){return std::bit_cast<std::int32_t>(value)>=0;}
#include "polygon_features_value_reference.inc"
void value_tests(){
    std::mt19937 rng(0x1f66beu);
    for(unsigned k=0;k<128u;++k){
        Memory seed;for(auto&v:seed.bank)v=std::byte(rng());const auto ids=base+0x4000u;const auto output=k%3u==0u?shape+0x30u:base+0x5000u;const auto count=k%8u;
        for(unsigned n=0;n<8u;++n){const std::uint16_t id=n%4u;std::memcpy(seed.view(ids+n*2u,2).data(),&id,2u);}
        Memory a=seed,b=seed;ValueMemory left{a,{},bool(k&1u),bool(k&2u)},right{b,{},bool(k&1u),bool(k&2u)};
        original_value(left,{shape},ids,count,output);
        native_polygon_features_detail::ValueAccess access{right,shape,ids,count,output};NativeTu15PolygonFeatures data;native_tu15_expand_polygon_features(access,data);
        if(a.bank!=b.bank||a.reads!=b.reads||left.stores!=right.stores)throw std::runtime_error("Shared polygon feature value mismatch");
    }
}
void supplemental(bool forced=false){
    if(!forced){admission();value_tests();}
    std::mt19937 rng(0x1f6690u);unsigned cases{},prefixes{};std::set<std::uint32_t>sites;
    const std::array counts{0u,1u,2u,3u,4u,5u,7u,0xffffffffu,0x80000001u};
    for(unsigned k=0;k<128u;++k){
        Memory seed;for(auto&v:seed.bank)v=std::byte(rng());SourcePhysicsWorldRuntimeAbi abi{};for(auto&v:abi.gp)v=rng();abi.gp[1]=shape;abi.gp[4]=stack;
        auto count=counts[k%counts.size()];const auto ids=base+0x4000u;auto output=base+0x5000u;
        if(k%8u==1u){output=stack-16u;count=1u;}if(k%8u==2u)output=shape+0x30u;
        seed.put(stack,0x63000000u);seed.put(stack+4u,ids);seed.put(stack+8u,count);seed.put(stack+12u,output);
        for(unsigned n=0;n<8u;++n){const std::uint16_t id=n%4u;std::memcpy(seed.view(ids+n*2u,2).data(),&id,2);}
        for(unsigned row=0;row<4u;++row)for(unsigned lane=0;lane<4u;++lane)seed.put(shape+(row+2u)*16u+lane*4u,rng());
        abi.eflags=0x202u|(rng()&0xcd5u);abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(0x37fu|((k%4)<<10u)),0x1f80u|((k%4)<<13u)|((k&4u)?0x8040u:0u)};
        abi.x87_depth=std::array{0u,1u,4u,8u}[k%4];abi.x87_status=0x4100u|((k%8)<<11u);for(unsigned n=0;n<abi.x87_depth;++n)abi.x87[n]=n%2?-(1.234567890123456789L+n):(9.876543210987654321L+n);
        for(auto&v:abi.xmm)for(auto&word:v)word=rng();
        Memory a=seed,b=seed;const auto x=run(a,abi,false,false,k%2),y=run(b,abi,true,false,k%2);compare(a,b,x,y,"normal-"+std::to_string(k));
        if(x.stopped)throw std::runtime_error("Normal polygon feature case stopped");sites.insert(x.sites.begin(),x.sites.end());++cases;
        for(unsigned route=1;route<=2u;++route){Memory c=seed,d=seed;const auto r=run(c,abi,false,false,k%2,0,route),s=run(d,abi,true,false,k%2,0,route);compare(c,d,r,s,"bound-"+std::to_string(route)+"-"+std::to_string(k),forced);
            if(forced?(s.native_routes||!s.callbacks):(s.native_routes!=1u||s.callbacks))throw std::runtime_error("Polygon feature default/reference route not measured");}
        if(k<12u&&!forced){
            Memory baseline=seed;const auto unchanged=run(baseline,abi,false);
            for(unsigned category=0;category<3u;++category){const auto limit=category==0?baseline.read_count:category==1?baseline.write_count:unsigned(unchanged.stores.size());
                for(unsigned n=1;n<=limit;++n){Memory c=seed,d=seed;if(category==0)c.fail_read=d.fail_read=n;if(category==1)c.fail_write=d.fail_write=n;
                    const auto r=run(c,abi,false,true,0,category==2?n:0),s=run(d,abi,true,false,0,category==2?n:0);compare(c,d,r,s,"ordered-prefix-"+std::to_string(k)+"-"+std::to_string(category)+"-"+std::to_string(n));
                    if(!r.stopped)throw std::runtime_error("Feature prefix did not stop");++prefixes;
                }
            }
        }
    }
    if(sites.size()!=23u)throw std::runtime_error("Feature source coverage "+std::to_string(sites.size()));
    std::cerr<<"{\"canonical_cases\":"<<cases<<",\"bound_route_cases\":"<<cases*2u<<",\"forced_reference\":"<<(forced?"true":"false")<<",\"ordered_flag_retained_prefixes\":"<<prefixes<<",\"source_sites\":"<<sites.size()<<",\"native_literal_callbacks\":0}\n";
}
std::uint32_t input_word(){std::uint32_t value{};std::cin.read(reinterpret_cast<char*>(&value),4);if(!std::cin)throw std::runtime_error("Truncated polygon feature fixture");return value;}
void emit(std::uint32_t value){std::cout.write(reinterpret_cast<const char*>(&value),4);}
void emit_abi(const SourcePhysicsWorldRuntimeAbi&abi){for(auto value:abi.gp)emit(value);emit(abi.eflags);emit(abi.floating_point->mxcsr);emit(abi.x87_status);emit(abi.floating_point->x87_control_word);emit(abi.x87_depth);for(unsigned n=0;n<abi.x87_depth;++n)std::cout.write(reinterpret_cast<const char*>(&abi.x87[n]),10);for(auto&vector:abi.xmm)for(auto value:vector)emit(value);}
void fixture(){
    const unsigned count=input_word();for(unsigned k=0;k<count;++k){
        SourcePhysicsWorldRuntimeAbi abi{};for(auto&value:abi.gp)value=input_word();abi.eflags=input_word();const auto mxcsr=input_word(),cw=input_word();abi.x87_status=input_word();abi.x87_depth=input_word();abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(cw),mxcsr};
        for(unsigned n=0;n<abi.x87_depth;++n){std::array<std::byte,10>raw{};std::cin.read(reinterpret_cast<char*>(raw.data()),10);std::memcpy(&abi.x87[n],raw.data(),10);}for(auto&vector:abi.xmm)for(auto&value:vector)value=input_word();
        Memory memory;std::cin.read(reinterpret_cast<char*>(memory.bank.data()),memory.bank.size());const auto got=run(memory,abi,true);if(got.stopped)throw std::runtime_error("Native physical feature stopped");emit_abi(got.abi);std::cout.write(reinterpret_cast<const char*>(memory.bank.data()),memory.bank.size());emit(got.stores.size());
        for(auto&event:got.stores){emit(event.instruction);emit(event.address);emit(event.written.size());std::cout.write(reinterpret_cast<const char*>(event.before.data()),event.before.size());std::cout.write(reinterpret_cast<const char*>(event.written.data()),event.written.size());}emit(got.pc);emit(got.packets.size());for(auto&[pc,state]:got.packets){emit(pc);emit_abi(state);}
    }
}
}
int main(int argc,char**argv){try{if(argc==2&&std::string(argv[1])=="--fixture")fixture();else supplemental(argc==2&&std::string(argv[1])=="--forced-reference");return 0;}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
