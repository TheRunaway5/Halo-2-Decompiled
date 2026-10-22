#include "halo2/macos/executable.hpp"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <mach/machine.h>
#include <spawn.h>
#include <stdexcept>
#include <string>
#include <system_error>

extern char** environ;

namespace {
void environment(const char* name, const std::filesystem::path& value) {
    if (setenv(name, value.c_str(), 1))
        throw std::system_error(errno, std::generic_category(), name);
}
}

int main(int, char** argv) {
    try {
        const auto binary = halo2::macos_executable_path();
        const auto root = binary.parent_path().parent_path();
        const auto real = binary.parent_path() / "python3.real";
        if (!std::filesystem::is_regular_file(real))
            throw std::runtime_error("The bundled Python interpreter is missing");
        environment("PYTHONHOME", root);
        environment("TCL_LIBRARY", root / HALO2_TCL_LIBRARY);
        environment("TK_LIBRARY", root / HALO2_TK_LIBRARY);
        environment("SSL_CERT_FILE", root / "lib/python3.14/site-packages/certifi/cacert.pem");
        environment("PYTHONNOUSERSITE", "1");
        environment("PYTHONDONTWRITEBYTECODE", "1");
        // The runtime owns its imports. User and build-host Python settings
        // must not redirect setup into another installation.
        for (const char* name : {"PYTHONPATH", "PYTHONUSERBASE", "__PYVENV_LAUNCHER__"})
            if (unsetenv(name)) throw std::system_error(errno, std::generic_category(), name);
        argv[0] = const_cast<char*>(real.c_str());
        posix_spawnattr_t attributes;
        int error = posix_spawnattr_init(&attributes);
        if (error) throw std::system_error(error, std::generic_category(), "posix_spawnattr_init");
#if defined(__arm64__)
        cpu_type_t architecture = CPU_TYPE_ARM64;
#elif defined(__x86_64__)
        cpu_type_t architecture = CPU_TYPE_X86_64;
#else
#error Unsupported macOS Python launcher architecture
#endif
        std::size_t count = 0;
        if (posix_spawnattr_setbinpref_np(&attributes, 1, &architecture, &count) || count != 1)
            error = errno ? errno : EINVAL;
        if (!error) error = posix_spawnattr_setflags(&attributes, POSIX_SPAWN_SETEXEC);
        pid_t process = 0;
        if (!error) error = posix_spawn(&process, real.c_str(), nullptr, &attributes, argv, environ);
        posix_spawnattr_destroy(&attributes);
        throw std::system_error(error ? error : EIO, std::generic_category(), "Start bundled Python");
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Halo 2 Python runtime: %s\n", error.what());
        return 1;
    }
}
