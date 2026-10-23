#!/usr/bin/env python3
"""Qualify live native grenade metadata against complete original TU1.5 MP fixup."""
import argparse
import hashlib
import json
import os
from pathlib import Path
from oracle_mp_combat_postprocess_updates import run_one, IMAGES, Cache, ROOT


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--map',type=Path,default=ROOT/'assets/disc/maps/coagulation.map')
    ap.add_argument('--report',type=Path,required=True)
    args=ap.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):ap.error('Use tools/run_test.py')
    c=Cache(args.map)
    try:
        result=run_one(c,IMAGES[1],Path(os.environ['HALO2_TEST_SCRATCH'])/'postload')
        writes=[r for r in result['ordered_stores'] if r.get('tag',{}).get('group')=='proj'
            and r.get('tag_relative_offset') in ('000000cc','000000d4','000000d8')]
        assert len(writes)==4,writes
        for row in writes:
            expected='3fa66666' if row['tag_relative_offset']=='000000cc' else '3f666666'
            assert row['after_bits']==expected,row
        report={'passed':True,'scope':'Complete unchanged original multiplayer fixup over actual Coagulation cache/tag table. Native gameplay throw and detonation acceptance remain separate.',
            'original_xbe_sha256':result['xbe_sha256'],'original_cache_sha256':hashlib.sha256(args.map.read_bytes()).hexdigest(),
            'original_entry':result['entry'],'original_caller':result['owning_caller'],
            'whole_original_instruction_count':result['whole_original_instruction_count'],
            'executed_instruction_addresses':result['executed_instruction_addresses'],
            'declared_cache_global_bindings':result['declared_cache_global_bindings'],
            'grenade_metadata_stores':writes,'supplied_results':[],
            'tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}
        args.report.parent.mkdir(parents=True,exist_ok=True)
        args.report.write_text(json.dumps(report,indent=2)+'\n')
        print(json.dumps({'passed':True,'original_arming':1.2999999523162842,
            'original_plasma_timer':.8999999761581421,'report':str(args.report)}))
    finally:c.data.close();c.stream.close()


if __name__=='__main__':main()
