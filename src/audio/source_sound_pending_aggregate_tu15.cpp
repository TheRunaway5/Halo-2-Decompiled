#include "halo2/source_sound_pending_aggregate_tu15.hpp"
#include "halo2/source_sound_frontend_dpc_tu15.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"

namespace halo2 {
void source_construct_sound_pcm_metadata_header_tu15(SourcePhysicsContactMemory& memory,
    SourcePhysicsWorldRuntimeAbi& abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observe) {
    using namespace physics_world_runtime_detail;FpScope scope(fp);
    WorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(abi);
    try {
        if(services.instruction)services.instruction(0x387073);w(memory,observe,0x387073,s.gp[0]+4,4,1);
        if(services.instruction)services.instruction(0x38707a);w(memory,observe,0x38707a,s.gp[0],4,0x42122c);
        if(services.instruction)services.instruction(0x387080);s.gp[1]=s.gp[0];
    }catch(...){abi=s.save();throw;}abi=s.save();
}
void source_run_sound_pending_aggregate_tu15(SourceRuntimeMemory& memory,
    SourcePhysicsWorldRuntimeAbi& abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observe) {
    using namespace physics_world_runtime_detail;
    FpScope scope(fp);WorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};s.load(abi);
    auto at=[&](std::uint32_t pc){if(services.instruction)services.instruction(pc);};
    try {
        at(0x3889f1);s.push(0x3889f1,s.gp[5]);
        at(0x3889f2);s.gp[5]=s.gp[4];
        at(0x3889f4);s.push(0x3889f4,s.gp[6]);
        at(0x3889f5);s.gp[6]=r(memory,s.gp[5]+8);
        at(0x3889f8);s.gp[0]=s.gp[6]+0x4b8;
        at(0x3889fe);w(memory,observe,0x3889fe,s.gp[5]+8,4,s.gp[0]);
        at(0x388a01);s.gp[2]=0;
        at(0x388a06);s.gp[1]=r(memory,s.gp[5]+8);
        at(0x388a09);s.gp[0]=r(memory,s.gp[1]);
        do {
            at(0x388a0b);
            auto bytes=memory.mutable_view(s.gp[1],4);
            const auto wide=reinterpret_cast<std::uintptr_t>(bytes.data());
            if((wide&3u)||(s.gp[1]&3u))throw std::runtime_error("Original sound aggregate requires aligned actual mutable native backing");
            const auto expected=s.gp[0];auto observed=expected;
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
            // Original CMPXCHG is under interrupt synchronization. Native
            // producers additionally get a real atomic operation, with no
            // source token cast to a host pointer and no copied status word.
            asm volatile("lock cmpxchgl %2,(%1)":"+a"(observed):"r"(bytes.data()),"r"(s.gp[2]):"cc","memory");
#else
            throw std::runtime_error("Native sound pending atomic backend unavailable");
#endif
            s.flags.minus(expected,observed,4);s.gp[0]=observed;
            SourcePhysicsContactRecordStore row{0x388a0b,s.gp[1],std::vector<std::byte>(4),std::vector<std::byte>(4)};
            const auto after=s.flags.zf?s.gp[2]:observed;
            for(unsigned i=0;i<4;++i){row.before[i]=std::byte(observed>>(8*i));row.written[i]=std::byte(after>>(8*i));}
            if(observe)observe(row);
            at(0x388a0e);
        }while(!s.flags.zf);
        at(0x388a10);w(memory,observe,0x388a10,s.gp[6]+0x4c0,4,s.flags.logic(r(memory,s.gp[6]+0x4c0)|s.gp[0],4));
        at(0x388a16);s.set(0,1,1);
        at(0x388a18);s.gp[6]=s.pop();
        at(0x388a19);s.gp[5]=s.pop();
        at(0x388a1a);s.pop();s.gp[4]+=4;
    }catch(...){abi=s.save();throw;}
    abi=s.save();
}
}
