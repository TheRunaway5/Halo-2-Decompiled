#include "halo2/source_device_world.hpp"
#include "halo2/content.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/static_scenery_collision.hpp"
#include "halo2/model_selection.hpp"
#include <algorithm>
#include <tuple>

namespace halo2 {
std::shared_ptr<const SourceDeviceWorldDefinition> load_source_device_world_definition(
    const Cache &cache,const ScenarioObject &object){
    if(object.definition==null_datum || (object.type!=7 && object.type!=8))return {};
    auto out=std::make_shared<SourceDeviceWorldDefinition>();
    out->motion=cache.load_source_device_motion_definition(object.definition);
    if(object.type==7){
        out->machine=cache.load_source_machine_definition(object.definition);
        out->placement=cache.load_source_machine_placement(unsigned(object.placement));
    }else{
        out->control=cache.load_source_control_definition(object.definition);
        const auto scenario=cache.query_tag({.group=tag_groups::scenario});
        if(!scenario)throw FormatError("control scenario missing");
        const auto metadata=cache.load_source_tag_metadata(scenario->datum);
        const Bytes s(metadata.bytes_at(metadata.source_address(),metadata.size()));
        const auto count=s.u32(0xb8);
        if(object.placement<0 || unsigned(object.placement)>=count || count>65536)
            throw FormatError("control placement outside scenario");
        const Bytes p(metadata.bytes_at(s.u32(0xbc)+unsigned(object.placement)*0x44,0x44));
        auto&d=out->placement;
        d.power_group=std::int16_t(p.u16(0x34));d.position_group=std::int16_t(p.u16(0x36));
        d.device_flags=p.u32(0x38);d.machine_flags=p.u32(0x3c);
        const auto initial=[&](int index,bool power){
            if(index==-1)return std::pair{power?(d.device_flags&2?0.f:1.f):(d.device_flags&1?1.f:0.f),
                std::uint16_t(power?4:(((d.device_flags&4)|0x10)>>2))};
            if(index<0 || unsigned(index)>=s.u32(0xa0))throw FormatError("control named group outside scenario");
            const Bytes g(metadata.bytes_at(s.u32(0xa4)+unsigned(index)*40,40));
            return std::pair{g.f32(32),std::uint16_t(g.u32(36)&1)};
        };
        std::tie(d.initial_power,d.power_group_flags)=initial(d.power_group,true);
        std::tie(d.initial_position,d.position_group_flags)=initial(d.position_group,false);
        out->control_bounds=cache.load_object_shadow_definition(object.definition);
        if(out->control->model!=null_datum){
            const auto model=cache.load_source_tag_metadata(out->control->model);
            const Bytes h(model.bytes_at(model.source_address(),model.size()));
            if(h.u32(4)!=null_datum){
                const auto selected=cache.load_initial_model_selection(object.definition,object.variant);
                if(!selected.resolved())throw FormatError("control targeting requires source model selection");
                auto hierarchy=cache.load_model(object.definition,false,object.variant,selected);
                out->control_nodes=pose_model_nodes(hierarchy);
                out->control_markers=source_model_markers(cache.load_source_tag_metadata(h.u32(4)),0x0500008f);
                out->control_permutations=selected.permutations;
                out->control_pose=cache.load_source_device_pose_definition(object.definition);
                hierarchy.mesh={};hierarchy.skin.clear();hierarchy.vertex_origins.clear();
                out->control_hierarchy=std::make_shared<RenderModel>(std::move(hierarchy));
            }
        }
    }
    if(out->machine && out->machine->model!=null_datum){
        const auto model=cache.load_source_tag_metadata(out->machine->model);
        const Bytes h(model.bytes_at(model.source_address(),model.size()));
        if(h.u32(4)!=null_datum && h.u32(0xc)!=null_datum){
            const auto selected=cache.load_initial_model_selection(object.definition,object.variant);
            if(!selected.resolved())throw FormatError("device collision requires source initial model selection");
            auto collision=std::make_shared<StaticSceneryCollisionDefinition>();
            collision->collision=cache.load_model_collision(out->machine->model);
            collision->hierarchy=cache.load_model(object.definition,false,object.variant,selected);
            collision->hierarchy.mesh={};collision->hierarchy.skin.clear();collision->hierarchy.vertex_origins.clear();
            collision->permutations=selected.permutations;out->collision=std::move(collision);
        }
    }
    return out;
}
std::vector<Transform> source_control_world_nodes(const SourceDeviceWorldDefinition &definition,
                                               const SourceDeviceWorldOwner &owner) {
    if(!definition.control)return {};
    if(definition.control_hierarchy && definition.control_pose) {
        const auto &pose=*definition.control_pose;
        if(pose.initialized_idle && pose.initialized_idle->frame_count>1)
            throw FormatError("Control targeting requires its current primary animation clock");
        const auto locals=source_machine_local_pose(*definition.control_hierarchy,pose,
            owner.motion.position,owner.motion.power);
        return pose_model_locals_nodes_world(*definition.control_hierarchy,owner.root,locals);
    }
    std::vector<Transform> nodes;nodes.reserve(definition.control_nodes.size());
    for(const auto &node:definition.control_nodes)nodes.push_back(compose_transform(owner.root,node));
    return nodes;
}
}
