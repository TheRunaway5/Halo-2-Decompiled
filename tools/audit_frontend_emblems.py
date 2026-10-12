#!/usr/bin/env python3
"""Retain literal wgit+60 type9 and wgit+68 grid metadata plus skin+44 emblems."""
from pathlib import Path
import json,struct
from audit_rendering import Cache,file_sha
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'analysis/frontend/customization';c=Cache(ROOT/'assets/disc/maps/mainmenu.map')
def record(p):
 datum=lambda x:c.tags.get(c.u32(p+x))
 return dict(raw=c.data[p:p+36].hex(),flags=c.u32(p),animation=c.u16(p+4),delay=c.u16(p+6),layer=struct.unpack_from('<h',c.data,p+8)[0],page=struct.unpack_from('<h',c.data,p+10)[0],bitmap=datum(16),shader=datum(24),bounds=list(struct.unpack_from('<4h',c.data,p+28)))
screens=[];skins=[]
for tag in c.tags.values():
 if tag['group']=='wgit':
  _,p=c.tag(tag['datum'],'wgit');screen=c.u16(p+4)
  if screen not in [47,49,50,52,233]:continue
  panes=[]
  for i,q in enumerate(c.block(p+32,76)):
   grids=[]
   for g in c.block(q+68,24):
    grids.append(dict(raw=c.data[g:g+24].hex(),skin=c.tags.get(c.u32(g+8)),position=list(struct.unpack_from('<2h',c.data,g+12))))
   panes.append(dict(index=i,emblems=[record(e)for e in c.block(q+60,36)],grids=grids))
  screens.append(dict(screen=screen,tag=tag,panes=panes))
 if tag['group']=='skin':
  _,p=c.tag(tag['datum'],'skin');es=c.block(p+44,36)
  if es:skins.append(dict(skin=tag,emblems=[record(e)for e in es]))
(OUT/'emblem-definitions.json').write_text(json.dumps(screens,indent=2)+'\n');(OUT/'emblem-skin-definitions.json').write_text(json.dumps(skins,indent=2)+'\n')
print(json.dumps(dict(mainmenu_sha256=file_sha(c.path),screens=len(screens),emblem_skins=len(skins),pane_emblems=sum(len(p['emblems'])for s in screens for p in s['panes']))))
