#pragma once
#include <utility>
namespace halo2::native_contact_detail {
// Native C++ resource ownership only. Arm/disarm points come from the actual
// TU1.5 unwind maps. Advance ownership BEFORE cleanup, as330DA8 does, without
// building a virtual FS chain or emulating source exception dispatch.
template<class Cleanup>class UnwindGuard {
    Cleanup cleanup_;bool armed_{};
public:
    explicit UnwindGuard(Cleanup cleanup):cleanup_(std::move(cleanup)){}
    UnwindGuard(const UnwindGuard&)=delete;
    UnwindGuard&operator=(const UnwindGuard&)=delete;
    void arm()noexcept{armed_=true;}
    void disarm()noexcept{armed_=false;}
    ~UnwindGuard()noexcept{if(std::exchange(armed_,false))cleanup_();}
};
} // namespace halo2::native_contact_detail
