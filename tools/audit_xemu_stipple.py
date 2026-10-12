#!/usr/bin/env python3
"""Read-only audit of the retained Xemu executable's exact PGRAPH sources."""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
from urllib.request import urlopen
import hashlib,json,re,subprocess,tomllib

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/multiplayer-shading/stipple-coordinate-audit/xemu-retained'
OUT.mkdir(parents=True,exist_ok=True)
log=ROOT/'analysis/multiplayer-shading/original-xemu/session.log'
exe=ROOT/'.tools/xemu-app/squashfs-root/usr/bin/xemu'
commit=re.search(r'^xemu_commit: ([a-f0-9]{40})$',log.read_text(),re.M)[1]
version=re.search(r'^xemu_version: (.+)$',log.read_text(),re.M)[1]
def digest(raw):return hashlib.sha256(raw).hexdigest()
tree_raw=urlopen(f'https://api.github.com/repos/xemu-project/xemu/git/trees/{commit}?recursive=1').read()
tree=json.loads(tree_raw);assert not tree.get('truncated')
(OUT/'tree.json').write_bytes(tree_raw)
entries={x['path']:x for x in tree['tree']}
paths=[x['path']for x in tree['tree']if x['type']=='blob'and
       x['path'].startswith('hw/xbox/nv2a/pgraph/')and
       x['path'].endswith(('.c','.h','.inc'))]
paths.append('hw/xbox/nv2a/nv2a_regs.h')
def fetch(path):
 url=f'https://raw.githubusercontent.com/xemu-project/xemu/{commit}/{path}'
 dest=OUT/path;raw=dest.read_bytes()if dest.exists()else urlopen(url).read()
 assert hashlib.sha1(b'blob '+str(len(raw)).encode()+b'\0'+raw).hexdigest()==entries[path]['sha'],path
 dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(raw)
 text=raw.decode();mentions=[dict(line=i,text=line.strip())for i,line in enumerate(text.splitlines(),1)
                          if re.search(r'stipple',line,re.I)]
 return dict(path=path,url=url,sha256=digest(raw),stipple_mentions=mentions)
with ThreadPoolExecutor(max_workers=8)as pool:files=list(pool.map(fetch,paths))
table_source='''#include <stdio.h>
#include "hw/xbox/nv2a/nv2a_regs.h"
static const char *table[0x800]={
#define DEF_METHOD(g,n) [g##_##n>>2]=#g "_" #n,
#define DEF_METHOD_RANGE(g,n,r) [g##_##n>>2 ... (g##_##n+4*(r)-1)>>2]=#g "_" #n,
#define DEF_METHOD_CASE_4_OFFSET(g,n,o,s) [(g##_##n+o)>>2]=#g "_" #n,[(g##_##n+o+s)>>2]=#g "_" #n,[(g##_##n+o+2*s)>>2]=#g "_" #n,[(g##_##n+o+3*s)>>2]=#g "_" #n,
#define DEF_METHOD_CASE_4(g,n,s) DEF_METHOD_CASE_4_OFFSET(g,n,0,s)
#include "hw/xbox/nv2a/pgraph/methods.h.inc"
};
int main(void){for(unsigned m=0x1480;m<0x1500;m+=4){printf("%04X %s\\n",m,table[m>>2]?table[m>>2]:"NULL");if(table[m>>2])return 1;}return 0;}
'''
(OUT/'method-table.c').write_text(table_source)
subprocess.run(['cc','-I',str(OUT),str(OUT/'method-table.c'),'-o',str(OUT/'method-table')],check=True)
methods=subprocess.run([str(OUT/'method-table')],check=True,capture_output=True,text=True).stdout
(OUT/'method-table.log').write_text(methods)
config=ROOT/'analysis/multiplayer-shading/original-xemu/xemu.toml'
renderer=tomllib.loads(config.read_text())['display']['renderer']
report=dict(version=version,commit=commit,executable=str(exe.relative_to(ROOT)),
 executable_sha256=digest(exe.read_bytes()),session_log_sha256=digest(log.read_bytes()),
 configured_renderer=renderer,config_sha256=digest(config.read_bytes()),source_files=len(files),
 mask_upload_method_handlers=methods.splitlines(),
 stipple_mentions=sum(len(f['stipple_mentions'])for f in files),files=files,
 scope='Exact retained executable commit. Sources include method dispatch, shared fragment shader state/generation, Vulkan pipeline and drawing, and other PGRAPH backends. No guest operation or original process change.')
(OUT/'report.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items()if k!='files'},indent=2))
