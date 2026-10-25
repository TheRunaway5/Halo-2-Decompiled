#!/usr/bin/env python3
"""Extract waypoint bitmap dimensions/registration points from original tags.

Only four signed-short asset columns are published. No executable instructions,
GPU headers, cache payloads or pixel resources are copied into native code.
"""
import argparse,hashlib,json,os
from pathlib import Path
from inspect_multiplayer import Cache

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--maps',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    assert Path(os.environ['HALO2_TEST_SCRATCH']).is_dir()
    selected={};pins={}
    for path in sorted(a.maps.glob('*.map')):
        if path.stem in ('mainmenu','shared','single_player_shared'):continue
        c=Cache(path);banks={}
        for tag in c.tags.values():
            if tag['group']!='hudg':continue
            root=tag['offset']
            for offset,stride in ((0x408,36),(0x410,24)):
                for row in c.block(root+offset,stride):
                    for field in ((4,32) if stride==36 else (4,)):
                        datum=c.u(row+field)
                        if datum==0xffffffff:continue
                        bitmap=c.tags[datum];q=bitmap['offset'];frames=c.block(q+0x44,116)
                        banks[bitmap['name']]=[[c.h(r+x) for x in (4,6,16,18)] for r in frames]
        if not banks:continue
        if selected:assert banks==selected,path
        selected=banks;pins[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
        c.data.close();c.stream.close()
    assert len(pins)>=12 and len(selected)==9
    lines=['// Original Xbox bitmap asset columns: width, height, registration X/Y.']
    for name,frames in sorted(selected.items()):
        rows=','.join('{'+','.join(map(str,r))+'}' for r in frames)
        lines.append('{'+json.dumps(name)+', {'+rows+'}},')
    a.output.write_text('\n'.join(lines)+'\n')
    a.report.write_text(json.dumps(dict(source_maps=pins,bitmaps=selected,columns=['width','height','registration_x','registration_y'],executable_bytes=0),indent=2)+'\n')
if __name__=='__main__':main()
