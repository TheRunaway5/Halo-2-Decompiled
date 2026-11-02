"""Lifetimes and typed IO for the private Rust social voice registry."""
import weakref
from .native_matchmaking import native, _response


class Relay:
    def __init__(self,api,kind):
        core=api.directory.matchmaking.core
        lib=native()
        self._handle=_response(lib.h2_matchmaking_object_open,request=dict(social_voice=dict(
            kind=kind,account_source=api.accounts._sessions._handle,source=core._handle if core else None)))
        self._finalizer=weakref.finalize(self,lib.h2_matchmaking_owner_drop,self._handle)
    def close(self):self._finalizer();self._handle=0
    def stats(self):return _response(native().h2_matchmaking_owner_request,self._handle,request={'op':'stats'})
