#pragma once
#include <cstdint>
#include <memory>
#include <string>
namespace halo2::online_protocol_bridge {class NativeFrontendSearchIo;struct SearchSnapshot;}
namespace halo2 {
enum class FrontendSearchAction:unsigned {ResetClient,Prepare,Quickmatch,Refresh,Update,PartySearch,OpenedLobby,OpenedSearch,Select,Launch};
class FrontendMatchmakingSearch {
public:
 FrontendMatchmakingSearch();~FrontendMatchmakingSearch();
 FrontendMatchmakingSearch(const FrontendMatchmakingSearch&);
 FrontendMatchmakingSearch&operator=(const FrontendMatchmakingSearch&);
 FrontendMatchmakingSearch(FrontendMatchmakingSearch&&)noexcept;
 FrontendMatchmakingSearch&operator=(FrontendMatchmakingSearch&&)noexcept;
 bool active()const;bool quick()const;bool pending()const;
 std::int32_t countdown()const;unsigned title()const;std::uint32_t clock()const;
 std::string selected()const;
 void stop()const;void quick(bool)const;void left()const;void reset(std::uint32_t)const;
 bool run(FrontendSearchAction,const online_protocol_bridge::NativeFrontendSearchIo&,unsigned=0)const;
 int ui(online_protocol_bridge::SearchSnapshot)const;
 unsigned pane(online_protocol_bridge::SearchSnapshot)const;
private:
 struct Impl;std::unique_ptr<Impl> impl_;
};
}
