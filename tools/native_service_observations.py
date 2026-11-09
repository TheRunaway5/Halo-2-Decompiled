"""Audited native observation-site semantics, never runtime count rewrites.

The complete canonical source hash must match. Unknown or changed sources stay
unclassified. A match describes the literal sites, not native dependency closure.
"""
NATIVE_SERVICE_OBSERVATIONS={
    ('src/platform/native_tu15_millisecond_clock.hpp','run_admitted'):
        'b2f42e0586cfb5aef4eba5a822a6a43c0b39bc33c0872f0d84ffc9f1d29a637a',
}


def native_service_observation(source,canonical_sha256,symbol):
    return NATIVE_SERVICE_OBSERVATIONS.get((source,symbol))==canonical_sha256
