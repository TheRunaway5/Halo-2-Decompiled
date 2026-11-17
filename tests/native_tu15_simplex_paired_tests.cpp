#include "physics/runtime/native_tu15_simplex_paired_frame.hpp"
#include "physics/runtime/native_tu15_simplex_triangle_abi.hpp"
#include <map>
#define HALO2_CLOSEST_SEGMENTS_TEST_COMPONENT_ONLY
#include "native_tu15_closest_segments_frame_tests.cpp"
namespace paired_reference {
#include "simplex_paired_reference.inc"
}
namespace {
struct PairedMemory:Memory {
    std::array<std::byte,0x1000>ones{},sign{};
    bool hostile{};
    std::span<std::byte>extra(std::uint32_t address,std::size_t size){
        if(address>=0x41c000u&&std::uint64_t(address-0x41c000u)+size<=ones.size())return std::span(ones).subspan(address-0x41c000u,size);
        if(address>=0x461000u&&std::uint64_t(address-0x461000u)+size<=sign.size())return std::span(sign).subspan(address-0x461000u,size);
        return {};
    }
    void read(std::uint32_t address,std::span<std::byte>out)const override{
        try{const auto bytes=const_cast<PairedMemory*>(this)->extra(address,out.size());if(bytes.empty())Memory::read(address,out);
        else{reads.emplace_back(address,out.size());if(++read_count==fail_read)throw std::runtime_error("Declared paired immutable read stop");if(on_read)on_read(address);std::ranges::copy(bytes,out.begin());}}
        catch(...){if(hostile)closest_poison();throw;}if(hostile)closest_poison();
    }
    void write(std::uint32_t address,std::span<const std::byte>in)override{try{Memory::write(address,in);}catch(...){if(hostile)closest_poison();throw;}if(hostile)closest_poison();}
};
struct PairedResult:Result{std::vector<std::pair<std::uint32_t,SourcePhysicsWorldRuntimeAbi>>children;};
PairedResult paired_run(PairedMemory&m,const SourcePhysicsWorldRuntimeAbi&abi,bool native,bool ordered=false,unsigned fail_observer=0,unsigned feedback=0,bool observed=true,std::pair<std::uint32_t,unsigned>fail_before={}){
    PairedResult out;SourcePhysicsContactRecordObserver observe=[&](const auto&e){out.stores.push_back(e);if(feedback==1u&&e.instruction==0x30d87fu)m.put(e.address+0x10u,0x3f800000u);if(feedback==2u&&e.instruction==0x30da43u)m.put(e.address,0x3f000000u);if(feedback==3u&&e.instruction==0x30d51eu)m.put(e.address+4u,0xbf800000u);if(m.hostile)closest_poison();if(out.stores.size()==fail_observer)throw std::runtime_error("Declared paired observer stop");};SourcePhysicsWorldRuntimeServices services;
    if(!observed)observe={};std::map<std::uint32_t,unsigned>before_counts;const auto check_before=[&](std::uint32_t pc){if(fail_before.first==pc&&++before_counts[pc]==fail_before.second)throw std::runtime_error("Declared paired Before stop");};
    services.instruction=[&](auto pc){out.pc=pc;out.sites.insert(pc);++out.callbacks;check_before(pc);};auto&cursor=services.instruction_cursor;constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};cursor.position=&out.pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    physics_step_detail::FpScope environment(*abi.floating_point);WorldState state{m,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
    const auto before=[&](auto&s,std::uint32_t pc){out.packets.emplace_back(pc,s.save());if(m.hostile)closest_poison();check_before(pc);};
    const auto child=[&](auto&s,std::uint32_t target){out.children.emplace_back(target,s.save());if(target==0x30d3f0u)native_simplex_triangle_detail::run_private_owner(s,before);else if(target==0x312620u)native_closest_segments_detail::run_private_owner(s,before);else throw std::runtime_error("Undeclared paired child target");};
    paired_reference::child=child;
    try{if(native)native_simplex_paired_detail::run_private_owner(state,[&](auto&s,auto target,auto site){if(site<0x309d40u||site>0x30a115u)throw std::runtime_error("Paired child internal callsite drift");child(s,target);},before);else if(ordered)paired_reference::run_ordered(state);else paired_reference::run_unmodified(state);}catch(const std::runtime_error&e){out.stopped=true;}
    paired_reference::child={};out.abi=state.save();return out;
}
void paired_compare(const PairedMemory&a,const PairedMemory&b,const PairedResult&x,const PairedResult&y,const std::string&name){
    compare(a,b,x,y,name);
    if(a.ones!=b.ones||a.sign!=b.sign||x.children.size()!=y.children.size())throw std::runtime_error("Paired extra banks/child count differ");
    for(unsigned n=0;n<x.children.size();++n)if(x.children[n].first!=y.children[n].first||!same_abi(x.children[n].second,y.children[n].second)){
        const auto&u=x.children[n].second;const auto&v=y.children[n].second;std::cerr<<"paired child "<<std::hex<<x.children[n].first<<" flags "<<u.eflags<<"/"<<v.eflags<<"\n";
        for(unsigned reg=0;reg<8u;++reg){if(u.gp[reg]!=v.gp[reg])std::cerr<<"child GP"<<reg<<" "<<u.gp[reg]<<"/"<<v.gp[reg]<<"\n";if(u.xmm[reg]!=v.xmm[reg])std::cerr<<"child XMM"<<reg<<" differs\n";}
        throw std::runtime_error("Paired actual child-entry ABI differs "+name);
    }
}
void paired_cases(bool dump=false,bool ordered=false,bool old_shift_counterexample=false){
    std::mt19937 rng(0x30d830u);std::set<std::uint32_t>sites,exits;unsigned count{},children{},prefixes{},free_prefixes{},before_prefixes{},hostile{};
    if(dump)emit(256u);
    for(unsigned k=0;k<(dump||ordered?256u:128u);++k){
        PairedMemory seed;for(auto&v:seed.bank)v=std::byte(rng());SourcePhysicsWorldRuntimeAbi abi{};for(auto&v:abi.gp)v=rng();abi.gp[1]=base+0x100u;abi.gp[4]=stack+k%16u;
        auto segment=base+0x1000u,triangle=base+0x2000u;const auto scratch=((abi.gp[4]-4u)&~15u)-0xb0u;
        if(k%16u==6u)segment=scratch+0x30u;if(k%16u==10u)triangle=scratch-0x100u;
        for(auto p:{abi.gp[1],segment,triangle})for(unsigned row=0;row<24u;++row)for(unsigned lane=0;lane<4u;++lane)seed.put(p+row*16u+lane*4u,std::bit_cast<std::uint32_t>(float(int(rng()%63u)-31)/8.0f));
        if(k<32u){constexpr std::array<float,12>vertices{0,0,0,0,1,0,0,0,0,1,0,0};for(unsigned n=0;n<12u;++n)seed.put(triangle+n*4u,std::bit_cast<std::uint32_t>(vertices[n]));for(unsigned point=0;point<2u;++point)for(unsigned lane=0;lane<4u;++lane)seed.put(segment+point*16u+lane*4u,std::bit_cast<std::uint32_t>(lane==2u?(point?1.0f:-1.0f):lane<2u?float(k%4u)/4.0f:0.0f));}
        if(k>=128u){constexpr std::array<std::uint32_t,16>edges{0,0x80000000u,1,0x80000001u,0x7fffffu,0x800000u,0x7f7fffffu,0xff7fffffu,0x7f800000u,0xff800000u,0x7fc12345u,0xffc23456u,0x7f812345u,0xff823456u,0x3f800000u,0xbf800000u};for(auto p:{segment,triangle,abi.gp[1]})for(unsigned row=0;row<24u;++row)for(unsigned lane=0;lane<4u;++lane)seed.put(p+row*16u+lane*4u,edges[(k+row*3u+lane*5u)%edges.size()]);}
        seed.put(base+0x4000u,2u);seed.put(base+0x4004u,3u);seed.put(abi.gp[4],0x63000000u);const std::array args{segment,triangle,base+0x4000u,base+0x4004u,k&1u,0x76543210u};for(unsigned n=0;n<args.size();++n)seed.put(abi.gp[4]+4u+n*4u,args[n]);
        constexpr std::uint32_t one=0x3f800000u,sign=0x80000000u;for(unsigned lane=0;lane<4u;++lane){std::memcpy(seed.ones.data()+0x500u+lane*4u,&one,4);std::memcpy(seed.sign.data()+0x960u+lane*4u,&sign,4);}seed.put(0x46a0d0u,one);seed.put(0x46a0e8u,0u);seed.put(0x46a25cu,0x35800000u);
        abi.eflags=0x202u|(rng()&0xcd5u);abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(0x37fu|((k%4u)<<10u)),0x1f80u|((k%4u)<<13u)|((k&4u)?0x8040u:0u)};abi.x87_depth=std::array{0u,1u,4u,6u}[k%4u];abi.x87_status=0x4100u|((k%8u)<<11u);for(unsigned n=0;n<abi.x87_depth;++n)abi.x87[n]=n%2u?-1.234567890123456789L:9.876543210987654321L;for(auto&v:abi.xmm)for(auto&w:v)w=rng();
        if(dump){for(auto v:abi.gp)emit(v);emit(abi.eflags);emit(abi.floating_point->mxcsr);emit(abi.floating_point->x87_control_word);emit(abi.x87_status);emit(abi.x87_depth);for(unsigned n=0;n<abi.x87_depth;++n)std::cout.write(reinterpret_cast<const char*>(&abi.x87[n]),10);for(auto&v:abi.xmm)for(auto w:v)emit(w);for(auto b:{std::span<std::byte>(seed.bank),std::span<std::byte>(seed.table),std::span<std::byte>(seed.constant),std::span<std::byte>(seed.ones),std::span<std::byte>(seed.sign)})std::cout.write(reinterpret_cast<const char*>(b.data()),b.size());continue;}
        PairedMemory a=seed,b=seed;if(old_shift_counterexample&&k==3u)a.fail_read=b.fail_read=292u;const auto expected=paired_run(a,abi,false,ordered,0,ordered?k%4u:0u),got=paired_run(b,abi,true,false,0,ordered?k%4u:0u);paired_compare(a,b,expected,got,"paired-"+std::to_string(k));if(expected.stopped)throw std::runtime_error("Normal paired source stopped");++count;children+=expected.children.size();sites.insert(expected.sites.begin(),expected.sites.end());exits.insert(expected.pc);
        if(ordered&&(k<8u||k==64u||k==128u||k==192u))for(bool observed:{false,true})for(unsigned category=0;category<(observed?3u:2u);++category){PairedMemory baseline=seed;const auto completed=paired_run(baseline,abi,false,true,0,0,observed);const auto limit=category==0?baseline.read_count:category==1?baseline.write_count:unsigned(completed.stores.size());for(unsigned n=1;n<=limit;++n){PairedMemory c=seed,d=seed;if(category==0)c.fail_read=d.fail_read=n;if(category==1)c.fail_write=d.fail_write=n;const auto x=paired_run(c,abi,false,true,category==2?n:0,0,observed),y=paired_run(d,abi,true,false,category==2?n:0,0,observed);paired_compare(c,d,x,y,"paired-prefix-"+std::to_string(k)+"-"+std::to_string(observed)+"-"+std::to_string(category)+"-"+std::to_string(n));if(!x.stopped)throw std::runtime_error("Paired prefix did not stop");if(observed)++prefixes;else ++free_prefixes;}}
        if(ordered&&(k==0u||k==3u))for(bool observed:{false,true}){std::map<std::uint32_t,unsigned>visits;for(const auto&[pc,packet]:got.packets){const auto ordinal=++visits[pc];PairedMemory c=seed,d=seed;const auto x=paired_run(c,abi,false,true,0,0,observed,{pc,ordinal}),y=paired_run(d,abi,true,false,0,0,observed,{pc,ordinal});paired_compare(c,d,x,y,"paired-before-"+std::to_string(k)+"-"+std::to_string(pc)+"-"+std::to_string(ordinal));if(!x.stopped)throw std::runtime_error("Paired Before prefix did not stop");++before_prefixes;}}
        if(ordered&&(k<8u||k==128u)){PairedMemory c=seed,d=seed;d.hostile=true;const auto x=paired_run(c,abi,false,true),y=paired_run(d,abi,true);paired_compare(c,d,x,y,"paired-hostile-"+std::to_string(k));++hostile;if(k<2u)for(unsigned category=0;category<3u;++category){PairedMemory e=seed,f=seed;f.hostile=true;if(category==0)e.fail_read=f.fail_read=1;if(category==1)e.fail_write=f.fail_write=1;const auto r=paired_run(e,abi,false,true,category==2?1:0),t=paired_run(f,abi,true,false,category==2?1:0);paired_compare(e,f,r,t,"paired-hostile-throw-"+std::to_string(k));++hostile;}}
    }
    std::cerr<<"{\"canonical_cases\":"<<count<<",\"source_sites\":"<<sites.size()<<",\"genuine_child_calls\":"<<children<<",\"return_sites\":"<<exits.size()<<",\"read_write_observer_prefixes\":"<<prefixes<<",\"observer_free_read_write_prefixes\":"<<free_prefixes<<",\"before_prefixes\":"<<before_prefixes<<",\"hostile_fp_return_throw_cases\":"<<hostile<<"}\n";
}
void paired_fixture(){for(unsigned count=input_word(),k=0;k<count;++k){SourcePhysicsWorldRuntimeAbi abi{};for(auto&v:abi.gp)v=input_word();abi.eflags=input_word();const auto mxcsr=input_word(),cw=input_word();abi.x87_status=input_word();abi.x87_depth=input_word();abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(cw),mxcsr};for(unsigned n=0;n<abi.x87_depth;++n){std::array<std::byte,10>b{};std::cin.read(reinterpret_cast<char*>(b.data()),10);std::memcpy(&abi.x87[n],b.data(),10);}for(auto&v:abi.xmm)for(auto&w:v)w=input_word();PairedMemory memory;for(auto b:{std::span<std::byte>(memory.bank),std::span<std::byte>(memory.table),std::span<std::byte>(memory.constant),std::span<std::byte>(memory.ones),std::span<std::byte>(memory.sign)})std::cin.read(reinterpret_cast<char*>(b.data()),b.size());const auto got=paired_run(memory,abi,true);if(got.stopped)throw std::runtime_error("Native paired hardware fixture stopped");emit_abi(got.abi);for(auto b:{std::span<std::byte>(memory.bank),std::span<std::byte>(memory.table),std::span<std::byte>(memory.constant),std::span<std::byte>(memory.ones),std::span<std::byte>(memory.sign)})std::cout.write(reinterpret_cast<const char*>(b.data()),b.size());emit(got.stores.size());for(const auto&e:got.stores){emit(e.instruction);emit(e.address);emit(e.written.size());std::cout.write(reinterpret_cast<const char*>(e.before.data()),e.before.size());std::cout.write(reinterpret_cast<const char*>(e.written.data()),e.written.size());}emit(got.pc);emit(got.packets.size());for(const auto&[pc,s]:got.packets){emit(pc);emit_abi(s);}}}
}
#ifndef HALO2_SIMPLEX_PAIRED_TEST_COMPONENT_ONLY
int main(int argc,char**argv){try{if(argc==2&&std::string(argv[1])=="--fixture")paired_fixture();else paired_cases(argc==2&&std::string(argv[1])=="--dump-inputs",argc==2&&std::string(argv[1])=="--ordered",argc==2&&std::string(argv[1])=="--unmodified-shift-prefix");return 0;}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
#endif
