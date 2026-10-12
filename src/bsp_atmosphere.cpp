#include "halo2/bsp_atmosphere.hpp"
#include "halo2/math.hpp"
#include "halo2/source_fog_clear.hpp"
#include <algorithm>
#include <cmath>
namespace halo2 {
namespace {
constexpr float epsilon=.0001f, upper=.9999f, spread_limit=10.f;
float saturated(float v) {return std::clamp(v,0.f,1.f);}
float snapped(float v) {return v<epsilon?0.f:v>upper?1.f:v;}
void clamp_fog(AtmosphericFog &fog) {
    if(fog.maximum_density<=epsilon) {fog={};return;}
    fog.color={saturated(fog.color.x),saturated(fog.color.y),saturated(fog.color.z)};
    fog.opaque=std::max(fog.opaque,fog.start+epsilon);
    if(fog.maximum_density>upper)fog.maximum_density=1;
}
struct Bank {std::array<float,6> sum{};float weight{};
    void add(const AtmosphericFog &fog,float w) {
        const std::array<float,6> values{fog.color.x,fog.color.y,fog.color.z,fog.maximum_density,fog.start,fog.opaque};
        for(unsigned i=0;i<6;++i)sum[i]+=values[i]*w;
        weight+=w;
    }
    AtmosphericFog result() const {
        if(weight<=0 || sum[3]<=0)return {};
        const float inv=1.f/weight;
        AtmosphericFog out{{sum[0]*inv,sum[1]*inv,sum[2]*inv},sum[3]*inv,sum[4]*inv,sum[5]*inv};
        clamp_fog(out);return out;
    }
};
float portal_distance(const BspAtmosphere::Portal &portal,const ShaderVector &plane,Vec3 eye) {
    // 14B240 distance to the portal's bounding disk. Preserve the SSE order
    // of the perpendicular projection before the x87 square roots.
    Vec3 delta=eye-portal.center;
    const float normal=(plane[2]*delta.z+plane[1]*delta.y)+plane[0]*delta.x;
    const float px=plane[0]*normal,py=plane[1]*normal,pz=plane[2]*normal;
    delta={delta.x-px,delta.y-py,delta.z-pz};
    // The original stores N dot (N * projection), rather than assuming
    // the authored plane normal has mathematically exact unit length.
    const float perpendicular=(plane[1]*py+plane[2]*pz)+plane[0]*px;
    const float radial=(delta.z*delta.z+delta.y*delta.y)+delta.x*delta.x;
    if(radial<=portal.radius*portal.radius)return std::abs(perpendicular);
    const double edge=std::sqrt(double(radial))-double(portal.radius);
    return float(std::sqrt(edge*edge+double(perpendicular)*double(perpendicular)));
}
std::array<float,10> patch_values(const PatchyFog &p) {
    return {p.color.x,p.color.y,p.color.z,0,0,0,p.density[0],p.density[1],p.distance[0],p.distance[1]};
}
void patch_values(PatchyFog &p,const std::array<float,10> &v) {
    p.color={v[0],v[1],v[2]};p.density={v[6],v[7]};p.distance={v[8],v[9]};
}
void clamp_patch(std::optional<PatchyFog> &patch) {
    if(!patch)return;
    auto &p=*patch;
    if(p.density[1]<=epsilon || p.distance[1]<=epsilon) {patch.reset();return;}
    p.color={saturated(p.color.x),saturated(p.color.y),saturated(p.color.z)};
    p.density[0]=saturated(p.density[0]);p.density[1]=saturated(std::max(p.density[0],p.density[1]));
    p.distance[1]=std::max(p.distance[0]+epsilon,p.distance[1]);
    if(p.density[1]>upper)p.density[1]=1;
}
}
BspAtmosphere::CameraLocation BspAtmosphere::location_at(Vec3 eye) const {
    if(nodes.empty())return {};
    unsigned node=0;
    for(std::size_t count=0;count<=nodes.size();++count) {
        if(node&0x800000u) {
            if(node==0xffffffu)return {};
            const unsigned leaf=node&0x7fffffu;
            if(leaf>=leaves.size())throw FormatError("atmosphere leaf outside BSP");
            return {int(leaf),leaves[leaf]};
        }
        if(node>=nodes.size())throw FormatError("atmosphere node outside BSP");
        const auto &n=nodes[node];
        if(n.plane>=planes.size())throw FormatError("atmosphere plane outside BSP");
        const auto &p=planes[n.plane];
        // 14A280 includes the positive child at a point exactly on the plane.
        const float side=((p[2]*eye.z+p[1]*eye.y)+p[0]*eye.x)-p[3];
        node=side>=0?n.positive:n.negative;
    }
    throw FormatError("atmosphere BSP traversal contains a cycle");
}
int BspAtmosphere::cluster_at(Vec3 eye) const {
    return location_at(eye).cluster;
}
bool BspAtmosphere::contains_planar(int cluster,std::uint32_t datum) const {
    if(cluster<=-2) {
        const auto instance=std::size_t(-(std::int64_t(cluster)+2));
        if(instance>=instance_membership.size())throw FormatError("planar instance outside BSP");
        return std::any_of(instance_membership[instance].begin(),instance_membership[instance].end(),
            [&](unsigned member){return contains_planar(int(member),datum);});
    }
    if(cluster<0)return false;
    if(std::size_t(cluster)>=clusters.size())throw FormatError("planar cluster outside BSP");
    const int designator=clusters[cluster].planar_designator;
    if(designator==-1)return false;
    if(datum==0xffffffffu)return true;
    int palette=designator;
    if(designator<0) {
        const unsigned plane=unsigned(designator)&127;
        if(plane>=fog_planes.size())throw FormatError("containing fog plane outside BSP");
        palette=fog_planes[plane].palette;
    }
    return palette>=0 && std::size_t(palette)<fog_palette.size() && fog_palette[palette].datum==datum;
}
std::vector<std::uint32_t> BspAtmosphere::potentially_visible_clusters(int camera_cluster) const {
    const auto words=(clusters.size()+31)/32;
    std::vector<std::uint32_t> out(words);
    if(camera_cluster<0)return out;
    if(std::size_t(camera_cluster)>=clusters.size())throw FormatError("PVS camera cluster outside BSP");
    if(cluster_pvs.size()!=words*clusters.size())throw FormatError("PVS bank outside BSP");
    std::copy_n(cluster_pvs.begin()+std::size_t(camera_cluster)*words,words,out.begin());
    return out;
}
std::vector<std::uint32_t> BspAtmosphere::visible_clusters(int camera_cluster,
    std::span<const std::uint32_t> open_portals) const {
    const auto potential=potentially_visible_clusters(camera_cluster);
    if(camera_cluster<0 || !(clusters.at(camera_cluster).flags&2))return potential;
    const auto words=potential.size();
    if(cluster_closed_pvs.size()!=words*clusters.size())throw FormatError("closed PVS bank outside BSP");
    std::vector<std::uint32_t> out(words);
    std::copy_n(cluster_closed_pvs.begin()+std::size_t(camera_cluster)*words,words,out.begin());
    std::vector<bool> visited(portals.size());bool changed;
    do {
        changed=false;
        for(unsigned cluster=0;cluster<clusters.size();++cluster) {
            if(!(out[cluster/32]&(1u<<(cluster&31))))continue;
            for(const auto index:clusters[cluster].portals) {
                if(index>=portals.size())throw FormatError("PVS portal outside BSP");
                if(visited[index])continue;
                visited[index]=true;const auto &portal=portals[index];
                if(!(portal.flags&4) || index/32>=open_portals.size() ||
                    !(open_portals[index/32]&(1u<<(index&31))))continue;
                if(portal.back<0 || portal.front<0 || std::size_t(portal.back)>=clusters.size() ||
                    std::size_t(portal.front)>=clusters.size())throw FormatError("PVS portal endpoint outside BSP");
                for(unsigned word=0;word<words;++word) {
                    const auto endpoints=cluster_closed_pvs[std::size_t(portal.back)*words+word]|
                        cluster_closed_pvs[std::size_t(portal.front)*words+word];
                    const auto next=out[word]|(endpoints&potential[word]);
                    changed=changed || next!=out[word];out[word]=next;
                }
            }
        }
    } while(changed);
    return out;
}
bool BspAtmosphere::geometry_visible(int cluster,std::span<const std::uint32_t> visible) const {
    if(cluster<=-2) {
        const auto instance=std::size_t(-(std::int64_t(cluster)+2));
        if(instance>=instance_membership.size())throw FormatError("PVS instance outside BSP");
        return std::any_of(instance_membership[instance].begin(),instance_membership[instance].end(),
            [&](unsigned member){return geometry_visible(int(member),visible);});
    }
    if(cluster==-1)return true;
    if(cluster<0 || std::size_t(cluster)>=clusters.size())throw FormatError("PVS geometry cluster outside BSP");
    return unsigned(cluster)/32<visible.size() && (visible[unsigned(cluster)/32]&(1u<<(unsigned(cluster)&31)));
}
bool BspAtmosphere::contains_planar(int cluster,std::uint32_t datum,
    std::span<const std::uint32_t> visible) const {
    if(cluster<=-2) {
        const auto instance=std::size_t(-(std::int64_t(cluster)+2));
        if(instance>=instance_membership.size())throw FormatError("planar instance outside BSP");
        return std::any_of(instance_membership[instance].begin(),instance_membership[instance].end(),
            [&](unsigned member){return geometry_visible(int(member),visible)&&contains_planar(int(member),datum);});
    }
    return cluster>=0&&geometry_visible(cluster,visible)&&contains_planar(cluster,datum);
}
const BspPatchyDefinition *BspAtmosphere::patchy_definition(std::uint32_t datum) const {
    const auto found=std::find_if(patchy_definitions.begin(),patchy_definitions.end(),
        [&](const auto &definition){return definition.datum==datum;});
    return found==patchy_definitions.end()?nullptr:&*found;
}
BspFogSample BspAtmosphere::sample(Vec3 eye,Vec3 forward,std::optional<int> selected) const {
    BspFogSample out;
    out.cluster=selected.value_or(cluster_at(eye));
    if(out.cluster<0)return out;
    if(std::size_t(out.cluster)>=clusters.size())throw FormatError("atmosphere cluster outside BSP");
    const auto &containing=clusters[out.cluster];
    out.visible_sky=containing.visible_sky;
    if(out.visible_sky<0 && (scenario_flags&2) && !skies.empty())out.visible_sky=0;
    bool sky_model=false;
    std::optional<Vec3> authored_clear;
    if(out.visible_sky>=0) {
        if(std::size_t(out.visible_sky)>=skies.size())throw FormatError("visible atmosphere sky outside scenario");
        const auto &sky=skies[out.visible_sky];
        sky_model=sky.present && sky.model_present;
        authored_clear=sky.clear;
    }
    // 12E960 traverses each reachable cluster once. Its stored distance is
    // the minimum camera-to-portal distance; it is not a summed path length.
    std::vector<unsigned> order{unsigned(out.cluster)};
    std::vector<int> seen(clusters.size(),-1);std::vector<float> distance{0};
    seen[out.cluster]=0;
    for(std::size_t current=0;current<order.size();++current) {
        const unsigned cluster=order[current];
        for(const auto pi:clusters[cluster].portals) {
            if(pi>=portals.size())throw FormatError("atmosphere portal outside BSP");
            const auto &portal=portals[pi];
            const int adjacent=portal.back==int(cluster)?portal.front:portal.back;
            if(adjacent<0)continue;
            if(std::size_t(adjacent)>=clusters.size() || portal.plane>=planes.size())throw FormatError("invalid atmosphere portal adjacency");
            const float d=portal_distance(portal,planes[portal.plane],eye);
            if(seen[adjacent]<0) {
                if(d<spread_limit) {seen[adjacent]=int(order.size());order.push_back(unsigned(adjacent));distance.push_back(d);}
            } else distance[seen[adjacent]]=std::min(distance[seen[adjacent]],d);
        }
    }
    Bank primary,secondary,sky_bank;
    std::array<float,10> patch_sum{};float patch_weight{},patch_choice{};
    float bloom_weight{},bloom_remaining=1;std::array<float,4> bloom_sum{};
    for(std::size_t i=0;i<order.size();++i) {
        const auto &cluster=clusters[order[i]];
        if(cluster.atmospheric!=-1)throw FormatError("scenario fog palette requires authored mixer aggregation");
        const auto sky_index=[&](int index){return index>=0 && std::size_t(index)<skies.size();};
        int sky=-1;
        // 130F60 returns a valid explicit slot's datum even when it is NONE.
        // Only an absent/rejected visible candidate proceeds to the BSP
        // default; never replace an explicitly selected NONE with that sky.
        if(cluster.sky!=-1 && sky_index(cluster.sky))sky=cluster.sky;
        else if(cluster.sky==-1 && sky_index(cluster.visible_sky) &&
                skies[cluster.visible_sky].present && !(skies[cluster.visible_sky].flags&16))
            sky=cluster.visible_sky;
        if(sky<0 && sky_index(default_sky) && skies[default_sky].present)sky=default_sky;
        const float fallback=std::max(0.f,1.f-distance[i]); // 377110 initializes default spread to 1.
        // 12EFB0's NONE branch adds this weight once to each atmospheric
        // denominator, and at 12FA87/12F57C twice to the patchy denominator.
        if(sky<0 || !skies[sky].present) {
            primary.weight+=fallback;secondary.weight+=fallback;sky_bank.weight+=fallback;
            patch_weight+=fallback;patch_weight+=fallback;
            continue;
        }
        const auto &definition=skies[sky];
        const float spread=definition.spread>0?std::min(spread_limit,definition.spread):1.f;
        const float w=saturated(1.f-distance[i]/spread);
        if(w<=0) {patch_weight+=fallback;continue;}
        primary.add(definition.primary,w);secondary.add(definition.secondary,w);sky_bank.add(definition.sky_fog,w);
        if(definition.patchy) {
            const auto values=patch_values(*definition.patchy);
            for(unsigned component=0;component<10;++component)patch_sum[component]+=values[component]*w;
            patch_weight+=w;
            if(order[i]==unsigned(out.cluster) || w>patch_choice) {out.patchy=definition.patchy;out.patchy_datum=definition.patchy_datum;patch_choice=w;}
        } else patch_weight+=fallback;
        if(definition.bloom[0]>0) {
            const float gamma=definition.bloom[3]==0?1:definition.bloom[3];
            bloom_sum[0]+=definition.bloom[0]*w;bloom_sum[1]+=definition.bloom[1]*w;
            bloom_sum[2]+=definition.bloom[2]*w;bloom_sum[3]+=gamma*w;
            bloom_weight+=w;bloom_remaining*=1.f-w;
        }
    }
    out.primary=primary.result();out.secondary=secondary.result();out.sky_fog=sky_bank.result();
    if(out.patchy && patch_weight>0 && patch_sum[7]>0) {
        const float inverse=1.f/patch_weight;
        for(float &v:patch_sum)v*=inverse;
        patch_values(*out.patchy,patch_sum);
    } else out.patchy.reset();
    if(bloom_weight>0) {
        const float inverse=1.f/bloom_weight;
        out.bloom={saturated((1.f-bloom_remaining)*bloom_sum[0]*inverse),
            saturated(bloom_sum[1]*inverse),saturated(bloom_sum[2]*inverse),bloom_sum[3]*inverse};
    }
    // 12FEA0 resolves the visible plane first, then the containing palette.
    const FogPalette *palette=nullptr;
    out.planar.plane={forward.x,forward.y,forward.z,dot(forward,eye)};
    if(containing.visible_plane>=0) {
        if(std::size_t(containing.visible_plane)>=fog_planes.size())throw FormatError("visible fog plane outside BSP");
        const auto &plane=fog_planes[containing.visible_plane];
        if(plane.palette>=0 && std::size_t(plane.palette)<fog_palette.size()) {
            palette=&fog_palette[plane.palette];out.planar.plane=plane.plane;
        }
    } else if(containing.planar_designator>=0 && std::size_t(containing.planar_designator)<fog_palette.size()) {
        palette=&fog_palette[containing.planar_designator];
        out.planar.plane={forward.x,forward.y,forward.z,std::max(palette->fog.opaque_distance,palette->fog.opaque_depth)};
    } else out.planar.plane={forward.x,forward.y,forward.z,dot(forward,eye)};
    const auto &plane=out.planar.plane;
    const float raw_height=((plane[2]*eye.z+plane[1]*eye.y)+plane[0]*eye.x)-plane[3];
    const bool below=raw_height<0 || (containing.visible_plane<0 && palette);
    if(palette && palette->datum!=0xffffffffu && !(palette->flags&16) && (!(palette->flags&32) || below)) {
        const auto retained_plane=out.planar.plane;out.planar=palette->fog;out.planar.plane=retained_plane;
        out.planar_datum=palette->datum;out.planar_flags=palette->flags;
        out.planar.eye_offset=std::clamp(out.planar.eye_offset,-1.f,1.f);
        out.planar.maximum_density=out.planar.maximum_density<=epsilon?0:out.planar.maximum_density>upper?1:out.planar.maximum_density;
        out.planar.color={saturated(out.planar.color.x),saturated(out.planar.color.y),saturated(out.planar.color.z)};
        if(out.planar.maximum_density>0) {
            out.planar.opaque_distance=std::max(epsilon,out.planar.opaque_distance);
            out.planar.opaque_depth=std::max(epsilon,out.planar.opaque_depth);
            out.eye_height=raw_height;
            out.eye_fraction=snapped(saturated(-raw_height/out.planar.opaque_depth));
            // 1306AC..1306C7: divide, multiply, then SUBSS from +0.
            // Unary negation before the multiply produces the wrong zero
            // sign, which 130DE0 also copies into the submerged start field.
            const float nonnegative_height=raw_height>=0.f?raw_height:0.f;
            out.eye_offset=0.f-(out.planar.eye_offset/out.planar.opaque_distance)*nonnegative_height;
            const bool admitted=contains_planar(out.cluster,palette->datum);
            if(raw_height<0 && !admitted)out.eye_height=out.eye_fraction=out.eye_offset=0;
            if(palette->patchy) {
                const float minimum=std::clamp(palette->patchy_minimum_depth,0.f,upper);
                const float amount=saturated((out.eye_fraction-minimum)/(1.f-minimum));
                if(amount>0) {
                    const auto a=out.patchy?patch_values(*out.patchy):std::array<float,10>{};
                    const auto b=patch_values(*palette->patchy);std::array<float,10> values{};
                    for(unsigned component=0;component<10;++component)values[component]=(b[component]-a[component])*amount+a[component];
                    if(!out.patchy) {out.patchy=palette->patchy;out.patchy_datum=palette->patchy_datum;}
                    patch_values(*out.patchy,values);
                }
            }
            float lo=palette->transition[0],hi=palette->transition[1];
            if(!(lo<hi)) {lo=1023;hi=1024;}
            lo=std::max(0.f,lo);hi=std::max(lo+epsilon,hi);
            out.transition=out.primary.maximum_density>0 || out.secondary.maximum_density>0?
                snapped(saturated((out.eye_height-lo)/(hi-lo))):0.f;
        }
    }
    clamp_patch(out.patchy);
    const float sky_opacity=saturated(out.eye_fraction*out.planar.maximum_density);
    const float background=saturated(1.f-sky_opacity)*saturated(out.sky_fog.maximum_density);
    out.sky_color_premultiplied={saturated(out.planar.color.x*sky_opacity+out.sky_fog.color.x*background),
        saturated(out.planar.color.y*sky_opacity+out.sky_fog.color.y*background),
        saturated(out.planar.color.z*sky_opacity+out.sky_fog.color.z*background)};
    // 1309C7/130A9E..130AD1: preserve the original complement-product
    // order. Adding the two alpha contributions rounds differently.
    if(out.eye_fraction>0.f)
        out.sky_opacity=snapped(saturated(1.f-saturated(1.f-sky_opacity)*
            saturated(1.f-out.sky_fog.maximum_density)));
    else out.sky_opacity=snapped(saturated(out.sky_fog.maximum_density));
    // 130DE0 moves fully submerged planar fog into an ordinary bank.
    if(out.planar.maximum_density<=0)out.classification=out.primary.maximum_density>0 || out.secondary.maximum_density>0?1:out.sky_fog.maximum_density>0?2:0;
    else if(out.eye_fraction!=1 || out.transition!=0)out.classification=3;
    else {
        const bool to_primary=out.primary.maximum_density==0 ||
            (out.secondary.maximum_density>0 && out.planar.maximum_density==1 &&
             std::max(out.planar.opaque_distance,out.planar.opaque_depth)<=out.primary.start);
        if(out.secondary.maximum_density>0 && !to_primary &&
            (out.planar.maximum_density!=1 || out.secondary.start<std::max(out.planar.opaque_distance,out.planar.opaque_depth)))out.classification=3;
        else {
            const AtmosphericFog submerged{out.planar.color,out.planar.maximum_density,out.eye_offset,out.planar.opaque_distance};
            if(to_primary) {out.primary=submerged;out.classification=4;}
            else {out.secondary=submerged;out.classification=5;}
            out.planar.maximum_density=0;out.eye_fraction=0;out.transition=1;
        }
    }
    out.planar_only=out.classification==3 && out.planar_datum!=0xffffffffu && out.eye_fraction==0 && out.transition==0 && (out.planar_flags&1);
    // 12E5E0 starts with zero RGB, selects fog RGB after classification, then
    //2B790 applies14B360's independent authored override.
    out.clear=source_fog_clear_color({{},std::uint8_t(sky_model),out.sky_color_premultiplied,
        out.sky_opacity,out.primary,out.secondary,out.planar.maximum_density,out.interpolation});
    // 12E6EC also forces bank+58 after classification. Extern32/color30
    // consume this final sky density, not its earlier aggregate value.
    if(sky_model && out.sky_opacity>=1.f)out.sky_fog.maximum_density=1.f;
    if(authored_clear)out.clear=authored_clear;
    return out;
}
BspAtmosphere Cache::load_bsp_atmosphere(std::size_t index) const {
    if(index>=bsps_.size())throw FormatError("atmosphere BSP index outside scenario");
    BspAtmosphere result;const auto &bsp=bsps_[index];const Bytes data(storage_);
    const Bytes metadata(data.slice(bsp.file_offset,bsp.size));
    
    const auto tag_root=[&](std::uint32_t datum,const std::string &group) {
        const auto tag=query_tag({.handle=TagHandle{datum}, .group=TagGroup::from_string(group)});
        if(!tag)throw FormatError("missing atmosphere "+group+" tag");
        return metadata_offset(tag->address,tag->size);
    };
    const auto vector=[&](const Bytes &b,std::size_t at){return Vec3{b.f32(at),b.f32(at+4),b.f32(at+8)};};
    const auto fog=[&](std::size_t at){return AtmosphericFog{vector(data,at),data.f32(at+12),data.f32(at+16),data.f32(at+20)};};
    const auto patch=[&](std::size_t at,std::uint32_t datum) {
        const auto p=tag_root(datum,"fpch");
        PatchyFog out{vector(data,at),{data.f32(at+24),data.f32(at+28)},
            {data.f32(at+32),data.f32(at+36)},data.u32(p),data.f32(p+4),data.f32(p+8),data.f32(p+12),data.f32(p+16),
            data.f32(p+28),data.f32(p+32),data.f32(p+36),data.f32(p+40),vector(data,p+68),{}};
        const auto bitmap=data.u32(p+24);const auto count=bitmap_count(bitmap);
        for(unsigned i=0;i<count;++i)out.noise.push_back(load_bitmap(bitmap,i));
        if(!result.patchy_definition(datum))result.patchy_definitions.push_back({datum,out,
            {data.f32(p+44),data.f32(p+48)},{data.f32(p+52),data.f32(p+56)},data.f32(p+60),data.f32(p+64)});
        return out;
    };
    const auto scenario=query_tag({.group=tag_groups::scenario, .name=scenario_});
    if(!scenario)throw FormatError("missing atmosphere scenario");
    const auto scnr=metadata_offset(scenario->address,scenario->size);result.scenario_flags=data.u16(scnr+18);
    const auto [sky_count,skies]=TagAddressSpace(data, metadata_magic_).block(scnr+8, 8);
    for(std::size_t i=0;i<sky_count;++i) {
        const auto datum=data.u32(skies+i*8+4);BspAtmosphere::Sky out;
        out.present=datum!=0xffffffffu;
        if(!out.present) {result.skies.push_back(std::move(out));continue;}
        const auto s=tag_root(datum,"sky ");
        out.flags=data.u32(s+16);out.spread=data.f32(s+0x44);
        out.model_present=data.u32(s+4)!=0xffffffffu;
        const auto [primary,p]=TagAddressSpace(data, metadata_magic_).block(s+0x48, 24);if(primary)out.primary=fog(p);
        const auto [secondary,q]=TagAddressSpace(data, metadata_magic_).block(s+0x50, 24);if(secondary)out.secondary=fog(q);
        const auto [sky_fog,r]=TagAddressSpace(data, metadata_magic_).block(s+0x58, 16);
        if(sky_fog)out.sky_fog={vector(data,r),data.f32(r+12),0,1};
        const auto [lights,light]=TagAddressSpace(data, metadata_magic_).block(s+0x78, 52);
        for(std::size_t j=0;j<lights;++j)if(data.u32(light+j*52+28) || data.u32(light+j*52+36))throw FormatError("directional sky fog requires retail aggregate port");
        if((out.flags&32) && out.model_present)out.clear=vector(data,s+0xa0);
        const auto [patch_count,pt]=TagAddressSpace(data, metadata_magic_).block(s+0x60, 80);
        if(patch_count && data.u32(pt+76)!=0xffffffffu && data.f32(pt+28)>0) {
            out.patchy_datum=data.u32(pt+76);out.patchy=patch(pt,out.patchy_datum);
        }
        for(unsigned j=0;j<4;++j)out.bloom[j]=data.f32(s+0x68+j*4);
        result.skies.push_back(std::move(out));
    }
    const auto [bsp_count,bsp_refs]=TagAddressSpace(data, metadata_magic_).block(scnr+0x210, 68);
    if(index<bsp_count && (data.u16(bsp_refs+index*68+60)&1))result.default_sky=std::int16_t(data.u16(bsp_refs+index*68+64));
    const auto [collisions,collision]=TagAddressSpace(metadata, bsp.memory_address).block(16+20, 64);
    if(collisions!=1)throw FormatError("atmosphere requires one structure collision BSP");
    const auto [nodes,node]=TagAddressSpace(metadata, bsp.memory_address).block(collision, 8);
    const auto byte=[&](std::size_t at){return std::to_integer<unsigned>(metadata.slice(at,1)[0]);};
    const auto packed24=[&](std::size_t at){return byte(at)|(byte(at+1)<<8)|(byte(at+2)<<16);};
    for(std::size_t i=0;i<nodes;++i)result.nodes.push_back({metadata.u16(node+i*8),packed24(node+i*8+2),packed24(node+i*8+5)});
    const auto [planes,plane]=TagAddressSpace(metadata, bsp.memory_address).block(collision+8, 16);
    for(std::size_t i=0;i<planes;++i)result.planes.push_back({metadata.f32(plane+i*16),metadata.f32(plane+i*16+4),metadata.f32(plane+i*16+8),metadata.f32(plane+i*16+12)});
    const auto [leaves,leaf]=TagAddressSpace(metadata, bsp.memory_address).block(16+0x2c, 8);
    for(std::size_t i=0;i<leaves;++i)result.leaves.push_back(std::int16_t(metadata.u16(leaf+i*8)));
    const auto [clusters,cluster]=TagAddressSpace(metadata, bsp.memory_address).block(16+156, 176);
    const auto [instances,instance]=TagAddressSpace(metadata, bsp.memory_address).block(16+0x140, 88);
    (void)instance;result.instance_membership.resize(instances);
    for(std::size_t i=0;i<clusters;++i) {
        const auto p=cluster+i*176;
        BspAtmosphere::Cluster out{std::int8_t(byte(p+108)),std::int8_t(byte(p+110)),std::int8_t(byte(p+111)),std::int8_t(byte(p+112)),std::int8_t(byte(p+113)),{}};
        out.flags=metadata.u16(p+0x80);
        const auto [portals,portal]=TagAddressSpace(metadata, bsp.memory_address).block(p+140, 2);
        for(std::size_t j=0;j<portals;++j)out.portals.push_back(metadata.u16(portal+j*2));
        const auto [members,member]=TagAddressSpace(metadata, bsp.memory_address).block(p+0x98, 2);
        for(std::size_t j=0;j<members;++j) {
            const auto instance_index=metadata.u16(member+j*2);
            if(instance_index>=instances)throw FormatError("atmosphere instance reference outside BSP");
            result.instance_membership[instance_index].push_back(unsigned(i));
        }
        result.clusters.push_back(std::move(out));
    }
    const auto [pvs_bytes,pvs]=TagAddressSpace(metadata, bsp.memory_address).block(16+0x54, 1);
    const auto pvs_words=(clusters+31)/32*clusters;
    if(pvs_bytes!=pvs_words*4 && pvs_bytes!=pvs_words*8)throw FormatError("unsupported BSP cluster PVS size");
    for(std::size_t i=0;i<pvs_words;++i)result.cluster_pvs.push_back(metadata.u32(pvs+i*4));
    for(std::size_t i=0;i<pvs_words;++i)result.cluster_closed_pvs.push_back(
        metadata.u32(pvs+(pvs_bytes>pvs_words*4?pvs_words:0)*4+i*4));
    const auto [portals,portal]=TagAddressSpace(metadata, bsp.memory_address).block(16+0x5c, 36);
    for(std::size_t i=0;i<portals;++i) {
        const auto p=portal+i*36;
        result.portals.push_back({std::int16_t(metadata.u16(p)),std::int16_t(metadata.u16(p+2)),metadata.u32(p+4),vector(metadata,p+8),metadata.f32(p+20),metadata.u32(p+24)});
    }
    const auto [fog_planes,fog_plane]=TagAddressSpace(metadata, bsp.memory_address).block(16+0x64, 24);
    for(std::size_t i=0;i<fog_planes;++i) {const auto p=fog_plane+i*24;
        result.fog_planes.push_back({std::int16_t(metadata.u16(p)),{metadata.f32(p+4),metadata.f32(p+8),metadata.f32(p+12),metadata.f32(p+16)}});}
    const auto [palette_count,palette]=TagAddressSpace(data, metadata_magic_).block(scnr+0x348, 16);
    for(std::size_t i=0;i<palette_count;++i) {
        const auto datum=data.u32(palette+i*16+8);
        if(datum==0xffffffffu) {result.fog_palette.push_back({});continue;}
        const auto p=tag_root(datum,"fog ");
        BspAtmosphere::FogPalette out;out.datum=datum;out.flags=data.u16(p);
        out.fog={{},vector(data,p+36),data.f32(p+12),data.f32(p+16),data.f32(p+20),data.f32(p+32)};
        out.transition={data.f32(p+24),data.f32(p+28)};
        const auto [patch_count,pt]=TagAddressSpace(data, metadata_magic_).block(p+48, 52);
        if(patch_count && data.u32(pt+48)!=0xffffffffu) {out.patchy_datum=data.u32(pt+48);out.patchy=patch(pt,out.patchy_datum);out.patchy_minimum_depth=data.f32(pt+40);}
        result.fog_palette.push_back(std::move(out));
    }
    const auto globals=query_tag({.group=tag_groups::globals});
    if(!globals)throw FormatError("missing atmosphere rasterizer globals");
    const auto matg=metadata_offset(globals->address,globals->size);
    const auto [rasterizers,rasterizer]=TagAddressSpace(data, metadata_magic_).block(matg+0x108, 0x1c8);
    if(rasterizers!=1)throw FormatError("unsupported atmosphere rasterizer globals");
    result.gradient_datum=data.u32(rasterizer+20);result.gradient=load_bitmap(result.gradient_datum,0);
    return result;
}
std::array<ShaderVector,6> bsp_fog_constants(const BspFogSample &fog,Vec3 eye,Vec3 forward) {
    const float camera=(forward.z*eye.z+forward.y*eye.y)+forward.x*eye.x;
    const float primary=1.f/std::max(epsilon,fog.primary.opaque-fog.primary.start);
    const float secondary=1.f/std::max(epsilon,fog.secondary.opaque-fog.secondary.start);
    const float distance=1.f/std::max(epsilon,fog.planar.opaque_distance);
    const float depth=1.f/std::max(epsilon,fog.planar.opaque_depth);
    const auto &p=fog.planar.plane;
    return {{{forward.x*primary,forward.y*primary,forward.z*primary,-(fog.primary.start+camera)*primary},
        // Original25AEA/25AF4..25BAF multiplies first, then subtracts from +0.
        {0.f-(p[0]*depth),0.f-(p[1]*depth),0.f-(p[2]*depth),p[3]*depth},
        {forward.x*distance,forward.y*distance,forward.z*distance,-(fog.eye_offset+camera)*distance},
        {saturated(-fog.eye_height*depth),0,fog.primary.maximum_density,fog.planar.maximum_density},
        {forward.x*secondary,forward.y*secondary,forward.z*secondary,-(camera+fog.secondary.start)*secondary},
        {fog.secondary.maximum_density,0,0,0}}};
}
MenuFogExterns bsp_fog_externs(const BspFogSample &fog) {
    MenuFogExterns out;auto &s=out.scalars;
    s[3]=fog.planar.maximum_density; // 336F0 slot 3 reads aggregate +0xAC.
    s[24]=fog.primary.maximum_density;s[25]=fog.transition;
    s[26]=(1.f-fog.transition)*fog.planar.maximum_density;s[27]=fog.transition*s[24];
    s[28]=fog.secondary.maximum_density;s[29]=fog.interpolation*s[24];
    s[30]=(1.f-fog.interpolation)*s[28];s[31]=fog.interpolation;
    s[32]=fog.sky_fog.maximum_density;
    s[33]=fog.planar.maximum_density;s[34]=fog.eye_fraction;
    s[35]=(1.f-fog.eye_fraction)*s[33];s[36]=fog.eye_fraction*s[33];
    const auto rgb=[](Vec3 c,float v=1.f){return ShaderVector{c.x*v,c.y*v,c.z*v,0};};
    auto &c=out.colors;c[25]=rgb(fog.primary.color);c[26]=rgb(fog.primary.color,s[24]);
    c[27]=rgb(fog.secondary.color);c[28]=rgb(fog.secondary.color,s[28]);
    c[29]=rgb(fog.sky_fog.color);c[30]=rgb(fog.sky_fog.color,s[32]);
    c[31]=rgb(fog.planar.color);c[32]=rgb(fog.planar.color,s[33]);
    return out;
}
MenuFogComposite bsp_fog_composite(const BspFogSample &fog,float far) {
    // The independently validated title encoder already ports 3A5C0's
    // quantized primary-depth fast path. Reuse it only with zero secondary
    // fog; substitute this sample's source-classified eye opacity below.
    MenuAtmosphere encoder;encoder.atmospheric=fog.primary;
    encoder.planar.opaque_depth=encoder.planar.opaque_distance=1;
    auto result=menu_fog_composite(encoder,{},far);
    auto &w=result.program.words;
    const auto byte=[](float v){return std::uint32_t(std::nearbyint(double(saturated(v))*255.));};
    const auto packed=[&](float a,Vec3 c){return byte(a)<<24|byte(c.x)<<16|byte(c.y)<<8|byte(c.z);};
    const auto premult=[](Vec3 c,float a){return Vec3{saturated(c.x*a),saturated(c.y*a),saturated(c.z*a)};};
    const float eye_alpha=fog.sky_opacity;
    const auto eye_rgb=fog.sky_color_premultiplied;
    const auto primary=premult(fog.primary.color,fog.primary.maximum_density);
    const auto secondary=premult(fog.secondary.color,fog.secondary.maximum_density);
    if(fog.secondary.maximum_density==0 && w[54]==1) {
        w[17]=packed(eye_alpha,eye_rgb);w[43]=w[17];return result;
    }
    w={};
    w[0]=0x111a120a;w[1]=0x110a121a;w[2]=0x3c3d0820;w[3]=0x3d3c1d11;
    w[8]=0xc;w[9]=0x1c00;
    w[10]=packed(saturated((1.f-fog.interpolation)*fog.secondary.maximum_density),primary);
    w[11]=byte(fog.primary.maximum_density)<<24;w[13]=packed(eye_alpha,eye_rgb);
    w[18]=packed(saturated(fog.primary.maximum_density*fog.interpolation),secondary);
    w[19]=byte(fog.secondary.maximum_density)<<24;
    w[26]=w[27]=w[28]=0xcd;w[29]=0xc00;w[34]=0x010a021a;w[35]=0x0c3c0d3d;w[37]=0x3d0c1d01;
    w[45]=0xcd;w[46]=w[48]=0xc00;w[53]=0x11004;w[54]=0x2621;w[55]=0x44;
    const float pr=1.f/std::max(epsilon,fog.primary.opaque-fog.primary.start);
    const float sr=1.f/std::max(epsilon,fog.secondary.opaque-fog.secondary.start);
    result.lookup_coordinates={{{far*pr,0,-fog.primary.start*pr,0},{far*sr,0,-fog.secondary.start*sr,0}}};
    return result;
}
std::optional<unsigned> bsp_planar_fog_implementation(const BspFogSample &fog,bool containing) {
    // 1332F0 sets admission bits 0x80/0x100; 4CBF0 layer 16 consumes them.
    if(fog.classification!=3 || (!containing && fog.planar_only))return {};
    if(fog.planar_only)return 2;
    if(!containing && fog.eye_fraction==0 && !(fog.planar_flags&2))return 1;
    return 0;
}
PixelProgram bsp_planar_fog_program(const MaterialPass &pass,const BspFogSample &fog) {
    auto result=pass.program;const auto values=bsp_fog_externs(fog);
    remap_pixel_externs(result,values.scalars,values.colors,pass.pixel_externs,pass.fragments,{});
    return result;
}
void apply_bsp_fog_pixel_externs(PixelProgram &program,const MaterialPass &pass,
    const BspFogSample &fog,std::span<const std::int32_t> switches) {
    const auto values=bsp_fog_externs(fog);
    std::vector<ShaderRemap> mappings;
    for(const auto &mapping:pass.pixel_externs) {
        const auto component=mapping.component&7;
        const bool scalar=component<4 && (mapping.source==3 ||
            (mapping.source>=24 && mapping.source<=36));
        const bool color=component==4 && mapping.source>=25 && mapping.source<=32;
        if(scalar || color)mappings.push_back(mapping);
    }
    remap_pixel_externs(program,values.scalars,values.colors,mappings,pass.fragments,switches);
}
BspBloomFrame bsp_bloom_frame(const BspFogSample &fog,float amount,float brightness,float threshold_factor) {
    // Original 138FD/13905 each round to float before the final MULSS.
    const float screen_inverse=1.f-threshold_factor;
    const float screen_factor=1.f-screen_inverse;
    const float base_threshold=screen_factor*.8999999761581421f;
    const float base_strength=saturated((brightness+1.f)*amount*.5f);
    const float threshold=fog.bloom[0]>0?(fog.bloom[1]-base_threshold)*fog.bloom[0]+base_threshold:base_threshold;
    const float strength=fog.bloom[0]>0?(fog.bloom[2]-base_strength)*fog.bloom[0]+base_strength:base_strength;
    return {menu_bloom(threshold,strength),threshold,strength,fog.bloom[3]};
}
void prepare_bsp_planar_fog(const Cache &cache,Mesh &mesh,const BspAtmosphere &atmosphere) {
    if(atmosphere.fog_palette.empty())return;
    const auto second=cache.load_bitmap(atmosphere.gradient_datum,1);
    for(auto &material:mesh.materials) {
        if(material.lods.empty())continue;
        if(material.selected_shader_lod>=material.lods.size())
            throw FormatError("BSP planar shader LOD outside authored template");
        const auto &lod=material.lods[material.selected_shader_lod];
        if(!(lod.available_layers&(1u<<16)) || lod.layers.count()<=16)continue;
        const auto layer=lod.layers.first()+16;
        if(layer>=material.layers.size())throw FormatError("BSP planar layer outside template");
        const auto range=material.layers[layer];
        for(unsigned implementation=0;implementation<3;++implementation) {
            const auto selected=std::find_if(material.passes.begin(),material.passes.end(),[&](const auto &pass){
                return pass.template_pass>=range.first() && pass.template_pass<range.first()+range.count() && pass.implementation==implementation;
            });
            if(selected==material.passes.end())throw FormatError("BSP planar implementation missing");
            const auto &pass=*selected;
            if(pass.name!="shaders\\shader_passes\\fog\\fog_planar" || !pass.fragments.empty())throw FormatError("unsupported BSP planar fog program");
            auto target=std::make_shared<Material>();target->name=material.name+" [planar fog "+std::to_string(implementation)+"]";
            target->forward_pass=true;target->forward_vertex=5;target->forward_blend=5;
            target->forward_pass_index=0;target->passes.push_back(pass);
            target->forward_program=pass.program;target->vertex_program=pass.vertex_code;target->program_constants=pass.constants;
            target->vertex_declaration=pass.vertex_declaration;
            const Texture zero{1,1,11,std::vector<std::byte>(4),{}};
            target->diffuse=target->detail=target->bump=target->auxiliary=zero;
            for(const auto &texture:pass.textures) {
                if(texture.stage>1 || (texture.external!=14 && texture.external!=15 && texture.external!=0))throw FormatError("unsupported BSP planar fog texture extern");
                if(texture.external==14)target->diffuse=atmosphere.gradient;
                if(texture.external==15)target->detail=second;
            }
            target->forward_raster.depth_function=0x202;target->forward_raster.depth_write=0;
            for(const auto &state:pass.states) {
                if(state.index==57)target->forward_raster.depth_function=state.value;
                if(state.index==64)target->forward_raster.depth_write=state.value;
                if(state.index==67)target->forward_raster.color_write=state.value;
                if((state.index==62 && state.value!=0x8001) || (state.index==63 && state.value!=0x303) ||
                   (state.index==75 && state.value!=0x00ffffff))throw FormatError("unsupported BSP planar fog blend");
            }
            std::copy(pass.texture_states.begin(),pass.texture_states.end(),target->texture_states.begin());
            material.world_planar_passes[implementation]=std::move(target);
        }
    }
}
} // namespace halo2
