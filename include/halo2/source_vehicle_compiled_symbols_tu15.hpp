#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
namespace halo2 {
class SourcePhysicsContactMemory;
struct SourceVehicleCompiledSymbolBindingTu15;
// Lease for the two compiled netgame variant names. Authored MCC identifiers
// and metadata retain their own namespace. Only three original literal sites
// request adaptation, after validating the immutable Cache's exact names.
class SourceVehicleCompiledSymbolsTu15 {
public:
    using Name=std::function<std::string(std::uint32_t)>;
    SourceVehicleCompiledSymbolsTu15(const SourcePhysicsContactMemory &,bool,const Name &);
    ~SourceVehicleCompiledSymbolsTu15();
    SourceVehicleCompiledSymbolsTu15(const SourceVehicleCompiledSymbolsTu15 &)=delete;
    SourceVehicleCompiledSymbolsTu15 &operator=(const SourceVehicleCompiledSymbolsTu15 &)=delete;
private:
    std::shared_ptr<const SourceVehicleCompiledSymbolBindingTu15> binding_;
};
std::uint32_t source_vehicle_compiled_symbol_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t instruction,std::uint32_t original);
}
