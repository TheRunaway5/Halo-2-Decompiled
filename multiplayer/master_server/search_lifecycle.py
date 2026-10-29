"""Temporary typed IO for Rust-owned search/gather lifecycle state.

Clocks, completion arithmetic, seen-session memory, jitter streams, native party
transitions and the named replacement service order live in Rust. These private
carriers do not emulate Live allocation, Xbox NAT or platform reservations.
"""
import ctypes
from dataclasses import asdict, dataclass
from functools import cache
import json
import weakref

from .protocol import require
from .security_policy import library

I32_MAX=0x7fffffff
REQUEST_LIMIT=256*1024
RESPONSE_LIMIT=64*1024


@cache
def native():
    lib=library()
    pointer=ctypes.c_void_p
    size=ctypes.c_size_t
    signatures={
        'h2_lifecycle_new':([pointer,size],ctypes.c_uint64),
        'h2_lifecycle_drop':([ctypes.c_uint64],None),
        'h2_lifecycle_command':([ctypes.c_uint64,pointer,size,pointer,size],size),
        'h2_lifecycle_value':([pointer,size,pointer,size],size),
    }
    for name,(arguments,result) in signatures.items():
        require(hasattr(lib,name),'Rust search lifecycle requires an update',503)
        function=getattr(lib,name)
        function.argtypes=arguments
        function.restype=result
    return lib


def _carrier(value):
    try:
        raw=json.dumps(value,separators=(',',':'),allow_nan=False).encode()
    except (TypeError,ValueError,OverflowError):
        raise ValueError('Invalid typed lifecycle carrier') from None
    if len(raw)>REQUEST_LIMIT:
        raise ValueError('Oversize lifecycle carrier')
    return ctypes.create_string_buffer(raw),len(raw)


def _response(function,*prefix,request):
    source,length=_carrier(request)
    output=(ctypes.c_uint8*RESPONSE_LIMIT)()
    count=function(*prefix,source,length,output,RESPONSE_LIMIT)
    if not 0<count<=RESPONSE_LIMIT:
        raise ValueError('Rust lifecycle rejected the owner, generation or operands')
    return json.loads(bytes(output[:count]))


def _value(op,**values):
    return _response(native().h2_lifecycle_value,request=dict(op=op,**values))


class _Owned:
    def __init__(self,kind,**values):
        source,length=_carrier(dict(kind=kind,**values))
        lib=native()
        handle=lib.h2_lifecycle_new(source,length)
        if not handle:
            raise ValueError('Rust lifecycle allocation or operands rejected')
        self.handle=handle
        self._finalizer=weakref.finalize(self,lib.h2_lifecycle_drop,handle)

    def close(self):
        self._finalizer()
        self.handle=0

    def call(self,op,**values):
        if not self.handle:
            raise ValueError('Lifecycle owner is closed')
        return _response(native().h2_lifecycle_command,self.handle,request=dict(op=op,**values))

    def snapshot(self):
        return self.call('snapshot')


def elapsed(now,start):
    return _value('elapsed',now=now,start=start)


def jitter(seed,search_spread=None,gather_spread=None):
    return tuple(_value('jitter',seed=seed,search_spread=search_spread,gather_spread=gather_spread))


def role(now,search_start,gather_start,capacity):
    return _value('role',now=now,search_start=search_start,gather_start=gather_start,capacity=capacity)


class JitterStream:
    def __init__(self,seed):
        self._owner=_Owned('jitter',seed=seed)

    @property
    def seed(self):
        return self._owner.snapshot()

    def next(self):
        return tuple(self._owner.call('jitter_next'))

    def close(self):
        self._owner.close()


class SearchProgress:
    def __init__(self,maximum,ranked,radius=0,total=0,active=True):
        self._owner=_Owned('progress',maximum=maximum,ranked=ranked,radius=radius,total=total,active=active)
        self._generation=0

    @classmethod
    def _view(cls,owner,generation):
        value=object.__new__(cls)
        value._owner,value._generation=owner,generation
        return value

    def snapshot(self):
        return self._owner.call('progress_snapshot',generation=self._generation)

    @property
    def maximum(self):return self.snapshot()['maximum']
    @property
    def ranked(self):return self.snapshot()['ranked']
    @property
    def radius(self):return self.snapshot()['radius']
    @property
    def total(self):return self.snapshot()['total']
    @property
    def active(self):return self.snapshot()['active']

    def complete(self,count,*,query_pending=False,full_pending=False,brief_pending=False,selected=False):
        return self._owner.call('progress_complete',generation=self._generation,count=count,
            query_pending=query_pending,full_pending=full_pending,brief_pending=brief_pending,selected=selected)


