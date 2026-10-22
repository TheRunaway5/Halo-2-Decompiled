#!/usr/bin/env python3
"""Portable provenance and finite policy generation controls."""
from copy import deepcopy
from pathlib import Path
import hashlib
import importlib.util
import json
import struct

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location(
    "permutation_generator", ROOT / "tools/generate_mcc_sound_permutation_time_reference.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
policy = json.loads((ROOT / "tools/mcc-sound-permutation-time-reference-profiles.json").read_text())
assert len(policy["keys"]) == 24624 and len(policy["owners"]) == 41
expected = (ROOT / "src/audio/mcc_sound_permutation_time_reference_profiles.inc").read_text()
assert module.generate(policy) == expected
small = deepcopy(policy)
small["keys"] = small["keys"][:2]
small["owners"] = small["owners"][:1]
good = module.generate(small)
controls = 0


def rejected(change):
    global controls
    value = deepcopy(small)
    change(value)
    try:
        module.generate(value)
    except (ValueError, KeyError, TypeError, OverflowError):
        controls += 1
        return
    raise AssertionError("Malformed finite timing provenance was accepted")


rejected(lambda p: p.update(schema="unowned"))
for field in ("path", "range", "permutation"):
    rejected(lambda p, f=field: p["keys"][0].update({f: ""}))
rejected(lambda p: p["keys"].append(deepcopy(p["keys"][0])))
rejected(lambda p: p["keys"][0].update(key_sha256="00" * 32))
rejected(lambda p: p["keys"][0].update(payload_sha256="00"))
for field in ("source_ms", "original_ms"):
    rejected(lambda p, f=field: p["keys"][0].update({f: -1}))
    rejected(lambda p, f=field: p["keys"][0].update({f: 65536}))
    rejected(lambda p, f=field: p["keys"][0].update({f: True}))
    rejected(lambda p, f=field: p["keys"][0].update({f: p["keys"][0][f] ^ 1}))
for field in ("mcc_reference", "original_reference"):
    rejected(lambda p, f=field: p["keys"][0][f].update(row_hex="00" * 15))
    rejected(lambda p, f=field: p["keys"][0][f].update(row_sha256="00" * 32))
    for name in ("sound", "gestalt", "sound_root", "source_range", "source_permutation"):
        rejected(lambda p, f=field, n=name: p["keys"][0][f].update({n: -1}))
rejected(lambda p: p["owners"].append(deepcopy(p["owners"][0])))
rejected(lambda p: p["owners"][0].update(metadata_sha256="00"))
rejected(lambda p: p.update(owners=[]))
rejected(lambda p: p.update(keys=[]))

# Canonical byte encoding must preserve both unsigned bounds and keep source
# and Original values separate, including authentic zero-duration rows.
edge = deepcopy(small)
for i, row in enumerate(edge["keys"]):
    for value, field, ref in ((0 if i else 65535, "source_ms", "mcc_reference"),
                              (65535 if i else 0, "original_ms", "original_reference")):
        raw = bytearray.fromhex(row[ref]["row_hex"])
        struct.pack_into("<H", raw, 6, value)
        row[field] = value
        row[ref].update(row_hex=raw.hex(), row_sha256=hashlib.sha256(raw).hexdigest())
assert module.generate(edge) != good
print(f"Permutation-time generator: 24624 exact named keys, 41 complete owners; {controls} provenance guards PASS")
