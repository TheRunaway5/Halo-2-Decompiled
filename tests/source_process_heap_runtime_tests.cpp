#include "halo2/source_process_heap_platform.hpp"
#include "halo2/source_runtime_host_virtual_memory.hpp"
#include "halo2/source_runtime_image.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace {
using J = nlohmann::json;
using M = halo2::SourceRuntimeMemory;
void check(bool value, const std::string &why) {
  if (!value) throw std::runtime_error(why);
}
std::vector<std::byte> bytes(const std::string &text) {
  check(!(text.size() & 1u), "Invalid original fixture byte encoding");
  std::vector<std::byte> out;
  for (std::size_t i = 0; i < text.size(); i += 2)
    out.push_back(std::byte(std::stoul(text.substr(i, 2), nullptr, 16)));
  return out;
}
std::vector<std::byte> file(const std::filesystem::path &path) {
  std::ifstream in(path, std::ios::binary | std::ios::ate);
  check(bool(in), "Missing retained original bytes: " + path.string());
  const auto size = in.tellg(); check(size >= 0, "Invalid retained byte extent");
  std::vector<std::byte> result(static_cast<std::size_t>(size)); in.seekg(0);
  check(bool(in.read(reinterpret_cast<char *>(result.data()), result.size())),
        "Truncated retained original bytes");
  return result;
}
std::uint32_t word(const M &memory, std::uint32_t address) {
  const auto b = memory.view(address, 4); std::uint32_t value = 0;
  for (unsigned i = 0; i < 4; ++i) value |= std::to_integer<unsigned>(b[i]) << (8*i);
  return value;
}
void put(M &memory, std::uint32_t address, std::uint32_t value) {
  std::array<std::byte, 4> b;
  for (unsigned i = 0; i < 4; ++i) b[i] = std::byte(value >> (8*i));
  memory.write(address, b);
}
void same(std::span<const std::byte> actual, std::span<const std::byte> expected,
          const std::string &why) {
  check(std::ranges::equal(actual, expected), why);
}
struct Boundary {};
}
int main(int argc, char **argv) {
  try {
    check(argc == 3, "Supply unchanged-original fixture and TU1.1 image");
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) return 77;
#if !defined(__linux__)
    return 77;
#endif
    std::ifstream in(argv[1]); const auto fixture = J::parse(in);
    const auto directory = std::filesystem::path(argv[1]).parent_path();
    const auto stack = fixture.at("stack_address").get<std::uint32_t>();
    const auto sp = fixture.at("stack_pointer").get<std::uint32_t>();
    const auto data = fixture.at("data_address").get<std::uint32_t>();
    const bool initializer = fixture.contains("compiled_compatibility_initializer") &&
        !fixture.at("compiled_compatibility_initializer").is_null();
    std::size_t cases = 0, stores_count = 0, resource_count = 0, retained_bytes = 0, lock_count = 0;
    for (const auto &row : fixture.at("cases")) {
      const auto name = row.at("name").get<std::string>();
      auto memory = halo2::load_source_runtime_image_1_1(argv[2]);
      if (row.contains("initial_xapi_flag") && !row.at("initial_xapi_flag").is_null()) {
        const std::array flag{std::byte(row.at("initial_xapi_flag").get<unsigned>())};
        memory.write(0x566ef1, flag);
      }
      memory.mount(stack, std::vector<std::byte>(0x10000), M::Access::ReadWrite,
                     "explicit standalone stack input");
      memory.mount(0, std::vector<std::byte>(0x1000), M::Access::ReadWrite,
                     "explicit standalone FS exception-chain input");
      put(memory, 0, 0xffffffff);
      memory.mount(data, file(directory / row.at("initial_data").get<std::string>()),
                     M::Access::ReadWrite, "explicit standalone parameter/copy inputs");
      put(memory, sp, 0x3000000);
      const auto args = row.at("args").get<std::vector<std::uint32_t>>();
      for (unsigned i = 0; i < args.size(); ++i) put(memory, sp+4+i*4, args[i]);
      halo2::SourceRuntimeHostVirtualMemory vm(memory,
          fixture.at("raw_address").get<std::uint32_t>());
      halo2::SourcePhysicsContactRuntimeAbi abi{};
      abi.gp = row.at("before").at("gp").get<std::array<std::uint32_t, 8>>();
      abi.eflags = row.at("before").at("eflags");
      abi.floating_point = halo2::SourcePhysicsFactoryFloatingPoint{0x27f, 0x1f80};
      halo2::SourcePhysicsContactRuntimeServices services;
      std::size_t resource = 0, boundary = 0, store = 0;
      const auto resource_observe = [&](const halo2::SourceProcessHeapMemoryCall &actual) {
        check(resource < row.at("resources").size(), name + " unexpected resource call");
        const auto &expected = row.at("resources").at(resource++);
        const auto &request = actual.request;
        const auto &result = actual.result;
        check(actual.instruction == expected.at("instruction") &&
                  std::array{actual.base_cell, actual.size_cell} ==
                    expected.at("cells").get<std::array<std::uint32_t, 2>>() &&
                  std::array{request.base, request.zero_bits, request.bytes, request.allocation_type,
                             request.protection} == expected.at("request").get<std::array<std::uint32_t, 5>>(),
              name + " original resource inputs differ");
        check(std::array{result.status, result.base, result.bytes} ==
                  expected.at("result").get<std::array<std::uint32_t, 3>>(),
              name + " actual native resource adaptation differs");
        if (!expected.at("initial_storage").is_null())
          same(memory.view(result.base, result.bytes),
               file(directory / expected.at("initial_storage").get<std::string>()),
               name + " actual committed initial bytes differ");
      };
      services.required_owner = [&](auto &, const auto &call, auto &frame, const auto &) {
        if (!initializer && halo2::source_try_native_process_heap_memory_call(memory, call, frame,
                halo2::SourcePhysicsContactImage::MapPack11, vm, resource_observe)) return;
        check(boundary < row.at("boundary").size(), name + " unexpected required owner");
        const auto &expected = row.at("boundary").at(boundary++);
        check(call.instruction == expected.at("instruction") &&
                  call.target == expected.at("target") && call.tail == expected.at("tail"),
              name + " required owner differs");
        throw Boundary{};
      };
      std::vector<J> expected_lock_stores;
      if (initializer) for (const auto &call : row.at("platform_calls")) {
        check(call.at("instruction") == 0x2dabb6 && call.at("target") == 0x80000123 &&
                  !call.at("tail").get<bool>() && call.at("return") == 0x2dabbc,
              name + " initializer call owner differs");
        for (const auto &change : call.at("stores")) expected_lock_stores.push_back(change);
      }
      std::size_t lock_store = 0;
      const auto lock_observe = [&](const halo2::PhysicsPlatformLockStore &actual) {
        check(lock_store < expected_lock_stores.size(), name + " extra native initializer store");
        const auto &expected = expected_lock_stores.at(lock_store++);
        check(actual.address == expected.at("address") && actual.before == bytes(expected.at("before")) &&
                  actual.written == bytes(expected.at("written")),
              name + " compiled primary initializer store differs at index " + std::to_string(lock_store-1));
      };
      halo2::PhysicsPlatformLocks locks;
      const halo2::PhysicsPlatformLockServices platform{
          [&]() -> std::uint32_t { throw std::runtime_error(name + " initializer requested thread identity"); },
          [&](auto) { throw std::runtime_error(name + " initializer requested wait"); },
          [&](auto) { throw std::runtime_error(name + " initializer requested wake"); }};
      const auto observe = [&](const halo2::SourcePhysicsContactRecordStore &actual) {
        check(store < row.at("stores").size(), name + " extra source store");
        const auto &expected = row.at("stores").at(store++);
        check(actual.instruction == expected.at("instruction") && actual.address == expected.at("address") &&
                  actual.before == bytes(expected.at("before")) && actual.written == bytes(expected.at("written")),
              name + " source store differs at index " + std::to_string(store-1));
      };
      bool stopped = false;
      try {
        if (initializer) halo2::source_run_process_heap_with_native_platform(memory, row.at("entry"), abi,
            halo2::SourcePhysicsContactImage::MapPack11, {0x27f, 0x1f80}, services, vm, locks, platform,
            observe, resource_observe, lock_observe);
        else halo2::source_run_process_heap_runtime(memory, row.at("entry"), abi,
            halo2::SourcePhysicsContactImage::MapPack11, {0x27f, 0x1f80}, services, observe);
      } catch (const Boundary &) { stopped = true; }
      check(stopped == !row.at("boundary").empty() && boundary == row.at("boundary").size(),
            name + " boundary delivery differs");
      check(resource == row.at("resources").size() && store == row.at("stores").size(),
            name + " missing resource/store operations");
      check(lock_store == expected_lock_stores.size(), name + " missing native initializer stores");
      const auto gp = row.at("after").at("gp").get<std::array<std::uint32_t, 8>>();
      for (unsigned i = 0; i < 8; ++i)
        check(abi.gp[i] == gp[i], name + " GP differs at index " + std::to_string(i));
      check(((abi.eflags ^ row.at("after").at("eflags").get<std::uint32_t>()) & 0xcd5u) == 0,
            name + " arithmetic/direction flags differ");
      check(abi.floating_point && abi.floating_point->mxcsr == row.at("after").at("mxcsr") &&
                abi.floating_point->x87_control_word == 0x27f && abi.x87_depth == 0,
            name + " floating-point context differs");
      for (const auto &bank : row.at("heap")) {
        const auto expected = file(directory / bank.at("storage").get<std::string>());
        same(memory.view(bank.at("address"), expected.size()), expected, name + " retained source heap differs");
        retained_bytes += expected.size();
      }
      same(memory.view(data, 0x1000), file(directory / row.at("final_data").get<std::string>()),
           name + " retained copy/parameter bank differs");
      same(memory.view(sp-0x200, 0x300), file(directory / row.at("final_stack").get<std::string>()),
           name + " retained caller/SEH stack differs");
      same(memory.view(0, 4), bytes(row.at("final_fs")), name + " retained FS exception chain differs");
      // The loaded image process-heap cell stays untouched; this standalone
      // owner never substitutes its result for the missing XAPI startup caller.
      check(word(memory, 0x566f08) == 0, name + " installed a process heap outside its owner");
      ++cases; stores_count += store; resource_count += resource; lock_count += lock_store;
    }
    std::cout << cases << " original cases, " << stores_count << " ordered source stores, "
              << resource_count << " actual reserve/commit calls, " << retained_bytes
              << " retained committed heap bytes, " << lock_count
              << " compiled primary initializer stores and ABI/SEH/copy state match\n";
  } catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
