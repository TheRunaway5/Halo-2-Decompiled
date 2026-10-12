#!/usr/bin/env python3
"""Execute original2356D8 against the captured original keyboard widgets.

This freezes ordering between distinct type/layer/depth groups. Equal-group
qsort tie order is deliberately not asserted.
"""
import functools
import hashlib
import json
import struct
from oracle_multiplayer_hud_positions import Retail, ROOT, PIN

source = ROOT / "analysis/frontend/profile-regression/original-keyboard-runtime-assets.json"
snapshot = json.loads(source.read_text())
original = Retail()
entries = {}
for index, widget in enumerate(snapshot["widgets"]):
    node = original.alloc(bytes.fromhex(widget["raw_hex"]))
    entries[index] = original.alloc(struct.pack("<Ifh2x", node,
                                                widget["animation_xyz"][2],
                                                widget["layer"]))
comparisons = []


def compare(left, right):
    result = original.run(0x2356D8, argv=[entries[left], entries[right]])
    result = struct.unpack("<i", struct.pack("<I", result))[0]
    comparisons.append(dict(left=left, right=right, result=result))
    return result


order = sorted(entries, key=functools.cmp_to_key(compare))
groups = []
for index in order:
    widget = snapshot["widgets"][index]
    key = (widget["layer"], widget["kind"], widget["animation_xyz"][2])
    if not groups or tuple(groups[-1]["key"]) != key:
        groups.append(dict(key=key, widgets=[]))
    groups[-1]["widgets"].append(index)
assert [group["key"] for group in groups] == [
    (0, 8, 0.), (0, 0, 0.), (1, 8, 0.), (2, 3, 0.),
    (5, 6, 0.), (32767, 6, 0.)]
report = dict(xbe_sha256=PIN, runtime_snapshot_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
              groups=groups, comparisons=comparisons,
              scope="Unchanged2356D8 compares actual captured original keyboard widget type/layer/depth. Python sorting chooses an order satisfying those comparisons; original qsort equal-group tie order is excluded. The runtime snapshot independently confirms all54widget depths0 and alpha1.")
path = ROOT / "analysis/frontend/profile-regression/keyboard-widget-order-original.json"
path.write_text(json.dumps(report, indent=2) + "\n")
print(json.dumps(report, indent=2))
