#include "halo2/source_grenade_scene.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/content.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/source_object_colors.hpp"
#include "halo2/source_object_render_flags.hpp"
#include "halo2/object_light_query.hpp"
#include "gameplay/objects/source_object_geometry.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <map>
#include <set>
#include <thread>
#include <utility>

namespace halo2 {
Vec3 grenade_render_position(std::span<const NativeGrenadeProjectile> previous,
                             const NativeGrenadeProjectile& current,float fraction) {
    if(!std::isfinite(fraction) || fraction<0 || fraction>1)
        throw FormatError("Invalid grenade presentation fraction");
    if(current.datum==0xffffffffu || current.parent!=0xffffffffu || fraction==1)
        return current.position;
    const auto prior=std::find_if(previous.begin(),previous.end(),[&](const auto&p) {
        return p.datum==current.datum && p.definition==current.definition &&
               p.type==current.type && p.parent==0xffffffffu;
    });
    if(prior==previous.end())return current.position;
    if(fraction==0)return prior->position;
    return {std::lerp(prior->position.x,current.position.x,fraction),
            std::lerp(prior->position.y,current.position.y,fraction),
            std::lerp(prior->position.z,current.position.z,fraction)};
}
struct SourceGrenadeScene::Impl {
    struct Template {SourceObjectGeometry geometry;ShadowCasterDefinition bounds;std::uint16_t flags{};};
    struct Owner {
        NativeGrenadeProjectile projectile;
        std::array<std::int8_t,4> previous_lod{-1,-1,-1,-1};
        ObjectLightingCache light;
    };
    const Cache& cache;
    const CollisionWorld& collision;
    std::size_t capacity;
    float one;
    BspLighting lighting;
    BspSurfaceLighting surface;
    ObjectLightingDefinition conversion;
    std::vector<Template> templates;
    std::vector<SourceGrenadePreparedBank> banks;
    std::map<std::uint32_t,Owner> owners;
    std::vector<std::uint32_t> order;
    std::array<std::vector<SourceGrenadeDraw>,4> draws;
    const std::thread::id executor=std::this_thread::get_id();
    const std::shared_ptr<const int> identity=std::make_shared<const int>(0);
    std::uint64_t revision{};bool busy{};
    void own()const {if(std::this_thread::get_id()!=executor)throw FormatError("Grenade scene crossed its executor");}
    struct Mutation {
        Impl &owner;
        explicit Mutation(Impl &s):owner(s){s.own();if(s.busy||s.revision==UINT64_MAX)throw FormatError("Grenade scene mutation is reentrant or exhausted");++s.revision;s.busy=true;}
        ~Mutation(){owner.busy=false;}
    };
    struct Presentation {
        std::map<std::uint32_t,Owner> owners;std::vector<std::uint32_t> order;
        std::array<std::vector<SourceGrenadeDraw>,4> draws;
        std::vector<SourceObjectGeometry::PresentationState> geometry;
    };
    Presentation capture()const {
        Presentation out{owners,order,draws,{}};out.geometry.reserve(templates.size());
        for(const auto &bank:templates)out.geometry.push_back(bank.geometry.capture_presentation());return out;
    }
    void exchange(Presentation &state)noexcept {
        static_assert(std::is_nothrow_swappable_v<decltype(owners)>);
        static_assert(std::is_nothrow_swappable_v<decltype(order)>);
        static_assert(std::is_nothrow_swappable_v<decltype(draws)>);
        owners.swap(state.owners);order.swap(state.order);draws.swap(state.draws);
        for(std::size_t n=0;n<templates.size();++n)templates[n].geometry.exchange_presentation(state.geometry[n]);
    }

