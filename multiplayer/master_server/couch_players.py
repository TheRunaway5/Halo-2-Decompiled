"""Verified connection-owned local seats. Guests never become stored accounts."""
from dataclasses import dataclass
import secrets

from .player_identity import VerifiedOwner
from .protocol import fields, require, text

NATIVE_CONTINUITY = "halo2-native-queue-v11"
NATIVE_DIRECT = "halo2-native-queue-v12"
NATIVE_EOS = "halo2-native-queue-v13"
NATIVE_EOS_ACCOUNT = "halo2-native-queue-v14"
NATIVE_COUCH = ("halo2-native-queue-v9", "halo2-native-queue-v10", NATIVE_CONTINUITY, NATIVE_DIRECT, NATIVE_EOS, NATIVE_EOS_ACCOUNT)
NATIVE_GATHERING = ("halo2-native-queue-v8", *NATIVE_COUCH)


def supports_compatibility(actual, required):
    return actual == required or (required == "halo2-native-queue-v10" and actual in (NATIVE_CONTINUITY, NATIVE_DIRECT, NATIVE_EOS, NATIVE_EOS_ACCOUNT))


def same_continuity_protocol(connections):
    versions = [member.compatibility for member in connections if member]
    for exclusive in (NATIVE_CONTINUITY, NATIVE_DIRECT, NATIVE_EOS, NATIVE_EOS_ACCOUNT):
        if exclusive in versions:
            return all(version == exclusive for version in versions)
    return True


@dataclass(frozen=True)
class LocalPlayer:
    player: str
    screen_name: str
    guest: bool = False
    appearance: bytes | None = None  # Exact current nine-byte local profile, session-only.


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
    def canonical(owner):
        row = directory.store.db.execute("SELECT player FROM player_identities WHERE provider=? AND subject=?", owner).fetchone()
        return ("player", row[0]) if row else owner
    owners, result = {canonical(primary)}, []
    for row in rows:
        fields(row, ("screen_name",), ("discord_token", "identity_token", "appearance"))
        if "appearance" in row:
            from .player_profile import appearance
            appearance(row["appearance"])
        name = screen_name(row["screen_name"])
        owner, username = None, ""
        require(not ("discord_token" in row and "identity_token" in row), "Supply one identity proof per couch player")
        if "discord_token" in row or "identity_token" in row:
            token = text(row.get("identity_token", row.get("discord_token")), maximum=2048)
            verified = await directory.verify_identity(token)
            if isinstance(verified, VerifiedOwner):
                owner, username = (verified.provider, verified.subject), verified.username
            else:
                owner = tuple(verified)
            require(len(owner) == 2 and owner[0] in ("discord", "steam"), "Verified account identity required", 401)
            require(canonical(owner) not in owners, "Each signed-in couch player needs a different account")
            directory.rate("owner-id:" + ":".join(owner), 20)
            owners.add(canonical(owner))
        result.append((name, owner, username))
    return result


def guest_id():
    return "guest-" + secrets.token_hex(16)
