// Differential test: the native object looping-sound update routines (TU1.5
// 18E970, 18DC70, 190A80, 129170, 18D960) against the lifted originals, over
// randomized structurally valid game states: listener records and loop slots
// ([4F2C50]), the sound-structure tables ([4ECC18]: sources, clusters,
// records, rows, definitions), the loop pool ([4F9B5C]), per-player loop
// slots ([4F9B58]) and tags, across x87 precision/rounding controls, MXCSR
// modes and special floats. Routines outside the group (14DD70, 32CCD8,
// 190520, 140910, 220200, 16E6A0, 18CDD0, 2531E0) run through
// Services::required_owner as deterministic stubs that log the full register,
// x87 and stack-argument state of every call. Both sides must agree on every
// register, XMM lane, x87 and FP control state, every external call and every
// memory byte. The lifted run reports instruction coverage per routine.
// Options: HALO2_LOOP_TEST_ONLY=18dc70,129170 runs those routines only;
// HALO2_STRICT_DEAD_STACK=1 also fails on differing dead callee scratch
// (below the post-return ESP; reported as a count otherwise);
// HALO2_LOOP_TEST_LIFTED_BOTH=1 runs the lifted routine on both sides to
// validate the harness itself.
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
constexpr std::uint32_t none=0xffffffffu;

// Test-owned game data.
constexpr std::uint32_t listeners=0x01000000u;   // [4F2C50]: +0x88 listener records (0x48), +0x1A8 loop slots (0x1C), +0x1E0 rate
constexpr std::uint32_t structure=0x01100000u;   // [4ECC18]: the sound structure tables
constexpr std::uint32_t sources=0x01200000u;     // structure +0x60: 36-byte sources
constexpr std::uint32_t records=0x01300000u;     // structure +0xA0: 0xB0-byte cluster records
constexpr std::uint32_t rows=0x01400000u;        // structure +0xD8: 0x64-byte rows
constexpr std::uint32_t definitions=0x01500000u; // structure +0xE0: 0x48-byte definitions
constexpr std::uint32_t clusters=0x01600000u;    // structure +0x154: 20-byte source clusters
constexpr std::uint32_t cluster_words=0x01610000u;
constexpr std::uint32_t scale_table=0x01700000u; // structure +0x228 (253070)
constexpr std::uint32_t scale_bytes=0x01710000u;
constexpr std::uint32_t cluster_bytes=0x01720000u; // structure +0xE8 (2531E0)
constexpr std::uint32_t players=0x01800000u;     // [4F54F0]: +0xC local player entries
constexpr std::uint32_t player_slots=0x01810000u;// [4F9B58]: +0 loop handle, +4 tag datum (stride 8), +0x240 byte
constexpr std::uint32_t pool=0x01820000u;        // [4F9B5C]: loop pool header
constexpr std::uint32_t pool_elements=0x01830000u;
constexpr std::uint32_t pool_bitmap=0x01840000u;
constexpr std::uint32_t flag_block=0x01850000u;  // [51DF50]
constexpr std::uint32_t tags=0x01900000u;        // [4F0414]
constexpr std::uint32_t tag_data=0x01a00000u;
constexpr std::uint32_t heap=0x01e00000u;        // routine arguments
// Stub plan: what the external stubs return.
constexpr std::uint32_t plan=0x01f00000u;
constexpr std::uint32_t plan_alloc_calls=plan+0x00u,plan_alloc_fail=plan+0x04u,plan_start_calls=plan+0x08u,
    plan_starts=plan+0x10u,plan_override=plan+0x40u,plan_override_value=plan+0x44u,plan_distances=plan+0x100u,
    plan_found=plan+0x200u,plan_rows=plan+0x210u,plan_gains=plan+0x250u,plan_sources=plan+0x290u,
    plan_clusters=plan+0x2d0u;

// Retail .rdata words the lifted routines read (TU1.5 XBE).
constexpr std::pair<std::uint32_t,std::uint32_t> retail[]{
    {0x44923cu,0x40c90fdbu},{0x455b0cu,0xc2800000u},{0x455b14u,0x40135d8eu},{0x45695cu,0x7f7fffffu},
    {0x46a0ccu,0x3f000000u},{0x46a0d0u,0x3f800000u},{0x46a0dcu,0xbf800000u},{0x46a0e4u,0x40400000u},
    {0x46a0e8u,0x00000000u},{0x46a0ecu,0x38d1b717u},{0x46a118u,0x41a00000u},{0x46a11cu,0x40000000u},
    {0x46a12cu,0x3d4ccccdu},{0x46a148u,0x437f0000u},{0x46a180u,0x3a83126fu},{0x46a26cu,0x43800000u},
    {0x46a2d4u,0x40010204u},{0x46a2d8u,0x3b81848eu},{0x46a2ecu,0x40490fdbu},{0x46a580u,0xc0c90fdbu}};

