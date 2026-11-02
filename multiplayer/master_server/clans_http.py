"""Typed HTTP/identity/clock IO for Rust account-session and clan owners.

Role/lifecycle source evidence: ../docs/halo2-clan-evidence.md. Provider
verification, canonical identities and friend-cloud IO remain shared adapters.
"""
import time
from aiohttp import web
from .player_identity import initialize,player,lookup,VERIFIED_DISCORD_USERNAME
from .native_account_sessions import Accounts
from .native_party_directory import run


class ClanApi:
    def __init__(self,directory,*,clock=time.monotonic,wall=time.time):
        self.directory,self.clock,self.wall=directory,clock,wall
        self.db=directory.store.db
        self._sessions=Accounts(self)
        initialize(self.db)
        run(self,'schema',family='clan')

    @property
    def sessions(self):return self._sessions

    def revoke_player(self,who):self._sessions.revoke(who)
    def countdown_rate(self,who):self._sessions.call('budget',player=who,bucket='countdown')

    def cleanup(self):
        self._sessions.call('cleanup')
        run(self,'cleanup',family='clan')

    def session(self,request,*,rate=True):
        self.cleanup()
        row=self._sessions.call('auth',header=request.headers.get('Authorization',''),rate=rate)
        return row['key'],row['player']

    async def login(self,request):
        body=await self.directory.body(request)
        self._sessions.call('body',body=body)
        owner=await self.directory.owner(request)
        self.cleanup()
        existing=lookup(self.db,owner)
        plan=self._sessions.call('prepare',body=body,owner=owner,existing=existing)
        try:
            who=player(self.db,owner,plan['name'],request.get(VERIFIED_DISCORD_USERNAME,''),now=self.directory.store.now())
            with self.db:
                self.directory.matchmaking.results.history.identity(who)
                self.directory.friend_cloud.enqueue(who)
            result=self._sessions.call('commit',permit=plan['permit'],owner=owner,player=who)
            return web.json_response(result,status=201)
        finally:
            self._sessions.call('abort',permit=plan['permit'])

    async def logout(self,request):
        key,_=self.session(request)
        self._sessions.call('revoke',key=key)
        return web.json_response({'state':'signed_out'})

    def member(self,who):
        row=run(self,'member',who,family='clan')
        return tuple(row) if row else None

    def view(self,who,presence=False):
        return run(self,'view',who,family='clan',features=dict(presence=presence))

    async def get(self,request):
        key,who=self.session(request)
        return web.json_response(run(self,'get',who,family='clan',account_key=key,features=dict(request.query)))

    async def command(self,request):
        body=await self.directory.body(request)
        key,who=self.session(request)
        run(self,'command',who,family='clan',account_key=key,body=body)
        return web.json_response(run(self,'get',who,family='clan',account_key=key,features=dict(request.query)))

    def routes(self):
        return [web.post('/v1/account/session',self.login),web.delete('/v1/account/session',self.logout),
                web.get('/v1/clans',self.get),web.post('/v1/clans/commands',self.command)]
