// Differential test: the native gain kernels (127CF0, 129B20, 21F780, 21F7F0,
// 220000, 220200) against the lifted TU1.5 routines they replace, from
// identical randomized game states across x87 precision/rounding controls,
// MXCSR modes, special floats, every branch, refused x87 stacks and
// unmapped game data.
#include "halo2/native_tu15_sound_manager.hpp"
#include "halo2/source_sound_gain_tu15.hpp"
#include "native_tu15_sound_test_support.hpp"
#include <algorithm>
#include <vector>

using namespace halo2;
using namespace halo2::sound_test;
namespace {
constexpr std::uint32_t tags=0x01000000u,gestalt=0x01100000u,scales=0x01110000u,globals=0x01300000u,
    reference_at=0x01310000u,class_tag=0x01320000u,class_rows=0x01330000u,definition=0x01400000u,
    runtime_rows=0x01500000u,ducking=0x01600000u;
constexpr std::uint32_t none=0xffffffffu;

// Gains in dB around the clamp range [-64, 0], with special floats.
float gain_db(Random &r) {
    switch(r.below(10)) {
    case 0:return r.scalar();
    case 1:return -64.f;
    case 2:return r.below(2)?0.f:-0.f;
    default:return float(int(r.below(10000))-8500)/100.f;
    }
}
// Durations in seconds: mostly positive, some below the 0.001 threshold.
float duration(Random &r) {
    switch(r.below(12)) {
    case 0:return r.scalar();
    case 1:return 0.f;
    case 2:return 0.0005f;
    case 3:return std::bit_cast<float>(0x3a83126fu);   // exactly the 0.001 threshold
    case 4:return -float(r.below(3000))/1000.f;
    default:return float(r.below(6000))/1000.f;
    }
}
// Amplitudes for 220200: [0, 1) mostly, tiny ones (below -64 dB), above 1,
// non-positive and special.
float amplitude(Random &r) {
    switch(r.below(12)) {
    case 0:return r.scalar();
    case 1:return r.below(2)?0.f:-0.f;
    case 2:return 1.f;
    case 3:return std::ldexp(float(r.word()|1u)/4294967296.f,-int(r.below(24)));
    case 4:return 1.f+float(r.below(100000))/1000.f;
    case 5:return -float(r.below(100000))/1000.f;
    default:return float(r.word())/4294967296.f;
    }
}
// Ducker types: none, cutscene (0), scripted dialog (1), the two record
// slots past them, and rarely a far type whose records are unmapped.
std::uint32_t ducker(Random &r) {
    const auto pick=r.below(40);
    if(pick==0)return 0x08000000u|r.below(256);
    if(pick<4)return 2u+r.below(2);
    if(pick<16)return none;
    return r.below(2);
}

Case scenario(Random &r,std::uint32_t entry) {
    auto c=base_case(r);auto &m=c.memory;auto &a=c.abi;
    // Retail .rdata words the lifted routines read (TU1.5 XBE).
    m.put32(0x455b0cu,0xc2800000u);m.put32(0x455b14u,0x40135d8eu);   // -64.0, ln 10
    m.put32(0x46a0d0u,0x3f800000u);m.put32(0x46a0e8u,0u);            // 1.0, 0.0
    m.put32(0x46a118u,0x41a00000u);m.put32(0x46a12cu,0x3d4ccccdu);   // 20.0, 0.05
    m.put32(0x46a180u,0x3a83126fu);                                  // 0.001
    // Sound globals and the sound-class tag; rarely no class reference,
    // which reads address 4 (unmapped) like the original.
    m.put32(0x4f0414u,tags);m.put32(0x52d664u,gestalt);m.put32(gestalt+0xcu,scales);
    m.put32(0x4ecc1cu,globals);m.put32(globals+0xc0u,r.below(64)?1u+r.below(3):0u);
    m.put32(globals+0xc4u,reference_at);
    const std::uint32_t class_datum=(r.word()&0xffff0000u)|7u;
    m.put32(reference_at+4u,class_datum);m.put32(tags+7u*16u+8u,class_tag);m.put32(class_tag+4u,class_rows);
    // The sound: class index (+2) and scale index (+0xB).
    const auto class_index=std::int8_t(r.word()),scale_index=std::int8_t(r.word());
    m.put8(definition+2u,std::uint8_t(class_index));m.put8(definition+0xbu,std::uint8_t(scale_index));
    // Scale row (20 bytes; +0/+4 gain modifier bounds).
    const auto scale=scales+std::uint32_t(scale_index)*20u;
    m.map(scale,20u);m.putf(scale,gain_db(r));m.putf(scale+4u,gain_db(r));
    // Class row (0x5C bytes; +0x20/+0x24 gain bounds, ducking records at
    // +0x28 per type: gain, fade-in, sustain, fade-out). Map the next row
    // for record slots 2 and 3.
    const auto row=class_rows+std::uint32_t(class_index)*0x5cu;
    m.map(row,0x5cu*2u);
    m.putf(row+0x20u,gain_db(r));m.putf(row+0x24u,gain_db(r));
    for(unsigned type=0;type<4;++type) {
        const auto record=row+0x28u+type*16u;
        m.putf(record,gain_db(r));m.putf(record+4u,duration(r));m.putf(record+8u,duration(r));m.putf(record+0xcu,duration(r));
    }
    // Class runtime row (16 bytes; +4 gain, +0xC flags).
    m.put32(0x50ebf0u,runtime_rows);
    const auto runtime=runtime_rows+std::uint32_t(class_index)*16u;
    m.putf(runtime+4u,gain_db(r));m.put8(runtime+0xcu,std::uint8_t(r.word()));
    // Ducking globals: the active ducker and the one fading out (often the
    // same: a restart), each with its clock.
    m.put32(0x4f2c50u,ducking);
    const auto active=ducker(r),fading=r.below(5)<2?active:ducker(r);
    m.put32(ducking+0x1f8u,active);m.put32(ducking+0x200u,fading);
    m.putf(ducking+0x1fcu,r.below(16)?float(r.below(10000))/1000.f:r.scalar());
    auto since=r.below(16)?float(r.below(10000))/1000.f:r.scalar();
    if(fading<4u && r.below(3)==0) {
        // Land the fading ducker's progress inside [0, 1] sometimes.
        const auto record=row+0x28u+fading*16u;
        const auto ratio=std::bit_cast<float>(m.get32(record+8u))/std::bit_cast<float>(m.get32(record+0xcu));
        if(std::isfinite(ratio))since=ratio+float(r.below(1000))/1000.f;
    }
    m.putf(ducking+0x204u,since);
    // Some x87 stacks too deep for the original's loads (both refuse them).
    if(r.below(12)==0) {
        a.x87_depth=5u+r.below(4);
        for(unsigned i=0;i<a.x87_depth;++i)a.x87[i]=static_cast<long double>(r.scalar());
    }
    if(entry==0x127cf0u) {
        a.gp[6]=definition;
        m.putf(stack+4u,gain_db(r));                                       // gain offset (dB)
        m.putf(stack+8u,r.below(10)?float(r.below(1001))/1000.f:r.scalar()); // interpolation fraction
    }
    if(entry==0x129b20u) {
        // Callers pass a sign-extended 8-bit class index in CX; rarely a
        // 16-bit one whose runtime row is unmapped.
        const auto index=r.below(32)?std::uint16_t(std::int16_t(class_index)):std::uint16_t(0x4000u|r.word());
        a.gp[1]=(r.word()&0xffff0000u)|index;
    }
    if(entry==0x21f780u || entry==0x21f7f0u)a.gp[2]=definition;
    if(entry==0x220000u)m.putf(stack+4u,gain_db(r));
    if(entry==0x220200u)a.xmm[0][0]=std::bit_cast<std::uint32_t>(amplitude(r));
    return c;
}

void reference(SourcePhysicsContactMemory &m,std::uint32_t entry,SourcePhysicsContactRuntimeAbi &a,
    SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsContactRecordObserver &o) {
    SourcePhysicsWorldRuntimeServices services;
    services.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("unowned callee");};
    source_run_sound_gain_tu15(m,entry,a,fp,services,o);
}

