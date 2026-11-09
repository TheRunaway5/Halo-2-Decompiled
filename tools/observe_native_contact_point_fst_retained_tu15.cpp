#include "point.cpp"
#include <csignal>
#include <csetjmp>
#include <iostream>
#include <sys/mman.h>
#include <ucontext.h>

namespace {
using namespace halo2;
using Value=std::array<std::byte,10>;
using J=std::vector<unsigned>;
void check(bool v,const char*why){if(!v)throw std::runtime_error(why);}
struct ReadFault{};struct WriteFault{};struct ObserverFault{};
std::uint16_t sw(){std::uint16_t s;asm volatile("fnstsw %0":"=a"(s));return s;}
Value value(std::uint64_t significand,std::uint16_t exponent){
    Value v;std::memcpy(v.data(),&significand,8);std::memcpy(v.data()+8,&exponent,2);return v;
}
Value take(){Value v;asm volatile("fstpt %0":"=m"(v)::"memory","st");return v;}
void load(const Value&v){asm volatile("fldt %0"::"m"(v):"memory","st");}
void exact_fst(void*base){asm volatile(".byte 0xd9,0x50,0x04"::"a"(base):"memory");}
struct State{std::uint16_t sw;std::uint32_t bits;Value value;};
constexpr std::uint32_t retained=0x211004u,base=retained-4u,local=0x610000u;
constexpr std::uint32_t preimage=0x7fc12345u;
sigjmp_buf escape;
volatile std::sig_atomic_t fault_seen{},fault_sw{},fault_cw{},fault_mx{};
void* fault_page{};
void on_fault(int sig,siginfo_t*info,void*context){
    const auto&uc=*static_cast<const ucontext_t*>(context);
    const auto*code=reinterpret_cast<const unsigned char*>(uc.uc_mcontext.gregs[REG_RIP]);
    const auto*fp=uc.uc_mcontext.fpregs;
    fault_seen=(sig==SIGSEGV&&info->si_addr==static_cast<std::byte*>(fault_page)+4&&
        code[0]==0xd9&&code[1]==0x50&&code[2]==0x04&&fp);
    if(fp){fault_sw=fp->swd;fault_cw=fp->cwd;fault_mx=fp->mxcsr;}
    siglongjmp(escape,1);
}
// No RAII scope is crossed by siglongjmp. The live Environment belongs to the caller.
bool hardware_reject(const Value&v){if(sigsetjmp(escape,1)==0){load(v);exact_fst(fault_page);return false;}return fault_seen!=0;}
class MemoryModel final:public SourcePhysicsContactMemory,
    public NativeTu15RetainedStoreObservationDomain,public NativeTu15RetainedStoreObserverProvider {
    NativeTu15RetainedStoreObserver observer_;
public:
    std::uint32_t data=preimage,private_word=0x11223344u;
    bool read_fault{},write_fault{},observer_fault{};
    mutable J order;
    unsigned observed{},pc{};
    std::uint32_t observed_before{},observed_after{};
    explicit MemoryModel(bool observed):observer_(observed?SourcePhysicsContactRecordObserver([this](const auto&e){
        order.push_back(4);++this->observed;pc=e.instruction;
        check(e.address==retained&&e.before.size()==4&&e.written.size()==4,"observer extent");
        std::memcpy(&observed_before,e.before.data(),4);std::memcpy(&observed_after,e.written.data(),4);
        check(data==observed_after,"observer called before commit");
        _mm_setcsr((_mm_getcsr()^0x6000u)|1u);
        asm volatile("fld1;fldz;fdivp;fstp %%st(0)":::"memory","st","st(1)");
        if(observer_fault)throw ObserverFault{};
    }):SourcePhysicsContactRecordObserver{}){}
    const SourcePhysicsContactRecordObserver&retained_store_observer()const noexcept override{return observer_.callback();}
    bool observes_retained_store(std::uint32_t pc,std::uint32_t p,std::size_t n)const override{
        check(n==4,"domain extent");order.push_back(1);
        if(p==local)return false;
        check(p==retained&&pc==0x322f19u,"wrong SourcePC or destination");return true;
    }
    void require_retained_extent(std::uint32_t,std::size_t)const override{}
    bool data_is_defined(std::uint32_t,std::size_t)const override{return true;}
    void read(std::uint32_t p,std::span<std::byte>b)const override{
        order.push_back(2);check(p==retained&&b.size()==4,"wrong preimage address");
        if(read_fault)throw ReadFault{};std::memcpy(b.data(),&data,4);
    }
    void write(std::uint32_t p,std::span<const std::byte>b)override{
        order.push_back(3);check((p==retained||p==local)&&b.size()==4,"wrong write address");
        if(write_fault)throw WriteFault{};
        std::memcpy(p==retained?&data:&private_word,b.data(),4);
    }
};
State original(const Value&v,std::uint16_t cw){
    NativeTu15PhysicsFrameFp fp{cw,0,0x1f80};
    joint_constraint_numeric::Environment scope(fp,0);
    alignas(16)std::array<std::uint32_t,2>words{0,preimage};load(v);exact_fst(words.data());
    return{sw(),words[1],take()};
}
void profile(const Value&v,std::uint16_t cw){
    const auto ref=original(v,cw);
    unsigned expected_sw{},expected_cw{},expected_mx{};
    {
        NativeTu15PhysicsFrameFp fp{cw,0,0x1f80};
        joint_constraint_numeric::Environment scope(fp,0);
        fault_seen=0;check(hardware_reject(v),"source hardware did not fault at authenticated FST");
        expected_sw=fault_sw;expected_cw=fault_cw;expected_mx=fault_mx;
    }
    for(unsigned mode=0;mode<6;++mode){
        MemoryModel m(mode!=0);m.read_fault=mode==2;m.write_fault=mode==3;m.observer_fault=mode==4;
        NativeTu15ContactPointCreateStats stats;
        EventMemory event(m,0x600e5030u,&stats);
        NativeTu15PhysicsFrameFp fp{cw,0,0x1f80};
        bool caught=false;unsigned after_sw{},after_mx{};Value live;
        {
            joint_constraint_numeric::Environment scope(fp,0);
            load(v);const auto initial_sw=sw();
            try{
                if(mode==5)store(event,local,0xaabbccddu);
                else keep_float(event,retained);
            }catch(const ReadFault&){check(mode==2,"unexpected preimage fault");caught=true;}
            catch(const WriteFault&){check(mode==3,"unexpected DATA fault");caught=true;}
            catch(const ObserverFault&){check(mode==4,"unexpected observer exception");caught=true;}
            after_sw=sw();after_mx=_mm_getcsr();live=take();
            if(mode==2||mode==5)check(after_sw==initial_sw&&live==v,"before-FST state changed");
        }
        check(caught==(mode>=2&&mode<=4),"fault completion differs");
        if(mode==0||mode==1||mode==4){
            check(after_sw==ref.sw&&live==ref.value&&m.data==ref.bits&&after_mx==0x1f80,"FST32 bits/no-pop/FP differ");
            if(mode){check(m.observed==1&&m.pc==0x322f19u&&m.observed_before==preimage&&m.observed_after==ref.bits&&m.order==J({1,2,3,4}),"typed source event/preimage/order differs");}
            else check(m.observed==0&&m.order==J({1,3}),"default performed hidden preimage read");
        }else if(mode==2)check(m.order==J({1,2})&&m.data==preimage&&m.observed==0,"rejected preimage publication");
        else if(mode==3)check(m.order==J({1,2,3})&&m.data==preimage&&m.observed==0&&after_sw==expected_sw&&cw==expected_cw&&after_mx==expected_mx&&live==v,"hardware write-fault FP or write boundary differs");
        else check(m.order==J({1,3})&&m.private_word==0xaabbccddu&&m.observed==0&&m.data==preimage,"private event-like write observed");
    }
}
}
int main(){try{
    const auto page=static_cast<std::size_t>(4096);
    fault_page=mmap(nullptr,page,PROT_READ,MAP_PRIVATE|MAP_ANONYMOUS|MAP_32BIT,-1,0);
    check(fault_page!=MAP_FAILED,"cannot map hardware fault DATA");
    struct sigaction action{};action.sa_sigaction=on_fault;action.sa_flags=SA_SIGINFO;
    check(sigaction(SIGSEGV,&action,nullptr)==0,"cannot install hardware fault observer");
    const std::array values{value(0x8000000000000000ull,0x3fffu),value(0x8000008000000000ull,0x3fffu),
      value(0x8000008000000000ull,0xbfffu),value(0x8000000000000000ull,0x3f60u),
      value(0x8000000000000000ull,0x4080u),value(0xc00000000000abcdull,0x7fffu),
      value(0x8000000000000000ull,0x7fffu)};
    unsigned cases{};
    for(auto rc:{0,0x400,0x800,0xc00})for(const auto&v:values){profile(v,0x27fu|rc);++cases;}
    check(munmap(fault_page,page)==0,"cannot release hardware fault DATA");
    std::cout<<"{\"passed\":true,\"normal_profiles\":"<<cases<<",\"hardware_write_fault_profiles\":"<<cases
      <<",\"native_modes_per_profile\":6,\"exact_retained_pc\":3288857,\"rollback_only_before_commit\":true,\"private_output_suppressed\":true}\n";
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}catch(...){std::cerr<<"Unexpected proof exception\n";return 1;}}
