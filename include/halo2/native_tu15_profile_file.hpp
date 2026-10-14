#pragma once
#include "halo2/source_runtime_memory.hpp"
#include "halo2/physics_world_runtime.hpp"
#include <filesystem>
#include <thread>
#include <optional>
namespace halo2 {
struct NativeTu15ProfileFileProbe {
    std::uint32_t instruction,entry_esp,return_esp,attributes,error;
    std::string logical_path;
    std::filesystem::path native_path;
};
struct NativeTu15DirectoryCreation {
    std::uint32_t instruction,entry_esp,return_esp,result,error;
    std::string logical_path;
    std::filesystem::path native_path;
};
// The fresh native session probes its own private player configuration file.
// Actual absence maps to the recovered caller's FILE_NOT_FOUND path. Reading
// an existing source-format profile requires a separate owner and stops here.
class NativeTu15ProfileFile final {
public:
    NativeTu15ProfileFile(SourceRuntimeMemory&,std::filesystem::path private_directory);
    bool try_call(SourcePhysicsContactMemory&,const SourcePhysicsContactRuntimeCall&,SourcePhysicsWorldRuntimeAbi&);
    const std::optional<NativeTu15ProfileFileProbe>& probe()const noexcept{return probe_;}
    const std::optional<NativeTu15ProfileFileProbe>& directory_probe()const noexcept{return directory_probe_;}
    const std::optional<NativeTu15DirectoryCreation>& directory_creation()const noexcept{return directory_creation_;}
    std::size_t directory_creations()const noexcept{return directory_creations_;}
private:
    SourceRuntimeMemory& memory_;
    std::filesystem::path directory_;
    std::thread::id thread_;
    std::optional<NativeTu15ProfileFileProbe> probe_;
    std::optional<NativeTu15ProfileFileProbe> directory_probe_;
    std::optional<NativeTu15DirectoryCreation> directory_creation_;
    std::optional<std::uint32_t> last_error_;
    std::size_t directory_creations_=0;
};
}
