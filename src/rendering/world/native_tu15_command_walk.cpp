#include "halo2/native_tu15_command_walk.hpp"
#include "halo2/detail/native_tu15_arithmetic_environment.hpp"
#include <bit>
#include <stdexcept>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <emmintrin.h>
#endif

namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory& m,std::uint32_t at,unsigned size=4) {
    std::array<std::byte,4> b{};m.read(at,std::span(b).first(size));std::uint32_t v{};
    for(unsigned i=0;i<size;++i)v|=std::to_integer<unsigned>(b[i])<<(8*i);
    return v;
}
void write(SourcePhysicsContactMemory& m,std::uint32_t at,std::uint32_t v,unsigned size=4) {
    std::array<std::byte,4> b{};for(unsigned i=0;i<size;++i)b[i]=std::byte(v>>(8*i));
    m.write(at,std::span(b).first(size));
}
std::int32_t signed_half(std::uint32_t v){return std::bit_cast<std::int16_t>(std::uint16_t(v));}
std::uint32_t unit_address(std::uint32_t unit){return 0x4c6958u+32u*std::uint32_t(signed_half(unit));}
std::int32_t signed_word(std::uint32_t v){return std::bit_cast<std::int32_t>(v);}
std::uint32_t part_geometry(const SourcePhysicsContactMemory& m,std::uint32_t at,std::uint32_t flags) {
    const auto model=read(m,at+4u),packed=read(m,at+0xcu);
    if(model!=0xffffffffu) {
        const auto table=read(m,0x4f0414u),definition=read(m,table+16u*(model&65535u)+8u);
        const auto sections=read(m,definition+0x28u);
        return read(m,sections+92u*((packed>>9)&511u)+0x34u);
    }
    const auto bsp=read(m,0x4ecc18u);
    if(flags&0x1000u) {
        const auto instances=read(m,bsp+0x144u);
        const auto definition=std::uint32_t(signed_half(read(m,instances+88u*((packed>>18)&2047u)+0x34u,2)));
        const auto definitions=read(m,bsp+0x13cu);
        return read(m,definitions+200u*definition+0x50u);
    }
    return read(m,read(m,bsp+0xa0u)+176u*((packed>>9)&511u)+0x50u);
}
}
bool native_tu15_command_part_visible_460c0(const SourcePhysicsContactMemory& m,
    std::uint32_t unit,std::uint32_t part,std::uint32_t mask) {
    const auto at=unit_address(unit),flags=read(m,at);
    if(!(flags&0x40u))return true;
    const auto geometry=part_geometry(m,at,flags);
    const auto parts=read(m,geometry+4u),p=parts+72u*part;
    const auto first=signed_half(read(m,p+0xau,2)),count=signed_half(read(m,p+0xcu,2));
    for(auto bit=first;bit<first+count;++bit)
        if(read(m,mask+4u*std::uint32_t(bit>>5))&(1u<<(std::uint32_t(bit)&31u)))return true;
    return false;
}
void native_tu15_command_parts_44ab0(SourcePhysicsContactMemory& m,std::uint32_t mode,
    std::uint32_t flag_mask,std::uint32_t group,NativeTu15PhysicsFrameFp& fp,const NativeTu15CommandPartServices& s) {
    const auto prefix=read(m,0x4cd39cu),draw_count=read(m,0x4cd398u); // CMOV reads both
    const auto end=std::uint16_t(mode?draw_count:prefix);
    for(std::uint32_t unit=mode?std::uint16_t(prefix):0u;unit<end;++unit) {
        const auto at=unit_address(unit),flags=read(m,at);
        if(flags&0x400u) {
            if(!mode) {
                const auto count=read(m,0x515f1cu);
                if(signed_word(count)<32) {
                    write(m,0x4d4f60u+2u*count,unit,2);write(m,0x515f1cu,count+1u);
                }
            } else {
                const auto offset=0x408u*read(m,0x515f20u),count_at=0x4d2f1cu+offset;
                const auto count=signed_half(read(m,count_at,2));
                if(count<32) {
                    const auto dest=0x4d2b18u+offset+32u*std::uint32_t(count);
                    // REP MOVSD reads and writes each word in order, including
                    // overlap. Its final INC rereads the potentially aliased row.
                    for(unsigned i=0;i<8;++i)write(m,dest+4u*i,read(m,at+4u*i));
                    write(m,count_at,read(m,count_at,2)+1u,2);
                }
            }
            continue;
        }
        if(!(flags&flag_mask&0x1fffffu))continue;
        const auto packed=read(m,at+0xcu);
        if(((packed>>4)&31u)!=group || !(flags&1u) || !read(m,at+0x1au,1))continue;
        const auto coverage=read(m,0x4cd3a4u+unit,1),matrix=packed&15u;
        const auto slot=read(m,0x4cd6f6u+std::uint32_t(signed_half(unit)),1);
        const auto override_shader=slot==255u?0xffffffffu:read(m,0x4cda48u+4u*slot);
        const auto geometry=s.geometry_449d0(unit,(flags>>12)&0xffffff01u,fp);
        if(!geometry || signed_word(read(m,geometry))<=0)continue;
        for(std::uint32_t part=0;;) {
            if(s.prepare_45cd0(unit,part,matrix,fp) &&
                native_tu15_command_part_visible_460c0(m,unit,part,read(m,at+0x14u))) {
                // Source reads the packet even when the pinned override wins.
                auto shader=read(m,s.packet);
                if(override_shader!=0xffffffffu)shader=override_shader;
                if(shader!=0xffffffffu)s.emit_41030({unit,part,mode,coverage,s.packet,shader,group!=31u,0xffffffffu,0},fp);
            }
            const auto count=read(m,geometry);++part;
            if(signed_word(part)>=signed_word(count))break;
        }
    }
}
std::uint32_t native_tu15_command_group_44c80(SourcePhysicsContactMemory& m,
    NativeTu15CommandGroup44c80 a,std::uint32_t caller_stack,NativeTu15PhysicsFrameFp& fp,
    const NativeTu15CommandPartServices& services) {
    const auto prefix=read(m,0x4cd39cu);
    const auto count=read(m,0x4cd398u);
    const std::uint32_t end=std::uint16_t(a.mode?count:prefix);
    for(std::uint32_t unit=a.mode?std::uint16_t(prefix):0;unit<end;++unit) {
        const auto address=unit_address(unit);
        const auto flags=read(m,address);
        if(!(flags&a.flag_mask&0x1fffffu))continue;
        const auto packed=read(m,address+0xcu);
        if(((packed>>4)&31u)!=a.section || !(flags&1u) || !read(m,address+0x1au,1))continue;
        const auto coverage=read(m,0x4cd3a4u+unit,1);
        const auto slot=read(m,0x4cd6f6u+std::uint32_t(signed_half(unit)),1);
        const auto override_shader=slot==255u?0xffffffffu:read(m,0x4cda48u+4u*slot);
        const auto geometry=part_geometry(m,address,flags);
        if(!geometry)continue;
        const auto object_context=read(m,address+0x10u);
        if(read(m,object_context+0x76u,1)==255u) {
            a.mask=a.opaque_mask;
            a.add=a.opaque_add;
            write(m,caller_stack+0x10u,a.mask);
            write(m,caller_stack+0x14u,a.add);
        }
        if(signed_word(read(m,geometry))<=0)continue;
        for(std::uint32_t part=0;;) {
            if(services.prepare_45cd0(unit,part,packed&15u,fp)) {
                // 44E1D reloads the unit flags after the packet producer.
                const auto shader=(a.use_override&255u)||(read(m,address)&0x8000u)?override_shader:0xffffffffu;
                const auto grouped=(a.add&0xffffff00u)|std::uint32_t(a.section!=31u);
                services.emit_41030({unit,part,a.mode,coverage,services.packet,shader,grouped,a.mask,a.add},fp);
            }
            ++part;
            if(signed_word(part)>=signed_word(read(m,geometry)))break;
        }
    }
    return end;
}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace {
__m128 queue_scalar(std::uint32_t bits) {
    return _mm_castsi128_ps(_mm_cvtsi32_si128(signed_word(bits)));
}
std::uint32_t queue_bits(__m128 value) {
    return std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(value)));
}
bool queue_above(std::uint32_t left,std::uint32_t right) {
    unsigned char result;
    asm volatile("comiss %2,%1; seta %0":"=qm"(result):"x"(queue_scalar(left)),"x"(queue_scalar(right)):"cc","memory");
    return result;
}
}
#endif
unsigned native_tu15_queue_camouflage_42090(SourcePhysicsContactMemory& m,NativeTu15PhysicsFrameFp& fp) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    constexpr auto pool_address=0x4c2aa8u,count_address=0x4c2aa0u;
    const auto section=[](unsigned index){return 0x4cdd4cu+36u*index;};
    const auto highwater=signed_word(read(m,0x4ce1ccu));
    if(highwater<=0)return 0;
    if(highwater>32)throw std::invalid_argument("Native42090 requires the original section table extent");
    const auto pool=read(m,pool_address);
    auto record=pool+0x70u*read(m,count_address);
    auto arithmetic=fp;
    {detail::NativeTu15ArithmeticEnvironment environment(arithmetic);}
    std::uint32_t visited=0;
    unsigned appended=0;
    for(unsigned seed=0;seed<unsigned(highwater);++seed) {
        const auto at=section(seed);
        if(read(m,at+32u)!=1u || (visited&(1u<<seed)))continue;
        if(signed_word(read(m,count_address))>=0x400) {
            if(read(m,0x4c2ab1u,1))write(m,0x4c2ab1u,0,1);
            continue;
        }
        const auto count=read(m,count_address);
        write(m,record,0);
        write(m,record+0xcu,0,1);
        write(m,record,3u);
        write(m,record+8u,0);
        write(m,record+0x20u,0x41cb0u);
        auto depth=read(m,at);
        visited|=1u<<seed;
        std::array<std::uint32_t,3> minimum{read(m,at+4u),read(m,at+8u),read(m,at+12u)};
        auto maximum=minimum;
        write(m,count_address,count+1u);

        std::array<std::uint8_t,32> members{};
        unsigned size=1;
        members[0]=std::uint8_t(seed);
        std::array<std::uint32_t,3> center{};
        std::uint32_t distance_bits;
        {
            detail::NativeTu15ArithmeticEnvironment environment(arithmetic);
            for(unsigned candidate=0;candidate<unsigned(highwater);++candidate) {
                const auto bit=1u<<candidate;
                if(visited&bit)continue;
                const auto row=section(candidate);
                const std::array point{read(m,row+4u),read(m,row+8u),read(m,row+12u)};
                const auto xmin=read(m,row+16u),xmax=read(m,row+20u),ymin=read(m,row+24u),ymax=read(m,row+28u);
                bool overlaps=false;
                for(unsigned member=0;member<size;++member) {
                    const auto rect=section(members[member]);
                    const auto other_xmin=read(m,rect+16u);
                    const bool crosses_xmin=queue_above(xmax,other_xmin);
                    const auto other_xmax=read(m,rect+20u),other_ymin=read(m,rect+24u),other_ymax=read(m,rect+28u);
                    if(crosses_xmin && queue_above(ymax,other_ymin) && queue_above(other_xmax,xmin) && queue_above(other_ymax,ymin)) {
                        overlaps=true;
                        break;
                    }
                }
                if(overlaps)continue;
                members[size++]=std::uint8_t(candidate);
                visited|=bit;
                for(unsigned axis=0;axis<3;++axis)
                    if(!queue_above(point[axis],minimum[axis]))minimum[axis]=point[axis];
                for(unsigned axis=0;axis<3;++axis)
                    if(queue_above(point[axis],maximum[axis]))maximum[axis]=point[axis];
                const auto candidate_depth=read(m,row);
                if(queue_above(depth,candidate_depth))depth=candidate_depth;
            }
            auto distance=_mm_setzero_ps();
            asm volatile("subss %1,%0":"+x"(distance):"x"(queue_scalar(depth)):"memory");
            distance_bits=queue_bits(distance);
            for(unsigned axis=0;axis<3;++axis) {
                auto sum=queue_scalar(minimum[axis]);
                asm volatile("addss %1,%0":"+x"(sum):"x"(queue_scalar(maximum[axis])):"memory");
                center[axis]=queue_bits(sum);
            }
        }
        write(m,record+0x24u,size,1);
        unsigned i=0;
        for(;i+4u<=size;i+=4u) {
            const auto word=std::uint32_t(members[i])|(std::uint32_t(members[i+1])<<8u)|
                (std::uint32_t(members[i+2])<<16u)|(std::uint32_t(members[i+3])<<24u);
            write(m,record+0x25u+i,word);
        }
        for(;i<size;++i)write(m,record+0x25u+i,members[i],1);
        write(m,record+4u,distance_bits);
        const auto half=queue_scalar(read(m,0x46a0ccu));
        {
            detail::NativeTu15ArithmeticEnvironment environment(arithmetic);
            for(auto& bits:center) {
                auto sum=queue_scalar(bits);
                asm volatile("mulss %1,%0":"+x"(sum):"x"(half):"memory");
                bits=queue_bits(sum);
            }
        }
        for(unsigned axis=0;axis<3;++axis)write(m,record+0x64u+4u*axis,center[axis]);
        record+=0x70u;
        ++appended;
    }
    arithmetic.control_word=fp.control_word;
    fp=arithmetic;
    return appended;
