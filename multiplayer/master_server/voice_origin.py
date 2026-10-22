"""Origin proof survives host mixing without trusting its claimed speaker."""
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey
from cryptography.exceptions import InvalidSignature
from .protocol import require


def voice_message(room_id, epoch, body):
    # The host may mix gains/radio delivery, but cannot change the speaker,
    # sequence, codec bytes, native protocol, room or authority generation.
    return b'H2VOICE1\0' + room_id.encode('ascii') + epoch.to_bytes(4, 'little') + body[:13] + body[22:]


def verify_voice(room, epoch, packet):
    require(22 + 64 < len(packet) <= 278 + 64, 'Voice origin proof required')
    body, signature = packet[:-64], packet[-64:]
    speaker = body[8]
    require(speaker < len(room.players), 'Invalid voice identity')
    owner = room.owners[room.players[speaker][0]]
    require(owner in room.tokens, 'Departed voice identity')
    try:
        room.voice_keys[owner].public_key().verify(signature, voice_message(room.id, epoch, body))
    except InvalidSignature:
        require(False, 'Invalid voice origin proof')
    return body
