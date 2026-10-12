#!/usr/bin/env python3
"""Export whole original model-query inputs, outputs and ordered stores."""
import argparse,json
from pathlib import Path
from freeze_mp_native_sound_storage import sha

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
    original=json.loads(a.report.read_text());assert not original['instruction_patches']and not original['supplied_callee_results']
    rows=[];count=stores=full=0
    for count,c in enumerate(original['cases'],1):
        rows.append('K '+' '.join([str(count),str(c['image']),c['pool'],c['tag_table'],c['object'],*c['outputs'],str(c['return_al'])]))
        for r in c['regions']:
            rows.append(f'M {r["address"]} {r["before"]} {r["after"]}');full+=len(r['after'])//2
        for s in c['stores']:
            assert s['bytes']==4
            before=int.from_bytes(bytes.fromhex(s['before']),'little')
            rows.append(f'W {s["instruction"]} {s["address"]} {before:08x} {s["value"]}');stores+=1
        rows.append('E')
    target=out/'fixtures.tsv';target.write_text('\n'.join(rows)+'\n')
    report=dict(schema='object-model-resource-fixture-v1',exporter_sha256=sha(Path(__file__)),
        original_report_sha256=sha(a.report),fixture_sha256=sha(target),cases=count,stores=stores,
        full_output_bytes=full,expected_from_native=False,private_x86_abi_stack_reproduced=False)
    (out/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(report_sha256=sha(out/'report.json'),fixture_sha256=sha(target))))

if __name__=='__main__':main()
