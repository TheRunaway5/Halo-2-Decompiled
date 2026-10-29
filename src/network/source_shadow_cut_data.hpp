#pragma once
#include "halo2/source_shadow_transport.hpp"
#include <mutex>
#include <atomic>
namespace halo2 {
struct AcceptedShadowCut::Data {
    std::shared_ptr<const int> connection_owner;
    std::uint64_t revision{};
    MultiplayerSnapshotOrigin origin;
    SourceShadowPayload payload;
    mutable std::once_flag encoded_once;
    mutable std::shared_ptr<const std::vector<std::byte>> encoded;
    mutable std::atomic<bool> local_acknowledged{};
    Data(std::shared_ptr<const int> owner,std::uint64_t value,MultiplayerSnapshotOrigin accepted,SourceShadowPayload source)
        :connection_owner(std::move(owner)),revision(value),origin(std::move(accepted)),payload(std::move(source)){}
};
}
