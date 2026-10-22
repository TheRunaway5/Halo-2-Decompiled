"""Pure metadata contract for exact prepared-map/native-bank ownership.

``verified_inputs`` must come from the caller's already verified final encoded
file or admitted snapshot: its byte count and SHA are required authorities, not
file-read proof supplied by this module. Copied MCC provenance, final encoded
output and the original Xbox provenance inside a native library are different
identities. No library source hash is compared with an MCC source hash here.

The helper neither opens paths nor follows symlinks. Private-root containment,
symlink and actual destination-case admission remain the caller's responsibility.
It does not wire Cache, repair/publish an index, implement the no-index legacy
consumer, authenticate shader programs, or promote conversion/gameplay readiness.
"""
import copy
import hashlib
import json
from pathlib import PurePosixPath, PureWindowsPath

from audit_rendering import FormatError
from mcc_conversion_coverage import PROFILE, validate as conversion_coverage

SCHEMA = "H2_MCC_NATIVE_MAP_OWNERS_V1"
STOCK = PurePosixPath("halo2/h2_maps_win64_dx11")
LIBRARY_SCHEMA = "H2_PRESERVED_NATIVE_SHADERS_V1"
CATALOG_SCHEMA = "H2_PRESERVED_NATIVE_SHADER_CATALOG_V1"
HUD_SCHEMA = "H2_MCC_NATIVE_HUD_BOOTSTRAP_V1"
BANK_FIELDS = ("bank_directory", "bank_namespace", "bank_library_sha256")


def _require(condition, message):
    if not condition:
        raise FormatError("MCC native map owner: " + message)


def _sha(value):
    return isinstance(value, str) and len(value) == 64 and all(c in "0123456789abcdef" for c in value)


def _relative(value):
    _require(isinstance(value, str) and value and "\\" not in value and "\0" not in value,
             "invalid exact relative path")
    _require(not PureWindowsPath(value).drive and all(p not in ("", ".", "..") for p in value.split("/")),
             "aliased or traversing relative path: " + value)
    path = PurePosixPath(value)
    _require(not path.is_absolute() and path.as_posix() == value, "noncanonical relative path: " + value)
    return path


def _map(value):
    path = _relative(value)
    _require(path.parts[0] == "halo2" and len(path.parts) > 1 and path.suffix.lower() == ".map",
             "map must retain its exact halo2-relative owner: " + value)
    return path


def _namespace(value):
    _require(isinstance(value, str) and value and len(value.encode("utf-8")) <= 255 and
             not any(c in value for c in ("/", "\\", "\0")), "invalid native library namespace")
    return value


def _identity(value, label):
    _require(isinstance(value, dict) and _sha(value.get("sha256")) and
             type(value.get("bytes")) is int and value["bytes"] >= 0, "invalid " + label + " identity")
    return {"bytes": value["bytes"], "sha256": value["sha256"]}


def _pairs(pairs):
    result = {}
    for key, value in pairs:
        _require(key not in result, "duplicate JSON key: " + key)
        result[key] = value
    return result


def _json(data):
    _require(isinstance(data, bytes), "JSON authority must be the exact encoded metadata bytes")
    try:
        return json.loads(data.decode("utf-8"), object_pairs_hook=_pairs,
                          parse_constant=lambda value: (_require(False, "nonfinite JSON value: " + value)))
    except FormatError:
        raise
    except (UnicodeError, ValueError) as error:
        raise FormatError("MCC native map owner: invalid JSON metadata") from error


def _encoded(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False,
                      allow_nan=False).encode("utf-8")


def _digest(value):
    return hashlib.sha256(_encoded(value)).hexdigest()


def _authority(value):
    _require(isinstance(value, dict) and all(isinstance(value.get(k), str) and value[k]
             for k in ("producer", "profile")) and _sha(value.get("receipt_sha256")),
             "missing producer/profile/receipt authority")
    # These fields describe ownership only, even when the producer has other
    # conversion or readiness evidence outside this contract.
    return {key: value[key] for key in ("producer", "profile", "receipt_sha256")}


