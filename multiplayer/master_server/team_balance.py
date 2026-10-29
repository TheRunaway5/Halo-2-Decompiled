"""Trusted bounded adapter to the original native team-balancing kernels.

No Python assignment fallback. Session identities and raw ratings remain owned
by authenticated admission; this interface receives only local integer arrays.
"""
import ctypes
from functools import lru_cache
from .protocol import integer, require
from .security_policy import library


@lru_cache(maxsize=1)
def native():
    lib=library()
    prefix=[ctypes.c_uint32,ctypes.c_uint32,ctypes.c_bool,ctypes.c_uint32,ctypes.c_uint32,
            ctypes.c_bool,ctypes.c_size_t,ctypes.POINTER(ctypes.c_int32)]
    signatures={
        "h2_team_balance":prefix+[ctypes.POINTER(ctypes.c_int32),ctypes.c_bool,ctypes.POINTER(ctypes.c_int32)],
        "h2_team_balance_count":prefix+[ctypes.POINTER(ctypes.c_int32)],
        "h2_team_balance_can_add":prefix+[ctypes.c_size_t,ctypes.c_int32],
        "h2_matchmaking_assignment":[ctypes.c_bool]+[ctypes.c_uint32]*7+[
            ctypes.c_size_t,ctypes.POINTER(ctypes.c_int32),ctypes.POINTER(ctypes.c_int32),
            ctypes.POINTER(ctypes.c_int32)],
        "h2_original_hopper_assignment":[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_size_t,
            ctypes.POINTER(ctypes.c_int32),ctypes.POINTER(ctypes.c_int32),ctypes.POINTER(ctypes.c_int32)],
    }
    for name,arguments in signatures.items():
        require(hasattr(lib,name),"Original native team-balancing kernel is not installed",503)
        function=getattr(lib,name);function.argtypes=arguments;function.restype=ctypes.c_bool
    return lib


def inputs(teams,maximum,parties):
    integer(teams,1,4);integer(maximum,1,16)
    require(type(parties) in (list,tuple) and len(parties)<=16,"Invalid native balancing roster")
    for party in parties:integer(party,0,15)
    return (ctypes.c_int32*len(parties))(*parties)


def assign(policy,parties,weights):
    """Typed temporary adapter; the entire assignment decision is in Rust.

    Existing native catalog fields are passed as declared. They are not
    authenticated historical hopper bytes; the dedicated header ABI handles
    those independently. No adjusted/display-rank arithmetic originates here.
    """
    ids=inputs(policy.team_count,policy.team_size or 1,parties)
    require(type(weights) in (list,tuple) and len(weights)==len(parties),"Invalid native assignment weights")
    for value in weights:integer(value,-0x80000000,0x7fffffff)
    values=(ctypes.c_int32*len(weights))(*weights)
    output=(ctypes.c_int32*len(parties))()
    if policy.hopper is not None:
        raw=bytes.fromhex(policy.hopper)
        header=ctypes.create_string_buffer(raw)
        if not native().h2_original_hopper_assignment(header,len(raw),len(parties),ids,values,output):
            return None
        return tuple(output)
    clan,significant=policy.clan_match,policy.party_balance
    if not native().h2_matchmaking_assignment(policy.ranked,policy.team_count,policy.team_size,
            getattr(clan,"minimum",0),getattr(clan,"imbalance",0),
            getattr(significant,"minimum_large",0),getattr(significant,"maximum",0),
            getattr(significant,"imbalance",0),len(parties),ids,values,output):
        return None
    return tuple(output)


def balance(teams,maximum,parties,weights,*,use_parties=True,optimize=True):
    ids=inputs(teams,maximum,parties)
    require(type(weights) in (list,tuple) and len(weights)==len(parties),"Invalid native balancing weights")
    for value in weights:integer(value,-0x80000000,0x7fffffff)
    values=(ctypes.c_int32*len(weights))(*weights)
    output=(ctypes.c_int32*len(parties))()
    if not native().h2_team_balance(teams,maximum,False,0,0,use_parties,len(parties),ids,values,optimize,output):
        return None
    return tuple(output)


def can_add(teams,maximum,parties,*,extra=0,use_parties=True,maximum_imbalance=0):
    ids=inputs(teams,maximum,parties)
    integer(extra,0,16-len(parties));integer(maximum_imbalance,0,15)
    return bool(native().h2_team_balance_can_add(teams,maximum,False,0,0,use_parties,
        len(parties),ids,extra,maximum_imbalance))
