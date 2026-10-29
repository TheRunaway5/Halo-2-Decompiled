"""Additive prepared-map owner metadata publication, never program conversion.

Call only inside the producer's private stage or for an already verified private
cache. Final encoded maps are hashed here without decoding. Existing bank program,
map, tag and data bytes are never written. A provenance pointer authenticates the
index; it is not gameplay, shader-program or full TU1.5 readiness evidence.
"""
import copy
import ctypes
from dataclasses import dataclass
import hashlib
import json
import os
from pathlib import Path, PurePosixPath, PureWindowsPath
import stat
import sys
import uuid

from audit_rendering import FormatError
import mcc_native_map_owners as owners
from native_shader_bundle import TABLE_SHA, fingerprint
from file_identity import descriptor_identity, path_identity

INDEX = "native-shaders/map-owners.json"
POLICIES = Path(__file__).resolve().parent / "data"
METADATA_LIMIT = 16 * 1024 * 1024


def _require(value, message):
    if not value:
        raise FormatError("MCC map-owner publication: " + message)


def _encoded(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False,
                      allow_nan=False).encode("utf-8") + b"\n"


def _sha(data):
    return hashlib.sha256(data).hexdigest()


def _json(data):
    def pairs(values):
        result = {}
        for key, value in values:
            _require(key not in result, "duplicate JSON key: " + key)
            result[key] = value
        return result
    try:
        return json.loads(data.decode("utf-8"), object_pairs_hook=pairs,
                          parse_constant=lambda _: (_require(False, "nonfinite metadata")))
    except FormatError:
        raise
    except (ValueError, UnicodeError) as error:
        raise FormatError("MCC map-owner publication: malformed JSON metadata") from error


def _relative(value):
    _require(isinstance(value, str) and value and "\\" not in value and "\0" not in value and
             not PureWindowsPath(value).drive and all(part not in ("", ".", "..") for part in value.split("/")),
             "invalid exact relative owner path")
    path = PurePosixPath(value)
    _require(not path.is_absolute() and path.as_posix() == value, "aliased owner path")
    return path


def _root(value):
    path = Path(value)
    _require(path.is_absolute() and path.is_dir() and path.resolve(strict=True) == path,
             "private root must be exact, absolute and have no symlink components")
    return path


def _path(root, name, *, directory=False, missing=False):
    relative = _relative(name)
    path = root
    for index, part in enumerate(relative.parts):
        entries = {entry.name for entry in path.iterdir()}
        final = index == len(relative.parts) - 1
        if part not in entries:
            _require(missing and final, "missing exact owner/case: " + name)
            return path / part
        path /= part
        _require(not path.is_symlink(), "symlink in private owner: " + name)
        _require(path.is_dir() if not final or directory else path.is_file(), "wrong private owner type: " + name)
    _require(path.resolve(strict=True).is_relative_to(root), "owner escaped private root")
    return path


def _stamp(path):
    value = path.lstat()
    _require(stat.S_ISREG(value.st_mode), "file lost regular private owner")
    return path_identity(path)


def _read(root, name, pins, *, payload=False):
    path = _path(root, name)
    before = _stamp(path)
    _require(payload or before[2] <= METADATA_LIMIT, "metadata exceeds finite bound")
    descriptor = os.open(path, os.O_RDONLY | getattr(os, "O_CLOEXEC", 0) | getattr(os, "O_NOFOLLOW", 0))
    digest, data, consumed = hashlib.sha256(), bytearray(), 0
    with os.fdopen(descriptor, "rb") as stream:
        first = descriptor_identity(stream.fileno())
        _require(first == before,
                 "owner changed while opening")
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
            consumed += len(block)
            if not payload:
                data.extend(block)
                _require(len(data) <= METADATA_LIMIT, "metadata grew beyond finite bound")
        after = descriptor_identity(stream.fileno())
        _require(after == before,
                 "owner changed while reading")
    _require(consumed == before[2] and _path(root, name) == path and _stamp(path) == before,
             "owner changed or read was incomplete during verification")
    pin = dict(bytes=before[2], sha256=digest.hexdigest())
    pins.append((root, name, before))
    return bytes(data), pin


def _unchanged(pins):
    for root, name, expected in pins:
        _require(_stamp(_path(root, name)) == expected, "input changed during metadata publication: " + name)


