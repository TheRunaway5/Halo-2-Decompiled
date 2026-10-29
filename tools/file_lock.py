"""Cross-process shared/exclusive locks for an owned installation lock file."""
import contextlib
import errno
import os
import time

if os.name == "nt":
    import ctypes
    from ctypes import wintypes
    import msvcrt

    class _Overlapped(ctypes.Structure):
        _fields_ = (("Internal", ctypes.c_size_t), ("InternalHigh", ctypes.c_size_t),
                    ("Offset", wintypes.DWORD), ("OffsetHigh", wintypes.DWORD),
                    ("hEvent", wintypes.HANDLE))

    _kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    _lock = _kernel.LockFileEx
    _lock.argtypes = (wintypes.HANDLE, wintypes.DWORD, wintypes.DWORD,
                      wintypes.DWORD, wintypes.DWORD, ctypes.POINTER(_Overlapped))
    _lock.restype = wintypes.BOOL
    _unlock = _kernel.UnlockFileEx
    _unlock.argtypes = (wintypes.HANDLE, wintypes.DWORD, wintypes.DWORD,
                        wintypes.DWORD, ctypes.POINTER(_Overlapped))
    _unlock.restype = wintypes.BOOL


@contextlib.contextmanager
def file_lock(fd, *, shared=False, blocking=True):
    """Lock the whole file without changing the descriptor's seek position.

    Windows blocking acquisition polls LockFileEx's immediate mode so Python
    cancellation can unwind the setup journal while another importer owns the
    lock. The kernel still arbitrates every acquisition across processes.
    """
    if os.name != "nt":
        import fcntl
        mode = fcntl.LOCK_SH if shared else fcntl.LOCK_EX
        fcntl.flock(fd, mode | (0 if blocking else fcntl.LOCK_NB))
        try:
            yield
        finally:
            fcntl.flock(fd, fcntl.LOCK_UN)
        return
    handle = msvcrt.get_osfhandle(fd)
    overlapped = _Overlapped()
    flags = 1 | (0 if shared else 2)  # FAIL_IMMEDIATELY, optional EXCLUSIVE_LOCK.
    while not _lock(handle, flags, 0, 0xffffffff, 0xffffffff, ctypes.byref(overlapped)):
        error = ctypes.get_last_error()
        if error != 33:  # ERROR_LOCK_VIOLATION.
            raise ctypes.WinError(error)
        if not blocking:
            raise BlockingIOError(errno.EAGAIN, "Installation lock is owned by another process")
        time.sleep(.05)
    try:
        yield
    finally:
        if not _unlock(handle, 0, 0xffffffff, 0xffffffff, ctypes.byref(overlapped)):
            raise ctypes.WinError(ctypes.get_last_error())
