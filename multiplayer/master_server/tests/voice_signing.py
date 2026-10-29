"""Independent Python signer exercises the production Rust voice verifier."""
import json
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey


def voice_message(room_id, epoch, body, recipient):
    return b'H2VOICE2\0' + room_id.encode('ascii') + epoch.to_bytes(4,'little') + bytes([recipient]) + body


def sign_voice(room, body, owner=None, epoch=0, recipient=None, room_id=None):
    if owner is None:
        owner = room.owners[room.players[body[8]][0]]
    seed = bytes.fromhex(json.loads(room.voice_auth(owner))['voice_auth']['seed'])
    key = Ed25519PrivateKey.from_private_bytes(seed)
    return body + key.sign(voice_message(room.id if room_id is None else room_id,
        epoch, body, room.host if recipient is None else recipient))
