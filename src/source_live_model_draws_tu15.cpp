#include "halo2/source_live_model_draws_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_draw_resources.hpp"
#include "halo2/source_object_render_queue.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <set>

namespace halo2 {
void source_live_model_draws_tu15(PhysicsLiveWorldTu15& world,SourceObjectAdmission& admission,
    SourceDrawUnitState& state,const SourceLiveModelDrawContextTu15& context) {
    if(context.entry.mode!=context.draw.draw.mode || context.view>=4)
        throw FormatError("TU1.5 live model collector has inconsistent view/mode");
    auto& memory=world.memory();
    const auto read=[&](std::uint32_t p,std::size_t n){return Bytes(memory.view(p,n));};
    const auto word=[&](std::uint32_t p){return read(p,4).u32(0);};
    const auto body_address=[&](std::uint32_t owner) {
        (void)world.object_body(owner); // Validate the live full generation.
        return word(word(word(0x4ecbd0u)+0x44u)+(owner&65535u)*12u+8u);
    };
    const auto metadata=[&](std::uint32_t datum){return word(word(0x4f0414u)+(datum&65535u)*16u+8u);};
    // Native caller storage for the original stack collectors and their
    // recursive copies. Outputs retain tokens into the separate live render
    // cache/geometry arena, so this caller storage expires after collection.
    constexpr std::uint32_t base=0x64000000u,size=0x100000u;
    memory.mount(base,std::vector<std::byte>(size),SourceRuntimeMemory::Access::ReadWrite,
        "native TU1.5 live render collector stack");
    struct Release {SourceRuntimeMemory& m;~Release(){m.release(base);}} release{memory};
    std::uint32_t cursor=base,section_counter=0;
    const auto allocate=[&]() {
        if(cursor-base>size-0x200u)throw FormatError("TU1.5 render collector recursion exceeds native caller storage");
        const auto p=cursor;cursor+=0x200u;return p;
    };
    SourceDrawResourceWorld resources{word(0x4f0414u),word(0x4ecc18u),word(0x4ecc14u),word(0x4ecbd0u),
        [&](auto p,auto n){return memory.view(p,n);},
        [&](auto flags,auto p) {
            // Actual130980 source request; no published pointer is inferred.
            return world.request_render_geometry(flags,p);
        }};
    std::vector<SourceObjectRenderQueueOwner> owners;
    std::vector<SourceObjectRenderQueueDefinition> definitions;
    std::vector<SourceObjectRenderQueueModel> models;
    std::vector<SourceObjectRenderQueueGeometry> geometry;
    const auto unique=[&](auto& bank,auto value) {
        if(std::none_of(bank.begin(),bank.end(),[&](const auto& e){return e.identity==value.identity;}))bank.push_back(value);
    };
    for(const auto& row:admission.objects) {
        const auto body=body_address(row.id),definition=word(body);
        unique(owners,SourceObjectRenderQueueOwner{row.id,true,definition});
        if(definition==0xffffffffu)continue;
        const auto dp=metadata(definition),model=dp?word(dp+0x38):0xffffffffu;
        unique(definitions,SourceObjectRenderQueueDefinition{definition,dp!=0,model});
        if(model==0xffffffffu)continue;
        const auto mp=metadata(model),render=mp?word(mp+4):0xffffffffu;
        unique(models,SourceObjectRenderQueueModel{model,mp!=0,render});
        if(render!=0xffffffffu)unique(geometry,SourceObjectRenderQueueGeometry{render,
            std::bit_cast<std::int32_t>(word(metadata(render)+0x74))});
    }
    source_object_render_queue_order(admission.objects,owners,definitions,models,geometry);
    auto draw_services=source_object_draw_resource_services(resources);
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
        if(context.built)context.built(collector);
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
}
}
