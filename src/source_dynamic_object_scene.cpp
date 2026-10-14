#include "halo2/source_dynamic_object_scene.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/source_object_colors.hpp"
#include "halo2/object_light_query.hpp"
#include "halo2/collision.hpp"
#include "halo2/math.hpp"
#include "source_object_geometry.hpp"
#include <map>
#include <set>
#include <cstring>

namespace halo2 {
struct SourceDynamicObjectScene::Impl {
    struct Bank {
        SourceObjectGeometry geometry;
        SourceInitialModelDefinition model;
        std::uint16_t flags{},type{};
    };
    struct Owner {
        SourceDynamicObject state;
        std::vector<Transform> nodes;
        ObjectLightingCache light;
        std::array<std::int8_t,4> previous_lod{-1,-1,-1,-1};
    };
    const Cache &cache;const CollisionWorld &collision;
    SourcePrtLighting sh;BspLighting lighting;BspSurfaceLighting surface;ObjectLightingDefinition conversion;
    std::map<std::uint32_t,Bank> banks;
    std::map<std::uint32_t,std::string> unavailable;
    std::set<std::uint32_t> invisible;
    std::map<std::uint32_t,Owner> owners;
    std::vector<std::uint32_t> order;
    std::array<std::vector<SourceDynamicObjectDraw>,4> draws;
    Impl(const Cache &c,std::size_t bsp,Mesh &mesh,const CollisionWorld &world)
        :cache(c),collision(world),sh(c.load_source_prt_lighting(bsp)),lighting(c.load_bsp_lighting(bsp)),
         surface(c.load_bsp_surface_lighting(bsp)),conversion(c.load_object_lighting_definition()) {
        for(const auto &tag:c.tags())if(tag.group=="eqip" || tag.group=="garb" || tag.group=="bloc") {
            const auto before=mesh.materials.size();
            try {
                const auto metadata=c.load_source_tag_metadata(tag.datum);
                const Bytes definition(metadata.bytes_at(metadata.source_address(),metadata.size()));
                if(definition.u32(0x38)==0xffffffffu){invisible.insert(tag.datum);continue;}
                Bank bank;bank.model=c.load_initial_model_definition(tag.datum);bank.flags=definition.u16(2);bank.type=definition.u16(0);
                ScenarioObject resource;resource.definition=tag.datum;resource.type=definition.u16(0);
                resource.variant=bank.model.default_variant;
                bank.geometry.load_union(c,resource,bank.model,bank.model.render_model);
                // Resource preparation only. These matrices never become a
                // live object's pose; every draw below requires source WORLD.
                const auto root=Transform{};const auto nodes=pose_model_nodes_world(bank.geometry.model(),root);
                bank.geometry.pose_world([&](const RenderModel &m){return pose_model_world(m,root);});
                bank.geometry.prepare(c,source_object_initial_colors({}, {},0),nodes,sh,mesh,0.f);
                banks.emplace(tag.datum,std::move(bank));
            }catch(const FormatError &e) {mesh.materials.resize(before);unavailable.emplace(tag.datum,e.what());}
        }
    }
    void synchronize(std::span<const SourceDynamicObject> live,std::span<const SourceWorldPose> poses) {
        validate_source_dynamic_objects(live);std::set<std::uint32_t> present;std::vector<std::uint32_t> next;
        for(const auto &o:live) {
            present.insert(o.object);next.push_back(o.object);
            if(const auto prior=owners.find(o.object);prior!=owners.end() && prior->second.state.definition!=o.definition)
                throw FormatError("Source dynamic full generation changed definition");
            const auto pose=std::find_if(poses.begin(),poses.end(),[&](const auto &p){return p.object==o.object;});
            if(o.has_world_nodes!=(pose!=poses.end()) || (pose!=poses.end() &&
                std::memcmp(&pose->sphere,&o.sphere,sizeof(o.sphere))))
                throw FormatError("Source dynamic WORLD bank differs from its completed object");
            if(!invisible.contains(o.definition)) {
                if(const auto bad=unavailable.find(o.definition);bad!=unavailable.end())
                    throw FormatError("Source dynamic model resource unavailable: "+bad->second);
                const auto bank=banks.find(o.definition);
                if(bank==banks.end() || o.type!=bank->second.type || pose==poses.end() || pose->nodes.size()!=bank->second.geometry.model().bones.size())
                    throw FormatError("Source dynamic model requires its actual prepared definition and WORLD hierarchy");
                if(o.regions.selection.permutations.size()!=bank->second.model.regions.size())
                    throw FormatError("Source dynamic region count differs from its actual model");
            }
        }
        auto staged=owners;
        for(const auto &o:live) {
            auto [at,fresh]=staged.try_emplace(o.object);auto &owner=at->second;(void)fresh;
            owner.state=o;const auto pose=std::find_if(poses.begin(),poses.end(),[&](const auto &p){return p.object==o.object;});
            owner.nodes=pose==poses.end()?std::vector<Transform>{}:pose->nodes;
        }
        std::erase_if(staged,[&](const auto &entry){return !present.contains(entry.first);});owners.swap(staged);order.swap(next);
    }
    const std::vector<SourceDynamicObjectDraw> &draw_view(std::int32_t frame,const SourceObjectViewLod &view,
        const std::function<std::vector<Transform>(std::uint32_t)> &sample) {
        if(view.view_index>=4)throw FormatError("Source dynamic view escapes original four-view bank");
        auto &out=draws[view.view_index];out.clear();
        for(auto id:order) {
            auto &owner=owners.at(id);const auto &o=owner.state;
            if(!o.visible || invisible.contains(o.definition))continue;
            auto &bank=banks.at(o.definition);auto &geometry=bank.geometry;
            const auto nodes=sample?sample(id):owner.nodes;
            if(nodes.size()!=owner.nodes.size())throw FormatError("Source dynamic interpolation changed its WORLD extent");
            geometry.regions(o.regions.selection);
            geometry.pose_world([&](const RenderModel &m){return skin_model_world_nodes(m,nodes);});
            const auto selected=geometry.select(id&65535u,id,o.sphere.center,o.sphere.radius,view,owner.previous_lod);
            const bool due=object_lighting_query_due(owner.light,frame,o.sphere.center,selected.source_distance,o.frozen);
            ObjectLightQueryContext context;context.bank_valid=source_object_light_query_bank_valid(lighting);
            context.object_type=o.type;context.definition_flags=bank.flags;context.runtime_flags=o.runtime_flags;
            const auto query=due?source_object_light_query(context,[&](unsigned mode) {
                return query_object_lightmap(surface,collision,o.sphere.center,ObjectLightMapQueryContext{mode,
                    !(owner.light.sample_valid && owner.light.lighting_valid),1.f,owner.light.target});
            },[]{return ObjectLightQueryResult{};}):ObjectLightQueryAdmission{};
            if(query.query.status==ObjectLightQueryStatus::attribution_unresolved)
                throw FormatError("Source dynamic object lighting attribution requires its actual contact owner");
            update_object_lighting_cache(owner.light,conversion,o.type,frame,o.sphere.center,o.velocity,due,
                query.accepted?query.query.sample:std::optional<ObjectLightmapSample>{},true,o.frozen);
            auto colors=source_object_initial_colors({}, {},0);
            for(unsigned i=0;i<o.colors.size();++i)colors[i]=o.colors[i];
            const auto &draw=geometry.view(view.view_index);
            out.push_back({id,o.definition,draw.vertices,owner.light.lighting,draw.ordinary_lighting,colors,
                geometry.owner_banks(cache,nodes,sh,selected.lod)});
        }
        return out;
    }
};
SourceDynamicObjectScene::SourceDynamicObjectScene(const Cache &c,std::size_t bsp,Mesh &m,const CollisionWorld &w)
    :impl_(std::make_unique<Impl>(c,bsp,m,w)){}
SourceDynamicObjectScene::~SourceDynamicObjectScene()=default;
void SourceDynamicObjectScene::synchronize(std::span<const SourceDynamicObject> o,std::span<const SourceWorldPose> p){impl_->synchronize(o,p);}
const SourceDynamicObject &SourceDynamicObjectScene::get(std::uint32_t o)const{return impl_->owners.at(o).state;}
std::span<const Transform> SourceDynamicObjectScene::nodes(std::uint32_t o)const{return impl_->owners.at(o).nodes;}
const std::vector<SourceDynamicObjectDraw> &SourceDynamicObjectScene::draw_view(std::int32_t f,const SourceObjectViewLod &v,
    const std::function<std::vector<Transform>(std::uint32_t)> &s){return impl_->draw_view(f,v,s);}
}
