#include "halo2/source_bsp_effect_lights.hpp"
#include "halo2/source_bsp_breakable_visibility_tu15.hpp"
#include "halo2/source_live_model_geometry_tu15.hpp"
#include "halo2/source_live_shadow_main_draws_tu15.hpp"
#include "halo2/source_live_shadow_selection_tu15.hpp"
#include "halo2/source_campaign_view_tu15.hpp"
#include "halo2/source_live_shadow_main_inputs_tu15.hpp"
#include "halo2/source_live_shadow_storage_tu15.hpp"
#include "halo2/source_live_effect_light_commands_tu15.hpp"
#include "halo2/source_model_effect_lights.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_effect_light_geometry_frame.hpp"
#include "halo2/source_tag_metadata_store.hpp"
#include "halo2/world_material.hpp"
#include "halo2/pstencil/material.hpp"
#include "halo2/pstencil/shared_light_materials.hpp"
#include "halo2/pstencil/caster.hpp"
#include "halo2/menu_world_depth.hpp"
#include "halo2/source_portal_visibility.hpp"
#include "halo2/source_bsp_ranges.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include <algorithm>
#include <cstring>
#include <map>
#include <set>
#include <bit>
#include <tuple>
#include <iostream>
#include <cstdlib>
#include <optional>

namespace halo2 {
struct SourceBspEffectLights::Impl {
    const Cache& cache;
    const Mesh& mesh_owner;
    pstencil::MaterialBank pstencil_materials;
    std::shared_ptr<pstencil::SharedLightMaterials> model_materials;
    std::size_t bsp_index;
    SourceBspAdmissionBsp bsp;
    SourceTagMetadataStore store;
    SourceTagMetadataProviders metadata;
    std::vector<SourceBspDrawGeometry> clusters,definitions;
    std::vector<SourceBspDrawInstance> instances;
    std::vector<Transform> transforms;
    std::vector<std::array<float,12>> palettes;
    std::vector<SourceBspCommandGeometry> cluster_commands,definition_commands;
    std::vector<std::int16_t> instance_definitions;
    std::vector<SourceBspCommandMaterial> materials;
    std::vector<std::byte> visibility=std::vector<std::byte>(0x4204);
    std::map<std::uint32_t,SourceBspCommandShader> shaders;
    using SectionKey=std::pair<bool,unsigned>;
    struct Section {
        std::shared_ptr<const MenuWorldSourceSection> raw;
        SourceBspRangeGeometry ranges;
        SourceOpaqueGeometryMetadata metadata;
    };
    std::map<SectionKey,Section> sections;
    using ModelKey=std::pair<std::uint32_t,unsigned>;
    struct ModelSection {
        SourceBspRangeGeometry ranges;
        SourceOpaqueGeometryMetadata metadata;
        std::unique_ptr<SourceModelEffectLights> draw;
    };
    std::map<ModelKey,ModelSection> models;
    std::unique_ptr<pstencil::PointCasterBank> pstencil_casters;
    using MaterialKey=std::tuple<std::uint32_t,std::uint32_t,unsigned,unsigned>;
    struct Slot {unsigned material;Material authored;MenuWorldSourcePass pass;};
    std::map<MaterialKey,Slot> slots;
    std::map<std::uint32_t,EffectLightDefinition> lights;
    std::optional<SourceLiveShadowStorageTu15> shadow_storage;
    SourceDrawUnitState units;
    SourceBspRangeArena main_arena,light_arena;
    SourceBspPartCommandState commands;
    std::array<SourceOpaqueSortRecord,1024> opaque{};
    std::vector<std::string> unavailable;
    bool lightmap_matches{};
    // These are native bank identities. They are never borrowed reference-RAM
    // addresses, and no host pointer is narrowed to32 bits.
    static constexpr std::uint32_t main_arena_token=0x71000000,
        light_arena_token=0x71001000,instance_token=0x71002000;

