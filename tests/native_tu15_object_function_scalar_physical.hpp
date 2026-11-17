#pragma once
#include "gameplay/objects/native_tu15_object_function_scalar_frame.hpp"
#include "physics/runtime/native_tu15_host_fp_io.hpp"
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"

#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
namespace halo2::native_object_function_scalar_detail {
struct HostIo {
    template<class Function> void operator()(Function&& function) const {
        native_host_fp_io_detail::preserve(std::forward<Function>(function));
    }
};

template<class State, class Before> class PhysicalIo :
    private native_mopp_ray_caller_detail::Frame<State, Before, HostIo> {
    using Base = native_mopp_ray_caller_detail::Frame<State, Before, HostIo>;
public:
    PhysicalIo(State& state, Before& before) : Base{state, before, HostIo{}} {}
    void at(State&, std::uint32_t pc) { Base::at(pc); }
    std::uint32_t read(State&, std::uint32_t address, unsigned width) { return Base::word(address, width); }
    void write(State& state, std::uint32_t pc, std::uint32_t address, std::uint32_t value, unsigned width) {
        if (width == 4) {
            Base::store(pc, address, value, []{}, true);
            return;
        }
        SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if (state.observe) {
            event.before.resize(1);
            Base::io([&] { state.memory.read(address, event.before); });
        }
        const std::array data{std::byte(value)};
        Base::io([&] { state.memory.write(address, data); });
        if (state.observe) {
            event.written.assign(data.begin(), data.end());
            Base::io([&] { state.observe(event); });
        }
    }
    void push(State& state, std::uint32_t pc, std::uint32_t value) {
        const auto address = state.gp[4]-4u;
        Base::store(pc,address,value,[&] { state.gp[4]=address; },true);
    }
    std::uint32_t pop(State& state) {
        const auto value = Base::word(state.gp[4]);
        state.gp[4] += 4u;
        return value;
    }
    template<unsigned Count,bool Left> std::uint32_t shift(State& state, std::uint32_t value) {
        return native_object_function_scalar_ordered_detail::shift_for_image<Count,Left>(state,value);
    }
};

template<class State, class Routes, class Before> void run_physical(State& state, Routes routes, Before before) {
    PhysicalIo io{state, before};
    Frame frame{state, io, routes};
    evaluate(frame);
}
} // namespace halo2::native_object_function_scalar_detail
#endif