def bank_identity(directory, library_bytes):
    """Fingerprint exact library JSON bytes without interpreting native programs."""
    path = _relative(directory)
    _require(path.parts[0] == "native-shaders" and len(path.parts) > 1,
             "bank directory must remain under native-shaders")
    library = _json(library_bytes)
    _require(isinstance(library, dict) and library.get("schema") == LIBRARY_SCHEMA,
             "invalid native library metadata schema")
    return dict(bank_directory=directory, bank_namespace=_namespace(library.get("map")),
                bank_library_sha256=hashlib.sha256(library_bytes).hexdigest(),
                library_bytes=len(library_bytes), canonical_sha256=_digest(library))


def _banks(banks):
    _require(isinstance(banks, dict), "missing verified native library identities")
    for directory, bank in banks.items():
        path = _relative(directory)
        _require(path.parts[0] == "native-shaders" and len(path.parts) > 1 and isinstance(bank, dict) and
                 bank.get("bank_directory") == directory and _sha(bank.get("bank_library_sha256")) and
                 _sha(bank.get("canonical_sha256")) and type(bank.get("library_bytes")) is int and
                 bank["library_bytes"] >= 0, "invalid verified bank identity: " + directory)
        _namespace(bank.get("bank_namespace"))
    return banks


def derive_final_inputs(provenance, conversion, verified_inputs):
    """Derive ordered SHA expectations and compare caller-verified final maps.

    Inventory keys retain their existing relative-to-halo2 spelling. Returned
    keys and ``verified_inputs`` keys are relative to the prepared cache, with
    halo2/ included. Final sizes cannot be inferred from source sizes after a
    conversion; the caller must supply every map's final encoded identity.
    """
    _require(isinstance(provenance, dict) and provenance.get("schema") == PROFILE,
             "invalid preparation provenance")
    records = provenance.get("source_files")
    converted, _ = conversion_coverage(records, conversion)
    hashes = {name: record["sha256"] for name, record in records.items()}
    hashes.update(converted)
    roles = {}
    for row in conversion["maps"]:
        _require(row.get("role") in ("resources", "multiplayer", "campaign-or-menu"),
                 "unknown source-bound conversion role")
        roles["halo2/h2_maps_win64_dx11/" + row["map"]] = (
            "resources" if row["role"] == "resources" else "scenario")
    graphics = provenance.get("graphics_corrections", {})
    _require(isinstance(graphics, dict), "invalid graphics correction report")
    phases = (provenance.get("output_changes", []), graphics.get("changes", []))
    for changes in phases:
        _require(isinstance(changes, list), "invalid ordered output chain")
        for change in changes:
            _require(isinstance(change, dict), "invalid output change")
            path = _relative(change.get("path"))
            _require(path.parts[0] == "halo2" and len(path.parts) > 1,
                     "output change lost its copied-source owner")
            name = path.relative_to("halo2").as_posix()
            _require(name in hashes and change.get("donor_sha256") == hashes[name] and
                     _sha(change.get("converted_sha256")), "stale output/graphics donor chain: " + str(path))
            hashes[name] = change["converted_sha256"]
    owners = {"halo2/" + name for name in records if _relative(name).suffix.lower() == ".map"}
    _require(isinstance(verified_inputs, dict) and set(verified_inputs) == owners,
             "final verified map owner set differs from copied inventory")
    result = {}
    for owner in sorted(owners):
        _map(owner)
        name = owner.removeprefix("halo2/")
        actual = _identity(verified_inputs[owner], "final encoded map")
        _require(actual["sha256"] == hashes[name], "wrong final encoded map output: " + owner)
        if actual["sha256"] == records[name]["sha256"]:
            _require(actual["bytes"] == records[name]["bytes"], "unchanged map extent differs: " + owner)
        result[owner] = dict(input_bytes=actual["bytes"], input_sha256=actual["sha256"],
                             source_sha256=records[name]["sha256"],
                             cache_role=roles.get(owner, "scenario"), stock_conversion_owner=owner in roles)
    return result


