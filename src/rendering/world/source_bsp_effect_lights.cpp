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
#include "halo2/native_tu15_cull_walk_44710.hpp"
#include "halo2/native_tu15_render_lighting.hpp"
#include "halo2/detail/native_tu15_arithmetic_environment.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
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
    std::weak_ptr<const void> shadow_world;
    std::uint64_t shadow_backing_revision{};
    SourceDrawUnitState units;
    SourceBspRangeArena main_arena,light_arena;
    SourceBspPartCommandState commands;
    std::array<SourceOpaqueSortRecord,1024> opaque{};
    std::vector<std::string> unavailable;
    bool lightmap_matches{};
    std::uint64_t parent_policy_queries{},parent_receiver_rows{};
    std::uint64_t flag_policy_calls{},flag_policy_bsp_rows{},flag_policy_object_rows{},flag_policy_changed{},
        flag_policy_none{},flag_policy_root{},flag_policy_f8{},flag_policy_reference{};
    std::uint64_t mode0_receivers{},mode0_entries{},mode0_model_entries{},mode0_exhausted{},mode0_reference{},mode0_reference_mismatches{};
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
        // Prepared metadata for callers without a live World. Live collectors
        // consume the original2CFE0 service at each append instead.
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
    // Publish every phase in source order. Genuine preparation/emission
    // services retain their banks; no flags are hidden and no streams merge.
    void main_commands_native(PhysicsLiveWorldTu15& world,SourcePhysicsWorldRuntimeAbi& abi) {
        const auto fp=world.run_native_command_frame_tu15(abi.gp[0]);
        abi.x87_status=fp.status;
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};
    }
    // Optional finite qualification output. The runner owns the raw file in
    // HALO2_TEST_SCRATCH and removes it; reports retain only its SHA-256/size.
    // Include every capacity slot, including all retained word5 bits, so a
    // matching emitted-record subsequence cannot hide a storage difference.
    void trace_command_banks(PhysicsLiveWorldTu15& world,std::uint32_t selector,const SourcePhysicsWorldRuntimeAbi& abi) {
        struct Output {
            std::ofstream file;
            Output(){if(const auto* path=std::getenv("HALO2_COMMAND_BANKS_PATH");path&&*path) {
                file.open(path,std::ios::binary|std::ios::trunc);
                if(!file)throw FormatError("Command bank trace cannot open its owned output");
            }}
        };
        static Output output;if(!output.file.is_open())return;
        const auto& memory=world.memory();const auto banks=CommandBankSet::read(memory);
        const auto controls=abi.floating_point.value();
        const std::array<std::uint32_t,6> header{0x40e80001u,selector,Bytes(memory.view(0x4cd3a0u,4)).u32(0),
            controls.x87_control_word,abi.x87_status,controls.mxcsr};
        const auto emit=[&](std::span<const std::byte> bytes){output.file.write(reinterpret_cast<const char*>(bytes.data()),std::streamsize(bytes.size()));};
        emit(std::as_bytes(std::span(header)));emit(memory.view(CommandBankSet::headers,CommandBankSet::header_bytes));
        for(const auto& [at,size]:CommandBankSet::words)emit(memory.view(at,size));
        for(const auto& [at,size]:banks.storage) {
            if(size>16u*1024u*1024u || (!at&&size))throw FormatError("Command bank trace lost a complete retained capacity");
            if(size)emit(memory.view(at,size));
        }
        if(!output.file)throw FormatError("Command bank trace failed to publish its owned bytes");
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
        std::uint64_t views{},native_entries{},residual_entries{},fallbacks{},
            unit_mismatches{},mask_mismatches{},fp_mismatches{};
    } main_cull_stats;
    std::vector<NativeTu15FrustumDrawCall> light_frustum_cull(PhysicsLiveWorldTu15& live,
        const SourceLiveEffectLightStateTu15& retained,SourceDrawUnitState& state,
        const SourceBspAdmission& admission,const SourceObjectAdmission& objects,
        const SourceLiveEffectLightBindingsTu15& bindings,const SourcePortalVisibilitySet& visibility,
        const SourceDrawUnitCullContext& context,
        const std::function<SourceCullDrawBanks(unsigned,const SourceCullDrawUnit&)>& resolve) {
#if !((defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER))
        return native_tu15_frustum_draw_cull_44710(state.units,state.arena,bindings.light_arena_token,
            context,visibility,admission.associations,resolve);
#else
        static const std::string reference=[] {
            const auto* p=std::getenv("HALO2_REFERENCE_LIGHT_FRUSTUM_CULL");
            return p&&*p&&*p!='0'?std::string(p):std::string{};
        }();
        if(!reference.empty() && reference!="1" && reference!="reference")
            throw FormatError("Large-sphere cull reference mode must be1 or reference");
        auto caller=live.prepare_source_call();
        const auto controls=caller.floating_point.value();
        NativeTu15PhysicsFrameFp fp{controls.x87_control_word,caller.x87_status,controls.mxcsr};
        const auto before=reference.empty()?std::optional<SourceDrawUnitState>{}:state;
        std::vector<NativeTu15FrustumDrawCall> calls;
        {
            detail::NativeTu15ArithmeticEnvironment environment(fp);
            calls=native_tu15_frustum_draw_cull_44710(state.units,state.arena,bindings.light_arena_token,
                context,visibility,admission.associations,resolve);
        }
        if(!before)return calls;
        auto& memory=live.memory();
        // Restore bytes through their existing owner. Check both the complete
        // region metadata and every committed backing identity; never restore
        // a memory checkpoint, remount a bank or replace a platform lease.
        const auto topology=[&] {
            using Region=std::tuple<std::uint32_t,unsigned,std::string,std::size_t,std::size_t,std::size_t>;
            std::vector<Region> regions;
            memory.visit_region_metadata([&](const auto& r){regions.emplace_back(r.address,unsigned(r.access),std::string(r.owner),r.bytes,r.page_bytes,r.committed_pages);});
            std::vector<std::tuple<std::uint32_t,std::size_t,const std::byte*>> backing;
            memory.visit_committed([&](auto at,auto bytes){backing.emplace_back(at,bytes.size(),bytes.data());});
            return std::pair{std::move(regions),std::move(backing)};
        };
        const auto initial_topology=topology();
        struct Journal {
            PhysicsLiveWorldTu15& live;
            struct Saved {std::uint32_t address;std::vector<std::byte> bytes;NativeRuntimeAddress backing;};
            std::vector<Saved> writes;
            bool failed{};
            void retain(std::uint32_t at,std::span<const std::byte> bytes) {
                writes.push_back({at,{bytes.begin(),bytes.end()},live.memory().native_address(at,bytes.size())});
            }
            void write(std::uint32_t at,std::span<const std::byte> bytes) {
                retain(at,live.memory().view(at,bytes.size()));live.write_source_call_input(at,bytes);
            }
            void restore() {
                if(failed)return;
                while(!writes.empty()) {
                    const auto& old=writes.back();
                    if(live.memory().native_address(old.address,old.bytes.size())!=old.backing) {
                        failed=true;throw std::runtime_error("Large-sphere reference cannot restore a replaced native lease");
                    }
                    live.write_source_call_input(old.address,old.bytes);writes.pop_back();
                }
            }
            ~Journal(){restore();}
        } journal{live};
        try {
            source_live_effect_light_handoff_tu15(memory,retained,*before,admission,objects,bindings,
                [&](auto at,auto bytes){journal.write(at,bytes);});
            constexpr std::uint32_t secondary=0x5598f8u,arena_at=secondary+0x25cu,units_at=0x4c6958u;
            const auto put=[&](std::uint32_t at,const auto& value){journal.write(at,std::as_bytes(std::span(&value,1)));};
            const auto set=Bytes(memory.view(secondary,4)).u32(0);
            if(visibility.groups.size()>6u || visibility.clusters.size()>128u || visibility.frustums.size()>512u)
                throw FormatError("Large-sphere cull reference exceeds its original visibility bank");
            put(set,std::int16_t(visibility.groups.size()));
            put(set+0xa6cu,std::uint16_t(visibility.clusters.size()));
            for(unsigned n=0;n<visibility.clusters.size();++n) {
                const auto& c=visibility.clusters[n];const auto at=set+0xa6eu+n*26u;
                put(at,c.source_cluster);put(at+2u,c.counts);put(at+14u,c.first_indices);
            }
            put(set+0x1970u,std::uint16_t(visibility.frustums.size()));
            for(unsigned n=0;n<visibility.frustums.size();++n) {
                const auto& f=visibility.frustums[n];const auto at=set+0x1974u+n*0x108u;
                put(at+0x60u,f.bounds);put(at+0x78u,f.planes);
            }
            std::array<SourceCullDrawUnit,850> projected;
            std::memcpy(projected.data(),memory.view(units_at,sizeof(projected)).data(),sizeof(projected));
            //44710/44E80 consume actual native resident geometry and poses.
            // Only typed query/draw inputs are projected; every instruction,
            // entry decision, transform, predicate and output remains genuine.
            live.run_actual(0x44710u,caller,[&](const auto& store){journal.retain(store.address,store.before);});
            ++mode0_reference;
            std::array<SourceCullDrawUnit,850> actual;
            SourceBspRangeArena arena;
            std::memcpy(actual.data(),memory.view(units_at,sizeof(actual)).data(),sizeof(actual));
            std::memcpy(arena.words.data(),memory.view(arena_at,sizeof(arena.words)).data(),sizeof(arena.words));
            arena.used=Bytes(memory.view(secondary+0xa5cu,2)).u16(0);
            for(unsigned n=0;n<actual.size();++n)projected[n].words[0]=state.units[n].words[0];
            const bool units_equal=std::memcmp(actual.data(),projected.data(),sizeof(actual))==0;
            const bool mask_equal=arena.words==state.arena.words && arena.used==state.arena.used;
            const bool fp_equal=caller.floating_point->x87_control_word==fp.control_word &&
                caller.x87_status==fp.status && caller.floating_point->mxcsr==fp.mxcsr;
            if(!units_equal || !mask_equal || !fp_equal) {
                ++mode0_reference_mismatches;
                throw FormatError("Large-sphere native44710 differs: units="+std::to_string(units_equal)+
                    " masks="+std::to_string(mask_equal)+" FP="+std::to_string(fp_equal)+
                    " native="+std::to_string(fp.control_word)+":"+std::to_string(fp.status)+":"+std::to_string(fp.mxcsr)+
                    " original="+std::to_string(caller.floating_point->x87_control_word)+":"+
                    std::to_string(caller.x87_status)+":"+std::to_string(caller.floating_point->mxcsr));
            }
            if(reference=="reference") {
                for(unsigned n=0;n<actual.size();++n)state.units[n].words[0]=actual[n].words[0];
                state.arena=arena;
            }
            journal.restore();
            if(topology()!=initial_topology)throw FormatError("Large-sphere cull reference changed a native World lease");
            // Reuse normal handoff admission without publishing its outputs:
            // verify retained draws, both arenas and every queue identity.
            (void)source_live_effect_light_handoff_tu15(memory,retained,*before,admission,objects,bindings,
                [](auto,auto){});
        } catch(...) {
            journal.restore();
            if(topology()!=initial_topology)throw FormatError("Large-sphere cull reference lost a native World lease");
            throw;
        }
        return calls;
#endif
    }
    ~Impl() {
        if(const auto* p=std::getenv("HALO2_NATIVE_LARGE_SPHERE_STATS");p&&*p)
            std::cerr<<"Sphere mode0 stats: receivers="<<mode0_receivers<<" entries="<<mode0_entries<<
                " model_entries="<<mode0_model_entries<<" exhausted="<<mode0_exhausted<<
                " reference="<<mode0_reference<<" mismatches="<<mode0_reference_mismatches<<'\n';
        if(const auto* p=std::getenv("HALO2_NATIVE_LIGHT_FLAGS_STATS");p&&*p)
            std::cerr<<"Sphere31650 stats: calls="<<flag_policy_calls<<" bsp_rows="<<flag_policy_bsp_rows<<
                " object_rows="<<flag_policy_object_rows<<" changed_objects="<<flag_policy_changed<<
                " none="<<flag_policy_none<<" root="<<flag_policy_root<<" f8="<<flag_policy_f8<<
                " reference="<<flag_policy_reference<<'\n';
        if(const auto* v=std::getenv("HALO2_NATIVE_LIGHT_PARENT_STATS");v&&*v)
            std::cerr<<"Sphere parent-biped stats: queries="<<parent_policy_queries<<" appended="<<parent_receiver_rows<<'\n';
        const auto* enabled=std::getenv("HALO2_NATIVE_CULL_WALK_STATS");
        if(!enabled || !*enabled)return;
        const auto& s=main_cull_stats;
        std::cerr<<"Native main cull: views="<<s.views<<" native_entries="<<s.native_entries<<
            " residual_entries="<<s.residual_entries<<" fallbacks="<<s.fallbacks<<
            " unit_mismatches="<<s.unit_mismatches<<" mask_mismatches="<<s.mask_mismatches<<
            " fp_mismatches="<<s.fp_mismatches<<'\n';
    }
    // The typed44710 walk interleaves native BSP entries and actual44E80
    // model/unavailable entries. Each entry publishes its mask before the
    // next one can inspect shared backing; flags are never hidden or merged.
    void main_cull(PhysicsLiveWorldTu15& world,const SourcePortalVisibilitySet* visibility,
        const std::function<void()>& translated) {
#if !((defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER))
        translated();return;
#else
        (void)translated;
        static const bool oracle=[]{const auto* p=std::getenv("HALO2_MAIN_CULL_ORACLE");return p && *p;}();
        constexpr std::uint32_t context=0x556e18u,units_at=0x4c6958u,arena_at=context+0x25cu;
        const auto& memory=world.memory();
        const auto word=[&](std::uint32_t p){return Bytes(memory.view(p,4)).u32(0);};
        const auto read_unit=[&](std::uint32_t at) {
            SourceCullDrawUnit unit;std::memcpy(&unit,memory.view(at,sizeof(unit)).data(),sizeof(unit));return unit;
        };
        const auto read_arena=[&] {
            SourceBspRangeArena out;
            std::memcpy(out.words.data(),memory.view(arena_at,sizeof(out.words)).data(),sizeof(out.words));
            out.used=Bytes(memory.view(context+0xa5cu,2)).u16(0);return out;
        };
        const auto write=[&](std::uint32_t at,const auto& v) {
            world.write_source_call_input(at,std::as_bytes(std::span(&v,1)));
        };
        bool owner=false;
        if(visibility && word(0x4cd3a0u)==context && !word(context+0x2a78u))try {
            const auto* scenario=cache.find_tag(cache.scenario_tag(),tag_groups::scenario);
            if(scenario && scenario->address && scenario->size) {
                (void)world.resident_cache_address(cache,scenario->address,scenario->size);
                const auto root=word(0x4ecc18u);
                owner=world.source_bsp_index()==bsp_index && root && word(root+0x9cu)==clusters.size() &&
                    word(root+0x138u)==definitions.size() && word(root+0x140u)==instances.size();
            }
        } catch(const FormatError&) {owner=false;}
        ++main_cull_stats.views;
        source_native_cull_walk_tu15(world,[&](auto index,auto entry,auto& arithmetic) {
            const auto residual=[&]{++main_cull_stats.residual_entries;return false;};
            if(!owner || word(0x4cd3a0u)!=context || word(context+0x2a78u))return residual();
            const auto at=units_at+32u*std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(index))));
            const auto before=read_unit(at);
            if(before.words[1]!=0xffffffffu)return residual();
            auto unit=before;auto arena=read_arena();const auto initial_arena=arena;
            auto native_fp=arithmetic;
            try {
                const auto instance=unit.words[3]>>18&2047u;
                if((unit.words[0]&0x1000u) && instance>=bsp.instances.size())return residual();
                const auto selected=key(unit);const auto found=sections.find(selected);
                if(found==sections.end() || found->second.ranges.accelerator_count<=0)return residual();
                std::optional<Transform> transform;
                if(unit.words[7]) {
                    transform.emplace();std::memcpy(&*transform,memory.view(unit.words[7],sizeof(Transform)).data(),sizeof(Transform));
                }
                detail::NativeTu15ArithmeticEnvironment environment(native_fp);
                source_bsp_frustum_cull_entry(unit,{&found->second.ranges,true,transform},arena,
                    {0,0,0,0,arena_at},*visibility,entry);
            } catch(const FormatError&) {++main_cull_stats.fallbacks;return residual();}
            ++main_cull_stats.native_entries;
            if(oracle) {
                auto actual=world.prepare_source_call(std::array{index,entry});actual.x87_status=arithmetic.status;
                actual.floating_point=SourcePhysicsFactoryFloatingPoint{arithmetic.control_word,arithmetic.mxcsr};
                world.run_actual(0x44e80u,actual);
                arithmetic={actual.floating_point->x87_control_word,actual.x87_status,actual.floating_point->mxcsr};
                const auto observed_unit=read_unit(at);const auto observed_arena=read_arena();
                const bool units_equal=observed_unit.words==unit.words,mask_equal=observed_arena.words==arena.words;
                const bool fp_equal=arithmetic.control_word==native_fp.control_word && arithmetic.status==native_fp.status && arithmetic.mxcsr==native_fp.mxcsr;
                main_cull_stats.unit_mismatches+=!units_equal;main_cull_stats.mask_mismatches+=!mask_equal;
                main_cull_stats.fp_mismatches+=!fp_equal;
                if(!units_equal || !mask_equal || !fp_equal)
                    throw FormatError("Ordered native44E80 entry differs at unit "+std::to_string(index)+" entry "+std::to_string(entry)+
                        " native FP="+std::to_string(native_fp.control_word)+":"+std::to_string(native_fp.status)+":"+std::to_string(native_fp.mxcsr)+
                        " actual FP="+std::to_string(arithmetic.control_word)+":"+std::to_string(arithmetic.status)+":"+std::to_string(arithmetic.mxcsr));
                return true; // genuine service has already published this entry
            }
            if(unit.words[0]!=before.words[0])write(at,unit.words[0]);
            for(unsigned k=0;k<arena.words.size();++k)
                if(arena.words[k]!=initial_arena.words[k])write(arena_at+4u*k,arena.words[k]);
            arithmetic=native_fp;return true;
        });