std::uint32_t rd32(const M &m,std::uint32_t p){std::array<std::byte,4> b;m.read(p,b);return std::bit_cast<std::uint32_t>(b);}
std::uint32_t rd16(const M &m,std::uint32_t p){std::array<std::byte,2> b;m.read(p,b);return std::bit_cast<std::uint16_t>(b);}
std::uint32_t rd8(const M &m,std::uint32_t p){std::array<std::byte,1> b;m.read(p,b);return std::to_integer<std::uint32_t>(b[0]);}
void wr32(M &m,std::uint32_t p,std::uint32_t v){const auto b=std::bit_cast<std::array<std::byte,4>>(v);m.write(p,b);}
void wr16(M &m,std::uint32_t p,std::uint32_t v){const auto b=std::bit_cast<std::array<std::byte,2>>(std::uint16_t(v));m.write(p,b);}
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
// Bytes of return address plus stack arguments each external takes.
unsigned argument_bytes(std::uint32_t target) {
    switch(target) {
    case 0x190520u:return 4u+0x24u;
    case 0x140910u:return 4u+0xcu;
    case 0x18cdd0u:return 4u+4u;
    default:return 4u;
    }
}
struct SplitMix {
    std::uint64_t x;
    std::uint32_t next(){x+=0x9e3779b97f4a7c15ull;auto z=x;z=(z^(z>>30))*0xbf58476d1ce4e5b9ull;
        z=(z^(z>>27))*0x94d049bb133111ebull;return std::uint32_t((z^(z>>31))>>7);}
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

// Deterministic owners for the routines outside the group.
struct Stubs {
    std::vector<CallRecord> log;FP fp{};
    void operator()(M &m,const Call &call,Abi &a,const O &o) {
        CallRecord x{call.target,call.instruction,a.gp,a.xmm,a.x87_depth,a.x87_status};
        for(unsigned i=0;i<a.x87_depth && i<8;++i)std::memcpy(x.lanes[i].data(),&a.x87[i],10);
        if(a.floating_point){x.fp_present=true;x.cw=a.floating_point->x87_control_word;x.mxcsr=a.floating_point->mxcsr;}
        const auto esp=a.gp[4];
        x.stack.resize(argument_bytes(call.target));
        for(unsigned i=0;i<x.stack.size();++i)x.stack[i]=std::uint8_t(rd8(m,esp+i));
        SplitMix h{digest(x)};
        const auto clobber_gp=[&]{a.gp[0]=h.next();a.gp[1]=h.next();a.gp[2]=h.next();};
        const auto clobber_xmm=[&](unsigned first){for(unsigned n=first;n<7;++n)for(auto &l:a.xmm[n])l=h.next();};
        const auto ret=[&](unsigned arguments){a.gp[4]+=4u+arguments;};
        if(call.target!=0x2531e0u)log.push_back(x);
        switch(call.target) {
        case 0x14dd70u: {   // distance from the source (ECX) to a point (EDX): XMM0
            const auto offset=a.gp[1]-sources;
            const auto distance=offset%36u==0u && offset/36u<64u?rd32(m,plan_distances+offset/36u*4u):
                0x40000000u|(h.next()&0x3fffffu);
            clobber_gp();clobber_xmm(1);
            a.xmm[0]={distance,h.next(),h.next(),h.next()};
            ret(0);break;
        }
        case 0x32ccd8u: {   // CRT arc cosine of ST(0)
            if(!a.x87_depth)throw std::runtime_error("32CCD8 stub without ST(0)");
            a.x87[0]=rd32(m,plan_override)?static_cast<long double>(std::bit_cast<float>(rd32(m,plan_override_value))):
                std::acos(a.x87[0]);
            clobber_gp();ret(0);break;
        }
        case 0x190520u: {   // the cluster's nearby rows: count and four parallel arrays
            const auto arg=[&](unsigned k){return rd32(m,esp+4u+4u*k);};
            const auto found=rd32(m,plan_found);
            for(std::uint32_t k=0;k<found;++k) {
                wr32(m,arg(4)+4u*k,rd32(m,plan_rows+4u*k));wr32(m,arg(5)+4u*k,rd32(m,plan_gains+4u*k));
                wr32(m,arg(6)+4u*k,rd32(m,plan_sources+4u*k));wr32(m,arg(7)+4u*k,rd32(m,plan_clusters+4u*k));
            }
            if(arg(8))wr32(m,arg(8),found);
            clobber_gp();clobber_xmm(0);ret(0x24u);break;
        }
        case 0x140910u: {   // sort the indices (ECX, EAX count) by their distances (third argument)
            const auto count=std::int32_t(a.gp[0]);const auto at=a.gp[1],distances=rd32(m,esp+0xcu);
            if(count>64)throw std::runtime_error("140910 stub count");
            std::vector<std::uint32_t> items;
            for(std::int32_t k=0;k<count;++k)items.push_back(rd32(m,at+4u*std::uint32_t(k)));
            const auto key=[&](std::uint32_t index) {
                const auto v=rd32(m,distances+4u*(index&63u));return v&0x80000000u?~v:(v|0x80000000u);
            };
            std::stable_sort(items.begin(),items.end(),[&](auto x,auto y){return key(x)<key(y);});
            for(std::size_t k=0;k<items.size();++k)wr32(m,at+4u*std::uint32_t(k),items[k]);
            clobber_gp();ret(0xcu);break;
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
            a.gp[0]=handle;a.gp[1]=h.next();a.gp[2]=h.next();ret(0);break;
        }
        case 0x18cdd0u: {   // start a sound (EAX request, stack tag)
            const auto calls=rd32(m,plan_start_calls);wr32(m,plan_start_calls,calls+1u);
            if(calls<8u){wr32(m,plan_starts+calls*8u,rd32(m,esp+4u));wr32(m,plan_starts+calls*8u+4u,rd32(m,a.gp[0]));}
            clobber_gp();clobber_xmm(0);ret(4u);break;
        }
        case 0x2531e0u: {   // the structure's (EDI) cluster byte for AX, CX
            const auto first=a.gp[0],second=a.gp[1];
            if(std::uint16_t(first)==std::uint16_t(second)){a.gp[0]&=~0xffu;ret(0);break;}
            const bool swap=std::int16_t(first)>std::int16_t(second);
            const auto low=sign16(swap?second:first),high=sign16(swap?first:second);
            a.gp[1]=low;
            auto triangle=(low+1u)*low;
            triangle-=std::int32_t(triangle)<0?none:0u;
            triangle=std::uint32_t(std::int32_t(triangle)>>1);
            const auto row=(rd32(m,a.gp[7]+0x9cu)-1u)*low-triangle+rd32(m,a.gp[7]+0xe8u);
            a.gp[2]=row;a.gp[0]=(high&~0xffu)|rd8(m,row+high-1u);
            ret(0);break;
        }
        default:
            char t[64];std::snprintf(t,sizeof t,"unplanned external %06x from %06x",call.target,call.instruction);
            throw std::runtime_error(t);
        }
    }
};

float uniform(Random &r,float lo,float hi){return lo+(hi-lo)*float(r.word()>>8)/16777216.f;}
// Mostly the given value, sometimes a special float.
float usual(Random &r,float x,unsigned odds=24){return r.below(odds)?x:r.scalar();}
std::uint32_t pick_id(Random &r) {
    constexpr std::uint32_t ids[]{0x100u,0x101u,0x102u,0x103u,none};
    return ids[r.below(5)];
}
float fade(Random &r) {
    switch(r.below(9)) {
    case 0:return 0.f;
    case 1:return 0.0005f;
    case 2:return std::bit_cast<float>(0x3a83126fu);   // exactly 0.001
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

struct World {
    std::uint32_t records,sources,rows,definitions,clusters,tags,capacity;
    std::uint32_t datum(Random &r,std::uint32_t tag)const{return ((0xe000u|r.below(0x1fffu))<<16)|tag;}
};
// The shared game state every routine walks.
World world(Case &c,Random &r) {
    auto &m=c.memory;
    World w{1u+r.below(8),1u+r.below(16),1u+r.below(10),1u+r.below(r.below(8)?10u:64u),1u+r.below(5),6u,1u+r.below(20)};
    for(const auto &[p,v]:retail)m.put32(p,v);
    // Callee scratch below the caller's frame holds old garbage.
    for(std::uint32_t p=stack-0x2000u;p<stack;++p)m.put8(p,std::uint8_t(r.word()));
    m.put32(0x4f2c50u,listeners);m.put32(0x4ecc18u,structure);m.put32(0x4f54f0u,players);
    m.put32(0x4f9b58u,player_slots);m.put32(0x4f9b5cu,pool);m.put32(0x4f0414u,tags);
    m.put32(0x515f00u,r.below(2));m.put32(0x51df50u,r.below(3)?flag_block:0u);m.put8(flag_block+5u,r.below(2));
    m.putf(0x47365cu,r.below(3)?1.f:usual(r,uniform(r,0.f,2.f),6));m.put8(0x474c24u,std::uint8_t(r.word()));
    // Listener records and the two loop slots.
    m.map(listeners,0x200u);
    for(std::uint32_t p=listeners;p<listeners+0x200u;p+=4)m.put32(p,r.word());
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
    m.map(scale_bytes,0x1000u);m.map(cluster_bytes,0x1000u);
    for(std::uint32_t k=0;k<0x1000u;++k) {
        const auto pick=r.below(5);
        m.put8(scale_bytes+k,std::uint8_t(pick==0?0u:pick==1?0xffu:r.word()));
        m.put8(cluster_bytes+k,std::uint8_t(r.word()));
    }
    // Local players, per-player loop slots and the loop pool.
    m.map(players,0x20u);
    for(std::uint32_t i=0;i<4;++i)m.put32(players+0xcu+i*4u,r.below(3)?i:none);
    m.map(pool,0x50u);
    m.put32(pool+0x24u,0x18u);m.put32(pool+0x38u,w.capacity);m.put32(pool+0x44u,pool_elements);m.put32(pool+0x48u,pool_bitmap);
    m.map(pool_elements,w.capacity*0x18u);m.map(pool_bitmap,8u);
    m.put32(pool_bitmap,r.below(5)?r.word():(r.below(2)?0u:none));m.put32(pool_bitmap+4u,r.word());
    std::vector<std::uint32_t> handles;
    for(std::uint32_t i=0;i<w.capacity;++i) {
        const auto at=pool_elements+i*0x18u;
        const auto salt=0x8000u|r.below(0x8000u);
        m.put16(at,std::uint16_t(salt));m.put8(at+2u,std::uint8_t(r.word()));
        m.put8(at+3u,std::uint8_t(r.below(5)?0u:1u+r.below(255)));
        m.put16(at+4u,std::uint16_t(r.word()));m.put16(at+6u,std::uint16_t(r.word()));
        m.putf(at+8u,usual(r,uniform(r,0.f,1.5f)));m.put32(at+0xcu,pick_id(r));
        m.putf(at+0x10u,usual(r,r.below(5)?uniform(r,-70.f,5.f):-64.f));
        m.put8(at+0x14u,std::uint8_t(r.below(5)?r.below(w.rows):0xffu));
        m.put8(at+0x15u,std::uint8_t(r.word()));m.put16(at+0x16u,std::uint16_t(r.word()));
        handles.push_back((salt<<16)|i);
    }
    m.map(tags,w.tags*16u);
    for(std::uint32_t t=0;t<w.tags;++t) {
        const auto at=tag_data+t*0x100u;
        m.map(at,0x100u);m.put32(tags+t*16u+8u,at);
        m.put32(at+0x3cu,pick_id(r));
        m.putf(at+0x48u,usual(r,uniform(r,-0.5f,1.5f)));m.putf(at+0x4cu,usual(r,uniform(r,-0.5f,1.5f)));
        m.put32(at+0x54u,r.below(3)?none:r.word());m.put32(at+0x5cu,r.below(3)?none:r.word());
    }
    m.map(player_slots,0x300u);
    for(std::uint32_t p=player_slots;p<player_slots+0x300u;p+=4)m.put32(p,r.word());
    for(std::uint32_t i=0;i<4;++i) {
        m.put32(player_slots+i*8u,r.below(3)?handles[r.below(w.capacity)]:none);
        m.put32(player_slots+i*8u+4u,r.below(3)?none:w.datum(r,r.below(w.tags)));
    }
    m.put8(player_slots+0x240u,std::uint8_t(r.below(2)));
    // What the external stubs return.
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
// A point near the sound sources, or a listener's position.
std::uint32_t point(Case &c,Random &r) {
    if(r.below(2))return listeners+0x88u+r.below(4)*0x48u+0x30u;
    for(std::uint32_t k=0;k<3;++k)c.memory.putf(heap+0x40u+k*4u,usual(r,uniform(r,-40.f,40.f)));
    return heap+0x40u;
}
float frame_time(Random &r) {
    switch(r.below(8)) {
    case 0:return r.scalar();
    case 1:return 0.f;
    default:return r.below(2)?1.f/30.f:uniform(r,0.001f,0.2f);
    }
}

Case scenario(Random &r,std::uint32_t entry) {
    auto c=base_case(r);
    const auto w=world(c,r);
    auto &m=c.memory;auto &a=c.abi;
    const auto arg=[&](unsigned k,std::uint32_t v){m.put32(stack+4u+4u*k,v);};
    switch(entry) {
    case 0x129170u:   // EAX: this frame's two loop requests (0x1C each)
        for(std::uint32_t k=0;k<2;++k) {
            const auto at=heap+k*0x1cu;
            m.putf(at,r.below(4)?uniform(r,0.0001f,3.f):fade(r));m.putf(at+4u,1.f);m.put32(at+8u,pick_id(r));
            for(std::uint32_t j=0;j<4;++j)m.putf(at+0xcu+j*4u,usual(r,uniform(r,-10.f,10.f)));
        }
        a.gp[0]=heap;break;
    case 0x18d960u: { // EAX structure, ECX source or -1, ESI listener, BL in-cluster; point, limit, output
        a.gp[0]=structure;a.gp[1]=r.below(5)?r.below(w.sources):none;
        a.gp[6]=listeners+0x88u+r.below(4)*0x48u;
        a.gp[3]=(a.gp[3]&~0xffu)|(r.below(2)?0u:1u+r.below(255));
        const auto limit=r.below(5)?distance_limit(r):
            std::bit_cast<float>(m.get32(plan_distances+(a.gp[1]&63u)*4u));   // exactly the distance
        arg(0,point(c,r));arg(1,bits(limit));arg(2,heap+0x80u);
        break;
    }
    case 0x190a80u: { // nearest definitions around a cluster record
        arg(0,structure);arg(1,r.below(w.records));arg(2,point(c,r));
        arg(3,r.below(4)?0x7f7fffffu:bits(distance_limit(r)));
        arg(4,heap+0x100u);arg(5,r.below(2)?0u:heap+0x140u);
        arg(6,r.below(6)?heap+0x180u:0u);arg(7,r.below(6)?heap+0x1c0u:0u);arg(8,r.below(4)?heap+0x200u:0u);
        if(!r.below(40))m.put32(structure+0xe0u,0u);   // no definitions table
        break;
    }
    case 0x18dc70u: { // the loops around one listener
        const auto cluster=r.below(6)?r.below(w.records):none;
        arg(0,structure);arg(1,r.below(4));arg(2,point(c,r));arg(3,cluster);
        arg(4,r.below(4)?w.datum(r,r.below(w.tags)):none);arg(5,bits(frame_time(r)));
        if(r.below(2))m.put32(plan_clusters,cluster);
        break;
    }
    case 0x18e970u: { // every local player's object loops
        arg(0,bits(frame_time(r)));
        for(std::uint32_t i=0;i<4;++i)if(m.get32(players+0xcu+i*4u)!=none) {
            if(r.below(2))m.put32(plan_clusters,sign16(m.get16(listeners+0x88u+i*0x48u+4u)));
            break;
        }
        break;
    }
    }
    return c;
}

// Instruction coverage of the lifted routines.
constexpr std::uint32_t routines[]{0x129170u,0x18d960u,0x190a80u,0x18dc70u,0x18e970u};
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

// Registers a routine's callers never read (with the caller evidence); none
// are excluded unless documented here.
std::uint8_t skipped_gp(std::uint32_t){return 0u;}

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
    if(n.size()!=l.size())return "external call count native="+std::to_string(n.size())+" lifted="+std::to_string(l.size());
    for(std::size_t k=0;k<n.size();++k) {
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
    return {};
}

// One case: the native routine (with its unconverted callees lifted) against
// the lifted routine, from identical copies of the state.
struct Options {
    bool strict_dead_stack=false;   // HALO2_STRICT_DEAD_STACK=1: dead scratch differences fail
    bool lifted_both=false;         // HALO2_LOOP_TEST_LIFTED_BOTH=1: harness self-check (lifted vs lifted)
    std::set<std::uint32_t> only;   // HALO2_LOOP_TEST_ONLY=18dc70,129170: these routines only
};
bool agree_loop(const Case &c,std::uint32_t entry,unsigned n,Totals &totals,const Options &options) {
    auto native=c,lifted=c;native.memory=c.memory.clone();lifted.memory=c.memory.clone();
    Stubs native_stubs{{},c.fp},lifted_stubs{{},c.fp};
    Services native_services,lifted_services;
    native_services.required_owner=[&](M &m,const Call &call,Abi &a,const O &o){native_stubs(m,call,a,o);};
    lifted_services.required_owner=[&](M &m,const Call &call,Abi &a,const O &o){lifted_stubs(m,call,a,o);};
    lifted_services.instruction=[&](std::uint32_t pc){totals.executed.insert(pc);};
    std::string native_fault,lifted_fault;
    try {
        if(options.lifted_both)source_run_object_loop_audio_tu15(native.memory,entry,native.abi,native.fp,native_services,{},nullptr);
        else native_tu15_object_loop_run(native.memory,entry,native.abi,native.fp,native_services,nullptr);
    }
    catch(const std::exception &e){native_fault=e.what();if(native_fault.empty())native_fault="(empty)";}
    try{source_run_object_loop_audio_tu15(lifted.memory,entry,lifted.abi,lifted.fp,lifted_services,{},nullptr);}
    catch(const std::exception &e){lifted_fault=e.what();if(lifted_fault.empty())lifted_fault="(empty)";}
    const auto fail=[&](const std::string &why) {
        std::cerr<<hex(entry)<<" case "<<n<<": "<<why<<'\n';return false;
    };
    if(native_fault!=lifted_fault)
        return fail("fault native \""+native_fault+"\" lifted \""+lifted_fault+"\"");
    for(const auto &x:lifted_stubs.log)++totals.calls[x.target];
    if(!lifted_fault.empty()) {
        ++totals.faults;
        ++totals.fault_kinds[std::regex_replace(lifted_fault,std::regex("[0-9a-f]{8}"),"*")];
        return true;
    }
    const auto &x=native.abi,&y=lifted.abi;
    const auto skip=skipped_gp(entry);
    for(unsigned i=0;i<8;++i)if(!(skip&(1u<<i)) && x.gp[i]!=y.gp[i])
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
            if(p>=stack-0x2000u && p<dead_end) {
                if(options.strict_dead_stack)return fail("dead stack "+why);
                if(!dead_differs && totals.first_dead_stack.empty())
                    totals.first_dead_stack=hex(entry)+" case "+std::to_string(n)+": "+why;
                dead_differs=true;continue;
            }
            return fail(why);
        }
    }
    if(dead_differs)++totals.dead_stack_cases;
    return true;
}
} // namespace

int main() {
    const auto pcs=routine_pcs();
    const auto set=[](const char *name){const auto *v=std::getenv(name);return v && *v && *v!='0';};
    Options options{set("HALO2_STRICT_DEAD_STACK"),set("HALO2_LOOP_TEST_LIFTED_BOTH")};
    if(const auto *only=std::getenv("HALO2_LOOP_TEST_ONLY")) {
        std::string list=only;
        for(std::size_t at=0;at<list.size();) {
            auto end=list.find(',',at);if(end==std::string::npos)end=list.size();
            if(end>at)options.only.insert(std::uint32_t(std::stoul(list.substr(at,end-at),nullptr,16)));
            at=end+1;
        }
    }
    Totals totals;
    constexpr std::pair<std::uint32_t,unsigned> plan_cases[]{
        {0x129170u,4000u},{0x18d960u,4000u},{0x190a80u,4000u},{0x18dc70u,4000u},{0x18e970u,4000u}};
    for(const auto &[entry,count]:plan_cases) {
        if(!options.only.empty() && !options.only.contains(entry))continue;
        Random r(0x0b1ec700u^entry);
        for(unsigned n=0;n<count;++n) {
            const auto c=scenario(r,entry);
            if(!agree_loop(c,entry,n,totals,options))return 1;
            ++totals.cases;
        }
    }
    for(const auto entry:routines) {
        if(!options.only.empty() && !options.only.contains(entry))continue;
        const auto found=pcs.find(entry);
        if(found==pcs.end()){std::cerr<<"no lifted body for "<<hex(entry)<<'\n';return 1;}
        const auto &all=found->second;
        std::vector<std::uint32_t> missed;
        for(const auto pc:all)if(!totals.executed.contains(pc))missed.push_back(pc);
        std::string ranges;
        for(std::size_t i=0;i<missed.size();) {
            // Consecutive instructions of the routine form one range.
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
    std::printf("PASS object loop update: %u cases, %u matching faults\n",totals.cases,totals.faults);
    return 0;
}
