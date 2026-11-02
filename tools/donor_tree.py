"""No-follow donor ownership shared by inventory and directory importers.

POSIX lookups are relative to retained directory descriptors. Windows opens
every component as a reparse point and holds non-delete-shared directory
handles, so pathname enumeration cannot be redirected by a renamed ancestor.
Files are read through the same admitted handle, never reopened by pathname.
"""
from contextlib import contextmanager, ExitStack
from functools import lru_cache
import os
from pathlib import Path
import stat


class DonorError(ValueError):
    pass


def admit(info, directory=None):
    if stat.S_ISLNK(info.st_mode) or getattr(info, 'st_file_attributes', 0) & 0x400:
        raise DonorError('Symlink or reparse donor entry')
    if not (stat.S_ISDIR(info.st_mode) or stat.S_ISREG(info.st_mode)):
        raise DonorError('Symlink or nonregular donor entry')
    if directory is not None and stat.S_ISDIR(info.st_mode) != directory:
        raise DonorError('Donor entry type changed')
    return info


def identity(info):
    return info.st_dev, info.st_ino, info.st_size, info.st_mtime_ns, info.st_ctime_ns


def parts(relative):
    path = Path(relative)
    if path.is_absolute() or not path.parts or any(p in ('.', '..') for p in path.parts):
        raise DonorError('Unsafe donor relative path')
    if os.name == 'nt' and any(':' in p or '\\' in p for p in path.parts):
        raise DonorError('Unsafe donor relative path')
    return path.parts


@lru_cache(maxsize=1)
def windows():
    return WindowsApi()


