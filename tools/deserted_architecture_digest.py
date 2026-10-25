"""Hash Deserted architecture independently of GLB storage and backdrop edits.

The canonical source digest includes the object's world matrix and each
triangle's palette key, position, normal, and UV corner attributes. Cyclic
corner rotations, triangle/primitive order, accessor layout, and vertex reuse
do not affect the digest. Winding and authored attribute changes do affect it.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct

import numpy as np

from build_dust2_mombasa import Glb


def architecture_digest(path: Path | str) -> dict:
    """Return the V1 architecture-only source provenance digest and counts."""
    source = Glb(path)
    try:
        matches = [row for row in source.nodes()
                   if row[1].get("name") == "architecture_polished"]
        if len(matches) != 1:
            raise ValueError("Expected one architecture_polished object")
        _, _, mesh, world = matches[0]
        if not np.isfinite(world).all():
            raise ValueError("Nonfinite architecture transform")
        digest = hashlib.sha256(b"H2_ARCHITECTURE_CORNER_PROVENANCE_V1\0")
        digest.update(np.asarray(world, dtype="<f8").tobytes())
        groups = {}
        triangles = 0
        for primitive in mesh["primitives"]:
            if primitive.get("mode", 4) != 4:
                raise ValueError("Architecture requires triangle lists")
            attributes = primitive["attributes"]
            indices = source.array(primitive["indices"])[:, 0].astype(int)
            if len(indices) % 3:
                raise ValueError("Incomplete architecture triangle")
            indices = indices.reshape(-1, 3)
            vertices = np.column_stack([
                source.array(attributes["POSITION"]),
                source.array(attributes["NORMAL"]),
                source.array(attributes["TEXCOORD_0"]),
            ]).astype("<f4")
            if not np.isfinite(vertices).all():
                raise ValueError("Nonfinite architecture attributes")
            values = vertices[indices]
            material = source.doc["materials"][primitive["material"]]
            key = material["extras"]["halo_palette"]
            records = groups.setdefault(key, [])
            for face in values:
                rotation = min(range(3), key=lambda corner: tuple(
                    np.roll(face, -corner, axis=0)[:, :3].reshape(-1)))
                records.append(np.roll(face, -rotation, axis=0)
                               .astype("<f4").tobytes())
            triangles += len(values)
        for key, records in sorted(groups.items()):
            digest.update(key.encode() + b"\0")
            digest.update(struct.pack("<Q", len(records)))
            for record in sorted(records):
                digest.update(record)
        return {
            "schema": "H2_ARCHITECTURE_CORNER_PROVENANCE_V1",
            "sha256": digest.hexdigest(),
            "triangles": triangles,
            "material_triangles": {
                key: len(value) for key, value in sorted(groups.items())
            },
        }
    finally:
        source.raw.close()
        source.file.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("glb", type=Path)
    args = parser.parse_args()
    print(json.dumps(architecture_digest(args.glb), indent=2))
