#include "gameplay/animation/source_first_person_context_memory.hpp"
#include "helpers/source_first_person_context_memory_reference_20261028.hpp"
#include <cerrno>
#include <cfenv>
#include <ctime>
#include <iostream>
#include <new>
#include <stdexcept>
#include <string>
#include <string_view>

namespace allocation_probe {thread_local bool active{};thread_local std::size_t count{};}
void *operator new(std::size_t bytes) {
    if(allocation_probe::active)++allocation_probe::count;
    if(auto *p=std::malloc(bytes?bytes:1))return p;throw std::bad_alloc();
}
void *operator new[](std::size_t bytes) {return ::operator new(bytes);}
void operator delete(void *p)noexcept {std::free(p);}
void operator delete[](void *p)noexcept {std::free(p);}
void operator delete(void *p,std::size_t)noexcept {std::free(p);}
void operator delete[](void *p,std::size_t)noexcept {std::free(p);}

namespace {
using halo2::SourcePhysicsContactMemory;
using Reference=halo2::first_person_context_reference::ContextMemory;
using Candidate=halo2::first_person_context_detail::ContextMemory;
using Range=halo2::SourceFirstPersonContextTu15::PrivateRange;
void require(bool result,const char *message) {if(!result)throw std::runtime_error(message);}
struct Read {std::uint32_t at;std::size_t bytes;friend bool operator==(const Read &,const Read &)=default;};
struct Source final:SourcePhysicsContactMemory {
    mutable std::vector<Read> calls;
    mutable unsigned view_queries{};
    unsigned seed=17,fail_call{};
    static std::byte value(std::uint32_t at,unsigned seed) {
        return std::byte((at*37u+(at>>8)*13u+seed)&255u);
    }
    void read(std::uint32_t at,std::span<std::byte> bytes)const override {
        calls.push_back({at,bytes.size()});errno=EIO+int(calls.size());
        const bool fail=fail_call&&calls.size()==fail_call;
        const auto count=fail?bytes.size()/2:bytes.size();
        for(std::size_t n=0;n<count;++n)bytes[n]=value(at+std::uint32_t(n),seed);
        if(fail){std::feraiseexcept(FE_INEXACT);throw halo2::FormatError("source read failed");}
    }
    void write(std::uint32_t,std::span<const std::byte>)override {
        throw std::runtime_error("private context wrote its borrowed source");
    }
    std::optional<std::span<const std::byte>> try_read_view(std::uint32_t,std::size_t)const noexcept override {
        ++view_queries;return std::nullopt;
    }
};
std::vector<std::byte> bytes(std::size_t count,unsigned seed=3) {
    std::vector<std::byte> result(count);
    for(std::size_t n=0;n<count;++n)result[n]=Source::value(std::uint32_t(n),seed);
    return result;
}
bool same_ranges(const std::vector<Range> &a,const std::vector<Range> &b) {
    if(a.size()!=b.size())return false;
    for(std::size_t n=0;n<a.size();++n)if(a[n].address!=b[n].address||a[n].bytes!=b[n].bytes)return false;
    return true;
}
template<class F> std::string error(F &&f) {
    try{f();return {};}catch(const halo2::FormatError &e){return e.what();}
}
struct Pair {
    Source original_source,candidate_source;
    Reference original;
    Candidate candidate;
    explicit Pair(bool reference=false):candidate(reference) {
        original.borrow(&original_source);candidate.borrow(&candidate_source);
    }
    void write(std::uint32_t at,std::span<const std::byte> data) {
        original.write(at,data);candidate.write(at,data);
    }
    void relinquish(std::uint32_t at) {original.relinquish(at);candidate.relinquish(at);}
    void compare_state()const {
        require(same_ranges(original.ranges(),candidate.ranges()),"private checkpoint ranges differ");
    }
    void read(std::uint32_t at,std::size_t count) {
        auto a=std::vector(count,std::byte{0xcd}),b=a;
        original_source.calls.clear();candidate_source.calls.clear();
        errno=EDOM;std::feclearexcept(FE_ALL_EXCEPT);
        const auto a_error=error([&]{original.read(at,a);});
        const auto a_errno=errno,a_fp=std::fetestexcept(FE_ALL_EXCEPT);
        errno=EDOM;std::feclearexcept(FE_ALL_EXCEPT);
        const auto b_error=error([&]{candidate.read(at,b);});
        require(a==b,"read bytes or failure prefix differs");
        require(a_error==b_error,"read fault differs");
        require(a_errno==errno&&a_fp==std::fetestexcept(FE_ALL_EXCEPT),"read callback effects differ");
        require(original_source.calls==candidate_source.calls,"source read call sequence differs");
        require(!candidate_source.view_queries,"private context changed source read capability");
    }
    void pin(std::uint32_t at,std::uint32_t count,bool force=false) {
        original_source.calls.clear();candidate_source.calls.clear();
        auto a_error=error([&]{
            if(!at||!count)throw halo2::FormatError("Native FP cache owner has an empty native extent");
            std::vector<std::byte> copy(count);original.read(at,copy);original.write(at,copy);
        });
        auto b_error=error([&]{candidate.pin_range(at,count,force);});
        require(a_error==b_error,"pin error differs");
        require(original_source.calls==candidate_source.calls,"pin source read call sequence differs");
        compare_state();
    }
};
void seams(bool reference) {
    Pair pair(reference);
    pair.read(0,0);pair.read(17,9000);
    // Alternate private/unwritten runs around mask-word and page boundaries.
    for(const auto &[at,count]:{Read{1,1},{62,2},{65,193},{4000,130},{8190,9},{16384,4096}})
        pair.write(at,bytes(count,at));
    pair.compare_state();pair.read(0,22000);
    require(pair.original_source.calls==std::vector<Read>{{0,1},{2,60},{64,1},{258,3742},
        {4130,4060},{8199,4089},{12288,4096},{20480,1520}},"expected page-clipped source read sequence changed");
    // Failure occurs after preceding private bytes and half of the failing run.
    for(unsigned fail=1;fail<=8;++fail) {
        pair.original_source.fail_call=pair.candidate_source.fail_call=fail;
        pair.read(0,22000);pair.compare_state();
    }
    pair.original_source.fail_call=pair.candidate_source.fail_call=0;
    for(std::uint32_t at=0;at<22000;++at)
        require(pair.original.owns(at)==pair.candidate.owns(at),"byte ownership differs");
    pair.relinquish(62);pair.relinquish(8192);pair.relinquish(1);pair.relinquish(700000);
    pair.compare_state();pair.read(0,22000);
    auto saved=pair.candidate.ranges();
    Candidate restored(reference);restored.borrow(&pair.candidate_source);
    for(const auto &range:saved)restored.write(range.address,range.bytes);
    require(same_ranges(saved,restored.ranges()),"checkpoint restore lost a private byte");
    if(!saved.empty())saved[0].bytes[0]^=std::byte{255};
    require(same_ranges(pair.original.ranges(),restored.ranges()),"checkpoint output aliases private storage");
    auto copy=restored;copy.relinquish(63);
    require(restored.owns(63)&&!copy.owns(63),"copied context aliases ownership");
    copy.clear();require(copy.ranges().empty(),"clear retained private bytes");
    std::array<std::byte,4> output{};copy.read(0,output);
    restored.borrow(nullptr);
    require(error([&]{restored.read(65,output);})=="Native FP context has no borrowed input owner",
        "private read no longer requires a borrowed owner");
    require(!restored.try_read_view(65,4)&&!restored.direct_page(65,4),"private pages became borrowed aliases");
    pair.candidate.clear();pair.original.clear();pair.compare_state();pair.read(0,22000);
}
void extent_and_owner(bool reference) {
    Pair pair(reference);auto data=bytes(8);
    pair.write(0xfffffff8u,data);pair.read(0xfffffff8u,8);pair.read(0xffffffffu,1);
    pair.read(0xffffffffu,0);pair.read(0xffffffffu,2);
    const auto a=error([&]{pair.original.write(0xfffffffcu,data);});
    const auto b=error([&]{pair.candidate.write(0xfffffffcu,data);});
    require(!a.empty()&&a==b,"write wrap was admitted");pair.compare_state();
    pair.write(4095,bytes(2));pair.compare_state();
    pair.relinquish(4095);pair.relinquish(4096);pair.compare_state();
    pair.original.borrow(nullptr);pair.candidate.borrow(nullptr);
    pair.read(0,0);pair.read(0xfffffff8u,4);pair.read(0xffffffffu,2);
    pair.pin(123,4);pair.pin(0,4);pair.pin(123,0);
    // Owner-free writes remain private; the next borrowed read sees them.
    pair.write(65535,data);Source other;other.seed=99;
    pair.original.borrow(&other);pair.candidate.borrow(&other);pair.compare_state();
    std::array<std::byte,8> a_read,b_read;pair.original.read(65535,a_read);pair.candidate.read(65535,b_read);
    require(a_read==b_read&&std::equal(a_read.begin(),a_read.end(),data.begin()),"reborrow changed private bytes");
}
void pin_and_restore(bool reference) {
    Pair pair(reference);pair.pin(0x1fffu,12345);pair.pin(0x1fffu,12345);
    require(pair.candidate_source.calls.empty(),"fully private repin called its source");
    pair.pin(0x1fffu,12345,true);
    for(const auto at:{0x2000u,0x203fu,0x2040u,0x4000u})pair.relinquish(at);
    pair.pin(0x1fffu,12345);
    require(pair.original_source.calls==std::vector<Read>{{0x2000,1},{0x203f,2},{0x4000,1}},
        "partial pin read boundaries changed");
    pair.relinquish(0x3001);pair.relinquish(0x3005);
    const auto before=pair.candidate.ranges();
    pair.original_source.fail_call=pair.candidate_source.fail_call=2;pair.pin(0x1fffu,12345);
    require(same_ranges(before,pair.candidate.ranges()),"failed pin partially published bytes");
    pair.original_source.fail_call=pair.candidate_source.fail_call=0;
    pair.pin(0xfffffffcu,8);
}
void randomized(bool reference) {
    Pair pair(reference);std::uint32_t random=0x51ca729d;
    auto next=[&]{random^=random<<13;random^=random>>17;random^=random<<5;return random;};
    for(unsigned n=0;n<2200;++n) {
        const auto at=0x10000000u+next()%24576u;
        const auto count=next()%1700u;
        switch(next()%5) {
        case 0:case 1:pair.write(at,bytes(count,next()));break;
        case 2:for(unsigned k=0;k<count%31;++k)pair.relinquish(at+k);break;
        case 3:pair.pin(at,count);break;
        default:pair.read(at,count);break;
        }
        if(n%31==0) {pair.compare_state();pair.read(0x10000000u,27000);}
    }
    pair.compare_state();pair.read(0x10000000u,27000);
}
void default_control(bool native) {
    Source source;Candidate memory;memory.borrow(&source);
    const auto data=bytes(12345);memory.write(0x1fffu,data);
    allocation_probe::count=0;allocation_probe::active=true;
    memory.pin_range(0x1fffu,unsigned(data.size()));
    allocation_probe::active=false;
    require(source.calls.empty(),"controlled private pin read its source");
    require((allocation_probe::count==0)==native,"runtime control selected the wrong pin path");
    require(same_ranges(memory.ranges(),{{0x1fffu,data}}),"controlled private pin changed bytes");
}
struct QuietSource final:SourcePhysicsContactMemory {
    mutable unsigned reads{};
    void read(std::uint32_t at,std::span<std::byte> output)const override {
        ++reads;for(std::size_t n=0;n<output.size();++n)output[n]=Source::value(at+std::uint32_t(n),17);
    }
    void write(std::uint32_t,std::span<const std::byte>)override {throw std::runtime_error("source write");}
};
double cpu_seconds() {
#ifdef CLOCK_THREAD_CPUTIME_ID
    timespec t{};require(clock_gettime(CLOCK_THREAD_CPUTIME_ID,&t)==0,"thread CPU clock failed");
    return double(t.tv_sec)+double(t.tv_nsec)*1e-9;
#else
    return double(std::clock())/CLOCKS_PER_SEC;
#endif
}
template<class Memory> double sampling_loop(std::uint64_t &sum) {
    QuietSource source;std::array<Memory,4> memory;
    const auto view=bytes(0x20cc),locals=bytes(0x2000),control=bytes(0x300),saved=bytes(0x800),feedback=bytes(0x34);
    for(auto &m:memory){m.borrow(&source);m.write(0x10000000,view);m.write(0x10010000,locals);}
    const auto start=cpu_seconds();
    for(unsigned tick=0;tick<1200;++tick)for(auto &m:memory) {
        for(unsigned installation=0;installation<2;++installation) {
            m.write(0x1000000c,control);m.write(0x1000101c,control);
            m.write(0x10010800,saved);m.write(0x10011800,saved);m.write(0x1000202c,feedback);
            m.put(0x10002094,tick);
        }
        for(unsigned field=0;field<64;++field) {
            std::array<std::byte,4> output;
            m.put(0x10000020+field*4,tick+field);m.read(0x10000020+field*4,output);
            sum+=std::to_integer<unsigned>(output[0]);
        }
    }
    return cpu_seconds()-start;
}
template<class Memory> double pin_loop(std::uint64_t &sum) {
    QuietSource source;Memory memory;memory.borrow(&source);
    constexpr std::array<unsigned,7> sizes{0x200*0x3c,0x100*0x190,0x200*0x10,0x80*0xc,0x15e*8,0x30fc,0x200*0x18};
    for(unsigned n=0;n<sizes.size();++n)memory.write(0x20000100+n*0x40000,bytes(sizes[n]));
    const auto start=cpu_seconds();
    for(unsigned iteration=0;iteration<40;++iteration)for(unsigned n=0;n<sizes.size();++n) {
        const auto at=0x20000100+n*0x40000;
        if constexpr(requires{memory.pin_range(at,sizes[n]);})memory.pin_range(at,sizes[n]);
        else {std::vector<std::byte> copy(sizes[n]);memory.read(at,copy);memory.write(at,copy);}
    }
    sum+=memory.ranges().size();return cpu_seconds()-start;
}
void benchmark() {
    // Actual source FP installation/cache-owner sizes, controlled addresses and
    // bytes. Component CPU only; this deliberately does not claim live FPS.
#ifdef _WIN32
    _putenv_s("HALO2_NATIVE_FP_CONTEXT_MEMORY","1");_putenv_s("HALO2_REFERENCE_FP_CONTEXT_MEMORY","");
#else
    setenv("HALO2_NATIVE_FP_CONTEXT_MEMORY","1",1);unsetenv("HALO2_REFERENCE_FP_CONTEXT_MEMORY");
#endif
    std::uint64_t sum{};std::cout<<"{\"sampling_pairs\":[";
    for(unsigned pair=0;pair<3;++pair) {
        const auto a=sampling_loop<Reference>(sum),b=sampling_loop<Candidate>(sum);
        if(pair)std::cout<<',';std::cout<<'['<<a<<','<<b<<']';
    }
    const auto a=pin_loop<Reference>(sum),b=pin_loop<Candidate>(sum);
    std::cout<<"],\"private_pin_seconds\":["<<a<<','<<b<<"],\"checksum\":"<<sum<<"}\n";
}
}
int main(int argc,char **argv) {
    try {
        if(argc==2&&std::string_view(argv[1])=="--benchmark"){benchmark();return 0;}
        for(const bool reference:{false,true}) {
            seams(reference);extent_and_owner(reference);pin_and_restore(reference);randomized(reference);
        }
        if(argc==2) {
            const auto mode=std::string_view(argv[1]);
            require(mode=="--expect-native"||mode=="--expect-reference","unknown control expectation");
            default_control(mode=="--expect-native");
        }
        std::cout<<"FP context private bytes, source read/fault sequence, pin and restore agree with literal original\n";
    } catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
