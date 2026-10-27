// Differential test: the native object looping-sound frame routines (TU1.5
// 18EE50 and its helpers 223DA0, 22C560, 226370, 18ED80, 18F510, 18F900,
// 129DD0, 18D390, 18D3D0) against the lifted originals, over randomized
// structurally valid game states: the sound state and its listeners
// ([4F2C50]), the sound structure ([4ECC18]), the looping-sound pool
// ([4F9B5C]), the per-player loops and impulse slots ([4F9B58]), the voice
// and channel arrays ([52D670], [52D668]), the ambience channels ([52D684])
// with their definition rows, the DSP parameter records ([51DF90]), the
// gestalt ([52D664]) and tags, across x87 precision/rounding controls, MXCSR
// modes, stack alignments and special floats, with and without native camera
// views. 18EE50 runs the whole frame: the group routines it calls (18E970 and
// the update group, 18D5A0 and the sources group, 253070) resolve to their
// native or lifted versions on the native side and run lifted on the lifted
// side. Routines outside the group run through Services::required_owner as
// deterministic stubs that log the full register, XMM, x87, FP-control and
// stack-argument state of every call, sometimes raise MXCSR/x87 sticky flags,
// and clobber the scratch registers. Both sides must agree on every register,
// XMM lane, x87 and FP control state, every external call and every memory
// byte, or fault identically. The lifted run reports instruction coverage per
// routine.
// Options: HALO2_LOOP_TEST_ONLY=18ee50,22c560 runs those routines only;
// HALO2_STRICT_DEAD_STACK=1 also fails on differing dead callee scratch
// (below the post-return ESP; reported as a count otherwise);
// HALO2_LOOP_TEST_LIFTED_BOTH=1 runs the lifted routine on both sides to
// validate the harness itself; HALO2_LOOP_TEST_CASES=n overrides the count;
// HALO2_LOOP_TEST_VERBOSE=1 prints every matching fault;
// HALO2_DEAD_STACK_FLOOR=n limits the strict dead-stack check to the routine's
// own frame (entry ESP - n and above), excluding deeper native callees.
#include "halo2/native_tu15_sound_manager.hpp"
#include "halo2/source_object_loop_audio_tu15.hpp"
#include "native_tu15_sound_test_support.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <regex>
#include <set>
#include <unordered_set>
#include <vector>