    Impl(const Cache& c,Mesh& mesh,std::size_t index,std::span<const std::uint32_t> ids,std::span<const std::uint32_t> model_ids,
        std::shared_ptr<pstencil::SharedLightMaterials> shared)
      :cache(c),mesh_owner(mesh),pstencil_materials(c,mesh),model_materials(shared?std::move(shared):std::make_shared<pstencil::SharedLightMaterials>(c,mesh)),
       bsp_index(index),bsp(c.load_source_bsp_admission(index)),
       store(std::shared_ptr<const Cache>(&c,[](const Cache*){}),{"shad","stem","spas","vrtx"}),metadata(store.providers()),
       clusters(bsp.clusters.size()),definitions(bsp.definitions.size()),instances(bsp.instances.size()),
       transforms(bsp.instances.size()),palettes(bsp.instances.size()),
       cluster_commands(clusters.size()),definition_commands(definitions.size()) {
        const auto owned=cache.load_source_bsp_metadata(index);
        const Bytes root(owned.bytes_at(owned.source_address(),owned.size()));
        const auto read=[&](auto p,auto n){return Bytes(owned.bytes_at(p,n));};
        const auto section=[&](bool instance,unsigned i,std::uint32_t address) {
            const auto kind=instance?MenuWorldSourceSectionKind::bsp_instance_definition:MenuWorldSourceSectionKind::bsp_cluster;
            const auto m=read(address,0x6c);
            auto& target=(instance?definitions:clusters)[i];
            target.geometry_class=m.u16(0x14);target.mask_subparts=m.u16(0x24);
            if(!(instance?bsp.definitions[i].geometry_count:bsp.clusters[i].geometry_count))return;
            Section value;value.ranges=load_source_bsp_range_geometry(cache,index,kind,i);
            value.metadata=load_source_opaque_bsp_geometry_metadata(cache,index,kind,i);
            value.raw=std::make_shared<const MenuWorldSourceSection>(load_menu_world_source_bsp_section(cache,index,kind,i));
            // This is actual owned native cache backing, loaded successfully
            // before the first frame. It is not an async completion fixture.
            target.resident=true;
            auto& command=(instance?definition_commands:cluster_commands)[i];
            command.resident=true;command.parts=value.ranges.parts;
            sections.emplace(SectionKey{instance,i},std::move(value));
        };
        const auto cp=root.u32(16+0xa0),dp=root.u32(16+0x13c),ip=root.u32(16+0x144);
        for(unsigned i=0;i<clusters.size();++i)section(false,i,cp+i*176);
        for(unsigned i=0;i<definitions.size();++i)section(true,i,dp+i*200);
        for(unsigned i=0;i<instances.size();++i) {
            const auto raw=read(ip+i*88,88);std::memcpy(&transforms[i],raw.slice(0,52).data(),52);
            palettes[i]=load_menu_world_source_bsp_owner(cache,index,i).rows;
            instances[i].lighting_policy=raw.u16(0x56);
            instances[i].transform_address=instance_token+i*88;
            instance_definitions.push_back(bsp.instances[i].definition);
        }
        // Resolve original2D000's presence/bitmap/checksum and per-instance
        // lightmap backing from actual map metadata, rather than geometry size.
        const auto lightmaps=cache.load_bsp_lighting(index);
        lightmap_matches=lightmaps.structure_matches && lightmaps.bitmap_datum!=0xffffffffu;
        if(lightmap_matches)for(unsigned i=0;i<instances.size();++i) {
            const auto bucket=lightmaps.instance_buckets.at(i).bucket;
            instances[i].lightmap_resident=bucket>=0 && unsigned(bucket)<lightmaps.buckets.size();
        }
        source_bsp_material_visibility_initialize(visibility);
        const auto count=root.u32(16+0xa4),start=root.u32(16+0xa8);
        for(unsigned i=0;i<count;++i) {
            const auto m=read(start+i*32,32);materials.push_back({m.u32(12),std::to_integer<std::uint8_t>(m.slice(28,1)[0])});
        }
        // Literal3B8A0 capacities; each retained24-byte record is owned here.
        for(unsigned i=0;i<3;++i) {const unsigned capacity=std::array{0xa00u,0x400u,0x200u}[i];
            commands.banks[i].header[0]=capacity;commands.banks[i].storage.resize(capacity);}
        for(auto id:ids)lights.emplace(id,load_effect_light(cache,id));
        for(const auto& material:materials)if(material.shader!=0xffffffffu && !shaders.contains(material.shader)) {
            const auto& shad=metadata.definition(material.shader);
            const auto sr=Bytes(metadata.read(shad.source_address(),0x40));
            const auto stem=Bytes(metadata.read(sr.u32(0x24),4)).u32(0);
            const auto& st=metadata.definition(stem);
            const auto post=Bytes(metadata.read(Bytes(metadata.read(st.source_address(),0x60)).u32(0x5c),8));
            auto& shader=shaders[material.shader];shader.lod=cache.load_source_shader_lod_definition(material.shader);
            auto authored=cache.load_material(material.shader);
            for(unsigned lod=0;lod<post.u32(0);++lod) {
                const auto lr=Bytes(metadata.read(post.u32(4)+lod*10,10));shader.layers.push_back(lr.u32(2));
                authored.selected_shader_lod=lod;
                for(const auto& [id,definition]:lights)try {
                    EffectLightState state;state.size=1;state.brightness=1;
                    const auto uniforms=source_effect_light_view_uniforms(definition,state,{}, {1,0,0},{0,0,1},{1,1,1,1,1},1,{});
                    auto prepared=prepare_effect_light_diffuse_material(cache,authored,definition,uniforms);
                    for(unsigned pass=0;pass<prepared.size();++pass) {
                        const auto slot=unsigned(mesh.materials.size());mesh.materials.push_back(std::move(prepared[pass]));
                        slots.emplace(MaterialKey{id,material.shader,lod,pass},Slot{slot,authored,load_menu_world_source_pass(cache,material.shader,{lod,12,pass,1})});
                    }
                }catch(const FormatError& e){unavailable.push_back(std::to_string(id)+"/"+std::to_string(material.shader)+": "+e.what());}
                for(const auto& [id,definition]:lights)try {pstencil_materials.preload(authored,id,definition);}
                catch(const FormatError& e){unavailable.push_back("pStencil "+std::to_string(id)+"/"+authored.name+": "+e.what());}
            }
        }
        for(auto model:model_ids) {
            const auto tag=cache.load_source_tag_metadata(model);
            const Bytes root(tag.bytes_at(tag.source_address(),0x68));
            for(unsigned n=0;n<root.u32(0x60);++n) {
                const auto id=Bytes(tag.bytes_at(root.u32(0x64)+n*32u,32)).u32(12);
                if(id!=0xffffffffu)shader_for(id);
            }
            for(unsigned section=0;section<root.u32(0x24);++section) {
                const auto row=Bytes(tag.bytes_at(root.u32(0x28)+section*92u,92));
                if(!row.u16(4) || models.contains({model,section}))continue;
                ModelSection value;value.ranges=load_source_model_range_geometry(cache,model,section);
                value.metadata=load_source_opaque_model_geometry_metadata(cache,model,section);
                value.draw=std::make_unique<SourceModelEffectLights>(cache,mesh,model,section,ids,model_materials);
                for(const auto& gap:value.draw->unavailable())unavailable.push_back(gap);
                models.emplace(ModelKey{model,section},std::move(value));
            }
        }
    }
    const ModelSection& model(const SourceCullDrawUnit& unit)const {
        const auto at=models.find({unit.words[1],unit.words[3]>>9&511u});
        if(at==models.end())throw FormatError("selected live light model section was not preloaded");
        return at->second;
    }
    void preload_native_bsp_resources(PhysicsLiveWorldTu15& world,const SourceBspAdmission& main) {
        auto& memory=world.memory();
        const auto word=[&](std::uint32_t p){return Bytes(memory.view(p,4)).u32(0);};
        const auto root=word(0x4ecc18u);
        if(!root)throw FormatError("TU1.5 BSP render publication lacks the actual current structure");
        const auto cluster_count=word(root+0x9cu),cluster_base=word(root+0xa0u);
        const auto definition_count=word(root+0x138u),definition_base=word(root+0x13cu);
        const auto instance_count=word(root+0x140u),instance_base=word(root+0x144u);
        std::map<std::uint32_t,SectionKey> sections;
        for(const auto& row:main.clusters) {
            if(row.id>=cluster_count)throw FormatError("TU1.5 BSP render cluster escapes the current source bank");
            sections.emplace(cluster_base+row.id*0xb0u,SectionKey{false,row.id});
        }
        // Instance definitions are requested by each appended unit's449D0
        // admission (definition_resident), as2CBF0 does; zero-alpha instances
        // never request their geometry.
        for(const auto& row:main.instances) {
            if(row.id>=instance_count)throw FormatError("TU1.5 BSP render instance escapes the current source bank");
            const auto definition=std::bit_cast<std::int16_t>(Bytes(memory.view(instance_base+row.id*0x58u+0x34u,2)).u16(0));
            if(definition<0 || std::uint32_t(definition)>=definition_count)
                throw FormatError("TU1.5 BSP render instance lacks its actual authored definition");
        }
        for(const auto& [section,key]:sections) {
            //449D0 selects the literal BSP+28 resource header and returns
            // its completed+50 geometry pointer. Use the same whole130980
            // request/fixup owner as native model publication; never set it.
            if(!Bytes(memory.view(section+0x24u,2)).u16(0))continue;
            const auto header=section+0x28u;
            auto ready=world.request_render_geometry(2u,header);
            if(!ready)ready=world.request_render_geometry(0u,header);
            if(ready && word(section+0x50u))continue;
            // A failed request makes449D0 return NULL, so2CBF0 drops this
            // frame's units for the section rather than drawing it.
            unavailable_sections.insert(key);
            if(reported_unavailable.insert(key).second)
                std::cerr<<"TU1.5 BSP section not resident this frame: "<<(key.first?"definition ":"cluster ")<<
                    key.second<<" resource=0x"<<std::hex<<word(header)<<std::dec<<'\n';
        }
    }
    // Original449D0 for an appended instance unit: request flags3, then the
    // published geometry. A failed request drops the unit for this frame.
    bool definition_resident(PhysicsLiveWorldTu15& world,unsigned definition,bool request) {
        const auto& memory=world.memory();
        const auto word=[&](std::uint32_t p){return Bytes(memory.view(p,4)).u32(0);};
        const auto root=word(0x4ecc18u);
        if(!root || definition>=word(root+0x138u))
            throw FormatError("TU1.5 BSP instance definition escapes the current source bank");
        const auto section=word(root+0x13cu)+definition*0xc8u;
        if(request && !world.request_render_geometry(3u,section+0x28u)) {
            if(reported_unavailable.insert({true,definition}).second)
                std::cerr<<"TU1.5 BSP section not resident this frame: definition "<<definition<<'\n';
            return false;
        }
        return definitions.at(definition).resident && word(section+0x50u);
    }
    // Clusters whose130980 request failed for the current draw.
    std::set<SectionKey> unavailable_sections,reported_unavailable;
    // Census of translated main40E80 stores: address → (hits, max size). The
    // observer sees each store before it lands; it changes nothing. Ranges
    // are reported coalesced so the native oracle's snapshot set is exact.
    struct MainCommandsCensus {
        std::map<std::uint32_t,std::pair<std::uint64_t,std::uint32_t>> stores;
        std::uint64_t views{},total{};
    } commands_census;
    // Oracle foundation for native main40E80: hash the closed observable
    // write set before/after the translated call and prove every observed
    // store lands inside it, the emulated stack, or the physical namespace.
    struct MainCommandsOracle {
        std::uint64_t views{},in_set{},stack{},physical{},outside{};
        std::map<std::uint32_t,std::uint64_t> outside_addresses;
        std::uint64_t digest_sum{};
    } commands_oracle;
    struct CommandBankSet {
        static constexpr std::uint32_t headers=0x4ce268u,header_stride=0x18u,header_bytes=0x48u;
        static constexpr std::array<std::uint32_t,2> words[]{{0x486040u,8},{0x4c2aa0u,8},{0x4d4f60u,64},{0x515f1cu,4}};
        std::array<std::pair<std::uint32_t,std::uint32_t>,3> storage{}; // data, bytes (capacity×24)
        static CommandBankSet read(const SourceRuntimeMemory& memory) {
            CommandBankSet set;
            for(unsigned b=0;b<3;++b) {
                const auto h=headers+b*header_stride;
                const auto capacity=Bytes(memory.view(h,4)).u32(0),data=Bytes(memory.view(h+0x14u,4)).u32(0);
                set.storage[b]={data,capacity*24u};
            }
            return set;
        }
        bool contains(std::uint32_t address,std::size_t bytes)const {
            const auto end=std::uint64_t(address)+bytes;
            const auto inside=[&](std::uint32_t first,std::uint64_t size){return address>=first && end<=std::uint64_t(first)+size;};
            if(inside(headers,header_bytes))return true;
            for(const auto& [first,size]:words)if(inside(first,size))return true;
            for(const auto& [data,size]:storage)if(data && inside(data,size))return true;
            return false;
        }
        std::uint64_t digest(const SourceRuntimeMemory& memory)const {
            std::uint64_t h=1469598103934665603ull;
            const auto mix=[&](std::uint32_t first,std::size_t size){for(auto byte:memory.view(first,size))h=(h^std::to_integer<std::uint8_t>(byte))*1099511628211ull;};
            mix(headers,header_bytes);
            for(const auto& [first,size]:words)mix(first,size);
            for(unsigned b=0;b<3;++b) {
                const auto count=Bytes(memory.view(headers+b*header_stride+4u,4)).u32(0);
                if(storage[b].first && count*24u<=storage[b].second)mix(storage[b].first,count*24u);
            }
            return h;
        }
    };
    void main_commands_oracle(PhysicsLiveWorldTu15& world,const std::function<void()>& translated) {
        auto& memory=world.memory();
        const auto set=CommandBankSet::read(memory);
        struct Observer {
            SourceRuntimeMemory& bank;MainCommandsOracle& oracle;const CommandBankSet& set;
            Observer(SourceRuntimeMemory& b,MainCommandsOracle& o,const CommandBankSet& s):bank(b),oracle(o),set(s) {
                source_runtime_register_write_observer(bank,this,[this](std::uint32_t address,std::size_t bytes) {
                    if(set.contains(address,bytes))++oracle.in_set;
                    else if(address>=0x600e0000u && address<0x60100000u)++oracle.stack;
                    else if(address>=0x80000000u)++oracle.physical;
                    else {++oracle.outside;++oracle.outside_addresses[address];}
                });
            }
            ~Observer(){source_runtime_unregister_write_observer(bank,this);}
        } observer{memory,commands_oracle,set};
        translated();
        commands_oracle.digest_sum+=set.digest(memory);
        if(++commands_oracle.views%240)return;
        std::cerr<<"Main commands oracle: views="<<commands_oracle.views<<" in_set="<<commands_oracle.in_set<<
            " stack="<<commands_oracle.stack<<" physical="<<commands_oracle.physical<<" outside="<<commands_oracle.outside<<
            " digest_sum="<<std::hex<<commands_oracle.digest_sum<<std::dec;
        unsigned shown=0;for(const auto& [address,hits]:commands_oracle.outside_addresses){if(shown++==8)break;std::cerr<<" ["<<std::hex<<address<<std::dec<<"x"<<hits<<"]";}
        std::cerr<<'\n';
    }
    // Native main40E80 for accelerated BSP units. The existing native
    //44AC0→45CE0→460D0→41040→40E30→3B9D0 kernel builds their command records
    // from the live draw units, masks, coverage and shader tables. Those units
    // are hidden from translated40E80 (flag1 cleared, as a culled-out unit),
    // which still produces model, special and object records and performs the
    // reset. The two record streams merge by the original ordering key: word3
    // token=(coverage<<16|ordinal)<<8|part, emitted by44AC0 in ascending
    // (ordinal,part). Header count and layers follow3B9D0 exactly.
    struct MainCommandsStats {std::uint64_t views{},secondary_views{},native_units{},native_model_units{},native_records{},translated_units{},model_units{},model_unresident{},fallbacks{},mismatches{},guard_fallbacks{},hook_calls{};std::map<std::array<std::uint32_t,3>,std::uint64_t> calls;} main_commands_stats;
    void main_commands_native(PhysicsLiveWorldTu15& world,unsigned players,std::array<std::int16_t,4> viewport,
        bool view_modifier,bool oracle,const std::function<void()>& translated,std::uint32_t selector=0) {
        constexpr std::uint32_t units_at=0x4c6958u,headers=CommandBankSet::headers;
        auto& memory=world.memory();
        const auto word=[&](std::uint32_t p){return Bytes(memory.view(p,4)).u32(0);};
        const auto draw_count=word(0x4cd398u),main_prefix=word(0x4cd39cu);
        //40E80 takes its mode from the current draw context's +2A78 word:
        // main context556E18 is mode0 over [0,main_prefix); the secondary
        // context5598F8 the shadow roots install is a nonzero mode over
        // [main_prefix,draw_count). EAX is the layer selector40E90 maps to
        // the unit flag mask. Banks reset by the translated remainder.
        const auto context_address=word(0x4cd3a0u);
        {
            // Diagnostic: every (context, mode word, selector) this kernel is asked for.
            const auto key=std::array{context_address,context_address>=0x400000u && context_address<0x10000000u?word(context_address+0x2a78u):0xffffffffu,selector};
            ++main_commands_stats.calls[key];
        }
        if(draw_count>850 || main_prefix>draw_count || (context_address!=0x556e18u && context_address!=0x5598f8u)){++main_commands_stats.guard_fallbacks;translated();return;}
        const auto mode=word(context_address+0x2a78u);
        if(mode>3u){++main_commands_stats.guard_fallbacks;translated();return;}
        if(mode)++main_commands_stats.secondary_views;
        const unsigned first_unit=mode?main_prefix:0u,end_unit=mode?draw_count:main_prefix;
        std::vector<SourceCullDrawUnit> units(draw_count);
        if(draw_count)std::memcpy(units.data(),memory.view(units_at,draw_count*32u).data(),draw_count*32u);
        // Eligible: culled-in (flag1), not special (flag400), and either
        // resident BSP geometry this owner resolved, or a model unit whose
        //449D0 request (flags3 on the section's +0x38 header, through the
        // native request owner) succeeds. A failed request drops the unit for
        // the frame exactly as449D0 returning NULL does, so it stays translated.
        std::vector<unsigned> eligible;std::uint64_t remaining{};
        std::map<std::array<std::uint32_t,8>,SourceModelCommandGeometry> live_models;
        for(unsigned n=first_unit;n<end_unit;++n) {
            const auto& u=units[n].words;
            if(!(u[0]&1u) || (u[0]&0x400u)){continue;}
            if(u[1]!=0xffffffffu) {
                ++main_commands_stats.model_units;
                bool ready=false;
                try {
                    const auto header=source_live_model_section_header_tu15(world,units[n]);
                    ready=world.request_render_geometry(3u,header);
                    if(ready) {
                        auto geometry=source_live_model_command_geometry_tu15(world,units[n]);
                        ready=geometry.geometry.resident;
                        if(ready)live_models.emplace(u,std::move(geometry));
                    }
                } catch(const std::exception&){ready=false;}
                if(!ready){++remaining;++main_commands_stats.model_unresident;continue;}
                eligible.push_back(n);++main_commands_stats.native_model_units;continue;
            }
            const SourceBspCommandGeometry* geometry=nullptr;
            if(u[0]&0x1000u){const auto instance=u[3]>>18&2047u;
                if(instance<instance_definitions.size()){const auto d=instance_definitions[instance];if(d>=0 && unsigned(d)<definition_commands.size())geometry=&definition_commands[unsigned(d)];}}
            else {const auto cluster=u[3]>>9&511u;if(cluster<cluster_commands.size())geometry=&cluster_commands[cluster];}
            if(!geometry || !geometry->resident){++remaining;continue;}
            eligible.push_back(n);
        }
        ++main_commands_stats.views;
        if(eligible.empty()){main_commands_stats.translated_units+=remaining;translated();return;}
        // Live inputs the translated kernel reads: coverage, override slots,
        // overrides, mask arena, and the three bank headers' capacities.
        std::array<std::uint8_t,850> coverage{},override_slots{};std::array<std::uint32_t,192> overrides{};
        std::memcpy(coverage.data(),memory.view(0x4cd3a4u,draw_count).data(),draw_count);
        std::memcpy(override_slots.data(),memory.view(0x4cd6f6u,draw_count).data(),draw_count);
        const auto override_count=std::min<std::uint32_t>(word(0x4cdd48u),192u);
        if(override_count)std::memcpy(overrides.data(),memory.view(0x4cda48u,override_count*4u).data(),override_count*4u);
        SourceBspRangeArena arena;
        std::memcpy(arena.words.data(),memory.view(context_address+0x25cu,sizeof(arena.words)).data(),sizeof(arena.words));
        SourceBspPartCommandState native;
        for(unsigned b=0;b<3;++b) {
            const auto h=headers+b*CommandBankSet::header_stride;
            native.banks[b].header[0]=word(h);native.banks[b].storage.resize(native.banks[b].header[0]);
        }
        SourceBspPartCommandContext context;
        context.mode=mode;context.flag_mask=!selector?0xffffffffu:selector==2u?0x10u:0x20u;context.group=31;
        context.callback_geometry=0x4cbe0u;context.callback_light=0x4d0a0u; // TU1.5 image.
        context.main_prefix=main_prefix;context.draw_count=draw_count;context.mask_arena_address=context_address+0x25cu;
        context.lod_view=source_shader_lod_view(viewport[0],viewport[2],false,false);
        context.player_multiplier=source_shader_lod_player_multiplier(players);context.view_modifier=view_modifier;
        context.coverage=std::span<const std::uint8_t>(coverage).first(draw_count);
        context.override_slots=std::span<const std::uint8_t>(override_slots).first(draw_count);
        context.overrides=std::span<const std::uint32_t>(overrides).first(override_count);
        SourceBspPartCommandWorld world_inputs{cluster_commands,definition_commands,instance_definitions,materials,
            visibility,std::int16_t(bsp_index),[&](auto datum)->const SourceBspCommandShader&{return shaders.at(datum);}};
        world_inputs.model=[&](const SourceCullDrawUnit& unit)->const SourceModelCommandGeometry& {
            const auto at=live_models.find(unit.words);
            if(at==live_models.end())throw FormatError("native main commands consumed an unselected model unit");
            return at->second;
        };
        world_inputs.shader=[&](auto datum)->const SourceBspCommandShader&{return shader_for(datum);};
        // Only eligible units are visible to the native pass.
        auto native_units=units;
        for(unsigned n=first_unit;n<end_unit;++n)
            if(std::find(eligible.begin(),eligible.end(),n)==eligible.end())native_units[n].words[0]&=~1u;
        try{source_bsp_append_part_commands(native,native_units,arena,world_inputs,context);}
        catch(const std::exception& error) {
            if(!main_commands_stats.fallbacks++)std::cerr<<"Native main commands fell back to TU1.5: "<<error.what()<<'\n';
            translated();return;
        }
        main_commands_stats.native_units+=eligible.size();main_commands_stats.translated_units+=remaining;
        for(const auto& bank:native.banks)main_commands_stats.native_records+=bank.header[1];
        const auto write=[&](std::uint32_t address,std::span<const std::byte> bytes){world.write_source_call_input(address,bytes);};
        const auto write_word=[&](std::uint32_t address,std::uint32_t value){write(address,std::as_bytes(std::span(&value,1)));};
        // Merge: read a live bank's records, insert native records by token.
        const auto merge=[&](unsigned b,bool apply) {
            const auto h=headers+b*CommandBankSet::header_stride;
            const auto capacity=word(h),count=word(h+4u),data=word(h+0x14u);
            const auto& mine=native.banks[b];
            if(!mine.header[1])return true;
            if(!data || count>capacity)return false;
            std::vector<SourcePartCommand> live(count);
            if(count)std::memcpy(live.data(),memory.view(data,count*24u).data(),count*24u);
            // 3B9D0 refuses past capacity; the merged stream must too, in the
            // original emission order, so recount as if emitted together.
            std::vector<SourcePartCommand> merged;merged.reserve(count+mine.header[1]);
            std::size_t a=0,n=0;std::uint32_t layers=0;
            const auto token=[](const SourcePartCommand& c){return c.words[3];};
            while(a<live.size() || n<mine.header[1]) {
                const bool take_native=n<mine.header[1] && (a>=live.size() || token(mine.storage[n])<token(live[a]));
                const auto& next=take_native?mine.storage[n++]:live[a++];
                if(merged.size()>=capacity)break;
                merged.push_back(next);layers|=next.words[4];
            }
            if(!apply)return true;
            write(data,std::as_bytes(std::span(merged)));
            write_word(h+4u,std::uint32_t(merged.size()));write_word(h+8u,word(h+8u)|layers);
            return true;
        };
        if(oracle) {
            // Translated alone is the reference; the native pass over all
            // eligible units must reproduce exactly the records it emitted for
            // them. Compare per bank the subsequence of records whose token
            // ordinal names an eligible unit.
            translated();
            for(unsigned b=0;b<3;++b) {
                const auto h=headers+b*CommandBankSet::header_stride;
                const auto count=word(h+4u),data=word(h+0x14u);
                std::vector<SourcePartCommand> live(count);
                if(count && data)std::memcpy(live.data(),memory.view(data,count*24u).data(),count*24u);
                std::vector<SourcePartCommand> reference;
                for(const auto& c:live) {
                    const auto ordinal=(c.words[3]>>8)&65535u;
                    if(std::find(eligible.begin(),eligible.end(),ordinal)!=eligible.end())reference.push_back(c);
                }
                const auto& mine=native.banks[b];
                const auto same=reference.size()==mine.header[1] &&
                    std::equal(reference.begin(),reference.end(),mine.storage.begin(),[](const auto& x,const auto& y){
                        // Word5 low16 is retained storage3B9D0 does not write.
                        return x.words[0]==y.words[0] && x.words[1]==y.words[1] && x.words[2]==y.words[2] &&
                               x.words[3]==y.words[3] && x.words[4]==y.words[4] && (x.words[5]&0xffff0000u)==(y.words[5]&0xffff0000u);});
                if(!same && main_commands_stats.mismatches++<6) {
                    std::cerr<<"Native main commands bank"<<b<<" differs: reference="<<reference.size()<<" native="<<mine.header[1];
                    for(std::size_t k=0;k<std::min<std::size_t>(reference.size(),mine.header[1]);++k)
                        if(reference[k].words!=mine.storage[k].words){std::cerr<<" first at "<<k<<std::hex<<" ref=";for(auto w:reference[k].words)std::cerr<<w<<",";std::cerr<<" nat=";for(auto w:mine.storage[k].words)std::cerr<<w<<",";std::cerr<<std::dec;break;}
                    std::cerr<<'\n';
                }
            }
            if(main_commands_stats.views==1) {
                // Live bank headers. The slot after bank2 (0x4CE2B0) is not
                // mapped, so40E20 has exactly three banks here.
                std::cerr<<"Native main commands headers:"<<std::hex;
                for(unsigned b=0;b<3;++b){const auto h=headers+b*CommandBankSet::header_stride;
                    std::cerr<<" ["<<b<<"] cap="<<word(h)<<" count="<<word(h+4u)<<" layers="<<word(h+8u)<<" data="<<word(h+0x14u);}
                std::cerr<<std::dec<<'\n';
            }
            if(main_commands_stats.views%240==0)std::cerr<<"Native main commands oracle: views="<<main_commands_stats.views<<
                " native_units="<<main_commands_stats.native_units<<" native_records="<<main_commands_stats.native_records<<
                " translated_units="<<main_commands_stats.translated_units<<" model_units="<<main_commands_stats.model_units<<
                " native_model_units="<<main_commands_stats.native_model_units<<" model_unresident="<<main_commands_stats.model_unresident<<" mismatches="<<main_commands_stats.mismatches<<
                " fallbacks="<<main_commands_stats.fallbacks<<" secondary_views="<<main_commands_stats.secondary_views<<
                " guard_fallbacks="<<main_commands_stats.guard_fallbacks<<" hook_calls="<<main_commands_stats.hook_calls<<'\n';
            if(main_commands_stats.views%240==0){std::cerr<<"Native main commands calls:";for(const auto& [key,n]:main_commands_stats.calls)std::cerr<<" ["<<std::hex<<key[0]<<"/mode"<<key[1]<<"/sel"<<key[2]<<std::dec<<"]x"<<n;std::cerr<<'\n';}
            return;
        }
        for(auto n:eligible)write_word(units_at+n*32u,units[n].words[0]&~1u);
        translated();
        for(auto n:eligible)write_word(units_at+n*32u,units[n].words[0]);
        for(unsigned b=0;b<3;++b)if(!merge(b,true)){
            if(!main_commands_stats.fallbacks++)std::cerr<<"Native main commands could not merge bank "<<b<<'\n';
        }
    }
    // Per-entry store census for any translated per-view call, keyed by
    // entry address (HALO2_VIEW_CALL_CENSUS=1). Same coalesced report.
    std::map<std::uint32_t,MainCommandsCensus> view_call_census;
    struct ViewCallCensusReport {
        Impl& owner;
        ~ViewCallCensusReport(){for(const auto& [entry,census]:owner.view_call_census)if(census.views%240)owner.report_view_call_census(entry,census,true);}
    } view_call_census_report{*this};
    void view_call_census_run(PhysicsLiveWorldTu15& world,std::uint32_t entry,const std::function<void()>& translated) {
        auto& census=view_call_census[entry];
        auto& memory=world.memory();
        struct Observer {
            SourceRuntimeMemory& bank;MainCommandsCensus& census;
            Observer(SourceRuntimeMemory& b,MainCommandsCensus& c):bank(b),census(c) {
                source_runtime_register_write_observer(bank,this,[this](std::uint32_t address,std::size_t bytes) {
                    auto& entry=census.stores[address];++entry.first;
                    entry.second=std::max(entry.second,std::uint32_t(bytes));++census.total;
                });
            }
            ~Observer(){source_runtime_unregister_write_observer(bank,this);}
        } observer{memory,census};
        translated();
        ++census.views;
        report_view_call_census(entry,census);
    }
    void report_view_call_census(std::uint32_t entry,const MainCommandsCensus& census,bool final=false) {
        if(!final && census.views%240)return;
        std::cerr<<"View call census "<<std::hex<<entry<<std::dec<<": views="<<census.views<<" stores="<<census.total<<" ranges:";
        std::uint64_t first=0,end=0,hits=0;unsigned ranges=0;
        const auto flush=[&]{if(end){std::cerr<<" ["<<std::hex<<first<<","<<end<<")x"<<std::dec<<hits;++ranges;}};
        for(const auto& [address,e]:census.stores) {
            if(end && address<=end){end=std::max(end,std::uint64_t(address)+e.second);hits+=e.first;continue;}
            flush();first=address;end=std::uint64_t(address)+e.second;hits=e.first;
        }
        flush();std::cerr<<" ("<<ranges<<" ranges)"<<'\n';
    }
    void main_commands_census(PhysicsLiveWorldTu15& world,const std::function<void()>& translated) {
        auto& memory=world.memory();
        struct Observer {
            SourceRuntimeMemory& bank;MainCommandsCensus& census;
            Observer(SourceRuntimeMemory& b,MainCommandsCensus& c):bank(b),census(c) {
                source_runtime_register_write_observer(bank,this,[this](std::uint32_t address,std::size_t bytes) {
                    auto& entry=census.stores[address];++entry.first;
                    entry.second=std::max(entry.second,std::uint32_t(bytes));++census.total;
                });
            }
            ~Observer(){source_runtime_unregister_write_observer(bank,this);}
        } observer{memory,commands_census};
        translated();
        if(++commands_census.views%240)return;
        // Coalesce into [first,end) ranges where consecutive stores touch.
        std::cerr<<"Main commands census: views="<<commands_census.views<<" stores="<<commands_census.total<<" ranges:";
        std::uint64_t first=0,end=0,hits=0;unsigned ranges=0;
        const auto flush=[&]{if(end){std::cerr<<" ["<<std::hex<<first<<","<<end<<")x"<<std::dec<<hits;++ranges;}};
        for(const auto& [address,entry]:commands_census.stores) {
            if(end && address<=end){end=std::max(end,std::uint64_t(address)+entry.second);hits+=entry.first;continue;}
            flush();first=address;end=std::uint64_t(address)+entry.second;hits=entry.first;
        }
        flush();std::cerr<<" ("<<ranges<<" ranges)"<<'\n';
    }
    // Shader LOD/layer table for any authored shader, loaded from the cache on
    // first use. The light preload fills the few light-receiving models; main
    // mode draws every live model, so its shaders load the same way on demand.
    const SourceBspCommandShader& shader_for(std::uint32_t id) {
        if(const auto at=shaders.find(id);at!=shaders.end())return at->second;
        SourceBspCommandShader value;value.lod=cache.load_source_shader_lod_definition(id);
        const auto& shad=metadata.definition(id);
        const auto stem=Bytes(metadata.read(Bytes(metadata.read(shad.source_address(),0x28)).u32(0x24),4)).u32(0);
        const auto& st=metadata.definition(stem);
        const auto post=Bytes(metadata.read(Bytes(metadata.read(st.source_address(),0x60)).u32(0x5c),8));
        for(unsigned lod=0;lod<post.u32(0);++lod)value.layers.push_back(Bytes(metadata.read(post.u32(4)+lod*10,10)).u32(2));
        return shaders.emplace(id,std::move(value)).first->second;
    }
    SectionKey key(const SourceCullDrawUnit& unit) const {
        if(unit.words[1]!=0xffffffffu)throw FormatError("BSP light bank consumes a dynamic model owner");
        if(unit.words[0]&0x1000u) {
            const auto instance=unit.words[3]>>18&2047u;
            return {true,unsigned(bsp.instances.at(instance).definition)};
        }
        return {false,unit.words[3]>>9&511u};
    }
    struct MainCullStats {
        std::uint64_t views{},native_units{},translated_units{},fallbacks{},
            unit_mismatches{},mask_mismatches{},shared_mask_words{};
    } main_cull_stats;
    // Main mode0 44710 over the actual published draw units, arena and
    // associations. Accelerated BSP units run the native 44E80 corridor;
    // flag40 hides them from the translated call, which keeps every other
    // unit. The cull only sets flag1 and mask bits, so OR-merging the native
    // arena into the translated one reproduces the whole translated result.
    void main_cull(PhysicsLiveWorldTu15& world,const SourcePortalVisibilitySet* visibility,
        const std::function<void()>& translated) {
        static const bool oracle=[]{const auto* p=std::getenv("HALO2_MAIN_CULL_ORACLE");return p && *p;}();
        constexpr std::uint32_t context=0x556e18u,units_at=0x4c6958u,arena_at=context+0x25cu;
        const auto& memory=world.memory();
        const auto word=[&](std::uint32_t p){return Bytes(memory.view(p,4)).u32(0);};
        const auto half=[&](std::uint32_t p){return Bytes(memory.view(p,2)).u16(0);};
        const auto draw_count=word(0x4cd398u),main_prefix=word(0x4cd39cu);
        if(!visibility || word(0x4cd3a0u)!=context || word(context+0x2a78u) || word(context+0x2a7cu) ||
           draw_count>850 || main_prefix>draw_count){translated();return;}
        const auto read_units=[&] {
            std::vector<SourceCullDrawUnit> out(draw_count);
            if(draw_count)std::memcpy(out.data(),memory.view(units_at,draw_count*sizeof(SourceCullDrawUnit)).data(),
                draw_count*sizeof(SourceCullDrawUnit));
            return out;
        };
        const auto read_arena=[&] {
            SourceBspRangeArena out;
            std::memcpy(out.words.data(),memory.view(arena_at,sizeof(out.words)).data(),sizeof(out.words));
            out.used=half(context+0xa5cu);return out;
        };
        const auto before=read_units();const auto arena=read_arena();
        std::vector<std::array<std::uint32_t,4>> associations(half(context+0xa5eu));
        if(!associations.empty())std::memcpy(associations.data(),
            memory.view(context+0xa60u,associations.size()*sizeof(associations[0])).data(),associations.size()*sizeof(associations[0]));
        auto native=before;std::vector<SourceBspFrustumDrawBank> banks(draw_count);
        std::vector<unsigned> eligible;std::uint64_t remaining{};
        for(unsigned n=0;n<main_prefix;++n) {
            auto& unit=native[n];const auto flags=unit.words[0];
            if((flags&1u) || !(flags&0x40u))continue;
            const Section* section=nullptr;std::optional<Transform> transform;
            if(unit.words[1]==0xffffffffu) {
                const auto instance=unit.words[3]>>18&2047u;
                const auto found=(flags&0x1000u) && instance>=bsp.instances.size()?sections.end():sections.find(key(unit));
                if(found!=sections.end() && found->second.ranges.accelerator_count>0 &&
                   (!(flags&0x1000u) || instance<transforms.size()))section=&found->second;
                if(flags&0x1000u && section)transform=transforms[instance];
            }
            // The native corridor never visits units it does not own.
            if(!section){unit.words[0]&=~0x40u;++remaining;continue;}
            banks[n]={&section->ranges,true,transform};eligible.push_back(n);
        }
        ++main_cull_stats.views;
        if(eligible.empty()){main_cull_stats.translated_units+=remaining;translated();return;}
        auto native_arena=arena;
        try {
            source_bsp_frustum_cull(native,banks,native_arena,{main_prefix,draw_count,0,0,arena_at},
                *visibility,associations);
        } catch(const FormatError& error) {
            if(!main_cull_stats.fallbacks++)std::cerr<<"Native main cull fell back to TU1.5: "<<error.what()<<'\n';
            translated();return;
        }
        main_cull_stats.native_units+=eligible.size();main_cull_stats.translated_units+=remaining;
        const auto write=[&](std::uint32_t address,const auto& value) {
            world.write_source_call_input(address,std::as_bytes(std::span(&value,1)));
        };
        if(oracle) {
            translated();
            const auto after=read_units();const auto merged=read_arena();
            std::vector<bool> owned(arena.words.size()),shared(arena.words.size());
            const auto region=[&](const SourceCullDrawUnit& unit,std::size_t words,std::vector<bool>& marks) {
                if(!(unit.words[0]&0x40u) || !unit.words[5] || unit.words[5]<arena_at)return;
                for(std::size_t k=(unit.words[5]-arena_at)/4,end=std::min(k+words,marks.size());k<end;++k)marks[k]=true;
            };
            const auto mask_words=[](const SourceBspRangeGeometry& g){return (std::size_t(std::max(g.subpart_count,0))+31)/32;};
            for(auto n:eligible)region(before[n],mask_words(*banks[n].geometry),owned);
            for(unsigned n=0;n<main_prefix;++n) {
                if(std::find(eligible.begin(),eligible.end(),n)!=eligible.end())continue;
                std::size_t words=512;
                try {
                    if(before[n].words[1]!=0xffffffffu)words=mask_words(model(before[n]).ranges);
                    else if(const auto found=sections.find(key(before[n]));found!=sections.end())words=mask_words(found->second.ranges);
                } catch(const std::exception&) {}
                region(before[n],words,shared);
            }
            for(auto n:eligible)if(after[n].words!=native[n].words) {
                if(main_cull_stats.unit_mismatches++<8)std::cerr<<"Native main cull unit "<<n<<" flags native=0x"<<std::hex<<
                    native[n].words[0]<<" translated=0x"<<after[n].words[0]<<std::dec<<'\n';
            }
            for(std::size_t k=0;k<owned.size();++k)if(owned[k] && merged.words[k]!=native_arena.words[k]) {
                if(shared[k]){++main_cull_stats.shared_mask_words;continue;}
                if(main_cull_stats.mask_mismatches++<8)std::cerr<<"Native main cull mask word "<<k<<" native=0x"<<std::hex<<
                    native_arena.words[k]<<" translated=0x"<<merged.words[k]<<std::dec<<'\n';
            }
            if(main_cull_stats.views%240==0)std::cerr<<"Native main cull oracle: views="<<main_cull_stats.views<<
                " native_units="<<main_cull_stats.native_units<<" translated_units="<<main_cull_stats.translated_units<<
                " unit_mismatches="<<main_cull_stats.unit_mismatches<<" mask_mismatches="<<main_cull_stats.mask_mismatches<<
                " shared_mask_words="<<main_cull_stats.shared_mask_words<<" fallbacks="<<main_cull_stats.fallbacks<<'\n';
            return;
        }
        for(auto n:eligible)write(units_at+n*32u,native[n].words[0]&~0x40u);
        translated();
        for(auto n:eligible)write(units_at+n*32u,native[n].words[0]);
        const auto current=read_arena();
        for(std::size_t k=0;k<current.words.size();++k)
            if(const auto merged=current.words[k]|native_arena.words[k];merged!=current.words[k])
                write(arena_at+std::uint32_t(k*4),merged);
    }
};
SourceBspEffectLights::SourceBspEffectLights(const Cache& c,Mesh& mesh,std::size_t bsp,
    std::span<const std::uint32_t> ids,std::span<const std::uint32_t> models)
    :SourceBspEffectLights(c,mesh,bsp,ids,models,{}){}
SourceBspEffectLights::SourceBspEffectLights(const Cache& c,Mesh& mesh,std::size_t bsp,
    std::span<const std::uint32_t> ids,std::span<const std::uint32_t> models,
    std::shared_ptr<pstencil::SharedLightMaterials> shared)
    :impl_(std::make_unique<Impl>(c,mesh,bsp,ids,models,std::move(shared))){}
SourceBspEffectLights::~SourceBspEffectLights()=default;
const std::vector<std::string>& SourceBspEffectLights::unavailable() const{return impl_->unavailable;}
std::size_t SourceBspEffectLights::resident_model_sections()const noexcept {return impl_->models.size();}
bool SourceBspEffectLights::resident_model_section(std::uint32_t model,unsigned section)const noexcept {
    return impl_->models.contains({model,section});
}
const Cache &SourceBspEffectLights::resident_cache()const noexcept {return impl_->cache;}
const Mesh &SourceBspEffectLights::resident_mesh()const noexcept {return impl_->mesh_owner;}
std::vector<EffectLightRawDraw> SourceBspEffectLights::draw(const SourceEffectLightView& initial_lights,
    std::span<const WorldEffectLightReceiver> initial_receivers,const SourceBspAdmission& main,
    const SourceViewProjectionBank& camera,const BspFogSample& fog,Vec3 eye,
    std::array<std::int16_t,4> viewport,std::array<std::uint32_t,2> surface,unsigned players,
    SourceEffectLightModelViewTu15* objects,std::vector<pstencil::PointCaster>* point_casters,
    std::uint32_t light_domain,const std::array<ShaderVector,12>* submitting_constants) {
    auto& i=*impl_;std::vector<EffectLightRawDraw> output;
    i.unavailable_sections.clear();
    if(objects)i.preload_native_bsp_resources(objects->world,main);
    if(initial_receivers.empty() && (!objects ||
        (!objects->main_draws && !objects->main_collected && !objects->current_lights)))return output;
    std::optional<SourceBspBreakableVisibilityTu15> current_visibility;
    if(objects)current_visibility=source_bsp_breakable_visibility_tu15(objects->world.memory());
    const auto coverage=source_shader_coverage_camera(camera);
    SourceBspDrawWorld draw_world{i.bsp,i.clusters,i.definitions,i.instances,i.lightmap_matches};
    // The collector's449E0 admission follows this frame's actual requests.
    std::vector<SourceBspDrawGeometry> resident_clusters,resident_definitions;
    if(!i.unavailable_sections.empty()) {
        resident_clusters=i.clusters;resident_definitions=i.definitions;
        for(const auto& [instance,section]:i.unavailable_sections)
            (instance?resident_definitions:resident_clusters).at(section).resident=false;
        draw_world.clusters=resident_clusters;draw_world.definitions=resident_definitions;
    }
    if(objects)draw_world.definition_resident=[&](unsigned definition,bool request) {
        return i.definition_resident(objects->world,definition,request);
    };
    auto admission=main;
    const bool completed_main=objects && objects->completed_campaign_main;
    if(!completed_main) {
        source_bsp_range_arena_begin(i.main_arena);i.units.arena=i.main_arena;
        source_bsp_collect_draw_units(i.units,admission,draw_world,{0,Impl::main_arena_token,0,fog,coverage});
    }
    const auto object_context=[&](const SourceBspDrawContext& c) {
        auto out=objects->context;
        out.entry.mode=out.draw.draw.mode=c.mode;
        if(c.mode)out.built={}; // A secondary query cannot replace the base selection.
        out.draw.draw.mask_arena_address=c.mask_arena_address;
        out.entry.eye=out.draw.eye=eye;out.draw.viewport=viewport;out.draw.rendering=camera;
        out.entry.coverage=coverage;out.local_players=players;
        out.entry.flags={std::uint8_t(c.override_byte),std::int32_t(fog.classification),fog.planar_datum,
            fog.eye_fraction,fog.planar_only,std::uint8_t(fog.planar_flags)};
        return out;
    };
    if(completed_main) {
        if(!objects->completed_campaign_main->main_commands_completed ||
           &main!=&objects->completed_campaign_main->bsp)
            throw FormatError("Campaign light draw requires its actual completed main frame");
        const auto retained=source_live_effect_light_state_tu15(objects->world.memory());
        if(retained.current_context!=0x556e18u)
            throw FormatError("Campaign light draw no longer owns its completed main context");
        i.units=retained.state;i.units.arena=retained.main_arena;
        if(objects->main_draws)objects->main_draws(i.units);
        if(objects->main_collected)objects->main_collected(admission,objects->main,objects->context);
    } else if(objects) {
        // Renderer BSP activation is separate from each view's collection.
        // Preserve the original source-populated plane hash across frames.
        if(!i.shadow_storage)i.shadow_storage=source_live_shadow_storage_activate_tu15(objects->world);
        else i.shadow_storage=source_live_shadow_storage_rebind_tu15(objects->world,*i.shadow_storage);
        const auto context=object_context({0,Impl::main_arena_token,0,fog,coverage});
        //134C42 selects556E18 for the ordinary main context. Publish the
        // same current owner before the native original model builder runs.
        const std::uint32_t main_context=0x556e18u;
        objects->world.write_source_call_input(0x4cd3a0u,std::as_bytes(std::span(&main_context,1)));
        source_live_shadow_main_inputs_tu15(objects->world,context);
        //4C3AE0 is the optional render-object override consumed by3D2B0.
        // Original127FE initializes it to NONE in the D3D startup owner.
        // This native renderer supplies its actual configured override as a
        // caller input, separately from original device initialization.
        if(objects->first_person_object_override!=0xffffffffu)
            (void)objects->world.source_object_body(objects->first_person_object_override);
        objects->world.write_source_call_input(0x4c3ae0u,
            std::as_bytes(std::span(&objects->first_person_object_override,1)));
        // Original2BABC completes the actual per-view FP cache before2BAC6
        // updates light fades and before the main model collector consumes it.
        //3D2B0 owns every mode/unit/count decision and calls complete168DED.
        auto first_person=objects->world.prepare_source_call();
        if(objects->view_calls)objects->view_calls(true,0x3d2b0u,first_person,objects->world.memory());
        static const bool view_census=[]{const auto* p=std::getenv("HALO2_VIEW_CALL_CENSUS");return p && *p;}();
        if(view_census)i.view_call_census_run(objects->world,0x3d2b0u,[&]{objects->world.run_actual(0x3d2b0u,first_person);});
        else objects->world.run_actual(0x3d2b0u,first_person);
        if(objects->view_calls)objects->view_calls(false,0x3d2b0u,first_person,objects->world.memory());
        // Actual2BAC6 completes native per-view light fades before main
        // collection. It consumes the current eye, light pool/authored tables;
        //2C470/1BE90 above own main policy/LOD.
        auto light_view=objects->world.prepare_source_call();
        if(objects->view_calls)objects->view_calls(true,0xc35d0u,light_view,objects->world.memory());
        if(view_census)i.view_call_census_run(objects->world,0xc35d0u,[&]{objects->world.run_actual(0xc35d0u,light_view);});
        else objects->world.run_actual(0xc35d0u,light_view);
        if(objects->view_calls)objects->view_calls(false,0xc35d0u,light_view,objects->world.memory());
        source_live_model_draws_tu15(objects->world,objects->main,i.units,context);
        if(objects->main_draws)objects->main_draws(i.units);
        const auto main_handoff=source_live_shadow_main_draws_tu15(objects->world.memory(),i.units,admission,
            objects->main.associations,Impl::main_arena_token,Impl::instance_token,i.transforms,
            [&](auto address,auto bytes){objects->world.write_source_call_input(address,bytes);});
        (void)main_handoff; // Pre-cull state; native44710 integration follows.

        // The auxiliary main view in134C00 consumes these actual completed
        // queues, independently of the draw-unit handoff above. Preserve all
        // four columns so secondary model queries retain color admission.
        source_live_shadow_main_queues_tu15(objects->world,admission,objects->main);

        // Original335D0 follows ordinary draw preparation and precedes
        //31C80's secondary light queries. Retain the same completed queues.
        static const bool reference_cull=[]{const auto* p=std::getenv("HALO2_REFERENCE_MAIN_CULL");return p && *p;}();
        struct MainCullScope {
            SourceEffectLightModelViewTu15& objects;
            ~MainCullScope(){objects.main_cull=nullptr;}
        } main_cull_scope{*objects};
        if(!reference_cull)objects->main_cull=[&](const std::function<void()>& translated) {
            i.main_cull(objects->world,objects->visibility,translated);
        };
        static const bool commands_census=[]{const auto* p=std::getenv("HALO2_MAIN_COMMANDS_CENSUS");return p && *p;}();
        struct MainCommandsScope {
            SourceEffectLightModelViewTu15& objects;
            ~MainCommandsScope(){objects.main_commands=nullptr;objects.secondary_commands=nullptr;}
        } main_commands_scope{*objects};
        static const bool commands_oracle=[]{const auto* p=std::getenv("HALO2_MAIN_COMMANDS_ORACLE");return p && *p;}();
        static const bool commands_containment=[]{const auto* p=std::getenv("HALO2_MAIN_COMMANDS_CONTAINMENT");return p && *p;}();
        static const bool reference_commands=[]{const auto* p=std::getenv("HALO2_REFERENCE_MAIN_COMMANDS");return p && *p;}();
        if(commands_census)objects->main_commands=[&](const std::function<void()>& translated) {
            i.main_commands_census(objects->world,translated);
        };
        else if(commands_containment)objects->main_commands=[&](const std::function<void()>& translated) {
            i.main_commands_oracle(objects->world,translated);
        };
        else if(!reference_commands)objects->main_commands=[&](const std::function<void()>& translated) {
            i.main_commands_native(objects->world,players,viewport,objects->context.view_modifier,commands_oracle,translated);
        };
        // The shadow roots' secondary-context40E80 calls can take the same
        // kernel (HALO2_NATIVE_SHADOW_COMMANDS=1). No measured workload has
        // reached that branch yet (hook_calls stayed0), so it is not the
        // default until the record oracle has seen it on live data.
        static const bool native_shadow_commands=[]{const auto* p=std::getenv("HALO2_NATIVE_SHADOW_COMMANDS");return p && *p;}();
        if(native_shadow_commands && !commands_census && !commands_containment && !reference_commands)
            objects->secondary_commands=[&](std::uint32_t selector,const std::function<void()>& translated) {
                ++i.main_commands_stats.hook_calls;
                i.main_commands_native(objects->world,players,viewport,objects->context.view_modifier,commands_oracle,translated,selector);
            };
        if(objects->main_collected)objects->main_collected(admission,objects->main,context);
    }
    i.main_arena=i.units.arena;
    std::optional<std::pair<SourceEffectLightView,std::vector<WorldEffectLightReceiver>>> current_lights;
    if(objects && objects->current_lights)current_lights=objects->current_lights();
    const auto& lights=current_lights?current_lights->first:initial_lights;
    const auto receivers=current_lights?std::span<const WorldEffectLightReceiver>(current_lights->second):initial_receivers;
    SourceBspPartCommandWorld command_world{i.cluster_commands,i.definition_commands,i.instance_definitions,
        i.materials,current_visibility?std::span<const std::byte>(*current_visibility):std::span<const std::byte>(i.visibility),
        std::int16_t(i.bsp_index),[&](auto datum)->const SourceBspCommandShader&{return i.shaders.at(datum);}};
    MenuWorldSourceConstants constants{};
    const auto bank=submitting_constants?*submitting_constants:source_view_shader_bank(camera,1024.f,eye,
        {float(viewport[1]),float(viewport[0]),float(viewport[3]-viewport[1]),float(viewport[2]-viewport[0])},false);
    std::copy(bank.begin(),bank.end(),constants.begin());
    for(const auto& receiver:receivers) {
        i.commands.calls.clear();
        const auto light=std::find_if(lights.records.begin(),lights.records.end(),[&](const auto& r){return r.runtime.identity==receiver.identity;});
        if(light==lights.records.end() || !light->definition)throw FormatError("BSP light receiver lost its live light");
        const auto& runtime=light->runtime;
        const auto uniforms=source_effect_light_view_uniforms(*light->definition,runtime.state,runtime.position,
            runtime.forward,runtime.up,light->fades,receiver.selected_fade,{});
        auto query=receiver.view.bsp;
        SourceObjectAdmission object_receivers;
        SourceEffectLightGeometryFrameContext context;
        context.draw={receiver.view.pass_mode,Impl::light_arena_token,std::int8_t(receiver.view.flags),fog,coverage};
        context.cull.query_mode=receiver.view.query_mode;context.cull.center=runtime.spatial_center;context.cull.radius=runtime.spatial_radius;
        context.commands.player_multiplier=source_shader_lod_player_multiplier(players);
        context.commands.view_modifier=objects && objects->context.view_modifier;
        context.commands.lod_view=source_shader_lod_view(viewport[0],viewport[2],false,false);
        const auto observe=[&](SourceEffectLightCommandPhaseTu15 phase,const SourceDrawUnitState& state,const SourceBspAdmission& bsp) {
            if(objects && objects->light_command_observer)objects->light_command_observer({phase,objects->world,
                receiver.identity,state,bsp,object_receivers,context,Impl::main_arena_token,Impl::light_arena_token,
                Impl::instance_token,i.transforms,objects->diagnostic_native_light_seed,true});
        };
        observe(SourceEffectLightCommandPhaseTu15::before_seed,i.units,query);
        std::optional<SourceLiveEffectLightStateTu15> retained_light;
        if(objects) {
            retained_light=source_live_effect_light_state_tu15(objects->world.memory());
            i.units=retained_light->state;i.light_arena=retained_light->state.arena;
        }
        source_bsp_range_arena_begin(i.light_arena);i.units.arena=i.light_arena;
        observe(SourceEffectLightCommandPhaseTu15::before_collection,i.units,query);
        SourceEffectLightGeometryFrameWorld world{draw_world,command_world,i.metadata};
        // Both geometry families append to the same units, command banks and
        // final sort. Optional model ownership is explicit for BSP-only callers.
        std::map<std::array<std::uint32_t,8>,SourceModelCommandGeometry> live_models;
        const auto live_model=[&](const SourceCullDrawUnit& unit)->const SourceModelCommandGeometry& {
            if(!objects)throw FormatError("model light unit has no live world owner");
            auto at=live_models.find(unit.words);
            if(at==live_models.end())at=live_models.emplace(unit.words,
                source_live_model_command_geometry_tu15(objects->world,unit)).first;
            return at->second;
        };
        world.commands.model=live_model;
        world.dynamic=[&](auto& units,auto& query,const auto& draw_context) {
            if(objects) {
            SourceObjectAdmissionInput input;
            input.flags=receiver.view.flags;input.context_2a78=receiver.view.pass_mode;input.mode=receiver.view.query_mode;
            input.sphere_center=runtime.spatial_center;input.sphere_radius=runtime.spatial_radius;
            input.containing_fog=objects->containing_fog;input.auxiliary=&objects->main;
            object_receivers=objects->world.admit_render_objects(receiver.view.visibility,input,query.associations);
            query.associations=object_receivers.associations;
            source_live_model_draws_tu15(objects->world,object_receivers,units,object_context(draw_context));
            }
            // Copy caster inputs before the receiver availability gate and
            // sphere/subpart color culling. Off-camera and shadow-only parts
            // belong to this light query even without a main color draw.
            if(point_casters) {
                if(!i.pstencil_casters)i.pstencil_casters=std::make_unique<pstencil::PointCasterBank>(i.cache,i.bsp_index);
                i.pstencil_casters->collect(units,objects?&objects->world:nullptr,runtime,constants,
                    light_domain,objects?objects->model_available:std::function<bool(const SourceCullDrawUnit&)>{},*point_casters);
            }
            if(!objects)return;
            if(objects->model_available)for(unsigned n=units.main_prefix;n<units.draw_count;++n) {
                auto& unit=units.units[n];
                if(unit.words[1]!=0xffffffffu && !objects->model_available(unit))
                    unit.words[0]&=~0x1fffffu;
            }
        };
        if(objects)world.complete_commands=[&](auto& commands,const auto& state,const auto& bsp,
            const auto& command_context,auto selector) {
            if(!retained_light || command_context.mode!=1 || selector)
                throw FormatError("Live light commands lack the actual31172 secondary caller");
            const SourceLiveEffectLightBindingsTu15 bindings{Impl::light_arena_token,
                Impl::instance_token,i.transforms,std::uint32_t(context.cull.query_mode)};
            auto completed=source_live_effect_light_commands_tu15(objects->world,*retained_light,
                state,bsp,object_receivers,bindings,[&](bool before,const auto& abi,const auto& memory) {
                    if(objects->light_commands)objects->light_commands(before,receiver.identity,abi,memory);
                });
            commands=std::move(completed.legacy);
        };
        else world.object_commands=[](auto&,auto){};
        world.cull_bank=[&](auto,const auto& unit) {
            if(unit.words[1]!=0xffffffffu) {
                if(!objects)throw FormatError("light model cull lacks its live world");
                return source_live_model_cull_geometry_tu15(objects->world,unit,i.model(unit).ranges);
            }
            const auto& section=i.sections.at(i.key(unit));SourceCullDrawBanks result;
            result.geometry_resolved=true;result.geometry=source_bsp_sphere_cull_geometry(section.ranges);
            if(unit.words[0]&0x1000u)result.transform=i.transforms.at(unit.words[3]>>18&2047u);
            return result;
        };
        world.opaque_geometry=[&](unsigned ordinal) {
            const auto& unit=i.units.units.at(ordinal);
            if(unit.words[1]!=0xffffffffu){const auto& section=i.model(unit);return SourceOpaqueGeometrySelection{section.metadata,section.ranges.parts};}
            const auto& section=i.sections.at(i.key(unit));return SourceOpaqueGeometrySelection{section.metadata,section.ranges.parts};
        };
        const auto result=source_effect_light_geometry_frame(i.units,query,i.commands,i.opaque,world,context,
            [&](auto phase,const auto& state,const auto& bsp) {
                observe(phase==SourceEffectLightGeometryFramePhase::collected?SourceEffectLightCommandPhaseTu15::collected:
                    SourceEffectLightCommandPhaseTu15::culled,state,bsp);
            });
        i.light_arena=i.units.arena;
        const auto& commands=i.commands.banks[1];
        const auto selected=std::span(i.opaque).first(result.opaque_count);
        // Bind the actual raw source inputs before accepting1CAA0's range
        // continuation. A declaration/program/resource failure is not success.
        std::map<unsigned,MenuWorldSourceBinding> bindings;
        std::map<unsigned,Material> completed;
        std::map<unsigned,EffectLightRawDraw> model_bindings;
        const auto ranges=source_effect_light_ordered_ranges(commands,i.units.units,selected,i.units.arena,
            Impl::light_arena_token,[&](unsigned command,unsigned unit,unsigned vertex_class) {
                const auto& c=commands.storage.at(command).words;const auto& u=i.units.units.at(unit);
                if(u.words[1]!=0xffffffffu) {
                    if(!objects)throw FormatError("light model binding lacks its live world");
                    const auto& section=i.model(u);
                    const auto& geometry=live_model(u);
                    const auto palette=source_live_model_palette_rows_tu15(objects->world,u,geometry);
                    SourceModelEffectLightDrawInput input;
                    input.object=Bytes(objects->world.memory().view(u.words[4],4)).u32(0);
                    input.light=light->definition_identity;input.shader=c[2];input.shader_lod=c[5]>>8&255u;
                    input.pass=c[5]&255u;input.part=c[3]&255u;input.vertex_class=vertex_class;
                    input.topology=source_effect_light_part_ranges(u,section.ranges,input.part,i.units.arena,Impl::light_arena_token).topology;
                    input.palette=palette;input.uniforms=uniforms;input.constants=constants;
                    MenuWorldRasterContext raster;raster.viewport_reset(surface);raster.game_initialize_offset();
                    raster.layer_begin(MenuWorldRasterLayer::sphere_diffuse_12);
                    raster.entry_begin(menu_world_raster_entry_from_source_flags(u.words[0],false,false));
                    input.raster=raster.snapshot({MenuWorldRasterPrimitive::ordinary_triangles,MenuWorldRasterPolygon::fill,
                        std::array<std::int32_t,2>{1,1},true});raster.entry_end();
                    input.raster.window_exclusive=false;for(auto& rect:input.raster.regions)rect={0,0,0,0};
                    input.raster.regions[0]={viewport[1],viewport[0],viewport[3],viewport[2]};
                    const auto depth=menu_world_depth_clip_planes({.015625f,1024.f,MenuWorldDepthBoundary::none,0,false});
                    input.clip_range={0,16777215.f,depth.minimum_w,depth.maximum_w};input.fog_parameters={1,1};
                    model_bindings.emplace(command,section.draw->draw(input));
                    return SourceOpaqueRangeGeometry{&section.ranges,true};
                }
                const auto& section=i.sections.at(i.key(u));
                const auto slot=i.slots.find({light->definition_identity,c[2],(c[5]>>8)&255u,c[5]&255u});
                if(slot==i.slots.end())throw FormatError("Selected BSP light material was not admitted during preload");
                auto prepared=prepare_effect_light_diffuse_material(i.cache,slot->second.authored,*light->definition,uniforms);
                const auto pass_index=c[5]&255u;if(pass_index>=prepared.size())throw FormatError("Selected BSP light pass missing");
                auto& material=completed.emplace(command,std::move(prepared[pass_index])).first->second;
                const auto source_constants=source_effect_light_raw_constants(constants,material);
                const auto part=c[3]&255u;const auto topology=source_effect_light_part_ranges(u,section.ranges,part,i.units.arena,Impl::light_arena_token).topology;
                const bool instance=u.words[0]&0x1000u;
                MenuWorldSourceOwner owner{instance?MenuWorldSourceOwnerKind::bsp_instance:MenuWorldSourceOwnerKind::bsp_cluster,section.raw->owner_datum(),instance?(u.words[3]>>18&2047u):section.raw->section(),{}};
                const auto draw=instance?menu_world_source_draw(section.raw,part,topology,std::span(&i.palettes.at(owner.ordinal),1)):
                    menu_world_source_cluster_draw(section.raw,part,topology);
                bindings.emplace(command,menu_world_source_binding_class(i.cache,owner,draw,slot->second.pass.vertex_shader(),vertex_class,source_constants));
                return SourceOpaqueRangeGeometry{&section.ranges,true};
            });
        MenuWorldRasterContext raster;raster.viewport_reset(surface);raster.game_initialize_offset();raster.layer_begin(MenuWorldRasterLayer::sphere_diffuse_12);
        for(const auto& range:ranges) {
            const auto& c=commands.storage.at(range.command).words;const auto& u=i.units.units.at(range.unit);
            if(u.words[1]!=0xffffffffu) {
                const auto& bound=model_bindings.at(range.command);
                for(const auto& indices:range.selected.ranges) {
                    auto packet=bound;packet.light_identity=runtime.identity;packet.light_definition=light->definition_identity;
                    packet.point_light_position=runtime.position;
                    auto draw=bound.source.draw();draw.range=indices;
                    packet.source=menu_world_source_binding_class(i.cache,bound.source.owner(),std::move(draw),
                        bound.source.vertex_shader(),range.vertex_class,bound.source.constants());
                    output.push_back(std::move(packet));
                }
                continue;
            }
            const auto& slot=i.slots.at({light->definition_identity,c[2],(c[5]>>8)&255u,c[5]&255u});
            const auto& bound=bindings.at(range.command);const auto& completed_material=completed.at(range.command);
            raster.entry_begin(menu_world_raster_entry_from_source_flags(u.words[0],false,false));
            auto packet=raster.snapshot({MenuWorldRasterPrimitive::ordinary_triangles,MenuWorldRasterPolygon::fill,std::array<std::int32_t,2>{1,1},true});raster.entry_end();
            // The view owns the original player rectangle. The raw vertex
            // output already includes its offset; Vulkan uses the full surface.
            packet.window_exclusive=false;for(auto& rect:packet.regions)rect={0,0,0,0};
            packet.regions[0]={viewport[1],viewport[0],viewport[3],viewport[2]};
            const auto depth=menu_world_depth_clip_planes({.015625f,1024.f,MenuWorldDepthBoundary::none,0,false});
            for(const auto& indices:range.selected.ranges) {
                auto draw=bound.draw();draw.range=indices;
                auto binding=menu_world_source_binding_class(i.cache,bound.owner(),std::move(draw),bound.vertex_shader(),range.vertex_class,bound.constants());
                auto completed_draw=source_effect_light_raw_draw(slot.material,completed_material,slot.pass,std::move(binding),packet,
                    {0,16777215.f,depth.minimum_w,depth.maximum_w},{1,1});
                completed_draw.light_identity=runtime.identity;completed_draw.light_definition=light->definition_identity;
                completed_draw.point_light_position=runtime.position;
                completed_draw.pstencil_casts_shadow=pstencil::shadow_casting_part(
                    completed_draw.source.draw().geometry->parts().at(completed_draw.source.draw().part).type);
                completed_draw.pstencil_specular=i.pstencil_materials.complete(c[2],(c[5]>>8)&255u,
                    light->definition_identity,uniforms);
                output.push_back(std::move(completed_draw));
            }
        }
    }
    return output;
}
}
