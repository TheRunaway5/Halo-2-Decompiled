#pragma once
#include <atomic>
#include <array>
#include <cstdint>
#include <type_traits>

namespace halo2 {enum class SourcePhysicsContactImage;}

namespace halo2::execution_accounting {
struct UnitIdentity {
    const char* source;
    const char* canonical_sha256;
    const char* input_sha256;
    const char* image_scope;
};
struct Statistics {
    std::atomic<std::uint64_t> entries{}, body_entries{}, statements{}, returns{}, unwinds{},
        returns_without_body{}, unwinds_without_body{}, active{};
};
struct Counter {
    const UnitIdentity* unit;
    const char* symbol;
    std::uint32_t literal_sites;
    std::uint32_t dynamic_forwarders;
    std::array<Statistics,3> images;
    Counter(const UnitIdentity*,const char*,std::uint32_t,std::uint32_t dynamic_forwarders=0) noexcept;
};
static_assert(std::is_trivially_destructible_v<Counter>);
template<class State> unsigned runtime_image(const State& state) noexcept {
    if constexpr(requires {state.image;}) {
        if constexpr(std::is_same_v<std::remove_cvref_t<decltype(state.image)>,SourcePhysicsContactImage>) {
            const auto value=static_cast<unsigned>(state.image);return value<2?value:2;
        }
    }
    return 2;
}
// Diagnostic activation ownership is independent of the runtime service table.
// A nested generated call owns a different local scope, including recursion.
class Scope {
    Statistics& counter_;
    Counter* owner_;
    Scope* previous_;
    int exceptions_;
    bool body_{};
    friend void record_literal(Counter&) noexcept;
public:
    explicit Scope(Counter&,unsigned image=2) noexcept;
    ~Scope() noexcept;
    void literal() noexcept;
    Scope(const Scope&)=delete;
    Scope& operator=(const Scope&)=delete;
};
// Static counters are accessible inside an unchanged lambda capture list.
// Lookup preserves source ownership across nested calls. A deferred callback
// with no live owning activation invalidates the diagnostic report explicitly.
void record_literal(Counter&) noexcept;
// A synchronized caller may request an intermediate snapshot. The normal
// diagnostic build also writes HALO2_NATIVE_EXECUTION_REPORT at process exit.
bool write_report(const char* path) noexcept;
}