// Path classification from the inputs (plain host comparisons: JA is an
// ordered >, JB is "not >=").
float at(const Memory &m,std::uint32_t p){return std::bit_cast<float>(m.get32(p));}
bool ja(float x,float y){return x>y;}
bool jb(float x,float y){return !(x>=y);}
enum Path : unsigned {
    undiminished,restart_sustained,restart_no_fade_in,restart_fade_in,restart_fade_in_full,
    restart_ramp,restart_ramp_full,fading_none,fading_no_fade_out,fading_zero,fading_full,fading_partial,
    active_none,active_no_fade_in,active_partial,active_full,path_count
};
constexpr const char *path_names[]{"undiminished","restart sustained","restart without fade-in",
    "restart fade-in","restart fade-in complete","restart ramp","restart ramp complete","no fading ducker",
    "fading without fade-out","fading t=0","fading t=1","fading partial","no active ducker",
    "active without fade-in","active partial","active complete"};
void classify_ducking(const Memory &m,std::uint32_t index,std::vector<unsigned> &paths) {
    const auto runtime=runtime_rows+(index<<4);
    if(m.get32(runtime+0xcu)&1u){paths.push_back(undiminished);return;}
    const auto row=class_rows+index*0x5cu;
    const auto record=[&](std::uint32_t type){return row+0x28u+(type<<4);};
    const auto active=m.get32(ducking+0x1f8u),fading=m.get32(ducking+0x200u);
    if(active>=4u && active!=none)return;   // unmapped records: a fault case
    if(fading>=4u && fading!=none)return;
    const float shortest=std::bit_cast<float>(0x3a83126fu);
    if(fading!=none && active==fading) {
        const auto d=record(fading);
        const float elapsed=at(m,ducking+0x1fcu)-at(m,ducking+0x204u);
        if(!ja(elapsed,at(m,d+8u))){paths.push_back(restart_sustained);return;}
        if(jb(at(m,d+4u),shortest)){paths.push_back(restart_no_fade_in);return;}
        const float fade_out=at(m,d+0xcu);
        if(!jb(fade_out,shortest) && ja(fade_out+at(m,d+8u),elapsed)) {
            const float position=(at(m,ducking+0x1fcu)-elapsed)+(elapsed-at(m,d+8u))*(1.f/at(m,d+4u))*fade_out;
            paths.push_back(ja(1.f,position*(1.f/at(m,d+4u)))?restart_ramp:restart_ramp_full);
            return;
        }
        paths.push_back(ja(1.f,(at(m,ducking+0x1fcu)-elapsed)/at(m,d+4u))?restart_fade_in:restart_fade_in_full);
        return;
    }
    if(fading==none)paths.push_back(fading_none);
    else {
        const auto d=record(fading);
        if(jb(at(m,d+0xcu),shortest))paths.push_back(fading_no_fade_out);
        else {
            const float x=at(m,ducking+0x204u)-at(m,d+8u)/at(m,d+0xcu);
            paths.push_back(ja(0.f,x)?fading_zero:ja(x,1.f)?fading_full:fading_partial);
        }
    }
    if(active==none){paths.push_back(active_none);return;}
    const auto d=record(active);
    if(jb(at(m,d+4u),shortest)){paths.push_back(active_no_fade_in);return;}
    paths.push_back(ja(1.f,at(m,ducking+0x1fcu)/at(m,d+4u))?active_partial:active_full);
}
// Both sides refuse an x87 stack the original's loads would overflow.
unsigned x87_room(std::uint32_t entry,const Case &c) {
    if(entry==0x220000u || entry==0x127cf0u)return 3u;
    if(entry==0x220200u) {
        const float x=std::bit_cast<float>(c.abi.xmm[0][0]);
        // The logarithm only runs for positive amplitudes (DAZ zeroes denormals).
        const bool daz=c.fp.mxcsr&0x40u;
        return x>0.f && !(daz && std::fpclassify(x)==FP_SUBNORMAL)?2u:0u;
    }
    return 0u;
}
} // namespace

