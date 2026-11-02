"""Temporary SQLite/provider IO for the Rust accepted-party directory owner."""
from contextlib import nullcontext
from .native_matchmaking import native, _response
from .native_database import _parameter, _cell
from .protocol import require, Problem


def _provider(api,method,args):
    directory=api.directory
    if method=='rooms':return directory.matchmaking.room_facts()
    if method=='online':
        from .player_presence import observed
        present=observed(directory)
        return [player in present for player in args]
    if method=='presence_states':
        from .player_presence import observed
        present=observed(directory)
        return [present.get(player,'offline') for player in args]
    if method=='voice_allowed':return [directory.voice_blocks.allowed(a,b) for a,b in args]
    if method=='friend_presence':
        from .player_presence import observed
        return observed(directory)
    if method=='friend_party_views':
        return [dict(joinable=directory.parties.friend_joinable(args['player'],target),
                     context=directory.parties.friend_context(args['player'],target)) for target in args['targets']]
    if method=='room_connections':
        from .player_presence import room_connections
        return room_connections(directory)
    require(False,'Unknown native party provider request',503)


def run(api,mode,player=None,*,family='party',account_key=None,friend_cloud=None,**values):
    directory=api.directory
    core=directory.matchmaking.core if directory else None
    accounts=api if family=='clan' else getattr(directory,'clans',None)
    account_source=accounts._sessions._handle if account_key is not None else None
    profile=api if family=='profile' and hasattr(api,'cache') else getattr(directory,'profiles',None) if family=='history' else None
    profile_source=profile.cache._handle if profile else None
    friends=api if family=='friends' and hasattr(api,'cache') else None
    friend_source=friends.cache._handle if friends else None
    voice=api if family=='voice' and hasattr(api,'relay') else None
    identity_accounts=getattr(directory,'clans',None) if family=='identity' and mode in ('link','link_preflight') else None
    steam=getattr(directory,'steam_friends',None) if family=='party' and mode=='command' and account_key is not None and values.get('body',{}).get('action')=='invite_steam' else None
    transaction_owned=not api.db.in_transaction
    lib=native()
    handle=_response(lib.h2_matchmaking_object_open,request=dict(party_directory=dict(
        mode=mode,source=core._handle if core else None,player=player,family=family,
        account_source=account_source,account_key=account_key,account_now=accounts.clock() if account_key is not None else None,
        profile_source=profile_source,profile_now=profile.clock() if profile else None,
        friend_source=friend_source,friend_now=friends.clock() if friends else None,
        friend_cloud=bool(getattr(getattr(directory,'friend_cloud',None),'enabled',False)) if friend_cloud is None else friend_cloud,
        voice_source=voice.relay._handle if voice else None,voice_now=voice.clock() if voice else None,
        voice_account_now=voice.accounts.clock() if voice else None,
        steam_source=steam._cache._handle if steam else None,steam_now=steam.clock() if steam else None,
        steam_credential=directory.settings.steam_key if steam else None,
        identity_account_source=identity_accounts._sessions._handle if identity_accounts else None,
        transaction_owned=transaction_owned,
        wall=int(api.wall()),clock=directory.matchmaking.clock() if directory else api.clock(),**values)))
    def call(**reply):
        if account_key is not None:reply['account_now']=accounts.clock()
        if profile:reply['profile_now']=profile.clock()
        if friends:reply['friend_now']=friends.clock()
        if voice:
            reply['voice_now']=voice.clock()
            reply['voice_account_now']=voice.accounts.clock()
        if steam:
            reply['steam_now']=steam.clock()
            reply['steam_credential']=directory.settings.steam_key
        return _response(lib.h2_matchmaking_owner_request,handle,request=reply)
    try:
        event=call(op='begin')
        db=api.db
        # Preserve the caller's existing transaction. Nested connection context
        # managers would commit its unrelated in-flight statements prematurely.
        with nullcontext() if db.in_transaction else db:
            if not db.in_transaction:db.execute('BEGIN')
            while event['kind'] in ('sql','provider'):
                frame=event['frame']
                if event['kind']=='provider':
                    reply=dict(value=_provider(api,event['method'],event['args']))
                else:
                    rows,changes=[],[]
                    for statement in event['statements']:
                        cursor=db.execute(statement['sql'],tuple(_parameter(v) for v in statement['params']))
                        changes.append(cursor.rowcount)
                        if event['mode']=='query':rows=[[_cell(v) for v in row] for row in cursor.fetchmany(129)]
                        else:require(event['mode']=='batch','Invalid native party SQL mode',503)
                    reply=dict(rows=rows,changes=changes)
                event=call(op='reply',frame=frame,**reply)
            require(event['kind']=='done','Invalid native party response',503)
            value=event['value']
        if (profile or friends or voice or steam or identity_accounts) and transaction_owned and not event.get('rejection'):
            committed=call(op='committed')
            if isinstance(committed,dict) and 'voice' in committed:value=committed['voice']
        if event.get('rejection'):
            raise Problem(event['rejection']['status'],event['rejection']['message'])
        if event.get('friend_cloud_wake'):
            cloud=getattr(directory,'friend_cloud',None)
            if cloud:cloud.wake.set()
        return value
    finally:
        lib.h2_matchmaking_owner_drop(handle)
