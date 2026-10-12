#!/usr/bin/env python3
"""Execute original615EA9 and every consumed session/map/variant leaf unchanged.

The supplied lifecycle globals and complete field bank are explicit fixture
inputs. No query is hooked or replaced. The actual5AD736/5AD782 predicates,
dispatch tables, selected-map missing branch and variant pointer all execute.
This is original instruction evidence, not a running Windows game or UI match.
"""
from pathlib import Path
import hashlib
import itertools
import json
import struct
import sys
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_ESI
from oracle_vista_pcr_screen import machine,STOP,STACK,OBJ,SHA,w32

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/frontend/vista-session-flow-audit/pc14-layout-ownership-original'
sys.path.insert(0,str(OUT.parent/'local-session-inputs-original'))
from audit import PE,EXE,EXE_SHA

def sha(b):return hashlib.sha256(b).hexdigest()
def mainmenu_prefix(image,reason):
    """Actual408786/448D6F until449650's separate player initialization.

    Only CRT memset/bounded UTF16 copy and the explicit8-byte RNG service are
    supplied. The constructor, tick-rate getter and mainmenu field writes run.
    """
    u=machine(image);visits=0;services=[]
    def read(at):return struct.unpack('<I',u.mem_read(at,4))[0]
    def ret(value=0):
        s=u.reg_read(UC_X86_REG_ESP)
        u.reg_write(UC_X86_REG_EAX,value);u.reg_write(UC_X86_REG_EIP,read(s))
        u.reg_write(UC_X86_REG_ESP,s+4)
    def hook(m,pc,n,_):
        nonlocal visits
        visits+=1;s=m.reg_read(UC_X86_REG_ESP)
        if pc==0x449650:m.emu_stop();return
        if pc==0x687ba9:
            dst,value,count=(read(s+4+i*4) for i in range(3))
            m.mem_write(dst,bytes([value&255])*count);services.append('CRT_memset');ret(dst)
        elif pc==0x43804a:
            dst,src,capacity=(read(s+4+i*4) for i in range(3));text=bytearray()
            for i in range(capacity-1):
                c=bytes(m.mem_read(src+2*i,2))
                if c==b'\0\0':break
                text.extend(c)
            m.mem_write(dst,bytes(text)+b'\0\0');services.append('bounded_UTF16_copy');ret(dst)
        elif pc==0x5b5a84:
            assert read(s+4)==8;m.mem_write(read(s+8),b'\x5a'*8)
            services.append('explicit_8byte_RNG');ret()
    u.hook_add(UC_HOOK_CODE,hook)
    esp=STACK+0x18000;w32(u,esp,STOP);w32(u,esp+4,reason)
    u.reg_write(UC_X86_REG_ESP,esp);u.reg_write(UC_X86_REG_ESI,OBJ)
    u.emu_start(0x408786,STOP+1,count=10000)
    assert u.reg_read(UC_X86_REG_EIP)==0x449650
    raw=bytes(u.mem_read(OBJ,0x1188));path=raw[0x88:0x290].decode('utf-16-le').split('\0')[0]
    assert read(OBJ)==3 and read(OBJ+0x80)==read(OBJ+0x84)==0xffffffff
    assert path=='scenarios\\ui\\mainmenu\\mainmenu' and read(OBJ+0x3d0)==reason
    return dict(reason=reason,visits=visits,services=services,world_kind=read(OBJ),
        campaign_id=read(OBJ+0x80),map_id=read(OBJ+0x84),path=path,options_sha256=sha(raw),
        stop_before='00449650',limits=mainmenu_prefix.__doc__)

def execute(image,q):
    u=machine(image)
    for offset,value in ((0x73a0,q['state']),(0x1c,q['session_class']),
        (0x4c90,q['subtype']),(0x4cb8,q['campaign_id']),(0x4dd8,q['map_id']),
        (0x4cbc,q['path_word']),(0x4e80,q['engine'])):
        w32(u,OBJ+offset,value&0xffffffff)
    w32(u,0x820fc0,q['enabled']);w32(u,0x820fc4,q['lifecycle_kind']&0xffffffff)
    w32(u,0x820fe8,OBJ)
    esp=STACK+0x18000;w32(u,esp,STOP);u.reg_write(UC_X86_REG_ESP,esp)
    visits=0;owners=[]
    entries={0x5ad736,0x5ad782,0x5b1643,0x5b1313,0x5b126f,0x5c2321,
             0x5c2367,0x5c23a7,0x5c24db,0x5b0e43,0x5c225b}
    def hook(m,pc,n,_):
        nonlocal visits
        visits+=1
        if pc in entries:owners.append(f'{pc:08X}')
        if pc==STOP:m.emu_stop()
    u.hook_add(UC_HOOK_CODE,hook);u.emu_start(0x615ea9,STOP+1,count=10000)
    assert u.reg_read(UC_X86_REG_EIP)==STOP
    returned=u.reg_read(UC_X86_REG_EAX)
    return dict(input=q,returned=returned if returned<0x80000000 else returned-0x100000000,
                visits=visits,owners=owners)

