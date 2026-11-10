#include "physics/runtime/native_tu15_closest_segments_frame.hpp"
#define main refinement_fixture_main
#include "native_tu15_simplex_face_refine_tests.cpp"
#undef main
namespace closest_reference {
#include "closest_segments_reference.inc"
}
namespace {
void closest_poison(){const std::uint16_t cw=0x740u;asm volatile("fninit;fldcw %0"::"m"(cw):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");_mm_setcsr(0x8065u);}
struct ClosestHostileMemory:Memory {
    explicit ClosestHostileMemory(const Memory&source):Memory(source){}
    void read(std::uint32_t address,std::span<std::byte>out)const override{try{Memory::read(address,out);}catch(...){closest_poison();throw;}closest_poison();}
    void write(std::uint32_t address,std::span<const std::byte>in)override{try{Memory::write(address,in);}catch(...){closest_poison();throw;}closest_poison();}
};
Result closest_run(Memory&m,const SourcePhysicsWorldRuntimeAbi&abi,bool native,bool unmodified=false,unsigned fail_observer=0,bool hostile=false){
    Result out;SourcePhysicsContactRecordObserver observe=[&](const auto&e){out.stores.push_back(e);if(hostile)closest_poison();if(out.stores.size()==fail_observer)throw std::runtime_error("Declared closest-segment observer stop");};SourcePhysicsWorldRuntimeServices services;
    services.instruction=[&](auto pc){out.pc=pc;out.sites.insert(pc);++out.callbacks;};auto&cursor=services.instruction_cursor;constexpr std::array sites{0x38846eu,0x21f590u,0x33b595u};cursor.position=&out.pc;cursor.service_sites=sites;cursor.callback_type=&services.instruction.target_type();cursor.executor=std::this_thread::get_id();cursor.literal_service_sites=true;cursor.observation_only_stores=true;
    physics_step_detail::FpScope environment(*abi.floating_point);WorldState state{m,observe,services,SourcePhysicsContactImage::TitleUpdate15};state.load(abi);
    try{if(native)native_closest_segments_detail::run_private_owner(state,[&](auto&s,auto pc){out.packets.emplace_back(pc,s.save());if(hostile)closest_poison();});else if(unmodified)closest_reference::run_unmodified(state);else closest_reference::run_ordered(state);}catch(const std::runtime_error&){out.stopped=true;}
    out.abi=state.save();return out;
}
void closest_supplemental(bool dump_inputs=false,bool unmodified=false){
    std::mt19937 rng(0x312620u);unsigned cases{},prefixes{},hostile_cases{};std::set<std::uint32_t>sites,choices;
    if(dump_inputs)emit(256u);
    for(unsigned k=0;k<(dump_inputs?256u:128u);++k){
        Memory seed;for(auto&v:seed.bank)v=std::byte(rng());SourcePhysicsWorldRuntimeAbi abi{};for(auto&v:abi.gp)v=rng();abi.gp[4]=stack+k%16u;const auto scratch=((abi.gp[4]-4u)&~15u)-0x30u;
        std::array<std::uint32_t,5>arguments{base+0x1000u,base+0x1100u,base+0x1200u,base+0x1300u,base+0x2000u};
        if(k%16u==4u)arguments[1]=scratch+0x10u;if(k%16u==8u)arguments[4]=scratch-0x10u;if(k%16u==12u)arguments[2]=scratch-0x20u;
        for(auto p:arguments)for(unsigned lane=0;lane<4u;++lane)seed.put(p+lane*4u,std::bit_cast<std::uint32_t>(float(int(rng()%63u)-31)/8.0f));
        if(k>=128u){constexpr std::array<std::uint32_t,16>edges{0u,0x80000000u,1u,0x80000001u,0x007fffffu,0x00800000u,0x7f7fffffu,0xff7fffffu,0x7f800000u,0xff800000u,0x7fc12345u,0xffc23456u,0x7f812345u,0xff823456u,0x3f800000u,0xbf800000u};for(unsigned n=0;n<arguments.size();++n)for(unsigned lane=0;lane<4u;++lane)seed.put(arguments[n]+lane*4u,edges[(k+n*3u+lane*5u)%edges.size()]);}
        if(k<32u){for(unsigned lane=0;lane<4u;++lane){seed.put(arguments[0]+lane*4u,0u);seed.put(arguments[1]+lane*4u,std::bit_cast<std::uint32_t>(lane==0?1.0f:0.0f));seed.put(arguments[2]+lane*4u,std::bit_cast<std::uint32_t>(lane==0?float(int(k%7u)-3):lane==1?1.0f:0.0f));seed.put(arguments[3]+lane*4u,std::bit_cast<std::uint32_t>(lane==1?float(k%3u):0.0f));}}
        seed.put(abi.gp[4],0x63000000u);for(unsigned n=0;n<arguments.size();++n)seed.put(abi.gp[4]+4u+n*4u,arguments[n]);
        seed.put(0x46a0d0u,0x3f800000u);seed.put(0x46a0e8u,0u);seed.put(0x46a25cu,0x35800000u);
        abi.eflags=0x202u|(rng()&0xcd5u);abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(0x37fu|((k%4u)<<10u)),0x1f80u|((k%4u)<<13u)|((k&4u)?0x8040u:0u)};abi.x87_depth=std::array{0u,1u,4u,6u}[k%4u];abi.x87_status=0x4100u|((k%8u)<<11u);for(unsigned n=0;n<abi.x87_depth;++n)abi.x87[n]=n%2u?-1.234567890123456789L:9.876543210987654321L;for(auto&v:abi.xmm)for(auto&w:v)w=rng();
        if(k>=128u){abi.floating_point->x87_control_word=std::array<std::uint16_t,3>{0x7fu,0x27fu,0x37fu}[k%3u]|((k%4u)<<10u);abi.floating_point->mxcsr|=k&0x3fu;constexpr std::array<unsigned char,10>quiet_nan{0x45,0x23,0x01,0,0,0,0,0xc0,0xff,0x7f};for(unsigned n=0;n<abi.x87_depth;++n)if(n%4u==3u)std::memcpy(&abi.x87[n],quiet_nan.data(),10);}
        if(dump_inputs){for(auto v:abi.gp)emit(v);emit(abi.eflags);emit(abi.floating_point->mxcsr);emit(abi.floating_point->x87_control_word);emit(abi.x87_status);emit(abi.x87_depth);for(unsigned n=0;n<abi.x87_depth;++n)std::cout.write(reinterpret_cast<const char*>(&abi.x87[n]),10);for(auto&v:abi.xmm)for(auto w:v)emit(w);for(auto b:{std::span<std::byte>(seed.bank),std::span<std::byte>(seed.table),std::span<std::byte>(seed.constant)})std::cout.write(reinterpret_cast<const char*>(b.data()),b.size());continue;}
        Memory a=seed,b=seed;const auto expected=closest_run(a,abi,false,unmodified),got=closest_run(b,abi,true);compare(a,b,expected,got,"closest-"+std::to_string(k));if(expected.stopped)throw std::runtime_error("Closest-segment normal source stopped");++cases;sites.insert(expected.sites.begin(),expected.sites.end());choices.insert(expected.abi.gp[0]);
        if(!unmodified&&(k<4u||k==8u||k==32u))for(unsigned category=0;category<3u;++category){const auto limit=category==0?a.read_count:category==1?a.write_count:unsigned(expected.stores.size());for(unsigned n=1;n<=limit;++n){Memory c=seed,d=seed;if(category==0)c.fail_read=d.fail_read=n;if(category==1)c.fail_write=d.fail_write=n;const auto x=closest_run(c,abi,false,false,category==2?n:0),y=closest_run(d,abi,true,false,category==2?n:0);compare(c,d,x,y,"closest-prefix-"+std::to_string(k)+"-"+std::to_string(category)+"-"+std::to_string(n));if(!x.stopped)throw std::runtime_error("Closest prefix did not stop");++prefixes;}}
        if(!unmodified&&(k<8u||k==32u)){Memory c=seed;ClosestHostileMemory d{seed};const auto x=closest_run(c,abi,false),y=closest_run(d,abi,true,false,0,true);compare(c,d,x,y,"closest-hostile-"+std::to_string(k));++hostile_cases;if(k<2u)for(unsigned kind=0;kind<3u;++kind){Memory e=seed;ClosestHostileMemory f{seed};if(kind==0)e.fail_read=f.fail_read=1;if(kind==1)e.fail_write=f.fail_write=1;const auto r=closest_run(e,abi,false,false,kind==2?1:0),t=closest_run(f,abi,true,false,kind==2?1:0,true);compare(e,f,r,t,"closest-hostile-throw-"+std::to_string(k)+"-"+std::to_string(kind));++hostile_cases;}}
    }
    std::cerr<<"{\"source_cases\":"<<cases<<",\"source_sites\":"<<sites.size()<<",\"read_write_observer_prefixes\":"<<prefixes<<",\"hostile_fp_return_throw_cases\":"<<hostile_cases<<",\"returned_masks\":"<<choices.size()<<",\"native_literal_callbacks\":0}\n";
}
void closest_fixture(){for(unsigned count=input_word(),k=0;k<count;++k){SourcePhysicsWorldRuntimeAbi abi{};for(auto&v:abi.gp)v=input_word();abi.eflags=input_word();const auto mxcsr=input_word(),cw=input_word();abi.x87_status=input_word();abi.x87_depth=input_word();abi.floating_point=SourcePhysicsFactoryFloatingPoint{std::uint16_t(cw),mxcsr};for(unsigned n=0;n<abi.x87_depth;++n){std::array<std::byte,10>b{};std::cin.read(reinterpret_cast<char*>(b.data()),10);std::memcpy(&abi.x87[n],b.data(),10);}for(auto&v:abi.xmm)for(auto&w:v)w=input_word();Memory memory;for(auto b:{std::span<std::byte>(memory.bank),std::span<std::byte>(memory.table),std::span<std::byte>(memory.constant)})std::cin.read(reinterpret_cast<char*>(b.data()),b.size());const auto got=closest_run(memory,abi,true);if(got.stopped)throw std::runtime_error("Native closest-segment physical fixture stopped");emit_abi(got.abi);for(auto b:{std::span<std::byte>(memory.bank),std::span<std::byte>(memory.table),std::span<std::byte>(memory.constant)})std::cout.write(reinterpret_cast<const char*>(b.data()),b.size());emit(got.stores.size());for(const auto&e:got.stores){emit(e.instruction);emit(e.address);emit(e.written.size());std::cout.write(reinterpret_cast<const char*>(e.before.data()),e.before.size());std::cout.write(reinterpret_cast<const char*>(e.written.data()),e.written.size());}emit(got.pc);emit(got.packets.size());for(const auto&[pc,s]:got.packets){emit(pc);emit_abi(s);}}}
}
int main(int argc,char**argv){try{if(argc==2&&std::string(argv[1])=="--fixture")closest_fixture();else closest_supplemental(argc==2&&std::string(argv[1])=="--dump-inputs",argc==2&&std::string(argv[1])=="--unmodified-source");return 0;}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
