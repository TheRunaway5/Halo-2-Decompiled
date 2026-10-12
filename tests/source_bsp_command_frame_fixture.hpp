#pragma once
#include "halo2/source_effect_light_geometry_frame.hpp"
#include <cstring>
#include <map>

namespace halo2::test {
// Independent Cache producers for the joined unchanged-source fixture. Only
// persistent visibility, retained command storage and per-view scalar inputs
// come from the fixture. Units/masks/commands are produced by the native join.
struct BspCommandFrameFixture {
    const Cache& cache;
    const SourceBspAdmissionBsp& bsp;
    std::span<const Transform> transforms;
    SourceTagMetadataStore store;
    SourceTagMetadataProviders metadata;
    SourceBspPartCommandContext context;
    SourceBspPartCommandWorld commands;
    SourceBspPartCommandState state;
    std::vector<SourceOpaqueSortRecord> opaque;
    std::vector<SourceBspCommandGeometry> clusters,definitions;
    std::vector<std::int16_t> instances;
    std::vector<SourceBspCommandMaterial> materials;
    std::vector<std::byte> visibility;
    std::map<std::uint32_t,SourceBspCommandShader> shaders;
    using Key=std::pair<bool,unsigned>;
    std::map<Key,SourceBspRangeGeometry> geometry;
    std::map<Key,SourceOpaqueGeometryMetadata> selectors;

    template<class Reader>BspCommandFrameFixture(Reader& r,const Cache& c,
        const SourceBspAdmissionBsp& b,std::span<const Transform> t,
        std::span<const SourceBspDrawGeometry> cg,std::span<const SourceBspDrawGeometry> dg)
        :cache(c),bsp(b),transforms(t),
         store(std::shared_ptr<const Cache>(&c,[](const Cache*){}),{"shad","stem","spas","vrtx"}),
         metadata(store.providers()),opaque(1024),clusters(cg.size()),definitions(dg.size()) {
        commands.bsp_selector=std::bit_cast<std::int16_t>(std::uint16_t(r.word()));
        context.player_multiplier=r.real();context.view_modifier=r.word()!=0;
        for(auto& v:context.lod_view.multipliers)v=r.real();
        if(r.word()!=0)throw FormatError("joined frame fixture consumes nonempty41C20 owner");
        visibility.resize(0x4204);
        for(unsigned i=0;i<visibility.size()/4;++i){const auto w=r.word();std::memcpy(visibility.data()+i*4,&w,4);}
        for(auto& bank:state.banks) {
            for(auto& w:bank.header)w=r.word();bank.storage.resize(r.word());
            for(auto& entry:bank.storage)for(auto& w:entry.words)w=r.word();
        }
        state.special_count=std::bit_cast<std::int16_t>(std::uint16_t(r.word()));
        for(auto& u:state.special_units)for(auto& w:u.words)w=r.word();
        for(auto& record:opaque)record.words.fill(0xa5a5a5a5u);
        for(unsigned i=0;i<cg.size();++i)clusters[i].resident=cg[i].resident;
        for(unsigned i=0;i<dg.size();++i)definitions[i].resident=dg[i].resident;
        for(const auto& i:bsp.instances)instances.push_back(i.definition);
        const auto raw=cache.load_source_bsp_metadata(0);const Bytes root(raw.bytes_at(raw.source_address(),raw.size()));
        const auto count=root.u32(16+0xa4),start=root.u32(16+0xa8);
        for(unsigned i=0;i<count;++i) {
            const Bytes m(raw.bytes_at(start+i*32,32));
            materials.push_back({m.u32(12),std::to_integer<std::uint8_t>(m.slice(28,1)[0])});
        }
        commands.clusters=clusters;commands.definitions=definitions;
        commands.instance_definitions=instances;commands.materials=materials;commands.material_visibility=visibility;
        commands.shader=[this](auto datum)->const SourceBspCommandShader& {
            auto [it,inserted]=shaders.try_emplace(datum);
            if(inserted) {
                const auto read=[this](auto p,auto n){return Bytes(metadata.read(p,n));};
                const auto& shad=metadata.definition(datum);
                if(shad.group()!="shad")throw FormatError("joined frame shader group differs");
                const auto root=read(shad.source_address(),0x40);it->second.lod.bias=root.u16(0x3c);
                const auto stem_id=read(root.u32(0x24),4).u32(0);const auto& stem=metadata.definition(stem_id);
                if(stem.group()!="stem")throw FormatError("joined frame template group differs");
                const auto post=read(read(stem.source_address(),0x60).u32(0x5c),8);
                for(unsigned j=0;j<post.u32(0);++j) {
                    const auto lod=read(post.u32(4)+j*10,10);
                    it->second.lod.thresholds.push_back(lod.f32(6));it->second.layers.push_back(lod.u32(2));
                }
            }
            return it->second;
        };
    }
    Key key(const SourceCullDrawUnit& unit) const {
        const auto& u=unit.words;
        if(u[1]!=0xffffffffu)throw FormatError("joined frame needs current dynamic model geometry");
        if(u[0]&0x1000u) {
            const auto instance=u[3]>>18&2047u;
            if(instance>=bsp.instances.size() || bsp.instances[instance].definition<0)
                throw FormatError("joined frame instance definition missing");
            return {true,unsigned(bsp.instances[instance].definition)};
        }
        return {false,u[3]>>9&511u};
    }
    SourceBspRangeGeometry& section(Key key) {
        auto [it,inserted]=geometry.try_emplace(key);
        if(inserted) {
            const auto kind=key.first?MenuWorldSourceSectionKind::bsp_instance_definition:MenuWorldSourceSectionKind::bsp_cluster;
            it->second=load_source_bsp_range_geometry(cache,0,kind,key.second);
            selectors.emplace(key,load_source_opaque_bsp_geometry_metadata(cache,0,kind,key.second));
            auto& command=(key.first?definitions:clusters).at(key.second);
            if(command.resident)command.parts=it->second.parts;
        }
        return it->second;
    }
    SourceCullDrawBanks cull(const SourceCullDrawUnit& unit) {
        const auto id=key(unit);SourceCullDrawBanks result;result.geometry_resolved=true;
        result.geometry=source_bsp_sphere_cull_geometry(section(id));
        if(id.first)result.transform=transforms[unit.words[3]>>18&2047u];
        return result;
    }
    void prepare(const SourceBspAdmission& admission) {
        for(const auto& r:admission.clusters)(void)section({false,r.id});
        for(const auto& r:admission.instances) {
            const auto definition=bsp.instances.at(r.id).definition;
            if(definition<0)throw FormatError("joined frame admission definition missing");
            (void)section({true,unsigned(definition)});
        }
    }
    SourceOpaqueGeometrySelection select(const SourceCullDrawUnit& unit) {
        const auto id=key(unit);const auto& data=section(id);return {selectors.at(id),data.parts};
    }
    void append(std::vector<std::uint32_t>& out,unsigned selected) const {
        for(const auto& bank:state.banks) {
            out.insert(out.end(),bank.header.begin(),bank.header.end());out.push_back(bank.storage.size());
            for(const auto& entry:bank.storage)out.insert(out.end(),entry.words.begin(),entry.words.end());
        }
        out.push_back(std::uint16_t(state.special_count));
        for(const auto& u:state.special_units)out.insert(out.end(),u.words.begin(),u.words.end());
        out.push_back(selected);
        for(const auto& record:opaque)out.insert(out.end(),record.words.begin(),record.words.end());
    }
};
}
