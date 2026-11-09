#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "physics/runtime/native_tu15_simplex_projection_abi.hpp"
#include <algorithm>
#include <bit>
#include <iostream>
#include <random>
#include <set>
#include <vector>
using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
using Next=NativeTu15SimplexProjectionNext;
namespace reference {
#include "simplex_projection_reference.inc"
}
namespace {
constexpr std::uint32_t base=0x61000000u,work=base+0x100u,stack=base+0x3000u;
struct Memory:SourcePhysicsContactMemory {
    std::vector<std::byte>bank=std::vector<std::byte>(0x4000u);
    std::array<std::byte,0x1000>ro=[] {std::array<std::byte,0x1000>value{};const std::uint32_t bits=0x34000000u;std::memcpy(value.data()+0x658u,&bits,4u);return value;}();
    mutable std::vector<std::pair<std::uint32_t,std::size_t>>reads;
    mutable unsigned read_count{};unsigned write_count{},fail_read{},fail_write{};
    std::span<std::byte>view(std::uint32_t at,std::size_t size){
        if(at>=0x41d000u&&std::uint64_t(at-0x41d000u)+size<=ro.size())return std::span(ro).subspan(at-0x41d000u,size);
        if(at<base||std::uint64_t(at-base)+size>bank.size())throw std::runtime_error("Projection DATA escaped bank");
        return std::span(bank).subspan(at-base,size);
    }
    void read(std::uint32_t at,std::span<std::byte>out)const override{
        reads.emplace_back(at,out.size());
        if(++read_count==fail_read)throw std::runtime_error("Declared projection read stop");
        std::ranges::copy(const_cast<Memory*>(this)->view(at,out.size()),out.begin());
    }
    void write(std::uint32_t at,std::span<const std::byte>in)override{if(++write_count==fail_write)throw std::runtime_error("Declared projection write stop");std::ranges::copy(in,view(at,in.size()).begin());}
    void put(std::uint32_t at,std::uint32_t value){std::memcpy(view(at,4).data(),&value,4);}
};
struct Result {
    SourcePhysicsWorldRuntimeAbi abi;std::uint32_t pc{};bool stopped{};Next next{};
    std::vector<SourcePhysicsContactRecordStore>stores;
    std::vector<std::pair<std::uint32_t,SourcePhysicsWorldRuntimeAbi>>packets;
    std::set<std::uint32_t>sites;std::uint64_t callbacks{};
};
Result run(Memory&m,const SourcePhysicsWorldRuntimeAbi&abi,bool native,unsigned feedback=0,bool unmodified_reference=false,unsigned fail_observer=0){
    Result out;SourcePhysicsContactRecordObserver observe=[&](const auto&e){
        out.stores.push_back(e);
        if(feedback==1u&&e.instruction==0x30ee08u)m.put(work+0xa0u,0x41000000u);
        if(feedback==2u&&e.instruction==0x30ee36u)m.put(e.address,0x3f800000u);
        if(feedback==3u&&e.instruction==0x30ee76u)m.put(abi.gp[4]+0x10u,0xdeadbeefu);
        if(out.stores.size()==fail_observer)throw std::runtime_error("Declared projection observer stop");
    };
    SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto pc){out.pc=pc;out.sites.insert(pc);++out.callbacks;};
    constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};auto&cursor=services.instruction_cursor;
    cursor.position=&out.pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    physics_step_detail::FpScope environment(*abi.floating_point);WorldState s{m,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(abi);
    try{
        if(native)out.next=native_simplex_projection_detail::run_private_stage(s,[&](auto&state,std::uint32_t pc){out.packets.emplace_back(pc,state.save());});
        else out.next=unmodified_reference?reference::run_unmodified(s):reference::run_stage(s);
    }catch(const std::runtime_error&){out.stopped=true;}
    out.abi=s.save();return out;
}
bool same_abi(const auto&a,const auto&b){
    if(a.gp!=b.gp||a.eflags!=b.eflags||a.xmm!=b.xmm||a.x87_depth!=b.x87_depth||a.x87_status!=b.x87_status||a.floating_point->mxcsr!=b.floating_point->mxcsr||a.floating_point->x87_control_word!=b.floating_point->x87_control_word)return false;
    for(unsigned n=0;n<a.x87_depth;++n)if(std::memcmp(&a.x87[n],&b.x87[n],10))return false;return true;
}
void compare(const Memory&a,const Memory&b,const Result&x,const Result&y,const std::string&name){
    if(!same_abi(x.abi,y.abi)||x.pc!=y.pc||x.stopped!=y.stopped||x.next!=y.next||a.bank!=b.bank||a.ro!=b.ro||a.reads!=b.reads||x.stores!=y.stores){
        std::cerr<<name<<" pc "<<std::hex<<x.pc<<"/"<<y.pc<<" flags "<<x.abi.eflags<<"/"<<y.abi.eflags<<" fp "<<x.abi.x87_status<<"/"<<y.abi.x87_status<<std::dec<<" reads "<<a.reads.size()<<"/"<<b.reads.size()<<" stores "<<x.stores.size()<<"/"<<y.stores.size()<<"\n";
        for(unsigned n=0;n<8;++n){if(x.abi.gp[n]!=y.abi.gp[n])std::cerr<<"gp "<<n<<" differs\n";if(x.abi.xmm[n]!=y.abi.xmm[n])std::cerr<<"xmm "<<n<<" differs\n";}
        throw std::runtime_error("Native simplex projection mismatch "+name);
    }
    if(y.callbacks)throw std::runtime_error("Native projection used literal callbacks");
}
void supplemental(bool unmodified_reference=false){
    std::mt19937 rng(0x30edd5u);unsigned cases{},prefixes{};std::set<std::uint32_t>sites;std::set<Next>branches;
    for(unsigned k=0;k<128u;++k){
        Memory seed;for(auto&v:seed.bank)v=std::byte(rng());SourcePhysicsWorldRuntimeAbi abi{};for(auto&v:abi.gp)v=rng();
        abi.gp[0]=k%4u?0u:std::array{1u,0xffffffffu,0x80000000u}[(k/4u)%3u];abi.gp[4]=k%8u==1u?work+0x100u:stack;abi.gp[6]=work;
        for(unsigned lane=0;lane<4u;++lane){
            seed.put(work+0x20u+lane*4u,std::bit_cast<std::uint32_t>(float(1+int(rng()%31))/8.0f));
            seed.put(work+0xa0u+lane*4u,std::bit_cast<std::uint32_t>((k&2u?-1.0f:1.0f)*float(32+int(rng()%31))/8.0f));
            seed.put(work+0x120u+lane*4u,std::bit_cast<std::uint32_t>(float(1+int(rng()%7))/8.0f));
        }
        seed.put(abi.gp[4]+0x14u,0x2edbe6ffu);seed.put(abi.gp[4]+0x1cu,k);
        abi.eflags=0x202u|(rng()&0xcd5u);abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(0x37fu|((k%4)<<10u)),0x1f80u|((k%4)<<13u)|((k&4u)?0x8040u:0u)};
        abi.x87_depth=std::array{0u,1u,4u,6u}[k%4];abi.x87_status=0x4100u|((k%8)<<11u);for(unsigned n=0;n<abi.x87_depth;++n)abi.x87[n]=n%2?-(1.234567890123456789L+n):(9.876543210987654321L+n);
        for(auto&v:abi.xmm)for(auto&word:v)word=rng();
        Memory a=seed,b=seed;const auto x=run(a,abi,false,k%4u,unmodified_reference),y=run(b,abi,true,k%4u);compare(a,b,x,y,"normal-"+std::to_string(k));
        if(x.stopped)throw std::runtime_error("Normal projection stopped");sites.insert(x.sites.begin(),x.sites.end());branches.insert(x.next);++cases;
        if(k<16u&&!unmodified_reference){
            Memory baseline=seed;const auto unchanged=run(baseline,abi,false);
            for(unsigned category=0;category<3u;++category){const auto limit=category==0?baseline.read_count:category==1?baseline.write_count:unsigned(unchanged.stores.size());
                for(unsigned n=1;n<=limit;++n){Memory c=seed,d=seed;if(category==0)c.fail_read=d.fail_read=n;if(category==1)c.fail_write=d.fail_write=n;
                    const auto r=run(c,abi,false,0,false,category==2?n:0),s=run(d,abi,true,0,false,category==2?n:0);compare(c,d,r,s,"retired-prefix-"+std::to_string(k)+"-"+std::to_string(category)+"-"+std::to_string(n));
                    if(!r.stopped)throw std::runtime_error("Projection prefix did not stop");++prefixes;
                }
            }
        }
    }
    if(branches.size()!=3u)throw std::runtime_error("Projection branch coverage incomplete");
    std::cerr<<"{\"canonical_normal_cases\":"<<cases<<",\"retired_fp_primitive_prefixes\":"<<prefixes<<",\"source_sites\":"<<sites.size()<<",\"next_branches\":"<<branches.size()<<",\"native_literal_callbacks\":0,\"physical_fault_atomicity\":false}\n";
}
std::uint32_t input_word(){std::uint32_t value{};std::cin.read(reinterpret_cast<char*>(&value),4);if(!std::cin)throw std::runtime_error("Truncated projection fixture");return value;}
void emit(std::uint32_t value){std::cout.write(reinterpret_cast<const char*>(&value),4);}
void emit_abi(const SourcePhysicsWorldRuntimeAbi&abi){for(auto value:abi.gp)emit(value);emit(abi.eflags);emit(abi.floating_point->mxcsr);emit(abi.x87_status);emit(abi.floating_point->x87_control_word);emit(abi.x87_depth);for(unsigned n=0;n<abi.x87_depth;++n)std::cout.write(reinterpret_cast<const char*>(&abi.x87[n]),10);for(auto&vector:abi.xmm)for(auto value:vector)emit(value);}
void fixture(){
    const unsigned count=input_word();for(unsigned k=0;k<count;++k){
        SourcePhysicsWorldRuntimeAbi abi{};for(auto&value:abi.gp)value=input_word();abi.eflags=input_word();const auto mxcsr=input_word(),cw=input_word();abi.x87_status=input_word();abi.x87_depth=input_word();abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(cw),mxcsr};
        for(unsigned n=0;n<abi.x87_depth;++n){std::array<std::byte,10>raw{};std::cin.read(reinterpret_cast<char*>(raw.data()),10);std::memcpy(&abi.x87[n],raw.data(),10);}for(auto&vector:abi.xmm)for(auto&value:vector)value=input_word();
        Memory memory;std::cin.read(reinterpret_cast<char*>(memory.bank.data()),memory.bank.size());std::cin.read(reinterpret_cast<char*>(memory.ro.data()),memory.ro.size());const auto got=run(memory,abi,true);if(got.stopped)throw std::runtime_error("Native physical projection stopped");emit_abi(got.abi);std::cout.write(reinterpret_cast<const char*>(memory.bank.data()),memory.bank.size());std::cout.write(reinterpret_cast<const char*>(memory.ro.data()),memory.ro.size());emit(got.stores.size());
        for(auto&event:got.stores){emit(event.instruction);emit(event.address);emit(event.written.size());std::cout.write(reinterpret_cast<const char*>(event.before.data()),event.before.size());std::cout.write(reinterpret_cast<const char*>(event.written.data()),event.written.size());}emit(got.pc);emit(std::uint32_t(got.next));emit(got.packets.size());for(auto&[pc,state]:got.packets){emit(pc);emit_abi(state);}
    }
}
}
int main(int argc,char**argv){try{if(argc==2&&std::string(argv[1])=="--fixture")fixture();else supplemental(argc==2&&std::string(argv[1])=="--unmodified-reference");return 0;}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
