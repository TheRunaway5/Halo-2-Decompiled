"""Temporary typed adapter to Rust's original raw/effective rating owners.

This module copies admitted operands and returned projections. It does not
calculate averages, extrema, rank floors, adjusted skills or search bounds.
"""
import ctypes
from dataclasses import dataclass
from functools import lru_cache

from .protocol import integer, require
from .security_policy import library


@lru_cache(maxsize=1)
def native():
    lib=library()
    i32=ctypes.POINTER(ctypes.c_int32)
    u32=ctypes.POINTER(ctypes.c_uint32)
    u8=ctypes.POINTER(ctypes.c_uint8)
    signatures={
        'h2_matchmaking_rank_limits':[ctypes.c_void_p,ctypes.c_size_t,u8,ctypes.c_size_t],
        'h2_matchmaking_raw_ratings':[ctypes.c_size_t,i32,u32,i32,ctypes.c_size_t],
        'h2_matchmaking_adjusted_ratings':[ctypes.c_size_t,i32,i32,ctypes.c_uint32,u8,ctypes.c_size_t,i32],
        'h2_matchmaking_advertised_minimum':[ctypes.c_int32,ctypes.c_int32,u8,ctypes.c_size_t,i32],
        'h2_matchmaking_rank_window':[ctypes.c_int32,ctypes.c_int32,ctypes.c_int32,u8,
                                     ctypes.c_size_t,i32,ctypes.c_size_t],
    }
    for name,arguments in signatures.items():
        require(hasattr(lib,name),'Original Rust matchmaking ratings require an update',503)
        function=getattr(lib,name);function.argtypes=arguments;function.restype=ctypes.c_bool
    return lib


@lru_cache(maxsize=32)
def rank_limits(header=None):
    """Read an admitted header's bytes, or explicit retail executable defaults."""
    raw=None if header is None else bytes.fromhex(header) if type(header) is str else bytes(header)
    if raw is not None and len(raw)!=0x614:
        raise ValueError('Invalid admitted hopper header')
    source=None if raw is None else ctypes.create_string_buffer(raw)
    output=(ctypes.c_uint8*128)()
    if not native().h2_matchmaking_rank_limits(source,0 if raw is None else len(raw),output,128):
        raise ValueError('Invalid admitted hopper rank map')
    return tuple(output)


def table(values=None):
    values=rank_limits() if values is None else tuple(values)
    if len(values)!=128 or any(type(n) is not int or not 0<=n<=255 for n in values):
        raise ValueError('Invalid rank-map carrier')
    return (ctypes.c_uint8*128)(*values)


def ranks(values):
    values=tuple(values)
    if len(values)>16:
        raise ValueError('Invalid bounded rating roster')
    for value in values:integer(value,-1,127)
    return values,(ctypes.c_int32*len(values))(*values)


@dataclass(frozen=True)
class RawStatistics:
    average: int
    maximum: int
    minimum: int
    rated_count: int


def raw_statistics(raw,identity_flags):
    values,source=ranks(raw)
    identity_flags=tuple(identity_flags)
    if len(identity_flags)!=len(values):
        raise ValueError('Invalid rating identity carrier')
    for value in identity_flags:integer(value,0,0xffffffff)
    flags=(ctypes.c_uint32*len(values))(*identity_flags)
    output=(ctypes.c_int32*4)()
    if not native().h2_matchmaking_raw_ratings(len(values),source,flags,output,4):
        raise ValueError('Invalid trusted raw rating inputs')
    return RawStatistics(*output)


def adjusted_ranks(raw,groups,catalog_index,limits=None):
    """Requires the actual catalog index; no caller/grouping origin is inferred."""
    values,source=ranks(raw)
    groups=tuple(groups)
    if len(groups)!=len(values):
        raise ValueError('Invalid rating group carrier')
    integer(catalog_index,0,15)
    for value in groups:integer(value,0,15)
    indices=(ctypes.c_int32*len(groups))(*groups)
    forward=table(limits);output=(ctypes.c_int32*len(values))()
    if not native().h2_matchmaking_adjusted_ratings(len(values),source,indices,catalog_index,forward,128,output):
        raise ValueError('Invalid trusted adjusted rating inputs')
    return tuple(output)


def advertised_minimum(minimum,maximum,limits=None):
    if type(minimum) is not int or type(maximum) is not int or not (
            -0x80000000<=minimum<=0x7fffffff and -0x80000000<=maximum<=0x7fffffff):
        raise ValueError('Invalid trusted gathering ranks')
    forward=table(limits);output=ctypes.c_int32()
    if not native().h2_matchmaking_advertised_minimum(minimum,maximum,forward,128,ctypes.byref(output)):
        raise ValueError('Invalid trusted gathering ranks')
    return output.value


def rank_window(minimum,maximum,radius=None,limits=None):
    if type(minimum) is not int or type(maximum) is not int or not (
            -0x80000000<=minimum<=0x7fffffff and -0x80000000<=maximum<=0x7fffffff) or (radius is not None and
            (type(radius) is not int or radius<0 or radius>0x7fffffff)):
        raise ValueError('Invalid trusted rank-window carrier')
    forward=table(limits);output=(ctypes.c_int32*3)()
    if not native().h2_matchmaking_rank_window(minimum,maximum,-2 if radius is None else radius,
            forward,128,output,3):
        raise ValueError('Invalid trusted gathering search radius or ranks')
    return tuple(output)


RETAIL_RANK_LIMITS=rank_limits()
