"""Verified connection-owned local seats. Guests never become stored accounts."""
from dataclasses import dataclass

from .player_identity import VerifiedOwner

NATIVE_CONTINUITY = "halo2-native-queue-v11"
NATIVE_DIRECT = "halo2-native-queue-v12"
NATIVE_EOS = "halo2-native-queue-v13"
NATIVE_EOS_ACCOUNT = "halo2-native-queue-v14"
NATIVE_HOPPERS = "halo2-native-queue-v15"
NATIVE_ACCURACY = "halo2-native-queue-v16"
NATIVE_OPTIONS = (NATIVE_HOPPERS,NATIVE_ACCURACY)
NATIVE_COUCH = ("halo2-native-queue-v9", "halo2-native-queue-v10", NATIVE_CONTINUITY, NATIVE_DIRECT, NATIVE_EOS, NATIVE_EOS_ACCOUNT, *NATIVE_OPTIONS)
NATIVE_GATHERING = ("halo2-native-queue-v8", *NATIVE_COUCH)


def supports_compatibility(actual, required):
    from .native_matchmaking import value
    return value('catalog_supports', actual=actual, required=required)


def same_continuity_protocol(connections):
    from .native_matchmaking import value
    return value('catalog_continuity', versions=[member.compatibility for member in connections if member])


@dataclass(frozen=True)
class LocalPlayer:
    player: str
    screen_name: str
    guest: bool = False
    appearance: bytes | None = None  # Exact current nine-byte local profile, session-only.


async def verify_local_players(directory, plan):
    """Execute the provider proof IO requested by the Rust session parser."""
    from .native_matchmaking import value
    result = []
    for row in plan['locals']:
        owner, username = None, ''
        if row['proof'] is not None:
            verified = await directory.verify_identity(row['proof'])
            if isinstance(verified, VerifiedOwner):
                account, username = (verified.provider, verified.subject), verified.username
            else:
                account = verified
            owner = tuple(value('session_verified_identity', owner=account))
            directory.rate('owner-id:' + ':'.join(owner), 20)
        result.append((row['screen_name'], owner, username))
    return result
