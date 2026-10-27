"""Bounded service-owned inputs to original native readiness leaves.

The replacement transport proves actual admission and measurement ownership.
It supplies no Xbox Live policy bit, cached Live task or debug override.
"""
import ctypes
from functools import lru_cache
from .protocol import integer, require
from .security_policy import library


@lru_cache(maxsize=1)
def native():
    try:
        lib=library()
        signatures={
            "h2_matchmaking_connected_teams":[ctypes.c_size_t,ctypes.POINTER(ctypes.c_uint32),ctypes.c_uint32,
                ctypes.c_size_t,ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint32),ctypes.POINTER(ctypes.c_int32)],
            "h2_matchmaking_eligibility":[ctypes.c_bool]*7,
        }
        for name,arguments in signatures.items():
            require(hasattr(lib,name),"Original native matchmaking readiness kernel is not installed",503)
            function=getattr(lib,name);function.argtypes=arguments;function.restype=ctypes.c_bool
        return lib
    except (OSError,AttributeError):
        require(False,"Original native matchmaking readiness kernel is unavailable",503)


def connected_teams(peer_masks,current,seat_owners,seat_teams,*,occupied=None):
    require(type(peer_masks) in (tuple,list) and 1<=len(peer_masks)<=16,"Invalid native readiness peers")
    require(type(seat_owners) in (tuple,list) and type(seat_teams) in (tuple,list) and
            len(seat_owners)==len(seat_teams)<=16,"Invalid native readiness seats")
    integer(current,0,len(peer_masks)-1)
    for value in peer_masks:integer(value,0,0xFFFFFFFF)
    for value in seat_owners:integer(value,0,15)
    for value in seat_teams:integer(value,-128,127)
    if occupied is None:occupied=(1<<len(seat_owners))-1
    integer(occupied,0,(1<<len(seat_owners))-1)
    masks=(ctypes.c_uint32*len(peer_masks))(*peer_masks)
    owners=(ctypes.c_uint32*len(seat_owners))(*seat_owners)
    teams=(ctypes.c_int32*len(seat_teams))(*seat_teams)
    return bool(native().h2_matchmaking_connected_teams(len(peer_masks),masks,current,
        len(seat_owners),occupied,owners,teams))


def start_eligible(peer_masks,current,seat_owners,seat_teams,require_teams):
    require(type(require_teams) is bool,"Invalid native readiness team requirement")
    connected=connected_teams(peer_masks,current,seat_owners,seat_teams)
    # Original Live authentication/policy is explicitly absent. Never derive
    # bit31, logon status or its override from replacement Discord/EOS data.
    return bool(native().h2_matchmaking_eligibility(require_teams,connected,False,False,False,False,False))
