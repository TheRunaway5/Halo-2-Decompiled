#!/usr/bin/env python3
"""Exact typed storage identity for original stem/spas/vrtx metadata.

This does not decode or evaluate shader instructions. Program and declaration
arrays are opaque exact bytes. Physical allocation order, pointer addresses,
and bytes outside every typed allocation are reported separately.
"""
import hashlib
import json
import struct


def require(ok, message):
    if not ok:
        raise ValueError("Native program storage: " + message)


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode()


def storage_graph(record, payload, *, diagnose_unclassified=False):
    require(record["group"] in ("stem", "spas", "vrtx"), "non-program owner")
    require(len(payload) == record["bytes"] and
            hashlib.sha256(payload).hexdigest() + ".bin" == record["payload"],
            "complete payload owner/hash differs")
    fixups = {}
    for label in ("pointers", "references", "strings"):
        for at, target in record[label]:
            require(type(at) is int and 0 <= at <= len(payload) - 4 and at not in fixups,
                    "invalid/overlapping relocation ownership")
            fixups[at] = label, target
    consumed = set()
    covered = bytearray(len(payload))
    allocations = []

    def u32(at):
        require(0 <= at <= len(payload) - 4, "word outside complete payload")
        return struct.unpack_from("<I", payload, at)[0]

    def node(at, count, stride, children=None):
        require(type(at) is int and type(count) is int and count >= 0 and stride > 0
                and count <= 1_000_000 and 0 <= at <= len(payload)
                and count * stride <= len(payload) - at, "typed allocation extent")
        allocations.append((at, count * stride))
        covered[at:at + count * stride] = b"\1" * (count * stride)
        rows = []
        for index in range(count):
            base = at + index * stride
            data = bytearray(payload[base:base + stride])
            links = {}
            for offset, (label, target) in fixups.items():
                if base <= offset < base + stride:
                    require(offset + 4 <= base + stride, "relocation crosses typed element")
                    require(offset not in consumed, "overlapping typed relocation ownership")
                    if label == "pointers":
                        continue
                    consumed.add(offset)
                    links[str(offset - base)] = {"kind": label, "target": target}
                    data[offset - base:offset - base + 4] = bytes(4)
            nested = {}
            for field, child_stride, decoder in children or ():
                length = u32(base + field)
                pointer_at = base + field + 4
                address = u32(pointer_at)
                if length:
                    require(pointer_at in fixups and fixups[pointer_at] == ("pointers", address),
                            "nonempty block lacks exact declared pointer owner")
                    require(pointer_at not in consumed, "pointer consumed twice")
                    consumed.add(pointer_at)
                    nested[str(field)] = node(address, length, child_stride, decoder)
                else:
                    require(pointer_at not in fixups and address == 0,
                            "empty block carries unexplained source allocation")
                    nested[str(field)] = []
                data[field + 4:field + 8] = bytes(4)
            rows.append({"bytes": data.hex(), "references": links, "blocks": nested})
        return rows

    # Counts and every non-relocation word remain literal. These are the Xbox
    # layouts consumed by Cache::load_material and the source vertex bank.
    if record["group"] == "vrtx":
        require(len(payload) >= 12 and struct.unpack_from("<H", payload)[0] == 1,
                "not an original Xbox vertex container")
        graph = node(0, 1, 12, [(4, 28, [(4, 2, None), (12, 1, None), (20, 1, None)])])
    elif record["group"] == "stem":
        require(len(payload) >= 96, "truncated original template")
        # The finite compiled program banks have no authoring/document data.
        for field in (0, 16, 24, 40, 48, 56):
            require(u32(field) == 0 and u32(field + 4) == 0,
                    "unreviewed original template authoring allocation")
        graph = node(0, 1, 96, [(88, 40, [(0, 10, None), (8, 2, None),
            (16, 10, None), (24, 6, None), (32, 4, None)])])
    else:
        require(len(payload) >= 36, "truncated original pass")
        for field in (0, 8, 20):
            require(u32(field) == 0 and u32(field + 4) == 0,
                    "unreviewed original pass authoring allocation")
        graph = node(0, 1, 36, [(28, 88, [(0, 306, None), (8, 4, None),
            (16, 5, None), (24, 24, None), (32, 4, None), (40, 6, None),
            (48, 32, None), (56, 4, None), (64, 7, None), (72, 7, None)])])
    require(consumed == set(fixups), "unexplained relocation outside complete typed graph")
    gaps = []
    at = 0
    while at < len(payload):
        if covered[at]:
            at += 1
            continue
        end = at + 1
        while end < len(payload) and not covered[end]:
            end += 1
        data = payload[at:end]
        gaps.append({"offset": at, "bytes": end - at,
                     "sha256": hashlib.sha256(data).hexdigest(), "all_zero": not any(data),
                     "hex": data.hex()})
        at = end
    # Original resident banks align interior arrays to four bytes; copied
    # out-of-owner arrays are aligned to sixteen by preserve_native_shaders.
    # Retain every padding byte/hash in the storage proof. Only the typed
    # graph comparison excludes these bounded, unreferenced alignment gaps.
    for gap in gaps:
        gap_end = gap["offset"] + gap["bytes"]
        original_alignment = min(gap["bytes"], (-gap["offset"]) % 4)
        gap["alignment_padding"] = (gap["bytes"] < 4 and gap_end % 4 == 0) or (
            gap["bytes"] < 16 and gap_end % 16 == 0 and
            not any(bytes.fromhex(gap["hex"])[original_alignment:]))
    require(diagnose_unclassified or all(gap["alignment_padding"] for gap in gaps),
            "unreferenced bytes lack a bounded source alignment classification")
    value = {"group": record["group"], "name": record["name"], "root": graph}
    return {"graph": value, "graph_sha256": hashlib.sha256(canonical(value)).hexdigest(),
            "storage": {"payload_sha256": hashlib.sha256(payload).hexdigest(),
                        "bytes": len(payload), "allocations": allocations, "alignment_padding": gaps,
                        "relocations": len(consumed)}}
