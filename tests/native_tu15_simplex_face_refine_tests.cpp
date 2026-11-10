#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_simplex_face_refine_abi.hpp"
#include <algorithm>
#include <bit>
#include <iostream>
#include <random>
#include <set>
#include <vector>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
namespace reference {
#include "simplex_face_refine_reference.inc"
}
namespace {
constexpr std::uint32_t base=0x61000000u,stack=base+0x7000u;
struct Memory:SourcePhysicsContactMemory {
    std::vector<std::byte>bank=std::vector<std::byte>(0x8000u);
    std::array<std::byte,0x1000>table{},constant{};
    mutable std::vector<std::pair<std::uint32_t,std::size_t>>reads;
    std::function<void(std::uint32_t)>on_read;
    mutable unsigned read_count{};unsigned write_count{},fail_read{},fail_write{};
    std::span<std::byte>view(std::uint32_t at,std::size_t size){
        if(at>=0x41d000u&&std::uint64_t(at-0x41d000u)+size<=table.size())return std::span(table).subspan(at-0x41d000u,size);
        if(at>=0x46a000u&&std::uint64_t(at-0x46a000u)+size<=constant.size())return std::span(constant).subspan(at-0x46a000u,size);
        if(at<base||std::uint64_t(at-base)+size>bank.size())throw std::runtime_error("Face refinement DATA escaped bank");
        return std::span(bank).subspan(at-base,size);
    }
    void read(std::uint32_t at,std::span<std::byte>out)const override{
        reads.emplace_back(at,out.size());if(++read_count==fail_read)throw std::runtime_error("Declared face-refine read stop");
        if(on_read)on_read(at);std::ranges::copy(const_cast<Memory*>(this)->view(at,out.size()),out.begin());
    }
    void write(std::uint32_t at,std::span<const std::byte>in)override{if(++write_count==fail_write)throw std::runtime_error("Declared face-refine write stop");std::ranges::copy(in,view(at,in.size()).begin());}
    void put(std::uint32_t at,std::uint32_t value){std::memcpy(view(at,4).data(),&value,4);}
};
struct Result{SourcePhysicsWorldRuntimeAbi abi;std::uint32_t pc{};bool stopped{};std::vector<SourcePhysicsContactRecordStore>stores;std::vector<std::pair<std::uint32_t,SourcePhysicsWorldRuntimeAbi>>packets;std::set<std::uint32_t>sites;std::uint64_t callbacks{};};
Result run(Memory&m,const SourcePhysicsWorldRuntimeAbi&abi,bool native,unsigned feedback=0,unsigned fail_observer=0,bool ordered=false){
    Result out;SourcePhysicsContactRecordObserver observe=[&](const auto&e){out.stores.push_back(e);
        if(feedback==1u&&e.instruction==0x30d25eu)m.put(e.address,0x3f800000u);
        if(feedback==2u&&e.instruction==0x30d2e3u)m.put(abi.gp[4]+4u,base+0x1100u);
        if(feedback==3u&&e.instruction==0x30d277u)m.put(e.address-8u,0xbf800000u);
        if(out.stores.size()==fail_observer)throw std::runtime_error("Declared face-refine observer stop");};
    SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto pc){out.pc=pc;out.sites.insert(pc);++out.callbacks;};auto&cursor=services.instruction_cursor;
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};cursor.position=&out.pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    physics_step_detail::FpScope environment(*abi.floating_point);WorldState state{m,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
    try{if(native)native_simplex_face_refine_detail::run_private_owner(state,[&](auto&s,auto pc){out.packets.emplace_back(pc,s.save());});else if(ordered)reference::child_ordered(state);else reference::child_unmodified(state);}catch(const std::runtime_error&){out.stopped=true;}
    out.abi=state.save();return out;
}
bool same_abi(const auto&a,const auto&b){if(a.gp!=b.gp||a.eflags!=b.eflags||a.xmm!=b.xmm||a.x87_depth!=b.x87_depth||a.x87_status!=b.x87_status||a.floating_point->mxcsr!=b.floating_point->mxcsr||a.floating_point->x87_control_word!=b.floating_point->x87_control_word)return false;for(unsigned n=0;n<a.x87_depth;++n)if(std::memcmp(&a.x87[n],&b.x87[n],10))return false;return true;}
void compare(const Memory&a,const Memory&b,const Result&x,const Result&y,const std::string&name){
    if(!same_abi(x.abi,y.abi)||x.pc!=y.pc||x.stopped!=y.stopped||a.bank!=b.bank||a.table!=b.table||a.constant!=b.constant||a.reads!=b.reads||x.stores!=y.stores){
        std::cerr<<name<<" pc "<<std::hex<<x.pc<<"/"<<y.pc<<" flags "<<x.abi.eflags<<"/"<<y.abi.eflags<<" sw "<<x.abi.x87_status<<"/"<<y.abi.x87_status<<std::dec<<" reads "<<a.reads.size()<<"/"<<b.reads.size()<<" stores "<<x.stores.size()<<"/"<<y.stores.size()<<"\n";
        for(unsigned n=0;n<8;++n){if(x.abi.gp[n]!=y.abi.gp[n])std::cerr<<"gp "<<n<<" differs "<<std::hex<<x.abi.gp[n]<<"/"<<y.abi.gp[n]<<std::dec<<"\n";if(x.abi.xmm[n]!=y.abi.xmm[n])std::cerr<<"xmm "<<n<<" differs\n";}
        throw std::runtime_error("Native simplex face-refine mismatch "+name);
    }if(y.callbacks)throw std::runtime_error("Native face-refine used literal callbacks");
}
void supplemental(){
    std::mt19937 rng(0x30d1a0u);unsigned cases{},prefixes{};std::set<std::uint32_t>sites,choices;
    for(unsigned k=0;k<192u;++k){
        Memory seed;for(auto&v:seed.bank)v=std::byte(rng());SourcePhysicsWorldRuntimeAbi abi{};for(auto&v:abi.gp)v=rng();abi.gp[7]=base+0x2000u;abi.gp[4]=stack+k%16u;
        auto point=base+0x1000u,scores=base+0x3000u;const auto scratch=((abi.gp[4]-4u)&~15u)-0x20u;
        if(k%16u==6u)abi.gp[7]=scratch-0x30u;if(k%16u==10u)scores=scratch+0x10u;
        for(auto p:{point,base+0x1100u,abi.gp[7]})for(unsigned row=0;row<12u;++row)for(unsigned lane=0;lane<4u;++lane)seed.put(p+row*16u+lane*4u,std::bit_cast<std::uint32_t>(float(int(rng()%63u)-31)/8.0f));
        for(unsigned n=0;n<3u;++n)seed.put(scores+n*4u,std::bit_cast<std::uint32_t>(float(int((k>>(n*2u))%4u)-1)));
        seed.put(abi.gp[4],0x63000000u);seed.put(abi.gp[4]+4u,point);seed.put(abi.gp[4]+8u,scores);
        constexpr std::array<unsigned,5>indices{2,0,1,2,0};for(unsigned n=0;n<indices.size();++n)seed.table[0x760u+n]=std::byte(indices[n]);
        abi.eflags=0x202u|(rng()&0xcd5u);abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(0x37fu|((k%4u)<<10u)),0x1f80u|((k%4u)<<13u)|((k&4u)?0x8040u:0u)};abi.x87_depth=std::array{0u,1u,4u,6u}[k%4u];abi.x87_status=0x4100u|((k%8u)<<11u);
        for(unsigned n=0;n<abi.x87_depth;++n)abi.x87[n]=n%2u?-1.234567890123456789L:9.876543210987654321L;for(auto&v:abi.xmm)for(auto&w:v)w=rng();
        Memory a=seed,b=seed;auto x=run(a,abi,false,k%4u),y=run(b,abi,true,k%4u);compare(a,b,x,y,"normal-"+std::to_string(k));if(x.stopped)throw std::runtime_error("Normal face-refine stopped");++cases;sites.insert(x.sites.begin(),x.sites.end());choices.insert(x.abi.gp[0]);
        if(k<8u)for(unsigned category=0;category<2u;++category){const auto limit=category==0?a.read_count:unsigned(x.stores.size());for(unsigned n=1;n<=limit;++n){Memory c=seed,d=seed;if(!category)c.fail_read=d.fail_read=n;auto u=run(c,abi,false,0,category?n:0,true),v=run(d,abi,true,0,category?n:0);compare(c,d,u,v,"prefix-"+std::to_string(k)+"-"+std::to_string(category)+"-"+std::to_string(n));if(!u.stopped)throw std::runtime_error("Face-refine prefix did not stop");++prefixes;}}
    }
    std::cerr<<"{\"canonical_cases\":"<<cases<<",\"read_observer_prefixes\":"<<prefixes<<",\"source_sites\":"<<sites.size()<<",\"returned_choices\":"<<choices.size()<<",\"native_literal_callbacks\":0}\n";
}
std::uint32_t input_word(){std::uint32_t v{};std::cin.read(reinterpret_cast<char*>(&v),4);if(!std::cin)throw std::runtime_error("Truncated face-refine fixture");return v;}
void emit(std::uint32_t v){std::cout.write(reinterpret_cast<const char*>(&v),4);}
void emit_abi(const SourcePhysicsWorldRuntimeAbi&abi){for(auto v:abi.gp)emit(v);emit(abi.eflags);emit(abi.floating_point->mxcsr);emit(abi.x87_status);emit(abi.floating_point->x87_control_word);emit(abi.x87_depth);for(unsigned n=0;n<abi.x87_depth;++n)std::cout.write(reinterpret_cast<const char*>(&abi.x87[n]),10);for(auto&v:abi.xmm)for(auto w:v)emit(w);}
void fixture(){for(unsigned count=input_word(),k=0;k<count;++k){SourcePhysicsWorldRuntimeAbi abi{};for(auto&v:abi.gp)v=input_word();abi.eflags=input_word();auto mxcsr=input_word(),cw=input_word();abi.x87_status=input_word();abi.x87_depth=input_word();abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(cw),mxcsr};for(unsigned n=0;n<abi.x87_depth;++n){std::array<std::byte,10>b{};std::cin.read(reinterpret_cast<char*>(b.data()),10);std::memcpy(&abi.x87[n],b.data(),10);}for(auto&v:abi.xmm)for(auto&w:v)w=input_word();Memory memory;for(auto b:{std::span<std::byte>(memory.bank),std::span<std::byte>(memory.table),std::span<std::byte>(memory.constant)})std::cin.read(reinterpret_cast<char*>(b.data()),b.size());auto got=run(memory,abi,true);if(got.stopped)throw std::runtime_error("Native physical face-refine stopped");emit_abi(got.abi);for(auto b:{std::span<std::byte>(memory.bank),std::span<std::byte>(memory.table),std::span<std::byte>(memory.constant)})std::cout.write(reinterpret_cast<const char*>(b.data()),b.size());emit(got.stores.size());for(auto&e:got.stores){emit(e.instruction);emit(e.address);emit(e.written.size());std::cout.write(reinterpret_cast<const char*>(e.before.data()),e.before.size());std::cout.write(reinterpret_cast<const char*>(e.written.data()),e.written.size());}emit(got.pc);emit(got.packets.size());for(auto&[pc,s]:got.packets){emit(pc);emit_abi(s);}}}
}
int main(int argc,char**argv){try{if(argc==2&&std::string(argv[1])=="--fixture")fixture();else supplemental();return 0;}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
