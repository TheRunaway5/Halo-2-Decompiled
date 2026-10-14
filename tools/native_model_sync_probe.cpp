#include "halo2/native_synchronization.hpp"

// Diagnostic ABI transport for the model-builder original-code comparison.
// Recreate the two originally unlocked queue mutexes in123450. Every acquire
// and release goes through the real native synchronization implementation.
// No resource request, lighting sample or GPU completion is provided here.
extern "C" {
void* model_sync_create() {
    try {
        auto* owner = new halo2::NativeSynchronization({0xf3000001u, 0xf3000100ull});
        for (unsigned i = 0; i < 2; ++i) {
            const auto result = owner->services().create_mutex(false);
            if (result.error || result.handle != 0xf3000001u + i) {
                delete owner;
                return nullptr;
            }
        }
        return owner;
    } catch (...) { return nullptr; }
}
void model_sync_destroy(void* owner) {
    delete static_cast<halo2::NativeSynchronization*>(owner);
}
int model_sync_acquire(void* owner, unsigned handle) {
    try {
        const auto result = static_cast<halo2::NativeSynchronization*>(owner)->services().wait(handle, 0);
        return result.error ? -1 : static_cast<int>(result.state);
    } catch (...) { return -1; }
}
int model_sync_release(void* owner, unsigned handle) {
    try {
        return static_cast<halo2::NativeSynchronization*>(owner)->services().release_mutex(handle).value();
    } catch (...) { return -1; }
}
}