@dataclass(frozen=True)
class VerifiedOwnerIndex:
    """Read-only verification beforeimage for a later legitimate closure refresh.

    Capture before changing retained closure metadata. Refresh still requires the
    exact same old pointer/index bytes; custom source/final/bank proofs cannot be
    changed or erased using this token. It grants no payload mutation authority.
    """
    cache: Path
    pointer_bytes: bytes
    index_bytes: bytes


@dataclass(frozen=True)
class OwnerIndexRefresh:
    """Prepared metadata pair; the enclosing shader transaction owns commit.

    This is an in-process plan, not a serialized authority token. Inputs include
    actual final encoded maps and current bank metadata. The enclosing caller
    must check the beforeimage immediately before replacing either metadata file
    and retain all backups until check_inputs succeeds after the final write.
    """
    cache: Path
    previous_provenance: bytes
    previous_index: bytes | None
    index_bytes: bytes
    provenance_bytes: bytes
    updated_provenance: dict
    input_pins: tuple

    def check_inputs(self):
        _unchanged(self.input_pins)

    def check_beforeimage(self):
        self.check_inputs()
        old, _ = _read(self.cache, "provenance.json", [])
        _require(old == self.previous_provenance, "refresh provenance beforeimage changed")
        _, index = _existing(self.cache, _json(old), [])
        _require(index == self.previous_index, "refresh index beforeimage changed")


def _existing(cache, provenance, pins):
    native = provenance.get("native_shader_library", {})
    pointer = native.get("owner_bindings") if isinstance(native, dict) else None
    if pointer is None:
        directory = _path(cache, "native-shaders", directory=True, missing=True)
        if not directory.exists():
            return None, None
        path = _path(cache, INDEX, missing=True)
        _require(not path.exists() and not path.is_symlink(), "unrecorded existing owner index; retained unchanged")
        return None, None
    _require(isinstance(pointer, dict) and set(pointer) == {"schema", "path", "bytes", "sha256"} and
             pointer.get("schema") == owners.SCHEMA and pointer.get("path") == INDEX and
             type(pointer.get("bytes")) is int and pointer["bytes"] >= 0,
             "corrupt declared owner pointer")
    raw, identity = _read(cache, INDEX, pins)
    _require(identity == {key: pointer[key] for key in ("bytes", "sha256")}, "declared owner pointer fingerprint differs")
    return owners.decode_index(raw), raw


