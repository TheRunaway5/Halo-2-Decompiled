"""Bounded reads from the setup child's owned progress and cancellation pipes."""
import os
import select
import time

if os.name == "nt":
    import ctypes
    from ctypes import wintypes
    import msvcrt

    _peek = ctypes.WinDLL("kernel32", use_last_error=True).PeekNamedPipe
    _peek.argtypes = (wintypes.HANDLE, ctypes.c_void_p, wintypes.DWORD,
                      ctypes.POINTER(wintypes.DWORD), ctypes.POINTER(wintypes.DWORD),
                      ctypes.POINTER(wintypes.DWORD))
    _peek.restype = wintypes.BOOL


def read_available(fd, maximum, timeout):
    """Return bytes, EOF, or None when the bounded wait expires.

    Windows select accepts sockets only. Peek the anonymous pipe before each
    read instead; at most the advertised byte count is read by its sole owner.
    No reader can remain blocked when the GUI needs to retire its thread.
    """
    if os.name != "nt":
        return os.read(fd, maximum) if select.select([fd], [], [], timeout)[0] else None
    deadline = time.monotonic() + timeout
    handle = msvcrt.get_osfhandle(fd)
    while True:
        available = wintypes.DWORD()
        if not _peek(handle, None, 0, None, ctypes.byref(available), None):
            error = ctypes.get_last_error()
            if error in (109, 232, 233):  # Broken, closing, or disconnected pipe.
                return b""
            raise ctypes.WinError(error)
        if available.value:
            return os.read(fd, min(maximum, available.value))
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            return None
        time.sleep(min(.01, remaining))
