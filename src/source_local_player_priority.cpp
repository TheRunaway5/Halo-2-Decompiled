#include "halo2/source_local_player_priority.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <string>
#include <vector>

namespace halo2 {
namespace {
constexpr auto none=0xffffffffu;
template<class T> T field(const std::optional<T> &value,const char *name) {
    if(!value)throw FormatError(std::string("source local-player priority unresolved ")+name);
    return *value;
}
const SourcePriorityOwner &find(std::span<const SourcePriorityOwner> owners,std::uint32_t id) {
    const SourcePriorityOwner *result=nullptr;
    for(const auto &owner:owners)if(owner.identity==id) {
        if(result)throw FormatError("source local-player priority duplicate owner");
        result=&owner;
    }
    if(!result || id==none)throw FormatError("source local-player priority missing owner");
    return *result;
}
}
std::uint8_t source_priority_observer_class(const SourcePriorityObserver &observer,
    std::optional<std::uint8_t> special_byte) {
    const auto callback=field(observer.callback,"observer callback");
    if(callback==0x23c110u) {
        //155780 UCOMISS then LAHF/TEST44/JP rejects unordered comparison.
        return (field(observer.scalar_bits,"observer scalar")&0x7fffffffu)==0?0:3;
    }
    if(callback==0x23cbb0u)return 1;
    if(callback==0x16c840u)return 2;
    if(callback==0x23d090u && field(special_byte,"observer special byte")!=0)return 2;
    return 3;
}
std::uint8_t source_local_player_priority(std::span<const SourcePriorityOwner> owners,
    std::uint32_t owner,const SourceLocalPlayerPriorityContext &context) {
    if(owner==none)return 0;
    const auto slot=field(context.local_slot,"current local slot");
    if(slot==-1)return 0;
    if(slot<0 || slot>=4 || std::size_t(slot)>=context.observers.size())
        throw FormatError("source local-player priority invalid observer slot");
    if(source_priority_observer_class(context.observers[std::size_t(slot)],context.observer_special)!=0)
        return 0;
    const auto *root=&find(owners,owner);
    std::vector<std::uint32_t> visited;
    for(;;) {
        if(std::find(visited.begin(),visited.end(),root->identity)!=visited.end())
            throw FormatError("source local-player priority parent cycle");
        visited.push_back(root->identity);
        const auto parent=field(root->parent,"parent link");
        if(parent==none)break;
        root=&find(owners,parent);
    }
    if(field(root->header_kind,"root header kind")!=0)return 0;
    if(std::size_t(slot)>=context.local_players.size())
        throw FormatError("source local-player priority missing local-player slot");
    const auto local_player=field(context.local_players[std::size_t(slot)],"local player datum");
    return local_player==field(root->player,"root player datum")?1:0;
}
}
