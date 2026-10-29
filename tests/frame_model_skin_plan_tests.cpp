#include "rendering/renderer/frame_model_skin_plan.hpp"
#include <array>
#include <bit>
#include <cerrno>
#include <cfenv>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <ranges>
#include <stdexcept>

namespace {bool count_allocations{};std::size_t allocated_bytes{};}
void *operator new(std::size_t size) {
    if(count_allocations)allocated_bytes+=size;
    if(auto *result=std::malloc(size?size:1))return result;
    throw std::bad_alloc();
}
void *operator new[](std::size_t size){return ::operator new(size);}
void operator delete(void *pointer)noexcept{std::free(pointer);}
void operator delete[](void *pointer)noexcept{std::free(pointer);}
void operator delete(void *pointer,std::size_t)noexcept{std::free(pointer);}
void operator delete[](void *pointer,std::size_t)noexcept{std::free(pointer);}

using namespace halo2;
namespace {
void need(bool condition,const char *message) {if(!condition)throw std::runtime_error(message);}
template<class F> void rejects(F &&action,const char *message) {
    bool rejected=false;try {action();}catch(const FormatError&){rejected=true;}
    need(rejected,message);
}
std::shared_ptr<ModelSkinGeometry> geometry(std::size_t count,std::uint32_t owner=1,float translation=0) {
    auto out=std::make_shared<ModelSkinGeometry>();out->owner=owner;out->definition=2;out->render_model=3;out->lod=1;
    out->palette.push_back({1,0,0,translation,0,1,0,0,0,0,1,0});
    for(std::size_t i=0;i<count;++i) {
        Vertex v;v.position={float(i+1),1,0};v.normal={0,0,1};v.tangent={1,0,0};v.binormal={0,1,0};
        v.source_section=std::uint32_t(i/3);v.source_part=std::uint32_t(i%2);v.material=unsigned(i);
        out->vertices.push_back(v);out->bindings.emplace_back();out->bindings.back().weights[0]=float(i+1)/8.f;
        out->source_vertices.push_back(std::uint32_t(i+20));
    }
    return out;
}
std::array<ModelSkinSubmission,1> submission(std::size_t first,std::shared_ptr<const ModelSkinGeometry> owner) {
    return {{{first,std::move(owner)}}};
}
void empty_and_bounds() {
    FrameModelSkinPlan plan;plan.add_bank({},{});need(!plan.contains({}),"Empty plan contains a source");
    plan.append({},0);plan.validate_complete();need(plan.records().empty() && plan.palette().empty(),"Empty plan produced work");
    FrameModelSkinPlan empty_geometry;auto zero=geometry(0);zero->post_identity_placement=true;
    empty_geometry.add_bank({},submission(0,zero));empty_geometry.validate_complete();
    auto pose=geometry(3);auto bank=pose->vertices;FrameModelSkinPlan bad;
    rejects([&]{bad.add_bank(bank,submission(1,pose));},"Escaping submission accepted");
    rejects([&]{bad.add_bank(bank,submission(0,{}));},"Missing geometry accepted");
    auto wrong=geometry(3);wrong->bindings.pop_back();
    rejects([&]{bad.add_bank(bank,submission(0,wrong));},"Mismatched bindings accepted");
    wrong=geometry(3);wrong->source_vertices.pop_back();
    rejects([&]{bad.add_bank(bank,submission(0,wrong));},"Mismatched ordinals accepted");
    bank[0].position.x=-bank[0].position.x;
    rejects([&]{bad.add_bank(bank,submission(0,pose));},"Different local attributes accepted");
    bank=pose->vertices;bank[0].normal.x=std::bit_cast<float>(0x80000000u);
    rejects([&]{bad.add_bank(bank,submission(0,pose));},"Signed-zero local mismatch accepted");
    need(bad.expected_vertices()==0 && bad.palette().empty(),"Rejected bank partially registered");
}
void grouped_parts_and_repeated_copies() {
    auto pose=geometry(6);auto bank=pose->vertices;bank.insert(bank.begin(),Vertex{});bank.push_back(Vertex{});
    // Material/lighting metadata can change after the local deformation owner.
    bank[1].material=100;bank[1].color={.25f,.5f,.75f};bank[1].prt_cluster=9;
    FrameModelSkinPlan plan;plan.add_bank(bank,submission(1,pose));
    need(plan.expected_vertices()==6 && !plan.contains(std::span{bank}.first(1)),"Source registration extent wrong");
    need(plan.contains(std::span{bank}.subspan(0,2)),"Partial overlap missed");
    const std::array<std::span<const Vertex>,3> parts{std::span{bank}.subspan(4,3),{},std::span{bank}.subspan(1,3)};
    const auto transformed=parts | std::views::transform([](auto part){return part;});
    need(plan.contains_parts(transformed),"Grouped transform parts missed");plan.append_parts(transformed,10);
    plan.validate_complete();need(plan.mapped_vertices()==6 && plan.records().size()==6,"Grouped mapping lost vertices");
    for(unsigned i=0;i<6;++i) {
        const auto &record=plan.records()[i];need(record.destination==10+i,"Grouped destination order wrong");
        const auto source=i<3?i+3:i-3;
        need(record.weights==pose->bindings[source].weights && record.palette==0 && record.palette_count==1,"Grouped binding/palette wrong");
        need(!record.post_identity_placement,"Ordinary WORLD geometry acquired first-person placement");
    }
    plan.append(std::span{bank}.subspan(1,3),30);plan.validate_complete();
    need(plan.records().size()==9 && plan.mapped_vertices()==6,"Repeated draw did not produce independent destinations");
    rejects([&]{plan.append(std::span{bank}.subspan(1,3),10);},"Duplicate final GPU destination accepted");
    rejects([&]{plan.add_bank({},{});},"Late bank registration accepted");
    plan.validate_complete();need(plan.records().size()==9,"Rejected append altered records");
}
void owner_identity_and_source_aliases() {
    auto first=geometry(3,7,1),second=geometry(3,7,2);auto a=first->vertices,b=second->vertices;
    FrameModelSkinPlan plan;plan.add_bank(a,submission(0,first));plan.add_bank(b,submission(0,second));
    plan.append(a,3);plan.append(b,6);plan.validate_complete();
    need(plan.palette().size()==2 && plan.palette()[0][3]==1 && plan.palette()[1][3]==2,"Byte-identical local owners merged different poses");
    need(plan.records()[0].palette==0 && plan.records()[3].palette==1,"Owner-specific palettes lost");
    auto c=first->vertices;FrameModelSkinPlan shared;shared.add_bank(a,submission(0,first));shared.add_bank(c,submission(0,first));
    shared.append(a,0);shared.append(c,3);shared.validate_complete();
    need(shared.palette().size()==1 && shared.records()[3].palette==0,"Shared immutable geometry did not merge its palette");
    FrameModelSkinPlan alias;alias.add_bank(a,submission(0,first));
    rejects([&]{alias.add_bank(a,submission(0,second));},"Same source storage acquired two poses");
    FrameModelSkinPlan within;
    const std::array<ModelSkinSubmission,2> duplicate{{{0,first},{0,first}}};
    rejects([&]{within.add_bank(a,duplicate);},"Overlapping submissions within one bank accepted");
    need(within.expected_vertices()==0,"Rejected overlapping bank partially registered");
}
void foreground_post_modes_and_lifetime() {
    // These two owners intentionally have identical locals and palette bytes.
    // Their append policy differs and must survive actual grouping separately.
    auto world=geometry(3,17),foreground=std::make_shared<ModelSkinGeometry>(*world);
    foreground->post_identity_placement=true;
    auto no_influences=geometry(3,19);no_influences->post_identity_placement=true;
    const auto negative_zero=std::bit_cast<float>(0x80000000u);
    for(std::size_t i=0;i<no_influences->vertices.size();++i) {
        auto &vertex=no_influences->vertices[i];
        vertex.position={negative_zero,-1,-1};vertex.normal={negative_zero,1,-1};
        vertex.tangent={negative_zero,-1,-1};vertex.binormal={0,negative_zero,negative_zero};
        no_influences->bindings[i].weights={0,negative_zero,0,negative_zero};
        no_influences->bindings[i].bones.fill(65535);
    }
    auto bank=foreground->vertices;
    bank.insert(bank.end(),world->vertices.begin(),world->vertices.end());
    bank.insert(bank.end(),no_influences->vertices.begin(),no_influences->vertices.end());
    const auto original=bank;
    std::array<ModelSkinSubmission,3> submissions{{{0,foreground},{3,world},{6,no_influences}}};
    FrameModelSkinPlan plan;plan.add_bank(bank,submissions);
    submissions={};foreground.reset();world.reset();no_influences.reset();
    const std::array<std::span<const Vertex>,3> grouped{
        std::span{bank}.first(3),std::span{bank}.subspan(6,3),std::span{bank}.subspan(3,3)};
    need(plan.contains_parts(grouped),"Foreground post-placement ranges did not disable byte reuse");
    plan.append_parts(grouped,8);plan.validate_complete();
    need(plan.expected_vertices()==9 && plan.mapped_vertices()==9 && plan.records().size()==9,
         "Mixed foreground/WORLD grouped runs lost complete source coverage");
    need(plan.palette().size()==3,"Identical bytes merged geometry owners with different post modes");
    for(unsigned i=0;i<9;++i) {
        const auto &record=plan.records()[i];
        need(record.destination==8+i && record.post_identity_placement==(i<6),
             "Grouped destination lost its independent foreground post-placement mode");
        need(record.palette==(i<3?0:i<6?2:1) && record.palette_count==1,
             "Mixed post modes lost geometry-specific palette extent");
        if(i>=3 && i<6) {
            const std::array expected_weights{0.f,negative_zero,0.f,negative_zero};
            need(!std::memcmp(record.weights.data(),expected_weights.data(),sizeof(expected_weights)) &&
                 record.bones==std::array<std::uint32_t,4>{65535,65535,65535,65535},
                 "Post-placement record omitted an all-zero-influence source or changed signed-zero weights");
        }
    }
    plan.append(std::span{bank}.first(3),30);plan.validate_complete();
    need(plan.records().size()==12 && plan.records().back().post_identity_placement==1,
         "Repeated foreground draw lost placement mode or immutable geometry lifetime");
    need(!std::memcmp(bank.data(),original.data(),bank.size()*sizeof(Vertex)),
         "Frame planning executed first-person placement on registered local attributes");
}
void section_duplicates_completion_and_failures() {
    auto pose=geometry(3);const auto original_vertices=pose->vertices;const auto original_bindings=pose->bindings;
    pose->vertices.insert(pose->vertices.end(),original_vertices.begin(),original_vertices.end());
    pose->bindings.insert(pose->bindings.end(),original_bindings.begin(),original_bindings.end());
    pose->source_vertices={20,21,22,20,21,22};
    pose->vertices[3].material=99;auto bank=pose->vertices;FrameModelSkinPlan repeated;
    repeated.add_bank(bank,submission(0,pose));repeated.append(std::span{bank}.first(3),0);
    rejects([&]{repeated.validate_complete();},"Omitted expanded section duplicate accepted");
    repeated.append(std::span{bank}.subspan(3),3);repeated.validate_complete();
    need(repeated.expected_vertices()==6 && repeated.records().size()==6,"Expanded section duplicates collapsed");
    auto wrong=geometry(3);wrong->source_vertices={20,20,22};FrameModelSkinPlan bad;
    rejects([&]{bad.add_bank(wrong->vertices,submission(0,wrong));},"One source ordinal changed local attributes");
    wrong=geometry(3);wrong->bindings[0].weights={0,1,0,0};wrong->bindings[0].bones[0]=1;
    rejects([&]{bad.add_bank(wrong->vertices,submission(0,wrong));},"Interior-zero invalid bone accepted");
    auto ordinary=geometry(3);auto vertices=ordinary->vertices;FrameModelSkinPlan changed;
    changed.add_bank(vertices,submission(0,ordinary));vertices[1].tangent.y=1;
    rejects([&]{changed.append(vertices,0);},"Source mutation after registration accepted");
    rejects([&]{changed.validate_complete();},"Failed changed-source append counted as mapped");
    FrameModelSkinPlan large;large.add_bank(ordinary->vertices,submission(0,ordinary));
    rejects([&]{large.append(ordinary->vertices,std::size_t(std::numeric_limits<std::uint32_t>::max()));},"Destination index overflow accepted");
    need(large.records().empty(),"Rejected destination range partially mapped");
}
void fragmented_ranges_and_atomic_aliases() {
    auto a=geometry(7,31),b=geometry(5,32,3);
    std::vector<Vertex> bank(2);bank.insert(bank.end(),a->vertices.begin(),a->vertices.end());
    bank.resize(12);bank.insert(bank.end(),b->vertices.begin(),b->vertices.end());bank.resize(19);
    // Registration order differs from actual source/destination order, with
    // untouched holes before/between/after admitted contiguous source runs.
    const std::array<ModelSkinSubmission,2> owners{{{12,b},{2,a}}};
    FrameModelSkinPlan plan;plan.add_bank(bank,owners);plan.append(bank,100);plan.validate_complete();
    need(plan.expected_vertices()==12 && plan.records().size()==12,"Fragmented full-bank append lost its independent runs");
    for(unsigned i=0;i<12;++i) {
        const bool first=i<7;const auto source=first?i:i-7;
        const auto &record=plan.records()[i];
        need(record.destination==(first?102+i:112+source) && record.palette==(first?1u:0u) &&
            record.weights==(first?a:b)->bindings[source].weights,"Source registration order or holes changed flattened run mapping");
    }
    // Append before existing destinations, make adjacent ranges in reverse
    // order, then bridge both sides of the original gap without overlap.
    plan.append(std::span{bank}.subspan(6,3),4);plan.append(std::span{bank}.subspan(2,4),0);
    plan.append(std::span{bank}.subspan(12,5),7);plan.append(std::span{bank}.subspan(2,3),109);
    plan.append(std::span{bank}.subspan(2,1),117);plan.append(std::span{bank}.subspan(2,1),101);
    plan.validate_complete();need(plan.mapped_vertices()==12,"Repeated fragmented copies counted source coverage more than once");
    const auto before=std::vector<ModelSkinGpuRecord>(plan.records().begin(),plan.records().end());
    const auto palette=std::vector<std::array<float,12>>(plan.palette().begin(),plan.palette().end());
    const auto mapped=plan.mapped_vertices();
    rejects([&]{plan.append(std::span{bank}.subspan(2,7),106);},"Partially overlapping run after a bridge was accepted");
    rejects([&]{plan.append(std::span{bank}.subspan(2,7),99);},"Partially overlapping run starting before an interval was accepted");
    need(plan.records().size()==before.size() && !std::memcmp(plan.records().data(),before.data(),before.size()*sizeof(before.front())) &&
         plan.palette().size()==palette.size() && !std::memcmp(plan.palette().data(),palette.data(),palette.size()*sizeof(palette.front())) &&
         plan.mapped_vertices()==mapped,"Rejected partial interval overlap changed records, palettes or completed coverage");
    plan.validate_complete();
    auto last=geometry(1);FrameModelSkinPlan edge;edge.add_bank(last->vertices,submission(0,last));
    const auto maximum=std::numeric_limits<std::uint32_t>::max();edge.append(last->vertices,maximum);
    edge.append(last->vertices,std::size_t(maximum)-1);edge.validate_complete();
    need(edge.records()[0].destination==maximum && edge.records()[1].destination==maximum-1,
         "Last legal uint32 destination or its adjacent range wrapped a half-open interval");
    rejects([&]{edge.append(last->vertices,maximum);},"Maximum uint32 destination alias was accepted");
}
void dense_and_sparse_ordinal_bindings() {
    for(bool sparse:{false,true}) {
        auto owner=geometry(3);const auto first_vertices=owner->vertices;const auto first_bindings=owner->bindings;
        owner->vertices.insert(owner->vertices.end(),first_vertices.begin(),first_vertices.end());
        owner->bindings.insert(owner->bindings.end(),first_bindings.begin(),first_bindings.end());
        const auto high=std::numeric_limits<std::uint32_t>::max();
        owner->source_vertices=sparse?std::vector<std::uint32_t>{high,7,high-1,high,7,high-1}:
            std::vector<std::uint32_t>{0,1,2,0,1,2};
        owner->vertices[3].material=73;owner->vertices[3].color={.25f,.5f,.75f};
        auto bank=owner->vertices;FrameModelSkinPlan good;good.add_bank(bank,submission(0,owner));good.append(bank,0);good.validate_complete();
        need(good.records().size()==6 && good.records()[0].weights==good.records()[3].weights,
             "Dense/sparse high ordinal duplicate lost its original binding");
        for(bool signed_zero:{false,true}) {
            auto wrong=std::make_shared<ModelSkinGeometry>(*owner);
            if(signed_zero)wrong->bindings[3].weights[3]=std::bit_cast<float>(0x80000000u);
            else wrong->bindings[3].weights={.5f,0,0,0};
            FrameModelSkinPlan bad;
            rejects([&]{bad.add_bank(wrong->vertices,submission(0,wrong));},"Repeated dense/sparse ordinal changed only its binding");
            need(!bad.expected_vertices() && bad.palette().empty() && bad.records().empty(),
                 "Rejected duplicate-binding owner partially registered its bank");
        }
    }
}
void shared_packet_geometry_and_bank_validation() {
    std::weak_ptr<const ModelSkinGeometry> retained;
    {
        auto owner=geometry(3);retained=owner;const auto original=*owner;
        auto bank=owner->vertices;bank.insert(bank.end(),owner->vertices.begin(),owner->vertices.end());
        bank[3].material=111;bank[3].lightmap=17;bank[3].prt_cluster=2;
        std::array<ModelSkinSubmission,2> packet{{{0,owner},{3,owner}}};
        auto wrong_bank=bank;wrong_bank[3].normal.x=std::bit_cast<float>(0x80000000u);
        FrameModelSkinPlan bad;
        rejects([&]{bad.add_bank(wrong_bank,packet);},"Cached geometry skipped a second bank's actual local attributes");
        need(!bad.expected_vertices() && bad.palette().empty(),"Rejected shared packet partially registered its first source");
        FrameModelSkinPlan plan;plan.add_bank(bank,packet);packet={};owner.reset();
        need(!retained.expired(),"Shared immutable geometry owner was released before its frame");
        plan.append(std::span{bank}.subspan(3,3),0);plan.append(std::span{bank}.first(3),3);plan.validate_complete();
        need(plan.palette().size()==1 && plan.records().size()==6 && plan.records().front().palette==plan.records().back().palette,
             "Same immutable packet geometry duplicated its palette or lost an expanded source");
        const auto actual=retained.lock();
        need(actual && !std::memcmp(actual->vertices.data(),original.vertices.data(),original.vertices.size()*sizeof(Vertex)) &&
             !std::memcmp(actual->bindings.data(),original.bindings.data(),original.bindings.size()*sizeof(SkinBinding)) &&
             actual->source_vertices==original.source_vertices && actual->palette==original.palette,
             "Shared packet planning mutated its immutable geometry owner");
    }
    need(retained.expired(),"Frame plan retained shared geometry after its lifetime");
}
std::size_t fragmented_append_bytes(unsigned count) {
    auto owner=geometry(count);for(auto &binding:owner->bindings)binding.weights={.5f,0,0,0};
    FrameModelSkinPlan plan;plan.add_bank(owner->vertices,submission(0,owner));
    allocated_bytes=0;count_allocations=true;
    try{for(unsigned i=0;i<count;++i)plan.append(std::span{owner->vertices}.subspan(i,1),i*2);}
    catch(...){count_allocations=false;throw;}
    count_allocations=false;plan.validate_complete();return allocated_bytes;
}
void bounded_fragmented_storage_growth() {
    const auto small=fragmented_append_bytes(256),large=fragmented_append_bytes(512);
    need(small>0 && large<=small*3,"Doubling fragmented runs caused quadratic allocation/copy storage growth");
}
void empty_submissions_preserve_registered_plan() {
    auto owner=geometry(3);std::vector<Vertex> cpu_only(5);
    FrameModelSkinPlan plan;plan.add_bank(owner->vertices,submission(0,owner));
    const auto expected=plan.expected_vertices();const auto palette=std::vector<std::array<float,12>>(plan.palette().begin(),plan.palette().end());
    allocated_bytes=0;count_allocations=true;
    try{plan.add_bank({},{});plan.add_bank(cpu_only,{});}catch(...){count_allocations=false;throw;}
    count_allocations=false;
    need(!allocated_bytes,"Empty submission banks allocated/cloned existing plan ownership");
    need(plan.expected_vertices()==expected && !plan.mapped_vertices() && plan.records().empty() &&
         plan.palette().size()==palette.size() && !std::memcmp(plan.palette().data(),palette.data(),palette.size()*sizeof(palette.front())),
         "Empty submissions changed a nonempty registered plan");
    plan.append(owner->vertices,0);plan.validate_complete();
    const auto records=std::vector<ModelSkinGpuRecord>(plan.records().begin(),plan.records().end());
    rejects([&]{plan.add_bank({},{});},"Late empty bank bypassed append ownership");
    rejects([&]{plan.add_bank(cpu_only,{});},"Late CPU-only bank bypassed append ownership");
    need(plan.records().size()==records.size() && !std::memcmp(plan.records().data(),records.data(),records.size()*sizeof(records.front())),
         "Rejected late empty submissions changed completed records");
}
}
void banded_local_mismatch() {
    // The per-vertex local comparison scans in bands; a mismatch anywhere,
    // including a partial final band and the first vertex, must still be
    // rejected, and an unaltered bank must still be admitted.
    constexpr std::size_t count=10000;
    auto pose=geometry(count);
    // The shared helper's weights grow past1 beyond eight vertices; keep
    // every influence inside the admitted normal domain for this large bank.
    for(auto &binding:pose->bindings)binding.weights[0]=.5f;
    {FrameModelSkinPlan plan;plan.add_bank(pose->vertices,submission(0,pose));plan.append(pose->vertices,0);plan.validate_complete();
     need(plan.records().size()==count,"Admitted banded bank lost records");}
    for(const std::size_t index:{std::size_t{0},std::size_t{5000},count-1}) {
        auto bank=pose->vertices;bank[index].position.x+=1.f;
        FrameModelSkinPlan plan;
        rejects([&]{plan.add_bank(bank,submission(0,pose));},"Banded scan accepted a mismatched local vertex");
        need(!plan.contains(bank),"Rejected mismatched bank was retained");
    }
}
void mapped_owner_witness() {
    auto world=geometry(3,0x10001),foreground=geometry(3,0x20002),rigid=geometry(3,0x30003);
    foreground->post_identity_placement=true;
    for(auto &binding:rigid->bindings) {
        binding.weights={0.f,std::bit_cast<float>(0x80000000u),0.f,0.f};binding.bones.fill(65535);
    }
    auto a=world->vertices,b=foreground->vertices,c=world->vertices,d=rigid->vertices;
    FrameModelSkinPlan plan;
    plan.add_bank(a,submission(0,world),0);plan.add_bank(b,submission(0,foreground),0);
    plan.add_bank(c,submission(0,world),3);plan.add_bank(d,submission(0,rigid),2);
    unsigned observations{};
    plan.observe_mapped_owners([&](auto,const auto&,auto,auto){++observations;});
    need(!observations,"Registered but unmapped owners were reported as dispatched records");
    plan.append(a,0);plan.append(b,10);plan.append(c,20);plan.append(d,30);
    plan.append(std::span{a}.first(1),40);plan.validate_complete();
    std::size_t vertices{},weighted{};unsigned world_views{};
    plan.observe_mapped_owners([&](unsigned view,const ModelSkinGeometry &owner,std::size_t count,std::size_t influences) {
        ++observations;vertices+=count;weighted+=influences;
        if(owner.owner==world->owner) {
            ++world_views;need((view==0 && count==4)||(view==3 && count==3),
                "Shared geometry owner lost its submitting view or repeated actual destination count");
            need(influences==count && !owner.post_identity_placement,"WORLD owner acquired foreground/zero-weight attribution");
        } else if(owner.owner==foreground->owner)
            need(view==0 && count==3 && influences==3 && owner.post_identity_placement,
                "First-person records were attributed to a third-person body");
        else need(owner.owner==rigid->owner && view==2 && count==3 && !influences,
            "Signed-zero influences were reported as weighted GPU deformation");
    });
    need(observations==4 && world_views==2 && vertices==plan.records().size() && weighted==10,
        "GPU owner witness differs from actual grouped record coverage");
}
void retained_frame_scratch() {
    auto pose=geometry(3000),other=geometry(3000,2,.25f);
    for(auto *owner:{pose.get(),other.get()}) {
        for(auto &binding:owner->bindings)binding.weights[0]=.5f;
        owner->arithmetic_admitted=true;
    }
    const auto a=pose->vertices,b=other->vertices;
    const auto populate=[&](FrameModelSkinPlan &plan,unsigned variant) {
        plan.add_bank(a,submission(0,pose),0);plan.add_bank(b,submission(0,other),3);
        std::size_t destination=100;
        const auto append=[&](std::span<const Vertex> bank) {
            const std::size_t run=variant%3==0?31:variant%3==1?127:2050;
            for(std::size_t first=0;first<bank.size();) {
                const auto part=bank.subspan(first,std::min(run,bank.size()-first));
                plan.append(part,destination);first+=part.size();destination+=part.size();
            }
        };
        // Renderer grouping can reorder sources and repeat a selected range.
        if(variant&1){append(b);append(a);}else{append(a);append(b);}
        plan.append(std::span{a}.first(5),destination);plan.validate_complete();
    };
    FrameModelSkinPlan retained;
    need(!retained.retained_bytes(),"Empty planner retained unexpected storage");
    std::fenv_t saved;std::fegetenv(&saved);const auto saved_errno=errno;
    for(unsigned frame=0;frame<12;++frame) {
        FrameModelSkinPlan fresh;
        populate(fresh,frame);
        const auto old_rounding=std::fegetround(),old_flags=std::fetestexcept(FE_ALL_EXCEPT);
        errno=EDOM;
        {
            const auto scope=retained.frame_scope();
            need(!retained.expected_vertices()&&!retained.mapped_vertices()&&retained.records().empty()&&
                retained.palette().empty()&&!retained.contains(a),"New scratch frame retained a prior identity or count");
            populate(retained,frame);
            need(retained.records().size()==fresh.records().size()&&retained.palette().size()==fresh.palette().size()&&
                !std::memcmp(retained.records().data(),fresh.records().data(),retained.records().size_bytes())&&
                !std::memcmp(retained.palette().data(),fresh.palette().data(),retained.palette().size_bytes()),
                "Retained planner changed complete ordered GPU records or palette bytes");
            rejects([&]{const auto nested=retained.frame_scope();},"A second frame scope invalidated a live planner");
            retained.validate_complete();
        }
        need(std::fegetround()==old_rounding&&std::fetestexcept(FE_ALL_EXCEPT)==old_flags&&errno==EDOM,
            "Scratch reset changed FP or errno state");
        need(retained.records().empty()&&retained.palette().empty()&&!retained.expected_vertices()&&
            !retained.mapped_vertices()&&!retained.contains(a),"Completed scratch frame retained a live source");
        need(retained.retained_bytes()&&retained.retained_bytes()<=FrameModelSkinPlan::retained_capacity_limit,
            "Completed scratch frame lost its capacity or exceeded its8MiB bound");
    }
    const auto measure=[&](bool reuse) {
        allocated_bytes=0;count_allocations=true;
        try {
            for(unsigned frame=0;frame<3;++frame) {
                if(reuse){const auto scope=retained.frame_scope();populate(retained,frame);}
                else {FrameModelSkinPlan fresh;populate(fresh,frame);}
            }
        }catch(...){count_allocations=false;throw;}
        count_allocations=false;return allocated_bytes;
    };
    const auto fresh_bytes=measure(false),reused_bytes=measure(true);
    need(reused_bytes<fresh_bytes/2,"Warmed scratch did not remove repeated planner growth allocations");
    // An append can fail after earlier runs committed. The frame scope must
    // retire the whole frame on unwind, without carrying those aliases on.
    rejects([&] {
        const auto scope=retained.frame_scope();
        retained.add_bank(a,submission(0,pose));retained.append(std::span{a}.first(100),0);
        retained.append(std::span{a}.subspan(100,100),50);
    },"Scratch frame accepted an overlapping GPU destination");
    need(retained.records().empty()&&!retained.contains(a)&&!retained.expected_vertices(),
        "Rejected frame retained records or source aliases");
    {
        const auto scope=retained.frame_scope();populate(retained,7);
    }
    // Small retention budgets exercise the same trim path as an oversized
    // production frame without allocating an oversized test vertex bank.
    rejects([&] {
        const auto scope=retained.frame_scope(128);
        populate(retained,0);
        retained.append(a,std::numeric_limits<std::size_t>::max());
    },"Scratch frame accepted an overflowing GPU destination");
    need(!retained.retained_bytes()&&retained.records().empty()&&!retained.contains(a),
        "Oversized rejected scratch frame did not release all retained capacity");
    std::weak_ptr<const ModelSkinGeometry> weak;
    {
        const auto scope=retained.frame_scope();
        auto owner=geometry(3);weak=owner;const auto bank=owner->vertices;
        retained.add_bank(bank,submission(0,owner));retained.append(bank,0);owner.reset();
        need(!weak.expired(),"Frame scratch released an owner before frame completion");
    }
    need(weak.expired(),"Frame scratch retained immutable geometry across frame completion");
    {const auto scope=retained.frame_scope(0);}
    need(!retained.retained_bytes(),"Empty low-memory frame retained idle storage");
    std::fesetenv(&saved);errno=saved_errno;
    std::cout<<"PASS retained planner: cold/repeated ordered differential, source retirement, failure/overflow cleanup, "
        <<"bounded capacity, FP/errno; allocated "<<fresh_bytes<<" -> "<<reused_bytes<<" bytes for 3 frames\n";
}
int main()try {
    empty_and_bounds();grouped_parts_and_repeated_copies();owner_identity_and_source_aliases();
    foreground_post_modes_and_lifetime();
    section_duplicates_completion_and_failures();
    fragmented_ranges_and_atomic_aliases();dense_and_sparse_ordinal_bindings();
    shared_packet_geometry_and_bank_validation();bounded_fragmented_storage_growth();
    empty_submissions_preserve_registered_plan();banded_local_mismatch();mapped_owner_witness();retained_frame_scratch();
    std::cout<<"PASS frame model skin source/owner/grouped destination provenance and rejection gates\n";
    return 0;
}catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}
