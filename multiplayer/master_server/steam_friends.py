"""Temporary HTTP IO for Rust-owned Steam lookup and invitation evidence."""
import time
from aiohttp import web
from .native_steam_friends import Cache, run


class SteamFriends:
    def __init__(self,directory,*,clock=time.monotonic):
        self.directory,self.clock=directory,clock
        self._cache=Cache(self)
    def close(self):self._cache.close()

    async def lookup(self,player,*,account_key,body):
        return (await run(self,player,account_key,mode='invite',body=body))['value']

    async def get(self,request):
        self._cache.call('rate',source=self.directory.source(request))
        key,who=self.directory.clans.session(request)
        response=await run(self,who,key,mode='view')
        return web.json_response(response['value'],status=response['status'],headers=response['headers'])

    def routes(self):return [web.get('/v1/account/steam-friends',self.get)]
