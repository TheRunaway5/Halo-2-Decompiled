// Native TU1.5 pose sample (212D40) against the unmodified translated body
// of the physics live support unit, extracted by
// tools/validate_native_frame_misc_tu15.py into body.inc, plus the
// verification reference the production binder generates (reference.inc).
// There are no children. Fixtures author records of every kind (and
// others), descriptions whose signed parts and frame indexes (negative too)
// land within the frames, and outputs over the frames; one region removed
// injects a fault. Modes: <cases> [census|verify]. Prints a JSON summary
// with branch coverage.
#include "native_tu15_frame_misc_harness.hpp"
#include "gameplay/units/native_tu15_pose_sample.hpp"
using namespace frame_misc_harness;
namespace sample=halo2::native_pose_sample_detail;
namespace original {
using namespace halo2::physics_world_runtime_detail;
std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){return p;}
#include "body.inc"
#include "reference.inc"
}
namespace {
constexpr std::uint32_t records=0x10000000u,descriptions=0x10010000u,frames=0x10100000u,outputs=0x10200000u,zero=0x10300000u;
Case author(std::mt19937 &rng,unsigned n) {
    Case c;auto &m=c.memory;
    const auto pick=[&](std::uint32_t k){return std::uint32_t(rng()%k);};
    const unsigned shape=n%16;
    const auto stack=0x20010000u+pick(64)*4u;
    m.put(0x474d04u,zero);
    for(std::uint32_t at=zero;at<zero+0x10u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=records;at<records+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    const auto record=records+4u*pick(8);
    m.put(record,frames+0x400u);
    const auto description=descriptions+4u*pick(8);
    m.put(record+4u,description);
    m.put(record+9u,pick(5)==0?pick(256):1u+pick(3),1);
    for(std::uint32_t at=descriptions;at<descriptions+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(description,(0x100u-4u*pick(3))&0xffu,1);
    m.put(description+1u,4u*pick(4),1);
    m.put(description+6u,pick(3)==0?0x10000u-0x40u:4u*pick(8),2);
    m.put(description+0xcu,4u*pick(16));
    for(std::uint32_t at=frames;at<frames+0x800u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=outputs;at<outputs+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    for(std::uint32_t at=stack-0x40u;at<stack+0x40u;at+=4)m.put(at,std::uint32_t(rng()));
    m.put(stack,0x1c0000u+pick(0x100));
    m.put(stack+4u,(std::uint32_t(rng())<<16)|(pick(4)==0?0x10000u-pick(4):pick(16)));
    random_abi(rng,c,stack);
    c.abi.gp[0]=outputs+4u*pick(4);c.abi.gp[1]=outputs+0x20u+4u*pick(4);c.abi.gp[2]=record;
    if(shape==7)c.abi.gp[1]=frames+0x400u+4u*pick(32);           // the vector over the frames
    if(shape==15) {
        static constexpr std::uint32_t regions[]{records,descriptions,frames,outputs,zero};
        m.drop(regions[pick(5)]);
    }
    return c;
}
bool native(WorldState &s){return sample::bridge(s,[](auto &v){original::native_frame_misc_reference_212d40(v);});}
void reference(WorldState &s){original::run_212d40(s);}
}
int main(int argc,char **argv)try {
    const unsigned cases=argc>1?unsigned(std::stoul(argv[1])):4000;
    const std::string mode=argc>2?argv[2]:"";
    set_mode(mode,"HALO2_VERIFY_POSE_SAMPLE");
    std::mt19937 rng(0x212d40u+(census_mode?1u:0u)+(mode=="verify"?2u:0u));
    Totals t;
    for(unsigned n=0;n<cases;++n) {
        const auto c=author(rng,n);
        if(!compare(n,c,native,reference,t))return 1;
    }
    if(!census_mode)check_refusals(author(rng,1),native);
    report(cases,t,sample::coverage().load(),unsigned(sample::branch_count),mode);
    return 0;
}catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
