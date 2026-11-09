#include "halo2/native_tu15_contact_candidate_private_data.hpp"
#include "halo2/native_tu15_contact_features.hpp"
#include "helpers/native_simplex_initial_count_input_capture_tu15.hpp"
#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <thread>
#include <xmmintrin.h>
using namespace halo2;
using J=nlohmann::json;
namespace {
void check(bool v,const char*why){if(!v)throw std::runtime_error(why);}
constexpr std::uint32_t first=0x01000000u,second=first+0x100u,table=first+0x200u,
    cache=first+0x280u,p=0x600e0000u,relative=p+0x300u,work=p+0x1000u,plane=p+0x1400u;
class Banks final:public SourcePhysicsContactMemory{
public:
    std::vector<std::byte> retained=std::vector<std::byte>(0x300),backing=std::vector<std::byte>(0x8000,std::byte{0xa5});
    std::array<std::byte,4> constant{};unsigned writes{};
    bool overlap{},missing_shape{},reverse_order{};
    unsigned mutable visits{};
    void read(std::uint32_t a,std::span<std::byte>b)const override{
        const auto copy=[&](const auto&src,std::uint32_t base){
            if(a<base||std::uint64_t(a-base)+b.size()>src.size())return false;
            std::copy_n(src.begin()+(a-base),b.size(),b.begin());return true;};
        check(copy(retained,first)||copy(backing,p)||copy(constant,0x41d658u),"unbacked fixture read");
    }
    void write(std::uint32_t,std::span<const std::byte>)override{++writes;throw std::runtime_error("retained fixture mutated");}
    template<class F>void visit_committed(F&&f)const{
        ++visits;std::vector<std::byte> ephemeral;
        const auto copy=[&](auto address,const auto&storage){
            ephemeral.assign(storage.begin(),storage.end());f(address,std::span<const std::byte>(ephemeral));std::fill(ephemeral.begin(),ephemeral.end(),std::byte{0x5a});};
        if(!missing_shape)copy(first,retained);
        if(overlap)copy(first+1u,retained);
        if(reverse_order&&visits%2u==0){copy(p,backing);copy(0x41d658u,constant);}
        else{copy(0x41d658u,constant);copy(p,backing);}
    }
    void put(std::uint32_t a,std::uint32_t word){
        check(a>=first&&std::uint64_t(a-first)+4<=retained.size(),"bad fixture store");
        for(unsigned i=0;i<4;++i)retained[a-first+i]=std::byte(word>>(i*8u));
    }
};
void word(SourcePhysicsContactMemory&m,std::uint32_t a,std::uint32_t x){
    std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(x>>(8*i));m.write(a,b);
}
std::uint16_t sw(){std::uint16_t x;asm volatile("fnstsw %0":"=a"(x));return x;}
struct Fixture{
    Banks root;
    const std::array<NativeTu15ContactPrivateRange,1>owned{{{p,0x8000}}};
    NativeTu15ContactCandidatePrivateData sidecar{root,owned,{}};
    NativeTu15ConvexSimplexMemory view{sidecar};
    NativeTu15ConvexSimplexSolverInput input{work,{first},{second},relative,cache,plane};
    NativeTu15ConvexSimplexSolverScratch scratch{p+0x1500u,p+0x1504u,p+0x1508u,p+0x1510u,p+0x1520u,{}};
    NativeTu15PhysicsFrameFp fp{0x27fu,0u,0x1f80u};
    NativeTu15ConvexSimplexSolverStats stats;
    Fixture(){
        root.put(first,table);root.put(second,table);root.put(table+0x34,0x1f6690u);
        root.put(second+0x20,0x3f800000u);root.put(cache+8,0x04040201u);
        for(unsigned i=0;i<16u;++i)word(view,relative+i*4u,(i==0||i==5||i==10||i==15)?0x3f800000u:0u);
        const std::array flag{std::byte{0}};view.write(work+0xcu,flag);
    }
    bool invoke(const SourcePhysicsContactMemory*owner,unsigned depth=0){
        NativeTu15ConvexSimplexSolverServices services;
        services.features=[](auto&view,auto shape,auto owner,auto ids,auto count,auto output,auto&current){
            native_tu15_contact_shape_features(view,shape,owner,ids,count,output,current,0);};
        services.initial_count_capture_owner=owner;
        try{native_tu15_solve_convex_contact_simplex(view,input,scratch,fp,depth,services,{},&stats);}
        catch(const std::runtime_error&e){return std::string(e.what())=="Native simplex solver requires its wider initial simplex";}
        return false;
    }
};
template<class T>bool fails(T f){try{f();}catch(const std::runtime_error&){return true;}return false;}
J read(const std::filesystem::path&p){std::ifstream f(p);return J::parse(f);}
}
int main(){try{
    const auto owner=std::filesystem::path(std::getenv("HALO2_TEST_SCRATCH"));
    const auto cap=owner/"good";
    Fixture actual;
    const auto initial_fp=actual.fp;
    const auto host_sw=sw();const auto host_mx=_mm_getcsr();
    test::NativeSimplexInitialCountInputCaptureTu15 capture(cap);
    unsigned calls{};NativeTu15SimplexInitialCountInput seen;
    NativeTu15SimplexInitialCountInputObserverBinding binding(actual.root,[&](const auto&owner,const auto&view,const auto&input){
        check(&owner==&actual.root&&&view==&actual.view,"wrong current owner/view");
        check(input.entry_fp.status==initial_fp.status&&input.first_count==1u&&input.second_count==2u,"entry/guard counts/FP not actual");
        check(!view.data_is_defined(plane,16u)&&!view.data_is_defined(work+0xcu+1,3u),"undefined plane/padding supplied");
        ++calls;seen=input;capture.observe(input,actual.root,view);
        _mm_setcsr((_mm_getcsr()^0x6000u)|1u);
        asm volatile("fld1;fldz;fdivp;fstp %%st(0)":::"memory","st","st(1)");
    });
    check(actual.invoke(&actual.root)&&actual.stats.initial_count_refusals==1&&actual.stats.feature_queries==0&&actual.stats.completed==0&&calls==1,"actual selected refusal changed");
    check(sw()==host_sw&&_mm_getcsr()==host_mx,"capture leaked hardware FP");
    check(seen.entry_fp.status==actual.fp.status&&seen.entry_fp.mxcsr==actual.fp.mxcsr,"current guard FP differs");
    const auto info=read(cap/"capture-native-before.json");
    check(info.at("owner")==0x30e930&&info.at("source_entry")==0x30e930&&info.at("preserved_x87_depth")==0,"initial_count manifest boundary");
    check(info.at("regions").size()==3&&info.at("guarded_data_fragments").size()==3&&info.at("output_plane_bytes_provided")==false,"fragment coverage");
    std::ifstream captured_cache(cap/"retained_support_cache.bin",std::ios::binary);std::array<unsigned char,12>data{};
    captured_cache.read(reinterpret_cast<char*>(data.data()),12);
    check(data[8]==1u&&data[9]==2u&&data[10]==4u&&data[11]==4u,"guarded cache did not reflect actual typed counts");
    std::ifstream b(cap/("before-"+std::to_string(p)+".bin"),std::ios::binary);const std::vector<char>raw((std::istreambuf_iterator<char>(b)),{});
    check(raw.size()==0x8000&&std::all_of(raw.begin(),raw.end(),[](char x){return static_cast<unsigned char>(x)==0xa5;}),"retained backing changed or borrowed bank span escaped");
    check(native_tu15_observe_simplex_initial_count_input(actual.root,actual.view,seen)&&actual.root.visits==2,"second capture did I/O");
    check(actual.root.writes==0&&actual.root.backing[0x1120]==std::byte{0xa5},"underlying retained bank modified");
    Fixture disabled;check(disabled.invoke(nullptr)&&calls==2,"disabled owner changed named refusal");
    check(!native_tu15_observe_simplex_initial_count_input(disabled.root,actual.view,seen),"foreign owner observer fired");
    bool foreign_thread=true;std::thread t([&]{foreign_thread=native_tu15_observe_simplex_initial_count_input(actual.root,actual.view,seen);});t.join();check(!foreign_thread,"foreign executor observer fired");
    {NativeTu15SimplexInitialCountInputObserverBinding nested(actual.root,[](const auto&,const auto&,const auto&){_mm_setcsr(0x7f80u);throw std::runtime_error("observer fault");});
      check(!native_tu15_observe_simplex_initial_count_input(actual.root,actual.view,seen)&&sw()==host_sw&&_mm_getcsr()==host_mx,"observer throw leaked FP");
      Fixture same;check(same.invoke(nullptr)&&same.fp.status==actual.fp.status,"initial_count FP changed by observer");}
    Fixture unselected;unselected.root.put(second+0x20,0u);unselected.root.put(cache+8,0x04040101u);check(!unselected.invoke(&actual.root)&&unselected.stats.initial_count_refusals==0&&calls==2,"non-initial_count fired capture");
    Fixture wrongdepth;check(!wrongdepth.invoke(&actual.root,1u)&&calls==2,"depth rejection changed");
    check(fails([&]{test::NativeSimplexInitialCountInputCaptureTu15 bad(owner);}),"accepted owner root");
    check(fails([&]{test::NativeSimplexInitialCountInputCaptureTu15 bad(owner.parent_path()/"elsewhere");}),"escaped owner");
    auto bad=seen;bad.second_count=1u;test::NativeSimplexInitialCountInputCaptureTu15 invalid(owner/"invalid");
    check(fails([&]{invalid.observe(bad,actual.root,actual.view);})&&!std::filesystem::exists(owner/"invalid"),"accepted unselected boundary");
    bad=seen;bad.solver.relative_transform=plane;check(fails([&]{invalid.observe(bad,actual.root,actual.view);})&&!std::filesystem::exists(owner/"invalid"),"read undefined DATA");
    actual.root.missing_shape=true;check(fails([&]{invalid.observe(seen,actual.root,actual.view);})&&!std::filesystem::exists(owner/"invalid"),"missing retained shape admitted");actual.root.missing_shape=false;
    actual.root.overlap=true;check(fails([&]{invalid.observe(seen,actual.root,actual.view);})&&!std::filesystem::exists(owner/"invalid"),"overlapping banks admitted");actual.root.overlap=false;
    check(!actual.root.writes,"diagnostic wrote retained memory");
    std::cout<<"{\"passed\":true,\"controls\":15,\"actual_native_guard_before_feature_calls\":true,\"ephemeral_bank_proof\":true,\"guarded_fragments\":3,\"underlying_32KiB_unchanged\":true}\n";
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
