"""Temporary HTTP/clock/SQLite IO for the Rust accepted-party directory.

Membership, invitations, privacy, activity capabilities, request replay and
presentation are Rust policy. These are native service rules; original party
privacy/countdown leaves retain their separate source evidence.
"""
import asyncio
import contextlib
import time

from aiohttp import web
from .native_party_countdowns import Countdowns
from .native_party_directory import run
from .native_database import apply as apply_plan


class PartyApi:
    def __init__(self,directory,*,wall=time.time,clock=time.monotonic):
        self.directory,self.db,self.wall=directory,directory.store.db,wall
        self.clock=clock
        self._countdowns=Countdowns(self)
        run(self,'schema')

    def matching_facts(self,who=None,*,party_id=None):
        return run(self,'facts',who,party=party_id)

    def matching_effects(self,plan):
        core=self.directory.matchmaking.core
        if core:core._call('party_effects',effects=plan['effects'])

    @property
    def countdowns(self):return self._countdowns

    def privacy_state(self,party):return tuple(run(self,'privacy_state',party=party))
    def cleanup(self):run(self,'cleanup')
    def party(self,who):
        row=run(self,'party',who)
        return tuple(row) if row else None
    def members(self,party):return [tuple(row) for row in run(self,'members',party=party)]

    def _state(self,who):
        core=self.directory.matchmaking.core
        return core._call('party_directory_context',players=[who],rooms=self.directory.matchmaking.room_facts())[0] if core else None

    def queue_session(self,who):
        state=self._state(who)
        row=state['session'] if state else None
        return self.directory.matchmaking.sessions.project(row) if row else None

    def playing(self,who):
        state=self._state(who)
        return state['playing'] if state else False

    def searching(self,who):
        state=self._state(who)
        return state['searching'] if state else False

    def friend_joinable(self,who,target):return run(self,'friend_joinable',who,target=target)
    def friend_context(self,who,target):return run(self,'friend_context',who,target=target)
    def changed(self,party,*,admitted=False):run(self,'changed',party=party,admitted=admitted)
    def session_lost(self,who):run(self,'session_lost',who)

    def advance_countdowns(self):
        core=self.directory.matchmaking.core
        if core and self._countdowns:
            versions=[dict(id=row[0],version=row[1]) for row in self.db.execute('SELECT id,version FROM parties LIMIT 1001')]
            self._countdowns.refresh(core._call('party_countdown_tick',clock_ms=int(self.clock()*1000),versions=versions))

    async def lifetime(self,app):
        async def pump():
            while True:
                self.advance_countdowns()
                await asyncio.sleep(.05)
        task=asyncio.create_task(pump())
        try:yield
        finally:
            task.cancel()
            with contextlib.suppress(asyncio.CancelledError):await task

    def countdown_view(self,who):
        core=self.directory.matchmaking.core
        if core:return core._call('party_countdown_view',player=who,facts=self.matching_facts(who))
        party=self.party(who)
        return dict(version=party[2] if party else None,countdown=None)

    def countdown_command(self,who,body):
        api=self.directory.matchmaking
        api.enabled()
        token=None
        try:
            with self.db:
                previous=self.db.execute('SELECT payload FROM party_requests WHERE player=? AND request=?',(who,body['request_id'])).fetchone()
                count=self.db.execute('SELECT count(*) FROM party_requests WHERE player=?',(who,)).fetchone()[0]
                plan=api.core._call('party_countdown_prepare',player=who,body=body,
                    facts=self.matching_facts(who),rooms=api.room_facts(),clock_ms=int(self.clock()*1000),
                    wall_seconds=int(self.wall()),requests=dict(payload=previous[0] if previous else None,count=count))
                token=plan['token']
                apply_plan(self.db,plan)
            if token is not None:
                rows=api.core._call('party_countdown_commit',token=token,facts=self.matching_facts(who),rooms=api.room_facts())
                self._countdowns.refresh(rows)
        finally:
            if token is not None:api.core._call('party_countdown_abort',token=token)

    async def get_countdown(self,request):
        _,who=self.directory.clans.session(request,rate=False)
        self.directory.clans.countdown_rate(who)
        return web.json_response(self.countdown_view(who))

    def view(self,who,**features):return run(self,'view',who,features=features)

    async def get(self,request):
        key,who=self.directory.clans.session(request)
        return web.json_response(run(self,'get',who,account_key=key,features=dict(request.query)))

    async def activity(self,request):
        body=await self.directory.body(request)
        key,who=self.directory.clans.session(request)
        return web.json_response(run(self,'activity',who,account_key=key,body=body))

    async def command(self,request):
        body=await self.directory.body(request)
        key,who=self.directory.clans.session(request)
        plan=run(self,'command_probe',who,account_key=key,body=body,features=dict(request.query))
        route=plan['route']
        if route=='countdown':
            self.countdown_command(who,plan['body'])
        elif route in ('command','steam'):
            steam=None
            if route=='steam':
                steam=await self.directory.steam_friends.lookup(who)
                # A provider await cannot retain an expired account grant.
                key,who=self.directory.clans.session(request)
            run(self,'command',who,account_key=key,body=body,steam=steam)
        return web.json_response(self.view(who,**plan['features']))

    def queue_members(self,who,policy):
        api=self.directory.matchmaking
        api.enabled()
        selected=api.core._call('party_select',player=who,playlist=policy.id,facts=self.matching_facts(who),rooms=api.room_facts())
        return selected['party_id'],tuple(api.sessions.project(row) for row in selected['members'])

    def queue_prepare(self,session,body):
        api=self.directory.matchmaking
        return api.core._call('party_queue_prepare',generation=session.generation,body=body,
                              facts=self.matching_facts(session.player),rooms=api.room_facts())

    def routes(self):
        return [web.get('/v1/party',self.get),web.get('/v1/party/countdown',self.get_countdown),
                web.post('/v1/party/commands',self.command),web.post('/v1/party/activity',self.activity)]
