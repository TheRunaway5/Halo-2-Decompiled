#include "halo2/menu_world_source_geometry.hpp"
#include "halo2/mcc_model_vertex_layout.hpp"
#include "halo2/source_bsp_ranges.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
}
int main(int argc,char **argv)try {
    using namespace halo2;
    if(argc!=2 || !std::filesystem::is_regular_file(argv[1]))return 77;
    const Cache cache(argv[1],std::filesystem::path(argv[1]).parent_path());
    check(cache.is_mcc() && cache.name()=="00a_introduction","Opening regression requires the MCC introduction cache");
    const auto *tag=cache.query_tag({.group=tag_groups::render_model,
        .name="scenarios\\objects\\covenant\\residential\\holo_panels\\holo_hallway_signs\\holo_hallway_signs",.require_address=true});
    check(tag,"MCC opening lacks its actual holographic hallway sign model");
    const auto metadata=cache.load_source_tag_metadata(tag->datum);
    const Bytes root(metadata.bytes_at(metadata.source_address(),0x68));
    check(root.u32(0x24)>0,"Opening model lost section zero");
    const Bytes section(metadata.bytes_at(root.u32(0x28),92));
    const unsigned opaque=std::to_integer<unsigned>(section.slice(0x14,1)[0]);
    const unsigned transparent=std::to_integer<unsigned>(section.slice(0x15,1)[0]);
    check(section.u16(0)==2 && opaque==0 && transparent==1 && root.u32(0x48)==3 && section.u16(4)==164,
        "Actual transparent-only rigid-boned opening regression tuple changed");
    const auto raw=cache.load_model_resources(tag->datum);const Bytes allocation(raw.at(0));
    const auto base=raw.at(0).size()-section.u32(0x44)-4;
    const auto header=allocation.slice(8,base-8);
    std::vector<MenuWorldSourceResource> resources;
    for(unsigned i=0;i<section.u32(0x48);++i) {
        const Bytes row(metadata.bytes_at(section.u32(0x4c)+i*16,16));
        const auto data=allocation.slice(base+row.u32(12),row.u32(8));
        resources.push_back({row.u16(4),row.u16(6),unsigned(base+row.u32(12)),{data.begin(),data.end()}});
    }
    const auto resource=[&](unsigned type,unsigned channel)->const auto& {
        const auto found=std::find_if(resources.begin(),resources.end(),[&](const auto &r){return r.type==type && r.channel==channel;});
        check(found!=resources.end(),"Opening regression lost its typed resource owner");return *found;
    };
    const auto expected=project_mcc_rigid_model_vertex_layout(2,std::max(opaque,transparent),3,164,
        header,resource(56,32).bytes,resource(56,0).bytes,resource(56,1).bytes,resource(56,2).bytes,resource(104,1).bytes);
    // This is the actual Cache -> section builder -> MCC layout call site,
    // which formerly passed opaque=0 and rejected the valid source section.
    const auto projected=load_menu_world_source_model_section(cache,tag->datum,0);
    check(projected.owner_datum()==tag->datum && projected.section()==0 && projected.model_geometry_class()==2 && projected.vertex_count()==164,
        "Opening section identity changed");
    check(std::ranges::equal(projected.raw_model_header(),header),"MCC opening raw header changed");
    check(std::ranges::equal(projected.node_map(),expected.node_map),"MCC opening node map changed");
    check(projected.resources().size()==resources.size(),"MCC opening resource owner count changed");
    for(unsigned i=0;i<resources.size();++i) {
        const auto &before=resources[i],&after=projected.resources()[i];auto bytes=before.bytes;
        if(before.type==56 && before.channel==2)bytes=expected.basis;
        if(before.type==56 && before.channel==32)std::copy(expected.descriptors.begin(),expected.descriptors.end(),bytes.begin());
        check(before.type==after.type && before.channel==after.channel && before.raw_offset==after.raw_offset && bytes==after.bytes,
            "MCC opening changed literal positions, UVs, indices, node bytes or resource ownership");
    }
    const auto ranges=load_source_model_range_geometry(cache,tag->datum,0),actual=source_model_range_geometry(projected);
    check(ranges.parts==actual.parts && ranges.subparts==actual.subparts && ranges.index_count==actual.index_count && ranges.mapping==actual.mapping,
        "MCC opening indexed ranges changed");
    check(std::any_of(projected.parts().begin(),projected.parts().end(),[](const auto &p){return p.type==4 || p.type==5;}),
        "MCC regression no longer covers a transparent part");
    std::cout<<"PASS actual MCC opening transparent-only rigid-boned section:164 vertices,3 nodes; literal streams and indexed ranges preserved\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
