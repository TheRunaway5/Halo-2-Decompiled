"""Temporary HTTP/clock IO for Rust current-profile/customization authority."""
from time import monotonic
from aiohttp import web
from .native_profiles import Cache
from .native_party_directory import run

FORMAT = "halo2-profile-appearance-v1"


class PlayerProfiles:
    def __init__(self,directory):
        self.directory,self.db=directory,directory.store.db
        self.wall=directory.store.clock
        self.cache=Cache()
        run(self,'schema',family='profile')

    def clock(self):return monotonic()
    def close(self):self.cache.close()
    def view(self,who):return run(self,'view',who,family='profile')
    def statistics(self,who):return run(self,'statistics',who,family='profile')

    async def get(self,request):
        key,who=self.directory.clans.session(request)
        return web.json_response(run(self,'get',who,family='profile',account_key=key))

    async def update(self,request):
        key,who=self.directory.clans.session(request)
        body=await self.directory.body(request)
        return web.json_response(run(self,'update',who,family='profile',account_key=key,body=body))

    async def public(self,request):
        return web.json_response(self.directory.history.read('profile',request,request.match_info['player']))

    def routes(self):
        return [web.get('/v1/account/profile',self.get),web.put('/v1/account/profile',self.update),
                web.get('/v1/history/players/{player}/profile',self.public)]
