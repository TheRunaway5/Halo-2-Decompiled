#pragma once
#include "halo2/physics_contact_runtime.hpp"
#include <exception>
#include <thread>

namespace halo2 {class SourcePhysicsGameArenaNative;}
namespace halo2::native_arena_publication_detail {
// The real owner alone can publish its synchronous const Services table.
// The generated runtime can inspect eligibility without linking back to the
// concrete platform-owner library or acquiring a source storage permission.
class Borrow final {
    friend class halo2::SourcePhysicsGameArenaNative;
    using Services=SourcePhysicsContactRuntimeServices;
    using Match=bool(*)(const Services&,const void*)noexcept;
    const SourcePhysicsContactMemory*const memory_;
    const Services*const services_;
    const void*const owner_;
    const Match matches_;
    const Borrow*const previous_;
    const std::thread::id executor_=std::this_thread::get_id();
    inline static thread_local const Borrow*active_{};
    Borrow(const SourcePhysicsContactMemory&memory,const Services&services,const void*owner,Match matches)noexcept
        :memory_(&memory),services_(&services),owner_(owner),matches_(matches),previous_(active_){active_=this;}
public:
    ~Borrow(){if(active_!=this||executor_!=std::this_thread::get_id())std::terminate();active_=previous_;}
    Borrow(const Borrow&)=delete;
    Borrow&operator=(const Borrow&)=delete;
    static bool qualified(const SourcePhysicsContactMemory&memory,const Services&services)noexcept {
        const auto*current=active_;
        return current&&current->executor_==std::this_thread::get_id()&&
            current->memory_==&memory&&current->services_==&services&&
            current->owner_&&current->matches_&&current->matches_(services,current->owner_);
    }
};
} // namespace halo2::native_arena_publication_detail