class WindowsApi:
    def __init__(self):
        import ctypes as c
        from ctypes import wintypes as w
        self.c, self.w = c, w
        self.kernel = c.WinDLL('kernel32', use_last_error=True)
        self.security = c.WinDLL('advapi32', use_last_error=True)
        self.kernel.CreateFileW.argtypes = [w.LPCWSTR, w.DWORD, w.DWORD, c.c_void_p, w.DWORD, w.DWORD, w.HANDLE]
        self.kernel.CreateFileW.restype = w.HANDLE
        self.kernel.CloseHandle.argtypes = [w.HANDLE]
        self.kernel.CloseHandle.restype = w.BOOL
        self.kernel.GetFileInformationByHandleEx.argtypes = [w.HANDLE, c.c_int, c.c_void_p, w.DWORD]
        self.kernel.GetFileInformationByHandleEx.restype = w.BOOL
        self.kernel.GetModuleHandleW.argtypes = [w.LPCWSTR]
        self.kernel.GetModuleHandleW.restype = w.HMODULE
        self.kernel.GetProcAddress.argtypes = [w.HMODULE, c.c_char_p]
        self.kernel.GetProcAddress.restype = c.c_void_p
        self.kernel.DeviceIoControl.argtypes = [w.HANDLE, w.DWORD, c.c_void_p, w.DWORD,
                                               c.c_void_p, w.DWORD, c.POINTER(w.DWORD), c.c_void_p]
        self.kernel.DeviceIoControl.restype = w.BOOL
        self.kernel.LocalFree.argtypes = [c.c_void_p]
        self.kernel.LocalFree.restype = c.c_void_p
        self.security.ConvertStringSecurityDescriptorToSecurityDescriptorW.argtypes = [w.LPCWSTR, w.DWORD, c.POINTER(c.c_void_p), c.c_void_p]
        self.security.ConvertStringSecurityDescriptorToSecurityDescriptorW.restype = w.BOOL
        self.kernel.GetCurrentProcess.argtypes = []
        self.kernel.GetCurrentProcess.restype = w.HANDLE
        self.security.OpenProcessToken.argtypes = [w.HANDLE, w.DWORD, c.POINTER(w.HANDLE)]
        self.security.OpenProcessToken.restype = w.BOOL
        self.security.GetTokenInformation.argtypes = [w.HANDLE, c.c_int, c.c_void_p, w.DWORD, c.POINTER(w.DWORD)]
        self.security.GetTokenInformation.restype = w.BOOL
        self.security.ConvertSidToStringSidW.argtypes = [c.c_void_p, c.POINTER(w.LPWSTR)]
        self.security.ConvertSidToStringSidW.restype = w.BOOL
        class Attributes(c.Structure):
            _fields_ = [('attributes', w.DWORD), ('reparse_tag', w.DWORD)]
        class Security(c.Structure):
            _fields_ = [('length', w.DWORD), ('descriptor', c.c_void_p), ('inherit', w.BOOL)]
        self.Attributes, self.Security = Attributes, Security

    def wine_host_mount(self, handle, info=None):
        """Recognize Wine's synthetic Unix mount metadata, never real links."""
        c, w = self.c, self.w
        ntdll = self.kernel.GetModuleHandleW('ntdll.dll')
        if not ntdll or not self.kernel.GetProcAddress(ntdll, b'wine_get_version'):
            return False
        if info is None:
            info = self.Attributes()
            if not self.kernel.GetFileInformationByHandleEx(handle, 9, c.byref(info), c.sizeof(info)):
                return False
        if info.reparse_tag != 0xa0000003:
            return False
        data, size = c.create_string_buffer(16384), w.DWORD()
        return (not self.kernel.DeviceIoControl(handle, 0x900a8, None, 0, data, len(data),
                                               c.byref(size), None) and c.get_last_error() == 4390)

    def private_sddl(self):
        c, w = self.c, self.w
        token, needed = w.HANDLE(), w.DWORD()
        if not self.security.OpenProcessToken(self.kernel.GetCurrentProcess(), 8, c.byref(token)):
            raise c.WinError(c.get_last_error())
        try:
            self.security.GetTokenInformation(token, 1, None, 0, c.byref(needed))
            if not 0 < needed.value <= 65536:
                raise DonorError('Cannot inspect importing Windows user')
            data = c.create_string_buffer(needed.value)
            if not self.security.GetTokenInformation(token, 1, data, len(data), c.byref(needed)):
                raise c.WinError(c.get_last_error())
            # TOKEN_USER starts with SID_AND_ATTRIBUTES, whose first member is
            # the SID pointer. Use the actual user, not a creator/owner alias.
            sid = c.cast(data, c.POINTER(c.c_void_p))[0]
            value = w.LPWSTR()
            if not self.security.ConvertSidToStringSidW(sid, c.byref(value)):
                raise c.WinError(c.get_last_error())
            try:
                return 'D:P(A;;FA;;;' + value.value + ')(A;;FA;;;SY)'
            finally:
                self.kernel.LocalFree(c.cast(value, c.c_void_p))
        finally:
            self.kernel.CloseHandle(token)

    def open(self, path, *, directory=False, create=False):
        c = self.c
        descriptor = c.c_void_p()
        security = None
        if create:
            # Protected DACL: only the importing user and SYSTEM have access.
            if not self.security.ConvertStringSecurityDescriptorToSecurityDescriptorW(
                    self.private_sddl(), 1, c.byref(descriptor), None):
                raise c.WinError(c.get_last_error())
            security = self.Security(c.sizeof(self.Security), descriptor, False)
        name = str(path)
        if not name.startswith('\\\\?\\'):
            name = '\\\\?\\UNC\\' + name[2:] if name.startswith('\\\\') else '\\\\?\\' + name
        try:
            handle = self.kernel.CreateFileW(name, 0x40000000 if create else 0x80000000,
                0 if create else 1, c.byref(security) if security else None,
                1 if create else 3, 0x00200000 | (0x02000000 if directory else 0), None)
        finally:
            if descriptor:
                self.kernel.LocalFree(descriptor)
        if handle == c.c_void_p(-1).value:
            raise c.WinError(c.get_last_error())
        try:
            info = self.Attributes()
            if not self.kernel.GetFileInformationByHandleEx(handle, 9, c.byref(info), c.sizeof(info)):
                raise c.WinError(c.get_last_error())
            if info.attributes & 0x400 and not self.wine_host_mount(handle, info):
                raise DonorError('Symlink or reparse donor entry')
            if bool(info.attributes & 0x10) != directory:
                raise DonorError('Donor entry type changed')
            return handle
        except BaseException:
            self.kernel.CloseHandle(handle)
            raise


class WindowsDirectory:
    def __init__(self, path):
        self.path = path
        self.handle = windows().open(path, directory=True)

    def close(self):
        windows().kernel.CloseHandle(self.handle)