    Impl(const Cache&c,std::size_t bsp,Mesh&mesh,const CollisionWorld&cw,std::size_t slots,std::uint32_t one_bits)
        :cache(c),collision(cw),capacity(slots),one(std::bit_cast<float>(one_bits)),
         lighting(c.load_bsp_lighting(bsp)),surface(c.load_bsp_surface_lighting(bsp)),
         conversion(c.load_object_lighting_definition()) {
        if(!capacity || capacity>65536)throw FormatError("grenade scene requires the actual native object capacity");
        if(one_bits!=0x3f800000u)throw FormatError("grenade constructor requires its imported source one operand");
        const auto matg=c.query_tag({.group=tag_groups::globals});
        if(!matg)throw FormatError("grenade scene is missing authored matg");
        const auto globals=c.load_source_tag_metadata(matg->datum);
        const Bytes root(globals.bytes_at(globals.source_address(),globals.size()));
        const auto count=root.u32(0x100);
        if(count>2)throw FormatError("grenade scene requires a supported authored grenade bank");
        const Bytes rows(globals.bytes_at(root.u32(0x104),count*0x2c));
        const auto sh=c.load_source_prt_lighting(bsp);
        for(unsigned i=0;i<count;++i){
            const auto definition=rows.u32(i*0x2c+0x28);
            if(definition==0xffffffffu)throw FormatError("grenade scene has an authored NONE projectile");
            const auto tag=c.load_source_tag_metadata(definition);
            if(tag.group()!="proj")throw FormatError("grenade scene matg reference is not proj");
            const Bytes object(tag.bytes_at(tag.source_address(),tag.size()));
            if(object.u16(0)!=5)throw FormatError("grenade scene requires original projectile object type5");
            const auto initial=c.load_initial_model_definition(definition);
            const auto selection=c.load_initial_model_selection(definition);
            if(!selection.resolved())throw FormatError("grenade scene requires actual variant/permutation selection");
            const auto model=c.load_source_tag_metadata(initial.model);
            const Bytes model_data(model.bytes_at(model.source_address(),model.size()));
            if(model_data.u32(0x14)!=0xffffffffu || c.load_initialized_object_idle(definition))
                throw FormatError("grenade scene requires a genuine current animation manager");
            // The supplied frag/plasma models have no variants and precisely
            // one eligible region permutation; damaged/current region owners
            // cannot silently be replaced by this bounded cold consumer.
            if(!initial.variants.empty() || selection.variant_index!=-1 || initial.regions.size()!=1 ||
               selection.permutations!=std::vector<std::uint8_t>{0} ||
               initial.regions[0].permutation_flags!=std::vector<std::uint8_t>{0})
                throw FormatError("grenade scene current region lifecycle is not the proved singleton path");
            if(c.load_source_prt_definition(initial.render_model))
                throw FormatError("grenade scene PRT requires per-owner projected-bank ownership");
            const auto attribution=source_object_render_flag31_construction_event(0,c.load_object_light_contact_definition(definition));
            if(attribution.model_attribution_available)
                throw FormatError("grenade scene requires the source attributed-light ownership path");
            const auto colors=c.load_source_object_color_definition(definition);
            if(!colors.channels.empty())throw FormatError("grenade scene requires actual constructor color inputs");
            Template prepared;prepared.flags=object.u16(2);prepared.bounds=c.load_object_shadow_definition(definition);
            ScenarioObject request;request.type=5;request.definition=definition;request.variant=initial.default_variant;request.scale=one;
            prepared.geometry.load(c,request,initial,selection,initial.render_model);
            const auto frame=source_object_transform({},{one,0,0},{0,0,one},one,false);
            const auto nodes=pose_model_nodes_world(prepared.geometry.model(),frame);
            if(nodes.size()!=1)throw FormatError("grenade scene requires its proved single default node");
            prepared.geometry.pose_world([&](const RenderModel&m){return pose_model_world(m,frame);});
            prepared.geometry.prepare(c,source_object_initial_colors(colors,{},0),nodes,sh,mesh);
            SourceGrenadePreparedBank bank;bank.type=i;bank.definition=definition;
            bank.first_material=prepared.geometry.first_material;bank.material_count=prepared.geometry.material_count;
            for(unsigned lod=0;lod<6;++lod)bank.lod_vertices[lod]=prepared.geometry.at(lod).vertices.size();
            banks.push_back(bank);templates.push_back(std::move(prepared));
        }
    }
    void synchronize(std::span<const NativeGrenadeProjectile> live){
        std::set<std::uint32_t> present;std::set<std::uint16_t> slots;order.clear();
        for(const auto&p:live){
            if(p.parent!=0xffffffffu)
                throw FormatError("grenade scene currently requires the verified released-owner branch");
            if(p.datum==0xffffffffu || (p.datum&65535u)>=capacity || !present.insert(p.datum).second || !slots.insert(std::uint16_t(p.datum)).second ||
               p.type<0 || unsigned(p.type)>=banks.size() || banks[unsigned(p.type)].definition!=p.definition)
                throw FormatError("grenade scene lost its actual full owner/definition/type binding");
            auto [at,created]=owners.try_emplace(p.datum);
            if(!created && (at->second.projectile.definition!=p.definition || at->second.projectile.type!=p.type))
                throw FormatError("grenade scene full owner changed its projectile definition");
            at->second.projectile=p;order.push_back(p.datum);
        }
        for(auto at=owners.begin();at!=owners.end();)
            if(!present.contains(at->first))at=owners.erase(at);else ++at;
        for(auto&view:draws)view.clear();
    }
    const std::vector<SourceGrenadeDraw>& draw(std::int32_t frame,const SourceObjectViewLod&view){
        if(view.view_index>=draws.size())throw FormatError("grenade scene view is outside four source players");
        auto&out=draws[view.view_index];out.clear();
        for(auto datum:order){
            auto&owner=owners.at(datum);const auto&p=owner.projectile;
            auto&t=templates[unsigned(p.type)];const auto&bank=banks[unsigned(p.type)];
            // Native attachment already retained WORLD position/forward/up.
            // Original BDF80's default single node composes its authored local
            // bind with this same frame; no velocity-derived rotation is used.
            const auto root=source_object_transform(p.position,p.forward,p.up,one,false);
            const auto nodes=pose_model_nodes_world(t.geometry.model(),root);
            const auto sphere=source_shadow_sphere(t.bounds,nodes.at(0),one);
            t.geometry.pose_world([&](const RenderModel&m){return pose_model_world(m,root);});
            const auto selected=t.geometry.select(p.datum&65535u,p.datum,sphere.center,sphere.radius,view,owner.previous_lod);
            const bool due=object_lighting_query_due(owner.light,frame,sphere.center,selected.source_distance,false);
            ObjectLightQueryContext context;context.bank_valid=source_object_light_query_bank_valid(lighting);
            context.object_type=5;context.definition_flags=t.flags;
            // The supported ordinary E8F30 release clears object+C1bit0
            // through BAA80. Held objects have different source state and
            // require their own visibility/frozen-state producer.
            // Queried generic object+4 bit13 remains clear in this path.
            // Projectile+12C flags in NativeGrenadeProjectile are a separate
            // namespace and must never be substituted for generic object+4.
            context.runtime_flags=0;
            const auto query=due?source_object_light_query(context,[&](unsigned mode){
                return query_object_lightmap(surface,collision,sphere.center,ObjectLightMapQueryContext{mode,
                    !(owner.light.sample_valid&&owner.light.lighting_valid),one,owner.light.target});
            },[]{return ObjectLightQueryResult{};}):ObjectLightQueryAdmission{};
            if(query.query.status==ObjectLightQueryStatus::attribution_unresolved)
                throw FormatError("grenade scene source light contact attribution is unresolved");
            update_object_lighting_cache(owner.light,conversion,5,frame,sphere.center,p.velocity,due,
                query.accepted?query.query.sample:std::optional<ObjectLightmapSample>{},true,false);
            const auto&vertices=t.geometry.view(view.view_index);
            out.push_back({p.datum,p.definition,unsigned(p.type),bank.first_material,bank.material_count,
                           selected,vertices.vertices,owner.light.lighting,vertices.ordinary_lighting});
        }
        return out;
    }
};
struct SourceGrenadeScene::Checkpoint::Data {
    std::weak_ptr<const int> owner;std::thread::id executor;Impl::Presentation state;
};
struct SourceGrenadeScene::PreparedData {
    std::weak_ptr<const int> owner;std::thread::id executor;std::uint64_t revision{};
    Impl::Presentation state;bool consumed{};
};
SourceGrenadeScene::SourceGrenadeScene(const Cache&c,std::size_t bsp,Mesh&mesh,const CollisionWorld&world,
    std::size_t capacity,std::uint32_t one):impl_(std::make_unique<Impl>(c,bsp,mesh,world,capacity,one)){}
SourceGrenadeScene::~SourceGrenadeScene()=default;
bool SourceGrenadeScene::Checkpoint::valid()const noexcept{return data_&&!data_->owner.expired()&&data_->executor==std::this_thread::get_id();}
SourceGrenadeScene::Checkpoint SourceGrenadeScene::checkpoint()const {
    if(!impl_)throw FormatError("Moved grenade scene has no presentation owner");
    if(!impl_)throw FormatError("Moved grenade scene has no presentation owner");
    const auto &s=*impl_;s.own();if(s.busy)throw FormatError("Grenade checkpoint requires completed callbacks/draws");
    Checkpoint out;out.data_=std::make_shared<const Checkpoint::Data>(Checkpoint::Data{s.identity,s.executor,s.capture()});return out;
}
bool SourceGrenadeScene::owns_checkpoint(const Checkpoint &checkpoint)const noexcept {
    return impl_&&checkpoint.data_&&std::this_thread::get_id()==impl_->executor&&checkpoint.data_->executor==impl_->executor&&checkpoint.data_->owner.lock()==impl_->identity;
}
SourceGrenadeScene::PreparedRestore::PreparedRestore()=default;
SourceGrenadeScene::PreparedRestore::~PreparedRestore()=default;
SourceGrenadeScene::PreparedRestore::PreparedRestore(PreparedRestore &&)noexcept=default;
SourceGrenadeScene::PreparedRestore &SourceGrenadeScene::PreparedRestore::operator=(PreparedRestore &&)noexcept=default;
SourceGrenadeScene::PreparedRestore SourceGrenadeScene::prepare_restore(const Checkpoint &checkpoint)const {
    if(!owns_checkpoint(checkpoint)||impl_->busy||impl_->revision==UINT64_MAX)throw FormatError("Grenade restore has a foreign or incomplete owner");
    PreparedRestore out;out.data_=std::make_unique<PreparedData>(PreparedData{impl_->identity,impl_->executor,impl_->revision,checkpoint.data_->state,false});return out;
}
bool SourceGrenadeScene::restore_ready(const PreparedRestore &prepared)const noexcept {
    const auto *p=prepared.data_.get();return impl_&&p&&!p->consumed&&std::this_thread::get_id()==impl_->executor&&p->executor==impl_->executor&&
        p->owner.lock()==impl_->identity&&!impl_->busy&&p->revision==impl_->revision&&impl_->revision!=UINT64_MAX;
}
bool SourceGrenadeScene::commit_restore(PreparedRestore &prepared)noexcept {
    if(!restore_ready(prepared))return false;impl_->exchange(prepared.data_->state);++impl_->revision;prepared.data_->consumed=true;return true;
}
void SourceGrenadeScene::reset_lifetimes() noexcept {
    if(!impl_||std::this_thread::get_id()!=impl_->executor||impl_->busy||impl_->revision==UINT64_MAX)return;
    Impl::Mutation mutation(*impl_);
    auto &s=*impl_;s.owners.clear();s.order.clear();
    for(auto &view:s.draws)view.clear();
    for(auto &bank:s.templates)bank.geometry.reset_history();
}
SourceGrenadeScene::SourceGrenadeScene(SourceGrenadeScene&&)noexcept=default;
SourceGrenadeScene&SourceGrenadeScene::operator=(SourceGrenadeScene&&)noexcept=default;
std::span<const SourceGrenadePreparedBank>SourceGrenadeScene::banks()const{impl_->own();return impl_->banks;}
void SourceGrenadeScene::synchronize(std::span<const NativeGrenadeProjectile>objects){Impl::Mutation mutation(*impl_);impl_->synchronize(objects);}
const std::vector<SourceGrenadeDraw>&SourceGrenadeScene::draw_view(std::int32_t frame,const SourceObjectViewLod&view){Impl::Mutation mutation(*impl_);return impl_->draw(frame,view);}
} // namespace halo2
