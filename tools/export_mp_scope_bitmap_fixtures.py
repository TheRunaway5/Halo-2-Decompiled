#!/usr/bin/env python3
"""Export only captured original bitmap inputs/draw request outputs."""
import argparse,json,struct
from pathlib import Path
from oracle_mp_scope_bitmap_geometry import sha

def words(raw):
    return [f'{value:08x}' for value in struct.unpack('<'+'I'*(len(raw)//8),bytes.fromhex(raw))]

def main():
    p=argparse.ArgumentParser();p.add_argument('--report',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    r=json.loads(a.report.read_text());lines=[]
    constants=[words(v['raw4_hex'])[0] for v in r['source_constants']]
    for case in r['cases']:
        x=case['kernel_input'];w=case['authored_widget'];slot=case['original_layout_slot']
        registration=words(struct.pack('<2f',*w['registration'][slot]).hex())
        values=[case['name'],case.get('explicitly_modified_widget_flag_word',w['flags']),*constants,*words(x['anchor8_hex']),*words(x['x_axis8_hex']),*words(x['y_axis8_hex']),
                *words(x['texture_width4_hex']),*words(x['texture_height4_hex']),*words(x['sprite_bounds10_hex']),*registration,len(case['original_draw_requests'])]
        for request in case['original_draw_requests']:values.extend([*words(request['positions20_hex']),*words(request['sprite_bounds10_hex'])])
        lines.append(' '.join(map(str,values)))
    assert not a.output.exists();a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text('\n'.join(lines)+'\n')
    print(json.dumps(dict(cases=len(lines),fixtures_sha256=sha(a.output))))

if __name__=='__main__':main()