class DonorTree:
    def __init__(self, root):
        self.path = Path(root).absolute()
        self.stack = ExitStack()

    def __enter__(self):
        try:
            # Start at the volume/root; no unchecked ancestor participates in
            # the lookup of the selected donor, including during root admission.
            self.root = self._directory(None, self.path.anchor)
            for component in self.path.parts[1:]:
                self.root = self._directory(self.root, component)
            return self
        except BaseException:
            self.stack.close()
            raise

    def __exit__(self, *args):
        return self.stack.__exit__(*args)

    def _open_directory(self, parent, name):
        if os.name == 'nt':
            owner = WindowsDirectory(Path(name) if parent is None else parent.path/name)
            try:
                info = owner.path.lstat()
                if (getattr(info, 'st_file_attributes', 0) & 0x400 and
                        windows().wine_host_mount(owner.handle)):
                    if stat.S_ISLNK(info.st_mode) or not stat.S_ISDIR(info.st_mode):
                        raise DonorError('Donor mount type changed')
                else:
                    admit(info, True)
                return owner
            except BaseException:
                owner.close()
                raise
        flags = os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW | os.O_CLOEXEC
        fd = os.open(name, flags, dir_fd=parent)
        try:
            admit(os.fstat(fd), True)
            return fd
        except BaseException:
            os.close(fd)
            raise

    def _close(self, owner):
        owner.close() if os.name == 'nt' else os.close(owner)

    def _directory(self, parent, name):
        owner = self._open_directory(parent, name)
        self.stack.callback(self._close, owner)
        return owner

    def _info(self, owner):
        return owner.path.lstat() if os.name == 'nt' else os.fstat(owner)

    def inventory(self, *, members=100_000, depth=32, byte_limit=256*1024**3):
        rows, total = [], 0
        def walk(parent, relative, level):
            nonlocal total
            if level > depth:
                raise DonorError('Donor directory depth budget exceeded')
            with os.scandir(parent.path if os.name == 'nt' else parent) as entries:
                for entry in entries:
                    if len(rows) >= members:
                        raise DonorError('Donor directory member budget exceeded')
                    info = (parent.path/entry.name).lstat() if os.name == 'nt' else os.stat(entry.name, dir_fd=parent, follow_symlinks=False)
                    admit(info)
                    directory = stat.S_ISDIR(info.st_mode)
                    total += 0 if directory else info.st_size
                    if total > byte_limit:
                        raise DonorError('Donor directory byte budget exceeded')
                    child = relative/entry.name
                    rows.append((child, info, directory))
                    if directory:
                        owner = self._open_directory(parent, entry.name)
                        try:
                            current = self._info(owner)
                            if (current.st_dev, current.st_ino) != (info.st_dev, info.st_ino):
                                raise DonorError('Donor directory changed during inventory')
                            walk(owner, child, level+1)
                        finally:
                            self._close(owner)
        walk(self.root, Path(), 0)
        return sorted(rows, key=lambda row:row[0])

    @contextmanager
    def open_file(self, relative, expected):
        components = parts(relative)
        with ExitStack() as parents:
            parent = self.root
            for name in components[:-1]:
                parent = self._open_directory(parent, name)
                parents.callback(self._close, parent)
            if os.name == 'nt':
                import msvcrt
                handle = windows().open(parent.path/components[-1])
                try:
                    fd = msvcrt.open_osfhandle(handle, os.O_RDONLY | os.O_BINARY)
                except BaseException:
                    windows().kernel.CloseHandle(handle)
                    raise
            else:
                fd = os.open(components[-1], os.O_RDONLY | os.O_NOFOLLOW | os.O_CLOEXEC | os.O_NONBLOCK, dir_fd=parent)
            with os.fdopen(fd, 'rb') as stream:
                info = admit(os.fstat(stream.fileno()), False)
                if identity(info) != identity(expected):
                    raise DonorError(f'Source changed during import: {relative}')
                yield stream
                if identity(os.fstat(stream.fileno())) != identity(expected):
                    raise DonorError(f'Source changed during copy: {relative}')


def private_output(path):
    path = Path(path).absolute()
    if os.name == 'nt':
        import msvcrt
        handle = windows().open(path, create=True)
        try:
            fd = msvcrt.open_osfhandle(handle, os.O_WRONLY | os.O_BINARY)
        except BaseException:
            windows().kernel.CloseHandle(handle)
            raise
    else:
        fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW | os.O_CLOEXEC, 0o600)
    return os.fdopen(fd, 'wb')
