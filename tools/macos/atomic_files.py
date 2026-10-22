"""Darwin atomic publication that never replaces an existing destination."""
import ctypes
import os

# Public Darwin sys/stdio.h: renamex_np is available from macOS 10.12.
RENAME_EXCL = 0x00000004


def rename_exclusive(source, destination):
    library = ctypes.CDLL(None, use_errno=True)
    rename = getattr(library, 'renamex_np', None)
    if rename is None:
        raise OSError('Darwin exclusive rename is unavailable')
    rename.argtypes = (ctypes.c_char_p, ctypes.c_char_p, ctypes.c_uint)
    rename.restype = ctypes.c_int
    if rename(os.fsencode(source), os.fsencode(destination), RENAME_EXCL) != 0:
        code = ctypes.get_errno()
        raise OSError(code, os.strerror(code), str(destination))
