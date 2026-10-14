"""Observed native presence; not an emulation of Xbox Live presence flags.

Only recent authenticated requests or a live, challenged match connection may
publish presence. Account admission by itself is not a fifteen-minute heartbeat.
"""
import time

ACTIVITY_SECONDS = 30


def observed(directory):
    result = {}
    for api in (directory.clans, directory.matchmaking):
        now = api.clock()
        for session in api.sessions.values():
            if session.expires > now and 0 <= now - session.seen < ACTIVITY_SECONDS:
                result[session.player] = "online"
                if hasattr(session, "seats"):
                    for player in session.seats():
                        if not player.guest:
                            result[player.player] = "online"
    transport = getattr(directory, "match_transport", None)
    now = time.monotonic()
    for room in transport.rooms.values() if transport else ():
        if room.closed or now > room.deadline:
            continue
        for who, peer in room.peers.items():
            if peer.socket.closed or (peer.challenge and now - peer.sent > 15):
                continue
            result[who] = "playing" if room.phase == "running" else "online"
            for player, owner in room.owners.items():
                if owner == who and player not in room.guests:
                    result[player] = result[who]
    return result
