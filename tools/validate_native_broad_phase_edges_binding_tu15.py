#!/usr/bin/env python3
"""Run the frozen independent original matrix through the sole bound sweep."""
from pathlib import Path
import hashlib,json,sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 parent=ROOT/'analysis/physics/native-broad-phase-edges-qualified-20261102';pins=json.loads((parent/'source-pins.json').read_text());template=parent/'source/tools/validate_native_broad_phase_edges_tu15.py';assert sha(template)==pins['source_sha256']['tools/validate_native_broad_phase_edges_tu15.py'];stage=ROOT/'analysis/physics/native-broad-phase-edges-world-stage-20261102/staged'
 text=template.read_text();text=text.replace('native_tu15_update_broad_phase_edges(memory,','native_tu15_update_bound_broad_phase_edges(memory,')
 needle="        test=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text()";assert text.count(needle)==1
 text=text.replace(needle,"        target=project/'include/halo2/native_tu15_broad_phase_edges_binding.hpp';shutil.copyfile(STAGE/'include/halo2/native_tu15_broad_phase_edges_binding.hpp',target)\n"+needle)
 text=text.replace('#include "halo2/native_tu15_broad_phase_edges.hpp"','#include "halo2/native_tu15_broad_phase_edges_binding.hpp"')
 header=stage/'include/halo2/native_tu15_broad_phase_edges_binding.hpp';header_pin=sha(header)
 namespace=dict(__name__='bound_sweep_proof',__file__=str(template));exec(compile(text,str(template),'exec'),namespace);namespace['STAGE']=stage
 result=namespace['main']();assert sha(header)==header_pin
 output=Path(sys.argv[sys.argv.index('--output')+1]);report=json.loads((output/'report.json').read_text());report.update(bound_header_sha256=header_pin,binding_validator_sha256=sha(Path(__file__)),frozen_parent_validator_sha256=sha(template));(output/'report.json').write_text(json.dumps(report,indent=2)+'\n');return result
if __name__=='__main__':raise SystemExit(main())
