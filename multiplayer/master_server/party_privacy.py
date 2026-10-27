"""Bounded original party privacy kernel adapter; no Python policy fallback.

The original authenticated leader alone changes privacy. This metadata update
does not change roster/search consent or the published countdown. Actual
relationships/invitations, block preferences and transport admission stay in
their existing authoritative service owners.
"""
import ctypes
from functools import lru_cache
from .protocol import integer, require
from .security_policy import library


@lru_cache(maxsize=1)
def native():
    try:
        lib=library()
        for name,args,result in (
            ("h2_party_privacy_mode",[ctypes.c_int32],ctypes.c_int32),
            ("h2_party_privacy_can_change",[ctypes.c_bool,ctypes.c_bool],ctypes.c_bool),
            ("h2_party_privacy_admission",[ctypes.c_int32,ctypes.c_bool,ctypes.c_int32,
                                           ctypes.c_int32,ctypes.c_int32],ctypes.c_int32)):
            require(hasattr(lib,name),"Original native party privacy kernel is not installed",503)
            function=getattr(lib,name);function.argtypes=args;function.restype=result
        return lib
    except (OSError,AttributeError):
        require(False,"Original native party privacy kernel is unavailable",503)


def mode(value):
    value=integer(value,0,2)
    result=native().h2_party_privacy_mode(value)
    require(result==value,"Invalid native party privacy mode",503)
    return result


def can_change(established,leader):
    require(type(established) is bool and type(leader) is bool,"Invalid party authority",503)
    return native().h2_party_privacy_can_change(established,leader)


def admission(value,invited,count,available):
    require(type(invited) is bool,"Invalid invitation authority",503)
    value=mode(value);count=integer(count,1,4);available=integer(available,0,16)
    #64D70 advertises public capacity for0, private capacity for1, and no
    #available slots for2. The source validator selects the matching field.
    result=native().h2_party_privacy_admission(value,invited,count,
        available if value==0 else 0,available if value==1 else 0)
    require(result in (0,7,8),"Invalid native party admission result",503)
    return result
