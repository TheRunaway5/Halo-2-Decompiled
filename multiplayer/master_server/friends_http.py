"""Typed HTTP/SQLite/clock IO for native accepted friendships and blocks."""
import time
from aiohttp import web
from .native_friends import Cache
from .native_party_directory import run


class FriendsApi:
    def __init__(self,directory,*,wall=None):
        self.directory,self.db=directory,directory.store.db
        self.wall=wall or (lambda:directory.store.clock())
        self.clock=time.monotonic
        self.cache=Cache()
        run(self,'schema',family='friends')
    def close(self):self.cache.close()
    def blocked(self,a,b):return run(self,'blocked',a,family='friends',target=b)
    def accepted(self,a,b):return run(self,'accepted',a,family='friends',target=b)
    def target(self,public):return run(self,'target',family='friends',body=public)
    def public_player(self,who,*,presence='offline',state=None):
        return run(self,'public_player',who,family='friends',body={'presence':presence,'state':state})
    def view(self,who,*,party=False):
        return run(self,'view',who,family='friends',features=[['party','1']] if party else [])
    async def get(self,request):
        key,who=self.directory.clans.session(request)
        value=run(self,'get',who,family='friends',account_key=key,features=list(request.query.items()))
        return web.json_response(value,headers={'Cache-Control':'no-store'})
    async def command(self,request):
        self.directory.clans.session(request)
        body=await self.directory.body(request)
        key,who=self.directory.clans.session(request)
        value=run(self,'command',who,family='friends',account_key=key,body=body,features=list(request.query.items()))
        return web.json_response(value,headers={'Cache-Control':'no-store'})
    def routes(self):
        return [web.get('/v1/friends',self.get),web.post('/v1/friends/commands',self.command)]