class SeenSessions:
    def __init__(self):
        self._owner=_Owned('seen')
        self._generation=0

    @classmethod
    def _view(cls,owner,generation):
        value=object.__new__(cls)
        value._owner,value._generation=owner,generation
        return value

    def reject(self,identity):
        self._owner.call('seen_reject',generation=self._generation,identity=identity)

    def clear(self):
        self._owner.call('seen_clear',generation=self._generation)

    def __contains__(self,identity):
        return self._owner.call('seen_contains',generation=self._generation,identity=identity)

    def __len__(self):
        return len(self._owner.call('seen_snapshot',generation=self._generation))


@dataclass(frozen=True)
class GatheringConfiguration:
    countdown_ms: tuple[int,...]
    timeout_ms: int
    provenance: str

    def __post_init__(self):
        if type(self.countdown_ms) is not tuple:
            raise ValueError('Typed gathering countdown tuple required')
        _value('config_valid',config=asdict(self))

    def delay(self,players,minimum,maximum):
        return _value('config_delay',config=asdict(self),players=players,minimum=minimum,maximum=maximum)


class GatheringClock:
    def __init__(self,started,growth,peak=0):
        self._owner=_Owned('clock',started=started,growth=growth,peak=peak)

    def snapshot(self):
        return self._owner.snapshot()

    @property
    def started(self):return self.snapshot()['started']
    @property
    def growth(self):return self.snapshot()['growth']
    @growth.setter
    def growth(self,value):
        # Trusted diagnostic restoration, never a request-supplied clock.
        self._owner.call('clock_growth',growth=value)
    @property
    def peak(self):return self.snapshot()['peak']

    def observe(self,now,players):
        return self._owner.call('clock_observe',now=now,players=players)

    def expired(self,now,configuration,offset):
        return self._owner.call('clock_expired',now=now,config=asdict(configuration),offset=offset)

    def ready(self,now,configuration,players,minimum,maximum):
        return self._owner.call('clock_ready',now=now,config=asdict(configuration),
            players=players,minimum=minimum,maximum=maximum)

    def remaining(self,now,configuration,players,minimum,maximum):
        return self._owner.call('clock_remaining',now=now,config=asdict(configuration),
            players=players,minimum=minimum,maximum=maximum)


class PartyState:
    def __init__(self,*,search_offset=0,gather_offset=0):
        self._owner=_Owned('party',search_offset=search_offset,gather_offset=gather_offset)

    def snapshot(self):
        return self._owner.snapshot()

    @property
    def mode(self):return self.snapshot()['mode']
    @property
    def search_start(self):return self.snapshot()['search_start']
    @property
    def gather_start(self):return self.snapshot()['gather_start']
    @property
    def search_offset(self):return self.snapshot()['search_offset']
    @property
    def gather_offset(self):return self.snapshot()['gather_offset']

    @property
    def progress(self):
        state=self.snapshot()
        return (None if state['progress'] is None else
                SearchProgress._view(self._owner,state['generation']))

    @property
    def seen(self):
        return SeenSessions._view(self._owner,self.snapshot()['generation'])

    def start_search(self,now,maximum,ranked):
        self._owner.call('party_start_search',now=now,maximum=maximum,ranked=ranked)

    def start_gather(self,now):
        self._owner.call('party_start_gather',now=now)

    def choose(self):
        self._owner.call('party_choose')

    def joined(self):
        self._owner.call('party_joined')

    def role(self,now,capacity):
        return self._owner.call('party_role',now=now,capacity=capacity)

    def search_expired(self,now):
        return self._owner.call('party_search_expired',now=now)

    def complete_query(self,count):
        generation=self.snapshot()['generation']
        return self._owner.call('party_complete_query',generation=generation,count=count)


@cache
def retail_configuration(index):
    minimum,maximum,config=_value('retail_configuration',index=index)
    config['countdown_ms']=tuple(config['countdown_ms'])
    return minimum,maximum,GatheringConfiguration(**config)


def native_birth_id_sessions(rows):
    """Explicit native provider order; original Live procedure remains unknown."""
    return tuple(_value('native_birth_id_order',rows=[dict(born=born,id=identity) for born,identity in rows]))