def main():
    p=PE(EXE,EXE_SHA);image=p.data;assert sha(image)==SHA
    rows=[]
    def add(**change):
        q=dict(enabled=1,lifecycle_kind=1,state=5,session_class=1,subtype=1,
               campaign_id=-1,map_id=17,path_word=ord('l'),engine=2)
        q.update(change);rows.append(execute(image,q))
    for cls,campaign,engine,subtype in itertools.product(
        (-1,0,1,2,3),(-1,0,1,2,7),(-1,0,1,2,3,4,5,6,7,8,9,10,0x7fffffff),(0,1,2)):
        add(session_class=cls,campaign_id=campaign,engine=engine,subtype=subtype)
    for enabled,kind,state,campaign in itertools.product((0,1),
        (-1,0,1,2,3,4,5,6,7,0x7fffffff),(0,1,2,3,4,5,6,7,8,9,10,11,0xffffffff),(-1,2)):
        add(enabled=enabled,lifecycle_kind=kind,state=state,campaign_id=campaign)
    for cls,campaign,map_id,path_word in itertools.product((0,1,2),(-1,2),
        (0,17,0xffffffff),(0,ord('l'),0x1200)):
        add(session_class=cls,campaign_id=campaign,map_id=map_id,path_word=path_word)
    fixture=ROOT/'tests/fixtures/vista_session_frontend_protocol_original.json'
    fixture.write_text(json.dumps(rows,separators=(',',':'))+'\n')
    spans={name:p.span(a,b) for name,a,b in (
        ('current_session',0x5ad736,0x5ad75c),('parameter_session',0x5ad782,0x5ad7b7),
        ('protocol',0x615ea9,0x615fd9),('class_query',0x5b1643,0x5b169b),
        ('map_query',0x5b126f,0x5b1313),('variant_query',0x5b1313,0x5b136b),
        ('missing_map',0x5c2321,0x5c2367),('custom_map',0x5c2367,0x5c23a7),
        ('map_fields',0x5c23a7,0x5c2413),('variant_fields',0x5c24db,0x5c2513),
        ('subtype_query',0x5b0e43,0x5b0e9b),('subtype_fields',0x5c225b,0x5c2293),
        ('configure',0x617138,0x6171a0),('multiplayer_configuration',0x6169c5,0x616a87),
        ('map_path_domain',0x459a1f,0x459a60),('map_path_setter',0x61557f,0x615624),
        ('mainmenu_options_prefix',0x408786,0x4087be),('options_constructor',0x448d6f,0x448dd2),
        ('mainmenu_player_initialization',0x449650,0x4497ea),
        ('mainmenu_alternate_prefix',0x408efe,0x408f4a),
        ('mainmenu_fallback_prefix',0x409625,0x409678))}
    report=dict(source_sha256=SHA,tool_sha256=sha(Path(__file__).read_bytes()),
        cases=len(rows),instructions=sum(r['visits'] for r in rows),
        fixture_sha256=sha(fixture.read_bytes()),source_spans=spans,
        source_multiplayer=next(r for r in rows if r['input']==dict(enabled=1,lifecycle_kind=1,
            state=5,session_class=1,subtype=1,campaign_id=-1,map_id=17,path_word=ord('l'),engine=2)),
        wrong_cache_type_producer=next(r for r in rows if r['input']==dict(enabled=1,lifecycle_kind=1,
            state=5,session_class=1,subtype=1,campaign_id=2,map_id=17,path_word=ord('l'),engine=2)),
        mainmenu_producer=[mainmenu_prefix(image,reason) for reason in (0,7,0xffffffff)],
        limits=__doc__)
    OUT.mkdir(parents=True,exist_ok=True)
    (OUT/'protocol-original.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k not in ('source_spans','limits')}))

if __name__=='__main__':main()
