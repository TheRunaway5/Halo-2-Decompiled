"""Temporary typed IO adapter for Rust-owned candidate search state.

Rust owns configuration defaults/validation, admission, score arithmetic, the
original sort, byte-cache behavior and selection. This is a private typed bank
carrier, not an endpoint or a source of Live/QoS/identity authority.
"""
import ctypes
from dataclasses import asdict, dataclass
from functools import lru_cache
import json
import weakref

from .protocol import require
from .security_policy import library

COUNT, STRIDE = 50, 0x204
BANK_SIZE = COUNT * STRIDE
EMPTY_BANK = bytes(BANK_SIZE)
I32_MIN, I32_MAX = -(1 << 31), (1 << 31)-1
REQUEST_LIMIT = 16 * 1024


@lru_cache(maxsize=1)
def native():
    lib=library()
    ptr=ctypes.c_void_p
    size=ctypes.c_size_t
    u64=ctypes.c_uint64
    signatures={
        'h2_candidates_defaults':([ptr,size],size),
        'h2_candidates_configuration_valid':([ptr,size],ctypes.c_bool),
        'h2_candidates_search_valid':([ptr,size],ctypes.c_bool),
        'h2_candidates_new':([ptr,size],u64),
        'h2_candidates_drop':([u64],None),
        'h2_candidates_configure':([u64,ptr,size],ctypes.c_bool),
        'h2_candidates_bank':([u64,ptr,size],ctypes.c_bool),
        'h2_candidates_restore_cache':([u64,ptr,size],ctypes.c_bool),
        'h2_candidates_update':([u64,ptr,size,ptr,size],ctypes.c_bool),
        'h2_candidates_select':([u64,ctypes.c_int32,ctypes.c_int32,ctypes.c_bool,ptr,size],ctypes.c_uint32),
        'h2_candidates_threshold':([ctypes.c_int32,ctypes.c_int32,ptr,size,ptr],ctypes.c_bool),
    }
    for name,(arguments,result) in signatures.items():
        require(hasattr(lib,name),'Rust candidate search requires an update',503)
        function=getattr(lib,name)
        function.argtypes=arguments
        function.restype=result
    return lib


def _carrier(value):
    raw=json.dumps(value,separators=(',',':'),allow_nan=False).encode()
    if len(raw)>REQUEST_LIMIT:
        raise ValueError('Invalid bounded candidate carrier')
    return ctypes.create_string_buffer(raw),len(raw)


def _defaults():
    output=(ctypes.c_uint8*REQUEST_LIMIT)()
    length=native().h2_candidates_defaults(output,REQUEST_LIMIT)
    require(0<length<=REQUEST_LIMIT,'Rust candidate defaults unavailable',503)
    return json.loads(bytes(output[:length]))


DEFAULTS=_defaults()


@dataclass(frozen=True)
class Configuration:
    """Explicit retail defaults come from the Rust original owner."""
    rank_radius: int = DEFAULTS['rank_radius']
    rank_high: int = DEFAULTS['rank_high']
    rank_low: int = DEFAULTS['rank_low']
    exact_space_bonus: int = DEFAULTS['exact_space_bonus']
    maximum_age_seconds: int = DEFAULTS['maximum_age_seconds']
    age_coefficient: float = DEFAULTS['age_coefficient']
    first_quality_threshold: int = DEFAULTS['first_quality_threshold']
    second_quality_threshold: int = DEFAULTS['second_quality_threshold']
    quality_adjustment: int = DEFAULTS['quality_adjustment']
    latency_curve: tuple[int,...] = tuple(DEFAULTS['latency_curve'])
    language: tuple[tuple[int,...],...] = tuple(tuple(row) for row in DEFAULTS['language'])
    partial_qos_delay_ms: int = DEFAULTS['partial_qos_delay_ms']
    threshold_initial: int = DEFAULTS['threshold_initial']
    threshold_final: int = DEFAULTS['threshold_final']

    def __post_init__(self):
        # Preserve the adapter's typed tuple/float contract before JSON erases
        # these Python representation distinctions. Rust validates all values.
        if (type(self.age_coefficient) is not float or type(self.latency_curve) is not tuple or
                type(self.language) is not tuple or any(type(row) is not tuple for row in self.language)):
            raise ValueError('Invalid typed candidate configuration')
        source,length=_carrier(asdict(self))
        if not native().h2_candidates_configuration_valid(source,length):
            raise ValueError('Invalid candidate configuration')