#endif
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
    if(initial_receivers.empty() && (!objects ||
        (!objects->main_draws && !objects->main_collected && !objects->current_lights)))return output;
    std::optional<SourceBspBreakableVisibilityTu15> current_visibility;
    if(objects)current_visibility=source_bsp_breakable_visibility_tu15(objects->world.memory());
    const auto coverage=source_shader_coverage_camera(camera);
    SourceBspDrawWorld draw_world{i.bsp,i.clusters,i.definitions,i.instances,i.lightmap_matches};
    NativeTu15PhysicsFrameFp admission_fp{};
    std::optional<SourceDrawUnitServices> live_admission;
    if(objects) {
        const auto abi=objects->world.prepare_source_call();
        admission_fp={abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
        live_admission.emplace(source_draw_resource_services_tu15(objects->world,admission_fp));
        draw_world.services=&*live_admission;
    }
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
        const auto revision=objects->world.authority_backing_revision();
        const auto owner=objects->world.authority_backing_owner();
        const bool same_owner=!i.shadow_world.expired() &&
            !i.shadow_world.owner_before(owner) && !owner.owner_before(i.shadow_world);
        if(!i.shadow_storage || !same_owner || i.shadow_backing_revision!=revision) {
            auto storage=source_live_shadow_storage_activate_tu15(objects->world);
            i.shadow_storage=storage;i.shadow_world=owner;i.shadow_backing_revision=revision;
        }else i.shadow_storage=source_live_shadow_storage_rebind_tu15(objects->world,*i.shadow_storage);
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
        if(commands_census)objects->main_commands=[&](SourcePhysicsWorldRuntimeAbi&,const std::function<void()>& translated) {
            i.main_commands_census(objects->world,translated);
        };
        else if(commands_containment || commands_oracle)objects->main_commands=[&](SourcePhysicsWorldRuntimeAbi&,const std::function<void()>& translated) {
            i.main_commands_oracle(objects->world,translated);
        };
        else if(!reference_commands)objects->main_commands=[&](SourcePhysicsWorldRuntimeAbi& abi,const std::function<void()>&) {
            i.main_commands_native(objects->world,abi);
        };
        // The shadow roots' secondary-context40E80 calls can take the same
        // kernel (HALO2_NATIVE_SHADOW_COMMANDS=1). Whole-World cases exercise
        // the nonzero mode; this remains opt-in pending a live shadow-root
        // workload reaching that branch.
        static const bool native_shadow_commands=[]{const auto* p=std::getenv("HALO2_NATIVE_SHADOW_COMMANDS");return p && *p;}();
        if(native_shadow_commands && !commands_census && !commands_containment && !commands_oracle && !reference_commands)
            objects->secondary_commands=[&](SourcePhysicsWorldRuntimeAbi& abi,const std::function<void()>&) {
                i.main_commands_native(objects->world,abi);
            };
        if(const auto* path=std::getenv("HALO2_COMMAND_BANKS_PATH");path&&*path) {
            auto selected=std::move(objects->main_commands);
            objects->main_commands=[&,selected=std::move(selected)](SourcePhysicsWorldRuntimeAbi& abi,const std::function<void()>& translated) {
                const auto selector=abi.gp[0];
                if(selected)selected(abi,translated);else translated();
                i.trace_command_banks(objects->world,selector,abi);
            };
        }
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
        world.visibility=&receiver.view.visibility;
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
            if(!retained_light)throw FormatError("Sphere light object admission lost its secondary queue owner");
            input.capacity=Bytes(objects->world.memory().view(retained_light->queues[1].header,4)).u32(0);
            input.flags=receiver.view.flags;input.context_2a78=receiver.view.pass_mode;input.mode=receiver.view.query_mode;
            input.sphere_center=runtime.spatial_center;input.sphere_radius=runtime.spatial_radius;
            input.containing_fog=objects->containing_fog;input.auxiliary=&objects->main;
            object_receivers=objects->world.admit_render_objects(receiver.view.visibility,input,query.associations);
            // Complete31650 follows134C00 and precedes31EC2's optional
            // parent append. BSP flags already ran before BSP draw collection.
            static const bool reference_flags=[]{const auto* p=std::getenv("HALO2_REFERENCE_LIGHT_FLAGS_31650");return p&&*p&&*p!='0';}();
            std::optional<SourceObjectAdmission> before_flags;
            if(reference_flags)before_flags=object_receivers;
            const auto owner=runtime.attached_node==-1?0xffffffffu:runtime.owner;
            const auto root=owner==0xffffffffu?0xffffffffu:objects->world.render_object_root(owner);
            const auto flags=light->definition->flags;
            const auto changes=source_effect_light_object_flags_31650(object_receivers,{flags,owner,root});
            const auto full_f8=source_effect_light_full_f8_31650(flags,std::uint16_t(query.clusters.size()));
            ++i.flag_policy_calls;i.flag_policy_object_rows+=changes.rows;i.flag_policy_changed+=changes.changed;
            i.flag_policy_none+=changes.owner_none_passes;i.flag_policy_root+=changes.root_passes;i.flag_policy_f8+=full_f8;
            if(flags&0x10000u)i.flag_policy_bsp_rows+=receiver.view.bsp.clusters.size()+receiver.view.bsp.instances.size();
            if(reference_flags) {
                if((flags&0x10000u)&&!receiver.view.bsp_before_flags)
                    throw FormatError("Sphere31650 reference lost its actual pre-edit BSP inputs");
                objects->world.reference_effect_light_flags_tu15({runtime.identity,owner,root,
                    receiver.view.bsp_before_flags?*receiver.view.bsp_before_flags:receiver.view.bsp,
                    receiver.view.bsp,*before_flags,object_receivers,full_f8});++i.flag_policy_reference;
            }else if(full_f8)objects->world.set_effect_light_full_f8_tu15(runtime.identity);
            if(light->definition->flags&0x400000u) {
                //31EC2 follows31650. A sphere's31E22 shadow byte is always0;
                // the receiver row is appended even when already admitted.
                ++i.parent_policy_queries;
                const SourceEffectLightParentBipedInput parent{light->definition->flags,input.flags,input.capacity,runtime.owner,
                    runtime.attached_node,Bytes(light->definition->bytes).u16(4)};
                i.parent_receiver_rows+=source_effect_light_append_parent_biped(object_receivers,parent,
                    [&](auto owner){return objects->world.render_light_parent_biped(owner);});
            }
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
        if(objects)world.frustum_cull=[&](auto& state,const auto& bsp,const auto& cull) {
            if(!retained_light)throw FormatError("Large-sphere cull lost its pre-collection World banks");
            const SourceLiveEffectLightBindingsTu15 bindings{Impl::light_arena_token,
                Impl::instance_token,i.transforms,0};
            return i.light_frustum_cull(objects->world,*retained_light,state,bsp,object_receivers,
                bindings,receiver.view.visibility,cull,world.cull_bank);
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
        if(receiver.view.query_mode==0) {
            ++i.mode0_receivers;i.mode0_entries+=result.frustum_calls.size();
            i.mode0_exhausted+=receiver.view.visibility.exhausted_limits!=0;
            for(const auto& call:result.frustum_calls)i.mode0_model_entries+=i.units.units.at(call.unit).words[1]!=0xffffffffu;
        }
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
