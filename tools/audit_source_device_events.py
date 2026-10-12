#!/usr/bin/env python3
"""Inventory literal device event references on all21 supplied Xbox maps."""
import argparse
import json
from pathlib import Path
from audit_rendering import Cache,file_sha
from audit_map_world_features import DISC,PACK
ROOT=Path(__file__).resolve().parents[1]

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();maps=[];counts={}
    for directory,names in (('disc',DISC),('map-pack',PACK)):
        for name in names:
            path=ROOT/'assets'/directory/'maps'/(name+'.map');cache=Cache(path)
            try:
                _,scenario=cache.tag(cache.scenario,'scnr');devices=[]
                for offset,stride,group in ((0xa8,0x48,'mach'),(0xb8,0x44,'ctrl')):
                    palette=cache.block(scenario+offset+8,40)
                    for ordinal,placement in enumerate(cache.block(scenario+offset,stride)):
                        datum=cache.u32(palette[cache.u16(placement)]+4);tag,definition=cache.tag(datum,group)
                        slots=[('opening',0xe0),('closing',0xe8),('opened',0xf0),('closed',0xf8),('delay',0x114)]
                        if group=='ctrl':slots += [('on',0x12c),('off',0x134),('deny',0x13c)]
                        events=[]
                        for slot,at in slots:
                            event=cache.u32(definition+at)
                            if event==0xffffffff:continue
                            target=cache.tags[event];counts[target['group']]=counts.get(target['group'],0)+1
                            events.append({'slot':slot,'datum':f'{event:08x}','group':target['group'],'name':target['name']})
                        devices.append({'placement':ordinal,'datum':f'{datum:08x}','group':group,'name':tag['name'],'events':events})
                maps.append({'map':name,'path':str(path.relative_to(ROOT)),'sha256':file_sha(path),'devices':devices})
            finally:cache.close()
    report={'maps':maps,'event_reference_counts':counts,'map_count':len(maps),
        'boundary':'Literal Xbox mach/ctrl placements and common/control event references. No live producer, original sound admission, attached effect engine or Vista-exclusive behavior is inferred.'}
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'map_count':len(maps),'event_reference_counts':counts}))
if __name__=='__main__':main()