def _prepare(cache, provenance, custom_receipts, policy_root):
    cache, policy_root = _root(cache), _root(policy_root)
    pins = []
    _require(isinstance(provenance, dict), "invalid provenance")
    previous, previous_raw = _existing(cache, provenance, pins)
    # Index/provenance are the only publication targets. Their exact beforeimage
    # is checked separately; all other input file identities remain pinned.
    pins = [row for row in pins if row[1] != INDEX]
    raw_conversion, conversion_pin = _read(cache, "tag-conversion.json", pins)
    conversion = _json(raw_conversion)
    records = provenance.get("source_files")
    _require(isinstance(records, dict), "missing copied-source inventory")
    verified = {}
    for name in records:
        relative = _relative(name)
        if relative.suffix.lower() == ".map":
            _, verified["halo2/" + name] = _read(cache, "halo2/" + name, pins, payload=True)
    final = owners.derive_final_inputs(provenance, conversion, verified)
    raw_catalog, catalog_pin = _read(cache, "native-shaders/catalog.json", pins)
    catalog = _json(raw_catalog)
    banks = {}

    def bank(directory):
        _require(_relative(directory).parts[0] == "native-shaders" and directory != "native-shaders",
                 "native bank lost private directory owner")
        if directory not in banks:
            raw, _ = _read(cache, directory + "/library.json", pins)
            banks[directory] = owners.bank_identity(directory, raw)
        return banks[directory]

    native = provenance.get("native_shader_library")
    _require(isinstance(native, dict) and isinstance(native.get("maps"), dict), "missing retained stock bank proofs")
    for name in native["maps"]:
        # This resolves only explicit retained native catalog proofs, never a
        # nested/custom map's leaf name as a new owner.
        _require(isinstance(name, str) and "/" not in name and "\\" not in name, "invalid retained namespace")
        bank("native-shaders/" + name)
    raw_closure, closure_pin = _read(policy_root, "mcc-native-library-closure.json", pins)
    closure = _json(raw_closure)
    _require(closure.get("schema") == "H2_MCC_RETAINED_NATIVE_LIBRARY_CLOSURE_V1" and
             isinstance(closure.get("libraries"), list), "invalid shipped closure policy")
    source_name, canonical_source = closure.get("source_map"), closure.get("source_canonical_sha256")
    _require(isinstance(source_name, str) and source_name and "/" not in source_name and "\\" not in source_name and
             fingerprint(canonical_source),
             "invalid common dependency namespace")
    dependencies = []
    canonical_recipients = {}
    for row in closure["libraries"]:
        _require(isinstance(row, dict) and isinstance(row.get("map"), str) and
                 row["map"] and "/" not in row["map"] and "\\" not in row["map"] and
                 row["map"] not in canonical_recipients and isinstance(row.get("missing"), list) and
                 fingerprint(row.get("canonical_sha256")),
                 "invalid/duplicate closure recipient")
        canonical_recipients[row["map"]] = row
    for owner, identity in final.items():
        if not identity["stock_conversion_owner"] or identity["cache_role"] != "scenario":
            continue
        name = PurePosixPath(owner).stem
        retained = banks.get("native-shaders/" + name)
        policy = canonical_recipients.get(name)
        if retained and policy and policy["missing"] and retained["canonical_sha256"] == policy.get("canonical_sha256"):
            source = bank("native-shaders/" + source_name)
            _require(source["bank_namespace"] == source_name and source["canonical_sha256"] == canonical_source,
                     "common dependency is missing or customized")
            dependencies.append(dict(kind="authenticated-common-templates",
                **{key: source[key] for key in owners.BANK_FIELDS}, canonical_sha256=canonical_source,
                authority_sha256=closure_pin["sha256"]))
    raw_hud, hud_pin = _read(policy_root, "mcc-native-hud-bootstrap.json", pins)
    hud = _json(raw_hud)
    _require(hud.get("schema") == owners.HUD_SCHEMA and hud.get("source_map") == source_name and
             hud.get("source_canonical_sha256") == canonical_source, "HUD/common dependency policy source differs")
    _, tables = _read(cache, "native-shaders/function-tables.bin", pins, payload=True)
    _require(tables == dict(bytes=28 * 1024, sha256=TABLE_SHA), "common scalar tables differ")
    dependencies.append(dict(kind="scalar-function-tables", path="native-shaders/function-tables.bin", **tables))
    receipt = dict(source_files_sha256=_sha(_encoded(records)), conversion_file=conversion_pin,
                   ordered_output_chain_sha256=_sha(_encoded([provenance.get("output_changes", []),
                       provenance.get("graphics_corrections", {}).get("changes", [])])),
                   catalog_file=catalog_pin, retained_bank_proofs=native["maps"],
                   closure_policy=closure_pin, HUD_policy=hud_pin)
    authority = dict(producer="mcc_native_map_owner_publication", profile=owners.PROFILE,
                     receipt_sha256=_sha(_encoded(receipt)))
    index = owners.build_index(provenance, conversion, verified, catalog, banks,
        compatibility_authority=authority, hud_bootstrap=raw_hud, dependencies=dependencies)
    # Retain previously authenticated explicit owners only while all their
    # actual source/final and current bank proofs remain unchanged.
    if previous is not None:
        base = {row["map"]: row for row in index["maps"]}
        custom = [row for row in previous["maps"] if row["compatibility_authority"]["scope"] == "explicit-custom-owner"]
        if custom:
            _require(previous["input_authority"] == index["input_authority"], "custom binding input authority changed; explicit producer refresh required")
        for row in custom:
            target = base.get(row["map"])
            _require(target is not None and all(row[key] == target[key] for key in
                ("input_bytes", "input_sha256", "source_sha256")), "custom bound map changed; retained index unchanged")
            _path(cache, row["shared_directory"], directory=True)
            if row["native_mode"] == "owned-bank":
                current = bank(row["bank_directory"])
                _require(all(row[key] == current[key] for key in owners.BANK_FIELDS), "custom bank changed; retained index unchanged")
            base[row["map"]] = copy.deepcopy(row)
        index["maps"] = [base[name] for name in sorted(base)]
    for raw in custom_receipts:
        _require(isinstance(raw, bytes) and len(raw) <= METADATA_LIMIT, "custom receipt must be bounded exact bytes")
        receipt = _json(raw)
        _require(isinstance(receipt, dict) and receipt.get("native_mode") in ("none", "owned-bank"), "invalid explicit custom receipt")
        current = bank(receipt["bank_directory"]) if receipt["native_mode"] == "owned-bank" else None
        index = owners.add_owned_map_binding(index, receipt["map"], verified[receipt["map"]],
            cache_role=receipt["cache_role"], shared_directory=receipt["shared_directory"],
            owned_bank=current, producer_receipt=raw)
    for row in index["maps"]:
        _path(cache, row["shared_directory"], directory=True)
    raw_index = owners.encode_index(index)
    _require(len(raw_index) <= METADATA_LIMIT, "owner index exceeds finite metadata bound")
    updated = copy.deepcopy(provenance)
    updated["native_shader_library"]["owner_bindings"] = dict(schema=owners.SCHEMA, path=INDEX,
        bytes=len(raw_index), sha256=_sha(raw_index))
    _unchanged(pins)
    return cache, index, raw_index, updated, previous_raw, pins


