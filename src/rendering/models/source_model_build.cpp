#include "halo2/source_model_build.hpp"
#include "halo2/source_render_cache.hpp"
#include <bit>
#include <cstring>

namespace halo2 {
namespace {
template<class T>T read(const SourceModelResourceWorld& w,std::uint32_t at) {
    if(!w.banks.read)throw FormatError("source model build reader absent");
    const auto raw=w.banks.read(at,sizeof(T));
    if(raw.size()!=sizeof(T))throw FormatError("source model build input truncated");
    T v;std::memcpy(&v,raw.data(),sizeof(T));return v;
}
template<class T>void put(const SourceModelResourceWorld& w,std::uint32_t at,const T& value) {
    if(!w.write)throw FormatError("source model build writer absent");
    auto raw=w.write(at,sizeof(T));
    if(raw.size()!=sizeof(T))throw FormatError("source model build output truncated");
    std::memcpy(raw.data(),&value,sizeof(T));
}
std::uint32_t body(const SourceModelResourceWorld& w,std::uint32_t owner) {
    return read<std::uint32_t>(w,read<std::uint32_t>(w,w.banks.objects+0x44)+(owner&65535u)*12+8);
}
std::uint32_t metadata(const SourceModelResourceWorld& w,std::uint32_t tag) {
    return read<std::uint32_t>(w,w.banks.tag_table+(tag&65535u)*16+8);
}
std::uint32_t acquire(const SourceModelResourceWorld& w,std::uint32_t owner) {
    const auto result=source_model_acquire_cache(owner,w);
    if(w.cache_acquired)w.cache_acquired(owner,result);
    return result;
}
std::uint32_t record(const SourceModelResourceWorld& w,std::uint32_t token) {
    if((token&65535u)>=256)throw FormatError("source model build token exceeds cache");
    return read<std::uint32_t>(w,w.render_cache_pool+0x44)+(token&65535u)*256;
}
std::uint32_t geometry(const SourceModelResourceWorld& w,std::uint32_t arena,std::uint32_t token) {
    if(token==0xffffffffu || std::bit_cast<std::int32_t>(token)<0)return 0;
    (void)w;return arena+(token&0xfffffff);
}
}
void source_model_build(std::uint32_t owner,float metric,float lighting_metric,
    std::uint32_t out,std::uint32_t rebase,std::uint32_t force_world,
    std::uint32_t mode,const SourceModelBuildWorld& world) {
    auto admission=world.admission;auto& w=admission.memory;
    if(!world.scratch)throw FormatError("source model build scratch owner absent");
    w.temporary_regions=world.scratch+0x3300;
    auto nodes_world=world.nodes;nodes_world.memory=w;
    const auto scratch=world.scratch;
    put<std::uint8_t>(w,scratch+0x15,255);
    auto models=std::int32_t(source_model_render_admission(owner,metric,mode,out+0x168,
        force_world,scratch+0x15,out,admission));
    const auto requested=read<std::int8_t>(w,scratch+0x15);
    put(w,out+0x164,std::int16_t(requested));
    if(read<std::uint8_t>(w,out+0x168) && world.view_owner!=0xffffffffu &&
        !source_foreground_models_admitted(read<std::int16_t>(w,world.view_players+world.view_owner*0x94+0x42)))models=0;
    put<std::uint16_t>(w,out+0x162,0);
    const auto header=read<std::uint32_t>(w,w.banks.objects+0x44)+(owner&65535u)*12;
    std::uint16_t auxiliary=65535;
    if(read<std::uint8_t>(w,header+3)==6)auxiliary=read<std::uint16_t>(w,read<std::uint32_t>(w,header+8)+0x134);
    put(w,out+0x166,auxiliary);
    if(models>5)throw FormatError("source model build exceeds five output slots");
    std::uint32_t skipped=0;
    for(std::int32_t ordinal=0;ordinal<models;++ordinal) {
        const auto slot=std::uint32_t(ordinal)-skipped;
        const auto owner_out=out+0x30+slot*4,model_out=out+0x58+slot*4;
        source_model_build_inputs(read<std::uint8_t>(w,out+0x168),std::uint32_t(ordinal),
            scratch+0x38,owner_out,model_out,scratch+0x2c,scratch+0x34,owner,world.cached_models,w);
        const auto resolved=read<std::uint32_t>(w,owner_out);
        const auto prior=source_model_prior_lod(resolved,w);
        if(w.view>=0 && w.view<4)(void)acquire(w,resolved);
        const auto token=acquire(w,resolved);
        const auto context=token==0xffffffffu?0:record(w,token)+0x44;
        put(w,out+0x44+slot*4,context);
        put(w,out+0x6c+slot,std::uint8_t(read<std::uint8_t>(w,scratch+0x38)&1));
        auto begin=source_render_geometry_begin(context!=0,
            context?read<std::uint32_t>(w,context+8):0xffffffffu,
            context?read<std::uint32_t>(w,context+12):0,prior,requested,w.render_frame,w.view);
        if(begin.reuse_candidate)put(w,out+0x164,std::int16_t(prior));
        const auto selected=out+0x71+slot*16,regions=out+0x14+slot*4;
        const auto ok=source_model_resources(owner,read<std::uint8_t>(w,out+0x168),
            read<std::uint32_t>(w,model_out),regions,out+0x164,std::uint8_t(prior),selected,scratch+0x16,w);
        const auto actual=read<std::int16_t>(w,out+0x164);
        if((ordinal>0 && actual!=requested) || !ok){++skipped;continue;}
        const auto count=source_model_geometry_node_count(read<std::uint32_t>(w,model_out),actual,selected,w);
        put(w,out+0x28+slot,std::uint8_t(count));put<std::uint32_t>(w,out+slot*4,0xffffffffu);
        SourceRenderGeometryInput input;
        input.begin=begin;input.requested_lod=requested;input.resource_succeeded=ok;
        input.region_changed=read<std::uint8_t>(w,scratch+0x16)!=0;input.actual_lod=actual;
        input.node_count=count;input.source_model_ordinal=ordinal;
        input.first_person=read<std::uint8_t>(w,out+0x168)!=0;input.context_present=context!=0;
        input.context_token=context?read<std::uint32_t>(w,context+8):0xffffffffu;
        input.context_stamp=context?read<std::uint32_t>(w,context+12):0;
        input.frame=w.render_frame;input.view=w.view;
        auto cursor=read<std::uint32_t>(w,world.arena_cursor);
        const auto allocation=source_render_geometry_allocate(input,cursor);
        if(allocation.continuation==SourceRenderGeometryContinuation::omitted){++skipped;continue;}
        put(w,out+slot*4,allocation.token);
        if(allocation.continuation==SourceRenderGeometryContinuation::build_nodes) {
            put(w,world.arena_cursor,cursor);
            if(allocation.context_written) {
                put(w,context+12,allocation.context_stamp);put(w,context+8,allocation.context_token);
            }
            auto node_bank=read<std::uint32_t>(w,scratch+0x2c);
            if(rebase&255u) {
                if(!world.rebase)throw FormatError("source model build sky rebase owner absent");
                world.rebase(owner,read<std::int32_t>(w,scratch+0x34),node_bank,scratch+0x3c);
                node_bank=scratch+0x3c;
            }
            source_model_geometry_node_bank(owner,read<std::uint32_t>(w,model_out),node_bank,
                selected,actual,read<std::uint8_t>(w,out+0x168),
                geometry(w,world.arena,allocation.token),nodes_world);
        }
        bool accepted=true;
        if(!allocation.reused) {
            if(read<std::uint8_t>(w,out+0x16c)&0x40) {
                if(!world.prt)throw FormatError("source model build PRT owner absent");
                const auto model=read<std::uint32_t>(w,model_out);
                accepted=world.prt({read<std::uint32_t>(w,metadata(w,model)+0x78),
                    std::uint32_t(std::int32_t(read<std::int16_t>(w,out+0x164))),
                    read<std::uint32_t>(w,owner_out),geometry(w,world.arena,allocation.token),
                    read<std::uint32_t>(w,regions),selected,model});
            }
            source_model_refresh_colors(read<std::uint32_t>(w,owner_out),0,w);
            if(context) {
                if(!world.lighting)throw FormatError("source model build lighting query owner absent");
                put(w,context+0x10,world.lighting(read<std::uint32_t>(w,owner_out),lighting_metric));
            }
        } else if(!read<std::uint8_t>(w,out+0x168) && w.view!=-1)
            put(w,out+slot*4,read<std::uint32_t>(w,context+8));
        if(!accepted){++skipped;continue;}
        put(w,out+0x162,std::uint16_t(read<std::uint16_t>(w,out+0x162)+1));
        const auto rendered=read<std::uint32_t>(w,owner_out);
        const auto cached=read<std::uint32_t>(w,body(w,rendered)+0xcc);
        if(cached!=0xffffffffu && read<std::uint32_t>(w,record(w,cached)+4)==rendered)
            put(w,record(w,cached)+0xc,w.render_frame);
    }
}
}