def _search_data(search):
    value=asdict(search)
    for name in ('own_address','clan'):
        if value[name] is not None:
            value[name]=value[name].hex()
    return value


@dataclass(frozen=True)
class Search:
    player_count: int
    average_rank: int
    language: int
    quality_first: int
    quality_second: int
    minimum_rank: int | None = None
    maximum_rank: int | None = None
    own_address: bytes | None = None
    clan: bytes | None = None

    def __post_init__(self):
        for value,size in ((self.own_address,36),(self.clan,12)):
            if value is not None and (type(value) is not bytes or len(value)!=size):
                raise ValueError('Invalid typed private session identity')
        source,length=_carrier(_search_data(self))
        if not native().h2_candidates_search_valid(source,length):
            raise ValueError('Invalid candidate search')


DEFAULT_CONFIGURATION=Configuration()


def _integer(value):
    if type(value) is not int or not I32_MIN<=value<=I32_MAX:
        raise ValueError('Invalid candidate scalar carrier')


def threshold(radius,maximum_radius,config=DEFAULT_CONFIGURATION):
    _integer(radius)
    _integer(maximum_radius)
    if type(config) is not Configuration:
        raise ValueError('Typed candidate configuration required')
    source,length=_carrier(asdict(config))
    result=ctypes.c_int32()
    if not native().h2_candidates_threshold(radius,maximum_radius,source,length,ctypes.byref(result)):
        raise ValueError('Invalid candidate threshold inputs')
    return result.value


class Candidates:
    """One Rust-owned searching party bank, configuration and exact byte cache.

    Every query owns a distinct handle. close() and finalization retire only
    that handle; no state pointer or handle is accepted from a public client.
    """
    def __init__(self,search,config=DEFAULT_CONFIGURATION):
        if type(search) is not Search or type(config) is not Configuration:
            raise ValueError('Trusted session-search values required')
        source,length=_carrier({'search':_search_data(search),'config':asdict(config)})
        lib=native()
        handle=lib.h2_candidates_new(source,length)
        require(handle!=0,'Rust candidate allocation unavailable',503)
        self._handle=handle
        self._search,self._config=search,config
        self._finalizer=weakref.finalize(self,lib.h2_candidates_drop,handle)

    def close(self):
        self._finalizer()
        self._handle=0

    def _owned(self):
        if not self._handle:
            raise ValueError('Candidate search is closed')
        return self._handle

    def _configure(self,search,config):
        if type(search) is not Search or type(config) is not Configuration:
            raise ValueError('Trusted session-search values required')
        source,length=_carrier({'search':_search_data(search),'config':asdict(config)})
        if not native().h2_candidates_configure(self._owned(),source,length):
            raise ValueError('Rust candidate configuration rejected')
        self._search,self._config=search,config

    @property
    def search(self):
        return self._search

    @search.setter
    def search(self,value):
        self._configure(value,self._config)

    @property
    def config(self):
        return self._config

    @config.setter
    def config(self,value):
        self._configure(self._search,value)

    @property
    def bank(self):
        output=(ctypes.c_uint8*BANK_SIZE)()
        if not native().h2_candidates_bank(self._owned(),output,BANK_SIZE):
            raise ValueError('Rust candidate snapshot rejected')
        return bytes(output)

    @bank.setter
    def bank(self,value):
        if type(value) is not bytes or len(value)!=BANK_SIZE:
            raise ValueError('A complete trusted candidate cache is required')
        source=ctypes.create_string_buffer(value)
        if not native().h2_candidates_restore_cache(self._owned(),source,BANK_SIZE):
            raise ValueError('Rust candidate cache restore rejected')

    def update(self,bank):
        if type(bank) is not bytes or len(bank)!=BANK_SIZE:
            raise ValueError('A complete session-candidate bank is required')
        source=ctypes.create_string_buffer(bank)
        output=(ctypes.c_uint8*BANK_SIZE)()
        if not native().h2_candidates_update(self._owned(),source,BANK_SIZE,output,BANK_SIZE):
            raise ValueError('Rust candidate bank rejected')
        return bytes(output)

    def select(self,minimum_score,elapsed_query_ms,*,active=True):
        _integer(minimum_score)
        _integer(elapsed_query_ms)
        if type(active) is not bool:
            raise ValueError('Invalid active-query flag')
        output=(ctypes.c_uint8*STRIDE)()
        status=native().h2_candidates_select(self._owned(),minimum_score,elapsed_query_ms,active,output,STRIDE)
        if status==1:
            return None
        if status!=0:
            raise ValueError('Rust candidate selection rejected')
        return bytes(output)
