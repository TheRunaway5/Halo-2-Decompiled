#include "halo2/bsp_surface_lighting.hpp"
#include "halo2/bsp_lighting.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <functional>
namespace halo2 {
namespace {

Vec3 point(const Bytes& b,std::size_t at) {
    Vec3 p{b.f32(at),b.f32(at+4),b.f32(at+8)};
    if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z))
        throw FormatError("nonfinite source render position");
    return p;
}
float axis(Vec3 v,unsigned i) {return i==0?v.x:i==1?v.y:v.z;}
Vec2 interpolate_uv(Vec2 a,Vec2 b,Vec2 c,float w_b,float w_c) {
    return {(c.x-a.x)*w_c+(b.x-a.x)*w_b+a.x,(c.y-a.y)*w_c+(b.y-a.y)*w_b+a.y};
}
Vec2 interpolate_float_uv(Vec2 a,Vec2 b,Vec2 c,float w_b,float w_c) {
    // D2DC0 FLOAT2 adds AB before AC for Y; SHORT2 retains the other order.
    return {(c.x-a.x)*w_c+(b.x-a.x)*w_b+a.x,(b.y-a.y)*w_b+(c.y-a.y)*w_c+a.y};
}
Vec3 interpolate_color(Vec3 a,Vec3 b,Vec3 c,float w_b,float w_c) {
    return {(c.x-a.x)*w_c+(b.x-a.x)*w_b+a.x,
            (c.y-a.y)*w_c+(b.y-a.y)*w_b+a.y,
            (c.z-a.z)*w_c+(b.z-a.z)*w_b+a.z};
}
bool same_attribution(const RenderSurfaceHit& a,const RenderSurfaceHit& b) {
    return a.cluster==b.cluster && a.instance==b.instance && a.geometry==b.geometry &&
        a.part==b.part && a.strip==b.strip && a.lightmap_triangle==b.lightmap_triangle &&
        a.weight_b==b.weight_b && a.weight_c==b.weight_c;
}
}
bool original_render_barycentrics(Vec3 a,Vec3 b,Vec3 c,Vec3 p,float& w_b,float& w_c) {
    // Original 11E800 SSE scalar operation order and XBE constants. Its normal
    // is AB cross AC, with axis/sign projection table at 440B94. 14AC60
    // passes the third render vertex in ECX and the second in EDX.
    const auto ab=c-a,ac=b-a,ap=p-a;
    const Vec3 n{ab.z*ac.y-ab.y*ac.z,ac.z*ab.x-ab.z*ac.x,ab.y*ac.x-ac.y*ab.x};
    constexpr float epsilon=.0001f;
    const float error=(n.y*ap.y+n.z*ap.z)+n.x*ap.x;
    if(!(error*error<((n.z*n.z+n.y*n.y)+n.x*n.x)*epsilon))return false;
    unsigned dominant;
    if(std::abs(n.y)<=std::abs(n.z) && std::abs(n.x)<=std::abs(n.z))dominant=2;
    else if(std::abs(n.x)<=std::abs(n.y))dominant=1;
    else dominant=0;
    constexpr unsigned axes[6][2]{{2,1},{1,2},{0,2},{2,0},{1,0},{0,1}};
    const auto* pair=axes[dominant*2+unsigned(axis(n,dominant)>0)];
    const auto i=pair[0],j=pair[1];
    const float numerator_c=axis(ap,j)*axis(ac,i)-axis(ac,j)*axis(ap,i);
    if(!(numerator_c>-epsilon))return false;
    const float numerator_b=axis(ab,j)*axis(ap,i)-axis(ap,j)*axis(ab,i);
    if(!(numerator_b>-epsilon))return false;
    const float denominator=axis(ab,j)*axis(ac,i)-axis(ac,j)*axis(ab,i);
    if(!((denominator<-epsilon || denominator>epsilon) &&
         numerator_b+numerator_c<=denominator+epsilon))return false;
    const float inverse=1.f/denominator;
    w_b=inverse*numerator_b;w_c=inverse*numerator_c;return true;
}
Vec3 decode_cpu_lightmap_incident(std::uint32_t packed) {
    const auto component=[](std::uint32_t word,unsigned bits,float inverse) {
        const auto sign=1u<<(bits-1),mask=(1u<<bits)-1;
        const auto s=std::int32_t((word&mask)^sign)-std::int32_t(sign);
        return float(2*s+1)*inverse;
    };
    // D31C0's midpoint conversion differs from GPU NORMPACKED3 normalization.
    return {component(packed,11,std::bit_cast<float>(0x3a001002u)),
            component(packed>>11,11,std::bit_cast<float>(0x3a001002u)),
            component(packed>>22,10,std::bit_cast<float>(0x3a802008u))};
}
std::optional<RenderSurfaceHit> BspSurfaceLighting::attribute(const CollisionSurfaceIdentity& source,
    Vec3 world_point,std::optional<std::uint32_t> original_leaf) const {
    if(source.bsp!=bsp || source.surface<0 || source.record<0)return {};
    const CollisionBank* bank{};const Geometry* geometry{};
    Vec3 local=world_point;int geometry_index=-1;
    if(source.instance>=0) {
        if(std::size_t(source.instance)>=instances.size())return {};
        const auto& instance=instances[source.instance];
        if(instance.definition!=source.record || instance.definition>=definition_collision.size() ||
           instance.definition>=definitions.size() || instance.scale==0)return {};
        bank=&definition_collision[instance.definition];geometry=&definitions[instance.definition];
        geometry_index=instance.definition;
        auto delta=world_point-instance.translation;
        if(instance.scale!=1)delta=delta*(1.f/instance.scale);
        const auto project=[&](Vec3 axis) {return (axis.z*delta.z+axis.y*delta.y)+axis.x*delta.x;};
        local={project(instance.axes[0]),project(instance.axes[1]),project(instance.axes[2])};
    } else {
        if(std::size_t(source.record)>=collision.size())return {};
        // 14AC60 uses the first authored collision BSP for static references.
        if(source.record!=0)return {};
        bank=&collision[source.record];
    }
    if(std::size_t(source.surface)>=bank->surface_leaves.size() ||
       (source.plane&0x7fff)!=(bank->surface_planes[source.surface]&0x7fff))return {};
    const auto scan=[&](std::uint32_t leaf)->std::optional<RenderSurfaceHit> {
        if(leaf>=bank->leaves.size())return {};
        const auto& l=bank->leaves[leaf];
        const Geometry* g=geometry;int section=geometry_index;
        if(source.instance<0) {
            if(l.cluster<0 || std::size_t(l.cluster)>=clusters.size())return {};
            g=&clusters[l.cluster];section=l.cluster;
        }
        for(std::uint32_t r=l.first;r<l.first+l.count;++r) {
            const auto& ref=bank->references.at(r);
            if(ref.collision_node<0 || std::size_t(ref.collision_node)>=bank->node_planes.size() ||
               bank->node_planes[ref.collision_node]!=(source.plane&0x7fff))continue;
            const auto offset=std::size_t(ref.strip);
            if(offset+2>=g->indices.size())continue;
            const auto a=g->indices[offset],b=g->indices[offset+1],c=g->indices[offset+2];
            if(a>=g->positions.size()||b>=g->positions.size()||c>=g->positions.size())continue;
            float w_b{},w_c{};
            if(!original_render_barycentrics(g->positions[a],g->positions[b],g->positions[c],local,w_b,w_c))continue;
            int part=-1;std::uint16_t relative{};
            for(std::size_t i=0;i<g->parts.size();++i) {
                const auto& p=g->parts[i];
                if(ref.strip>=p.first && unsigned(ref.strip)<unsigned(p.first)+p.count) {
                    part=int(i);relative=std::uint16_t(ref.strip-p.first);break;
                }
            }
            if(part<0)return {};
            return RenderSurfaceHit{l.cluster,source.instance,section,part,relative,ref.lightmap_triangle,w_b,w_c};
        }
        return {};
    };
    if(original_leaf)return scan(*original_leaf);
    std::optional<RenderSurfaceHit> result;
    for(const auto leaf:bank->surface_leaves[source.surface]) {
        const auto candidate=scan(leaf);
        // A source traversal could return this leaf and fail attribution.
        // Without that exact leaf, partial membership coverage is unresolved.
        if(!candidate)return {};
        if(result && !same_attribution(*result,*candidate))return {};
        result=candidate;
    }
    return result;
}
std::optional<BspSurfaceSample> BspSurfaceLighting::interpolate(const RenderSurfaceHit& hit) const {
    if(!lighting.structure_matches)return {};
    const Geometry* geometry{};
    if(hit.geometry<0)return {};
    if(hit.instance>=0) {
        if(std::size_t(hit.instance)>=instances.size() || std::size_t(hit.geometry)>=definitions.size())return {};
        geometry=&definitions[hit.geometry];
    } else {
        if(std::size_t(hit.geometry)>=clusters.size())return {};
        geometry=&clusters[hit.geometry];
    }
    if(hit.part<0 || std::size_t(hit.part)>=geometry->parts.size())return {};
    const auto index=std::size_t(hit.lightmap_triangle)*3;
    if(index+2>=geometry->indices.size())return {};
    const std::array<unsigned,3> v{geometry->indices[index],geometry->indices[index+1],geometry->indices[index+2]};
    if(*std::max_element(v.begin(),v.end())>=geometry->diffuse_uv.size())return {};
    BspSurfaceSample out;
    const auto material=geometry->parts[hit.part].material;
    if(material>=shaders.size())return {};
    out.shader=shaders[material];
    out.diffuse_uv=interpolate_float_uv(geometry->diffuse_uv[v[0]],geometry->diffuse_uv[v[1]],geometry->diffuse_uv[v[2]],hit.weight_b,hit.weight_c);
    out.diffuse_uv.x=std::clamp(out.diffuse_uv.x,-1000.f,1000.f);
    out.diffuse_uv.y=std::clamp(out.diffuse_uv.y,-1000.f,1000.f);
    if(!geometry->lightmap_uv.empty()) {
        if(*std::max_element(v.begin(),v.end())>=geometry->lightmap_uv.size())return {};
        out.lightmap_uv=interpolate_uv(geometry->lightmap_uv[v[0]],geometry->lightmap_uv[v[1]],geometry->lightmap_uv[v[2]],hit.weight_b,hit.weight_c);
    }
    if(hit.instance>=0) {
        if(instances[hit.instance].policy!=0) {
            if(std::size_t(hit.instance)>=lighting.instance_buckets.size())return {};
            const auto& ref=lighting.instance_buckets[hit.instance];
            if(ref.bucket<0 || std::size_t(ref.bucket)>=lighting.buckets.size() || ref.section_offsets.size()!=1 || ref.section_offsets[0]==0xffff)return {};
            const auto& bucket=lighting.buckets[ref.bucket];
            std::array<Vec3,3> color;
            for(unsigned i=0;i<3;++i) {
                const auto source=std::size_t(ref.section_offsets[0])+v[i];
                if(source>=bucket.radiance.size())return {};
                color[i]=decode_bsp_radiance(bucket.radiance[source]);
            }
            out.radiance=interpolate_color(color[0],color[1],color[2],hit.weight_b,hit.weight_c);
        } else if(std::size_t(hit.instance)<lighting.instances.size())out.lightmap=lighting.instances[hit.instance];
    } else {
        if(std::size_t(hit.cluster)<lighting.clusters.size())out.lightmap=lighting.clusters[hit.cluster];
    }
    // D1E10 calls D31C0 with the resident render geometry for both cluster
    // and instance contacts. GPU instance buckets are not its incident bank.
    // With at most three declarations D31C0 leaves the initialized +Z value.
    out.incident={0,0,1};
    if(geometry->incident.empty())return out;
    if(*std::max_element(v.begin(),v.end())>=geometry->incident.size())return {};
    const std::array<std::uint32_t,3> incident{geometry->incident[v[0]],geometry->incident[v[1]],geometry->incident[v[2]]};
    const auto a=decode_cpu_lightmap_incident(incident[0]),b=decode_cpu_lightmap_incident(incident[1]),c=decode_cpu_lightmap_incident(incident[2]);
    // D31C0 X adds AB first; Y/Z add AC first. No instance rotation occurs.
    out.incident={(b.x-a.x)*hit.weight_b+(c.x-a.x)*hit.weight_c+a.x,
                  (c.y-a.y)*hit.weight_c+(b.y-a.y)*hit.weight_b+a.y,
                  (c.z-a.z)*hit.weight_c+(b.z-a.z)*hit.weight_b+a.z};
    return out;
}
std::optional<ObjectLightmapSample> BspSurfaceLighting::sample(const RenderSurfaceHit& hit,float detail) const {
    return sample(hit,detail,true,{});
}
std::optional<ObjectLightmapSample> BspSurfaceLighting::sample(const RenderSurfaceHit& hit,float detail,
    bool initial_query,const std::optional<ObjectLightmapSample>& caller_sample) const {
    if(!lighting.structure_matches || hit.geometry<0)return {};
    const auto& geometries=hit.instance>=0?definitions:clusters;
    if(std::size_t(hit.geometry)>=geometries.size())return {};
    const auto& geometry=geometries[hit.geometry];
    if(hit.part<0 || std::size_t(hit.part)>=geometry.parts.size())return {};
    const auto type=geometry.parts[hit.part].type;
    if(type==4 || type==5) {
        if(initial_query)return ObjectLightmapSample{{0,0,-1},0xff404040u,0xff404040u};
        if(!caller_sample)throw FormatError("retained special-part light query requires caller sample");
        return caller_sample;
    }
    const auto source=interpolate(hit);
    if(!source)return {};
    ObjectLightmapSample result;result.incident=source->incident;
    if(source->radiance) {
        const auto color=*source->radiance;
        result.lightmap_rgb=pack_object_lighting_rgb({std::clamp(color.x,0.f,1.f),
            std::clamp(color.y,0.f,1.f),std::clamp(color.z,0.f,1.f)});
    } else {
        if(!source->lightmap || source->lightmap>lighting.images.size())return {};
        const auto color=sample_object_texture(lighting.images[source->lightmap-1],source->lightmap_uv,detail);
        if(!color)return {};
        result.lightmap_rgb=*color&0xffffffu;
    }
    // D1E10's static/instance path defaults failed D30D0 to .5 RGB, then
    // clamps and packs with 131ED0. This is independent of change colors.
    result.diffuse_rgb=pack_object_lighting_rgb({.5f,.5f,.5f});
    const auto material=geometry.parts.at(hit.part).material;
    if(material<diffuse.size()) {
        if(const auto color=sample_object_texture(diffuse[material],source->diffuse_uv,detail))
            result.diffuse_rgb=*color&0xffffffu;
    }
    return result;
}
BspSurfaceLighting Cache::load_bsp_surface_lighting(std::size_t index) const {
    if(index>=bsps_.size())throw FormatError("surface-lighting BSP outside scenario");
    const auto& bsp=bsps_[index];
    const Bytes meta(Bytes(cache_bytes()).slice(bsp.file_offset,bsp.size));
    const auto magic=std::int64_t(bsp.memory_address);
    BspSurfaceLighting out;out.bsp=std::int32_t(index);out.lighting=load_bsp_lighting(index);
    const auto materials=TagAddressSpace(meta, magic).block(16+0xa4, 32, 1000000);
    for(std::size_t i=0;i<materials.count;++i) {
        const auto shader=meta.u32(materials.offset+i*32+12);
        out.shaders.push_back(shader);
        const auto texture=load_object_surface_texture(shader);
        out.diffuse.push_back(texture?*texture:Texture{});
    }
    const auto decode_geometry=[&](std::size_t at) {
        BspSurfaceLighting::Geometry g;const auto count=meta.u16(at);
        if(!count)return g;
        const auto resources=TagAddressSpace(meta, magic).block(at+56, 16, 1000000);
        const auto raw=raw_data(meta.u32(at+40),meta.u32(at+44));const Bytes bytes(raw);
        const auto base=std::size_t(meta.u32(at+48))+8;
        struct RawStream {std::size_t offset{},size{};bool present{};};
        std::array<RawStream,32> streams{};RawStream descriptors;
        std::vector<std::uint32_t> canonical_lightmap;
        for(std::size_t r=0;r<resources.count;++r) {
            const auto p=resources.offset+r*16,size=std::size_t(meta.u32(p+8)),offset=base+std::size_t(meta.u32(p+12));
            bytes.require(offset,size);const auto kind=std::to_integer<unsigned>(meta.slice(p,1)[0]);
            const auto locator=meta.u16(p+4),channel=meta.u16(p+6);
            if(kind==0 && locator==0) {
                if(size%72)throw FormatError("source render parts size invalid");
                for(std::size_t s=0;s<size;s+=72)g.parts.push_back({bytes.u16(offset+s+6),bytes.u16(offset+s+8),bytes.u16(offset+s+4),bytes.u16(offset+s)});
            } else if(kind==0 && locator==32) {
                if(size%2)throw FormatError("source render indices size invalid");
                for(std::size_t s=0;s<size;s+=2)g.indices.push_back(bytes.u16(offset+s));
            } else if(kind==0 && locator==56 && channel==32) {
                if(descriptors.present || size%32)throw FormatError("source render descriptors invalid");
                descriptors={offset,size,true};
            } else if(kind==2 && locator==56) {
                if(channel>=streams.size() || streams[channel].present)throw FormatError("source render stream ordinal invalid");
                streams[channel]={offset,size,true};
                if(channel==0) {
                    if(size%count || size/count<12)throw FormatError("source render positions invalid");
                    for(unsigned v=0;v<count;++v)g.positions.push_back(point(bytes,offset+v*(size/count)));
                } else if(channel==1) {
                    if(size!=std::size_t(count)*8)throw FormatError("source diffuse UV invalid");
                    for(unsigned v=0;v<count;++v)g.diffuse_uv.push_back({bytes.f32(offset+v*8),bytes.f32(offset+v*8+4)});
                } else if(channel==3) {
                    const auto stride=pc_geometry()?8u:4u;
                    if(size!=std::size_t(count)*stride)throw FormatError("source lightmap UV invalid");
                    constexpr float factor=std::bit_cast<float>(0x37800080u);
                    for(unsigned v=0;v<count;++v) {
                        const auto x=pc_geometry()?pack_bsp_lightmap_uv_tu15(bytes.f32(offset+v*8)):
                            std::int16_t(bytes.u16(offset+v*4));
                        const auto y=pc_geometry()?pack_bsp_lightmap_uv_tu15(bytes.f32(offset+v*8+4)):
                            std::int16_t(bytes.u16(offset+v*4+2));
                        canonical_lightmap.push_back(std::uint16_t(x)|(std::uint32_t(std::uint16_t(y))<<16));
                        g.lightmap_uv.push_back({float(2*x+1)*factor,float(2*y+1)*factor});
                    }
                } else if(channel==4) {
                    if(size!=std::size_t(count)*(pc_geometry()?12u:4u))throw FormatError("source incident stream invalid");
                }
            }
        }
        if(g.positions.size()!=count || g.diffuse_uv.size()!=count || g.parts.empty() || g.indices.empty())
            throw FormatError("source render geometry streams incomplete");
        // D31C0 scans literal declarations starting at3. If48 is absent it
        // reads the last descriptor as packed incident, even when that is31
        // (SHORT2 lightmap UV) on an instance definition. This CPU behavior
        // is independent of the GPU's separately bound instance bucket.
        if(!descriptors.present || descriptors.size/32>streams.size())throw FormatError("source render descriptor bank missing");
        if(pc_geometry()) {
            // Same PC declarations already admitted by load_bsp. The CPU
            // bank below restores the original packed representation before
            // running the unchanged descriptor-selection/interpolation path.
            const auto declared=[&](unsigned channel,unsigned type,unsigned stride) {
                if(descriptors.size<(channel+1)*32 ||
                    bytes.slice(descriptors.offset+channel*32,1)[0]!=std::byte(type) ||
                    bytes.slice(descriptors.offset+channel*32+1,1)[0]!=std::byte(stride))
                    throw FormatError("unsupported PC CPU-lighting vertex declaration");
            };
            declared(0,1,12);declared(1,23,8);
            if(streams[2].present)declared(2,25,36);
            if(streams[3].present)declared(3,29,8);
            if(streams[4].present)declared(4,47,12);
        }
        g.incident.clear();const auto descriptor_count=descriptors.size/32;
        if(descriptor_count>3) {
            std::size_t selected=3;
            for(;selected<descriptor_count;++selected)
                if(bytes.slice(descriptors.offset+selected*32,1)[0]==(pc_geometry()?std::byte{47}:std::byte{48}))break;
            if(selected==descriptor_count)--selected;
            const auto& stream=streams[selected];
            if(!stream.present || stream.size<std::size_t(count)*4)throw FormatError("CPU incident stream escapes resident geometry");
            if(pc_geometry()) {
                const auto type=bytes.slice(descriptors.offset+selected*32,1)[0];
                if(selected==4 && type==std::byte{47} && stream.size==std::size_t(count)*12) {
                    for(unsigned v=0;v<count;++v)g.incident.push_back(pack_bsp_incident_tu15(point(bytes,stream.offset+v*12)));
                } else if(selected==3 && type==std::byte{29} && canonical_lightmap.size()==count) {
                    // With no incident declaration, D31C0 interprets the
                    // final original SHORT2 UV words as packed incident.
                    g.incident=std::move(canonical_lightmap);
                } else throw FormatError("unsupported PC CPU incident fallback declaration");
            } else for(unsigned v=0;v<count;++v)g.incident.push_back(bytes.u32(stream.offset+v*4));
        }
        return g;
    };
    const auto clusters=TagAddressSpace(meta, magic).block(16+0x9c, 176, 1000000),definitions=TagAddressSpace(meta, magic).block(16+0x138, 200, 1000000);
    for(std::size_t i=0;i<clusters.count;++i) {
        out.clusters.push_back(decode_geometry(clusters.offset+i*176));
        const auto membership=TagAddressSpace(meta, magic).block(clusters.offset+i*176+0x98, 2, 1000000);
        auto& list=out.cluster_instances.emplace_back();
        for(std::size_t m=0;m<membership.count;++m)list.push_back(meta.u16(membership.offset+m*2));
        const auto capacity=meta.u32(clusters.offset+i*176+0xa8);
        auto& accelerator=out.cluster_accelerators.emplace_back();
        if(capacity) {
            const auto pointer=TagAddressSpace(meta,magic).offset(meta.u32(clusters.offset+i*176+0xac),std::size_t(capacity)+0x30);
            accelerator=decode_source_instance_accelerator(meta.slice(std::size_t(pointer),std::size_t(capacity)+0x30),capacity);
            // Validate both branches, including code not reached by a current
            // ray. Unknown or truncated source programs are never admitted.
            (void)source_instance_accelerator_inventory(*accelerator);
        }
    }
    for(std::size_t i=0;i<definitions.count;++i)out.definitions.push_back(decode_geometry(definitions.offset+i*200));
    const auto decode_collision=[&](std::size_t at,std::size_t render_leaf_block,std::size_t ref_block) {
        BspSurfaceLighting::CollisionBank bank;
        const auto nodes=TagAddressSpace(meta, magic).block(at, 8, 1000000),planes=TagAddressSpace(meta, magic).block(at+8, 16, 1000000),leaves=TagAddressSpace(meta, magic).block(at+0x10, 4, 1000000),
            refs=TagAddressSpace(meta, magic).block(at+0x18, 4, 1000000),nodes2=TagAddressSpace(meta, magic).block(at+0x20, 16, 1000000),surfaces=TagAddressSpace(meta, magic).block(at+0x28, 8, 1000000),
            edges=TagAddressSpace(meta, magic).block(at+0x30, 12, 1000000),vertices=TagAddressSpace(meta, magic).block(at+0x38, 16, 1000000),
            render_leaves=TagAddressSpace(meta, magic).block(render_leaf_block, 8, 1000000),render_refs=TagAddressSpace(meta, magic).block(ref_block, 8, 1000000);
        const auto u24=[&](std::size_t p) {return std::uint32_t(std::to_integer<unsigned>(meta.slice(p,1)[0]))|
            (std::uint32_t(meta.u16(p+1))<<8);};
        for(std::size_t i=0;i<nodes.count;++i) {
            const auto p=nodes.offset+i*8;
            bank.node_planes.push_back(meta.u16(p));bank.nodes.push_back({meta.u16(p),u24(p+2),u24(p+5)});
        }
        for(std::size_t i=0;i<planes.count;++i) {
            const auto p=planes.offset+i*16;bank.planes.push_back({meta.f32(p),meta.f32(p+4),meta.f32(p+8),meta.f32(p+12)});
        }
        for(std::size_t i=0;i<leaves.count;++i) {
            const auto p=leaves.offset+i*4;bank.collision_leaves.push_back({std::to_integer<std::uint8_t>(meta.slice(p,1)[0]),
                std::to_integer<std::uint8_t>(meta.slice(p+1,1)[0]),meta.u16(p+2)});
        }
        for(std::size_t i=0;i<refs.count;++i) {
            const auto p=refs.offset+i*4;bank.references2.push_back({meta.u16(p),meta.u16(p+2)});
        }
        for(std::size_t i=0;i<nodes2.count;++i) {
            const auto p=nodes2.offset+i*16;
            bank.nodes2.push_back({{meta.f32(p),meta.f32(p+4)},meta.f32(p+8),{meta.u16(p+12),meta.u16(p+14)}});
        }
        for(std::size_t i=0;i<surfaces.count;++i) {
            const auto p=surfaces.offset+i*8;
            bank.surface_planes.push_back(meta.u16(p));bank.surfaces.push_back({meta.u16(p),meta.u16(p+2),
                std::to_integer<std::uint8_t>(meta.slice(p+4,1)[0]),std::to_integer<std::uint8_t>(meta.slice(p+5,1)[0]),std::int16_t(meta.u16(p+6))});
        }
        for(std::size_t i=0;i<edges.count;++i) {
            const auto p=edges.offset+i*12;bank.edges.push_back({{meta.u16(p),meta.u16(p+2)},
                {meta.u16(p+4),meta.u16(p+6)},{std::int16_t(meta.u16(p+8)),std::int16_t(meta.u16(p+10))}});
        }
        for(std::size_t i=0;i<vertices.count;++i)bank.vertices.push_back(point(meta,vertices.offset+i*16));
        bank.surface_leaves.resize(surfaces.count);
        for(std::size_t i=0;i<render_refs.count;++i) {
            const auto p=render_refs.offset+i*8;bank.references.push_back({meta.u16(p),meta.u16(p+2),std::int32_t(meta.u32(p+4))});
        }
        for(std::size_t i=0;i<render_leaves.count;++i) {
            const auto p=render_leaves.offset+i*8;
            BspSurfaceLighting::Leaf l{std::int16_t(meta.u16(p)),meta.u16(p+2),meta.u32(p+4)};
            if(l.count && (l.first>render_refs.count || l.count>render_refs.count-l.first))throw FormatError("source render leaf refs outside block");
            bank.leaves.push_back(l);
        }
        // 1DDEA0/1DE220 and 1DEA00 encode 2D terminal surfaces with bit15,
        // preserving the low15 index. Enumerate authored membership only.
        for(std::size_t leaf=0;leaf<leaves.count;++leaf) {
            const auto p=leaves.offset+leaf*4;
            const auto first=meta.u16(p+2);
            const auto count=std::to_integer<unsigned>(meta.slice(p+1,1)[0]);
            if(!count)continue;
            if(first>refs.count || count>refs.count-first)throw FormatError("source BSP2D refs outside block");
            std::vector<std::uint16_t> pending;
            for(unsigned i=0;i<count;++i)pending.push_back(meta.u16(refs.offset+(first+i)*4+2));
            std::vector<bool> visited(nodes2.count);
            while(!pending.empty()) {
                const auto child=pending.back();pending.pop_back();
                if(child==0xffff)continue; // Source 296520's empty 2D terminal.
                const auto node=std::size_t(child&0x7fff);
                if(child&0x8000) {
                    if(node>=surfaces.count)throw FormatError("source BSP2D terminal surface outside block");
                    bank.surface_leaves[node].push_back(std::uint32_t(leaf));
                } else {
                    if(node>=nodes2.count)throw FormatError("source BSP2D node outside block");
                    if(visited[node])continue;
                    visited[node]=true;
                    pending.push_back(meta.u16(nodes2.offset+node*16+12));pending.push_back(meta.u16(nodes2.offset+node*16+14));
                }
            }
        }
        for(auto& memberships:bank.surface_leaves) {
            std::sort(memberships.begin(),memberships.end());memberships.erase(std::unique(memberships.begin(),memberships.end()),memberships.end());
        }
        return bank;
    };
    const auto collision=TagAddressSpace(meta, magic).block(16+0x14, 64, 1000000);
    for(std::size_t i=0;i<collision.count;++i)out.collision.push_back(decode_collision(collision.offset+i*64,16+0x2c,16+0x4c));
    for(std::size_t i=0;i<definitions.count;++i) {
        const auto at=definitions.offset+i*200;
        out.definition_collision.push_back(decode_collision(at+0x70,at+0xb8,at+0xc0));
    }
    const auto instances=TagAddressSpace(meta, magic).block(16+0x140, 88, 1000000);
    for(std::size_t i=0;i<instances.count;++i) {
        const auto p=instances.offset+i*88;
        out.instances.push_back({meta.u16(p+52),meta.u16(p+86),meta.f32(p),
            {point(meta,p+4),point(meta,p+16),point(meta,p+28)},point(meta,p+40),point(meta,p+60),meta.f32(p+72)});
    }
    return out;
}
} // namespace halo2