def derive_owner_index(cache, provenance, *, custom_receipts=(), policy_root=POLICIES):
    """Derive final input/bank metadata without writing any file.

    Explicit custom receipt bytes alone may create a new custom binding. Existing
    pointer-authenticated custom rows are never inferred, changed or dropped.
    """
    _, index, _, updated, _, _ = _prepare(cache, provenance, custom_receipts, policy_root)
    return index, updated


def verify_owner_index(cache, provenance, *, policy_root=POLICIES):
    """Read-only verification; absent legacy index returns None without repair.

    A declared corrupt/stale pointer or binding fails. The returned beforeimage
    may be captured before an independently authorized closure metadata repair.
    """
    cache = _root(cache)
    previous, raw = _existing(cache, provenance, [])
    if previous is None:
        return None
    _, expected, _, _, _, _ = _prepare(cache, provenance, (), policy_root)
    owners.validate_index(previous, expected)
    return VerifiedOwnerIndex(cache, _encoded(provenance["native_shader_library"]["owner_bindings"]), raw)


def prepare_owner_index_refresh(cache, provenance, candidate_provenance, *,
                                verified_beforeimage, policy_root=POLICIES):
    """Derive a pair while old provenance/index remain installed.

    The caller authenticates an existing index BEFORE changing closure inputs,
    then temporarily installs only its validated catalog/common dependencies
    inside one rollback boundary. This function performs no publication. An
    absent legacy index needs no token; a declared index always needs its exact
    previously verified token. The candidate may change only native-library
    metadata, preserving all source/conversion/graphics and readiness fields.
    """
    cache = _root(cache)
    old_provenance, _ = _read(cache, "provenance.json", [])
    _require(_json(old_provenance) == provenance, "refresh disk provenance differs from beforeimage")
    _, previous = _existing(cache, provenance, [])
    if previous is None:
        _require(verified_beforeimage is None, "unexpected indexed beforeimage for legacy cache")
    else:
        token = verified_beforeimage
        _require(isinstance(token, VerifiedOwnerIndex) and token.cache == cache and
                 token.index_bytes == previous and token.pointer_bytes ==
                 _encoded(provenance["native_shader_library"]["owner_bindings"]),
                 "refresh requires unchanged authenticated index beforeimage")
    old_core = {key: value for key, value in provenance.items() if key != "native_shader_library"}
    next_core = {key: value for key, value in candidate_provenance.items() if key != "native_shader_library"}
    _require(old_core == next_core, "closure refresh cannot change map inputs or readiness metadata")
    old_pointer = provenance.get("native_shader_library", {}).get("owner_bindings")
    _require(candidate_provenance.get("native_shader_library", {}).get("owner_bindings") == old_pointer,
             "candidate replaced authenticated owner pointer")
    _, _, raw, updated, observed, pins = _prepare(cache, candidate_provenance, (), policy_root)
    _require(observed == previous, "refresh index changed during derivation")
    plan = OwnerIndexRefresh(cache, old_provenance, previous, raw, _encoded(updated), updated, tuple(pins))
    plan.check_beforeimage()
    return plan


def _temporary(path, data):
    with path.open("xb") as stream:
        stream.write(data)
        stream.flush()


