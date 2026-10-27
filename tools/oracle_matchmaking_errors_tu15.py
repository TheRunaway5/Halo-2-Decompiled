#!/usr/bin/env python3
"""Execute the unchanged TU1.5 lobby error selector and state-clear owner.

The widget constructor is a stop boundary, not a substituted callee. Original
getters, local-host admission and the state-clear writer execute in full.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import sys
ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'tools'),str(ROOT/'tools/reference/halo2-decompiled/tools')]
from oracle_game_variants import Source, STACK, STOP, MENU_PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from inspect_multiplayer import Cache
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EDI,UC_X86_REG_ESP,UC_X86_REG_EIP

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report-output',type=Path,required=True);a=p.parse_args()
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    src=Source(dict(path=str(NEW_DEFAULT),pin=NEW_PIN,globals=0x51d048));g=src.g
    screen,session=0x1100000,0x1102000;g.mem_map(screen,0x10000)
    def put(at,v):g.mem_write(at,struct.pack('<I',v&0xffffffff))
    rows=[]
    for state in range(-2,23):
      for local_cancel in (False,True):
        g.mem_write(screen,bytes(0x10000));put(0x535f34,session);g.mem_write(0x535f00,b'\1')
        put(session+0x741c,5);put(session+0x49f8,state);put(session+0x50,0);put(session+0x72d8,0)
        g.mem_write(0x52d729,bytes([local_cancel]));put(0x55c448,100)
        hit=[]
        def observe(_g,at,_size,_):
          if at==0x195d21:
            sp=g.reg_read(UC_X86_REG_ESP);hit.append(src.u(sp+8));g.emu_stop()
        h=g.hook_add(UC_HOOK_CODE,observe)
        try:
          put(STACK,STOP);g.reg_write(UC_X86_REG_ESP,STACK);g.reg_write(UC_X86_REG_EDI,screen)
          g.emu_start(0x256af7,STOP,count=10000)
        finally:g.hook_del(h)
        expected=None if state in (0,2,3) or (state==4 and local_cancel) else 0xac if state==5 else 0xad if state==4 else 0xae if state in (6,14,19) else 0xab
        assert hit==([] if expected is None else [expected]),(state,local_cancel,hit,expected,hex(g.reg_read(UC_X86_REG_EIP)))
        rows.append(dict(state=state,local_cancel=local_cancel,dialog=expected))
    clears=[]
    for shown,now,host,state,expected in ((100,3099,True,1,1),(100,3100,True,1,0),(100,4000,False,1,1),(100,4000,True,0,0),(0xfffffff0,0xbc0,True,1,0)):
      g.mem_write(screen,bytes(0x10000));put(0x535f34,session);g.mem_write(0x535f00,b'\1');put(session+0x741c,5)
      put(session+0x49f8,state);put(session+0x50,0);put(session+0x72d8,0 if host else 1)
      put(screen+0x1470,shown);put(0x55c448,now)
      src.call(0x256af7,registers=((UC_X86_REG_EDI,screen),))
      assert src.u(session+0x49f8)==expected,(shown,now,host,state,hex(src.u(session+0x49f8)))
      clears.append(dict(shown=shown,now=now,local_host=host,input_state=state,result_state=expected,shown_after=src.u(screen+0x1470)))
    cache=Cache(ROOT/'assets/disc/maps/mainmenu.map');assert hashlib.sha256(cache.data).hexdigest()==MENU_PIN
    root=next(t['offset'] for t in cache.tags.values() if t['group']=='matg')
    n,size,ix,st=struct.unpack_from('<4I',cache.data,root+0x190)
    loc=[(cache.u(ix+i*8),cache.text(st+cache.u(ix+i*8+4))) for i in range(n)]
    global_ui=next(t['offset'] for t in cache.tags.values() if t['group']=='wigl');dialogs=[]
    for group in cache.block(global_ui+0x88,40):
      u=cache.tags[cache.u(group+12)]['offset'];start,length=struct.unpack_from('<HH',cache.data,u+16);strings=dict(loc[start:start+length])
      for ent in cache.block(group+32,24):
        if cache.u(ent) in (0xab,0xac,0xad,0xae):
          dialogs.append(dict(id=cache.u(ent),file_offset=ent,strings=[strings.get(cache.u(ent+8+i*4) or cache.u(group+16+i*4)) for i in range(4)]))
    report=dict(passed=True,source_sha256=NEW_PIN,cache_sha256=MENU_PIN,selector='0x256AF7',widget_stop='0x195D21',selector_controls=rows,state_clear_controls=clears,dialogs=dialogs,
      scope=__doc__,executed_distinct_instructions=len(src.executed-{0x195d21}),tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    encoded=json.dumps(report,indent=2)+'\n';assert len(encoded.encode())<=1024*1024
    a.report_output.parent.mkdir(parents=True,exist_ok=True);a.report_output.write_text(encoded)
    print(json.dumps(dict(passed=True,selector_controls=len(rows),clear_controls=len(clears),report=str(a.report_output))))
if __name__=='__main__':main()
