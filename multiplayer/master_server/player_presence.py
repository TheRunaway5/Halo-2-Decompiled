"""Typed clock/socket IO for native observed presence and anonymous population.

The actual Rust account, queue and room owners select all seats and activity.
These observations are modern service policy, not Xbox Live presence flags.
"""
from .native_matchmaking import native, _response


def _view(directory, view):
    api = directory.matchmaking
    transport = getattr(directory, 'match_transport', None)
    rooms = [dict(id=room.id, io_now=room.lifecycle_clock(),
                  peers=[dict(player=player, connection=peer.connection,
                              closed=peer.socket.closed, control_now=peer.control_clock())
                         for player, peer in room.peers.items()])
             for room in transport.rooms.values()] if transport else []
    return _response(native().h2_matchmaking_owner_request, directory.clans._sessions._handle,
        request=dict(op='presence', view=view, source=api.core._handle if api.core else None,
                     now=directory.clans.clock(), queue_now=api.clock(),
                     wall=int(directory.store.clock()), rooms=rooms))


def observed(directory):
    return _view(directory, 'observed')


def population(directory):
    return _view(directory, 'population')


def room_connections(directory):
    return _view(directory, 'rooms')
