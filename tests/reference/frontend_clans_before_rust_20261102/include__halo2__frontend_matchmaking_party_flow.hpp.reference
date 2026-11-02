#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
namespace halo2::online_protocol_bridge {class NativeFrontendPartyFlowIo;class NativeFrontendPartyViewIo;class NativeFrontendAccountFlowIo;class NativeFrontendFriendsIo;struct FriendsFlowResult;struct PartyFlowState;}
namespace halo2 {
struct FrontendFriendsResult {bool handled{};std::string text;};
struct OnlinePartyCommand;
enum class FrontendPartyFlowAction:unsigned {Accept,Notice,SwitchTab,OpenedPrivacy,SelectPrivacy,ObservePrivacy};
class FrontendMatchmakingPartyFlow {
public:
  FrontendMatchmakingPartyFlow();~FrontendMatchmakingPartyFlow();
  FrontendMatchmakingPartyFlow(const FrontendMatchmakingPartyFlow&);
  FrontendMatchmakingPartyFlow&operator=(const FrontendMatchmakingPartyFlow&);
  FrontendMatchmakingPartyFlow(FrontendMatchmakingPartyFlow&&)noexcept;
  FrontendMatchmakingPartyFlow&operator=(FrontendMatchmakingPartyFlow&&)noexcept;
  std::uint64_t friend_join_serial()const;
  void friend_join_serial(std::uint64_t)const;
  std::string account_name()const;
  void account_name(const std::string&)const;
  bool voice_party()const;
  void voice_party(bool)const;
  bool voice_wanted()const;
  void voice_wanted(bool)const;
  bool match_voice(bool,bool)const;
  void observe_account(const online_protocol_bridge::NativeFrontendAccountFlowIo&,bool)const;
  online_protocol_bridge::FriendsFlowResult friends(unsigned,unsigned,unsigned,const online_protocol_bridge::NativeFrontendFriendsIo&)const;
  unsigned tab()const;
  std::string target()const;
  unsigned kind()const;
  std::vector<std::string> dismissed()const;
  void clear_dismissed()const;
  void prune(const std::vector<std::string>&)const;
  bool run(FrontendPartyFlowAction,const online_protocol_bridge::NativeFrontendPartyFlowIo&,int value=0)const;
  void command(const OnlinePartyCommand&,const online_protocol_bridge::NativeFrontendPartyFlowIo&)const;
  online_protocol_bridge::PartyFlowState state()const;
  void populate(const online_protocol_bridge::NativeFrontendPartyViewIo&)const;
  std::string text(unsigned,const online_protocol_bridge::NativeFrontendPartyViewIo&)const;
private:
  struct Impl;std::unique_ptr<Impl> impl_;
};
bool frontend_party_flow_screen(int mode,unsigned screen,unsigned context);
bool frontend_party_flow_picker(unsigned screen,unsigned context);
bool frontend_friends_flow_screen(unsigned screen,unsigned context);
}
