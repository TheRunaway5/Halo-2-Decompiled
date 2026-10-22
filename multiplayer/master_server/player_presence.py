"""Observed native presence; not an emulation of Xbox Live presence flags.

Only recent authenticated requests or a live, challenged match connection may
publish presence. Account admission by itself is not a fifteen-minute heartbeat.
"""
import time

ACTIVITY_SECONDS = 30


def recent_sessions(directory):
    for api in (directory.clans, directory.matchmaking):
        now = api.clock()
        for session in api.sessions.values():
            if session.expires > now and 0 <= now - session.seen < ACTIVITY_SECONDS:
                yield session


def connected_seats(directory):
    """A connection owns its verified couch seats, including transient guests."""
    transport = getattr(directory, "match_transport", None)
    now = time.monotonic()
    for room in transport.rooms.values() if transport else ():
        if room.closed or now >= room.deadline:
            continue
        for who, peer in room.peers.items():
            if peer.socket.closed or (peer.challenge and now - peer.sent > 15):
                continue
            for player, owner in room.owners.items():
                if owner == who:
                    yield room, player, player in room.guests


def observed(directory):
    result = {}
    for session in recent_sessions(directory):
        result[session.player] = "online"
        if hasattr(session, "seats"):
            for player in session.seats():
                if not player.guest:
                    result[player.player] = "online"
    for room, player, guest in connected_seats(directory):
        if not guest:
            result[player] = "playing" if room.phase == "running" else "online"
    return result


STATES = ("idle", "postgame", "queued", "joining", "playing")


def population(directory):
    """Count observed accounts once; publish counts only, never an online roster.

    Ticket leases alone are not presence. Proposals reserve joining seats until
    the match connection takes over. A connected room takes priority over a
    lingering queue ticket, so the queue-to-match handoff cannot double count.
    """
    online, guests, activity = set(), set(), {}
    connections = list(connected_seats(directory))
    for session in recent_sessions(directory):
        online.add(session.player)
        if hasattr(session, "seats"):
            for seat in session.seats():
                (guests if seat.guest else online).add(seat.player)
    for _, player, guest in connections:
        (guests if guest else online).add(player)
    guests.difference_update(online)
    present = online | guests

    def assign(player, playlist, state):
        previous = activity.get(player)
        if player in present and (previous is None or STATES.index(state) > STATES.index(previous[1])):
            activity[player] = playlist, state

    api = directory.matchmaking
    now = api.clock()
    if api.core:
        reserved = {ticket for proposal in api.core.proposals.values() if proposal.expires_at > now
                    for ticket in proposal.tickets}
        for ticket in api.core.tickets.values():
            if ticket.expires_at > now:
                for player in ticket.players:
                    assign(player.id, ticket.playlist, "joining" if ticket.id in reserved else "queued")
    for room, player, _ in connections:
        state = ("postgame" if room.phase in ("reviewing", "postgame") else
                 "joining" if room.phase in ("connecting", "loading") else "playing")
        # The transport, rather than a proposal's shorter admission lease, owns
        # presence during loading, gameplay, migration and postgame review.
        assign(player, room.proposal.playlist, state)

    def counters():
        return {key: {"users": 0, "guests": 0} for key in ("online", *STATES)}

    totals = counters()
    playlists = {entry.policy.id: {"id": entry.policy.id, "name": entry.name, **counters()}
                 for entry in api.entries}
    for player in present:
        kind = "guests" if player in guests else "users"
        playlist, state = activity.get(player, (None, "idle"))
        totals["online"][kind] += 1
        totals[state][kind] += 1
        if playlist in playlists:
            row = playlists[playlist]
            row["online"][kind] += 1
            row[state][kind] += 1
    return {"updated_at": directory.store.now(), "activity_window_seconds": ACTIVITY_SECONDS,
            **totals, "playlists": list(playlists.values())}