using namespace halo2;
using namespace halo2::sound_test;
namespace {
using Abi=SourcePhysicsContactRuntimeAbi;
using Services=SourcePhysicsContactRuntimeServices;
using Call=SourcePhysicsContactRuntimeCall;
using O=SourcePhysicsContactRecordObserver;
using M=SourcePhysicsContactMemory;
using FP=SourcePhysicsFactoryFloatingPoint;
using Views=SourceObjectAudioViewsTu15;
constexpr std::uint32_t none=0xffffffffu;

// Test-owned game data. The update group's layout (18E970 runs inside 18EE50).
constexpr std::uint32_t listeners=0x01000000u;   // [4F2C50]: +0x78.. ducking bytes, +0x88 listener records (0x48),
                                                 // +0x1A8 loop slots (0x1C), +0x1E0 rate, +0x1F8.. ducking state
constexpr std::uint32_t structure=0x01100000u;   // [4ECC18]: the sound structure tables
constexpr std::uint32_t sources=0x01200000u;
constexpr std::uint32_t records=0x01300000u;
constexpr std::uint32_t rows=0x01400000u;
constexpr std::uint32_t definitions=0x01500000u;
constexpr std::uint32_t clusters=0x01600000u;
constexpr std::uint32_t cluster_words=0x01610000u;
constexpr std::uint32_t scale_table=0x01700000u;
constexpr std::uint32_t scale_bytes=0x01710000u;
constexpr std::uint32_t cluster_bytes=0x01720000u;
constexpr std::uint32_t players=0x01800000u;     // [4F54F0]
constexpr std::uint32_t player_slots=0x01810000u;// [4F9B58]: +0 loop handles, +0x24 ducking end tick,
                                                 // +0x128 impulse slots (16 x 0x10), +0x240 byte
constexpr std::uint32_t pool=0x01820000u;        // [4F9B5C]: loop pool header
constexpr std::uint32_t pool_elements=0x01830000u;
constexpr std::uint32_t pool_bitmap=0x01840000u;
constexpr std::uint32_t flag_block=0x01850000u;  // [51DF50]
constexpr std::uint32_t tags=0x01900000u;        // [4F0414]
constexpr std::uint32_t tag_data=0x01a00000u;    // 0x200 bytes per tag
constexpr std::uint32_t heap=0x01e00000u;        // routine arguments
constexpr std::uint32_t plan=0x01f00000u;        // what the external stubs return
constexpr std::uint32_t plan_alloc_calls=plan+0x00u,plan_alloc_fail=plan+0x04u,plan_start_calls=plan+0x08u,
    plan_starts=plan+0x10u,plan_override=plan+0x40u,plan_override_value=plan+0x44u,plan_distances=plan+0x100u,
    plan_found=plan+0x200u,plan_rows=plan+0x210u,plan_gains=plan+0x250u,plan_sources=plan+0x290u,
    plan_clusters=plan+0x2d0u;
// The frame routines' data.
constexpr std::uint32_t game_time=0x02000000u;   // [51DF54]: +4 seconds per tick, +8 tick
constexpr std::uint32_t voices=0x02010000u;      // [52D670]: voice array header
constexpr std::uint32_t voice_elements=0x02020000u;
constexpr std::uint32_t voice_bitmap=0x02030000u;
constexpr std::uint32_t channels=0x02040000u;    // [52D668]: channel array header
constexpr std::uint32_t channel_elements=0x02050000u;
constexpr std::uint32_t ambience=0x02060000u;    // [52D684]: two 0x14-byte channels
constexpr std::uint32_t ambience_rows=0x02070000u; // looping definition rows (0x4C)
constexpr std::uint32_t curves=0x02080000u;
constexpr std::uint32_t dsp_state=0x02090000u;   // [52D674]: +0x2AB0 handle
constexpr std::uint32_t dsp_globals=0x020a0000u; // [51DF90]: +0 parameter records, +0x22 dirty mask
constexpr std::uint32_t dsp_records=0x020b0000u; // 32 x 0x20
constexpr std::uint32_t dsp_data=0x020c0000u;    // 0x40 per parameter record
constexpr std::uint32_t gestalt=0x020d0000u;     // [52D664]: +0x24 pitch rows, +0x2C permutations, +0x54 playback rows
constexpr std::uint32_t gestalt_pitch=0x020e0000u;
constexpr std::uint32_t gestalt_permutations=0x020f0000u;
constexpr std::uint32_t gestalt_playback=0x02100000u;
constexpr std::uint32_t class_rows=0x02110000u;  // what 228220 returns
constexpr std::uint32_t listener_vector=0x02120000u; // [474CE8]
// The sources group's data (18D5A0 runs inside 18EE50).
constexpr std::uint32_t sound_globals=0x02200000u;  // [4ECC1C]
constexpr std::uint32_t class_reference=0x02210000u;
constexpr std::uint32_t pool3=0x02220000u;          // [4ECBD0]
constexpr std::uint32_t pool3_rows=0x02230000u;
constexpr std::uint32_t pool3_records=0x02240000u;
constexpr std::uint32_t pool2=0x02260000u;          // [52D68C], [4F720C]
constexpr std::uint32_t pool2_rows=0x02270000u;
constexpr std::uint32_t audible_clusters=0x55d730u; // retail global bit array

// Retail .rdata words the lifted routines read (TU1.5 XBE).
constexpr std::pair<std::uint32_t,std::uint32_t> retail[]{
    {0x44923cu,0x40c90fdbu},{0x455b0cu,0xc2800000u},{0x455b14u,0x40135d8eu},{0x45695cu,0x7f7fffffu},
    {0x46a0ccu,0x3f000000u},{0x46a0d0u,0x3f800000u},{0x46a0dcu,0xbf800000u},{0x46a0e4u,0x40400000u},
    {0x46a0e8u,0x00000000u},{0x46a0ecu,0x38d1b717u},{0x46a110u,0x41200000u},{0x46a118u,0x41a00000u},
    {0x46a11cu,0x40000000u},{0x46a120u,0x40800000u},{0x46a12cu,0x3d4ccccdu},{0x46a148u,0x437f0000u},
    {0x46a180u,0x3a83126fu},{0x46a18cu,0x3c23d70au},{0x46a1dcu,0x447a0000u},{0x46a20cu,0x41f00000u},
    {0x46a26cu,0x43800000u},{0x46a2d4u,0x40010204u},{0x46a2d8u,0x3b81848eu},{0x46a2ecu,0x40490fdbu},
    {0x46a580u,0xc0c90fdbu},{0x46a584u,0x46a2f8e0u}};

std::uint32_t rd32(const M &m,std::uint32_t p){std::array<std::byte,4> b;m.read(p,b);return std::bit_cast<std::uint32_t>(b);}
std::uint32_t rd8(const M &m,std::uint32_t p){std::array<std::byte,1> b;m.read(p,b);return std::to_integer<std::uint32_t>(b[0]);}
void wr32(M &m,std::uint32_t p,std::uint32_t v){const auto b=std::bit_cast<std::array<std::byte,4>>(v);m.write(p,b);}
void wr16(M &m,std::uint32_t p,std::uint32_t v){const auto b=std::bit_cast<std::array<std::byte,2>>(std::uint16_t(v));m.write(p,b);}
void wr8(M &m,std::uint32_t p,std::uint32_t v){const std::array<std::byte,1> b{std::byte(v)};m.write(p,b);}
std::uint32_t sign16(std::uint32_t x){return std::uint32_t(std::int32_t(std::int16_t(std::uint16_t(x))));}
std::uint32_t bits(float x){return std::bit_cast<std::uint32_t>(x);}

// One external call as the stub saw it.
struct CallRecord {
    std::uint32_t target{},site{};
    std::array<std::uint32_t,8> gp{};
    std::array<std::array<std::uint32_t,4>,8> xmm{};
    unsigned depth{};std::uint16_t status{};
    std::array<std::array<std::uint8_t,10>,8> lanes{};
    bool fp_present{};std::uint16_t cw{};std::uint32_t mxcsr{};
    std::vector<std::uint8_t> stack;   // return address and stack arguments
};
// Stack argument bytes each external pops (RET n), from the TU1.5 image.
unsigned popped(std::uint32_t target) {
    switch(target) {
    case 0x190520u:return 0x24u;
    case 0x140910u:case 0xbcb70u:case 0xbad60u:case 0x1902e0u:case 0x230b10u:case 0x385b4fu:return 0xcu;
    case 0x18cdd0u:case 0x129f10u:case 0xbcdf0u:case 0x2292e0u:case 0x228220u:case 0x21f860u:
    case 0xbc200u:case 0x2208e0u:return 4u;
    case 0x18bd80u:case 0x13de70u:case 0x224040u:case 0x17e160u:return 8u;
    case 0x18f810u:return 0x10u;
    case 0x3856edu:return 0x18u;
    case 0x220c60u:return 0x20u;
    default:return 0u;
    }
}
struct SplitMix {
    std::uint64_t x;
    std::uint32_t next(){x+=0x9e3779b97f4a7c15ull;auto z=x;z=(z^(z>>30))*0xbf58476d1ce4e5b9ull;
        z=(z^(z>>27))*0x94d049bb133111ebull;return std::uint32_t((z^(z>>31))>>7);}
    std::uint32_t below(std::uint32_t n){return next()%n;}
};
std::uint64_t digest(const CallRecord &x) {
    std::uint64_t h=0xcbf29ce484222325ull;
    const auto add=[&](std::uint32_t v){for(unsigned i=0;i<4;++i){h^=(v>>(i*8))&255u;h*=0x100000001b3ull;}};
    add(x.target);add(x.site);for(const auto g:x.gp)add(g);for(const auto &l:x.xmm)for(const auto v:l)add(v);
    add(x.depth);add(x.status);
    for(unsigned i=0;i<x.depth;++i)for(const auto b:x.lanes[i])add(b);
    for(const auto b:x.stack)add(b);
    return h;
}

// A DSP parameter list (226370's input; 2292E0's output): records of dwords
// at `at` (+4 mask, +8 flags, +0xC offset, +0xE length, +0x10 data). Returns
// the list's byte count. Some records copy their parameter's current data so
// both REPE CMPSB outcomes occur.
std::uint32_t parameter_list(M &m,SplitMix &h,std::uint32_t at,std::uint32_t capacity_dwords) {
    std::uint32_t used=0;
    const auto count=h.below(7);
    for(std::uint32_t k=0;k<count;++k) {
        const auto length=h.below(5)==0?0u:1u+h.below(h.below(4)?12u:40u);
        const auto need=(length+15u)>>2;
        if(used+need+1u>capacity_dwords)break;
        const auto record=at+used*4u;
        std::uint32_t mask;
        switch(h.below(6)) {
        case 0:mask=0u;break;
        case 1:mask=0x80000000u|h.next();break;
        case 2:mask=1u<<h.below(32);break;
        default:mask=h.next()&(h.below(2)?0xffu:0x0000ffffu);break;
        }
        wr32(m,record+4u,mask);
        wr8(m,record+8u,h.below(3)?(h.next()&0xfcu):h.next());
        const auto offset=h.below(4)?h.below(16):h.below(0x100);
        wr16(m,record+0xcu,offset);wr16(m,record+0xeu,length);
        for(std::uint32_t i=0;i<length;++i)wr8(m,record+0x10u+i,h.below(3)?h.below(4):h.next());
        if(h.below(3)==0 && mask) {
            // The lowest set bit's parameter already holds this data.
            const auto bit=std::uint32_t(std::countr_zero(mask));
            const auto data=dsp_data+bit*0x40u;
            for(std::uint32_t i=0;i<length && offset+i<0x40u;++i)wr8(m,data+offset+i,rd8(m,record+0x10u+i));
        }
        used+=need;
    }
    switch(h.below(40)) {
    case 0:case 1:case 2:return 0u;
    case 3:return 1u+h.below(4);                  // shorter than its header
    case 4:return 4u+used*4u+1u+h.below(3);        // not a whole dword count
    default:return 4u+used*4u;
    }
}

// Deterministic owners for the routines outside the group.
struct Stubs {
    std::vector<CallRecord> log;FP fp{};
    void operator()(M &m,const Call &call,Abi &a,const O &o) {
        CallRecord x{call.target,call.instruction,a.gp,a.xmm,a.x87_depth,a.x87_status};
        for(unsigned i=0;i<a.x87_depth && i<8;++i)std::memcpy(x.lanes[i].data(),&a.x87[i],10);
        if(a.floating_point){x.fp_present=true;x.cw=a.floating_point->x87_control_word;x.mxcsr=a.floating_point->mxcsr;}
        const auto esp=a.gp[4];
        x.stack.resize(4u+popped(call.target));
        for(unsigned i=0;i<x.stack.size();++i)x.stack[i]=std::uint8_t(rd8(m,esp+i));
        SplitMix h{digest(x)};
        const auto clobber_gp=[&]{a.gp[0]=h.next();a.gp[1]=h.next();a.gp[2]=h.next();};
        const auto clobber_xmm=[&](unsigned first){for(unsigned n=first;n<7;++n)for(auto &l:a.xmm[n])l=h.next();};
        // A callee may raise floating-point exceptions: sticky flags only.
        const auto raise=[&] {
            if(a.floating_point && !h.below(6))a.floating_point->mxcsr|=h.next()&0x3fu;
            if(!h.below(6))a.x87_status=std::uint16_t(a.x87_status|(h.next()&0x3fu));
        };
        const auto ret=[&]{a.gp[4]+=4u+popped(call.target);};
        const auto low=[&](std::uint32_t v){a.gp[0]=(a.gp[0]&~0xffu)|(v&0xffu);};
        if(call.target!=0x2531e0u)log.push_back(x);
        switch(call.target) {
        // ---- the update group's externals (as tests/native_tu15_sound_object_loop_update_tests.cpp) ----
        case 0x14dd70u: {   // distance from the source (ECX) to a point (EDX): XMM0
            const auto offset=a.gp[1]-sources;
            const auto distance=offset%36u==0u && offset/36u<64u?rd32(m,plan_distances+offset/36u*4u):
                0x40000000u|(h.next()&0x3fffffu);
            clobber_gp();clobber_xmm(1);
            a.xmm[0]={distance,h.next(),h.next(),h.next()};
            ret();break;
        }
        case 0x32ccd8u: {   // CRT arc cosine of ST(0)
            if(!a.x87_depth)throw std::runtime_error("32CCD8 stub without ST(0)");
            a.x87[0]=rd32(m,plan_override)?static_cast<long double>(std::bit_cast<float>(rd32(m,plan_override_value))):
                std::acos(a.x87[0]);
            clobber_gp();ret();break;
        }
        case 0x190520u: {   // the cluster's nearby rows: count and four parallel arrays
            const auto arg=[&](unsigned k){return rd32(m,esp+4u+4u*k);};
            const auto found=rd32(m,plan_found);
            for(std::uint32_t k=0;k<found;++k) {
                wr32(m,arg(4)+4u*k,rd32(m,plan_rows+4u*k));wr32(m,arg(5)+4u*k,rd32(m,plan_gains+4u*k));
                wr32(m,arg(6)+4u*k,rd32(m,plan_sources+4u*k));wr32(m,arg(7)+4u*k,rd32(m,plan_clusters+4u*k));
            }
            if(arg(8))wr32(m,arg(8),found);
            clobber_gp();clobber_xmm(0);ret();break;
        }
        case 0x140910u: {   // sort the indices (ECX, EAX count) by their distances (third argument)
            const auto count=std::int32_t(a.gp[0]);const auto at=a.gp[1],distances=rd32(m,esp+0xcu);
            if(count>64)throw std::runtime_error("140910 stub count");
            std::vector<std::uint32_t> items;
            for(std::int32_t k=0;k<count;++k)items.push_back(rd32(m,at+4u*std::uint32_t(k)));
            const auto key=[&](std::uint32_t index) {
                const auto v=rd32(m,distances+4u*(index&63u));return v&0x80000000u?~v:(v|0x80000000u);
            };
            std::stable_sort(items.begin(),items.end(),[&](auto p,auto q){return key(p)<key(q);});
            for(std::size_t k=0;k<items.size();++k)wr32(m,at+4u*std::uint32_t(k),items[k]);
            clobber_gp();ret();break;
        }
        case 0x220200u:     // amplitude (XMM0) to dB: the native gain kernel
            native_tu15_sound_manager_run(m,0x220200u,a,fp,o,{});break;
        case 0x16e6a0u: {   // new pool element (EDX pool): EAX handle or -1
            const auto calls=rd32(m,plan_alloc_calls);wr32(m,plan_alloc_calls,calls+1u);
            std::uint32_t handle=none;
            if(!((rd32(m,plan_alloc_fail)>>(calls&31u))&1u)) {
                const auto at=a.gp[2],capacity=rd32(m,at+0x38u),used=rd32(m,at+0x48u);
                for(std::uint32_t i=0;i<capacity && i<64u;++i) {
                    const auto word=rd32(m,used+(i>>5)*4u);
                    if(word&(1u<<(i&31u)))continue;
                    wr32(m,used+(i>>5)*4u,word|(1u<<(i&31u)));
                    const auto salt=0x8000u|((calls*0x1d3u+i)&0x7fffu);
                    wr16(m,rd32(m,at+0x44u)+i*0x18u,salt);
                    handle=(salt<<16)|i;break;
                }
            }
            a.gp[0]=handle;a.gp[1]=h.next();a.gp[2]=h.next();ret();break;
        }
        case 0x18cdd0u: {   // start a sound (EAX request, stack tag)
            const auto calls=rd32(m,plan_start_calls);wr32(m,plan_start_calls,calls+1u);
            if(calls<8u){wr32(m,plan_starts+calls*8u,rd32(m,esp+4u));wr32(m,plan_starts+calls*8u+4u,rd32(m,a.gp[0]));}
            clobber_gp();clobber_xmm(0);ret();break;
        }
        case 0x2531e0u: {   // the structure's (EDI) cluster byte for AX, CX (not logged: 253070 is native)
            const auto first=a.gp[0],second=a.gp[1];
            if(std::uint16_t(first)==std::uint16_t(second)){a.gp[0]&=~0xffu;ret();break;}
            const bool swap=std::int16_t(first)>std::int16_t(second);
            const auto lo=sign16(swap?second:first),hi=sign16(swap?first:second);
            a.gp[1]=lo;
            auto triangle=(lo+1u)*lo;
            triangle-=std::int32_t(triangle)<0?none:0u;
            triangle=std::uint32_t(std::int32_t(triangle)>>1);
            const auto row=(rd32(m,a.gp[7]+0x9cu)-1u)*lo-triangle+rd32(m,a.gp[7]+0xe8u);
            a.gp[2]=row;a.gp[0]=(hi&~0xffu)|rd8(m,row+hi-1u);
            ret();break;
        }
        // ---- the frame routines' externals ----
        case 0x129f10u:     // the ducking update (stack: 0)
            clobber_gp();clobber_xmm(0);raise();ret();break;
        case 0xbcdf0u: {    // whether object EAX exists (stack: type): EAX
            const auto found=h.below(3)?(h.next()|1u):0u;
            clobber_gp();a.gp[0]=found;raise();ret();break;
        }
        case 0x16e8a0u: {   // delete datum EAX from the data array EDX
            const auto handle=a.gp[0],array=a.gp[2];
            if(handle!=none) {
                const auto index=handle&0xffffu,capacity=rd32(m,array+0x38u);
                if(index<capacity && index<64u) {
                    const auto word=rd32(m,array+0x48u)+(index>>5)*4u;
                    wr32(m,word,rd32(m,word)&~(1u<<(index&31u)));
                }
            }
            clobber_gp();ret();break;
        }
        case 0xbbd50u:      // whether object EAX is live: AL
            clobber_gp();low(h.below(4)?1u:0u);clobber_xmm(1);ret();break;
        case 0xbc330u: {    // object ECX's location into [EAX]: +4 cluster (0xFFFF none)
            const auto out=a.gp[0];
            wr32(m,out,h.next());
            const auto cluster=h.below(5)?h.below(12):(h.below(2)?0xffffu:h.next()&0x7fffu);
            wr32(m,out+4u,(h.next()&0xffff0000u)|(cluster&0xffffu));
            clobber_gp();ret();break;
        }
        case 0x18bd80u: {   // the looping sound of ESI (stack: -1, 0): EAX handle or -1
            std::uint32_t handle=none;
            if(h.below(4)) {
                const auto index=h.below(h.below(8)?20u:64u);
                handle=((index<20u?rd32(m,pool_elements+index*0x18u)&0xffffu:h.next()&0xffffu)<<16)|index;
            }
            clobber_gp();a.gp[0]=handle;clobber_xmm(1);ret();break;
        }
        case 0x1903a0u: {   // a playback (EAX): EAX
            const auto result=h.below(4)?h.next():none;
            clobber_gp();a.gp[0]=result;ret();break;
        }
        case 0x13de70u: {   // fade curve at the time (stack: time, 0): XMM0
            float value;
            switch(h.below(8)) {
            case 0:value=-0.25f;break;
            case 1:value=1.5f;break;
            case 2:value=std::bit_cast<float>(h.below(2)?0x7fc00000u:0x00000123u);break;
            default:value=float(h.below(1001))/1000.f;break;
            }
            clobber_gp();clobber_xmm(1);a.xmm[0]={bits(value),h.next(),h.next(),h.next()};raise();ret();break;
        }
        case 0x18fe80u: {   // whether the playback (EDX) needs its DSP block: EAX
            const auto result=h.below(3)?1u+h.below(4):0u;
            clobber_gp();a.gp[0]=result;ret();break;
        }
        case 0x2292e0u: {   // the playback's (EDX) DSP block into ESI (0x404 bytes; count at +0x400)
            const auto block=a.gp[6];
            for(std::uint32_t p=block;p<block+0x400u;p+=4)wr32(m,p,h.next());
            wr32(m,block+0x400u,parameter_list(m,h,block,0xffu));
            clobber_gp();clobber_xmm(0);raise();ret();break;
        }
        case 0x224040u:     // stop a voice (stack: handle, mode)
            clobber_gp();clobber_xmm(0);ret();break;
        case 0x223fb0u:     // whether voice ECX finished: AL
            clobber_gp();low(h.below(3)?0u:1u);ret();break;
        case 0x385b4fu:case 0x3856edu:   // DSP parameter posts
            clobber_gp();raise();ret();break;
        case 0x228220u:     // the sound class (stack: index): EAX row (+8 flags)
            clobber_gp();a.gp[0]=class_rows+h.below(4)*0x10u;ret();break;
        case 0x130980u:     // whether the playback row (ECX) is ready: AL
            clobber_gp();low(h.below(3)?1u:0u);ret();break;
        case 0x21f860u:case 0x18f7a0u:case 0x18f810u:   // impulse playback
            clobber_gp();clobber_xmm(0);raise();ret();break;
        case 0x213690u: {   // a permutation's duration (EAX, EDX): XMM0
            float value;
            switch(h.below(6)) {
            case 0:value=0.f;break;
            case 1:value=std::bit_cast<float>(0x7fc00000u);break;
            default:value=float(h.below(4000))/1000.f;break;
            }
            clobber_gp();clobber_xmm(1);a.xmm[0]={bits(value),h.next(),h.next(),h.next()};ret();break;
        }
        // ---- the sources group's externals ----
        case 0x220c60u:     // the loop-sound request builder (8 arguments): AL
            clobber_gp();low(h.below(3)?1u:0u);clobber_xmm(0);raise();ret();break;
        case 0x16ea60u:case 0x220320u:case 0x220370u: {   // a slot or platform handle: EAX (-1 none)
            const auto result=h.below(7)?h.below(8):none;
            clobber_gp();a.gp[0]=result;ret();break;
        }
        case 0xbcb70u:case 0xbad60u:case 0x1902e0u:case 0x230b10u:case 0xbc200u:case 0x2208e0u:case 0x17e160u:
        case 0x11e600u:case 0x18bbc0u:case 0xbac70u:case 0xbbe00u:case 0xbbff0u:
            clobber_gp();clobber_xmm(1);ret();break;
        default:
            char t[64];std::snprintf(t,sizeof t,"unplanned external %06x from %06x",call.target,call.instruction);
            throw std::runtime_error(t);
        }
    }
};

float uniform(Random &r,float lo,float hi){return lo+(hi-lo)*float(r.word()>>8)/16777216.f;}
float usual(Random &r,float x,unsigned odds=24){return r.below(odds)?x:r.scalar();}
std::uint32_t pick_id(Random &r) {
    constexpr std::uint32_t ids[]{0x100u,0x101u,0x102u,0x103u,none};
    return ids[r.below(5)];
}
float fade(Random &r) {
    switch(r.below(9)) {
    case 0:return 0.f;
    case 1:return 0.0005f;
    case 2:return std::bit_cast<float>(0x3a83126fu);
    case 3:return 1.f;
    case 4:return r.scalar();
    default:return uniform(r,0.f,1.2f);
    }
}
float distance_limit(Random &r) {
    switch(r.below(10)) {
    case 0:return 0.0005f;
    case 1:return std::bit_cast<float>(0x3a83126fu);
    case 2:return r.scalar();
    case 3:return 0.f;
    default:return uniform(r,1.f,60.f);
    }
}
float rate(Random &r) {
    switch(r.below(8)) {
    case 0:return 0.f;
    case 1:return 0.00005f;
    case 2:return r.scalar();
    default:return uniform(r,0.05f,5.f);
    }
}
float frame_time(Random &r) {
    switch(r.below(8)) {
    case 0:return r.scalar();
    case 1:return 0.f;
    default:return r.below(2)?1.f/30.f:uniform(r,0.001f,0.2f);
    }
}

struct World {
    std::uint32_t records,sources,rows,definitions,clusters,tags,capacity,voices,channels;
    std::vector<std::uint32_t> handles,voice_handles;
    std::uint32_t datum(Random &r,std::uint32_t tag)const{return ((0xe000u|r.below(0x1fffu))<<16)|tag;}
};
// A data array header (+0x24 element size, +0x38 capacity, +0x44 elements,
// +0x48 used bits) with random used bits; returns the element handles.
std::vector<std::uint32_t> data_array(Memory &m,Random &r,std::uint32_t header,std::uint32_t elements,
    std::uint32_t bitmap,std::uint32_t size,std::uint32_t capacity) {
    m.map(header,0x50u);
    m.put32(header+0x24u,size);m.put32(header+0x38u,capacity);m.put32(header+0x44u,elements);m.put32(header+0x48u,bitmap);
    m.map(bitmap,8u);
    m.put32(bitmap,r.below(5)?r.word():(r.below(2)?0u:none));m.put32(bitmap+4u,r.word());
    m.map(elements,std::max(capacity,1u)*size);
    std::vector<std::uint32_t> handles;
    for(std::uint32_t i=0;i<capacity;++i) {
        const auto salt=0x8000u|r.below(0x8000u);
        for(std::uint32_t k=0;k<size;k+=4)m.put32(elements+i*size+k,r.word());
        m.put16(elements+i*size,std::uint16_t(salt));
        handles.push_back((salt<<16)|i);
    }
    return handles;
}
std::uint32_t handle_or_none(Random &r,const std::vector<std::uint32_t> &handles) {
    if(handles.empty() || !r.below(4))return none;
    return handles[r.below(std::uint32_t(handles.size()))];
}

// The shared game state every routine walks.
World world(Case &c,Random &r) {
    auto &m=c.memory;
    World w{1u+r.below(8),1u+r.below(16),1u+r.below(10),1u+r.below(r.below(8)?10u:64u),1u+r.below(5),8u,
        1u+r.below(20),1u+r.below(r.below(4)?12u:40u),1u+r.below(12),{},{}};
    for(const auto &[p,v]:retail)m.put32(p,v);
    // The callee frames below the caller hold old garbage.
    m.map(stack-0x10000u,0x11000u);
    for(std::uint32_t p=stack-0x10000u;p<stack;++p)m.put8(p,std::uint8_t(r.word()));
    m.put32(0x4f2c50u,listeners);m.put32(0x4ecc18u,structure);m.put32(0x4f54f0u,players);
    m.put32(0x4f9b58u,player_slots);m.put32(0x4f9b5cu,pool);m.put32(0x4f0414u,tags);
    m.put32(0x515f00u,r.below(2));m.put32(0x51df50u,r.below(3)?flag_block:0u);
    m.map(flag_block,0x10u);m.put8(flag_block+5u,r.below(2));
    m.putf(0x47365cu,r.below(3)?1.f:usual(r,uniform(r,0.f,2.f),6));m.put8(0x474c24u,std::uint8_t(r.word()));
    m.put32(0x51df54u,game_time);m.put32(0x52d670u,voices);m.put32(0x52d668u,channels);m.put32(0x52d684u,ambience);
    m.put32(0x52d674u,dsp_state);m.put32(0x51df90u,dsp_globals);m.put32(0x52d664u,gestalt);
    m.put32(0x474ce8u,listener_vector);
    // Sound state: ducking bytes, listener records, loop slots, ducking state.
    m.map(listeners,0x300u);
    for(std::uint32_t p=listeners;p<listeners+0x300u;p+=4)m.put32(p,r.word());
    for(const auto at:{0x78u,0x79u,0x7au})m.put8(listeners+at,std::uint8_t(r.below(4)?1u+r.below(3):0u));
    for(std::uint32_t i=0;i<4;++i) {
        const auto at=listeners+0x88u+i*0x48u;
        m.put16(at+4u,std::uint16_t(r.below(8)?r.below(w.records):0xffffu));
        m.put8(at+6u,std::uint8_t(r.below(5)?1u+r.below(3):0u));
        for(std::uint32_t k=0;k<9;++k)m.putf(at+0xcu+k*4u,usual(r,uniform(r,-1.f,1.f),40));
        for(std::uint32_t k=0;k<3;++k)m.putf(at+0x30u+k*4u,usual(r,uniform(r,-40.f,40.f),40));
    }
    for(std::uint32_t j=0;j<2;++j) {
        const auto at=listeners+0x1a8u+j*0x1cu;
        m.putf(at,r.below(4)?uniform(r,0.0001f,3.f):fade(r));m.putf(at+4u,fade(r));m.put32(at+8u,pick_id(r));
        for(std::uint32_t k=0;k<4;++k)m.putf(at+0xcu+k*4u,usual(r,uniform(r,-10.f,10.f)));
    }
    m.putf(listeners+0x1e0u,rate(r));
    {   // ducking: the active type, its clock, the type fading out, its clock
        constexpr std::uint32_t types[]{0u,1u,none,2u};
        m.put32(listeners+0x1f8u,types[r.below(4)]);m.putf(listeners+0x1fcu,uniform(r,0.f,2.f));
        m.put32(listeners+0x200u,types[r.below(4)]);m.putf(listeners+0x204u,uniform(r,0.f,2.f));
    }
    // The sound structure.
    m.map(structure,0x300u);
    m.put32(structure+0x60u,sources);m.put32(structure+0x9cu,w.records);m.put32(structure+0xa0u,records);
    m.put32(structure+0xd8u,rows);m.put32(structure+0xdcu,w.definitions);m.put32(structure+0xe0u,definitions);
    m.put32(structure+0xe8u,cluster_bytes);m.put32(structure+0x154u,clusters);
    m.put32(structure+0x224u,r.below(4)?1u+r.below(3):0u);m.put32(structure+0x228u,scale_table);
    m.map(records,w.records*0xb0u);
    for(std::uint32_t i=0;i<w.records;++i) {
        m.put16(records+i*0xb0u+0x74u,std::uint16_t(r.below(6)?r.below(w.definitions):0xffffu));
        m.put16(records+i*0xb0u+0x7au,std::uint16_t(r.below(w.clusters)));
    }
    m.map(sources,w.sources*36u);
    for(std::uint32_t i=0;i<w.sources;++i) {
        const auto at=sources+i*36u;
        m.put16(at,std::uint16_t(r.below(w.records)));m.put16(at+2u,std::uint16_t(r.below(w.records)));
        for(std::uint32_t k=0;k<3;++k)m.putf(at+8u+k*4u,usual(r,uniform(r,-40.f,40.f),40));
        m.putf(at+0x14u,usual(r,uniform(r,-4.f,4.f)));
        m.put8(at+0x18u,std::uint8_t(r.below(6)?(r.word()&0xd7u):(0x08u|(r.word()&0xffu))));
    }
    m.map(rows,w.rows*0x64u);
    for(std::uint32_t i=0;i<w.rows;++i) {
        const auto at=rows+i*0x64u;
        m.put32(at+0x24u,pick_id(r));m.put32(at+0x2cu,pick_id(r));
        m.putf(at+0x44u,r.below(8)?uniform(r,1.f,60.f):rate(r));
        m.put32(at+0x48u,r.word()&0xfu);
        for(std::uint32_t k=0;k<3;++k)m.putf(at+0x4cu+k*4u,usual(r,uniform(r,0.f,2.f)));
        m.putf(at+0x58u,rate(r));
    }
    m.map(definitions,w.definitions*0x48u);
    for(std::uint32_t i=0;i<w.definitions;++i) {
        const auto at=definitions+i*0x48u;
        m.put32(at+0x24u,pick_id(r));m.putf(at+0x28u,distance_limit(r));m.putf(at+0x2cu,rate(r));
    }
    m.map(clusters,w.clusters*20u);
    for(std::uint32_t i=0;i<w.clusters;++i) {
        const auto count=r.below(5)?1u+r.below(6):0u;
        m.put32(clusters+i*20u+4u,count);m.put32(clusters+i*20u+8u,cluster_words+i*0x40u);
        for(std::uint32_t k=0;k<count;++k)
            m.put16(cluster_words+i*0x40u+k*2u,std::uint16_t(r.below(w.sources)|(r.below(3)?0u:0x8000u)));
    }
    m.map(scale_table,0x30u);
    m.putf(scale_table+4u,uniform(r,0.f,128.f));m.putf(scale_table+8u,uniform(r,128.f,256.f));
    m.put32(scale_table+0x28u,scale_bytes);
    m.map(scale_bytes,0x2000u);m.map(cluster_bytes,0x2000u);
    for(std::uint32_t k=0;k<0x2000u;++k) {
        const auto pick=r.below(5);
        m.put8(scale_bytes+k,std::uint8_t(pick==0?0u:pick==1?0xffu:r.word()));
        m.put8(cluster_bytes+k,std::uint8_t(r.word()));
    }
    // Cluster bits around the array cover any 16-bit cluster index.
    m.map((audible_clusters&~0xfffu)-0x1000u,0x3000u);
    for(std::uint32_t p=(audible_clusters&~0xfffu)-0x1000u;p<(audible_clusters&~0xfffu)+0x2000u;p+=4)m.put32(p,r.word());
    // Local players, per-player loops and impulse slots, the loop pool.
    m.map(players,0x20u);
    for(std::uint32_t i=0;i<4;++i)m.put32(players+0xcu+i*4u,r.below(3)?i:none);
    w.handles=data_array(m,r,pool,pool_elements,pool_bitmap,0x18u,w.capacity);
    for(std::uint32_t i=0;i<w.capacity;++i) {
        const auto at=pool_elements+i*0x18u;
        m.put8(at+2u,std::uint8_t(r.below(3)?r.below(4):r.word()));
        // Mode (+3): mostly object-attached (1), the others start or update.
        m.put8(at+3u,std::uint8_t(r.below(3)?1u:(r.below(6)?r.below(5):r.word())));
        // +6/+0x16: object attachment indices (0xFFFF: none).
        m.put16(at+4u,std::uint16_t(r.word()));m.put16(at+6u,std::uint16_t(r.below(4)?0xffffu:r.below(16)));
        m.putf(at+8u,usual(r,uniform(r,0.f,1.5f)));
        // +0xC: the sound definition (a tag datum) or, for attached loops, the owner's handle.
        m.put32(at+0xcu,r.below(3)?w.datum(r,r.below(w.tags)):w.handles[r.below(w.capacity)]);
        // +0x10: an attached loop's object datum (index below 0x40), else a tick or gain.
        if(rd8(m,at+3u)==1u)m.put32(at+0x10u,r.below(12)?((0x8000u|r.below(0x8000u))<<16)|r.below(0x40u):none);
        else m.put32(at+0x10u,r.below(4)?r.below(40):(r.below(2)?none:bits(usual(r,uniform(r,-70.f,5.f)))));
        m.put8(at+0x14u,std::uint8_t(r.below(5)?r.below(w.rows):0xffu));
        m.put8(at+0x15u,std::uint8_t(r.word()));m.put16(at+0x16u,std::uint16_t(r.below(4)?0xffffu:r.below(16)));
    }
    // Every tag index a handle or datum can carry (up to 0x110) has data.
    m.map(tags,0x110u*16u);
    for(std::uint32_t t=0;t<0x110u;++t)m.put32(tags+t*16u+8u,tag_data+(t%w.tags)*0x200u);
    for(std::uint32_t t=0;t<w.tags;++t) {
        const auto at=tag_data+t*0x200u;
        m.map(at,0x200u);m.put32(tags+t*16u+8u,at);
        for(std::uint32_t k=0;k<0x200u;k+=4)m.put32(at+k,r.word());
        m.put8(at+1u,std::uint8_t(r.below(4)?(r.word()&0xfdu):r.word()));   // +1 bit 1: muted
        m.put8(at+2u,std::uint8_t(r.below(4)));                              // class
        m.put16(at+8u,std::uint16_t(r.below(4)));                            // pitch range base
        m.put16(at+0xeu,std::uint16_t(r.below(5)?r.below(6):0xffffu));       // playback row
        m.put32(at+0x3cu,pick_id(r));
        m.putf(at+0x48u,usual(r,uniform(r,-0.5f,1.5f)));m.putf(at+0x4cu,usual(r,uniform(r,-0.5f,1.5f)));
        m.putf(at+0x54u,usual(r,uniform(r,-0.5f,1.5f)));m.put32(at+0x5cu,r.below(3)?none:r.word());
        m.put32(at+0x70u,ambience_rows+t*0x200u);                            // ambience definition rows
    }
    m.map(player_slots,0x300u);
    for(std::uint32_t p=player_slots;p<player_slots+0x300u;p+=4)m.put32(p,r.word());
    for(std::uint32_t i=0;i<4;++i) {
        m.put32(player_slots+i*8u,r.below(3)?w.handles[r.below(w.capacity)]:none);
        m.put32(player_slots+i*8u+4u,r.below(3)?none:w.datum(r,r.below(w.tags)));
    }
    m.put8(player_slots+0x240u,std::uint8_t(r.below(2)));
    // Game time and the impulse slots.
    m.map(game_time,0x10u);
    const auto tick=r.below(4)?1000u+r.below(100000u):r.word();
    m.putf(game_time+4u,r.below(8)?1.f/30.f:usual(r,uniform(r,0.f,0.1f),3));m.put32(game_time+8u,tick);
    m.put32(player_slots+0x24u,r.below(2)?tick+r.below(60u)-30u:r.word());
    for(std::uint32_t i=0;i<16;++i) {
        const auto at=player_slots+0x128u+i*0x10u;
        m.put32(at,w.datum(r,r.below(w.tags)));
        m.put32(at+4u,r.below(6)?tick-r.below(r.below(3)?200u:3000u):r.word());
        m.put32(at+8u,r.below(5)?r.word():none);
        m.put8(at+0xcu,std::uint8_t(r.below(3)));m.put8(at+0xdu,std::uint8_t(r.below(4)));
        m.put8(at+0xeu,std::uint8_t(r.below(3)?r.below(3):0u));
    }
    // Voices and channels.
    w.voice_handles=data_array(m,r,voices,voice_elements,voice_bitmap,0x14u,w.voices);
    const auto channel_handles=data_array(m,r,channels,channel_elements,channel_elements+0x1000u,0x1cu,w.channels);
    for(std::uint32_t i=0;i<w.voices;++i) {
        const auto at=voice_elements+i*0x14u;
        m.put8(at+4u,std::uint8_t(r.word()));
        m.put32(at+8u,handle_or_none(r,channel_handles));
    }
    for(std::uint32_t i=0;i<w.channels;++i) {
        const auto pick=r.below(4);
        m.put8(channel_elements+i*0x1cu+2u,std::uint8_t(pick==0?(r.word()&0x80u)|1u:pick==1?(r.word()&0x80u):r.word()));
    }
    // Ambience channels and their definition rows.
    m.map(ambience,0x28u);
    for(std::uint32_t j=0;j<2;++j) {
        const auto at=ambience+j*0x14u;
        m.put8(at,std::uint8_t(r.below(4)?1u+r.below(2):0u));
        for(std::uint32_t k=1;k<4;++k)m.put8(at+k,std::uint8_t(r.word()));
        m.put32(at+4u,handle_or_none(r,w.voice_handles));
        m.putf(at+8u,r.below(5)?uniform(r,0.f,8.f):fade(r));
        m.put32(at+0xcu,w.datum(r,r.below(w.tags)));
        if(j==0)m.put32(at+0x10u,r.below(4));
        else m.putf(at+0x10u,usual(r,uniform(r,0.f,1.f)));
    }
    for(std::uint32_t t=0;t<w.tags;++t)for(std::uint32_t k=0;k<4;++k) {
        const auto at=ambience_rows+t*0x200u+k*0x4cu;
        m.map(at,0x4cu);
        for(std::uint32_t q=0;q<0x4cu;q+=4)m.put32(at+q,r.word());
        m.put32(at+0x3cu,r.word());
        m.putf(at+0x40u,r.below(6)?uniform(r,0.f,6.f):fade(r));                // length / fade time
        m.put32(at+0x44u,r.below(4)?1u+r.below(3):(r.below(2)?0u:none));        // curve count
        m.put32(at+0x48u,r.below(5)?curves+(t*4u+k)*0x10u:0u);                  // curve (0: none)
        const auto curve=curves+(t*4u+k)*0x10u;
        m.map(curve,0x10u);
        m.put8(curve+1u,std::uint8_t(r.below(3)?0u:r.word()));
        m.putf(curve+4u,usual(r,uniform(r,0.f,1.f)));m.putf(curve+8u,usual(r,uniform(r,0.f,1.f)));
    }
    // DSP: the parameter records and their data.
    m.map(dsp_state,0x3000u);m.put32(dsp_state+0x2ab0u,r.word());
    m.map(dsp_globals,0x30u);m.put32(dsp_globals,dsp_records);m.put32(dsp_globals+0x20u,r.word());
    m.map(dsp_records,0x400u);m.map(dsp_data,0x1000u);
    for(std::uint32_t b=0;b<32;++b) {
        m.put32(dsp_records+b*0x20u+0x10u,dsp_data+b*0x40u);
        m.put32(dsp_records+b*0x20u+0x14u,r.below(5)?r.below(17):r.word());
        for(std::uint32_t k=0;k<0x40u;++k)m.put8(dsp_data+b*0x40u+k,std::uint8_t(r.below(3)?r.below(4):r.word()));
    }
    m.map(listener_vector,0x10u);
    for(std::uint32_t k=0;k<3;++k)m.putf(listener_vector+k*4u,uniform(r,-40.f,40.f));
    // The gestalt (impulse sounds) and sound classes.
    m.map(gestalt,0x60u);
    m.put32(gestalt+0x24u,gestalt_pitch);m.put32(gestalt+0x2cu,gestalt_permutations);m.put32(gestalt+0x54u,gestalt_playback);
    m.map(gestalt_pitch,0x400u);m.map(gestalt_permutations,0x400u);m.map(gestalt_playback,0x400u);
    for(std::uint32_t k=0;k<0x400u;k+=4) {
        m.put32(gestalt_pitch+k,r.word());m.put32(gestalt_permutations+k,r.word());m.put32(gestalt_playback+k,r.word());
    }
    for(std::uint32_t k=0;k<0x40u;++k)m.put16(gestalt_pitch+k*12u+8u,std::uint16_t(r.below(8)));
    for(std::uint32_t k=0;k<8;++k)m.put32(gestalt_playback+k*0x2cu,r.below(3)?r.below(3):r.word());
    m.map(class_rows,0x40u);
    for(std::uint32_t k=0;k<4;++k)m.put8(class_rows+k*0x10u+8u,std::uint8_t(r.word()));
    // The sources group's tables (18D5A0, 18F9B0, 1904D0).
    m.put32(0x4ecc1cu,sound_globals);
    m.map(sound_globals,0x100u);m.put32(sound_globals+0xc0u,1u+r.below(3));m.put32(sound_globals+0xc4u,class_reference);
    m.map(class_reference,0x20u);m.put32(class_reference+0x14u,r.below(4)?r.below(w.tags):none);
    m.put32(0x4ecbd0u,pool3);m.map(pool3,0x50u);m.put32(pool3+0x44u,pool3_rows);
    m.map(pool3_rows,0x40u*0xcu);
    for(std::uint32_t i=0;i<0x40u;++i) {
        m.put8(pool3_rows+i*0xcu+3u,std::uint8_t(r.below(3)?10u:r.word()));
        m.put32(pool3_rows+i*0xcu+8u,pool3_records+(i&15u)*0x200u);
    }
    // 18F9B0 reads the object array's +3 byte at index 0xFFFF for a loop
    // without an object (the retail array's storage covers it).
    m.map(pool3_rows+0xbf000u,0x1000u);
    for(std::uint32_t p=pool3_rows+0xbf000u;p<pool3_rows+0xc0000u;p+=4)m.put32(p,r.word());
    m.put32(pool3_rows+0xffffu*0xcu+8u,pool3_records);
    m.map(pool3_records,16u*0x200u);
    for(std::uint32_t p=pool3_records;p<pool3_records+16u*0x200u;p+=4)m.put32(p,r.word());
    m.put32(0x52d68cu,pool2);m.put32(0x4f720cu,pool2);m.map(pool2,0x50u);m.put32(pool2+0x44u,pool2_rows);
    m.map(pool2_rows,0x40u*0x190u);
    for(std::uint32_t p=pool2_rows;p<pool2_rows+0x40u*0x190u;p+=4)m.put32(p,r.word());
    m.map(pool2_rows+0xffffu*0x190u,0x200u);   // index 0xFFFF (no object)
    for(std::uint32_t p=pool2_rows+0xffffu*0x190u;p<pool2_rows+0xffffu*0x190u+0x200u;p+=4)m.put32(p,r.word());
    m.put32(0x474d10u,r.word());
    m.map(0x44e000u,0x2000u);m.put32(0x44ebd8u,0u);
    m.map(0x455800u,0x1000u);
    for(std::uint32_t p=0x455800u;p<0x456800u;p+=4u)m.put32(p,0xccccccccu);
    {
        static const std::uint32_t t_a[7]{1,1,2,5,5,1,6},t_b[7]{3,3,2,4,4,3,6},t_c[7]{0x14,0x1c,0x24,0x40,0x2c,0x48,0x34};
        for(unsigned i=0;i<7;++i){m.put32(0x455ba8u+i*4u,t_a[i]);m.put32(0x455bc4u+i*4u,t_b[i]);m.put32(0x455be0u+i*4u,t_c[i]);}
    }
    for(const auto &[p,v]:retail)m.put32(p,v);
    // What the update group's stubs return.
    m.map(plan,0x400u);
    m.put32(plan_alloc_fail,r.below(3)?0u:r.word());
    if(!r.below(5)) {
        m.put32(plan_override,1u);
        constexpr float choices[]{7.f,-7.f,0.5f,3.5f,-0.25f};
        m.putf(plan_override_value,r.below(6)?choices[r.below(5)]:r.scalar());
    }
    for(std::uint32_t s=0;s<64;++s)m.putf(plan_distances+s*4u,r.below(16)?uniform(r,0.f,60.f):r.scalar());
    const auto found=r.below(6)?1u+r.below(4):(r.below(2)?0u:5u+r.below(12));
    m.put32(plan_found,found);
    for(std::uint32_t k=0;k<16;++k) {
        m.put32(plan_rows+k*4u,r.below(6)?r.below(w.rows):none);
        m.putf(plan_gains+k*4u,usual(r,uniform(r,-30.f,5.f)));
        m.put32(plan_sources+k*4u,r.below(6)?r.below(w.sources):none);
        m.put32(plan_clusters+k*4u,r.below(w.records));
    }
    m.map(heap,0x1000u);
    for(std::uint32_t p=heap;p<heap+0x1000u;p+=4)m.put32(p,r.word());
    return w;
}

Case scenario(Random &r,std::uint32_t entry,Views &views,const Views *&bound) {
    auto c=base_case(r);
    auto w=world(c,r);
    auto &m=c.memory;auto &a=c.abi;
    // The caller's stack pointer, at either 8-byte phase (22C560 aligns).
    const auto esp=stack-4u*r.below(2);
    a.gp[4]=esp;m.put32(esp,0x00abcdefu);
    const auto arg=[&](unsigned k,std::uint32_t v){m.put32(esp+4u+4u*k,v);};
    if(!r.below(16))a.eflags|=0x400u;   // DF: the string instructions run downward
    bound=nullptr;
    const auto bind_views=[&] {
        for(auto &view:views) {
            if(r.below(3))view=SourceObjectAudioViewTu15{{uniform(r,-40.f,40.f),uniform(r,-40.f,40.f),uniform(r,-40.f,40.f)},
                std::int16_t(r.below(6)?std::int32_t(r.below(w.records)):-1)};
            else view.reset();
        }
        bound=&views;
    };
    switch(entry) {
    case 0x18d3d0u:   // EAX: a pool handle
        a.gp[0]=r.below(6)?w.handles[r.below(w.capacity)]:(r.word()&0xffff001fu);
        break;
    case 0x18d390u:   // EAX: the owner (-1: none); XMM0: the gain
        a.gp[0]=r.below(4)?r.word():none;
        a.xmm[0][0]=bits(usual(r,uniform(r,0.f,1.f)));
        break;
    case 0x129dd0u: { // ECX: the ducking type
        constexpr std::uint32_t types[]{0u,1u,none,2u};
        a.gp[1]=r.below(3)?m.get32(listeners+(r.below(2)?0x1f8u:0x200u)):types[r.below(4)];
        break;
    }
    case 0x18f900u:
        if(r.below(2)) {   // more clusters than one bit word
            const auto count=r.below(10)?1u+r.below(r.below(2)?40u:90u):(r.below(2)?0u:0x80000000u|r.below(4));
            m.put32(structure+0x9cu,count);
            for(std::uint32_t i=0;i<4;++i)
                m.put16(listeners+0x88u+i*0x48u+4u,std::uint16_t(r.below(8)?r.below(std::max(count&0xffu,1u)):0xffffu));
        }
        if(r.below(2))bind_views();
        break;
    case 0x226370u: { // EAX: the byte count; stack: the list
        SplitMix h{(std::uint64_t(r.word())<<32)|r.word()};
        const auto count=parameter_list(m,h,heap+0x400u,0xffu);
        a.gp[0]=r.below(16)?count:r.word()&0x3ffu;
        arg(0,r.below(16)?heap+0x400u:0u);
        break;
    }
    case 0x223da0u:
        break;
    case 0x18f510u: { // EDI: the sound datum; stack: handle, pitch range, permutation, time
        a.gp[7]=w.datum(r,r.below(w.tags));
        arg(0,r.below(4)?r.word():none);arg(1,sign16(r.below(4)));arg(2,r.below(4));
        arg(3,bits(r.below(5)?uniform(r,0.f,4.f):r.scalar()));
        break;
    }
    case 0x18ed80u:
        break;
    case 0x22c560u:   // stack: the elapsed time
        arg(0,bits(frame_time(r)));
        break;
    case 0x18ee50u:   // stack: the elapsed time
        arg(0,bits(frame_time(r)));
        for(std::uint32_t i=0;i<4;++i)if(m.get32(players+0xcu+i*4u)!=none) {
            if(r.below(2))m.put32(plan_clusters,sign16(m.get16(listeners+0x88u+i*0x48u+4u)));
            break;
        }
        if(r.below(2))bind_views();
        break;
    }
    return c;
}

// Instruction coverage of the lifted routines.
constexpr std::uint32_t routines[]{0x18d3d0u,0x18d390u,0x129dd0u,0x18f900u,0x226370u,0x223da0u,0x18f510u,
    0x18ed80u,0x22c560u,0x18ee50u};
std::map<std::uint32_t,std::set<std::uint32_t>> routine_pcs() {
    std::ifstream file("src/audio/source_object_loop_audio_tu15_generated.cpp");
    if(!file)throw std::runtime_error("lifted object-loop source not found (run from the repository root)");
    std::map<std::uint32_t,std::set<std::uint32_t>> pcs;
    const std::regex begin(R"(^static void run_([0-9a-f]+)\(WorldState &s\) \{)"),pc(R"(s\.services\.instruction\(0x([0-9a-f]+)u\))");
    std::string line;std::uint32_t current=0;
    while(std::getline(file,line)) {
        std::smatch match;
        if(std::regex_search(line,match,begin)) {
            const auto address=std::uint32_t(std::stoul(match[1],nullptr,16));
            current=std::find(std::begin(routines),std::end(routines),address)!=std::end(routines)?address:0u;
            continue;
        }
        if(line=="}"){current=0;continue;}
        if(!current)continue;
        for(auto it=std::sregex_iterator(line.begin(),line.end(),pc);it!=std::sregex_iterator();++it)
            pcs[current].insert(std::uint32_t(std::stoul((*it)[1],nullptr,16)));
    }
    return pcs;
}

struct Totals {
    unsigned cases=0,faults=0,dead_stack_cases=0;
    std::string first_dead_stack;
    std::unordered_set<std::uint32_t> executed;
    std::map<std::uint32_t,unsigned> calls;        // external calls by target (lifted)
    std::map<std::string,unsigned> fault_kinds;
};
const char *gp_name(unsigned i){static const char *n[]{"eax","ecx","edx","ebx","esp","ebp","esi","edi"};return n[i];}
std::string hex(std::uint64_t v){char t[24];std::snprintf(t,sizeof t,"%llx",static_cast<unsigned long long>(v));return t;}

// Differences between two call logs, or empty.
std::string compare_logs(const std::vector<CallRecord> &n,const std::vector<CallRecord> &l) {
    for(std::size_t k=0;k<std::min(n.size(),l.size());++k) {
        const auto &x=n[k],&y=l[k];
        const auto at="external call "+std::to_string(k)+" ("+hex(y.target)+" from "+hex(y.site)+") ";
        if(x.target!=y.target||x.site!=y.site)return at+"target native="+hex(x.target)+"/"+hex(x.site);
        for(unsigned i=0;i<8;++i)if(x.gp[i]!=y.gp[i])
            return at+gp_name(i)+" native="+hex(x.gp[i])+" lifted="+hex(y.gp[i]);
        for(unsigned i=0;i<8;++i)for(unsigned j=0;j<4;++j)if(x.xmm[i][j]!=y.xmm[i][j])
            return at+"xmm"+std::to_string(i)+"["+std::to_string(j)+"] native="+hex(x.xmm[i][j])+" lifted="+hex(y.xmm[i][j]);
        if(x.depth!=y.depth)return at+"x87 depth native="+std::to_string(x.depth)+" lifted="+std::to_string(y.depth);
        if(x.status!=y.status)return at+"x87 status native="+hex(x.status)+" lifted="+hex(y.status);
        for(unsigned i=0;i<y.depth;++i)if(x.lanes[i]!=y.lanes[i])return at+"st"+std::to_string(i);
        if(x.fp_present!=y.fp_present||x.cw!=y.cw||x.mxcsr!=y.mxcsr)
            return at+"fp controls native="+hex(x.cw)+"/"+hex(x.mxcsr)+" lifted="+hex(y.cw)+"/"+hex(y.mxcsr);
        for(std::size_t i=0;i<y.stack.size();++i)if(x.stack[i]!=y.stack[i])
            return at+"stack byte esp+"+hex(i)+" native="+hex(x.stack[i])+" lifted="+hex(y.stack[i]);
    }
    if(n.size()!=l.size())return "external call count native="+std::to_string(n.size())+" lifted="+std::to_string(l.size());
    return {};
}

struct Options {
    bool strict_dead_stack=false;   // HALO2_STRICT_DEAD_STACK=1: dead scratch differences fail
    bool lifted_both=false;         // HALO2_LOOP_TEST_LIFTED_BOTH=1: harness self-check (lifted vs lifted)
    std::set<std::uint32_t> only;   // HALO2_LOOP_TEST_ONLY=18ee50,22c560: these routines only
    unsigned cases=0;               // HALO2_LOOP_TEST_CASES=n: cases per routine
    bool verbose=false;             // HALO2_LOOP_TEST_VERBOSE=1: print every matching fault
    std::uint32_t dead_floor=0;     // HALO2_DEAD_STACK_FLOOR=n: strict checks only (entry ESP - n) and above
};
// One case: the native routine (callees native or lifted) against the lifted
// routine, from identical copies of the state.
bool agree_loop(const Case &c,std::uint32_t entry,const Views *views,unsigned n,Totals &totals,const Options &options) {
    auto native=c,lifted=c;native.memory=c.memory.clone();lifted.memory=c.memory.clone();
    Stubs native_stubs{{},c.fp},lifted_stubs{{},c.fp};
    Services native_services,lifted_services;
    native_services.required_owner=[&](M &m,const Call &call,Abi &a,const O &o){native_stubs(m,call,a,o);};
    lifted_services.required_owner=[&](M &m,const Call &call,Abi &a,const O &o){lifted_stubs(m,call,a,o);};
    std::uint32_t last_pc=0;
    lifted_services.instruction=[&](std::uint32_t pc){totals.executed.insert(pc);last_pc=pc;};
    std::string native_fault,lifted_fault;
    try {
        if(options.lifted_both)source_run_object_loop_audio_tu15(native.memory,entry,native.abi,native.fp,native_services,{},views);
        else native_tu15_object_loop_run(native.memory,entry,native.abi,native.fp,native_services,views);
    }
    catch(const std::exception &e){native_fault=e.what();if(native_fault.empty())native_fault="(empty)";}
    try{source_run_object_loop_audio_tu15(lifted.memory,entry,lifted.abi,lifted.fp,lifted_services,{},views);}
    catch(const std::exception &e){lifted_fault=e.what();if(lifted_fault.empty())lifted_fault="(empty)";}
    const auto fail=[&](const std::string &why) {
        std::cerr<<hex(entry)<<" case "<<n<<": "<<why<<'\n';
        if(const auto log=compare_logs(native_stubs.log,lifted_stubs.log);!log.empty())std::cerr<<"  first log difference: "<<log<<'\n';
        return false;
    };
    if(native_fault!=lifted_fault)
        return fail("fault native \""+native_fault+"\" lifted \""+lifted_fault+"\"");
    for(const auto &x:lifted_stubs.log)++totals.calls[x.target];
    if(!lifted_fault.empty()) {
        ++totals.faults;
        ++totals.fault_kinds[hex(entry)+": "+std::regex_replace(lifted_fault,std::regex("[0-9a-f]{8}"),"*")];
        if(options.verbose)std::cerr<<hex(entry)<<" case "<<n<<" fault: "<<lifted_fault<<" at "<<hex(last_pc)<<'\n';
        return true;
    }
    const auto &x=native.abi,&y=lifted.abi;
    for(unsigned i=0;i<8;++i)if(x.gp[i]!=y.gp[i])
        return fail(std::string(gp_name(i))+" native="+hex(x.gp[i])+" lifted="+hex(y.gp[i]));
    for(unsigned i=0;i<8;++i)for(unsigned j=0;j<4;++j)if(x.xmm[i][j]!=y.xmm[i][j])
        return fail("xmm"+std::to_string(i)+"["+std::to_string(j)+"] native="+hex(x.xmm[i][j])+" lifted="+hex(y.xmm[i][j]));
    if(x.x87_depth!=y.x87_depth)return fail("x87 depth native="+std::to_string(x.x87_depth)+" lifted="+std::to_string(y.x87_depth));
    if(x.x87_status!=y.x87_status)return fail("x87 status native="+hex(x.x87_status)+" lifted="+hex(y.x87_status));
    for(unsigned i=0;i<y.x87_depth && i<8;++i)if(std::memcmp(&x.x87[i],&y.x87[i],10))return fail("st"+std::to_string(i));
    if(bool(x.floating_point)!=bool(y.floating_point))return fail("floating-point controls presence");
    if(y.floating_point) {
        if(x.floating_point->x87_control_word!=y.floating_point->x87_control_word)
            return fail("x87 control native="+hex(x.floating_point->x87_control_word)+" lifted="+hex(y.floating_point->x87_control_word));
        if(x.floating_point->mxcsr!=y.floating_point->mxcsr)
            return fail("mxcsr native="+hex(x.floating_point->mxcsr)+" lifted="+hex(y.floating_point->mxcsr));
    }
    if(const auto why=compare_logs(native_stubs.log,lifted_stubs.log);!why.empty())return fail(why);
    // Memory: every byte. Below the caller's stack pointer after the return
    // is the callees' dead scratch: counted separately (fatal when strict).
    const auto dead_end=y.gp[4];
    bool dead_differs=false;
    for(const auto &[page,bytes]:lifted.memory.pages) {
        const auto found=native.memory.pages.find(page);
        if(found==native.memory.pages.end())return fail("page "+hex(page<<12)+" missing natively");
        const auto &other=*found->second;
        for(unsigned i=0;i<4096;++i) {
            if((*bytes)[i]==other[i])continue;
            const auto p=(page<<12)+i;
            const auto why="store differs at "+hex(p)+" native="+hex(std::to_integer<unsigned>(other[i]))+
                " lifted="+hex(std::to_integer<unsigned>((*bytes)[i]));
            if(p>=stack-0x10000u && p<dead_end) {
                if(options.strict_dead_stack && (!options.dead_floor || p>=c.abi.gp[4]-options.dead_floor))
                    return fail("dead stack "+why);
                if(!dead_differs && totals.first_dead_stack.empty())
                    totals.first_dead_stack=hex(entry)+" case "+std::to_string(n)+": "+why;
                dead_differs=true;continue;
            }
            return fail(why);
        }
    }
    if(native.memory.pages.size()!=lifted.memory.pages.size())return fail("page count differs");
    if(dead_differs)++totals.dead_stack_cases;
    return true;
}
} // namespace

