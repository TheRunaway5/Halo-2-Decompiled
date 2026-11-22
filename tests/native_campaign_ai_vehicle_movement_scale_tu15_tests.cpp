#include "halo2/native_campaign_ai_vehicle_movement_scale_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_runtime_write_protection.hpp"

#include <bit>
#include <iostream>
#include <stdexcept>
#include <xmmintrin.h>

namespace {
using namespace halo2;
using Word = std::uint32_t;
using Bytes = std::array<std::byte, 4>;
constexpr auto read_write = SourceRuntimeMemory::Access::ReadWrite;
constexpr Word stack_base = 0x900000, sp = stack_base + 0x100;
constexpr Word actor_desc = 0x601000, actor = 0x602000;
constexpr Word object_desc = 0x600000, objects = 0x600100, vehicle = 0x603000;
constexpr Word tags = 0x604000, character = 0x604800, property = 0x605000;
constexpr Word previous = 0x606000, direction = 0x606010, actor_direction = 0x606020;
constexpr Word one = 0x3f800000, two = 0x40000000;
constexpr SourcePhysicsContactRuntimeCall actual{0x2a46ba, 0x202130, false};

void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

Word bits(float value) { return std::bit_cast<Word>(value); }

void store(SourceRuntimeMemory &memory, Word address, Word value) {
    memory.write(address, std::bit_cast<Bytes>(value));
}

Word load(const SourceRuntimeMemory &memory, Word address) {
    Bytes data;
    memory.read(address, data);
    return std::bit_cast<Word>(data);
}

struct Case {
    SourceRuntimeMemory memory;
    SourcePhysicsWorldRuntimeAbi abi{};

