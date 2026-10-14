#pragma once
#include "halo2/cache.hpp"
#include <exception>

namespace halo2 {
// Completed-call qualification for externally quiescent native transactions.
// The root executor still owns the barrier over references and shared owners.
class NativeAuthorityBoundary {
public:
    std::size_t depth{};
    std::uint64_t mutations{};
    bool staging{},fault{};
    NativeAuthorityBoundary()=default;
    NativeAuthorityBoundary(const NativeAuthorityBoundary &other):fault(other.fault){
        if(other.depth || other.staging)throw FormatError("Copying an active native authority component");
    }
    NativeAuthorityBoundary &operator=(const NativeAuthorityBoundary &other) {
        if(depth || staging || other.depth || other.staging)throw FormatError("Replacing or copying an active native authority component");
        mutations=0;fault=other.fault;return *this;
    }
    class Call {
    public:
        explicit Call(NativeAuthorityBoundary &owner):owner_(owner),before_(owner.mutations) {
            if(owner.staging)throw FormatError("Native component mutation during authority preparation");
            ++owner.depth;
        }
        ~Call() {
            if(std::uncaught_exceptions()>exceptions_ && before_!=owner_.mutations)owner_.fault=true;
            --owner_.depth;
        }
        Call(const Call &)=delete;
        Call &operator=(const Call &)=delete;
    private:
        NativeAuthorityBoundary &owner_;
        std::uint64_t before_;
        int exceptions_=std::uncaught_exceptions();
    };
    void capture()const {
        if(depth || staging || fault)throw FormatError("Native continuation requires a completed unfaulted authority boundary");
    }
    void prepare()const {
        if(depth || staging)throw FormatError("Native continuation preparation requires a completed authority boundary");
    }
};
}
