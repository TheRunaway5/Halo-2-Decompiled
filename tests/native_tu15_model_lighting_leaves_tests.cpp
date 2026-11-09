#include "rendering/models/native_tu15_model_lighting_leaves.hpp"
#include "rendering/models/native_tu15_model_color.hpp"
#include "rendering/models/native_tu15_model_direction.hpp"
#include "rendering/models/native_tu15_model_color_step.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace halo2;
namespace {
constexpr std::array bases{0x46a000u,0x4ec000u,0x61000000u,0x62000000u};
constexpr std::array sizes{4096u,4096u,32768u,8192u};
class Memory final : public SourcePhysicsContactMemory {
public:
    std::array<std::vector<std::byte>, bases.size()> banks;
    Memory() { for (unsigned i = 0; i < banks.size(); ++i) banks[i].resize(sizes[i]); }
    std::span<std::byte> view(std::uint32_t address, std::size_t count) {
        for (unsigned i = 0; i < banks.size(); ++i)
            if (address >= bases[i] && address - bases[i] <= sizes[i] &&
                count <= sizes[i] - (address - bases[i]))
                return std::span(banks[i]).subspan(address - bases[i], count);
        throw std::out_of_range("PRT helper escaped declared original banks");
    }
    void read(std::uint32_t address, std::span<std::byte> bytes) const override {
        std::ranges::copy(const_cast<Memory*>(this)->view(address, bytes.size()), bytes.begin());
    }
    void write(std::uint32_t address, std::span<const std::byte> bytes) override {
        std::ranges::copy(bytes, view(address, bytes.size()).begin());
    }
};
std::uint32_t word() {
    std::array<unsigned char, 4> bytes{};
    std::cin.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
    if (!std::cin) throw std::runtime_error("Truncated native PRT request");
    return bytes[0] | (std::uint32_t(bytes[1]) << 8) |
        (std::uint32_t(bytes[2]) << 16) | (std::uint32_t(bytes[3]) << 24);
}
void emit(std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) std::cout.put(char(value >> (8 * i)));
}
void emit(const std::vector<std::byte>& bytes) {
    std::cout.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}
void color_step_observer_transport() {
    Memory memory;
    const auto put=[&](std::uint32_t at,std::uint32_t word){
        prt_helpers_detail::store(memory,at,word,4,0,{});
    };
    constexpr auto stack=0x62001000u,current=0x61001000u,target=0x61002000u;
    put(stack,0x63000000u);put(stack+4u,target);
    put(current,0x00112233u);put(target,0x00445566u);
    put(0x46a0c4u,0x3b808081u);put(0x46a148u,0x437f0000u);
    const SourcePhysicsContactRecordObserver observer=[&](const auto& event){
        if(event.instruction==0x134a3du)put(event.address,0xdeadbeefu);
    };
    SourcePhysicsWorldRuntimeAbi abi{};
    abi.gp[4]=stack;abi.gp[6]=current;abi.eflags=0x202u;
    abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27fu,0x1f80u};
    SourcePhysicsWorldRuntimeServices services;
    physics_world_runtime_detail::WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};
    state.load(abi);model_color_step_detail::bridge(state);
    const auto result=state.save();
    if(result.gp[0]!=0xdeadbeefu||result.gp[2]!=0x00112233u)
        throw std::runtime_error("Color-step caller lost cached GP2 across the child's scratch reread");
}
}
int main() try {
    color_step_observer_transport();
    while (std::cin.peek() != std::char_traits<char>::eof()) {
        const auto entry = word();
        SourcePhysicsWorldRuntimeAbi abi{};
        for (auto& value : abi.gp) value = word();
        abi.eflags = word();
        const auto control = word();
        abi.floating_point = SourcePhysicsFactoryFloatingPoint{std::uint16_t(word()), control};
        abi.x87_status = std::uint16_t(word());
        abi.x87_depth = word();
        if(abi.x87_depth>(entry==0x2c390u?6u:4u))throw std::runtime_error("Invalid model leaf x87 fixture depth");
        for(unsigned i=0;i<abi.x87_depth;++i) {
            std::cin.read(reinterpret_cast<char*>(&abi.x87[i]),10);
            if(!std::cin)throw std::runtime_error("Truncated model leaf live x87 value");
        }
        for (auto& lane : abi.xmm) for (auto& value : lane) value = word();
        Memory memory;
        for (auto& bytes : memory.banks) {
            std::cin.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
            if (!std::cin) throw std::runtime_error("Truncated native PRT bank");
        }
        std::vector<SourcePhysicsContactRecordStore> stores;
        SourcePhysicsContactRecordObserver observer = [&](const auto& event) { stores.push_back(event); };
        unsigned instructions{};
        SourcePhysicsWorldRuntimeServices services;
        services.instruction = [&](auto) { ++instructions; };
        physics_world_runtime_detail::WorldState state{memory, observer, services,
            SourcePhysicsContactImage::TitleUpdate15};
        state.load(abi);
        if(entry==0x3e4d0u)model_color_step_detail::bridge(state);
        else if(entry==0x11a360u)model_direction_detail::bridge(state);
        else if(entry==0x1346c0u||entry==0x1349e0u)model_color_detail::bridge(state,entry);
        else model_lighting_leaves_detail::bridge(state,entry);
        if (instructions) throw std::runtime_error("Native PRT executed translated instructions");
        const auto result = state.save();
        if(result.x87_depth!=abi.x87_depth)throw std::runtime_error("Native leaf changed live x87 depth");
        for(unsigned i=0;i<abi.x87_depth;++i)
            if(std::memcmp(&result.x87[i],&abi.x87[i],10))throw std::runtime_error("Native leaf changed live x87 input");
        for (auto value : result.gp) emit(value);
        emit(result.eflags);
        emit(result.floating_point->mxcsr);
        emit(result.x87_status);
        emit(result.floating_point->x87_control_word);
        for (const auto& lane : result.xmm) for (auto value : lane) emit(value);
        for (const auto& bytes : memory.banks) emit(bytes);
        emit(stores.size());
        for (const auto& event : stores) {
            emit(event.instruction); emit(event.address); emit(event.written.size());
            for (unsigned i = 0; i < 4; ++i)
                std::cout.put(i < event.before.size() ? char(event.before[i]) : '\0');
            for (unsigned i = 0; i < 4; ++i)
                std::cout.put(i < event.written.size() ? char(event.written[i]) : '\0');
        }
    }
    if (!std::cout) throw std::runtime_error("Cannot publish native PRT results");
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n'; return 1;
}
