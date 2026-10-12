#!/usr/bin/env python3
"""Observe unchanged C0890 on genuine original player/weapon construction."""
import argparse,json,os,struct,subprocess
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EIP,UC_X86_REG_ESP
import oracle_mp_spawn_player_visibility as owning
from freeze_mp_native_sound_storage import sha
ROOT=owning.ROOT

def save(path,value):
    data=(json.dumps(value,sort_keys=True,indent=2)+'\n').encode();path.parent.mkdir(parents=True,exist_ok=True)
    if path.exists():assert path.read_bytes()==data,path
    else:path.write_bytes(data)

class Capture(owning.PlayerTrace):
    def __init__(self,*args,**kw):
        self.model_enabled=False;self.model_calls=[];self.model_pending=[];super().__init__(*args,**kw)
        self.g.hook_add(unicorn.UC_HOOK_MEM_READ,self.model_read)
        self.g.hook_add(unicorn.UC_HOOK_MEM_WRITE,self.model_write)
    def raw(self,address,length):return bytes(self.g.mem_read(address,length)).hex()
    def model_read(self,g,kind,address,length,value,user):
        pc=g.reg_read(UC_X86_REG_EIP)
        if self.model_pending and 0xc0890<=pc<=0xc0924:
            self.model_pending[-1][2]['reads'].append(dict(instruction=f'{pc:08x}',address=f'{address:08x}',bytes=length,raw_hex=self.raw(address,length)))
    def model_write(self,g,kind,address,length,value,user):
        pc=g.reg_read(UC_X86_REG_EIP)
        if self.model_pending and 0xc0890<=pc<=0xc0924:
            self.model_pending[-1][2]['writes'].append(dict(instruction=f'{pc:08x}',address=f'{address:08x}',bytes=length,before=self.raw(address,length),value=f'{value&((1<<(length*8))-1):0{length*2}x}'))
    def hook(self,g,address,size,user):
        if self.model_enabled:
            for pending in self.model_pending[:]:
                ret,esp,row=pending
                if address==ret and g.reg_read(UC_X86_REG_ESP)>esp:
                    row.update(return_eax=f'{g.reg_read(UC_X86_REG_EAX):08x}',outputs_after=[self.raw(int(p,16),4)for p in row['outputs']])
                    self.model_pending.remove(pending)
            if address==0xc0890:
                esp=g.reg_read(UC_X86_REG_ESP);obj=g.reg_read(UC_X86_REG_ECX)
                pool=self.u(0x4e78d0);table=self.u(0x4eb114);heads=self.u(pool+0x44)
                record=heads+(obj&65535)*12;body=self.u(record+8);definition=self.u(body)
                tag=lambda datum:self.u(table+(datum&65535)*16+8)
                definition_body=tag(definition);regions={}
                def retain(p,n):regions[f'{p:08x}']=self.raw(p,n)
                retain(pool,0x4c);retain(record,12);retain(body,4)
                retain(table+(definition&65535)*16,16);retain(definition_body,0x3c)
                hlmt=self.u(definition_body+0x38);details=dict(definition=f'{definition:08x}',definition_body=f'{definition_body:08x}',hlmt=f'{hlmt:08x}')
                if hlmt!=0xffffffff:
                    retain(table+(hlmt&65535)*16,16);model=tag(hlmt);retain(model,16)
                    mode=self.u(model+4);collision=self.u(model+0xc);details.update(model=f'{model:08x}',mode=f'{mode:08x}',collision=f'{collision:08x}')
                    if mode!=0xffffffff and collision!=0xffffffff:
                        retain(table+(mode&65535)*16,16);mode_body=tag(mode);retain(mode_body,0x80)
                        details.update(mode_body=f'{mode_body:08x}',counts=[self.u(mode_body+q)for q in (0x74,0x7c)])
                outputs=[self.u(esp+4),self.u(esp+8)]
                for p in outputs:retain(p,4)
                row=dict(object=f'{obj:08x}',pool=f'{pool:08x}',tag_table=f'{table:08x}',outputs=[f'{p:08x}'for p in outputs],
                    caller=f'{self.u(esp):08x}',regions_before=regions,details=details,reads=[],writes=[])
                self.model_calls.append(row);self.model_pending.append((self.u(esp),esp,row))
        super().hook(g,address,size,user)

def capture(out):
    for name,pin in owning.FROZEN.items():assert sha(ROOT/name)==pin,name
    with(out/'host-provider.stderr.log').open('w')as err:
        process=subprocess.Popen(['wine',str(owning.CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,
            stderr=err,text=True,env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(owning.CONTEXT_OUT/'wine-prefix')))
        try:
            context=json.loads(process.stdout.readline());trace=owning.prepare(context,trace_type=Capture)
            trace.model_enabled=True;assert owning.spawn_and_activate(trace,activate=False)
            assert not trace.model_pending and len(trace.model_calls)==2
            old=json.loads((ROOT/'analysis/multiplayer/integration/spawn-source/player-visibility/report.json').read_text())
            assert trace.player_stages==old['cases'][2]['stages'][:4]
            report=dict(schema='object-model-resource-original-owning-capture-v1',oracle_sha256=sha(Path(__file__)),
                image_sha256=sha(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'),
                calls=trace.model_calls,source_stages=trace.player_stages,instruction_patches=[],supplied_model_results=[],
                boundary='Exact inherited four-stage PlayerTrace preparation; no complete title/map/physics readiness or native Unit restoration.')
            save(out/'capture.json',report)
            print(json.dumps(dict(capture_sha256=sha(out/'capture.json'),calls=len(trace.model_calls),
                details=[c['details']for c in trace.model_calls])))
        finally:
            if process.poll()is None:process.stdin.write('x');process.stdin.flush();process.wait(timeout=30)
            for stream in (process.stdin,process.stdout):stream.close()
            assert process.returncode==0

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);capture(out)
