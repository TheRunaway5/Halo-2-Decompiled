#include "halo2/source_process_heap_platform.hpp"
#include <algorithm>
#include <iostream>

namespace {
using M=halo2::SourceRuntimeMemory;
using A=halo2::SourcePhysicsContactRuntimeAbi;
using C=halo2::SourcePhysicsContactRuntimeCall;
void check(bool ok,const char *message) { if(!ok) throw std::runtime_error(message); }
template<class F> void rejects(F f) {
    bool failed=false;
    try { f(); } catch(const std::exception &) { failed=true; }
    check(failed,"Invalid retained resource/initializer frame was accepted");
}
void put(M &m,std::uint32_t p,std::uint32_t v) {
    std::array<std::byte,4> b;
    for(unsigned i=0;i<4;++i) b[i]=std::byte(v>>(i*8));
    m.write(p,b);
}
std::uint32_t word(const M &m,std::uint32_t p) {
    std::uint32_t v=0; const auto b=m.view(p,4);
    for(unsigned i=0;i<4;++i) v|=std::to_integer<std::uint32_t>(b[i])<<(i*8);
    return v;
}
void same_abi(const A &a,const A &b) {
    check(a.gp==b.gp && a.xmm==b.xmm && a.eflags==b.eflags && a.fs_base==b.fs_base &&
        a.x87==b.x87 && a.x87_depth==b.x87_depth && a.x87_status==b.x87_status &&
        a.floating_point.has_value()==b.floating_point.has_value() &&
        a.active_mmx.has_value()==b.active_mmx.has_value(),"Native platform ABI state changed");
    if(a.active_mmx)check(a.active_mmx->physical==b.active_mmx->physical &&
        a.active_mmx->known_mask==b.active_mmx->known_mask,"Native platform MMX state changed");
    if(a.floating_point) check(a.floating_point->x87_control_word==b.floating_point->x87_control_word &&
        a.floating_point->mxcsr==b.floating_point->mxcsr,"Native platform FP controls changed");
}
A frame() {
    A a{};
    a.gp={0x12345678,0x87654321,0x11223344,0x55667788,0x2000100,0x2000800,0x76543210,0x44332211};
    for(unsigned i=0;i<8;++i) for(unsigned j=0;j<4;++j) a.xmm[i][j]=0x10000000+i*4+j;
    a.eflags=0xcd7; a.fs_base=0x7000000; a.x87_depth=3; a.x87_status=0x2800;
    a.x87[0]=1.25L; a.x87[1]=-4.5L; a.x87[2]=9.0L;
    a.floating_point=halo2::SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
    return a;
}
void resource_frame(M &m,const A &a,bool reserve,std::uint32_t ret,std::uint32_t base) {
    const auto sp=a.gp[4],bp=a.gp[5]-(reserve?0x1c:0x20),np=a.gp[5]+(reserve?0x10:0x14);
    const std::array words{ret,bp,0u,np,reserve?0x2000u:0x1000u,4u};
    for(unsigned i=0;i<words.size();++i) put(m,sp+i*4,words[i]);
    put(m,bp,base); put(m,np,reserve?0x100000u:0x1000u);
}
void run(halo2::SourcePhysicsContactImage image,std::uint32_t offset) {
    const bool tu15=image==halo2::SourcePhysicsContactImage::TitleUpdate15;
    M m,other;
    m.mount(0x2000000,std::vector<std::byte>(0x1000),M::Access::ReadWrite,"explicit platform caller stack input");
    halo2::SourceRuntimeHostVirtualMemory vm(m),foreign(other);
    halo2::PhysicsPlatformLocks locks;
    A a=frame();
    const C reserve{0x2daa75,0x800000b8,false},commit{0x2daaae,0x800000b8,false},init{0x2dabb6,0x80000123,false};
    resource_frame(m,a,true,tu15?0x2de52b:0x2daa7b,0);
    const auto unchanged=[&](auto action) {
        const A before=a; const auto view=m.view(0x2000000,0x1000);
        const std::vector<std::byte> bytes(view.begin(),view.end());
        action(); same_abi(a,before);
        check(std::ranges::equal(m.view(0x2000000,0x1000),bytes),"Rejected caller changed retained memory");
    };
    unchanged([&] { check(!halo2::source_try_native_process_heap_memory_call(m,{0x2daa76,0x800000b8,false},a,image,vm),
        "Unauthenticated resource caller was handled"); });
    unchanged([&] { rejects([&] { halo2::source_try_native_process_heap_memory_call(m,{reserve.instruction,reserve.target,true},a,image,vm); }); });
    unchanged([&] { rejects([&] { halo2::source_try_native_process_heap_memory_call(m,reserve,a,image,foreign); }); });
    for(const auto displacement: {0u,4u,12u,8u,16u,20u}) {
        const auto p=a.gp[4]+displacement,old=word(m,p); put(m,p,old^1u);
        unchanged([&] { rejects([&] { halo2::source_try_native_process_heap_memory_call(m,reserve,a,image,vm); }); });
        put(m,p,old);
    }
    auto expected=a; expected.gp[0]=0; expected.gp[4]+=24;
    check(halo2::source_try_native_process_heap_memory_call(m,reserve,a,image,vm),"Authenticated reserve was not handled");
    same_abi(a,expected);
    const auto base=word(m,a.gp[5]-0x1c);
    check(base==0x10000000,"Rejected requests consumed actual resource ownership");
    rejects([&] { m.view(base,1); });
    a=frame(); resource_frame(m,a,false,tu15?0x2de564:0x2daab4,base);
    expected=a; expected.gp[0]=0; expected.gp[4]+=24;
    check(halo2::source_try_native_process_heap_memory_call(m,commit,a,image,vm),"Authenticated commit was not handled");
    same_abi(a,expected); check(m.view(base,0x1000).size()==0x1000,"Real commit did not retain source backing");
    rejects([&] { m.view(base+0x1000,1); });
    const auto pointer=base+offset;
    const std::vector<std::byte> dirty(28,std::byte{0xa5}); m.write(pointer,dirty);
    a=frame(); put(m,a.gp[4],tu15?0x2de66c:0x2dabbc); put(m,a.gp[4]+4,pointer);
    const halo2::PhysicsPlatformLockServices forbidden{
        []() -> std::uint32_t { throw std::runtime_error("Initializer invoked thread owner"); },
        [](auto) { throw std::runtime_error("Initializer invoked wait owner"); },
        [](auto) { throw std::runtime_error("Initializer invoked wake owner"); }};
    const auto untouched=[&](auto action) {
        const A before=a;
        action(); same_abi(a,before);
        check(std::ranges::equal(m.view(pointer,28),dirty),"Rejected initializer changed its record");
    };
    untouched([&] { check(!halo2::source_try_native_physics_lock_call(m,{init.instruction+1,init.target,false},a,image,locks,forbidden),
        "Unauthenticated initializer caller was handled"); });
    untouched([&] { rejects([&] { halo2::source_try_native_physics_lock_call(m,{init.instruction,init.target,true},a,image,locks,forbidden); }); });
    const auto ret=word(m,a.gp[4]); put(m,a.gp[4],ret+1);
    untouched([&] { rejects([&] { halo2::source_try_native_physics_lock_call(m,init,a,image,locks,forbidden); }); });
    put(m,a.gp[4],ret); expected=a; expected.gp[4]+=8;
    unsigned stores=0;
    check(halo2::source_try_native_physics_lock_call(m,init,a,image,locks,forbidden,[&](const auto &change) {
        check(change.address>=pointer && change.address+change.written.size()<=pointer+28,
            "Initializer wrote outside actual passed record"); ++stores;
    }),"Authenticated initializer was not handled");
    same_abi(a,expected);
    check(stores==8 && m.view(pointer,28)[1]==std::byte{0xa5} && m.view(pointer,28)[3]==std::byte{0xa5},
        "Initializer did not preserve untouched event bytes");
}
void shutdown(halo2::SourcePhysicsContactImage image) {
    const bool later=image==halo2::SourcePhysicsContactImage::TitleUpdate15;
    const auto record=later?0x46e2e8u:0x469228u;
    M m,other;
    m.mount(0x2000000,std::vector<std::byte>(0x1000),M::Access::ReadWrite,"explicit shutdown caller stack");
    m.mount(record,std::vector<std::byte>(16,std::byte{0xa5}),M::Access::ReadWrite,"explicit dirty shutdown record");
    halo2::SourceProcessShutdownRegistry registry(m,0x5000000),foreign(other,0x5000000);
    A a=frame();const C call{0x2d7fa5,0x8000002f,false};
    put(m,a.gp[4],later?0x2dba5b:0x2d7fab);put(m,a.gp[4]+4,record);put(m,a.gp[4]+8,1);
    const auto unchanged=[&](auto action) {
        const A before=a;const auto view=m.view(record,16);const std::vector<std::byte> bytes(view.begin(),view.end());
        action();same_abi(a,before);
        check(std::ranges::equal(m.view(record,16),bytes) && registry.records().empty(),
            "Rejected shutdown call changed list/source record");
    };
    unchanged([&] { check(!halo2::source_try_native_process_shutdown_call(m,{call.instruction+1,call.target,false},a,image,registry),
        "Unauthenticated shutdown caller was handled"); });
    unchanged([&] { check(!halo2::source_try_native_process_shutdown_call(m,{call.instruction,call.target+1,false},a,image,registry),
        "Unauthenticated shutdown target was handled"); });
    unchanged([&] { rejects([&] { halo2::source_try_native_process_shutdown_call(m,{call.instruction,call.target,true},a,image,registry); }); });
    unchanged([&] { rejects([&] { halo2::source_try_native_process_shutdown_call(m,call,a,image,foreign); }); });
    for(const auto offset:{0u,4u,8u}) {
        const auto p=a.gp[4]+offset,old=word(m,p);put(m,p,old^1u);
        unchanged([&] { rejects([&] { halo2::source_try_native_process_shutdown_call(m,call,a,image,registry); }); });
        put(m,p,old);
    }
    auto expected=a;expected.gp[4]+=12;
    check(halo2::source_try_native_process_shutdown_call(m,call,a,image,registry),"Authenticated shutdown caller was not handled");
    same_abi(a,expected);
    check(registry.records().size()==1 && registry.records()[0].address==record,
        "Actual shutdown record identity was not retained");
}
void startup_records(halo2::SourcePhysicsContactImage image,bool dpc) {
    const bool later=image==halo2::SourcePhysicsContactImage::TitleUpdate15;
    const auto record=dpc?(later?0x56edc0u:0x566ea0u):(later?0x56ede0u:0x566ec0u);
    const auto size=dpc?28u:40u,width=dpc?16u:12u;
    const C call{dpc?0x2dc611u:0x2dc61eu,dpc?0x8000006bu:0x80000071u,false};
    M m;
    m.mount(0x2000000,std::vector<std::byte>(0x1000),M::Access::ReadWrite,"explicit startup caller stack");
    const std::vector<std::byte> dirty(size,std::byte{0xa5});
    m.mount(record,dirty,M::Access::ReadWrite,"explicit dirty startup record");
    A a=frame();
    put(m,a.gp[4],dpc?(later?0x2e00c7u:0x2dc617u):(later?0x2e00d4u:0x2dc624u));
    put(m,a.gp[4]+4,record);put(m,a.gp[4]+8,dpc?(later?0x2e009du:0x2dc5edu):0u);
    if(dpc)put(m,a.gp[4]+12,0);
    unsigned stores=0;
    const auto observe=[&](const auto &) { ++stores; };
    const auto unchanged=[&](auto action) {
        const A before=a;action();same_abi(a,before);
        check(stores==0 && std::ranges::equal(m.view(record,size),dirty),"Rejected startup call changed its record");
    };
    unchanged([&] { check(!halo2::source_try_native_process_startup_record_call(m,{call.instruction+1,call.target,false},a,image,observe),
        "Unauthenticated startup caller was handled"); });
    unchanged([&] { check(!halo2::source_try_native_process_startup_record_call(m,{call.instruction,call.target+1,false},a,image,observe),
        "Unauthenticated startup target was handled"); });
    unchanged([&] { rejects([&] { halo2::source_try_native_process_startup_record_call(m,{call.instruction,call.target,true},a,image,observe); }); });
    for(unsigned offset=0;offset<width;offset+=4) {
        const auto p=a.gp[4]+offset,old=word(m,p);put(m,p,old^1u);
        unchanged([&] { rejects([&] { halo2::source_try_native_process_startup_record_call(m,call,a,image,observe); }); });
        put(m,p,old);
    }
    unchanged([&] { rejects([&] { halo2::source_try_native_process_startup_record_call(m,call,a,
        static_cast<halo2::SourcePhysicsContactImage>(99),observe); }); });
    const auto sp=a.gp[4];a.gp[4]=0x2001000u-width+4;
    unchanged([&] { rejects([&] { halo2::source_try_native_process_startup_record_call(m,call,a,image,observe); }); });
    a.gp[4]=sp;
    m.release(record);
    {
        const A before=a;
        rejects([&] { halo2::source_try_native_process_startup_record_call(m,call,a,image,observe); });
        same_abi(a,before);check(stores==0,"Absent startup record produced stores");
    }
    m.mount(record,dirty,M::Access::ReadWrite,"explicit remounted startup record");
    for(const auto access:{M::Access::ReadOnly,M::Access::ReadWrite}) {
        m.release(record);
        m.mount(record,std::span(dirty).first(access==M::Access::ReadOnly?size:size-1),access,"explicit inaccessible startup record");
        const A before=a;
        rejects([&] { halo2::source_try_native_process_startup_record_call(m,call,a,image,observe); });
        same_abi(a,before);check(stores==0,"Partial/readonly startup record was mutated");
        check(std::ranges::equal(m.view(record,access==M::Access::ReadOnly?size:size-1),
            std::span(dirty).first(access==M::Access::ReadOnly?size:size-1)),"Rejected record lost dirty bytes");
    }
    m.release(record);m.mount(record,dirty,M::Access::ReadWrite,"explicit admitted startup record");
    auto expected=a;expected.gp[4]+=width;
    check(halo2::source_try_native_process_startup_record_call(m,call,a,image,observe),"Authenticated startup call was not handled");
    same_abi(a,expected);check(stores==(dpc?4u:11u),"Startup field-store count differs from primary capture");
    // Direct adapters also preserve an explicit active physical MMX alias.
    a=frame();a.x87_depth=0;a.x87_status=0;a.active_mmx=halo2::SourcePhysicsMmxAlias{};
    a.active_mmx->known_mask=0xa5;a.active_mmx->physical[3][7]=std::byte{0x91};
    expected=a;expected.gp[4]+=width;
    check(halo2::source_try_native_process_startup_record_call(m,call,a,image),"MMX-preserving startup adapter unavailable");
    same_abi(a,expected);
}
}
int main() {
#if !defined(__linux__)
    return 77;
#endif
    try {
        for(const auto image: {halo2::SourcePhysicsContactImage::MapPack11,halo2::SourcePhysicsContactImage::TitleUpdate15})
        {
            for(const auto offset: {0x610u,0xc1cu}) run(image,offset);
            shutdown(image);
            startup_records(image,true);startup_records(image,false);
        }
        std::cout << "Both authenticated images: retained reserve/commit/initializer/shutdown ABI, rejected frames, bank ownership and untouched bytes match\n";
    } catch(const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
