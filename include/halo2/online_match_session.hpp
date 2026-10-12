#pragma once
#include "halo2/multiplayer_datagrams.hpp"
#include "halo2/game_voice.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
namespace halo2 {
struct UiVariant;
struct MatchVoicePacket { unsigned peer{};bool preferences{},enabled{};std::uint16_t muted{};GameVoiceFrame frame; };
class OnlineMatchSession final : public MultiplayerDatagrams {
public:
  enum class Phase { Connecting, Loading, Running, Reviewing, Postgame, Failed };
  OnlineMatchSession(std::string origin, std::string room, std::string bearer,
                     unsigned self, std::vector<OnlineAdmission>, bool loopback = false);
  ~OnlineMatchSession();
  Phase phase() const;
  unsigned self() const;
  unsigned host() const;
  const std::vector<OnlineAdmission> &roster() const;
  std::string error() const;
  void loaded(std::uint64_t fingerprint,const UiVariant&);
  void send(std::span<const std::byte>, unsigned peer) override;
  bool receive(std::vector<std::byte> &, unsigned &peer) override;
  void voice(unsigned target, const GameVoiceFrame &);
  void voice_preferences(unsigned target,bool enabled,std::uint16_t muted);
  std::vector<MatchVoicePacket> voice_packets();
  std::uint16_t voice_blocks() const;
  // One microphone/codec owner survives world teardown through PCR Done.
  void retain_voice(std::shared_ptr<GameVoice>);
  std::shared_ptr<GameVoice> retained_voice() const;
  void update_postgame_voice(bool focused);
  bool postgame_voice_enabled(unsigned) const;
  void close_postgame();
  void complete(std::span<const std::byte> consumed_report);
  // Call only after validating the report against the final native snapshot.
  void confirm_result();
  std::vector<std::byte> postgame() const;
private:
  void pump_postgame_voice();
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace halo2
