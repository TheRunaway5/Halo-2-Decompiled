#!/usr/bin/env python3
"""Collect original Vista menu pages for reviewed, semantic SVG reconstruction.

This is a reference exporter, not a vectorizer. It neither draws replacement
art nor treats an unimplemented page as a completed redraw.
"""
import argparse
import hashlib
import json
from pathlib import Path

from PIL import Image
from audit_rendering import file_sha
from export_vista_menu_ui import VistaCache, dds_image, expand_resource, mip_shapes, preview, sequences
from setup_assets import banks, resource

ROOT = Path(__file__).resolve().parents[1]


def digest(data):
    return hashlib.sha256(data).hexdigest()


def collect(vista, output):
    output.mkdir(parents=True, exist_ok=True)
    pages, owners, sources, unavailable = {}, {}, {}, {}
    # Shell, shared caches, and every level own separate native banks.
    paths=sorted((vista/'maps').glob('*.map'),key=lambda p:(p.name!='mainmenu.map',p.name))
    for path in paths:
        filename=path.name
        sources[filename] = dict(bytes=path.stat().st_size, sha256=file_sha(path))
        try:cache = VistaCache(path)
        except Exception as e:
            unavailable[filename]=str(e)
            continue
        local = []
        try:
            for name, (frames, effective) in sorted(banks(cache).items()):
                if name.startswith('ui\\hud\\'):
                    continue
                tag = next(t for t in cache.tags.values() if t['name']==name and t['group']=='bitm')
                seq = sequences(cache, cache.tag(tag['datum'], 'bitm')[1], len(frames))
                entries = []
                for ordinal, p in enumerate(frames):
                    w,h,fmt,mips = [cache.u16(p+i) for i in (4,6,12,20)]
                    levels = mip_shapes(w,h,mips,fmt)
                    raw = expand_resource(resource(cache,p),sum(l['bytes'] for l in levels))
                    dds,_ = dds_image(w,h,mips,fmt,raw)
                    rgba = preview(w,h,fmt,raw,dds)
                    key = digest(w.to_bytes(4,'little')+h.to_bytes(4,'little')+rgba.tobytes())
                    if key not in pages:
                        relative = 'reference/'+key+'.png'
                        destination = output/relative
                        destination.parent.mkdir(parents=True,exist_ok=True)
                        rgba.save(destination)
                        pages[key] = dict(width=w,height=h,png=relative,png_sha256=file_sha(destination),uses=[])
                    pages[key]['uses'].append(dict(cache=filename,name=name,frame=ordinal))
                    entries.append(dict(frame=ordinal,page=key,dds_sha256=digest(dds),format=fmt,
                                        registration=[cache.u16(p+i) for i in (16,18)]))
                local.append(dict(name=name,frames=entries,sequences=seq))
        finally:
            cache.close()
        owners[filename] = local
    # Loading-screen textures are executable-owned, outside mainmenu.map.
    loading=ROOT/'assets/menu-ui/vista/loading'
    if (loading/'loading-textures.tsv').is_file():
        sources['loading-source.json']=dict(bytes=(loading/'source.json').stat().st_size,sha256=file_sha(loading/'source.json'))
        entries=[]
        for line in (loading/'loading-textures.tsv').read_text().splitlines()[1:]:
            index,sw,sh,w,h,fmt,relative=line.split('\t')
            rgba=Image.open(loading/relative).convert('RGBA')
            key=digest(int(w).to_bytes(4,'little')+int(h).to_bytes(4,'little')+rgba.tobytes())
            if key not in pages:
                png='reference/'+key+'.png';rgba.save(output/png)
                pages[key]=dict(width=int(w),height=int(h),png=png,png_sha256=file_sha(output/png),uses=[])
            name='executable\\loading\\'+index
            pages[key]['uses'].append(dict(cache='loading.bin',name=name,frame=0))
            entries.append(dict(name=name,frames=[dict(frame=0,page=key,dds_sha256=file_sha(loading/relative),
                format=int(fmt),source_extent=[int(sw),int(sh)])],sequences=[]))
        owners['loading.bin']=entries
    report = dict(schema='halo2-vista-menu-reference-v1',sources=sources,owners=owners,pages=pages,
        unavailable_caches=unavailable,
        policy='Original decoded reference pages only; every replacement needs an explicit authored recipe.',
        summary=dict(banks={n:len(b) for n,b in owners.items()},
                     pages={n:sum(len(b['frames']) for b in bs) for n,bs in owners.items()},
                     unique_reference_pages=len(pages)))
    (output/'reference.json').write_text(json.dumps(report,indent=2)+'\n')
    return report


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--vista',type=Path,required=True)
    p.add_argument('--output',type=Path,default=ROOT/'assets/vista-menu-svg-4k')
    args=p.parse_args()
    print(json.dumps(collect(args.vista,args.output)['summary'],indent=2))


if __name__=='__main__':main()
