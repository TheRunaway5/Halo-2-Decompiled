#pragma once
#include "halo2/physics_contact_record.hpp"
#include "halo2/source_effect_location.hpp"
#include <thread>
#include <typeinfo>

namespace halo2 {
// The actual birth-color observer changes only a host presentation receipt.
// Its private callable forwards an outer observer first, preserving its order.
// An exact synchronous borrow qualifies it only when that outer observer is
// absent or is itself an actively borrowed collector from this same domain.
class NativeWeaponEffectBirthColorCollector final {
    struct Capture {
        SourceWeaponEffectPresentation* presentation;
        const SourcePhysicsContactRecordObserver* outer;
        void operator()(const SourcePhysicsContactRecordStore&store)const {
            if(*outer)(*outer)(store);
            const auto pc=store.instruction;
            const bool a=pc==0x17b0a3u||pc==0x17e484u||pc==0x17e4d2u||pc==0x17e58eu;
            const bool b=pc==0x17b0a9u||pc==0x17e48du||pc==0x17e4dbu||pc==0x17e594u;
            if((!a&&!b)||store.written.size()!=4)return;
            std::uint32_t color{};for(unsigned n=0;n<4;++n)color|=std::to_integer<unsigned>(store.written[n])<<(n*8u);
            (a?presentation->colors.a:presentation->colors.b)=color;presentation->colors_observed=true;
        }
    };
public:
    class Borrow final {
        friend class NativeWeaponEffectBirthColorCollector;
        const NativeWeaponEffectBirthColorCollector&owner_;
        const Borrow*previous_;
        std::thread::id executor_;
        bool active_;
        explicit Borrow(const NativeWeaponEffectBirthColorCollector&);
    public:
        ~Borrow(){if(active_)current_=previous_;}
        Borrow(const Borrow&)=delete;
        Borrow&operator=(const Borrow&)=delete;
        Borrow(Borrow&&)=delete;
        Borrow&operator=(Borrow&&)=delete;
    };
private:
    inline static thread_local const Borrow*current_{};
    SourceWeaponEffectPresentation*const presentation_;
    const SourcePhysicsContactRecordObserver&outer_;
    const std::thread::id executor_;
    const SourcePhysicsContactRecordObserver observer_;
public:
    NativeWeaponEffectBirthColorCollector(SourceWeaponEffectPresentation&presentation,
        const SourcePhysicsContactRecordObserver&outer)
        :presentation_(&presentation),outer_(outer),executor_(std::this_thread::get_id()),observer_(Capture{&presentation,&outer}){}
    NativeWeaponEffectBirthColorCollector(const NativeWeaponEffectBirthColorCollector&)=delete;
    NativeWeaponEffectBirthColorCollector&operator=(const NativeWeaponEffectBirthColorCollector&)=delete;
    const SourcePhysicsContactRecordObserver&observer()const{return observer_;}
    Borrow borrow()const{return Borrow(*this);}
    static bool qualified(const SourcePhysicsContactRecordObserver&);
};
inline bool NativeWeaponEffectBirthColorCollector::qualified(const SourcePhysicsContactRecordObserver&observer) {
    const auto thread=std::this_thread::get_id();
    const auto*expected=&observer;auto*cursor=current_;
    while(cursor) {
        const Borrow*found{};
        for(auto*scope=cursor;scope;scope=scope->previous_) {
            const auto&owner=scope->owner_;
            if(scope->executor_!=thread||owner.executor_!=thread||expected!=&owner.observer_)continue;
            const auto*target=expected->target<Capture>();
            if(target&&expected->target_type()==typeid(Capture)&&target->presentation==owner.presentation_&&target->outer==&owner.outer_){found=scope;break;}
        }
        if(!found)return false;
        const auto&outer=found->owner_.outer_;
        if(!outer)return true;
        // The outer std::function is live caller data, including an external
        // initially-empty object. Revalidate it now; activation is no promise
        // that the caller has left that object empty for the whole borrow.
        expected=&outer;cursor=found->previous_;
    }
    return false;
}
inline NativeWeaponEffectBirthColorCollector::Borrow::Borrow(const NativeWeaponEffectBirthColorCollector&owner)
    :owner_(owner),previous_(current_),executor_(std::this_thread::get_id()),
    active_(owner.executor_==executor_&&(!owner.outer_||qualified(owner.outer_))) {
    if(active_)current_=this;
}
} // namespace halo2