    Case() {
        memory.mount(0x46a000, std::vector<std::byte>(0x1000), read_write, "scalar constants");
        memory.mount(0x4ec000, std::vector<std::byte>(0x20000), read_write, "retained globals");
        memory.mount(object_desc, std::vector<std::byte>(0x7000), read_write, "actor and character banks");
        memory.mount(stack_base, std::vector<std::byte>(0x1000, std::byte{0xbd}), read_write, "private call stack");
        const auto fill = [&](Word address, Word value) { store(memory, address, value); };
        fill(0x46a0d0, one);
        fill(0x46a0ec, bits(0.001f));
        fill(0x46a11c, one);
        fill(0x46a174, bits(0.5f));
        fill(0x5020c8, actor_desc);
        fill(0x4ecbd0, object_desc);
        fill(0x4f0414, tags);
        fill(actor_desc + 0x44, actor);
        fill(object_desc + 0x44, objects);
        fill(objects + 0x20, vehicle);
        fill(vehicle, 0xe5610004);
        fill(vehicle + 0x88, one);
        fill(actor + 0x54, 0x82830001);
        fill(actor + 0x4b4, one);
        fill(actor + 0x50c, 0x42);
        fill(actor + 0x504, 1);
        fill(actor + 0x5b0, 6);
        fill(tags + 0x18, character);
        fill(character + 0xe4, 1);
        fill(character + 0xe8, property);
        fill(property + 4, 0xe5610004);
        fill(property + 0x1c, two);
        fill(property + 0x5c, one);
        fill(previous, bits(3.0f));
        fill(direction, one);
        fill(actor_direction, one);
        fill(sp, 0x2a46bf);
        fill(sp + 4, previous);
        fill(sp + 8, 0x606030);
        fill(sp + 12, actor_direction);
        fill(sp + 16, direction);
        fill(sp + 20, 0);
        fill(sp + 24, 0x7fc00000);
        fill(sp + 28, 0);
        abi.gp = {0xe5890002, 0xe3610000, 0xbead1002, 0xbead1003, sp, 0xbead1005, 0xbead1006, 0xbead1007};
        abi.xmm.fill({2, 3, 5, 7});
        abi.eflags = 0x202;
        abi.fs_base = 0xbead2000;
        abi.x87_status = 0x3800;
        abi.floating_point = SourcePhysicsFactoryFloatingPoint{0x027f, 0x1f80};
    }
};

std::vector<std::byte> stack(const SourceRuntimeMemory &memory) {
    const auto range = memory.view(sp - 0x45, 0x66);
    return {range.begin(), range.end()};
}

void verify_decline(Case &test, const SourcePhysicsContactRuntimeCall &call = actual,
                    const SourcePhysicsContactRecordObserver &observer = {}) {
    const SourceRuntimeWriteSerial serial(test.memory);
    const auto before = stack(test.memory);
    const auto abi = test.abi;
    const auto host_mxcsr = _mm_getcsr();
    require(!native_campaign_ai_vehicle_movement_scale_tu15_try(test.memory, call, test.abi, observer),
            "unsupported movement call was admitted");
    require(serial.load() == 0 && stack(test.memory) == before, "declined movement changed its stack");
    require(test.abi.gp == abi.gp && test.abi.xmm == abi.xmm && test.abi.eflags == abi.eflags &&
            test.abi.x87_status == abi.x87_status && test.abi.x87_depth == abi.x87_depth &&
            test.abi.floating_point->mxcsr == abi.floating_point->mxcsr &&
            test.abi.floating_point->x87_control_word == abi.floating_point->x87_control_word &&
            _mm_getcsr() == host_mxcsr, "declined movement changed ABI or host FP state");
}

void verify_success() {
    Case test;
    const auto input = test.abi;
    const SourceRuntimeWriteSerial serial(test.memory);
    const auto host_mxcsr = _mm_getcsr();
    require(native_campaign_ai_vehicle_movement_scale_tu15_try(test.memory, actual, test.abi), "proved movement call did not run");
    require(serial.load() == 23, "an original stack store was combined or skipped");
    require(test.abi.gp == std::array<Word,8>{previous | 0x42, 0, 0, input.gp[3], sp + 32,
                                             input.gp[5], input.gp[6], input.gp[7]}, "returned GP/RET 1C differs");
    const std::array<std::array<Word,4>,8> expected_xmm{{
        {one, 0, 0, 0}, {one, 0, 0, 0}, {one, 0, 0, 0}, {},
        {two, 0, 0, 0}, {}, {one, 0, 0, 0}, {one, 0, 0, 0}}};
    require(test.abi.xmm == expected_xmm && test.abi.eflags == 0x216 && test.abi.fs_base == input.fs_base,
            "returned SSE state, flags, or FS differs");
    require(test.abi.x87_status == 0x3800 && test.abi.x87_depth == 0 &&
            test.abi.floating_point->x87_control_word == 0x027f && test.abi.floating_point->mxcsr == 0x1f80 &&
            _mm_getcsr() == host_mxcsr, "x87 TOP, source controls or host MXCSR differs");
    require(load(test.memory, sp - 0x44) == actor && load(test.memory, sp - 0x40) == vehicle &&
            load(test.memory, sp - 0x3c) == input.gp[3] && load(test.memory, sp - 0x38) == objects &&
            load(test.memory, sp - 0x34) == 0x20216e && load(test.memory, sp-0x30) == 0xe5610004,
            "genuine property child did not leave its original frame");
    require(load(test.memory, sp) == 0x2a46bf && load(test.memory, sp + 16) == two &&
            load(test.memory, sp + 24) == bits(3.0f) && load(test.memory, sp + 28) == one &&
            load(test.memory, sp - 0x48) == 0xbdbdbdbd && load(test.memory, sp + 32) == 0xbdbdbdbd,
            "original local stores or adjacent stack bytes differ");
}

void verify_displacement_scaling() {
    Case test;
    store(test.memory, previous, one);
    store(test.memory, property + 0x54, bits(0.5f));
    const SourceRuntimeWriteSerial serial(test.memory);
    require(native_campaign_ai_vehicle_movement_scale_tu15_try(test.memory, actual, test.abi),
            "close-displacement movement call did not run");
    require(serial.load() == 24 && load(test.memory, sp + 24) == one &&
            load(test.memory, sp + 28) == bits(0.75f),
            "close-displacement adjustment did not preserve its original stack stores");
    require(test.abi.xmm[6][0] == bits(0.75f) && test.abi.xmm[0][0] == bits(0.75f),
            "close-displacement speed scale differs");
}

void verify_refusals() {
    Case test;
    verify_decline(test, {actual.instruction+1, actual.target, false});
    verify_decline(test, actual, [](const auto &) { throw std::runtime_error("unexpected callback"); });
    unsigned callbacks = 0;
    source_runtime_register_write_guard(test.memory, &callbacks, [&](auto, auto) { ++callbacks; });
    verify_decline(test);
    source_runtime_unregister_write_guard(test.memory, &callbacks);
    source_runtime_register_write_observer(test.memory, &callbacks, [&](auto, auto) { ++callbacks; });
    verify_decline(test);
    source_runtime_unregister_write_observer(test.memory, &callbacks);
    require(callbacks == 0, "lease invoked a write guard or observer while declining");

    for (const auto &[address, value] : std::array<std::pair<Word,Word>,6>{{
          {sp, 0x2a46c0}, {sp+20, direction}, {sp+28, 1}, {actor+0x4ae, 1},
          {actor+0x5b0, 7}, {actor+0x54, 0xffffffff}}}) {
        const auto save = load(test.memory, address);
        store(test.memory, address, value);
        verify_decline(test);
        store(test.memory, address, save);
    }
    auto control = *test.abi.floating_point;
    test.abi.floating_point->mxcsr &= ~0x80u;
    verify_decline(test);
    test.abi.floating_point = control;
    test.abi.x87_depth = 1;
    verify_decline(test);
}
} // namespace

int main() try {
    verify_success();
    verify_displacement_scaling();
    verify_refusals();
    return 0;
} catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
}