def _dependency(row, banks):
    _require(isinstance(row, dict), "invalid common dependency")
    if row.get("kind") == "authenticated-common-templates":
        bank = banks.get(row.get("bank_directory"))
        _require(bank is not None and all(row.get(k) == bank[k] for k in BANK_FIELDS) and
                 row.get("canonical_sha256") == bank["canonical_sha256"] and
                 _sha(row.get("authority_sha256")), "common dependency identity differs")
    elif row.get("kind") == "scalar-function-tables":
        _require(row.get("path") == "native-shaders/function-tables.bin", "invalid scalar dependency owner")
        _identity(row, "scalar dependency")
    else:
        _require(False, "unknown dependency kind")
    return copy.deepcopy(row)


def build_index(provenance, conversion, verified_inputs, catalog, banks, *,
                compatibility_authority, hud_bootstrap=None, dependencies=()):
    """Project existing producer metadata; no publication or legacy Cache wiring.

    ``banks`` contains identities from bank_identity() over caller-admitted
    library metadata. Common dependencies remain separate from source-map
    ownership; their full typed/program closure guards remain with the runtime.
    """
    inputs = derive_final_inputs(provenance, conversion, verified_inputs)
    banks, authority = _banks(banks), _authority(compatibility_authority)
    _require(isinstance(catalog, dict) and catalog.get("schema") == CATALOG_SCHEMA and
             isinstance(catalog.get("maps"), list), "invalid retained bank catalog")
    catalog_names = {}
    for row in catalog["maps"]:
        _require(isinstance(row, dict), "invalid retained bank catalog row")
        name = _namespace(row.get("map"))
        _require(name not in catalog_names, "duplicate retained bank namespace: " + name)
        catalog_names[name] = row.get("complete") is True
    native = provenance.get("native_shader_library", {})
    _require(isinstance(native, dict) and isinstance(native.get("maps"), dict), "missing retained bank hashes")
    stock_banks = {}
    for name, sha in native["maps"].items():
        _namespace(name)
        directory = "native-shaders/" + name
        bank = banks.get(directory)
        _require(catalog_names.get(name) and _sha(sha) and bank is not None and
                 bank["bank_namespace"] == name and bank["bank_library_sha256"] == sha,
                 "retained bank catalog/manifest identity differs: " + name)
        stock_banks[name] = bank
    common = [_dependency(row, banks) for row in dependencies]
    hud_maps, hud_policy, hud_digest = {}, None, None
    if hud_bootstrap is not None:
        hud_policy = _json(hud_bootstrap)
        _require(isinstance(hud_policy, dict) and hud_policy.get("schema") == HUD_SCHEMA and
                 isinstance(hud_policy.get("maps"), list) and _sha(hud_policy.get("source_canonical_sha256")),
                 "invalid HUD bootstrap authority")
        _namespace(hud_policy.get("source_map"))
        hud_digest = hashlib.sha256(hud_bootstrap).hexdigest()
        for row in hud_policy["maps"]:
            _require(isinstance(row, dict), "invalid HUD bootstrap input")
            name = _namespace(row.get("map"))
            _identity(row, "HUD bootstrap input")
            _require(name not in hud_maps, "duplicate HUD bootstrap owner")
            hud_maps[name] = row
    rows = []
    for owner, identity in inputs.items():
        stock = identity["stock_conversion_owner"]
        row = {key: identity[key] for key in ("input_bytes", "input_sha256", "source_sha256", "cache_role")}
        row.update(map=owner, shared_directory=STOCK.as_posix(), native_mode="none",
                   **{key: "" for key in (*BANK_FIELDS, "bank_owner_map")},
                   compatibility_authority=authority | {"scope": "stock-conversion-owner" if stock else "copied-input-only"})
        name = _map(owner).stem
        if stock and row["cache_role"] == "scenario" and name in stock_banks:
            row.update({key: stock_banks[name][key] for key in BANK_FIELDS})
            row.update(native_mode="owned-bank", bank_owner_map=owner)
        elif stock and row["cache_role"] == "scenario" and name in hud_maps:
            expected = hud_maps[name]
            _require(row["input_sha256"] == row["source_sha256"] == expected["sha256"] and
                     row["input_bytes"] == expected["bytes"], "HUD bootstrap exact input differs: " + owner)
            source = stock_banks.get(hud_policy["source_map"])
            _require(source is not None and source["canonical_sha256"] == hud_policy["source_canonical_sha256"],
                     "HUD common dependency is missing or customized")
            dependency = dict(kind="authenticated-common-templates", **{key: source[key] for key in BANK_FIELDS},
                              canonical_sha256=source["canonical_sha256"], authority_sha256=hud_digest)
            common.append(dependency)
            row.update(native_mode="authenticated-hud-bootstrap", bootstrap_policy_sha256=hud_digest,
                       bootstrap_dependency=source["bank_directory"],
                       bootstrap_source_canonical_sha256=source["canonical_sha256"])
        rows.append(row)
    unique = {_encoded(row): row for row in common}
    index = dict(schema=SCHEMA, ownership_only=True, maps=rows,
                 dependencies=[unique[key] for key in sorted(unique)],
                 input_authority=dict(copied_source_inventory_sha256=_digest(provenance["source_files"]),
                     conversion_sha256=_digest(conversion),
                     ordered_output_chain_sha256=_digest([provenance.get("output_changes", []),
                         provenance.get("graphics_corrections", {}).get("changes", [])])),
                 stock_bank_directories=sorted(bank["bank_directory"] for bank in stock_banks.values()))
    _structure(index)
    return index


