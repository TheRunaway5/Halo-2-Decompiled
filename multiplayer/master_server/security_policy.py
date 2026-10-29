"""Typed service adapter for the Rust security policy library. No Python fallback."""
import ctypes
from functools import lru_cache
import hashlib
import os
from pathlib import Path
import struct
import weakref
from .protocol import require


@lru_cache(maxsize=1)
def library():
    path = Path(os.environ.get('H2_SECURITY_LIBRARY', Path(__file__).with_name('libhalo2_online_security_ffi.so')))
    require(path.is_absolute() and path.is_file(), 'Rust security library is not installed', 503)
    lib = ctypes.CDLL(str(path))
    lib.h2_security_abi.argtypes = []
    lib.h2_security_abi.restype = ctypes.c_uint32
    require(lib.h2_security_abi() == 2, 'Rust security library ABI mismatch', 503)
    signatures = {
        'h2_security_abi': ([], ctypes.c_uint32),
        'h2_player_capacity': ([], ctypes.c_uint64),
        'h2_listing_admission': ([ctypes.c_uint64]*3, ctypes.c_uint16),
        'h2_local_listing_admission': ([ctypes.c_uint64]*3, ctypes.c_uint16),
        'h2_eos_queue_new': ([], ctypes.c_uint64),
        'h2_eos_queue_drop': ([ctypes.c_uint64], None),
        'h2_eos_admit': ([ctypes.c_uint64]*5, ctypes.c_uint16),
        'h2_eos_release': ([ctypes.c_uint64]*5, None),
        'h2_eos_pending': ([ctypes.c_uint64], ctypes.c_uint64),
        'h2_result_projection': ([ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t], ctypes.c_bool),
    }
    for name, (arguments, result) in signatures.items():
        function = getattr(lib, name); function.argtypes = arguments; function.restype = result
    return lib


def player_capacity():
    return library().h2_player_capacity()


def listing_admission(origin, foreign, total):
    status = library().h2_listing_admission(origin, foreign, total)
    require(not status, 'Origin or federation listing capacity reached', status or 503)


def local_listing_admission(owner, local, total):
    status = library().h2_local_listing_admission(owner, local, total)
    require(not status, 'Owner or local listing capacity reached', status or 503)


def persistable_result(raw, *, accuracy_verified=False):
    source = ctypes.create_string_buffer(raw)
    result = ctypes.create_string_buffer(len(raw))
    lib=library()
    projection=lib.h2_result_projection
    if accuracy_verified:
        require(hasattr(lib,'h2_result_accuracy_projection'),'Accuracy projection requires a Rust library update',503)
        projection=lib.h2_result_accuracy_projection
        projection.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.c_size_t]
        projection.restype=ctypes.c_bool
    require(projection(source, result, len(raw)), 'Invalid persistable result')
    return result.raw


class EosAdmissions:
    def __init__(self):
        self.lib = library(); self.id = self.lib.h2_eos_queue_new()
        require(self.id != 0, 'EOS admission unavailable', 503)
        self.finalizer = weakref.finalize(self, self.lib.h2_eos_queue_drop, self.id)

    @staticmethod
    def key(principal):
        return struct.unpack('<4Q', hashlib.sha256(principal.encode()).digest())

    def admit(self, principal):
        key = self.key(principal)
        status = self.lib.h2_eos_admit(self.id, *key)
        require(not status, 'EOS verification busy for player; retry', status or 503)
        return key

    def release(self, key):
        self.lib.h2_eos_release(self.id, *key)

    @property
    def pending(self):
        return self.lib.h2_eos_pending(self.id)
