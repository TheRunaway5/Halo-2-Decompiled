#!/usr/bin/env python3
"""Retain only exact small Host API source inputs from owned1480 recovery."""
from pathlib import Path
import hashlib,json,os,shutil
ROOT=Path(__file__).resolve().parents[1]
def main():
 base=Path(os.environ['HALO2_FROZEN_SOURCE']);out=ROOT/'analysis/physics/native-moving-shape-world-stage-20261102/base';manifest=json.loads((ROOT/'analysis/multiplayer/physics-live-tu15-20261012/mcc-native-broad-phase-edges-original-v1480-20261102/source-manifest.json').read_text())['source_files_sha256']
 for name in ('include/halo2/physics_live_world_tu15.hpp','include/halo2/multiplayer_object_runtime.hpp','src/multiplayer_object_runtime.cpp','include/halo2/native_tu15_world_integration_residuals.hpp'):
  p=base/name;assert hashlib.sha256(p.read_bytes()).hexdigest()==manifest[name];q=out/name;q.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,q)
 print('Four exact1480 Host/World API base files retained')
if __name__=='__main__':main()