int main() {
    const auto pcs=routine_pcs();
    const auto set=[](const char *name){const auto *v=std::getenv(name);return v && *v && *v!='0';};
    Options options{set("HALO2_STRICT_DEAD_STACK"),set("HALO2_LOOP_TEST_LIFTED_BOTH"),{},0u};
    if(const auto *only=std::getenv("HALO2_LOOP_TEST_ONLY")) {
        std::string list=only;
        for(std::size_t at=0;at<list.size();) {
            auto end=list.find(',',at);if(end==std::string::npos)end=list.size();
            if(end>at)options.only.insert(std::uint32_t(std::stoul(list.substr(at,end-at),nullptr,16)));
            at=end+1;
        }
    }
    if(const auto *cases=std::getenv("HALO2_LOOP_TEST_CASES"))options.cases=unsigned(std::stoul(cases));
    options.verbose=set("HALO2_LOOP_TEST_VERBOSE");
    if(const auto *floor=std::getenv("HALO2_DEAD_STACK_FLOOR"))options.dead_floor=std::uint32_t(std::stoul(floor,nullptr,0));
    Totals totals;
    constexpr std::pair<std::uint32_t,unsigned> plan_cases[]{
        {0x18d3d0u,2000u},{0x18d390u,3000u},{0x129dd0u,3000u},{0x18f900u,4000u},{0x226370u,6000u},
        {0x223da0u,4000u},{0x18f510u,6000u},{0x18ed80u,3000u},{0x22c560u,6000u},{0x18ee50u,4000u}};
    for(const auto &[entry,count]:plan_cases) {
        if(!options.only.empty() && !options.only.contains(entry))continue;
        Random r(0xf2a3e000u^entry);
        const auto total=options.cases?options.cases:count;
        for(unsigned n=0;n<total;++n) {
            Views views{};const Views *bound=nullptr;
            const auto c=scenario(r,entry,views,bound);
            if(!agree_loop(c,entry,bound,n,totals,options))return 1;
            ++totals.cases;
        }
    }
    for(const auto entry:routines) {
        if(!options.only.empty() && !options.only.contains(entry) && !options.only.contains(0x18ee50u))continue;
        const auto found=pcs.find(entry);
        if(found==pcs.end()){std::cerr<<"no lifted body for "<<hex(entry)<<'\n';return 1;}
        const auto &all=found->second;
        std::vector<std::uint32_t> missed;
        for(const auto pc:all)if(!totals.executed.contains(pc))missed.push_back(pc);
        std::string ranges;
        for(std::size_t i=0;i<missed.size();) {
            auto j=i;
            while(j+1<missed.size() && std::next(all.find(missed[j]))!=all.end() && *std::next(all.find(missed[j]))==missed[j+1])++j;
            ranges+=(ranges.empty()?"":",")+hex(missed[i])+(j>i?"-"+hex(missed[j]):"");
            i=j+1;
        }
        std::printf("coverage %06x: %zu/%zu instructions%s%s\n",entry,all.size()-missed.size(),all.size(),
            missed.empty()?"":"; never executed: ",ranges.c_str());
    }
    std::string calls;
    for(const auto &[target,count]:totals.calls)calls+=" "+hex(target)+"="+std::to_string(count);
    std::printf("external calls:%s\n",calls.c_str());
    for(const auto &[why,count]:totals.fault_kinds)std::printf("matching fault x%u: %s\n",count,why.c_str());
    if(totals.dead_stack_cases)
        std::printf("dead stack scratch differs in %u cases (first: %s)\n",totals.dead_stack_cases,totals.first_dead_stack.c_str());
    std::printf("PASS object loop frame: %u cases, %u matching faults\n",totals.cases,totals.faults);
    return 0;
}
