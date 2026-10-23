#include "rendering/renderer/presentation_copy_worker.hpp"
#include <array>
#include <cerrno>
#include <cfenv>
#include <cstring>
#include <iostream>
#include <latch>
#include <optional>
#include <stdexcept>
#if defined(__SSE2__)
#include <xmmintrin.h>
#endif
using namespace halo2;
namespace halo2 {
// Exercise exception propagation and an in-flight shutdown deterministically;
// ordinary callers can submit only the fixed immutable-plan byte copier.
struct PresentationCopyWorkerTestAccess {
    static auto start(PresentationCopyWorker &worker,const void *source,void *destination,
                      void(*copy)(const void*,void*,std::size_t)) {
        return worker.enqueue(source,destination,1,copy);
    }
};
}
namespace {
void need(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
struct Fp {
    std::fenv_t saved{};int rounding{},flags{},error{};unsigned mxcsr{};
    Fp(){error=errno;std::fegetenv(&saved);rounding=std::fegetround();flags=std::fetestexcept(FE_ALL_EXCEPT);
#if defined(__SSE2__)
        mxcsr=_mm_getcsr();
#endif
    }
    bool same()const {Fp now;return rounding==now.rounding && flags==now.flags && error==now.error && mxcsr==now.mxcsr;}
    void restore()const {std::fesetenv(&saved);errno=error;}
};
}
int main()try {
    Fp initial;
    std::vector<Vertex> a(17),b(19),ui(6);unsigned seed=0x1376c0;
    for(auto *bank:{&a,&b,&ui})for(auto &x:std::as_writable_bytes(std::span{*bank})) {
        seed=seed*1664525u+1013904223u;x=std::byte(seed>>24);
    }
    PresentationCopyWorker worker;std::optional<std::thread::id> executor;
    unsigned checks{};
    for(unsigned frame=0;frame<64;++frame) {
        FrameVertexUploadPlan<unsigned> plan(frame%2);
        plan.append(0,a);plan.append(0,a);plan.append(1,b);
        const auto prefix=plan.size();plan.append_raw(ui);
        std::vector<Vertex> actual(plan.size()),expected(plan.size());
        std::fesetround(frame%2?FE_DOWNWARD:FE_TONEAREST);errno=EDOM;const Fp before;
        auto copy=worker.start(plan,actual.data(),prefix);
        ui.front().material=frame; // Only the excluded UI tail is editable.
        const auto result=copy.finish();
        need(before.same(),"Presentation copy changed render-owner FP/errno state");
        plan.copy_to(actual.data()+prefix,prefix,plan.size()-prefix);plan.copy_to(expected.data());
        need(!std::memcmp(actual.data(),expected.data(),actual.size()*sizeof(Vertex)),"Worker prefix/UI tail changed complete upload bytes");
        need(result.executor!=std::this_thread::get_id(),"Presentation copy still executes on the render-owner thread");
        if(executor)need(*executor==result.executor,"Presentation worker was recreated between frames");else executor=result.executor;
        need(result.vertices==prefix && result.end_ns>=result.begin_ns,"Worker completion lost its actual copied range/timing");
        ++checks;
    }
    FrameVertexUploadPlan<unsigned> empty(false);auto zero=worker.start(empty,nullptr,0);need(zero.finish().vertices==0,"Empty worker copy changed extent");
    bool rejected{};try{(void)worker.start(empty,nullptr,1);}catch(const FormatError&){rejected=true;}
    need(rejected,"Worker published an invalid borrowed range");
    FrameVertexUploadPlan<unsigned> plan(false);plan.append_raw(a);std::vector<Vertex> out(a.size());
    plan.copy_to(nullptr,0,0);plan.copy_to(nullptr,plan.size(),0);
    rejected=false;try{plan.copy_to(nullptr,0,1);}catch(const FormatError&){rejected=true;}
    need(rejected,"Ranged upload accepted a null nonempty destination");
    rejected=false;try{plan.copy_to(out.data(),plan.size(),1);}catch(const FormatError&){rejected=true;}
    need(rejected,"Ranged upload accepted an escaping tail");
    {
        auto pending=worker.start(plan,out.data(),plan.size());
        rejected=false;try{(void)worker.start(empty,nullptr,0);}catch(const FormatError&){rejected=true;}
        need(rejected,"A new handoff overwrote an outstanding ticket");
        auto moved=std::move(pending);need(!pending && bool(moved),"Moving the ticket duplicated its borrowed lifetime");
        (void)moved.finish();need(!moved,"Finished ticket still owns its frame");
    }
    FrameVertexUploadPlan<unsigned> segmented(false);segmented.append_raw(a);segmented.append_raw(b);segmented.append_raw(ui);
    std::vector<Vertex> all(segmented.size());segmented.copy_to(all.data());
    for(std::size_t first=0;first<=all.size();++first)for(std::size_t count=0;count<=all.size()-first;++count) {
        std::vector<Vertex> sub(count);segmented.copy_to(sub.data(),first,count);
        need(!count || !std::memcmp(sub.data(),all.data()+first,count*sizeof(Vertex)),"Segmented subrange changed vertex bytes at a chunk boundary");
    }
    {
        auto owner=std::make_unique<PresentationCopyWorker>();auto pending=owner->start(plan,out.data(),plan.size());
        owner.reset();need(pending.finish().vertices==a.size(),"Shutdown lost an outstanding copy");
    }
    need(!std::memcmp(out.data(),a.data(),a.size()*sizeof(Vertex)),"Shutdown freed a borrowed copy before completion");
    try{auto pending=worker.start(plan,out.data(),plan.size());throw 1;}catch(int){}
    need(!std::memcmp(out.data(),a.data(),a.size()*sizeof(Vertex)),"Exceptional scope failed to join its presentation copy");
    {
        const Fp before;
        auto failing=PresentationCopyWorkerTestAccess::start(worker,nullptr,nullptr,[](const void*,void*,std::size_t) {
            errno=EFAULT;std::fesetround(FE_UPWARD);throw std::runtime_error("worker copy sentinel");
        });
        bool observed=false;try{(void)failing.finish();}catch(const std::runtime_error &error) {
            observed=std::string_view(error.what())=="worker copy sentinel";
        }
        need(observed,"Worker lost an asynchronous copy exception");
        need(before.same(),"Worker failure changed its render owner's FP/errno");
        auto recovery=worker.start(plan,out.data(),plan.size());(void)recovery.finish();
    }
    {
        struct HeldCopy {std::latch entered{1},release{1};bool finished{};} held;
        auto owner=std::make_unique<PresentationCopyWorker>();
        auto pending=PresentationCopyWorkerTestAccess::start(*owner,&held,nullptr,[](const void *p,void*,std::size_t) {
            auto &h=*const_cast<HeldCopy*>(static_cast<const HeldCopy*>(p));
            h.entered.count_down();h.release.wait();h.finished=true;
        });
        held.entered.wait(); // Handoff returned while the worker is still in its job.
        std::thread release([&]{held.release.count_down();});
        owner.reset();release.join();
        need(held.finished,"Worker teardown returned while a job still borrowed its frame");
        need(pending.finish().vertices==1,"Shutdown lost the joined job's completion");
    }
    initial.restore();std::cout<<"PASS "<<checks<<" exact segmented/deduplicated prefix+editable-tail frames, persistent separate executor, FP/errno, empty/error, unwind and pending shutdown\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
