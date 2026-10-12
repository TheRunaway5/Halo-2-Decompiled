#include "halo2/source_weather_assets.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>

namespace halo2 {
namespace {

Vec3 vector(const Bytes &b,std::size_t p){return {b.f32(p),b.f32(p+4),b.f32(p+8)};}
template<std::size_t N> std::array<std::byte,N> original(const Bytes&b,std::size_t p){
    std::array<std::byte,N> out;const auto raw=b.slice(p,N);
    std::copy(raw.begin(),raw.end(),out.begin());return out;
}
std::uint32_t rain_alpha(float opacity){
    // The original keeps FLD/FMULT's extended result until __ftol2. Do not
    // round the product to float before truncating to the packed byte.
    const double value=double(opacity)*256.;
    if(!std::isfinite(value) || value<-2147483648. || value>=2147483648.)
        throw FormatError("weather opacity outside source conversion range");
    return std::uint32_t(std::int32_t(value))<<24;
}
template<class Rows> SourceWeatherSubmission immediate(std::size_t capacity,unsigned type,
    float width,std::array<float,2> opacity,float streak,float elapsed,
    std::int16_t views,std::int16_t view,Rows rows) {
    if(!std::isfinite(elapsed))throw FormatError("weather immediate elapsed is nonfinite");
    const unsigned count=std::clamp<int>(views,1,4);
    if(view<0 || unsigned(view)>=count)throw FormatError("weather source view outside partition");
    const auto per_view=capacity/count,first=unsigned(view)*per_view;
    SourceWeatherSubmission out;out.lines=type==2;out.line_width=width;
    const auto alpha0=rain_alpha(opacity[0]),alpha1=rain_alpha(opacity[1]);
    for(std::size_t i=first;i<first+per_view;++i) {
        const auto [r,s]=rows(i);if(!(s.age>0.f))continue;
        if(out.lines) {
            out.vertices.push_back({{r.position.x-(s.velocity.x*elapsed)*streak,
                r.position.y-(s.velocity.y*elapsed)*streak,
                r.position.z-(s.velocity.z*elapsed)*streak,1.f},(r.color&0xffffffu)|alpha0});
            out.vertices.push_back({{r.position.x,r.position.y,r.position.z,1.f},(r.color&0xffffffu)|alpha1});
        }else out.vertices.push_back({{r.position.x,r.position.y,r.position.z,r.size*16777215.f},r.color});
    }
    return out;
}
}
SourceWeatherDefinition Cache::load_source_weather_definition(std::uint32_t datum) const {
    const auto found=query_tag({.handle=TagHandle{datum}, .group=tag_groups::weather_system, .require_address=true});
    if(!found || found->size<0xb0)throw FormatError("missing/truncated original weather tag");
    const Bytes b(storage_);const auto p=metadata_offset(found->address,0xb0);
    SourceWeatherDefinition out;out.datum=datum;out.name=found->name;
    out.authored=original<0xb0>(b,p);
    std::array<std::uint32_t,39> wind;
    for(unsigned i=0;i<wind.size();++i)wind[i]=b.u32(p+0x10+i*4);
    out.wind=std::bit_cast<SourceWeatherWind>(wind);out.fade_radius=b.f32(p+0xac);
    const auto particles=TagAddressSpace(b, metadata_magic_).block(p, 0x8c, 64);
    for(std::size_t i=0;i<particles.count;++i){
        const auto r=particles.offset+i*0x8c;auto &q=out.particles.emplace_back();
        q.authored=original<0x8c>(b,r);q.sprites=b.u32(r+4);q.view_box=vector(b,r+8);
        q.exclusion_radius=b.f32(r+0x14);q.maximum_velocity=b.f32(r+0x18);
        q.mass={b.f32(r+0x1c),b.f32(r+0x20)};q.size={b.f32(r+0x24),b.f32(r+0x28)};
        q.maximum_particles=b.u32(r+0x2c);q.initial_velocity=vector(b,r+0x30);
        q.animation_speed=b.f32(r+0x3c);q.type=b.u16(r+0x6c);
        q.opacity={b.f32(r+0x70),b.f32(r+0x74)};
        q.streak_scale=b.f32(r+0x78);q.line_width=b.f32(r+0x7c);
        if(q.type>7 || q.maximum_particles>1000000)throw FormatError("weather particle metadata outside source bounds");
        const auto resource_size=b.u32(r+0x44);
        if(!resource_size){
            if(q.maximum_particles || b.u32(r+0x50))throw FormatError("weather particle resource missing");
            continue;
        }
        const auto raw=raw_data(b.u32(r+0x40),resource_size);const Bytes data(raw);
        q.resource_storage=raw;
        // 223C70 fixes up count/pointer pairs at raw+8 using the actual
        // descriptor locators and section size; it does not invent buffers.
        if(data.u32(0)!=0x626c6b68u || data.u32(resource_size-4)!=0x626c6b66u)
            throw FormatError("weather resource wrapper differs from original");
        const auto header=b.u32(r+0x48);
        if(std::uint64_t(header)+8>raw.size())throw FormatError("weather resource section truncated");
        const auto resources=TagAddressSpace(b, metadata_magic_).block(r+0x50, 16, 128);
        bool render=false,state=false;
        for(std::size_t j=0;j<resources.count;++j){
            const auto d=resources.offset+j*16;SourceWeatherResource retained;
            retained.descriptor=original<16>(b,d);
            const auto at=std::uint64_t(header)+8+b.u32(d+12);
            const auto size=b.u32(d+8);
            if(at>raw.size())throw FormatError("weather resource offset outside wrapper");
            const auto payload=data.slice(std::size_t(at),size);
            retained.bytes.assign(payload.begin(),payload.end());q.resources.push_back(std::move(retained));
            const auto kind=std::to_integer<unsigned>(b.slice(d,1)[0]);
            const auto locator=b.u16(d+4),stride=b.u16(d+6);
            if(kind!=0 || (locator!=0 && locator!=8))throw FormatError("unsupported weather particle resource declaration");
            if(locator==0){
                if(render || stride!=20 || size%20)throw FormatError("weather render resource declaration differs");
                render=true;const auto count=size/20;
                if(data.u32(8)!=count)throw FormatError("weather render resource count differs");
                for(unsigned k=0;k<count;++k){const auto v=std::size_t(at)+k*20;
                    q.render.push_back({vector(data,v),data.f32(v+12),data.u32(v+16)});}
            }else{
                if(state || stride!=32 || size%32)throw FormatError("weather state resource declaration differs");
                state=true;const auto count=size/32;
                if(data.u32(16)!=count)throw FormatError("weather state resource count differs");
                for(unsigned k=0;k<count;++k){const auto v=std::size_t(at)+k*32;
                    q.state.push_back({vector(data,v),vector(data,v+12),data.f32(v+24),data.f32(v+28)});}
            }
        }
        if(!render || !state || q.render.size()!=q.state.size() || q.render.size()!=q.maximum_particles)
            throw FormatError("weather particle arrays do not match authored capacity");
    }
    const auto plates=TagAddressSpace(b, metadata_magic_).block(p+8, 0x3a8, 64);
    for(std::size_t i=0;i<plates.count;++i){
        const auto r=plates.offset+i*0x3a8;auto &q=out.plates.emplace_back();
        q.authored=original<0x3a8>(b,r);
        for(unsigned j=0;j<3;++j){q.textures[j]=b.u32(r+j*8+4);q.positions[j]=b.f32(r+0x18+j*4);
            q.velocity[j]=vector(b,r+0x24+j*12);q.texture_scale[j]=b.f32(r+0x48+j*4);
            q.jitter[j]=vector(b,r+0x54+j*12);q.opacity[j]=b.f32(r+0x88+j*4);
            q.tint[j]=vector(b,r+0x98+j*12);q.mass[j]=b.f32(r+0xbc+j*4);}
        q.depth={b.f32(r+0x78),b.f32(r+0x7c)};q.depth_blend={b.f32(r+0x80),b.f32(r+0x84)};
        q.flags=b.u32(r+0x94);
    }
    const auto globals=query_tag({.group=tag_groups::globals, .require_address=true});
    if(!globals)throw FormatError("original weather rasterizer globals missing");
    const auto g=metadata_offset(globals->address,0x110);
    const auto rasterizer=TagAddressSpace(b, metadata_magic_).block(g+0x108, 0x108);
    if(rasterizer.count!=1)throw FormatError("original weather rasterizer data missing");
    const auto shaders=TagAddressSpace(b, metadata_magic_).block(rasterizer.offset+0x58, 8);
    if(shaders.count<=22)throw FormatError("original weather global vertex shader missing");
    for(unsigned j=0;j<2;++j){out.vertex_datums[j]=b.u32(shaders.offset+(21+j)*8+4);
        out.vertex_programs[j]=load_vertex_shader_code(out.vertex_datums[j],0);}
    return out;
}
SourceWeatherBsp Cache::load_source_weather_bsp(std::size_t index) const {
    if(index>=bsps_.size())throw FormatError("weather BSP outside scenario");
    const auto &bsp=bsps_[index];const Bytes b(Bytes(storage_).slice(bsp.file_offset,bsp.size));
    SourceWeatherBsp out;const auto palettes=TagAddressSpace(b, bsp.memory_address).block(16+0x84, 0x88, 16);
    for(std::size_t i=0;i<palettes.count;++i){const auto p=palettes.offset+i*0x88;
        out.palette.push_back({original<0x88>(b,p),b.string(p,32),b.u32(p+0x24)});}
    const auto clusters=TagAddressSpace(b, bsp.memory_address).block(16+0x9c, 0xb0);
    for(std::size_t i=0;i<clusters.count;++i){const auto index=std::int16_t(b.u16(clusters.offset+i*0xb0+0x76));
        if(index<-1 || (index>=0&&unsigned(index)>=out.palette.size()))throw FormatError("weather cluster palette outside BSP");
        out.cluster_palette.push_back(index);}
    const auto polyhedra=TagAddressSpace(b, bsp.memory_address).block(16+0x8c, 24, 32);
    for(std::size_t i=0;i<polyhedra.count;++i){const auto p=polyhedra.offset+i*24;
        SourceWeatherBsp::Polyhedron q{vector(b,p),b.f32(p+12),{}};
        const auto planes=TagAddressSpace(b, bsp.memory_address).block(p+16, 16);
        for(std::size_t j=0;j<planes.count;++j){const auto p=planes.offset+j*16;
            q.planes.push_back({b.f32(p),b.f32(p+4),b.f32(p+8),b.f32(p+12)});}
        out.polyhedra.push_back(std::move(q));}
    return out;
}
std::vector<SourceWeatherAdmission> source_weather_admission(const SourceWeatherBsp &b,
    const BspAtmosphere &a,int current,std::span<const unsigned> visible,Vec3 eye){
    if(b.palette.size()>16 || a.clusters.size()!=b.cluster_palette.size())throw FormatError("weather visibility bank differs from BSP");
    std::vector<SourceWeatherAdmission> out(b.palette.size());
    if(current==-1 || b.palette.empty())return out;
    if(current<0 || unsigned(current)>=a.clusters.size())throw FormatError("weather camera cluster outside BSP");
    for(const auto cluster:visible){
        if(cluster>=b.cluster_palette.size())throw FormatError("weather visible cluster outside BSP");
        const int index=b.cluster_palette[cluster];
        if(index==-1)continue;
        if(index<0 || unsigned(index)>=b.palette.size())throw FormatError("weather visible palette outside BSP");
        if(b.palette[index].datum==0xffffffffu)continue;
        auto &selected=out[index];selected.visible=true;
        if(cluster==unsigned(current)){selected.portal_distance=-1.f;continue;}
        float distance=32767.f;
        for(const auto portal_index:a.clusters[current].portals){
            if(portal_index>=a.portals.size())throw FormatError("weather portal outside BSP");
            const auto &portal=a.portals[portal_index];
            if(portal.back!=int(cluster)&&portal.front!=int(cluster))continue;
            if(portal.plane>=a.planes.size())throw FormatError("weather portal plane outside BSP");
            const auto &plane=a.planes[portal.plane];
            float value=0.f-((plane[1]*eye.y+plane[2]*eye.z+eye.x*plane[0])-plane[3]);
            if(value<0.f)value=0.f-value;
            if(value<distance)distance=value;
        }
        if(distance<selected.portal_distance)selected.portal_distance=distance;
    }
    return out;
}
SourceWeatherSubmission source_weather_submission(const SourceWeatherParticles &p,float elapsed,
    std::int16_t views,std::int16_t view){
    if(p.render.size()!=p.state.size())throw FormatError("weather immediate input bank differs");
    return immediate(p.render.size(),p.type,p.line_width,p.opacity,p.streak_scale,elapsed,views,view,
        [&](std::size_t i){return std::pair{p.render[i],p.state[i]};});
}
SourceWeatherSubmission source_weather_submission(std::span<const std::byte> system,
    std::span<const std::byte> render,std::span<const std::byte> state,float elapsed,
    std::int16_t views,std::int16_t view) {
    if(system.size()!=140 || render.size()%20 || state.size()%32 || render.size()/20!=state.size()/32)
        throw FormatError("weather immediate runtime banks differ");
    const Bytes b(system);
    return immediate(render.size()/20,b.u16(0x6c),b.f32(0x7c),
        std::array{b.f32(0x70),b.f32(0x74)},b.f32(0x78),elapsed,views,view,[&](std::size_t i){
            SourceWeatherParticleRender r;SourceWeatherParticleState s;
            std::memcpy(&r,render.data()+i*20,20);std::memcpy(&s,state.data()+i*32,32);
            return std::pair{r,s};
        });
}
PixelProgram source_weather_pixel_program(bool rain){
    PixelProgram p;p.words[0]=rain?0xd4301010u:0xd4db1010u;
    p.words[8]=0x0c200000u;p.words[9]=0x1c80u;p.words[26]=0xc0u;
    p.words[34]=rain?0xc4200000u:0xc4cb0000u;p.words[45]=0xc0u;
    p.words[53]=0x11101u;p.words[54]=rain?0u:0x8000u;
    p.words[57]=p.words[58]=0xffffffffu;p.words[59]=0x1ffu;return p;
}
std::array<ShaderVector,2> source_weather_viewport_constants(std::int16_t left,
    std::int16_t top,std::int16_t right,std::int16_t bottom,int mode){
    float width=float(int(right)-int(left)),height=float(int(bottom)-int(top));
    float x=float(left),y=float(top);
    if(mode==2){width=height=128.f;x=y=0.f;}
    return {{{width*.5f,height*-.5f,16777215.f,0.f},
        {width*.5f+x+.5f,height*.5f+y+.5f,0.f,0.f}}};
}
}
