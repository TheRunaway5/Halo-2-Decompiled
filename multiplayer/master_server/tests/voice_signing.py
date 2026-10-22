from multiplayer.master_server.voice_origin import voice_message


def sign_voice(room, body, owner=None, epoch=0):
    if owner is None:
        owner = room.owners[room.players[body[8]][0]]
    return body + room.voice_keys[owner].sign(voice_message(room.id, epoch, body))
