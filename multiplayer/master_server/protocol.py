"""Version-one signing and validation shared by masters and the simulator."""
import json
import ipaddress
import os
import re
import uuid
from pathlib import Path

from cryptography.exceptions import InvalidSignature
from cryptography.hazmat.primitives.asymmetric.ed25519 import Ed25519PrivateKey, Ed25519PublicKey
from cryptography.hazmat.primitives.serialization import Encoding, PrivateFormat, PublicFormat, NoEncryption

VERSION = 1
MAX_BODY = 16 * 1024
MAX_SYNC = 4 * 1024 * 1024
MAX_RECORDS = 1000
MAX_LEASE = 180
SKEW = 30
DOMAIN = b"halo2-directory-v1\n"


class Problem(Exception):
    def __init__(self, status, message):
        self.status, self.message = status, message
        super().__init__(message)


def require(condition, message="Invalid request", status=400):
    if not condition:
        raise Problem(status, message)


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True, allow_nan=False).encode()


def parse(raw):
    def pairs(items):
        result = {}
        for key, value in items:
            require(key not in result, "Duplicate JSON key")
            result[key] = value
        return result
    try:
        return json.loads(raw, object_pairs_hook=pairs,
                          parse_constant=lambda _: require(False, "Non-finite JSON number"))
    except (ValueError, UnicodeError, RecursionError):
        raise Problem(400, "Invalid JSON") from None


def fields(value, required, optional=()):
    require(type(value) is dict, "Expected an object")
    require(set(required) <= value.keys() <= set(required) | set(optional), "Unexpected or missing fields")


def integer(value, low, high):
    require(type(value) is int and low <= value <= high, "Integer outside allowed range")
    return value


def text(value, maximum=64, empty=False):
    require(type(value) is str, "Invalid text")
    try:
        encoded = value.encode("utf-8")
    except UnicodeError:
        raise Problem(400, "Invalid Unicode text") from None
    require((empty or bool(value)) and len(encoded) <= maximum,
            "Invalid text length")
    require(all(ord(c) >= 32 and ord(c) != 127 for c in value), "Control character in text")
    return value


def identifier(value):
    try:
        require(type(value) is str and str(uuid.UUID(value)) == value, "Invalid UUID")
    except (ValueError, AttributeError):
        raise Problem(400, "Invalid UUID") from None
    return value


def public_key(value):
    require(type(value) is str and bool(re.fullmatch("[0-9a-f]{64}", value)), "Invalid Ed25519 public key")
    return value


def master_id(key):
    return str(uuid.uuid5(uuid.NAMESPACE_URL, "halo2-master:" + public_key(key)))


def server_id(key):
    # One key per running server instance, independent of its address or owner.
    return str(uuid.uuid5(uuid.NAMESPACE_URL, "halo2-server:" + public_key(key)))


def public_hex(key):
    return key.public_key().public_bytes(Encoding.Raw, PublicFormat.Raw).hex()


def save_private(path, value, exclusive=False):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    if exclusive:
        fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(fd, "wb") as output:
            output.write(canonical(value) + b"\n")
            output.flush()
            os.fsync(output.fileno())
        return
    temporary = path.with_name(path.name + "." + uuid.uuid4().hex + ".tmp")
    try:
        save_private(temporary, value, True)
        os.replace(temporary, path)
    finally:
        temporary.unlink(missing_ok=True)


def load_key(path):
    path = Path(path)
    if not path.exists():
        key = Ed25519PrivateKey.generate()
        seed = key.private_bytes(Encoding.Raw, PrivateFormat.Raw, NoEncryption()).hex()
        save_private(path, {"private_key": seed}, True)
    value = parse(path.read_bytes())
    return Ed25519PrivateKey.from_private_bytes(bytes.fromhex(value["private_key"]))


def signed(key, payload):
    return {"payload": payload, "signature": key.sign(DOMAIN + canonical(payload)).hex()}


def verified(envelope, key):
    fields(envelope, ("payload", "signature"))
    try:
        signature = envelope["signature"]
        require(type(signature) is str and bool(re.fullmatch("[0-9a-f]{128}", signature)), "Invalid signature", 401)
        Ed25519PublicKey.from_public_bytes(bytes.fromhex(public_key(key))).verify(
            bytes.fromhex(signature), DOMAIN + canonical(envelope["payload"]))
    except (InvalidSignature, ValueError, TypeError):
        raise Problem(401, "Invalid signature") from None
    return envelope["payload"]


def metadata(value):
    fields(value, ("name", "host", "port", "map", "gametype", "players", "max_players",
                   "game_version", "visibility", "server_type"), ("region",))
    for name in ("name", "map", "gametype", "game_version"):
        text(value[name])
    host = text(value["host"], 253)
    try:
        address = ipaddress.ip_address(host)
        require(not (address.is_unspecified or address.is_multicast), "Invalid endpoint address")
    except ValueError:
        require(bool(re.fullmatch(r"(?=.{1,253}$)(?:[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?\.)*[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?", host)), "Invalid endpoint hostname")
    integer(value["port"], 1, 65535)
    maximum = integer(value["max_players"], 1, 16)
    integer(value["players"], 0, maximum)
    require(value["visibility"] in ("public", "private"), "Invalid visibility")
    require(value["server_type"] in ("dedicated", "player-hosted"), "Invalid server type")
    if "region" in value:
        require(type(value["region"]) is str and bool(re.fullmatch("[A-Z0-9-]{2,16}", value["region"])), "Invalid operator-declared region")
    require(len(canonical(value)) <= 2048, "Metadata too large", 413)
    return value.copy()
