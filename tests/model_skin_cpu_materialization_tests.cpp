#include "rendering/models/model_skin_cpu_materialization.hpp"
#include "rendering/models/model_skin_gpu.hpp" // Actual record ABI; no Vulkan calls.
#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <new>

namespace {std::atomic<bool> forbid_allocation{};}
void *operator new(std::size_t size) {
    if(forbid_allocation.load(std::memory_order_relaxed))throw std::bad_alloc();
    if(auto *result=std::malloc(size?size:1))return result;
    throw std::bad_alloc();
}
void *operator new[](std::size_t size) {return ::operator new(size);}
void operator delete(void *p)noexcept {std::free(p);}
void operator delete[](void *p)noexcept {std::free(p);}
void operator delete(void *p,std::size_t)noexcept {std::free(p);}
void operator delete[](void *p,std::size_t)noexcept {std::free(p);}

using namespace halo2;
namespace {
void need(bool condition,const char *message) {if(!condition)throw std::runtime_error(message);}
struct Fp {
    std::fenv_t environment{};
    unsigned mxcsr{};
    std::uint16_t control{},status{};
    int error{};
    static Fp get() {
        Fp result;result.error=errno;
        need(std::fegetenv(&result.environment)==0,"Capture full test FP environment");
#if defined(__x86_64__) && !defined(_MSC_VER)
        result.mxcsr=_mm_getcsr();
        asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(result.control),"=m"(result.status)::"memory");
#endif
        return result;
    }
    void restore()const {
        need(std::fesetenv(&environment)==0,"Restore full test FP environment");errno=error;
    }
    bool same(const Fp &other)const {
        return mxcsr==other.mxcsr && control==other.control && status==other.status && error==other.error &&
            !std::memcmp(&environment,&other.environment,sizeof(environment));
    }
};
struct Restore {Fp saved=Fp::get();~Restore(){saved.restore();}};
struct NoAllocation {
    NoAllocation(){forbid_allocation.store(true,std::memory_order_relaxed);}
    ~NoAllocation(){forbid_allocation.store(false,std::memory_order_relaxed);}
};
struct Fixture {
    std::vector<Vertex> vertices=std::vector<Vertex>(23);
    std::vector<ModelSkinGpuRecord> records=std::vector<ModelSkinGpuRecord>(8);
    std::vector<std::array<float,12>> palette=std::vector<std::array<float,12>>(8);
    Fixture() {
        auto bytes=std::as_writable_bytes(std::span{vertices});
        for(std::size_t i=0;i<bytes.size();++i)bytes[i]=std::byte((i*37+11)%256);
        for(unsigned i=0;i<palette.size();++i)
            palette[i]={1.f+float(i)/8.f,-.25f,.125f,float(i)/2.f,
                .25f,1,-.5f,-float(i)/4.f,-.125f,.25f,1,float(i)/8.f};
        for(unsigned i=0;i<records.size();++i) {
            auto &record=records[i];record.destination=2+i*2;record.palette=i%2*4;record.palette_count=4;
            record.post_identity_placement=i%3!=1;record.bones={0,1,2,3};
            record.weights=i%2?std::array{.5f,-.25f,-0.f,.75f}:std::array{1.f,0.f,0.f,0.f};
            auto &vertex=vertices[record.destination];vertex.position={float(i)+.25f,-2,3};
            vertex.normal={-.25f,.5f,-1};vertex.tangent={1,-.5f,0};vertex.binormal={0,1,-0.f};
        }
        records[0].bones.fill(0xffffffffu);records[0].weights={0,-0.f,0,-0.f};
        auto &zero=vertices[records[0].destination];zero.position={-0.f,-1,-1};zero.normal={-0.f,1,-1};
        zero.tangent={-0.f,-1,-1};zero.binormal={0,-0.f,-0.f};
    }
};
void same_vertices(std::span<const Vertex> a,std::span<const Vertex> b,const char *message) {
    need(a.size()==b.size() && (a.empty() || !std::memcmp(a.data(),b.data(),a.size_bytes())),message);
}
// Independent literal original SSE kernel and append graph. Never call the
// new materializer's binding/placement helpers to construct its expected data.
std::vector<Vertex> original(const Fixture &fixture) {
    Restore saved;std::fenv_t held{};
    need(std::feholdexcept(&held)==0 && std::fesetround(FE_TONEAREST)==0,"Prepare original reference FP owner");
    auto result=fixture.vertices;
    for(const auto &record:fixture.records) {
        SkinBinding binding;
        for(unsigned j=0;j<4;++j)binding.bones[j]=std::uint16_t(record.bones[j]);
        binding.weights=record.weights;
        auto &vertex=result[record.destination];
        skin_model_vertices({&vertex,1},{&binding,1},std::span{fixture.palette}.subspan(record.palette,record.palette_count),true);
        if(record.post_identity_placement) {
            const Transform identity{};
            const auto rotate=[&](Vec3 v) {
                return identity.basis[0]*v.x+identity.basis[1]*v.y+identity.basis[2]*v.z;
            };
            vertex.position=identity.position+rotate(vertex.position)*identity.scale;
            vertex.normal=rotate(vertex.normal);vertex.tangent=rotate(vertex.tangent);vertex.binormal=rotate(vertex.binormal);
        }
    }
    return result;
}
void materialize(Fixture &fixture) {
    model_skin_cpu_materialize(std::span{fixture.vertices},std::span{fixture.records},fixture.palette);
}
void successful_jobs() {
    Restore saved;Fixture fixture;const auto sources=fixture;
    const auto expected=original(fixture);
    for(unsigned order=0;order<3;++order) {
        fixture=sources;
        if(order==1)std::reverse(fixture.records.begin(),fixture.records.end());
        if(order==2)std::rotate(fixture.records.begin(),fixture.records.begin()+3,fixture.records.end());
        const auto records=fixture.records;const auto palette=fixture.palette;const auto before=Fp::get();
        {NoAllocation no_heap;materialize(fixture);}
        need(before.same(Fp::get()),"Successful materialization changed caller full FP/errno environment");
        same_vertices(fixture.vertices,expected,"Sorted/reversed/permuted materialization changed complete original vertex bytes");
        need(!std::memcmp(records.data(),fixture.records.data(),records.size()*sizeof(ModelSkinGpuRecord)) &&
             !std::memcmp(palette.data(),fixture.palette.data(),palette.size()*sizeof(palette.front())),
             "Materialization changed immutable records or palette inputs");
    }
    need(std::bit_cast<std::uint32_t>(expected[sources.records[0].destination].normal.x)==0u &&
         std::bit_cast<std::uint32_t>(expected[sources.records[0].destination].tangent.x)==0x80000000u,
         "Zero-weight identity graph did not preserve its distinct zero-sign obligations");
    fixture=sources;fixture.records[0].post_identity_placement=0;const auto untouched=fixture.vertices[fixture.records[0].destination];
    materialize(fixture);
    need(!std::memcmp(&untouched,&fixture.vertices[fixture.records[0].destination],sizeof(Vertex)),
         "Zero-weight mode0 vertex was modified");
    fixture=sources;fixture.records[0].palette=unsigned(fixture.palette.size());fixture.records[0].palette_count=0;
    materialize(fixture);same_vertices(fixture.vertices,expected,"Zero-weight empty owner palette was read");
    Fixture empty;empty.records.clear();const auto original_bytes=empty.vertices;materialize(empty);
    same_vertices(empty.vertices,original_bytes,"Empty record job changed mapped sentinels");
    model_skin_cpu_materialize(std::span<Vertex>{},std::span<const ModelSkinGpuRecord>{},std::span<const std::array<float,12>>{});
}
template<class Change> void rejected(Change change,const char *message) {
    Fixture fixture;change(fixture);const auto original_bytes=fixture.vertices;
    const auto records=fixture.records;const auto palette=fixture.palette;const auto before=Fp::get();
    bool failed=false;try{materialize(fixture);}catch(const FormatError&){failed=true;}
    need(failed,message);need(before.same(Fp::get()),"Rejected job changed caller FP/errno owner");
    same_vertices(fixture.vertices,original_bytes,"Validation failure partially wrote mapped vertices");
    need(!std::memcmp(records.data(),fixture.records.data(),records.size()*sizeof(ModelSkinGpuRecord)) &&
         !std::memcmp(palette.data(),fixture.palette.data(),palette.size()*sizeof(palette.front())),
         "Rejected job changed immutable source records/palette");
}
void rejected_jobs() {
    rejected([](auto &f){f.records.back().destination=unsigned(f.vertices.size());},"Escaping last destination was accepted");
    rejected([](auto &f){f.records.back().destination=f.records.front().destination;},"Nonadjacent destination alias was accepted");
    rejected([](auto &f){f.records.back().destination=f.records[f.records.size()-2].destination;},"Adjacent destination alias was accepted");
    rejected([](auto &f){f.records.back().post_identity_placement=2;},"Unknown final post mode was accepted");
    rejected([](auto &f){f.records.back().palette=unsigned(f.palette.size()+1);},"Escaping palette base was accepted");
    rejected([](auto &f){f.records.back().palette_count=0xffffffffu;},"Escaping owner extent was accepted");
    rejected([](auto &f){f.records.back().bones[0]=4;},"Bone spilled into another owner's adjacent palette");
    rejected([](auto &f){f.records.back().bones[2]=4;},"Interior zero-weight bone escaped its owner");
    rejected([](auto &f){f.records.back().bones[3]=65536;},"Active bone wider than original uint16 was accepted");
    rejected([](auto &f){f.records.back().weights[0]=std::bit_cast<float>(1u);},"Subnormal weight was accepted");
    rejected([](auto &f){f.records.back().weights[0]=std::bit_cast<float>(0x7f800001u);},"Signaling weight NaN was accepted");
    rejected([](auto &f){f.vertices[f.records.back().destination].position.x=std::bit_cast<float>(1u);},"Subnormal destination local was accepted");
    rejected([](auto &f){f.vertices[f.records.back().destination].normal.x=std::bit_cast<float>(0x49800001u);},"Oversized local was accepted");
    rejected([](auto &f){f.palette[6][0]=std::bit_cast<float>(1u);},"Referenced interior zero-influence denormal row was accepted");
    rejected([](auto &f){f.palette.push_back({});f.palette.back()[0]=std::bit_cast<float>(0x7f800001u);},"Unused signaling palette row was accepted");
    rejected([](auto &f){f.palette.push_back({});f.palette.back()[0]=std::bit_cast<float>(0x49800001u);},"Unused oversized palette row was accepted");
    Fixture legal;legal.palette.push_back({});legal.palette.back()[0]=std::bit_cast<float>(1u);
    const auto expected=original(legal);materialize(legal);
    same_vertices(legal.vertices,expected,"Unused finite subnormal palette data was incorrectly rejected");
    Fixture wide;wide.palette.resize(65537);wide.records.back().palette=0;
    wide.records.back().palette_count=65537;wide.records.back().bones[0]=65536;
    const auto bytes=wide.vertices;bool failed=false;
    try{materialize(wide);}catch(const FormatError&){failed=true;}
    need(failed,"Wide bone was truncated to uint16 even though its declared palette contains it");
    same_vertices(wide.vertices,bytes,"Wide-bone rejection partially wrote the mapped bank");
    Fixture aliases;const auto original_bytes=aliases.vertices;const auto before=Fp::get();
    // Deliberately malformed aliased views: the validator must reject their
    // addresses without reading any record/row through the invalid view.
    failed=false;try {
        auto rows=std::span<const std::array<float,12>>{reinterpret_cast<const std::array<float,12>*>(aliases.vertices.data()),1};
        model_skin_cpu_materialize(std::span{aliases.vertices},std::span{aliases.records},rows);
    }catch(const FormatError&){failed=true;}
    need(failed && before.same(Fp::get()),"Aliased palette view was read or changed the FP owner");
    failed=false;try {
        auto records=std::span<const ModelSkinGpuRecord>{reinterpret_cast<const ModelSkinGpuRecord*>(aliases.vertices.data()),1};
        model_skin_cpu_materialize(std::span{aliases.vertices},records,aliases.palette);
    }catch(const FormatError&){failed=true;}
    need(failed && before.same(Fp::get()),"Aliased record view was read or changed the FP owner");
    same_vertices(aliases.vertices,original_bytes,"Input alias rejection mutated its mapped/source storage");
}
void hostile_address_ranges() {
    using namespace model_skin_cpu_materialization_detail;
    // Exercise the exact structural guard with integer addresses rather than
    // constructing std::span ranges that violate their C++ object preconditions.
    const auto reject=[]<class T>(std::uintptr_t first,std::size_t count) {
        bool failed=false;try{(void)address<T>(first,count);}catch(const FormatError&){failed=true;}
        need(failed,"Null, misaligned or wrapping nonempty address range was accepted");
    };
    const auto checks=[&]<class T>() {
        constexpr auto alignment=std::uintptr_t(alignof(T));
        const auto empty=address<T>(0,0);need(!empty.first && !empty.end,"Empty address range did not remain uninterpreted");
        const auto valid=address<T>(alignment,1);
        need(valid.first==alignment && valid.end==alignment+sizeof(T),"Valid structural address range changed its byte extent");
        reject.template operator()<T>(0,1);
        reject.template operator()<T>(alignment+1,1);
        reject.template operator()<T>(std::numeric_limits<std::uintptr_t>::max()&~(alignment-1),1);
        reject.template operator()<T>(alignment,std::numeric_limits<std::size_t>::max()/sizeof(T)+1);
    };
    checks.template operator()<Vertex>();checks.template operator()<ModelSkinGpuRecord>();
    checks.template operator()<std::array<float,12>>();
}
void floating_environments() {
#if defined(__x86_64__) && !defined(_MSC_VER)
    Restore saved;Fixture fixture;const auto input=fixture.vertices;const auto expected=original(fixture);
    unsigned cases{};
    for(unsigned x87_round=0;x87_round<4;++x87_round)for(unsigned sse_round=0;sse_round<4;++sse_round)
        for(unsigned denorm=0;denorm<4;++denorm)for(unsigned sticky=0;sticky<64;++sticky) {
            fixture.vertices=input;
            std::uint16_t control{};asm volatile("fnstcw %0":"=m"(control));
            control=std::uint16_t((control&~0xc00u)|0x3fu|(x87_round<<10));
            asm volatile("fldcw %0"::"m"(control):"memory");
            const auto csr=0x1f80u|(sse_round<<13)|((denorm&1)<<6)|((denorm>>1)<<15)|sticky;
            _mm_setcsr(csr);errno=cases%2?EDOM:ERANGE;const auto before=Fp::get();
            {NoAllocation no_heap;materialize(fixture);}
            const auto after=Fp::get();_mm_setcsr(0x1f80u);
            need(before.same(after),"Materialization failed to restore full SSE/x87 rounding/sticky/errno state");
            same_vertices(fixture.vertices,expected,"Caller FP controls changed forced-RTE materialization bytes");++cases;
        }
    need(cases==4096,"Incomplete all-sticky/DAZ/FTZ/SSE/x87 rounding matrix");
    for(unsigned mask=0;mask<6;++mask) {
        fixture.vertices=input;_mm_setcsr((0x1f80u&~(1u<<(7+mask)))|0x3fu);
        errno=EDOM;const auto before=Fp::get();{NoAllocation no_heap;materialize(fixture);}
        const auto after=Fp::get();_mm_setcsr(0x1f80u);
        need(before.same(after),"Materialization failed to restore unmasked SSE exception state");
        same_vertices(fixture.vertices,expected,"Unmasked caller changed materialization result");
    }
    // Rejected signaling input must not run floating-point classification,
    // even with every SSE exception unmasked and every sticky bit retained.
    _mm_setcsr(0x3fu);errno=ERANGE;
    rejected([](auto &f){f.palette.back()[0]=std::bit_cast<float>(0x7f800001u);},"Unmasked signaling input was accepted");
    _mm_setcsr(0x1f80u);
#endif
}
void x87_owned_environments() {
#if defined(__x86_64__) && !defined(_MSC_VER)
    Restore saved;Fixture fixture;const auto input=fixture.vertices;const auto expected=original(fixture);
    const auto seed=[](unsigned precision,unsigned rounding,unsigned sticky,int unmasked=-1) {
        // FNSTENV's 28-byte legacy image has control/status at byte0/4 in
        // x86-64. Keep the original tags/pointers/condition codes intact and
        // modify only the declared controls and pending-exception bits.
        std::array<std::byte,28> image{};
        asm volatile("fnstenv %0":"=m"(image)::"memory");
        std::uint16_t control{},status{};std::memcpy(&control,image.data(),2);std::memcpy(&status,image.data()+4,2);
        control=std::uint16_t((control&~0xf00u)|0x3fu|(precision<<8)|(rounding<<10));
        if(unmasked>=0)control=std::uint16_t(control&~(1u<<unsigned(unmasked)));
        status=std::uint16_t((status&~0x80bfu)|sticky);
        if(sticky&~control&0x3fu)status=std::uint16_t(status|0x8080u);
        std::memcpy(image.data(),&control,2);std::memcpy(image.data()+4,&status,2);
        asm volatile("fldenv %0"::"m"(image):"memory");
    };
    unsigned cases{};
    for(unsigned precision:{0u,2u,3u})for(unsigned rounding=0;rounding<4;++rounding)for(unsigned sticky=0;sticky<64;++sticky) {
        fixture.vertices=input;_mm_setcsr(0x1f80u|(((rounding+1)%4)<<13)|sticky);
        seed(precision,rounding,sticky);errno=cases%2?EDOM:ERANGE;const auto before=Fp::get();
        {NoAllocation no_heap;materialize(fixture);}const auto after=Fp::get();saved.saved.restore();
        need(before.same(after),"Materialization changed x87 precision/rounding/sticky/full FP ownership");
        same_vertices(fixture.vertices,expected,"x87 precision/rounding/sticky state changed forced-RTE vertex bytes");++cases;
    }
    for(int unmasked=0;unmasked<6;++unmasked) {
        fixture.vertices=input;_mm_setcsr(0x1fa0u);seed(3,2,0x3f,unsigned(unmasked));errno=EDOM;
        const auto before=Fp::get();{NoAllocation no_heap;materialize(fixture);}const auto after=Fp::get();saved.saved.restore();
        need(before.same(after),"Materialization failed to restore pending unmasked x87 exception ownership");
        same_vertices(fixture.vertices,expected,"Pending unmasked x87 state changed forced-RTE vertex bytes");++cases;
    }
    need(cases==774,"Incomplete independent x87 sticky/precision/unmasked matrix");
#endif
}
}
int main()try {
    Restore saved;successful_jobs();rejected_jobs();hostile_address_ranges();floating_environments();x87_owned_environments();
    std::cout<<"PASS CPU model skin materialization: complete bytes, mixed post modes, full prevalidation, hostile structural ranges, immutable inputs, heap-free ordered/unordered jobs, 4096 SSE/rounding and774 independent x87 environments plus unmasked rejection\n";
    return 0;
}catch(const std::exception &error) {forbid_allocation.store(false);std::cerr<<error.what()<<'\n';return 1;}
