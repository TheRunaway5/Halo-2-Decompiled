#pragma once
#include "halo2/native_runtime_address.hpp"
#include "halo2/native_tu15_online_services.hpp"
#include "halo2/online_directory.hpp"
#include "halo2/online_account.hpp"
#include "halo2/online_matchmaking.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/physics_world_runtime.hpp"

namespace halo2 {
// Existing desktop clients supplied by the process/frontend owner. Startup
// retains their real service objects without signing in, refreshing discovery
// or starting matchmaking. This is not an Xbox Live implementation.
class NativeTu15OnlineStartup final {
public:
    NativeTu15OnlineStartup(SourceRuntimeMemory&,std::shared_ptr<NativeTu15OnlineServices>);
    bool try_call(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,SourcePhysicsWorldRuntimeAbi&);
    unsigned references()const{return references_;}
    bool initialized()const{return bool(context_);}
    NativeRuntimeAddress native_address()const;
    void release();
private:
    SourceRuntimeMemory& memory_;
    std::shared_ptr<NativeTu15OnlineServices> services_;
    struct Context {std::shared_ptr<NativeTu15OnlineServices> services;};
    std::unique_ptr<Context> context_;
    unsigned references_{};
};
}