#else
    (void)m;(void)fp;
    throw std::invalid_argument("Exact native42090 arithmetic backend unavailable");
#endif
}
unsigned native_tu15_queue_sections_42750(SourcePhysicsContactMemory& m,std::uint32_t mask,NativeTu15PhysicsFrameFp& fp) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    const auto highwater=signed_word(read(m,0x4ce1ccu));
    if(highwater<=0)return 0;
    const auto pool=read(m,0x4c2aa8u);
    auto record=pool+0x70u*read(m,0x4c2aa0u);
    auto arithmetic=fp;
    unsigned appended=0;
    // Validate the selected arithmetic environment before publishing rows.
    {detail::NativeTu15ArithmeticEnvironment environment(arithmetic);}
    for(std::int32_t index=0;index<highwater;++index) {
        const auto section=0x4cdd4cu+36u*std::uint32_t(index);
        if(read(m,section+32u)!=3u)continue;
        const auto count=read(m,0x4c2aa0u);
        if(signed_word(count)>=0x400) {
            // 4282A..42838 follows the first return and re-enters the loop.
            if(read(m,0x4c2ab1u,1))write(m,0x4c2ab1u,0,1);
            continue;
        }
        const auto distance=record+4u,center=record+0x64u;
        write(m,record,0);
        write(m,record+0xcu,0,1);
        write(m,0x4c2aa0u,count+1u);
        write(m,record,3u);
        write(m,record+8u,0);
        write(m,record+0x20u,0x423b0u);
        if(distance)write(m,distance,read(m,section));
        if(center) {
            write(m,center,read(m,section+4u));
            write(m,center+4u,read(m,section+8u));
            write(m,center+8u,read(m,section+12u));
        }
        write(m,record+0x24u,std::uint32_t(index));
        write(m,record+0x28u,mask);
        const auto raw_distance=read(m,distance);
        std::uint32_t depth_bits;
        {
            detail::NativeTu15ArithmeticEnvironment environment(arithmetic);
            auto depth=_mm_setzero_ps();
            const auto source=_mm_castsi128_ps(_mm_cvtsi32_si128(signed_word(raw_distance)));
            // The source SUBSS starts from positive zero and retains
            // its arithmetic flags and operand treatment for NaNs.
            asm volatile("subss %1,%0":"+x"(depth):"x"(source):"memory");
            depth_bits=std::uint32_t(_mm_cvtsi128_si32(_mm_castps_si128(depth)));
        }
        write(m,distance,depth_bits);
        record+=0x70u;
        ++appended;
    }
    arithmetic.control_word=fp.control_word;
    fp=arithmetic;
    return appended;
