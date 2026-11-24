// Native TU1.5 tiny leaves against the unmodified translated bodies of the
// physics live support unit (the validator repeats 33210C and 116480 with
// the AI frame unit's and 161530 with the sandbox unit's), extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// Shared by one test per owner, which defines OWNER first: 31EDF0 six,
// 33210C double exponent, 161530 variant option, 1D4140 definition word,
// 116480 block ready, E32C0 tick seconds. There are no children. Fixtures
// author every path (doubles of every exponent, sessions with and without
// a variant record, records whose tags have definitions or none, blocks
// with every gate and state bit, objects with zero, positive and negative
// tick counts); one region removed injects a fault. Modes: <cases>
// [census|verify]. Prints a JSON summary with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#if OWNER==0x31edf0
#include "physics/runtime/native_tu15_six.hpp"
namespace leaf=halo2::native_six_detail;
#elif OWNER==0x33210c
#include "physics/runtime/native_tu15_double_exponent.hpp"
namespace leaf=halo2::native_double_exponent_detail;
#elif OWNER==0x161530
#include "physics/runtime/native_tu15_variant_option.hpp"
namespace leaf=halo2::native_variant_option_detail;
#elif OWNER==0x1d4140
#include "physics/runtime/native_tu15_definition_word.hpp"
namespace leaf=halo2::native_definition_word_detail;
#elif OWNER==0x116480
#include "physics/runtime/native_tu15_block_ready.hpp"
namespace leaf=halo2::native_block_ready_detail;
#else
#include "physics/runtime/native_tu15_tick_seconds.hpp"
namespace leaf=halo2::native_tick_seconds_detail;
#endif
using namespace frame_misc_harness;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t headers=0x10000000u,header_data=0x10010000u,objects=0x10100000u,tags=0x10180000u,
    definitions=0x10200000u,session=0x10300000u,state=0x10310000u,time_record=0x10320000u,outputs=0x10330000u,
    records=0x10340000u;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x4ecbd0u,headers);m.put(0x4f0414u,tags);m.put(0x4f63b8u,session);m.put(0x4f3218u,state);m.put(0x51df54u,time_record);
    m.put(0x44a8a0u,std::bit_cast<std::uint32_t>(float(int(pick(2001))-1000)/37.0f));
    for(std::uint32_t at=headers;at<headers+0x50u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(headers+0x44u,header_data);
    for(std::uint32_t k=0;k<4;++k) {
        const auto object=objects+0x1000u*k+0x800u+4u*k;
        m.put(header_data+12u*k,std::uint32_t(rng()));m.put(header_data+12u*k+4u,std::uint32_t(rng()));
        m.put(header_data+12u*k+8u,object);
        for(std::uint32_t at=object-0x800u;at<object+0x800u;at+=4)m.put(at,std::uint32_t(rng()));
        const auto offset=pick(3)==0?0x10000u-0x400u:0x200u;
        m.put(object+0x33eu,offset,2);
        const auto block=object+std::uint32_t(std::int32_t(std::int16_t(offset)));
        m.put(block+0x89u,pick(3)==0?0u:pick(256),1);
        m.put(object+0x3aau,pick(3)==0?0u:pick(256),1);
        const auto tag_index=0x1000u*pick(2)+k;
        m.put(tags+16u*tag_index+8u,pick(4)==0?0u:definitions+0x100u*k+2u*pick(4));
    }
    for(std::uint32_t at=definitions;at<definitions+0x400u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=records;at<records+0x100u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t k=0;k<4;++k)m.put(records+0x68u+4u*k,(std::uint32_t(rng())<<16)|(0x1000u*pick(2)+pick(4)));
    for(std::uint32_t at=session+0xc00u;at<session+0xc20u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(session+0xc14u,pick(4));
    for(std::uint32_t k=0;k<4;++k)m.put(0x56dfa0u+4u*k,pick(4)==0?0u:std::uint32_t(rng())|1u);
    for(std::uint32_t at=state+0x180u;at<state+0x190u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=time_record;at<time_record+0x10u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(time_record+4u,std::bit_cast<std::uint32_t>(1.0f/float(1+pick(60))));
    for(std::uint32_t at=outputs;at<outputs+0x20u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x110000u+pick(0x100));
    random_abi(rng,c,stack);
    // The double's high word and the out pointer share the stack.
    m.put(stack+8u,pick(3)==0?(0x7ff00000u|std::uint32_t(rng())):std::uint32_t(rng()));
    m.put(stack+4u,outputs+4u*pick(8));
    c.abi.gp[0]=(std::uint32_t(rng())<<16)|pick(4);
    c.abi.gp[1]=records+4u*pick(4);
    if(shape==15) {
        static constexpr std::uint32_t regions[]{headers,header_data,objects+0x1000u,tags,definitions,session,state,time_record,
            outputs,records};
        m.drop(regions[pick(10)]);
    }
    return c;
}
#if OWNER==0x31edf0
bool native(WorldState &s){return leaf::bridge(s,[](auto &v){original::native_frame_misc_reference_31edf0(v);});}
void reference(WorldState &s){original::run_31edf0(s);}
#elif OWNER==0x33210c
bool native(WorldState &s){return leaf::bridge(s,[](auto &v){original::native_frame_misc_reference_33210c(v);});}
void reference(WorldState &s){original::run_33210c(s);}
#elif OWNER==0x161530
bool native(WorldState &s){return leaf::bridge(s,[](auto &v){original::native_frame_misc_reference_161530(v);});}
void reference(WorldState &s){original::run_161530(s);}
#elif OWNER==0x1d4140
bool native(WorldState &s){return leaf::bridge(s,[](auto &v){original::native_frame_misc_reference_1d4140(v);});}
void reference(WorldState &s){original::run_1d4140(s);}
#elif OWNER==0x116480
bool native(WorldState &s){return leaf::bridge(s,[](auto &v){original::native_frame_misc_reference_116480(v);});}
void reference(WorldState &s){original::run_116480(s);}
#else
bool native(WorldState &s){return leaf::bridge(s,[](auto &v){original::native_frame_misc_reference_e32c0(v);});}
void reference(WorldState &s){original::run_e32c0(s);}
#endif
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_TINY_LEAVES");
    std::mt19937 rng(OWNER+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,leaf::coverage().load(),unsigned(leaf::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