def _structure(index):
    _require(isinstance(index, dict) and index.get("schema") == SCHEMA and index.get("ownership_only") is True and
             isinstance(index.get("maps"), list) and isinstance(index.get("dependencies"), list), "invalid owner index")
    _require(isinstance(index.get("input_authority"), dict) and
             set(index["input_authority"]) == {"copied_source_inventory_sha256", "conversion_sha256", "ordered_output_chain_sha256"} and
             all(_sha(value) for value in index["input_authority"].values()), "invalid input authority subhashes")
    _require(isinstance(index.get("stock_bank_directories"), list), "invalid stock bank owner set")
    for directory in index["stock_bank_directories"]:
        _require(_relative(directory).parts[0] == "native-shaders", "invalid stock bank directory")
    owners, bank_owners = {}, {}
    for row in index["maps"]:
        _require(isinstance(row, dict), "invalid map binding")
        owner = _map(row.get("map")).as_posix()
        _require(owner not in owners, "duplicate exact map owner: " + owner)
        _require(type(row.get("input_bytes")) is int and row["input_bytes"] >= 0 and
                 _sha(row.get("input_sha256")) and _sha(row.get("source_sha256")), "invalid map identity: " + owner)
        _require(row.get("cache_role") in ("scenario", "resources"), "invalid explicit cache role")
        shared = _relative(row.get("shared_directory"))
        _require(shared.parts[0] == "halo2", "shared directory lost its private owner")
        _authority(row.get("compatibility_authority"))
        _require(row["compatibility_authority"].get("scope") in
                 ("stock-conversion-owner", "copied-input-only", "explicit-custom-owner"), "invalid binding authority scope")
        mode = row.get("native_mode")
        if mode == "owned-bank":
            directory = _relative(row.get("bank_directory")).as_posix()
            _require(directory.startswith("native-shaders/") and row.get("bank_owner_map") == owner and
                     _sha(row.get("bank_library_sha256")), "native bank is not owned by the exact map")
            _namespace(row.get("bank_namespace"))
            _require(directory not in bank_owners, "whole native bank aliases distinct map owners")
            bank_owners[directory] = owner
        else:
            _require(mode in ("none", "authenticated-hud-bootstrap") and
                     all(row.get(key) == "" for key in (*BANK_FIELDS, "bank_owner_map")),
                     "unbound/HUD mode must not own a whole native bank")
            if mode == "authenticated-hud-bootstrap":
                _require(_sha(row.get("bootstrap_policy_sha256")) and _sha(row.get("bootstrap_source_canonical_sha256")),
                         "missing exact HUD authority")
                _relative(row.get("bootstrap_dependency"))
        owners[owner] = row
    return owners


