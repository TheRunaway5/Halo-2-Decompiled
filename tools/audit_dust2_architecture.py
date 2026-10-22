#!/usr/bin/env python3
"""Reject loose architectural faces and open building seams in an authored GLB.

Run through tools/run_test.py. Retail props, intentional glass/fence/light
cards and the nonplayable desert are separate from the closed architecture.
This topology gate complements player-height visual review; it cannot certify
the visual quality or route fidelity of a map.
"""
import argparse
import json
import os
from pathlib import Path

import numpy as np
from scipy.sparse import coo_matrix
from scipy.sparse.csgraph import connected_components
from build_dust2_mombasa import Glb, sha


def audit(model, report):
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    scene = Glb(model)
    positions, faces, names = [], [], []
    offset = 0
    degenerate=0
    invalid_attributes=0
    collapsed_floor_uv=0
    for _, node, mesh, world in scene.nodes():
        name = node.get('name', '')
        if not name.startswith(('district_', 'architecture_')):
            continue
        if any(word in name for word in ('desert_', 'fence', 'glass', 'light', 'terrain')):
            continue
        names.append(name)
        for primitive in mesh['primitives']:
            p = scene.array(primitive['attributes']['POSITION']).astype(float)
            p = p @ world[:3, :3].T + world[:3, 3]
            t = scene.array(primitive['indices'])[:, 0].reshape(-1, 3)
            uv=scene.array(primitive['attributes']['TEXCOORD_0']).astype(float)
            normal=scene.array(primitive['attributes']['NORMAL']).astype(float)
            invalid_attributes+=int(not (np.isfinite(p).all() and np.isfinite(uv).all() and np.isfinite(normal).all()))
            cross=np.cross(p[t[:,1]]-p[t[:,0]],p[t[:,2]]-p[t[:,0]])
            area=np.linalg.norm(cross,axis=1)
            degenerate+=int((area<=1e-12).sum())
            if name in ('architecture_floor','architecture_road','architecture_walkway'):
                # glTF Y is Halo Z. Upward floor faces must have real UV area;
                # a top-down projection of a vertical wall is not a floor.
                top=cross[:,1]>.5*area
                du=uv[t[:,1]]-uv[t[:,0]];dv=uv[t[:,2]]-uv[t[:,0]]
                uv_area=abs(du[:,0]*dv[:,1]-du[:,1]*dv[:,0])
                collapsed_floor_uv+=int((top & (uv_area<=1e-12)).sum())
            positions.append(p)
            faces.append(t + offset)
            offset += len(p)
    assert positions, 'No architecture selected'
    p, remap = np.unique(np.round(np.concatenate(positions), 5), axis=0, return_inverse=True)
    t = remap[np.concatenate(faces)]
    edges = np.concatenate((t[:, [0, 1]], t[:, [1, 2]], t[:, [2, 0]]))
    edges.sort(axis=1)
    unique, counts = np.unique(edges, axis=0, return_counts=True)
    graph = coo_matrix((np.ones(len(unique)), (unique[:, 0], unique[:, 1])), shape=(len(p), len(p)))
    _, labels = connected_components(graph, directed=False)
    sizes = np.bincount(labels[t[:, 0]])
    loose = int(((sizes > 0) & (sizes <= 2)).sum())
    boundary = int((counts == 1).sum())
    nonmanifold = int((counts > 2).sum())
    result = dict(schema='H2_DUST2_ARCHITECTURE_TOPOLOGY_V1', model_sha256=sha(model),
        selected_meshes=names, triangles=len(t), boundary_edges=boundary,
        nonmanifold_edges=nonmanifold, loose_face_components=loose,
        degenerate_triangles=degenerate,invalid_attribute_groups=invalid_attributes,
        upward_floor_triangles_with_collapsed_uv=collapsed_floor_uv,
        passed=not (boundary or nonmanifold or loose or degenerate or invalid_attributes or collapsed_floor_uv),
        scope='closed architecture only; visual/UV/route review remains separate')
    report.parent.mkdir(parents=True, exist_ok=True)
    report.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({k: v for k, v in result.items() if k != 'selected_meshes'}, indent=2))
    return 0 if result['passed'] else 1


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--model', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    args = parser.parse_args()
    raise SystemExit(audit(**vars(args)))
