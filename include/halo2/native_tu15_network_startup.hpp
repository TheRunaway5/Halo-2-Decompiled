#pragma once
#include "halo2/native_tu15_network_service.hpp"
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_runtime_memory.hpp"

namespace halo2 {
// Named native platform API owner. The desktop OS provides IP/driver startup;
// an owned native service provides resources and socket/configuration limits.
// Later Xbox Live/secure-association APIs require their own native owners.
class NativeTu15NetworkStartup final {
public:
    explicit NativeTu15NetworkStartup(SourceRuntimeMemory& memory):memory_(memory){}
    bool try_call(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,
        SourcePhysicsWorldRuntimeAbi&,const SourcePhysicsContactRecordObserver& = {});
    NativeTu15NetworkService* service()const{return service_.get();}
    unsigned xnet_references()const{return xnet_references_;}
    unsigned winsock_references()const{return winsock_references_;}
private:
    SourceRuntimeMemory& memory_;
    std::unique_ptr<NativeTu15NetworkService> service_;
    unsigned xnet_references_{},winsock_references_{};
};
}