def add_owned_map_binding(index, relative_map, verified_input, *, cache_role, shared_directory,
                          owned_bank, producer_receipt):
    """Return a copy with an explicit custom owner bound to its producer receipt.

    The exact receipt JSON bytes must name every input/role/shared/bank field.
    This records the producer's bounded ownership proof, not its semantic or
    TU1.5 readiness. A resource producer may explicitly select no whole bank.
    """
    rows = _structure(index)
    _map(relative_map)
    _require(relative_map in rows and rows[relative_map]["compatibility_authority"]["scope"] != "stock-conversion-owner",
             "custom producer needs its exact copied non-stock map owner")
    prior = rows[relative_map]
    actual = _identity(verified_input, "custom final encoded map")
    _require(actual == dict(bytes=prior["input_bytes"], sha256=prior["input_sha256"]), "custom final output differs")
    _require(cache_role in ("scenario", "resources"), "custom cache role must be explicit")
    _require(_relative(shared_directory).parts[0] == "halo2", "custom shared directory lost its private owner")
    row = copy.deepcopy(prior)
    row.update(cache_role=cache_role, shared_directory=shared_directory, native_mode="none",
               **{key: "" for key in (*BANK_FIELDS, "bank_owner_map")})
    if owned_bank is not None:
        _banks({owned_bank.get("bank_directory"): owned_bank})
        _require(owned_bank["bank_directory"] not in index["stock_bank_directories"],
                 "custom map cannot alias a retained stock whole-bank owner")
        row.update({key: owned_bank[key] for key in BANK_FIELDS})
        row.update(native_mode="owned-bank", bank_owner_map=relative_map)
    receipt = _json(producer_receipt)
    _require(isinstance(receipt, dict), "invalid explicit custom producer receipt")
    keys = ("map", "input_bytes", "input_sha256", "source_sha256", "cache_role", "shared_directory",
            "native_mode", *BANK_FIELDS, "bank_owner_map")
    _require(all(receipt.get(key) == row[key] for key in keys), "custom receipt lost its exact beforeimage/output/bank owner")
    row["compatibility_authority"] = _authority(dict(producer=receipt.get("producer"), profile=receipt.get("profile"),
        receipt_sha256=hashlib.sha256(producer_receipt).hexdigest())) | {"scope": "explicit-custom-owner"}
    result = copy.deepcopy(index)
    result["maps"] = sorted([row if value["map"] == relative_map else value for value in result["maps"]], key=lambda value: value["map"])
    _structure(result)
    return result


def validate_index(index, expected_index):
    """Reject stale/corrupt metadata against a freshly derived producer contract."""
    _structure(index)
    _structure(expected_index)
    _require(index == expected_index, "index differs from current exact source/output/bank authorities")
    return copy.deepcopy(index)


def resolve_binding(index, relative_map, admitted_encoded_source, banks):
    """Resolve exact authenticated-index metadata against the admitted snapshot.

    Missing owner returns None; there is no leaf-name fallback. The caller must
    first authenticate the index/provenance and paths. Reading metadata alone
    never proves filesystem containment or shader-program/semantic readiness.
    """
    rows, banks = _structure(index), _banks(banks)
    _map(relative_map)
    row = rows.get(relative_map)
    if row is None:
        return None
    _require(isinstance(admitted_encoded_source, (bytes, bytearray, memoryview)), "missing admitted encoded snapshot")
    _require(len(admitted_encoded_source) == row["input_bytes"] and
             hashlib.sha256(admitted_encoded_source).hexdigest() == row["input_sha256"], "admitted encoded map differs: " + relative_map)
    for dependency in index["dependencies"]:
        _dependency(dependency, banks)
    if row["native_mode"] == "owned-bank":
        bank = banks.get(row["bank_directory"])
        _require(bank is not None and all(row[key] == bank[key] for key in BANK_FIELDS), "owned native bank identity differs")
    elif row["native_mode"] == "authenticated-hud-bootstrap":
        _require(any(value.get("kind") == "authenticated-common-templates" and
                     value.get("bank_directory") == row["bootstrap_dependency"] and
                     value.get("canonical_sha256") == row["bootstrap_source_canonical_sha256"] and
                     value.get("authority_sha256") == row["bootstrap_policy_sha256"] for value in index["dependencies"]),
                 "HUD bootstrap lacks its authenticated dependency")
    return copy.deepcopy(row)


def encode_index(index):
    """Encode a validated metadata shape deterministically without writing it."""
    _structure(index)
    return _encoded(index) + b"\n"


def decode_index(data):
    """Decode strictly, including duplicate keys; authenticity remains external."""
    index = _json(data)
    _structure(index)
    return index
