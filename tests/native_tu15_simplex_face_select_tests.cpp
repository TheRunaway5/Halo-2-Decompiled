#include "physics/runtime/native_tu15_simplex_face_select_abi.hpp"
#define main refinement_core_main
#include "native_tu15_simplex_face_refine_tests.cpp"
#undef main
namespace reference {
#include "simplex_face_select_reference.inc"
}
namespace {
struct SelectMemory:Memory {
    std::array<std::byte,0x1000>mask{};
    void read(std::uint32_t at,std::span<std::byte>out)const override{
        if(at>=0x461000u&&std::uint64_t(at-0x461000u)+out.size()<=mask.size()){
            reads.emplace_back(at,out.size());if(++read_count==fail_read)throw std::runtime_error("Declared face-select mask read stop");if(on_read)on_read(at);std::ranges::copy(std::span(mask).subspan(at-0x461000u,out.size()),out.begin());return;
        }Memory::read(at,out);
    }
};
inline void poison_host_fp(){const std::uint16_t cw=0x740u;asm volatile("fninit;fldcw %0"::"m"(cw):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(0x8065u);}
struct SelectResult:Result{std::uint64_t child_entries{};std::vector<SourcePhysicsWorldRuntimeAbi>child_packets;};
SelectResult select_run(SelectMemory&m,const SourcePhysicsWorldRuntimeAbi&abi,bool native,unsigned feedback=0,unsigned fail_observer=0,bool ordered=false,bool hostile_fp=false){
    if(hostile_fp){const auto prior=m.on_read;m.on_read=[prior](auto address){if(prior)prior(address);poison_host_fp();};}
    SelectResult out;SourcePhysicsContactRecordObserver observer=[&](const auto&e){out.stores.push_back(e);if(feedback==1u&&e.instruction==0x30d74au)m.put(e.address,0x3f800000u);if(feedback==2u&&e.instruction==0x30d719u)m.put(e.address+16u,0x40000000u);if(feedback==3u&&e.instruction==0x30d763u)m.put(e.address+4u,0xbf800000u);if(hostile_fp)poison_host_fp();if(out.stores.size()==fail_observer)throw std::runtime_error("Declared face-select observer stop");};
    SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto pc){out.pc=pc;out.sites.insert(pc);++out.callbacks;};auto&cursor=services.instruction_cursor;constexpr std::array service_sites{0x38846eu,0x21f590u,0x33b595u};cursor.position=&out.pc;cursor.service_sites=service_sites;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    physics_step_detail::FpScope environment(*abi.floating_point);WorldState state{m,observer,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);reference::select_child_entries=0;reference::select_child_packets.clear();reference::select_ordered_children=ordered;
    auto before=[&](auto&s,std::uint32_t pc){out.packets.emplace_back(pc,s.save());if(hostile_fp)poison_host_fp();};
    try{
        if(native)native_simplex_face_select_detail::run_private_owner(state,[&](auto&s,auto target,auto site){if(target!=0x30d1a0u||site!=0x309d2fu)throw std::runtime_error("Wrong real face-refine call transport");++out.child_entries;out.child_packets.push_back(s.save());native_simplex_face_refine_detail::run_private_owner(s,before);},before);
        else if(ordered)reference::select_ordered(state);else reference::select_unmodified(state);
    }catch(const std::runtime_error&){out.stopped=true;}
    if(!native){out.child_entries=reference::select_child_entries;out.child_packets=reference::select_child_packets;}
    out.abi=state.save();return out;
}
void select_compare(const SelectMemory&a,const SelectMemory&b,const SelectResult&x,const SelectResult&y,const std::string&name){
    if(!same_abi(x.abi,y.abi)||x.pc!=y.pc||x.stopped!=y.stopped||x.child_entries!=y.child_entries||a.bank!=b.bank||a.table!=b.table||a.constant!=b.constant||a.mask!=b.mask||a.reads!=b.reads||x.stores!=y.stores){
        std::cerr<<name<<" pc "<<std::hex<<x.pc<<"/"<<y.pc<<" flags "<<x.abi.eflags<<"/"<<y.abi.eflags<<" sw "<<x.abi.x87_status<<"/"<<y.abi.x87_status<<std::dec<<" reads "<<a.reads.size()<<"/"<<b.reads.size()<<" stores "<<x.stores.size()<<"/"<<y.stores.size()<<" child "<<x.child_entries<<"/"<<y.child_entries<<"\n";
        for(unsigned n=0;n<8;++n){if(x.abi.gp[n]!=y.abi.gp[n])std::cerr<<"gp "<<n<<" differs\n";if(x.abi.xmm[n]!=y.abi.xmm[n])std::cerr<<"xmm "<<n<<" differs\n";}
        throw std::runtime_error("Native simplex face-select mismatch "+name);
    }
    if(x.child_packets.size()!=y.child_packets.size())throw std::runtime_error("Face-select child-entry count differs");for(unsigned n=0;n<x.child_packets.size();++n)if(!same_abi(x.child_packets[n],y.child_packets[n]))throw std::runtime_error("Face-select true child-entry ABI differs");
    if(y.callbacks)throw std::runtime_error("Native face-select executed literal callbacks");
}
void select_supplemental(){
    std::mt19937 rng(0x30d5d0u);unsigned cases{},prefixes{},children{},hostile_cases{};std::set<std::uint32_t>sites,choices;
    for(unsigned k=0;k<192u;++k){
        SelectMemory seed;for(auto&v:seed.bank)v=std::byte(rng());SourcePhysicsWorldRuntimeAbi abi{};for(auto&v:abi.gp)v=rng();abi.gp[0]=base+0x2000u;abi.gp[3]=base+0x1000u;abi.gp[4]=stack+k%16u;const auto frame_base=((abi.gp[4]-4u)&~15u)-0x60u;
        if(k%16u==6u)abi.gp[0]=frame_base+0x10u;if(k%16u==10u)abi.gp[3]=frame_base+0x30u;
        for(auto p:{abi.gp[0],abi.gp[3]})for(unsigned row=0;row<8u;++row)for(unsigned lane=0;lane<4u;++lane)seed.put(p+row*16u+lane*4u,std::bit_cast<std::uint32_t>(float(int(rng()%63u)-31)/8.0f));
        if(k<32u){constexpr std::array<float,16>tetra{0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0};for(unsigned n=0;n<16u;++n)seed.put(abi.gp[0]+n*4u,std::bit_cast<std::uint32_t>(k%8u==5u?0.0f:tetra[n]));for(unsigned lane=0;lane<4u;++lane)seed.put(abi.gp[3]+lane*4u,std::bit_cast<std::uint32_t>(lane==3u?0.0f:k%8u==2u?2.0f:0.25f));}
        seed.put(abi.gp[4],0x63000000u);seed.put(abi.gp[4]+4u,std::bit_cast<std::uint32_t>(k%7u==0u?0.0f:k%7u==1u?-1.0f:1.0f));seed.put(abi.gp[4]+8u,0x23456789u);
        constexpr std::array<unsigned,5>indices{2,0,1,2,0};for(unsigned n=0;n<5u;++n)seed.table[0x760u+n]=std::byte(indices[n]);seed.put(0x46a190u,0x3f8ccccdu);seed.put(0x46a454u,0x358637beu);const std::uint32_t absolute_mask=0x7fffffffu;for(unsigned lane=0;lane<4u;++lane)std::memcpy(seed.mask.data()+0x950u+lane*4u,&absolute_mask,4);
        abi.eflags=0x202u|(rng()&0xcd5u);abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(0x37fu|((k%4u)<<10u)),0x1f80u|((k%4u)<<13u)|((k&4u)?0x8040u:0u)};abi.x87_depth=std::array{0u,1u,3u,5u}[k%4u];abi.x87_status=0x4100u|((k%8u)<<11u);for(unsigned n=0;n<abi.x87_depth;++n)abi.x87[n]=n%2u?-1.234567890123456789L:9.876543210987654321L;for(auto&v:abi.xmm)for(auto&w:v)w=rng();
        SelectMemory a=seed,b=seed;const auto x=select_run(a,abi,false,k%4u),y=select_run(b,abi,true,k%4u);select_compare(a,b,x,y,"select-normal-"+std::to_string(k));if(x.stopped)throw std::runtime_error("Normal face-select stopped");++cases;children+=x.child_entries;sites.insert(x.sites.begin(),x.sites.end());choices.insert(x.abi.gp[0]);
        if(k<8u||x.child_entries){SelectMemory c=seed,d=seed;const auto u=select_run(c,abi,false,k%4u),v=select_run(d,abi,true,k%4u,0,false,true);select_compare(c,d,u,v,"hostile-host-FP-"+std::to_string(k));++hostile_cases;
            if(k<2u){SelectMemory e=seed,f=seed;const auto r=select_run(e,abi,false,0,1,true),t=select_run(f,abi,true,0,1,false,true);select_compare(e,f,r,t,"hostile-observer-throw-"+std::to_string(k));++hostile_cases;}}
        if(k<4u||(x.child_entries&&children<=4u))for(unsigned category=0;category<2u;++category){const auto limit=category==0?a.read_count:unsigned(x.stores.size());for(unsigned n=1;n<=limit;++n){SelectMemory c=seed,d=seed;if(!category)c.fail_read=d.fail_read=n;const auto u=select_run(c,abi,false,0,category?n:0,true),v=select_run(d,abi,true,0,category?n:0);select_compare(c,d,u,v,"select-prefix-"+std::to_string(k)+"-"+std::to_string(category)+"-"+std::to_string(n));if(!u.stopped)throw std::runtime_error("Face-select prefix did not stop");++prefixes;}}
    }
    const auto parent_sites=std::count_if(sites.begin(),sites.end(),[](auto pc){return pc>=0x30d5d0u&&pc<=0x30d82bu;});
    std::cerr<<"{\"canonical_cases\":"<<cases<<",\"read_observer_prefixes\":"<<prefixes<<",\"source_and_child_sites\":"<<sites.size()<<",\"parent_sites\":"<<parent_sites<<",\"child_sites\":"<<(sites.size()-parent_sites)<<",\"actual_refine_entries\":"<<children<<",\"hostile_host_fp_cases\":"<<hostile_cases<<",\"returned_choices\":"<<choices.size()<<",\"native_literal_callbacks\":0}\n";
}
void select_fixture(){for(unsigned count=input_word(),k=0;k<count;++k){SourcePhysicsWorldRuntimeAbi abi{};for(auto&v:abi.gp)v=input_word();abi.eflags=input_word();auto mxcsr=input_word(),cw=input_word();abi.x87_status=input_word();abi.x87_depth=input_word();abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(cw),mxcsr};for(unsigned n=0;n<abi.x87_depth;++n){std::array<std::byte,10>b{};std::cin.read(reinterpret_cast<char*>(b.data()),10);std::memcpy(&abi.x87[n],b.data(),10);}for(auto&v:abi.xmm)for(auto&w:v)w=input_word();SelectMemory memory;for(auto b:{std::span<std::byte>(memory.bank),std::span<std::byte>(memory.table),std::span<std::byte>(memory.constant),std::span<std::byte>(memory.mask)})std::cin.read(reinterpret_cast<char*>(b.data()),b.size());auto got=select_run(memory,abi,true);if(got.stopped)throw std::runtime_error("Native physical face-select stopped");emit_abi(got.abi);for(auto b:{std::span<std::byte>(memory.bank),std::span<std::byte>(memory.table),std::span<std::byte>(memory.constant),std::span<std::byte>(memory.mask)})std::cout.write(reinterpret_cast<const char*>(b.data()),b.size());emit(got.stores.size());for(auto&e:got.stores){emit(e.instruction);emit(e.address);emit(e.written.size());std::cout.write(reinterpret_cast<const char*>(e.before.data()),e.before.size());std::cout.write(reinterpret_cast<const char*>(e.written.data()),e.written.size());}emit(got.pc);emit(got.packets.size());for(auto&[pc,s]:got.packets){emit(pc);emit_abi(s);}}}
}
#ifndef HALO2_SIMPLEX_FACE_SELECT_TEST_COMPONENT_ONLY
int main(int argc,char**argv){try{if(argc==2&&std::string(argv[1])=="--fixture")select_fixture();else select_supplemental();return 0;}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}

#endif
