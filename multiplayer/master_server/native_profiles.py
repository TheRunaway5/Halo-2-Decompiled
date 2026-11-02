"""Typed ownership/SQLite IO for Rust profile and derived-cache policy."""
import time
import weakref
from types import SimpleNamespace
from .native_matchmaking import native,_response
from .native_party_directory import run


class Cache:
    def __init__(self):
        lib=native()
        self._handle=_response(lib.h2_matchmaking_object_open,request={'profile_cache':{}})
        self._finalizer=weakref.finalize(self,lib.h2_matchmaking_owner_drop,self._handle)
    def close(self):self._finalizer();self._handle=0
    def call(self,op):
        return _response(native().h2_matchmaking_owner_request,self._handle,request={'op':op})
    def clear(self):return self.call('clear')
    def stats(self):return self.call('stats')


def database(db,mode,player=None,**values):
    io=SimpleNamespace(directory=None,db=db,wall=time.time,clock=time.monotonic)
    return run(io,mode,player,family='profile',**values)
