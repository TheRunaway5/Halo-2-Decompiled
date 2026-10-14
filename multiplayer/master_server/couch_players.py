"""Verified connection-owned local seats. Guests never become stored accounts."""
from dataclasses import dataclass
import secrets

from .player_identity import VerifiedOwner
from .protocol import fields, require, text

NATIVE_CONTINUITY = "halo2-native-queue-v11"
NATIVE_DIRECT = "halo2-native-queue-v12"
NATIVE_COUCH = ("halo2-native-queue-v9", "halo2-native-queue-v10", NATIVE_CONTINUITY, NATIVE_DIRECT)
NATIVE_GATHERING = ("halo2-native-queue-v8", *NATIVE_COUCH)


def supports_compatibility(actual, required):
    return actual == required or (required == "halo2-native-queue-v10" and actual in (NATIVE_CONTINUITY, NATIVE_DIRECT))


def same_continuity_protocol(connections):
    versions = [member.compatibility for member in connections if member]
    for exclusive in (NATIVE_CONTINUITY, NATIVE_DIRECT):
        if exclusive in versions:
            return all(version == exclusive for version in versions)
    return True


@dataclass(frozen=True)
class LocalPlayer:
    player: str
    screen_name: str
    guest: bool = False


def screen_name(value):
    name = text(value)
    try:
        encoded = name.encode("utf-16-le")
    except UnicodeError:
        require(False, "Screen name contains invalid Unicode")
    require(len(encoded) <= 62, "Screen name must fit 31 UTF-16 characters")
    return name


async def verify_local_players(directory, body, primary):
    """Validate every proof before creating any persistent account record."""
    rows = body.get("local_players", [])
    require(type(rows) is list and len(rows) <= 3, "ONLINE supports at most four local players")
    require(not rows or body["compatibility"] in NATIVE_COUCH,
            "Update Halo 2 for ONLINE split-screen", 409)
    owners, result = {primary}, []
    for row in rows:
        fields(row, ("screen_name",), ("discord_token",))
        name = screen_name(row["screen_name"])
        owner, username = None, ""
        if "discord_token" in row:
            token = text(row["discord_token"], maximum=2048)
            verified = await directory.verify_owner(directory.session, token, directory.settings.discord_client_id)
            if isinstance(verified, VerifiedOwner):
                owner, username = (verified.provider, verified.subject), verified.username
            else:
                owner = tuple(verified)
            require(len(owner) == 2 and owner[0] == "discord", "Discord identity required", 401)
            require(owner not in owners, "Each signed-in couch player needs a different Discord account")
            directory.rate("owner-id:" + ":".join(owner), 20)
            owners.add(owner)
        result.append((name, owner, username))
    return result


def guest_id():
    return "guest-" + secrets.token_hex(16)