int main() try {
    Random r(0x6a1e5eedu);unsigned compared=0,faults=0,refused=0;
    for(const auto entry:{0x127cf0u,0x129b20u,0x21f780u,0x21f7f0u,0x220000u,0x220200u}) {
        std::vector<unsigned> paths(path_count),clamps(3),levels(3),flags(2);
        unsigned entry_faults=0;
        for(unsigned n=0;n<8000;++n) {
            const auto c=scenario(r,entry);
            const auto before=faults;
            if(!agree(c,entry,reference,faults,"sound gain",n))return 1;
            // agree() accepts an oracle fault from the reference alone;
            // require the native kernel to fault identically too.
            if(faults!=before) {
                std::string native="returned",lifted="returned";
                auto x=c;x.memory=c.memory.clone();
                try{native_tu15_sound_manager_run(x.memory,entry,x.abi,x.fp);}catch(const std::exception &e){native=e.what();}
                auto y=c;y.memory=c.memory.clone();
                try{reference(y.memory,entry,y.abi,y.fp,{});}catch(const std::exception &e){lifted=e.what();}
                if(native!=lifted) {
                    std::cerr<<"sound gain case "<<n<<" ("<<std::hex<<entry<<std::dec<<"): native "<<native
                        <<", lifted "<<lifted<<'\n';
                    return 1;
                }
                ++entry_faults;
                if(native=="Required original solver x87 stack-fault owner unavailable")++refused;
            }
            if(c.abi.x87_depth+x87_room(entry,c)>8u && faults==before) {
                std::cerr<<"sound gain case "<<n<<": an overflowing x87 stack was not refused\n";return 1;
            }
            const auto &m=c.memory;
            const auto index=entry==0x129b20u?std::uint32_t(std::int32_t(std::int16_t(c.abi.gp[1]))):
                std::uint32_t(std::int32_t(std::int8_t(m.get16(definition+2u)&0xffu)));
            if((entry==0x129b20u || entry==0x127cf0u) && faults==before) {
                std::vector<unsigned> taken;classify_ducking(m,index,taken);
                for(const auto p:taken)++paths[p];
            }
            if(entry==0x127cf0u && faults==before)++flags[m.get32(runtime_rows+(index<<4)+0xcu)&1u];
            if(entry==0x220000u) {   // clamped low / high / passed through
                const float db=at(m,stack+4u);
                ++clamps[ja(-64.f,db)?0:ja(db,0.f)?1:2];
            }
            if(entry==0x220200u && faults==before) {   // no logarithm / silent / level
                const float x=std::bit_cast<float>(c.abi.xmm[0][0]);
                const bool daz=(c.fp.mxcsr&0x40u) && std::fpclassify(x)==FP_SUBNORMAL;
                if(!(x>0.f) || daz)++levels[0];
                else ++levels[20.0*std::log10(std::min(double(x),1.0))< -64.0?1:2];
            }
            ++compared;
        }
        const auto require=[&](bool covered,const char *what) {
            if(!covered)std::cerr<<std::hex<<entry<<std::dec<<" coverage incomplete: "<<what<<'\n';
            return covered;
        };
        if(entry==0x129b20u)
            for(unsigned p=0;p<path_count;++p)if(!require(paths[p]>0,path_names[p]))return 1;
        if(entry==0x127cf0u && !(require(flags[0]&&flags[1],"class flag")&&
            require(paths[undiminished]&&paths[restart_ramp]&&paths[fading_partial]&&paths[active_partial],"ducking")))
            return 1;
        if(entry==0x220000u && !require(clamps[0]&&clamps[1]&&clamps[2],"dB clamp"))return 1;
        if(entry==0x220200u && !require(levels[0]&&levels[1]&&levels[2],"logarithm"))return 1;
        if(!require(entry_faults>0,"faults"))return 1;
    }
    std::cout<<"PASS native sound gain kernels: "<<compared<<" randomized states identical to the lifted routines ("
        <<faults<<" identical faults, "<<refused<<" of them refused x87 stacks)\n";
    return 0;
} catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
