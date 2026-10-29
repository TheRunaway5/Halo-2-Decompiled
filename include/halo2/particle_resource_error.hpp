#pragma once
#include "halo2/cache.hpp"

namespace halo2 {
// An unavailable or unsupported visual resource can be omitted without
// changing native simulation. Device, allocation and native-state failures
// deliberately keep their original exception types.
struct ParticleResourceError : FormatError {
    using FormatError::FormatError;
};
}
