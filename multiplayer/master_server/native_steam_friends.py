"""Temporary clock/HTTP/SQL/presence IO for the complete Rust Steam request."""
import asyncio
from contextlib import nullcontext
import weakref
from aiohttp import ClientError
from .native_matchmaking import native, _response
from .native_database import _parameter, _cell
from .protocol import require


class Cache:
    def __init__(self,api):
        self.api=api
        lib=native()
        self._handle=_response(lib.h2_matchmaking_object_open,request=dict(steam_cache=dict(account_source=api.directory.clans._sessions._handle)))
        self._finalizer=weakref.finalize(self,lib.h2_matchmaking_owner_drop,self._handle)
    def close(self):self._finalizer();self._handle=0
    def call(self,op,**values):return _response(native().h2_matchmaking_owner_request,self._handle,request=dict(op=op,now=self.api.clock(),**values))
    def clear(self):self.call('clear')
    def stats(self):return self.call('stats')


async def run(api,who,key,*,mode,body=None):
    directory=api.directory
    db=directory.store.db
    accounts=directory.clans
    lib=native()
    handle=_response(lib.h2_matchmaking_object_open,request=dict(steam_request=dict(
        cache_source=api._cache._handle,account_source=accounts._sessions._handle,
        account_now=accounts.clock(),player=who,key=key,mode=mode,body=body,
        now=api.clock(),credential=directory.settings.steam_key,transaction_owned=not db.in_transaction)))
    def call(**reply):
        return _response(lib.h2_matchmaking_owner_request,handle,request=dict(
            now=api.clock(),account_now=accounts.clock(),credential=directory.settings.steam_key,**reply))
    try:
        event=call(op='begin')
        while event['kind']!='done':
            frame=event['frame']
            if event['kind']=='wait':
                await asyncio.sleep(event['delay_ms']/1000)
                reply={}
            elif event['kind']=='sql':
                owned=not db.in_transaction
                banks=[]
                with db if owned else nullcontext():
                    if owned:db.execute('BEGIN')
                    for statement in event['statements']:
                        cursor=db.execute(statement['sql'],tuple(_parameter(v) for v in statement['params']))
                        banks.append([[_cell(v) for v in row] for row in cursor.fetchmany(129)])
                reply=dict(rows=banks,committed=owned)
            elif event['kind']=='presence':
                from .player_presence import observed
                reply=dict(value=observed(directory))
            elif event['kind']=='http':
                try:
                    async with directory.session.get(event['url'],params=dict(event['params'],key=directory.settings.steam_key),allow_redirects=event['allow_redirects']) as response:
                        data=bytearray()
                        async for chunk in response.content.iter_chunked(8192):
                            data.extend(chunk[:event['read_limit']+1-len(data)])
                            if len(data)>event['read_limit']:break
                        reply=dict(status=response.status,encoding=response.headers.get('Content-Encoding',''),length=response.content_length,body=data.hex())
                except (ClientError,asyncio.TimeoutError):
                    reply=dict(status=0,encoding='',length=None,body='')
            else:require(False,'Unknown native Steam IO request',503)
            event=call(op='reply',frame=frame,**reply)
        return event
    finally:lib.h2_matchmaking_owner_drop(handle)
