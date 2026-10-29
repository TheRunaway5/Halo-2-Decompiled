"""Typed IO and immutable views for Rust-owned verified-account sessions."""
from collections.abc import Mapping
from dataclasses import dataclass
import time
import weakref
from .native_matchmaking import native,_response


@dataclass(frozen=True)
class Snapshot:
    key:str
    generation:int
    player:str
    expires:float
    seen:float


class Session:
    __slots__=('_snapshot','__weakref__')
    def __setattr__(self,name,value):raise AttributeError('Account sessions belong to Rust')


for _field in Snapshot.__dataclass_fields__:
    setattr(Session,_field,property(lambda self,name=_field:getattr(self._snapshot,name)))


class Accounts(Mapping):
    def __init__(self,api):
        self.api=api
        lib=native()
        self._handle=_response(lib.h2_matchmaking_object_open,request={'accounts':{}})
        self._finalizer=weakref.finalize(self,lib.h2_matchmaking_owner_drop,self._handle)
        self._views=weakref.WeakValueDictionary()
    def close(self):
        self._finalizer()
        self._handle=0
    def call(self,op,**values):
        return _response(native().h2_matchmaking_owner_request,self._handle,
            request=dict(op=op,now=self.api.clock(),rate_now=time.monotonic(),**values))
    def __iter__(self):return iter(self.call('keys'))
    def __len__(self):return self.call('stats')['sessions']
    def __getitem__(self,key):
        row=self.call('get',key=key)
        if row is None:raise KeyError(key)
        result=self._views.get(row['generation'])
        if result is None:
            result=Session();self._views[row['generation']]=result
        object.__setattr__(result,'_snapshot',Snapshot(**row))
        return result
    def clear(self):self.call('clear')
    def revoke(self,player):self.call('revoke_player',player=player)
