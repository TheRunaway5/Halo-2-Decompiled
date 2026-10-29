"""Temporary SQLite/provider IO for the Rust accepted-party directory owner."""
from contextlib import nullcontext
from .native_matchmaking import native, _response
from .native_database import _parameter
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
    if method=='friend_tag':
        from .friend_tags import resolve
        return resolve(api.db,args)
    if method=='room_connections':
        transport=getattr(directory,'match_transport',None)
        return [dict(closed=r.closed,phase=r.phase,
                     peers=[dict(player=who,closed=p.socket.closed) for who,p in r.peers.items()])
                for r in (transport.rooms.values() if transport else ())]
    require(False,'Unknown native party provider request',503)


def run(api,mode,player=None,*,family='party',account_key=None,**values):
    core=api.directory.matchmaking.core
    accounts=api if family=='clan' else getattr(api.directory,'clans',None)
    account_source=accounts._sessions._handle if account_key is not None else None
    lib=native()
    handle=_response(lib.h2_matchmaking_object_open,request=dict(party_directory=dict(
        mode=mode,source=core._handle if core else None,player=player,family=family,
        account_source=account_source,account_key=account_key,account_now=accounts.clock() if account_key is not None else None,
        wall=int(api.wall()),clock=api.directory.matchmaking.clock(),**values)))
    try:
        event=_response(lib.h2_matchmaking_owner_request,handle,request={'op':'begin'})
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
                        if event['mode']=='query':rows=[list(row) for row in cursor.fetchmany(129)]
                        else:require(event['mode']=='batch','Invalid native party SQL mode',503)
                    reply=dict(rows=rows,changes=changes)
                event=_response(lib.h2_matchmaking_owner_request,handle,request=dict(op='reply',frame=frame,**reply))
            require(event['kind']=='done','Invalid native party response',503)
            value=event['value']
        if event.get('rejection'):
            raise Problem(event['rejection']['status'],event['rejection']['message'])
        return value
    finally:
        lib.h2_matchmaking_owner_drop(handle)