#else
    (void)m;(void)mask;(void)fp;
    throw std::invalid_argument("Exact native42750 arithmetic backend unavailable");
#endif
}
void native_tu15_command_frame_40e80(SourcePhysicsContactMemory& m,std::uint32_t selector,
    NativeTu15PhysicsFrameFp& fp,const NativeTu15CommandFrameServices& s) {
    const auto mask=!selector?0xffffffffu:selector==2u?0x10u:0x20u;
    const auto context=read(m,0x4cd3a0u),mode=read(m,context+0x2a78u);
    if(!mode) {
        for(const auto at:{0x4ce26cu,0x4ce274u,0x4ce270u,0x4c2aa0u,0x4c2aa4u})write(m,at,0);
    } else {
        for(const auto at:{0x4ce284u,0x4ce28cu,0x4ce288u,0x4ce29cu,0x4ce2a4u,0x4ce2a0u})write(m,at,0);
    }
    s.prepare_41c70(context,mode,fp);
    native_tu15_command_parts_44ab0(m,mode,mask,31,fp,s.parts);
    if(mode)return;
    if(read(m,read(m,0x4f3218u)+8u)==3)s.producer_179b40(fp);
    else {s.producer_179a40(fp);s.producer_1770c0(fp);}
    s.producer_42090(fp);s.producer_42750(mask,fp);s.producer_41630(fp);
}
}
