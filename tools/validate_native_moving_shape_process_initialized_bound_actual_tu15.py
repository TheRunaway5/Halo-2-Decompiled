#!/usr/bin/env python3
"""Exact production helper with every private arena byte initially unavailable."""
import inspect
from pathlib import Path
import validate_native_moving_shape_process_bound_actual_tu15 as parent
ROOT=Path(__file__).resolve().parents[1]
OVERLAY=ROOT/'analysis/physics/native-moving-shape-initialized-arena-stage-20261102/staged/include/halo2/native_tu15_moving_shape_process_binding.hpp'
def main():
 source=inspect.getsource(parent.main)
 old='helper_pin=hashlib.sha256(helper.read_bytes()).hexdigest()';assert source.count(old)==1;source=source.replace(old,'helper_pin=hashlib.sha256(OVERLAY.read_bytes()).hexdigest()')
 old='else original\n  target=project/name';assert source.count(old)==1;source=source.replace(old,"else OVERLAY if name=='include/halo2/native_tu15_moving_shape_process_binding.hpp'else original\n  target=project/name")
 source=source.replace('hashlib.sha256(helper.read_bytes()).hexdigest()==helper_pin','hashlib.sha256(OVERLAY.read_bytes()).hexdigest()==helper_pin')
 # Normalize compiler paths such as src/physics/../core before lookup, so ALL
 # arithmetic sources and their local headers compile from the owned snapshot.
 source=source.replace('str(Path(p).relative_to(ROOT))','str(Path(p).resolve().relative_to(ROOT))')
 env=dict(parent.__dict__,__file__=__file__,OVERLAY=OVERLAY);exec(source,env);return env['main']()
if __name__=='__main__':raise SystemExit(main())
