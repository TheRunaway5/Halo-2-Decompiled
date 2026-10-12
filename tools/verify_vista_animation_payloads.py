#!/usr/bin/env python3
"""Compare inline Vista clip payloads with original Xbox resource-group bytes."""
import argparse
from contextlib import ExitStack
import hashlib
import json
from pathlib import Path

from audit_rendering import Cache, FormatError, file_sha
from setup_assets import VistaUiCache

GRAPH = "objects\\characters\\masterchief\\fp\\weapons\\rifle\\fp_smg\\fp_smg"


def verify(vista_maps, xbox_maps, graph):
    with ExitStack() as stack:
        memo = {}

        def cache(cls, path):
            key = cls, path
            if key not in memo:
                memo[key] = cls(path)
                stack.callback(memo[key].close)
            return memo[key]

        vista = cache(VistaUiCache, vista_maps / "shared.map")
        xbox = cache(Cache, xbox_maps / "lockout.map")
        sets = []
        for source, is_vista in ((vista, True), (xbox, False)):
            tag = next(t for t in source.tags.values() if t["group"] == "jmad" and t["name"] == graph)
            _, root = source.tag(tag["datum"], "jmad")
            clips = source.block(root+0x2C, 0x60 if is_vista else 0x6C)
            groups = [] if is_vista else source.block(root+0xAC, 20)
            rows = []
            for p in clips:
                if is_vista:
                    length = source.u32(p+0x28)
                    at = source.pointer(source.u32(p+0x2C), source.magic, length)
                    payload = source.data[at:at+length]
                else:
                    index = source.u32(p+0x24)
                    if index >= len(groups):
                        raise FormatError("Xbox clip escapes authored resource groups")
                    group = groups[index]
                    pointer, size, offset = source.u32(group+8), source.u32(group+4), source.u32(p+0x28)
                    length = source.u16(p+0x42)+source.u32(p+0x48)+source.data[p+0x3C]+source.data[p+0x3D]
                    if offset > size or length > size-offset:
                        raise FormatError("Xbox clip escapes its resource group")
                    owner = source if pointer >> 30 == 0 else cache(Cache, xbox_maps / {
                        1: "mainmenu.map", 2: "shared.map", 3: "single_player_shared.map"}[pointer >> 30])
                    at = (pointer & 0x3FFFFFFF)+offset
                    owner.require(at, length)
                    payload = owner.data[at:at+length]
                rows.append(dict(frames=source.u16(p+0x14), bytes=length,
                    codec=payload[0] if payload else None, sha256=hashlib.sha256(payload).hexdigest()))
            sets.append(rows)
        if sets[0] != sets[1]:
            raise FormatError("Vista animation payloads differ from the Xbox graph")
        return dict(schema="H2_VISTA_ANIMATION_PAYLOAD_DIFFERENTIAL_V1", graph=graph,
            sources={str(path): file_sha(path) for (_, path) in memo},
            identical_clips=len(sets[0]), clips=sets[0],
            limits="Byte equality does not implement unsupported codecs or Xbox unit-animation metadata conversion")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("vista_maps", type=Path)
    parser.add_argument("xbox_maps", type=Path)
    parser.add_argument("--graph", default=GRAPH)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    report = verify(args.vista_maps, args.xbox_maps, args.graph)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2)+"\n")
    print(f"{report['identical_clips']} Vista codec payloads are byte-identical to Xbox")
