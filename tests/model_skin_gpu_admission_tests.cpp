#include "halo2/model_skin_gpu_admission.hpp"
#include "helpers/model_skin_gpu_admission_reference_20261023.hpp"
#ifdef HALO2_ADMISSION_MCC_POSES
#include "halo2/content.hpp"
#include "halo2/fp_registration.hpp"
#include "halo2/model_skin_submission.hpp"
#endif
#include <atomic>
#include <cerrno>
#include <cfenv>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <new>
#include <numeric>
#include <string_view>
#include <vector>

namespace {std::atomic<std::size_t> allocations{};}
void *operator new(std::size_t size) {
    allocations.fetch_add(1,std::memory_order_relaxed);
    if(auto *p=std::malloc(size?size:1))return p;
    throw std::bad_alloc();
}
void *operator new[](std::size_t size) {return ::operator new(size);}
void operator delete(void *p)noexcept {std::free(p);}
void operator delete[](void *p)noexcept {std::free(p);}
void operator delete(void *p,std::size_t)noexcept {std::free(p);}
void operator delete[](void *p,std::size_t)noexcept {std::free(p);}
using namespace halo2;
namespace reference=halo2::model_skin_gpu_admission_reference_20261023;
namespace {
constexpr const char *reference_sha="7a5539bda19f59738aa84d01a33afaed5414fe50fafb940ebdcc92194410fc0a";
std::size_t comparisons{},accepted{},rejected{};
void need(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
float bits(std::uint32_t value) {return std::bit_cast<float>(value);}
float power(int exponent,bool negative=false) {return bits(std::uint32_t(exponent+127)<<23|(negative?0x80000000u:0));}
struct Fp {
    std::fenv_t env{};unsigned csr{};std::uint16_t control{},status{};int error{};
    static Fp get() {
        Fp out;out.error=errno;std::fegetenv(&out.env);out.csr=_mm_getcsr();
        asm volatile("fnstcw %0\n\tfnstsw %1":"=m"(out.control),"=m"(out.status)::"memory");return out;
    }
    void restore()const {std::fesetenv(&env);_mm_setcsr(csr);errno=error;}
    bool same(const Fp &other)const {
        return csr==other.csr && control==other.control && status==other.status && error==other.error &&
            !std::memcmp(&env,&other.env,sizeof(env));
    }
};
struct Restore {Fp saved=Fp::get();~Restore(){saved.restore();}};
struct Fixture {
    std::vector<Vertex> vertices=std::vector<Vertex>(1);
    std::vector<SkinBinding> bindings=std::vector<SkinBinding>(1);
    std::vector<std::array<float,12>> palette;
    explicit Fixture(std::size_t rows=2):palette(rows) {
        vertices[0].position={.5f,-1,2};vertices[0].normal={-.25f,.5f,1};
        vertices[0].tangent={1,-.5f,0};vertices[0].binormal={0,1,-0.f};
        // Non-skin metadata may contain signaling encodings; it is data only.
        vertices[0].uv={bits(0x7f800001u),bits(1)};vertices[0].source_part=43;
        bindings[0].bones={0,1,0,1};bindings[0].weights={1,0,-0.f,0};
        for(auto &row:palette)row={1,.5f,-.25f,.125f,-.5f,1,.25f,-.125f,.25f,-.5f,1,.5f};
    }
};
bool compare(std::span<const Vertex> vertices,std::span<const SkinBinding> bindings,
             std::span<const std::array<float,12>> palette) {
    const auto count=allocations.load(std::memory_order_relaxed);const auto before=Fp::get();
    const bool old=reference::model_skin_gpu_arithmetic_admitted(vertices,bindings,palette);
    const auto middle=Fp::get();const bool now=model_skin_gpu_arithmetic_admitted(vertices,bindings,palette);
    const auto after=Fp::get();
    need(before.same(middle) && before.same(after),"Arithmetic predicate changed full SSE/x87/errno ownership");
    need(allocations.load(std::memory_order_relaxed)==count,"Arithmetic predicate allocated heap storage");
    need(now==old,"Row summary changed source-pinned arithmetic admission");
    const auto summary=model_skin_gpu_admission_static(vertices,bindings);
    need(model_skin_gpu_arithmetic_admitted(summary,palette)==old,"Static summary changed source-pinned arithmetic admission");
    ++comparisons;if(now)++accepted;else ++rejected;return now;
}
bool compare(const Fixture &fixture) {return compare(fixture.vertices,fixture.bindings,fixture.palette);}
float &attribute(Vertex &v,unsigned component) {
    Vec3 *attributes[]={&v.position,&v.normal,&v.tangent,&v.binormal};
    switch(component%3) {case 0:return attributes[component/3]->x;case 1:return attributes[component/3]->y;default:return attributes[component/3]->z;}
}
void boundaries() {
    constexpr std::uint32_t values[]={0,0x80000000u,1,0x007fffffu,0x00800000u,0x00800001u,
        0x0bffffffu,0x0c000000u,0x0c000001u,0x2b7fffffu,0x2b800000u,0x2b800001u,
        0x3f7fffffu,0x3f800000u,0x3f800001u,0x497fffffu,0x49800000u,0x49800001u,
        0x7f7fffffu,0x7f800000u,0x7f800001u,0x7fc12345u};
    for(std::size_t size:{1u,2u,17u,255u,256u,257u}) {
        Fixture fixture(size);const auto baseline=fixture;
        for(auto value:values)for(auto sign:{0u,0x80000000u}) {
            for(unsigned component=0;component<12;++component) {
                fixture=baseline;attribute(fixture.vertices[0],component)=bits(value|sign);compare(fixture);
            }
            for(unsigned lane=0;lane<4;++lane) {
                fixture=baseline;fixture.bindings[0].weights[lane]=bits(value|sign);compare(fixture);
            }
            for(unsigned component=0;component<12;++component) {
                // Active, unused and interior-zero referenced rows have distinct policies.
                fixture=baseline;fixture.palette[0][component]=bits(value|sign);compare(fixture);
                if(size>1) {
                    fixture=baseline;fixture.palette.back()[component]=bits(value|sign);compare(fixture);
                    fixture=baseline;fixture.bindings[0].weights={.5f,0,.5f,0};
                    fixture.bindings[0].bones[1]=std::uint16_t(size-1);
                    fixture.palette.back()[component]=bits(value|sign);compare(fixture);
                }
            }
        }
        fixture=baseline;fixture.palette.back()[0]=bits(1);
        if(size>1)need(compare(fixture),"Unused finite subnormal row stopped being legal");
        fixture=baseline;fixture.bindings[0].weights={.5f,-0.f,.5f,0};
        fixture.bindings[0].bones[1]=std::uint16_t(size-1);fixture.palette.back()[0]=bits(1);
        need(!compare(fixture),"Interior zero influence stopped validating its referenced row");
    }
    Fixture fixture;fixture.bindings.clear();need(!compare(fixture),"Binding/vertex length mismatch accepted");
    fixture.vertices.clear();need(compare(fixture),"Empty complete vertex bank rejected");
    fixture.palette[1][0]=bits(0x7f800001u);need(!compare(fixture),"Empty bank bypassed the full palette finite gate");
    Fixture empty(0);empty.bindings[0].weights={0,-0.f,0,-0.f};empty.bindings[0].bones.fill(65535);
    need(compare(empty),"All-zero weights started evaluating palette or bone indices");
    empty.vertices[0].position.x=bits(1);need(!compare(empty),"All-zero weights bypassed attribute validation");
    for(std::size_t size:{0u,1u,2u,255u,256u,257u,65535u,65536u}) {
        Fixture bank(size);
        for(unsigned lane=0;lane<4;++lane)for(unsigned bone:{0u,1u,254u,255u,256u,65534u,65535u}) {
            bank.bindings[0].weights={0,-0.f,0,-0.f};bank.bindings[0].weights[lane]=1;
            bank.bindings[0].bones.fill(0);bank.bindings[0].bones[lane]=std::uint16_t(bone);compare(bank);
            bank.bindings[0].weights[lane]=0;need(compare(bank),"All-zero weights evaluated a trailing uint16 bone");
        }
    }
}
void lattices() {
    // Every representable coefficient/weight exponent pair straddles all
    // blend lower bounds. Exercise DP3 and one-influence DPH translation
    // bounds separately at their exact lower neighbor, endpoint and upper neighbor.
    Fixture fixture;
    for(int p=-126;p<=20;++p)for(int w=-126;w<=0;++w)for(unsigned influences:{1u,2u,3u,4u}) {
        fixture.palette[0].fill(power(p));fixture.palette[1].fill(power(p,true));
        fixture.bindings[0].weights.fill(0);
        for(unsigned j=0;j<influences;++j)fixture.bindings[0].weights[j]=power(w,j%2);
        for(int delta:{-1,0,1}) {
            const int a=std::clamp(-103-p-w+(influences>1?23:0)+delta,-126,20);
            for(unsigned k=0;k<12;++k)attribute(fixture.vertices[0],k)=power(a,k%2);
            compare(fixture);
        }
    }
    for(std::size_t size:{2u,256u,257u})for(int t=-126;t<=20;++t)for(int w:{-126,-103,-80,-23,-1,0}) {
        Fixture bank(size);bank.palette[0].fill(1);bank.bindings[0].weights={power(w),0,0,0};
        for(unsigned k:{3u,7u,11u})bank.palette[0][k]=power(t,k==7);
        compare(bank);bank.vertices[0].position={0,-0.f,0};compare(bank);
    }
}
std::uint32_t random_word(std::uint32_t &state) {state^=state<<13;state^=state>>17;state^=state<<5;return state;}
void varied_banks() {
    std::uint32_t state=0x1376c0u;
    for(std::size_t size:{2u,8u,255u,256u,257u,300u}) {
        Fixture bank(size);bank.vertices.resize(19,bank.vertices[0]);bank.bindings.resize(19,bank.bindings[0]);
        for(unsigned trial=0;trial<128;++trial) {
            for(auto &row:bank.palette)for(auto &value:row) {
                const auto word=random_word(state);value=bits(((word%147u+1u)<<23)|(word&0x807fffffu));
            }
            for(unsigned i=0;i<bank.vertices.size();++i) {
                for(unsigned k=0;k<12;++k) {
                    const auto word=random_word(state);attribute(bank.vertices[i],k)=bits(((word%147u+1u)<<23)|(word&0x807fffffu));
                }
                for(unsigned j=0;j<4;++j) {
                    const auto word=random_word(state);bank.bindings[i].bones[j]=std::uint16_t(word%(size+1));
                    bank.bindings[i].weights[j]=word%3?bits(((word%127u+1u)<<23)|(word&0x807fffffu)):bits(word&0x80000000u);
                }
            }
            const auto vertices=bank.vertices;const auto bindings_bytes=std::as_bytes(std::span{bank.bindings});
            const std::vector<std::byte> bindings(bindings_bytes.begin(),bindings_bytes.end());const auto palette=bank.palette;
            compare(bank);
            need(!std::memcmp(vertices.data(),bank.vertices.data(),vertices.size()*sizeof(Vertex)) &&
                 !std::memcmp(bindings.data(),bank.bindings.data(),bindings.size()) &&
                 !std::memcmp(palette.data(),bank.palette.data(),palette.size()*sizeof(palette[0])),
                 "Arithmetic admission mutated its source vertex/binding/palette bank");
        }
    }
}
void environments() {
    Restore saved;Fixture accepted_fixture,rejected_fixture;rejected_fixture.palette[0][0]=bits(0x7f800001u);
    for(unsigned rounding=0;rounding<4;++rounding)for(unsigned denorm=0;denorm<4;++denorm)
        for(unsigned masks=0;masks<64;++masks)for(unsigned flags=0;flags<64;++flags) {
            _mm_setcsr((rounding<<13)|((denorm&1)<<6)|((denorm>>1)<<15)|(masks<<7)|flags);errno=ERANGE;
            compare(flags%2?accepted_fixture:rejected_fixture);
        }
    saved.saved.restore();
    const auto seed=[](unsigned precision,unsigned rounding,unsigned flags,int unmasked=-1) {
        std::array<std::byte,28> image{};asm volatile("fnstenv %0":"=m"(image)::"memory");
        std::uint16_t control{},status{};std::memcpy(&control,image.data(),2);std::memcpy(&status,image.data()+4,2);
        control=std::uint16_t((control&~0xf00u)|0x3fu|(precision<<8)|(rounding<<10));
        if(unmasked>=0)control=std::uint16_t(control&~(1u<<unsigned(unmasked)));
        status=std::uint16_t((status&~0x80bfu)|flags);if(flags&~control&0x3fu)status=std::uint16_t(status|0x8080u);
        std::memcpy(image.data(),&control,2);std::memcpy(image.data()+4,&status,2);asm volatile("fldenv %0"::"m"(image):"memory");
    };
    for(unsigned precision:{0u,2u,3u})for(unsigned rounding=0;rounding<4;++rounding)for(unsigned flags=0;flags<64;++flags) {
        seed(precision,rounding,flags);errno=EDOM;compare(accepted_fixture);compare(rejected_fixture);saved.saved.restore();
    }
    for(int unmasked=0;unmasked<6;++unmasked) {
        seed(3,2,0x3f,unmasked);errno=ERANGE;compare(accepted_fixture);compare(rejected_fixture);saved.saved.restore();
    }
}
using Predicate=bool(*)(std::span<const Vertex>,std::span<const SkinBinding>,std::span<const std::array<float,12>>)noexcept;
#if defined(__GNUC__)
__attribute__((noinline))
#endif
bool invoke(Predicate predicate,std::span<const Vertex> v,std::span<const SkinBinding> b,std::span<const std::array<float,12>> p) {
    asm volatile("":::"memory");return predicate(v,b,p);
}
void benchmark(std::span<const Vertex> vertices,std::span<const SkinBinding> bindings,
               std::span<const std::array<float,12>> palette,const char *label) {
    constexpr unsigned rounds=9,repetitions=32;std::array<std::uint64_t,rounds> old{},now{};unsigned receipts{};
    const auto measure=[&](Predicate predicate) {
        const auto start=std::chrono::steady_clock::now();
        for(unsigned n=0;n<repetitions;++n)receipts+=invoke(predicate,vertices,bindings,palette);
        return std::uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-start).count())/repetitions;
    };
    for(unsigned round=0;round<rounds;++round) {
        if(round%2){now[round]=measure(model_skin_gpu_arithmetic_admitted);old[round]=measure(reference::model_skin_gpu_arithmetic_admitted);}
        else {old[round]=measure(reference::model_skin_gpu_arithmetic_admitted);now[round]=measure(model_skin_gpu_arithmetic_admitted);}
    }
    std::sort(old.begin(),old.end());std::sort(now.begin(),now.end());
    need(receipts==rounds*repetitions*2,"Component benchmark workload was not fully admitted");
    std::cout<<"{\"component_benchmark\":\""<<label<<"\",\"reference_sha256\":\""<<reference_sha
        <<"\",\"vertices\":"<<vertices.size()<<",\"palette_rows\":"<<palette.size()<<",\"rounds\":"<<rounds
        <<",\"calls_per_round\":"<<repetitions<<",\"old_median_ns\":"<<old[rounds/2]<<",\"new_median_ns\":"<<now[rounds/2]<<"}\n";
}
void synthetic_benchmark() {
    Fixture bank(72);bank.vertices.resize(15000,bank.vertices[0]);bank.bindings.resize(15000,bank.bindings[0]);
    for(unsigned i=0;i<bank.bindings.size();++i) {
        bank.bindings[i].bones={std::uint16_t(i%72),std::uint16_t((i+1)%72),std::uint16_t((i+7)%72),std::uint16_t((i+19)%72)};
        bank.bindings[i].weights={.5f,.25f,.125f,.125f};
    }
    benchmark(bank.vertices,bank.bindings,bank.palette,"declared-15000-vertex-four-influence-bank");
}
#ifdef HALO2_ADMISSION_MCC_POSES
void mcc_poses(const std::filesystem::path &path,bool timing) {
    Cache cache(path);need(cache.is_mcc(),"Admission pose differential requires explicit prepared MCC input");
    const auto content=cache.load_content(0,true);
    const auto *weapon=cache.query_tag({.group=tag_groups::weapon,.name="objects\\weapons\\rifle\\smg\\smg",.require_address=true});
    need(weapon,"Prepared MCC SMG owner absent");unsigned poses{},models{};std::size_t actual_records{};
    const auto identifiers=cache.load_weapon_animation_identifiers(weapon->datum);
    for(unsigned representation:{2u,3u}) {
        const auto body=cache.load_model(content.player_representations.at(representation).biped.datum,false);
        const auto fp=cache.load_first_person_binding(weapon->datum,representation);
        const auto gun=cache.load_model(fp.weapon_model,false),arms=cache.load_model(fp.arms_model,false);
        const auto body_clip=cache.load_unit_weapon_action(body.animation,0x06000086u,identifiers[0],identifiers[1],0x0400000cu);
        const auto idle=std::find_if(fp.clock_clips.begin(),fp.clock_clips.end(),[](const auto &clip){return clip.activity==0x0400000cu;});
        need(idle!=fp.clock_clips.end(),"Prepared MCC first-person idle clip absent");const auto fp_clip=cache.load_animation(fp.graph,idle->index);
        for(unsigned kind=0;kind<3;++kind) {
            const auto &model=kind==0?body:kind==1?gun:arms;const auto &clip=kind==0?body_clip:fp_clip;
            need(!model.mesh.vertices.empty() && model.mesh.vertices.size()==model.skin.size(),"Prepared MCC pose has no complete skin bank");
            const auto source_vertices=model.mesh.vertices;const auto source_bindings=model.skin;++models;
            for(unsigned sample=0;sample<32;++sample) {
                Transform root;root.position={3.125f,-.07f,.625f};root.scale=sample%2?.625f:1.f;
                const float frame=float(sample)*float(clip.frame_count?clip.frame_count-1:0)/31.f;
                const auto nodes=pose_model_nodes_world_frame(model,root,&clip,frame);const auto palette=model_skin_world_palette(model,nodes);
                const bool admitted=compare(model.mesh.vertices,model.skin,palette);
                auto slow=palette;slow.resize(std::max<std::size_t>(257,slow.size()));
                need(compare(model.mesh.vertices,model.skin,slow)==admitted,"Slow larger-palette path changed the same actual MCC pose");
                if(timing && sample==0 && admitted)benchmark(model.mesh.vertices,model.skin,palette,kind==0?"actual-mcc-body-idle":kind==1?"actual-mcc-fp-smg-idle":"actual-mcc-fp-arms-idle");
                if(sample==0) {
                    auto rejected_vertices=model.mesh.vertices;rejected_vertices[0].position.x=bits(0x7f800001u);
                    need(!compare(rejected_vertices,model.skin,palette),"Actual MCC bank admitted a signaling skin attribute");
                }
                ++poses;actual_records+=model.mesh.vertices.size();
            }
            need(!std::memcmp(source_vertices.data(),model.mesh.vertices.data(),source_vertices.size()*sizeof(Vertex)) &&
                 !std::memcmp(source_bindings.data(),model.skin.data(),source_bindings.size()*sizeof(SkinBinding)),"Actual MCC predicate changed source geometry");
        }
    }
    need(models==6 && poses==192,"Incomplete genuine MCC species/body/FP pose differential");
    std::cout<<"PASS genuine MCC differential: "<<models<<" authored body/FP models, "<<poses<<" sampled authored idle poses, "<<actual_records<<" complete source records (ordinary runtime not run)\n";
}
#endif
}
int main(int argc,char **argv)try {
    Restore saved;bool timing=false;const char *map=nullptr;
    for(int i=1;i<argc;++i)if(std::string_view(argv[i])=="--benchmark")timing=true;else {need(!map,"Only one explicit prepared MCC map is allowed");map=argv[i];}
#ifdef HALO2_ADMISSION_MCC_POSES
    need(map,"MCC pose differential requires a prepared MCC map");mcc_poses(map,timing);
#else
    need(!map,"Standalone admission differential does not accept a map");boundaries();lattices();varied_banks();environments();if(timing)synthetic_benchmark();
#endif
    need(accepted && rejected,"Differential did not exercise acceptance and rejection");
    std::cout<<"PASS source-pinned admission differential: "<<comparisons<<" cases, "<<accepted<<" admitted, "<<rejected<<" rejected; heap-free/full FP and errno ownership; reference "<<reference_sha<<'\n';return 0;
}catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}
