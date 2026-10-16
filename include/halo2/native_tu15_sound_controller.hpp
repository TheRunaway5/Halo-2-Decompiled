#pragma once
#include "halo2/native_tu15_sound_controller_arithmetic.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <span>
namespace halo2::tu15_sound_graph_storage {class GraphStorage;}
namespace halo2::tu15_sound_controller {
using Word24=tu15_sound::Word24;
inline constexpr unsigned ring_words=640,ring_bytes=2560,packet_samples=320;
struct SinkSeed {
    std::span<const Word24,ring_words> words;
    std::uint32_t byte_cursor{},last_copied_count{};
    bool descriptor_constructed{};
};
struct CopyResult {
    std::uint32_t copied_words{},cursor_before{},cursor_after{},first_chunk_words{},second_chunk_words{};
    bool descriptor_constructed_this_call{};
};
// Native values and their actual owner. This is not a physical scratch byte
// image or an emulated DMA/peripheral. Its copy result records completed C++
// writes only. Explicit restore requires a complete validated native seed.
class Sink final {
public:
    Sink()=default;
    explicit Sink(const SinkSeed& seed){restore(seed);}
    Sink(const Sink&)=delete;Sink& operator=(const Sink&)=delete;
    Sink(Sink&&)=delete;Sink& operator=(Sink&&)=delete;
    std::span<Word24,ring_words> words()noexcept{return words_;}
    std::span<const Word24,ring_words> words()const noexcept{return words_;}
    std::uint32_t byte_cursor()const noexcept{return byte_cursor_;}
    std::uint32_t last_copied_count()const noexcept{return last_copied_count_;}
    bool descriptor_constructed()const noexcept{return descriptor_constructed_;}
    void restore(const SinkSeed&);
    void validate_contents()const;
    void validate(bool source_cold_initialization)const;
    CopyResult append(std::span<const Word24>,bool source_cold_initialization);
private:
    std::array<Word24,ring_words> words_{};
    std::uint32_t byte_cursor_{},last_copied_count_{};
    bool descriptor_constructed_{};
};
struct ProducerResult {tu15_sound_controller_arithmetic::Result arithmetic;CopyResult copy;};
ProducerResult process(const tu15_sound_controller_arithmetic::Controls&,
                       const tu15_sound_controller_arithmetic::Parameters&,
                       tu15_sound_controller_arithmetic::State&,
                       std::span<Word24,32>,Sink&);
ProducerResult process(const tu15_sound_controller_arithmetic::Controls&,
                       tu15_sound_graph_storage::GraphStorage&,unsigned port);

// The caller supplies real native endpoint objects with lifetimes longer than
// Consumer. Neither interface invents a readiness/result value. Submitted
// packet views remain owned at stable addresses until the actual next callback
// for that packet index. Backend scheduling/completion is outside this module.
class Gate {
public:virtual ~Gate()=default;virtual bool can_copy()noexcept=0;
};
class PacketEndpoint {
public:virtual ~PacketEndpoint()=default;
    virtual std::uint32_t submit(unsigned packet_index,std::span<const std::uint16_t,packet_samples>)noexcept=0;
};
struct ConsumerSeed {
    std::uint32_t reader_byte_cursor;
    std::array<std::array<std::uint16_t,packet_samples>,2> packets;
};
struct CallbackResult {
    std::uint32_t copied_samples{},reader_before{},reader_after{};
    std::optional<std::uint32_t> actual_submission_result;
};
struct ResetResult {
    std::uint32_t returned_bits{},cleared_packets{},submitted_packets{};
};
class Consumer final {
public:
    Consumer(unsigned port,Gate&,PacketEndpoint*,const ConsumerSeed&);
    Consumer(const Consumer&)=delete;Consumer& operator=(const Consumer&)=delete;
    Consumer(Consumer&&)=delete;Consumer& operator=(Consumer&&)=delete;
    unsigned port()const noexcept{return port_;}
    std::uint32_t reader_byte_cursor()const noexcept{return reader_byte_cursor_;}
    std::span<const std::uint16_t,packet_samples> packet(unsigned)const;
    std::optional<std::uint32_t> last_actual_submission_result()const noexcept{return last_submission_;}
    CallbackResult callback(const Sink&,unsigned packet_index);
    CallbackResult callback(const tu15_sound_graph_storage::GraphStorage&,unsigned packet_index);
    ResetResult reset(const Sink&);
    ResetResult reset(const tu15_sound_graph_storage::GraphStorage&);
private:
    unsigned port_;
    Gate& gate_;PacketEndpoint* const endpoint_;
    std::uint32_t reader_byte_cursor_;
    std::array<std::array<std::uint16_t,packet_samples>,2> packets_;
    std::optional<std::uint32_t> last_submission_;
};
}
