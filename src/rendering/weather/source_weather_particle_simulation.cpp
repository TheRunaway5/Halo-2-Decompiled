#include "halo2/source_weather_particle_simulation.hpp"
#include <array>
#include <bit>
#include <cmath>
#include <cfenv>
#include <cfloat>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>
#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
std::uint32_t word(std::span<const std::byte> b,std::size_t p) {
    std::uint32_t v;std::memcpy(&v,b.data()+p,4);return v;
}
float scalar(std::span<const std::byte> b,std::size_t p) {
    return std::bit_cast<float>(word(b,p));
}
void store(std::span<std::byte> b,std::size_t p,std::uint32_t v) {
    std::memcpy(b.data()+p,&v,4);
}
void put(std::span<std::byte> b,std::size_t p,float v) {
    if(!std::isfinite(v))throw std::invalid_argument("weather particle derived exceptional float is outside admitted controls");
    store(b,p,std::bit_cast<std::uint32_t>(v));
}
std::int32_t cvtt(float v) {
    if(!std::isfinite(v) || v>=2147483648.f || v< -2147483648.f)
        return std::numeric_limits<std::int32_t>::min();
    return static_cast<std::int32_t>(v);
}
void finite(float v) {
    if(!std::isfinite(v))throw std::invalid_argument("weather particle exceptional float is outside admitted source controls");
}
void arithmetic_environment() {
    static_assert(FLT_RADIX==2 && std::numeric_limits<float>::digits==24 && std::numeric_limits<double>::digits==53);
    if constexpr(FLT_EVAL_METHOD!=0)
        throw std::invalid_argument("weather particle native excess-precision evaluation is outside admitted controls");
    if(std::fegetround()!=FE_TONEAREST)
        throw std::invalid_argument("weather particle requires nearest native arithmetic");
#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86)
    if((_mm_getcsr()&~0x3fu)!=0x1f80)
        throw std::invalid_argument("weather particle requires masked SSE nearest/noDAZ/noFTZ execution");
#else
    throw std::invalid_argument("weather particle native floating-control inspection is unproved on this host");
#endif
}
std::array<float,3> point(std::span<const std::byte> b,std::size_t p) {
    return {scalar(b,p),scalar(b,p+4),scalar(b,p+8)};
}
float squared_zyx(const std::array<float,3> &p) {
    return (p[2]*p[2]+p[1]*p[1])+p[0]*p[0];
}
}
static SourceWeatherParticleStepResult step_ready_owned(
    std::span<const std::byte> system,std::span<std::byte> render,
    std::span<std::byte> state,const SourceWeatherParticleSimulationInput &in,
    std::uint32_t &auxiliary) {
    if(system.size()!=140 || in.view.size()!=244 || in.weather.size()<176 ||
       render.size()%20 || state.size()%32 || render.size()/20!=state.size()/32 ||
       render.size()/20>std::size_t(std::numeric_limits<std::int32_t>::max()))
        throw std::invalid_argument("weather particle step requires complete owned source banks");
    if(!in.rsqrt)throw std::invalid_argument("weather particle step needs an explicit source RSQRTSS producer");
    if(in.x87_control!=0x023f || (in.mxcsr&~0x3fu)!=0x1f80)
        throw std::invalid_argument("weather particle step supports actual023F/nearest masked SSE controls only");
    std::int16_t slot;std::memcpy(&slot,in.view.data(),2);
    const auto views=std::int32_t(in.view_count<1?1:in.view_count>4?4:in.view_count);
    if(slot<0 || slot>=views)
        throw std::invalid_argument("weather particle view partition is outside owned row storage");
    finite(in.elapsed);finite(in.intensity);finite(in.portal_distance);
    for(std::size_t p=8;p<=0x38;p+=4)finite(scalar(system,p));
    finite(scalar(system,0x80));
    for(std::size_t p=0x18;p<=0x20;p+=4)finite(scalar(in.view,p));
    for(std::size_t p=0x8c;p<=0xf0;p+=4)finite(scalar(in.view,p));
    for(const auto p:{0x30u,0x34u,0x38u,0x90u,0x94u,0x98u,0x9cu,0xa8u,0xacu})finite(scalar(in.weather,p));
    const auto count=std::int32_t(render.size()/20),quotient=count/views;
    const auto first=std::int32_t(slot)*quotient;
    const auto desired=cvtt(float(quotient)*in.intensity);
    for(std::int32_t i=first;i<first+quotient;++i) {
        const auto rp=std::size_t(i)*20,sp=std::size_t(i)*32;
        for(std::size_t p=0;p<16;p+=4)finite(scalar(render,rp+p));
        for(std::size_t p=0;p<32;p+=4)finite(scalar(state,sp+p));
    }
    const auto camera=point(in.view,0x18);
    const std::array<float,3> extent={scalar(system,8),scalar(system,12),scalar(system,16)};
    float maximum=extent[0]>extent[1]?extent[0]:extent[1];
    if(!(maximum>extent[2]))maximum=extent[2];
    std::vector<const SourceWeatherExclusionVolume *> selected;
    for(const auto &volume:in.exclusions) {
        finite(volume.radius);for(auto v:volume.center)finite(v);
        if(volume.plane_count>0 && std::uint32_t(volume.plane_count)>volume.planes.size())
            throw std::invalid_argument("weather particle exclusion plane bank is incomplete");
        for(std::int32_t p=0;p<volume.plane_count;++p)for(auto v:volume.planes[std::size_t(p)])finite(v);
        std::array<float,3> d;
        for(unsigned a=0;a<3;++a)d[a]=volume.center[a]-camera[a];
        const float radius=volume.radius+maximum;
        if(radius*radius>squared_zyx(d))selected.push_back(&volume);
    }
    if(selected.size()>32)throw std::invalid_argument("weather particle source32-entry exclusion scratch would overflow");
    float portal_alpha=1.f;
    if(in.portal_distance>0.f) {
        const float distance=scalar(in.weather,0xac);
        if(distance>0.f) {
            float fraction=in.portal_distance/distance;
            if(!(1.f>fraction))fraction=1.f;
            portal_alpha=1.f-fraction;
        } else portal_alpha=0.f;
    }
    SourceWeatherParticleStepResult result{true};
    auto random=[&] {
        auxiliary=auxiliary*0x19660du+0x3c6ef35fu;++result.auxiliary_draws;
        // ActualFCW023F selects PC53: the x87 FILD/FMUL/FADD chain retains
        // double precision until its explicit float store, unlike SSE scalars.
        return double(auxiliary>>16)*std::bit_cast<float>(0x37800080u);
    };
    auto symmetric=[&] {const double r=random();return (r+r)-1.;};
    std::int32_t active=0;
    for(std::int32_t i=first;i<first+quotient;++i) {
        ++result.visited;
        const auto rp=std::size_t(i)*20,sp=std::size_t(i)*32;
        const float age=scalar(state,sp+28);
        if(age<0.f) {
            if(active<desired) {
                store(render,rp+16,0xffffffffu);
                const double mass_range=double(scalar(system,0x20))-scalar(system,0x1c);
                put(state,sp+24,random()*mass_range+scalar(system,0x1c));
                for(unsigned a=0;a<3;++a)put(state,sp+a*4,random()*scalar(system,0x30+a*4));
                const double size_range=double(scalar(system,0x28))-scalar(system,0x24);
                put(render,rp+12,random()*size_range+scalar(system,0x24));
                put(state,sp+28,0.f);
                for(unsigned a=0;a<3;++a)put(render,rp+a*4,symmetric()*extent[(a+2)%3]+camera[a]);
                for(unsigned a=0;a<3;++a)put(state,sp+12+a*4,symmetric());
            }
            continue;
        }
        put(state,sp+28,age+in.elapsed);
        for(const auto axis:{2u,0u,1u}) {
            float value=symmetric()*scalar(in.weather,0xa8)+scalar(state,sp+12+axis*4);
            if(-1.f>value)value=-1.f;else if(value>1.f)value=1.f;
            put(state,sp+12+axis*4,value);
        }
        float x=scalar(in.weather,0x90)*scalar(state,sp+12)+scalar(in.weather,0x30);
        float y=scalar(in.weather,0x94)*scalar(state,sp+16)+scalar(in.weather,0x34);
        float z=scalar(in.weather,0x98)*scalar(state,sp+20)+scalar(in.weather,0x38);
        float mass=scalar(state,sp+24);
        if(!(mass>.0001f)) {
            mass=float(random()*std::bit_cast<float>(0x3f333333u)+std::bit_cast<float>(0x3dcccccdu));
            put(state,sp+24,mass);
        }
        const float inverse_mass=1.f/mass;
        x=inverse_mass*x;y=inverse_mass*y;z=inverse_mass*z;
        z=z-scalar(in.weather,0x9c);
        x=in.elapsed*x+scalar(state,sp);
        y=in.elapsed*y+scalar(state,sp+4);
        z=in.elapsed*z+scalar(state,sp+8);
        put(state,sp,x);put(state,sp+4,y);put(state,sp+8,z);
        const float speed=(z*z+y*y)+x*x,limit=scalar(system,0x18);
        finite(speed);
        if(speed>limit*limit && speed>std::bit_cast<float>(0x322bcc76u)) {
            const auto answer=in.rsqrt(std::size_t(i),std::bit_cast<std::uint32_t>(speed));
            arithmetic_environment();
            const float reciprocal=std::bit_cast<float>(answer);
            if(!std::isfinite(reciprocal) || !(reciprocal>0.f))
                throw std::invalid_argument("weather particle source RSQRTSS answer must be finite and positive");
            ++result.rsqrt_calls;const float factor=reciprocal*limit;
            put(state,sp,scalar(state,sp)*factor);
            put(state,sp+4,scalar(state,sp+4)*factor);
            put(state,sp+8,scalar(state,sp+8)*factor);
        }
        for(unsigned a=0;a<3;++a)put(render,rp+a*4,scalar(state,sp+a*4)*in.elapsed+scalar(render,rp+a*4));
        auto p=point(render,rp);
        const auto v=in.view;
        const float scale=scalar(v,0x8c);
        if(scale!=1.f)for(auto &a:p)a=a*scale;
        float local_x=((scalar(v,0xa8)*p[2]+p[1]*scalar(v,0x9c))+p[0]*scalar(v,0x90))+scalar(v,0xb4);
        float local_y=((scalar(v,0xa0)*p[1]+scalar(v,0x94)*p[0])+scalar(v,0xac)*p[2])+scalar(v,0xb8);
        float local_z=((scalar(v,0xa4)*p[1]+scalar(v,0x98)*p[0])+scalar(v,0xb0)*p[2])+scalar(v,0xbc);
        if(extent[0]>.0001f && extent[1]>.0001f && extent[2]>.0001f) {
            bool wrapped=false;
            float value=local_x/extent[0];value=value-float(cvtt(value));
            if(-.5f>value){value=value+1.f;wrapped=true;}
            else if(value>.5f){value=value-1.f;wrapped=true;}
            local_x=value*extent[0];
            value=local_y/extent[1];value=value-float(cvtt(value));
            if(-.5f>value){value=value+1.f;wrapped=true;}
            else if(value>.5f){value=value-1.f;wrapped=true;}
            local_y=value*extent[1];
            if(0.f-extent[2]>local_z)wrapped=true;
            value=local_z/extent[2];value=value-float(cvtt(value));
            if(value>0.f){value=value-1.f;wrapped=true;}
            local_z=value*extent[2];
            if(wrapped) {
                const float world_scale=scalar(v,0xc0);
                if(world_scale!=1.f){local_x=local_x*world_scale;local_y=local_y*world_scale;local_z=local_z*world_scale;}
                put(render,rp,((scalar(v,0xdc)*local_z+scalar(v,0xd0)*local_y)+scalar(v,0xc4)*local_x)+scalar(v,0xe8));
                put(render,rp+4,((scalar(v,0xe0)*local_z+scalar(v,0xd4)*local_y)+scalar(v,0xc8)*local_x)+scalar(v,0xec));
                put(render,rp+8,((scalar(v,0xe4)*local_z+scalar(v,0xd8)*local_y)+scalar(v,0xcc)*local_x)+scalar(v,0xf0));
            } else {
                std::uint16_t type;std::memcpy(&type,system.data()+0x6c,2);
                const float distance=(local_z*local_z+local_y*local_y)+local_x*local_x;
                if(type!=2 && std::bit_cast<float>(0x3d23d70bu)>distance) {
                    const double opacity=((double(distance)*std::bit_cast<float>(0x41c7ffffu))*portal_alpha)*256.;
                    // Original__ftol2 consumes the unstored PC53 x87 value.
                    const auto alpha=std::isfinite(opacity) && opacity<9223372036854775808. && opacity>=-9223372036854775808.
                        ?std::uint32_t(static_cast<std::int64_t>(opacity)):0u;
                    store(render,rp+16,(alpha<<24)|0x00ffffffu);
                }
            }
        }
        const float near=scalar(system,0x14);
        p=point(render,rp);std::array<float,3> delta;
        for(unsigned a=0;a<3;++a)delta[a]=camera[a]-p[a];
        bool excluded=near>0.f && near*near>squared_zyx(delta);
        if(!excluded)for(auto volume:selected)if(source_weather_point_in_exclusion(p,*volume)){excluded=true;break;}
        if(active>desired || excluded)put(state,sp+28,-1.f);else ++active;
    }
    return result;
}
void source_weather_advance_particle_elapsed(std::span<std::byte> system,float elapsed) {
    arithmetic_environment();
    if(system.size()!=140)
        throw std::invalid_argument("weather particle elapsed store requires complete source system");
    finite(elapsed);finite(scalar(system,0x80));
    put(system,0x80,scalar(system,0x80)+elapsed);
}
SourceWeatherParticleStepResult source_weather_step_ready_particles(
    std::span<const std::byte> system,std::span<std::byte> render,
    std::span<std::byte> state,const SourceWeatherParticleSimulationInput &in,
    std::uint32_t &auxiliary) {
    auto overlaps=[](std::span<const std::byte> a,std::span<const std::byte> b) {
        if(a.empty() || b.empty())return false;
        const auto x=reinterpret_cast<std::uintptr_t>(a.data()),y=reinterpret_cast<std::uintptr_t>(b.data());
        return x<y?y-x<a.size():x-y<b.size();
    };
    arithmetic_environment();
    if(overlaps(system,render) || overlaps(system,state) || overlaps(render,state))
        throw std::invalid_argument("weather particle mutable source banks must not overlap");
    const std::array<std::span<const std::byte>,3> banks={system,render,state};
    for(auto owned:banks) {
        if(overlaps(owned,in.view) || overlaps(owned,in.weather) || overlaps(owned,std::as_bytes(in.exclusions)) ||
           overlaps(owned,std::as_bytes(std::span(&auxiliary,1))))
            throw std::invalid_argument("weather particle mutable and immutable source banks must not overlap");
        for(const auto &v:in.exclusions)if(overlaps(owned,std::as_bytes(v.planes)))
            throw std::invalid_argument("weather particle mutable and exclusion plane banks must not overlap");
    }
    const auto auxiliary_bytes=std::as_bytes(std::span(&auxiliary,1));
    if(overlaps(auxiliary_bytes,in.view) || overlaps(auxiliary_bytes,in.weather) || overlaps(auxiliary_bytes,std::as_bytes(in.exclusions)))
        throw std::invalid_argument("weather particle auxiliary RNG must have exclusive owned storage");
    for(const auto &v:in.exclusions)if(overlaps(auxiliary_bytes,std::as_bytes(v.planes)))
        throw std::invalid_argument("weather particle auxiliary RNG and plane storage must not overlap");
    std::vector<std::byte> next_render(render.begin(),render.end()),next_state(state.begin(),state.end());
    auto next_auxiliary=auxiliary;
    const auto result=step_ready_owned(system,next_render,next_state,in,next_auxiliary);
    std::copy(next_render.begin(),next_render.end(),render.begin());
    std::copy(next_state.begin(),next_state.end(),state.begin());
    auxiliary=next_auxiliary;return result;
}
} // namespace halo2
