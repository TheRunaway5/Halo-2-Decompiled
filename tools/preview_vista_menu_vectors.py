#!/usr/bin/env python3
"""Build a disposable native preview of the corrected Vista keyboard keys.

All other artwork/fonts come from the selected MCC base. This explicitly
labelled composite is not a new MCC import or the completed menu redraw.
"""
import argparse
import json
from pathlib import Path
import shutil

from audit_rendering import file_sha

ROOT=Path(__file__).resolve().parents[1]


def preview(base,vectors,output):
    if output.exists():raise ValueError('Choose a fresh preview destination')
    imported=json.loads((base/'manifest.json').read_text())
    report=json.loads((vectors/'reconstruction.json').read_text())
    pages={}
    for row in report['records']:
        if row['cache']=='mainmenu.map' and row['reconstruction'] and \
           row['reconstruction']['family']=='virtual-keyboard-key' and 'virtual_keyboard\\button_' in row['name']:
            assert imported['focus_page_proofs'][row['name']]['vista_dds_identity']
            pages[(row['name'],row['frame'])]=row['reconstruction']
    assert len(pages)==15,'Expected all five keys and all three Vista states'
    shutil.copytree(base,output)
    (output/'manifest.json').rename(output/'base-mcc-manifest.json')
    changes={}
    for manifest in ('pack.tsv','vista.tsv','pause.tsv'):
        rows=(output/manifest).read_text().splitlines();new=[rows[0]];updated=[]
        for line in rows[1:]:
            columns=line.split('\t');record=pages.get((columns[0],int(columns[1])))
            if record:
                relative='vista-vectors/'+Path(record['dds']).name
                destination=output/relative;destination.parent.mkdir(exist_ok=True)
                shutil.copy2(vectors/record['dds'],destination)
                assert file_sha(destination)==record['dds_sha256']
                columns[5]=relative
                updated.append(dict(name=columns[0],frame=int(columns[1]),dds=relative,
                                    dds_sha256=record['dds_sha256'],vector_sha256=record['vector_sha256']))
            new.append('\t'.join(columns))
        (output/manifest).write_text('\n'.join(new)+'\n');changes[manifest]=updated
    wrapper=dict(schema='halo2-vista-menu-vector-preview-v1',complete=False,
        base_manifest='base-mcc-manifest.json',base_manifest_sha256=file_sha(output/'base-mcc-manifest.json'),
        redraw_reference_sha256=report['reference_sha256'],changes=changes,
        claim='Native preview of fifteen corrected keyboard pages; all other samples remain the selected base artwork.',
        fonts='MCC font selection and existing Vista prompt bindings preserved')
    (output/'manifest.json').write_text(json.dumps(wrapper,indent=2)+'\n')
    return wrapper


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--base',type=Path,required=True)
    p.add_argument('--vectors',type=Path,default=ROOT/'assets/vista-menu-svg-4k')
    p.add_argument('--output',type=Path,required=True)
    args=p.parse_args();r=preview(args.base,args.vectors,args.output)
    print(json.dumps({n:len(v) for n,v in r['changes'].items()},indent=2))


if __name__=='__main__':main()
