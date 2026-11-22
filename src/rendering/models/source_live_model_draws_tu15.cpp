#include "halo2/source_live_model_draws_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_draw_resources.hpp"
#include "halo2/native_tu15_render_lighting.hpp"
#include "halo2/detail/source_object_render_queue_promotion.hpp"
#include "halo2/source_execution_calls.hpp"
#include "halo2/source_execution_model_inputs.hpp"
#include <algorithm>
#include <bit>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <set>

namespace halo2 {
namespace {
constexpr std::uint32_t collector_stack_base=0x64000000u,collector_stack_size=0x100000u;

class LiveModelCollectorStack {
    SourceRuntimeMemory& memory_;
public:
    explicit LiveModelCollectorStack(SourceRuntimeMemory& memory):memory_(memory) {
        constexpr auto owner="native TU1.5 live render collector stack";
        constexpr auto access=SourceRuntimeMemory::Access::ReadWrite;
        static const bool reference=[] {
            const auto* value=std::getenv("HALO2_REFERENCE_MODEL_COLLECTOR_STACK_COPY");
            return value && *value;
        }();
        if(reference) {
            memory_.mount(collector_stack_base,std::vector<std::byte>(collector_stack_size),access,owner);
        } else {
            // The region owns the fresh zeroed caller storage directly. Its
            // source address and observed mount/release boundaries stay the same.
            auto bytes=std::make_shared<std::vector<std::byte>>(collector_stack_size);
            auto backing=std::make_shared<SourceRuntimeMemoryBacking>(bytes,std::span(*bytes));
            memory_.mount(collector_stack_base,std::move(backing),access,owner);
        }
    }
    LiveModelCollectorStack(const LiveModelCollectorStack&)=delete;
    LiveModelCollectorStack& operator=(const LiveModelCollectorStack&)=delete;
    ~LiveModelCollectorStack(){memory_.release(collector_stack_base);}
};
}

void source_live_model_draws_tu15(PhysicsLiveWorldTu15& world,SourceObjectAdmission& admission,
    SourceDrawUnitState& state,const SourceLiveModelDrawContextTu15& context) {
    if(context.entry.mode!=context.draw.draw.mode || context.view>=4)
        throw FormatError("TU1.5 live model collector has inconsistent view/mode");
    auto& memory=world.memory();
    SourceExecutionCallScope journal(&memory,SourceExecutionCallKind::LiveModelDraws,[&]{return source_execution_arguments(admission,state,source_execution_model_input(context));});
    const auto read=[&](std::uint32_t p,std::size_t n){return Bytes(memory.view(p,n));};
    const auto word=[&](std::uint32_t p){return read(p,4).u32(0);};
    const auto body_address=[&](std::uint32_t owner) {
        const auto body=world.source_object_body(owner);
        (void)memory.view(body,4);
        return body;
    };
    const auto metadata=[&](std::uint32_t datum){return word(word(0x4f0414u)+(datum&65535u)*16u+8u);};
    // Native caller storage for the original stack collectors and their
    // recursive copies. Outputs retain tokens into the separate live render
    // cache/geometry arena, so this caller storage expires after collection.
    const LiveModelCollectorStack caller_storage(memory);
    std::uint32_t cursor=collector_stack_base,section_counter=0;
    const auto allocate=[&]() {
        if(cursor-collector_stack_base>collector_stack_size-0x200u)
            throw FormatError("TU1.5 render collector recursion exceeds native caller storage");
        const auto p=cursor;cursor+=0x200u;return p;
    };
    SourceDrawResourceWorld resources{word(0x4f0414u),word(0x4ecc18u),word(0x4ecc14u),word(0x4ecbd0u),
        [&](auto p,auto n){return memory.view(p,n);},{}};
    // 134240 classifies by the live root's current model. We already own
    // the read-only lookup banks, so classify each row once from those banks.
    source_object_render_queue_detail::promote(admission.objects,[&](const auto& row) {
        constexpr auto none=0xffffffffu;
        const auto definition=word(body_address(row.id));
        if(definition==none)return false;
        const auto dp=metadata(definition),model=dp?word(dp+0x38u):none;
        if(model==none)return false;
        const auto mp=metadata(model),render=mp?word(mp+4u):none;
        return render!=none && std::bit_cast<std::int32_t>(word(metadata(render)+0x74u))>0;
    });
    const auto unique=[&](auto& bank,auto value) {
        if(std::none_of(bank.begin(),bank.end(),[&](const auto& e){return e.identity==value.identity;}))bank.push_back(value);
    };
    auto draw_services=source_object_draw_resource_services(resources);
    const auto abi=world.prepare_source_call();
    NativeTu15PhysicsFrameFp arithmetic{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
    draw_services.draw=source_draw_resource_services_tu15(world,arithmetic);
    draw_services.draw.context=[&](auto p){return memory.mutable_view(p,0x78);};
    auto presentation=source_object_presentation_resource_services(resources);
    const auto counter_token=allocate();
    presentation.section_counter=[&](auto token)->std::uint32_t& {
        if(token!=counter_token)throw FormatError("TU1.5 live section counter identity differs");
        return section_counter;
    };
    presentation.copy_identity=allocate;
    presentation.build=[&](auto& collector,const auto& request){
        world.build_render_model(collector,request,context.view,context.local_players,context.view_modifier);
        if(context.built){SourceExecutionObserverScope observer(memory);context.built(collector);}
    };
    auto draw=context.draw;draw.temporary_address=allocate();
    presentation.collect=[&](const auto& collector,auto filter){
        source_object_collect_draw_units(state,collector,filter,draw,draw_services);
    };
    SourceObjectRenderEntryServices entry;
    entry.sphere=[&](auto owner){const auto b=read(body_address(owner)+0x30,16);
        return SourceObjectRenderSphere{{b.f32(0),b.f32(4),b.f32(8)},b.f32(12)};};
    entry.fade=[&](auto owner,float metric) {
        std::vector<SourceModelFadeOwner> os;
        std::vector<SourceModelFadeDefinition> ds;
        std::vector<SourceModelFadeModel> ms;
        const SourceModelFadeInput input{owner,metric,std::uint8_t(context.view_modifier)};
        const auto read_owner=[&](std::uint32_t id) {
            const auto b=read(body_address(id),0x40);
            return SourceModelFadeOwner{id,b.u32(4),b.u32(0x14),b.u32(0),b.f32(0x3c)};
        };
        os.push_back(read_owner(owner));
        // Whole4BAE0 returns before any definition/model/parent service for
        // these overrides. Do not resolve unconsumed metadata eagerly.
        if((*os.front().flags&0x80000u) || context.view_modifier)
            return source_model_fade(os,ds,ms,input);
        const auto owner_model=[&](const SourceModelFadeOwner& o) {
            const auto definition=*o.definition,model=word(metadata(definition)+0x38u);
            unique(ds,SourceModelFadeDefinition{definition,model});return model;
        };
        auto selected=owner_model(os.front());
        if(selected==0xffffffffu)return source_model_fade(os,ds,ms,input);
        std::set<std::uint32_t> seen{owner};
        while(*os.back().parent!=0xffffffffu) {
            const auto parent=*os.back().parent;
            if(!seen.insert(parent).second)throw FormatError("TU1.5 fade owner parent cycle");
            os.push_back(read_owner(parent));
        }
        if(os.back().identity!=owner) {
            const auto root_model=owner_model(os.back());
            if(root_model!=0xffffffffu)selected=root_model;
        }
        const auto m=read(metadata(selected),0x4e);SourceModelFadeModel value;
        value.identity=selected;value.fade_end=m.f32(0x28);value.fade_start=m.f32(0x2c);
        for(unsigned n=0;n<5;++n)value.render_cutoffs[n]=m.f32(0x34+n*4);
        value.render_lod_count=m.u16(0x4c);ms.push_back(value);
        return source_model_fade(os,ds,ms,input);
    };
    entry.build=presentation.build;
    entry.present=[&](auto& collector,auto children,auto skip,auto flags) {
        source_object_present(collector,children,skip,flags,
            {context.entry.eye,context.forward,context.entry.mode},presentation);
    };
    SourceObjectDrawCollector collector;collector.address=allocate();
    for(auto& row:admission.objects) {
        const SourceObjectRenderEntryRequest request{0,counter_token,0xffffffffu,context.children};
        const auto first_person=source_object_render_entry(row,collector,request,context.entry,entry);
        if(first_person && context.entry.mode==1)
            source_object_render_entry(row,collector,{1,counter_token,0xffffffffu,context.children},context.entry,entry);
    }
    journal.complete([&]{return source_execution_model_result(admission,state);});
}
}
