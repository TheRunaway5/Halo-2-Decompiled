#pragma once
#include "halo2/online_voice.hpp"
#include "halo2/math.hpp"
#include <array>
#include <cstddef>
#include <optional>
namespace halo2 {
struct GameVoicePlayer {
  bool connected{}, active{}, juggernaut{}, disabled{};
  int team=-1;
  Vec3 position;
  float yaw{};
  std::uint16_t muted{};
};
struct GameVoiceGains {float left{},right{};};
bool game_voice_active(bool valid, bool current_unit, bool previous_unit,
                       std::optional<float> seconds_since_death);
std::optional<GameVoiceGains> game_voice_route(unsigned sender,unsigned listener,
    const std::array<GameVoicePlayer,16>&,bool radio,bool team_game,
    bool juggernaut_game,float motion_sensor_range);
class GameVoiceRadio {
  bool armed_{},released_=true,previous_{};
  double start_{},last_{};
public:
  bool step(double seconds,bool pressed,bool speech,bool active);
};
// Original 0x162DE0/0x162FD0: four consecutive audible/talking updates,
// smallest eligible counter, player-iterator order on ties, remote users only.
class GameVoiceSpeaker {
  std::array<unsigned,16> counters_{};
public:
  std::optional<unsigned> update(const std::array<bool,16>& talking,unsigned local);
};
struct GameVoiceFrame {
  unsigned source{},sequence{};
  bool radio{};
  GameVoiceGains gains{1,1};
  std::vector<std::uint8_t> opus;
  // Local delivery stamp, never serialized on the peer wire. A session tags
  // decoded incoming frames; the microphone worker tags outgoing frames.
  std::uint64_t delivery_generation{};
  // Microphone proof bound to the intended encrypted peer recipient.
  std::optional<std::array<std::byte,64>> origin_signature;
};
struct GameVoiceSnapshot {
  bool enabled{},microphone_muted{},transmitting{},radio{};
  std::uint16_t muted{};
  std::array<bool,16> talking{};
  std::string error;
  std::uint16_t blocked{}; // Service-owned; manual unmute cannot clear this.
};
// Codec/device work runs off the simulation/render thread. Speech detection is
// Opus DTX, a native replacement for the original platform's opaque XHV VAD.
class GameVoice {
public:
  explicit GameVoice(std::unique_ptr<VoiceDevices> = sdl_voice_devices());
  ~GameVoice();
  void enable(bool);
  void close(); // Join the audio worker before another session opens devices.
  void microphone_muted(bool);
  void focus(bool);
  void radio(bool pressed,bool active);
  void mute(unsigned,bool);
  void blocks(std::uint16_t);
  // Close delivery immediately and request worker-owned codec/talker/device
  // queue retirement. The returned stamp is unique within this process.
  // No device I/O or worker join occurs on the requesting simulation/network
  // thread. ready() acknowledges retirement before a new native role opens.
  std::uint64_t fence();
  bool ready(std::uint64_t)const;
  bool resume(std::uint64_t);
  GameVoiceSnapshot snapshot()const;
  std::vector<GameVoiceFrame> outgoing();
  void receive(GameVoiceFrame);
private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
} // namespace halo2
