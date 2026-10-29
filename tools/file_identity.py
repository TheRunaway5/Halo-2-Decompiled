"""Comparable descriptor/path identities for read-only owner admission.

Windows CRT stat and path stat can report different meanings for ctime. Both
Windows entry points below query the same kernel file ID, size, last-write and
change timestamps. POSIX retains its original dev/inode/size/mtime/ctime tuple.
"""
import os


def stat_identity(value):
    return (value.st_dev, value.st_ino, value.st_size,
            value.st_mtime_ns, value.st_ctime_ns)


if os.name == "nt":
    import ctypes
    from ctypes import wintypes
    import msvcrt

    class _FileInformation(ctypes.Structure):
        _fields_ = (("attributes", wintypes.DWORD), ("creation", wintypes.FILETIME),
                    ("access", wintypes.FILETIME), ("write", wintypes.FILETIME),
                    ("volume", wintypes.DWORD), ("size_high", wintypes.DWORD),
                    ("size_low", wintypes.DWORD), ("links", wintypes.DWORD),
                    ("index_high", wintypes.DWORD), ("index_low", wintypes.DWORD))

    class _BasicInformation(ctypes.Structure):
        _fields_ = (("creation", ctypes.c_longlong), ("access", ctypes.c_longlong),
                    ("write", ctypes.c_longlong), ("change", ctypes.c_longlong),
                    ("attributes", wintypes.DWORD))

    _kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    _information = _kernel.GetFileInformationByHandle
    _information.argtypes = (wintypes.HANDLE, ctypes.POINTER(_FileInformation))
    _information.restype = wintypes.BOOL
    _basic = _kernel.GetFileInformationByHandleEx
    _basic.argtypes = (wintypes.HANDLE, ctypes.c_int, ctypes.c_void_p, wintypes.DWORD)
    _basic.restype = wintypes.BOOL
    _open = _kernel.CreateFileW
    _open.argtypes = (wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD, ctypes.c_void_p,
                      wintypes.DWORD, wintypes.DWORD, wintypes.HANDLE)
    _open.restype = wintypes.HANDLE
    _close = _kernel.CloseHandle
    _close.argtypes = (wintypes.HANDLE,)
    _close.restype = wintypes.BOOL

    def _handle_identity(handle):
        information, basic = _FileInformation(), _BasicInformation()
        if not _information(handle, ctypes.byref(information)):
            raise ctypes.WinError(ctypes.get_last_error())
        if not _basic(handle, 0, ctypes.byref(basic), ctypes.sizeof(basic)):
            raise ctypes.WinError(ctypes.get_last_error())
        file_id = information.index_high << 32 | information.index_low
        if not file_id:
            raise ValueError("File owner has no kernel file identity")
        epoch = 116444736000000000  # FILETIME ticks before the Unix epoch.
        return (information.volume, file_id,
                information.size_high << 32 | information.size_low,
                (basic.write - epoch) * 100, (basic.change - epoch) * 100)


def descriptor_identity(fd):
    if os.name != "nt":
        return stat_identity(os.fstat(fd))
    return _handle_identity(msvcrt.get_osfhandle(fd))


def path_identity(path):
    if os.name != "nt":
        return stat_identity(path.stat())
    # Query the final named owner itself, rather than following a replacement
    # reparse point. Sharing permits inspection without denying an independent
    # writer/replacer; the caller's before/after identity guards detect those.
    handle = _open(str(path), 0x80, 7, None, 3, 0x02200000, None)
    if handle == ctypes.c_void_p(-1).value:
        raise ctypes.WinError(ctypes.get_last_error())
    try:
        return _handle_identity(handle)
    finally:
        if not _close(handle):
            raise ctypes.WinError(ctypes.get_last_error())
