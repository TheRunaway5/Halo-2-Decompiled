#include "halo2/native_tu15_affine_inverse_value.hpp"
#include "halo2/native_tu15_camera_projection.hpp"
#include <bit>
#include <cstring>
#include <xmmintrin.h>
#include <emmintrin.h>
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory &memory,std::uint32_t address,unsigned width=4) {
    std::array<std::byte,4> raw{};memory.read(address,std::span(raw).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<std::uint32_t>(raw[n])<<(8u*n);return value;
}
void publish(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRecordObserver &observe,
    std::uint32_t site,std::uint32_t address,std::uint32_t value) {
    SourcePhysicsContactRecordStore event{site,address,{},std::vector<std::byte>(4)};
    if(observe){event.before.resize(4);memory.read(address,event.before);}
    for(unsigned n=0;n<4;++n)event.written[n]=std::byte(value>>(8u*n));
    memory.write(address,event.written);if(observe)observe(event);
}
using Scalar=__m128;
Scalar scalar(std::uint32_t value){return _mm_castsi128_ps(_mm_cvtsi32_si128(std::bit_cast<std::int32_t>(value)));}
std::uint32_t bits(Scalar value){return std::bit_cast<std::uint32_t>(_mm_cvtsi128_si32(_mm_castps_si128(value)));}
std::uint32_t add(std::uint32_t a,std::uint32_t b){auto x=scalar(a);const auto y=scalar(b);asm volatile("addss %1,%0":"+x"(x):"x"(y):"memory");return bits(x);}
std::uint32_t sub(std::uint32_t a,std::uint32_t b){auto x=scalar(a);const auto y=scalar(b);asm volatile("subss %1,%0":"+x"(x):"x"(y):"memory");return bits(x);}
std::uint32_t mul(std::uint32_t a,std::uint32_t b){auto x=scalar(a);const auto y=scalar(b);asm volatile("mulss %1,%0":"+x"(x):"x"(y):"memory");return bits(x);}
std::uint32_t div(std::uint32_t a,std::uint32_t b){auto x=scalar(a);const auto y=scalar(b);asm volatile("divss %1,%0":"+x"(x):"x"(y):"memory");return bits(x);}
bool equal(std::uint32_t a,std::uint32_t b) {
    const auto x=scalar(a),y=scalar(b);unsigned char same,ordered;
    asm volatile("ucomiss %3,%2; sete %0; setnp %1":"=qm"(same),"=qm"(ordered):"x"(x),"x"(y):"cc","memory");return same&&ordered;
}
class SimdEnvironment {
    NativeTu15PhysicsFrameFp &fp_;unsigned host_;
public:
    explicit SimdEnvironment(NativeTu15PhysicsFrameFp &fp):fp_(fp),host_(_mm_getcsr()) {
        if((fp.mxcsr&0x1f80u)!=0x1f80u||(fp.mxcsr&0xffff0000u))throw std::invalid_argument("Native projection requires masked SIMD exceptions");
        _mm_setcsr(fp.mxcsr);
    }
    ~SimdEnvironment(){fp_.mxcsr=_mm_getcsr();_mm_setcsr(host_);}
};
using X87Image=std::array<std::byte,108>;
unsigned tag(const std::array<std::byte,10> &value) {
    std::uint64_t significand{};for(unsigned n=0;n<8;++n)significand|=std::uint64_t(std::to_integer<unsigned>(value[n]))<<(8u*n);
    const auto exponent=(std::to_integer<unsigned>(value[8])|(std::to_integer<unsigned>(value[9])<<8u))&0x7fffu;
    if(!exponent&&!significand)return 1;
    return !exponent||exponent==0x7fffu||!(significand>>63u)?2u:0u;
}
class ArithmeticEnvironment {
    SimdEnvironment simd_;alignas(16) X87Image host_{},native_{};
    NativeTu15PhysicsFrameFp &fp_;NativeTu15CameraProjectionX87State &state_;
public:
    ArithmeticEnvironment(NativeTu15PhysicsFrameFp &fp,NativeTu15CameraProjectionX87State &state):simd_(fp),fp_(fp),state_(state) {
        if((fp.control_word&0x3fu)!=0x3fu||(fp.status&0x8080u))
            throw std::invalid_argument("Native projection requires masked retained x87 exceptions");
        asm volatile("fnsave %0":"=m"(host_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        native_=host_;std::memcpy(native_.data(),&fp.control_word,2);std::memcpy(native_.data()+4,&fp.status,2);
        std::uint16_t tags=0xffffu;const auto top=(fp.status>>11u)&7u;
        for(unsigned n=0;n<8;++n) {
            const auto shift=((top+n)&7u)*2u;
            if(state.occupied&(1u<<n))tags=std::uint16_t((tags&~(3u<<shift))|(tag(state.values[n])<<shift));
            std::memcpy(native_.data()+28u+n*10u,state.values[n].data(),10);
        }
        std::memcpy(native_.data()+8,&tags,2);restore(native_);
    }
    ~ArithmeticEnvironment() {
        asm volatile("fnsave %0":"=m"(native_)::"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");
        std::memcpy(&fp_.control_word,native_.data(),2);std::memcpy(&fp_.status,native_.data()+4,2);
        std::uint16_t tags{};std::memcpy(&tags,native_.data()+8,2);const auto top=(fp_.status>>11u)&7u;state_.occupied=0;
        for(unsigned n=0;n<8;++n) {
            if(((tags>>(((top+n)&7u)*2u))&3u)!=3u)state_.occupied|=1u<<n;
            std::memcpy(state_.values[n].data(),native_.data()+28u+n*10u,10);
        }
        restore(host_);
    }
    static void restore(const X87Image &value){asm volatile("frstor %0"::"m"(value):"memory","st","st(1)","st(2)","st(3)","st(4)","st(5)","st(6)","st(7)");}
};
void load(std::uint32_t value){asm volatile("flds %0"::"m"(value):"memory","st");}
void multiply(std::uint32_t value){asm volatile("fmuls %0"::"m"(value):"memory","st");}
std::uint32_t spill(bool pop) {
    std::uint32_t value;
    if(pop)asm volatile("fstps %0":"=m"(value)::"memory","st");
    else asm volatile("fsts %0":"=m"(value)::"memory");
    return value;
}
std::uint32_t integer_float(std::int32_t value) {
    auto result=_mm_setzero_ps();asm volatile("cvtsi2ss %1,%0":"+x"(result):"r"(value):"memory");return bits(result);
}
enum class Field:unsigned {left,right,bottom,top,width,height,half_width,half_height,center_x,center_y,horizontal,vertical,clip_left,clip_right,clip_bottom,clip_top};
class ProgramStorage {
public:
    virtual ~ProgramStorage()=default;
    virtual std::uint32_t get(Field)const=0;
    virtual void set(Field,std::uint32_t,std::uint32_t site)=0;
};
class RetainedProgram final:public ProgramStorage {
    SourcePhysicsContactMemory &memory_;std::uint32_t address_;const SourcePhysicsContactRecordObserver &observe_;
public:
    RetainedProgram(SourcePhysicsContactMemory &memory,std::uint32_t address,const SourcePhysicsContactRecordObserver &observe):memory_(memory),address_(address),observe_(observe){}
    std::uint32_t get(Field field)const override{return read(memory_,address_+unsigned(field)*4u);}
    void set(Field field,std::uint32_t value,std::uint32_t site)override{publish(memory_,observe_,site,address_+unsigned(field)*4u,value);}
};
class ValueProgram final:public ProgramStorage {
    std::array<std::uint32_t,16> values_{};
public:
    std::uint32_t get(Field field)const override{return values_[unsigned(field)];}
    void set(Field field,std::uint32_t value,std::uint32_t)override{values_[unsigned(field)]=value;}
    NativeTu15CameraProjectionProgram value()const {
        return {{values_[0],values_[1],values_[2],values_[3]},values_[4],values_[5],values_[6],values_[7],values_[8],values_[9],values_[10],values_[11],
            {values_[12],values_[13],values_[14],values_[15]}};
    }
};
std::int32_t short_value(std::uint32_t value){return std::bit_cast<std::int16_t>(std::uint16_t(value));}
void projection_program(SourcePhysicsContactMemory &memory,std::uint32_t camera,
    std::uint32_t bounds,ProgramStorage &output) {
    const auto one=read(memory,0x46a0d0u);
    const auto right=short_value(read(memory,camera+0x36u,2)),left=short_value(read(memory,camera+0x32u,2));
    output.set(Field::width,integer_float(right-left),0x2fb69u);
    const auto bottom=short_value(read(memory,camera+0x34u,2)),top=short_value(read(memory,camera+0x30u,2));
    output.set(Field::height,integer_float(bottom-top),0x2fb7eu);
    if(!bounds) {
        const auto negative_one=read(memory,0x46a0dcu);
        output.set(Field::bottom,negative_one,0x2fb8du);output.set(Field::left,negative_one,0x2fb92u);
        output.set(Field::top,one,0x2fb96u);output.set(Field::right,one,0x2fb9bu);
    }else {
        for(unsigned n=0;n<4;++n) {
            const auto value=read(memory,bounds+n*4u);
            constexpr std::array<std::uint32_t,4> sites{0x2fba6u,0x2fbabu,0x2fbb1u,0x2fbb7u};
            output.set(Field(n),value,sites[n]);
        }
    }
    const auto half=read(memory,0x46a0ccu);
    auto half_width=output.get(Field::right);auto half_height=output.get(Field::top);
    half_height=sub(half_height,output.get(Field::bottom));half_width=sub(half_width,output.get(Field::left));
    auto center_y=output.get(Field::bottom);center_y=add(center_y,output.get(Field::top));
    const auto negative_half=read(memory,0x46a0d4u);
    half_height=mul(half_height,half);half_width=mul(half_width,half);
    auto center_x=output.get(Field::right);center_x=add(center_x,output.get(Field::left));
    center_y=div(center_y,half_height);output.set(Field::half_height,half_height,0x2fbfcu);
    center_x=div(center_x,half_width);center_x=mul(center_x,negative_half);center_y=mul(center_y,negative_half);
    output.set(Field::half_width,half_width,0x2fc0du);output.set(Field::center_x,center_x,0x2fc12u);output.set(Field::center_y,center_y,0x2fc17u);
    load(read(memory,camera+0x28u));multiply(read(memory,0x46a0ccu));
    const auto layout=read(memory,0x4f3218u);auto aspect=output.get(Field::width);
    asm volatile("fptan":::"memory","st","st(1)");
    aspect=div(aspect,output.get(Field::height));asm volatile("fstp %%st(0)":::"memory","st");
    auto tangent=spill(true);
    const bool excluded=layout&&read(memory,layout+1u,1)&&short_value(read(memory,layout+2u,2))!=-1&&read(memory,layout+8u)==3u;
    if(!excluded&&read(memory,0x4922e2u,1)) {
        const auto auxiliary=read(memory,0x51df50u),wide=read(memory,0x46a514u);aspect=mul(aspect,wide);
        if(auxiliary&&read(memory,auxiliary+5u,1)) {tangent=mul(tangent,wide);tangent=mul(tangent,read(memory,0x46a670u));}
    }else {
        const auto auxiliary=read(memory,0x51df50u);
        if(auxiliary&&read(memory,auxiliary+5u,1)) {
            auto scale=output.get(Field::height);scale=mul(scale,read(memory,0x46a260u));
            const auto adjusted_bottom=mul(output.get(Field::bottom),scale);
            const auto adjusted_top=mul(scale,output.get(Field::top));
            output.set(Field::bottom,adjusted_bottom,0x2fcbcu);output.set(Field::top,adjusted_top,0x2fcc1u);
        }
    }
    auto horizontal_denominator=mul(half_width,aspect);horizontal_denominator=mul(horizontal_denominator,tangent);
    const auto horizontal=div(one,horizontal_denominator);auto vertical_denominator=output.get(Field::half_height);
    output.set(Field::horizontal,horizontal,0x2fcdfu);vertical_denominator=mul(vertical_denominator,tangent);
    const auto horizontal_inverse=div(one,horizontal);
    auto clip_left=add(center_x,one);clip_left=mul(clip_left,horizontal_inverse);
    const auto vertical=div(one,vertical_denominator);
    const auto negative_clip_left=sub(0,clip_left);auto clip_right=sub(center_x,one);clip_right=mul(clip_right,horizontal_inverse);
    const auto negative_clip_right=sub(0,clip_right);output.set(Field::clip_right,negative_clip_right,0x2fd1au);
    auto clip_bottom=output.get(Field::center_y);clip_bottom=add(clip_bottom,one);output.set(Field::vertical,vertical,0x2fd28u);
    const auto vertical_inverse=div(one,vertical);clip_bottom=mul(clip_bottom,vertical_inverse);
    const auto negative_clip_bottom=sub(0,clip_bottom);auto clip_top=output.get(Field::center_y);clip_top=sub(clip_top,one);clip_top=mul(clip_top,vertical_inverse);
    const auto negative_clip_top=sub(0,clip_top);
    output.set(Field::clip_left,negative_clip_left,0x2fd50u);output.set(Field::clip_bottom,negative_clip_bottom,0x2fd55u);output.set(Field::clip_top,negative_clip_top,0x2fd5au);
}
using Vector=std::array<std::uint32_t,3>;
void inverse_frame(SourcePhysicsContactMemory &,std::uint32_t,std::uint32_t,const SourcePhysicsContactRecordObserver &);
void magnitude(const Vector &value) {
    load(value[2]);multiply(value[2]);load(value[1]);multiply(value[1]);
    asm volatile("faddp %%st,%%st(1)":::"memory","st","st(1)");
    load(value[0]);multiply(value[0]);
    asm volatile("faddp %%st,%%st(1);fsqrt":::"memory","st","st(1)");
}
bool magnitude_too_small(const SourcePhysicsContactMemory &memory,std::uint32_t &rounded) {
    rounded=spill(false);asm volatile("fabs":::"memory","st");
    load(read(memory,0x46a0ecu));unsigned char above;
    asm volatile("fcomip %%st(1),%%st; seta %0; fstp %%st(0)":"=qm"(above)::"memory","cc","st","st(1)");
    return above!=0;
}
bool positive(std::uint32_t value) {
    const auto x=scalar(value),zero=_mm_setzero_ps();unsigned char result;
    asm volatile("comiss %2,%1; seta %0":"=qm"(result):"x"(x),"x"(zero):"cc","memory");return result!=0;
}
void projection(SourcePhysicsContactMemory &memory,std::uint32_t camera,std::uint32_t bounds,
    std::uint32_t destination,const SourcePhysicsContactRecordObserver &observe) {
    ValueProgram program_storage;projection_program(memory,camera,bounds,program_storage);const auto program=program_storage.value();
    // Construct right, reconstructed up and backward axes from the retained
    // authored camera. Each source value remains lazy; the first norm is
    // deliberately staged across the intervening authored-vector reads.
    auto up_z=read(memory,camera+0x20u);auto up_y=read(memory,camera+0x1cu);
    up_y=mul(up_y,read(memory,camera+0x14u));up_z=mul(up_z,read(memory,camera+0x10u));
    auto z_x=read(memory,camera+0x20u);z_x=mul(z_x,read(memory,camera+0xcu));
    auto y_x=read(memory,camera+0x10u);y_x=mul(y_x,read(memory,camera+0x18u));
    Vector right;right[0]=sub(up_z,up_y);
    auto x_z=read(memory,camera+0x14u);x_z=mul(x_z,read(memory,camera+0x18u));right[1]=sub(x_z,z_x);
    auto y_z=read(memory,camera+0x1cu);y_z=mul(y_z,read(memory,camera+0xcu));right[2]=sub(y_z,y_x);
    load(right[2]);multiply(right[2]);load(right[1]);multiply(right[1]);
    const auto forward_z=read(memory,camera+0x14u),forward_y=read(memory,camera+0x10u),second_z=read(memory,camera+0x14u);
    asm volatile("faddp %%st,%%st(1)":::"memory","st","st(1)");
    const auto forward_x=read(memory,camera+0xcu);load(right[0]);
    const auto y_right_z=mul(forward_y,right[2]);multiply(right[0]);
    Vector up;up[0]=sub(mul(forward_z,right[1]),y_right_z);
    const auto second_x=read(memory,camera+0xcu);
    asm volatile("faddp %%st,%%st(1)":::"memory","st","st(1)");
    const auto z_right_x=mul(second_z,right[0]);up[1]=sub(mul(second_x,right[2]),z_right_x);
    asm volatile("fsqrt":::"memory","st");
    const auto second_y=read(memory,camera+0x10u);
    up[2]=sub(mul(second_y,right[0]),mul(forward_x,right[1]));
    Vector backward;
    backward[0]=sub(0,read(memory,camera+0xcu));backward[1]=sub(0,read(memory,camera+0x10u));backward[2]=sub(0,read(memory,camera+0x14u));
    const auto one=read(memory,0x46a0d0u);std::uint32_t length;
    if(!magnitude_too_small(memory,length)) {
        const auto reciprocal=div(one,length);
        right[0]=mul(reciprocal,right[0]);right[1]=mul(right[1],reciprocal);right[2]=mul(right[2],reciprocal);
    }
    magnitude(up);
    if(!magnitude_too_small(memory,length)) {
        const auto reciprocal=div(one,length);up[0]=mul(reciprocal,up[0]);up[1]=mul(up[1],reciprocal);up[2]=mul(up[2],reciprocal);
    }
    magnitude(backward);
    if(!magnitude_too_small(memory,length)) {
        const auto reciprocal=div(one,length);
        backward[0]=mul(reciprocal,backward[0]);backward[1]=mul(backward[1],reciprocal);backward[2]=mul(backward[2],reciprocal);
    }
    constexpr std::array<std::uint32_t,3> right_sites{0x2ff91u,0x2ff97u,0x2ff9au},up_sites{0x2ffa8u,0x2ffaeu,0x2ffb1u},back_sites{0x2ffbfu,0x2ffc5u,0x2ffc8u};
    for(unsigned n=0;n<3;++n)publish(memory,observe,right_sites[n],destination+0x38u+n*4u,right[n]);
    for(unsigned n=0;n<3;++n)publish(memory,observe,up_sites[n],destination+0x44u+n*4u,up[n]);
    for(unsigned n=0;n<3;++n)publish(memory,observe,back_sites[n],destination+0x50u+n*4u,backward[n]);
    constexpr std::array<std::uint32_t,3> position_sites{0x2ffd2u,0x2ffd7u,0x2ffddu};
    for(unsigned n=0;n<3;++n){const auto value=read(memory,camera+n*4u);publish(memory,observe,position_sites[n],destination+0x5cu+n*4u,value);}
    publish(memory,observe,0x2ffe5u,destination+0x34u,one);inverse_frame(memory,destination+0x34u,destination,observe);
    const auto half=read(memory,0x46a0ccu);auto metric_x=mul(program.horizontal,program.width);metric_x=mul(metric_x,half);
    publish(memory,observe,0x30011u,destination+0x68u,program.clipping[0]);publish(memory,observe,0x30017u,destination+0xb8u,metric_x);
    auto metric_y=mul(program.vertical,program.height);
    publish(memory,observe,0x3002bu,destination+0x6cu,program.clipping[1]);publish(memory,observe,0x30032u,destination+0x70u,program.clipping[2]);
    metric_y=mul(metric_y,half);publish(memory,observe,0x30039u,destination+0xbcu,metric_y);publish(memory,observe,0x30041u,destination+0x74u,program.clipping[3]);
    const auto near=read(memory,camera+0x40u);Vector normal;std::uint32_t distance;
    if(equal(near,0)) {
        const auto plane_y=read(memory,camera+0x4cu),plane_z=read(memory,camera+0x50u),plane_x=read(memory,camera+0x48u);
        const auto ux=read(memory,destination+0x1cu),lx=read(memory,destination+0x10u),uy=read(memory,destination+0x20u);
        const auto lx_term=mul(lx,plane_y);normal[0]=add(mul(ux,plane_z),lx_term);
        const auto fx=read(memory,destination+4u);normal[0]=add(normal[0],mul(fx,plane_x));
        const auto ly=read(memory,destination+0x14u);const auto ly_term=mul(ly,plane_y);normal[1]=add(mul(uy,plane_z),ly_term);
        const auto fy=read(memory,destination+8u);normal[1]=add(normal[1],mul(fy,plane_x));
        const auto uz=read(memory,destination+0x24u);const auto uz_term=mul(uz,plane_z);
        const auto lz=read(memory,destination+0x18u);const auto lz_term=mul(lz,plane_y);
        const auto fz=read(memory,destination+0xcu);const auto fz_term=mul(fz,plane_x);
        const auto plane_w=read(memory,camera+0x54u);distance=mul(plane_w,read(memory,destination));
        normal[2]=add(add(uz_term,lz_term),fz_term);
        const auto pz=read(memory,destination+0x30u);distance=add(distance,mul(pz,normal[2]));
        const auto py=read(memory,destination+0x2cu);distance=add(distance,mul(py,normal[1]));
        distance=add(distance,mul(normal[0],read(memory,destination+0x28u)));
    }else {distance=sub(0,read(memory,camera+0x40u));normal={0,0,one};}
    const auto reciprocal=div(one,normal[2]);const auto far=read(memory,camera+0x44u);
    load(reciprocal);multiply(normal[1]);const auto distance_product=mul(reciprocal,distance);
    asm volatile("fabs":::"memory","st");load(reciprocal);multiply(normal[0]);
    asm volatile("fabs;fxch %%st(1)":::"memory","st","st(1)");
    const auto negative_distance=sub(0,distance_product);
    asm volatile("fxch %%st(1);faddp %%st,%%st(1)":::"memory","st","st(1)");
    const auto depth_one=read(memory,0x46a0d0u);asm volatile("fadds %0"::"m"(depth_one):"memory","st");
    load(far);asm volatile("fsubs %0;fmulp %%st,%%st(1);fdivrs %1"::"m"(negative_distance),"m"(far):"memory","st","st(1)");
    const auto depth_scale=spill(false),retained_depth_scale=spill(true);
    auto depth_y=mul(reciprocal,depth_scale);depth_y=mul(depth_y,normal[1]);
    auto depth_x=mul(reciprocal,depth_scale);depth_x=mul(depth_x,normal[0]);
    auto offset=sub(0,mul(depth_scale,negative_distance));std::uint32_t final_scale;
    if(positive(offset)&&equal(read(memory,camera+0x40u),0)) {
        depth_x=sub(0,depth_x);depth_y=sub(0,depth_y);offset=sub(0,offset);final_scale=sub(0,depth_scale);
    }else final_scale=retained_depth_scale;
    for(unsigned n=0;n<16;++n)publish(memory,observe,0x301f1u,destination+0x78u+n*4u,0);
    publish(memory,observe,0x301f3u,destination+0x78u,program.horizontal);
    const auto negative_depth_x=sub(0,depth_x);publish(memory,observe,0x30204u,destination+0x8cu,program.vertical);
    const auto negative_depth_y=sub(0,depth_y),negative_center_x=sub(0,program.center_x);
    publish(memory,observe,0x3021cu,destination+0x98u,negative_center_x);
    const auto negative_center_y=sub(0,program.center_y);publish(memory,observe,0x3022du,destination+0x9cu,negative_center_y);
    const auto negative_one=read(memory,0x46a0dcu);const auto negative_scale=sub(0,final_scale);
    publish(memory,observe,0x30241u,destination+0x80u,negative_depth_x);publish(memory,observe,0x30249u,destination+0x90u,negative_depth_y);
    publish(memory,observe,0x30251u,destination+0xa0u,negative_scale);publish(memory,observe,0x30259u,destination+0xa4u,negative_one);
    publish(memory,observe,0x30261u,destination+0xb0u,offset);
}
// Semantic affine inversion. Names describe matrix axes and the reciprocal
// position; no virtual source registers or source instruction dispatch exist.
void inverse_frame(SourcePhysicsContactMemory &memory,std::uint32_t source,
    std::uint32_t destination,const SourcePhysicsContactRecordObserver &observe) {
    auto read_word=[&](std::uint32_t address){return read(memory,address);};
    auto write_word=[&](std::uint32_t pc,std::uint32_t address,std::uint32_t value){publish(memory,observe,pc,address,value);};
    native_affine_inverse_detail::Values view{read_word,write_word,source,destination};
    native_affine_inverse_detail::Progress data;data.source=source;data.destination=destination;
    native_affine_inverse_detail::invert(view,data);
}
}
void native_tu15_camera_inverse_frame(SourcePhysicsContactMemory &memory,std::uint32_t source,
    std::uint32_t destination,NativeTu15PhysicsFrameFp &fp,const SourcePhysicsContactRecordObserver &observe) {
    SimdEnvironment environment(fp);inverse_frame(memory,source,destination,observe);
}
NativeTu15CameraProjectionProgram native_tu15_camera_projection_program(SourcePhysicsContactMemory &memory,
    std::uint32_t camera,std::uint32_t bounds,NativeTu15PhysicsFrameFp &fp,NativeTu15CameraProjectionX87State &state) {
    ArithmeticEnvironment environment(fp,state);ValueProgram output;projection_program(memory,camera,bounds,output);return output.value();
}
void native_tu15_camera_projection_program_publish(SourcePhysicsContactMemory &memory,
    std::uint32_t camera,std::uint32_t bounds,std::uint32_t destination,NativeTu15PhysicsFrameFp &fp,
    NativeTu15CameraProjectionX87State &state,const SourcePhysicsContactRecordObserver &observe) {
    ArithmeticEnvironment environment(fp,state);RetainedProgram output(memory,destination,observe);projection_program(memory,camera,bounds,output);
}
void native_tu15_camera_projection(SourcePhysicsContactMemory &memory,std::uint32_t camera,
    std::uint32_t bounds,std::uint32_t destination,NativeTu15PhysicsFrameFp &fp,
    NativeTu15CameraProjectionX87State &state,const SourcePhysicsContactRecordObserver &observe) {
    ArithmeticEnvironment environment(fp,state);projection(memory,camera,bounds,destination,observe);
}
}