def _publish_absent(source, destination):
    """Atomic no-overwrite rename, including filesystems without hard links."""
    try:
        if os.name == "nt":
            os.rename(source, destination)  # Windows refuses an existing target.
        elif sys.platform == "darwin":
            from macos.atomic_files import rename_exclusive
            rename_exclusive(source, destination)
        else:
            library = ctypes.CDLL(None, use_errno=True)
            rename = getattr(library, "renameat2", None)
            _require(rename is not None, "atomic absent-only publication requires renameat2 on this platform")
            rename.argtypes = (ctypes.c_int, ctypes.c_char_p, ctypes.c_int, ctypes.c_char_p, ctypes.c_uint)
            rename.restype = ctypes.c_int
            if rename(-100, os.fsencode(source), -100, os.fsencode(destination), 1):
                code = ctypes.get_errno()
                raise OSError(code, os.strerror(code), str(destination))
    except FileExistsError as error:
        raise FormatError("MCC map-owner publication: destination changed; existing destination retained") from error


def publish_staged_owner_index(cache, provenance, *, custom_receipts=(), policy_root=POLICIES):
    """Publish one new stage index; caller's stage/provenance rename owns commit.

    Existing matching stage metadata is idempotent; a different index is never
    replaced by this fresh-stage API. No provenance file is written here.
    """
    cache, index, raw, updated, previous, pins = _prepare(cache, provenance, custom_receipts, policy_root)
    if previous is not None:
        _require(previous == raw, "existing stage index differs; explicit verified repair required")
        return updated
    target = _path(cache, INDEX, missing=True)
    temporary = target.with_name(".map-owners-" + uuid.uuid4().hex + ".tmp")
    try:
        _temporary(temporary, raw)
        _unchanged(pins)
        _publish_absent(temporary, target)
    finally:
        temporary.unlink(missing_ok=True)
    return updated


def repair_owner_index(cache, provenance, *, custom_receipts=(), policy_root=POLICIES,
                       verified_beforeimage=None):
    """Rollback-safe replacement of only the index and provenance metadata.

    Without a verified-beforeimage token, existing declared metadata must verify
    against current files before repair. A token supports legitimate closure
    refresh only if its exact old pointer/index still match. Publication uses two
    atomic file replacements with rollback, not a cross-file crash-atomic claim.
    """
    cache = _root(cache)
    if verified_beforeimage is None:
        verified_beforeimage = verify_owner_index(cache, provenance, policy_root=policy_root)
    else:
        _require(isinstance(verified_beforeimage, VerifiedOwnerIndex) and verified_beforeimage.cache == cache,
                 "beforeimage belongs to another private cache")
        _, old = _existing(cache, provenance, [])
        pointer = provenance.get("native_shader_library", {}).get("owner_bindings")
        _require(old == verified_beforeimage.index_bytes and _encoded(pointer) == verified_beforeimage.pointer_bytes,
                 "verified beforeimage pointer/index changed")
    _, index, raw, updated, previous, pins = _prepare(cache, provenance, custom_receipts, policy_root)
    old_provenance, _ = _read(cache, "provenance.json", [])
    _require(_json(old_provenance) == provenance, "provenance changed outside this metadata repair")
    if previous == raw and updated == provenance:
        return cache, copy.deepcopy(provenance)
    target = _path(cache, INDEX, missing=True)
    provenance_path = _path(cache, "provenance.json")
    identifier = uuid.uuid4().hex
    new_index = target.with_name(".map-owners-" + identifier + ".tmp")
    new_provenance = cache / (".map-owner-provenance-" + identifier + ".tmp")
    old_index = target.with_name(".map-owners-" + identifier + ".previous")
    backup_provenance = cache / (".map-owner-provenance-" + identifier + ".previous")
    index_written = provenance_written = False
    try:
        _temporary(new_index, raw)
        _temporary(new_provenance, _encoded(updated))
        if previous is not None:
            _temporary(old_index, previous)
        _temporary(backup_provenance, old_provenance)
        _unchanged(pins)
        _, current = _existing(cache, provenance, [])
        _require(current == previous and _read(cache, "provenance.json", [])[0] == old_provenance,
                 "publication beforeimage changed")
        os.replace(new_index, target)
        index_written = True
        os.replace(new_provenance, provenance_path)
        provenance_written = True
        _unchanged(pins)
    except BaseException:
        if provenance_written:
            os.replace(backup_provenance, provenance_path)
        if index_written:
            if previous is None:
                target.unlink()
            else:
                os.replace(old_index, target)
        raise
    finally:
        for path in (new_index, new_provenance, old_index, backup_provenance):
            path.unlink(missing_ok=True)
    return cache, updated
