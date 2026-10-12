#!/usr/bin/env python3
"""Replay original primary sampling with captured channel/clock and literal assets.

The supplied local bank is an independent paused output and a replay destination.
An idempotence result only applies to components actually written by the original
decoder. A captured post-render channel does not establish earlier update timing.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from inspect_multiplayer import Cache
from oracle_multiplayer_hud_positions import Retail, ROOT, PIN
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ESI,
    UC_X86_REG_ESP, UC_X86_REG_XMM1, UC_X86_REG_XMM4)


def unpack(raw, form): return struct.unpack(form, raw)
def fword(word): return unpack(struct.pack('<I', word), '<f')[0]
def words(raw): return unpack(raw, '<'+str(len(raw)//4)+'I')
def digest(raw): return hashlib.sha256(raw).hexdigest()


CODECS = (0x28c470,0x28c4e0,0x28c510,0x2c4d60,0x2c4da0,0x2c4de0,
    0x28c960,0x28c750,0x28c790,0x28c880,0x28c6b0,
    0x28cdb0,0x28cf40,0x28d090,0x28c9e0,0x28cb70,0x28ccc0,
    0x28d560,0x28d6f0,0x28d840,0x28d180,0x28d320,0x28d470)


class Sampler(Retail):
    def __init__(self):
        super().__init__(); self.calls={}; self.dispatch=[]; self.decoder=[]
    def hook(self, uc, pc, size, user):
        if pc in (0x2798a0,0x27a100,0x279860,*CODECS):
            self.calls[f'{pc:08x}']=self.calls.get(f'{pc:08x}',0)+1
        if pc==0x279860:
            settings=bytes(uc.mem_read(0x504450,0x64))
            self.dispatch.append(dict(settings_hex=settings.hex(),
                frame_index=unpack(settings[0x14:0x18],'<I')[0],
                next_frame=unpack(settings[0x18:0x1c],'<I')[0],
                fraction_bits=f'{unpack(settings[0x1c:0x20],"<I")[0]:08x}'))
        if pc in CODECS:
            self.decoder.append(dict(entry=f'{pc:08x}',destination_pointer=self.r(0x5044c0),
                rotation_ordinal=self.r(0x5044b4),translation_ordinal=self.r(0x5044b8),
                scale_ordinal=self.r(0x5044bc),resource=self.r(0x504480)))
        super().hook(uc,pc,size,user)


def graph_setup(r,cache,graph_datum):
    tag=cache.tags[graph_datum];root=tag['offset'];va=root+cache.magic
    r.g.mem_map(0x80000000,0x4000000)
    r.g.mem_write(va,bytes(cache.data[root:root+tag['size']]))
    r.w(r.table+(graph_datum&0xffff)*16+8,va)
    return root,va


def resource(cache,root,index):
    at=cache.block(root+0x2c,0x6c)[index];clip=bytes(cache.data[at:at+0x6c])
    group=cache.block(root+0xac,20)[cache.u(at+0x24)];location=cache.u(group+8)
    assert location>>30==0, 'Only resident local-cache clip resources are covered'
    sf,af,movement,u04,static,u08,animated=unpack(clip[0x3c:0x4c],'<bbhhhii')
    assert movement==u04==u08==0 and cache.u(at+0x64)==0
    start=location+cache.u(at+0x28);size=sf+af+static+animated
    raw=bytes(cache.data[start:start+size])
    assert len(raw)==size
    return clip,raw,dict(static_flags=sf,animated_flags=af,static_size=static,animated_size=animated)


def source_keys(cache,root,clip,raw,sizes,frame):
    count=clip[0x13];node_at=cache.block(root+0xc,32);mask_bytes=((count+31)//32)*4
    records=[];masks=raw[sizes['static_size']+sizes['animated_size']:]
    assert len(masks)==sizes['static_flags']+sizes['animated_flags']
    for section in (0,1):
        size=sizes['animated_size'] if section else sizes['static_size']
        if not size:continue
        data=raw[sizes['static_size']:] if section else raw
        data=data[:size];codec=data[0];ordinals=[0,0,0]
        for n in range(count):
            sid=cache.u(node_at[n]);name=cache.text(cache.u(368)+cache.u(cache.u(364)+(sid&0xffff)*4))
            for channel in range(3):
                mask_at=(sizes['static_flags'] if section else 0)+channel*mask_bytes+(n//32)*4
                if not struct.unpack_from('<I',masks,mask_at)[0]&(1<<(n%32)):continue
                ordinal=ordinals[channel];ordinals[channel]+=1
                element=(8,12,4)[channel]
                if section and codec>=4:
                    descriptor_at=48 if channel==0 else struct.unpack_from('<I',data,8+channel*4)[0]
                    descriptor=struct.unpack_from('<I',data,descriptor_at+ordinal*4)[0]
                    start,key_count=descriptor>>12,descriptor&0xfff
                    width=2 if codec in (5,7) else 1
                    frame_base=struct.unpack_from('<I',data,20+channel*4)[0]
                    key_base=struct.unpack_from('<I',data,32+channel*4)[0]
                    times=[struct.unpack_from('<H' if width==2 else '<B',data,frame_base+(start+k)*width)[0] for k in range(key_count)]
                    low=max((k for k,t in enumerate(times) if t<=int(frame)),default=0)
                    selected=[low,min(low+1,key_count-1)]
                    ats=[key_base+(start+k)*element for k in selected]
                else:
                    base=32 if channel==0 else struct.unpack_from('<I',data,8+channel*4)[0]
                    stride=struct.unpack_from('<I',data,20+channel*4)[0] if section else element
                    selected=[int(frame),int(frame)] if section else [0,0]
                    times=selected;ats=[base+ordinal*stride+k*element for k in selected]
                form='<4h' if channel==0 else '<3f' if channel==1 else '<f'
                records.append(dict(node=n,name=name,component=channel,codec=codec,ordinal=ordinal,
                    key_times=times,selected_keys=selected,selected_times=[times[k] for k in selected] if section and codec>=4 else times,
                    selected_values=[list(struct.unpack_from(form,data,at)) for at in ats]))
        assert ordinals==list(data[1:4]), 'Source key masks disagree with actual codec counts'
    return records


def replay(cache,obj,interpolate,mask):
    r=Sampler();channel=bytes.fromhex(obj['primary_raw_hex']);graph,packed=unpack(channel[:8],'<2I')
    root,va=graph_setup(r,cache,graph);clip,raw,sizes=resource(cache,root,packed>>16)
    assert clip[0x10]==0 and clip[0x13]==obj['node_count']
    clip_at=r.alloc(clip);data=r.alloc(raw)
    descriptor=r.alloc(struct.pack('<IIBbh',data,clip_at+0x3c,clip[0x13],clip[0x11],struct.unpack_from('<H',clip,0x14)[0]))
    baseline=bytes.fromhex(obj['local_bank_hex']);dest=r.alloc(baseline)
    mask_ptr=r.alloc(struct.pack('<8I',*mask)) if mask else 0
    frame_bits=struct.unpack_from('<I',channel,0x1c)[0]
    r.run(0x2798a0,argv=[va,clip_at,obj['node_count'],0,mask_ptr,dest,interpolate,0,0,0],
        registers=[(UC_X86_REG_ESI,descriptor),(UC_X86_REG_XMM1,frame_bits),(UC_X86_REG_XMM4,0x3f800000)])
    output=bytes(r.g.mem_read(dest,len(baseline)));before,after=words(baseline),words(output)
    assert r.calls['002798a0']==r.calls['0027a100']==1 and r.calls['00279860']==2
    assert sum(r.calls.get(f'{pc:08x}',0) for pc in CODECS)>0, 'No real primary decoder executed'
    nodes=[];node_at=cache.block(root+0xc,32)
    for n in range(obj['node_count']):
        sid=cache.u(node_at[n]);name=cache.text(cache.u(368)+cache.u(cache.u(364)+(sid&0xffff)*4))
        b,a=before[n*8:(n+1)*8],after[n*8:(n+1)*8]
        errors=[abs(fword(x)-fword(y)) for x,y in zip(a,b)]
        nodes.append(dict(node=n,name=name,admitted=not mask or bool(mask[n//32]&(1<<(n%32))),
            captured_words=[f'{v:08x}' for v in b],original_replay_words=[f'{v:08x}' for v in a],
            differing_words=sum(x!=y for x,y in zip(a,b)),quaternion_error=max(errors[:4]),
            translation_error=max(errors[4:7]),scale_error=errors[7]))
    nongrip=[n for n in nodes if n['admitted'] and not any(s in n['name'] for s in ('finger','thumb','ring','index','pinky'))]
    return dict(interpolate=bool(interpolate),mask=[f'{v:08x}' for v in mask],resource_hex=raw.hex(),
        resource_sha256=digest(raw),clip_hex=clip.hex(),sizes=sizes,instructions=r.calls,
        dispatch=r.dispatch,decoder_calls=r.decoder,nodes=nodes,output_hex=output.hex(),
        maximum_nongrip_quaternion_error=max(n['quaternion_error'] for n in nongrip),
        maximum_nongrip_translation_error=max(n['translation_error'] for n in nongrip),
        differing_nongrip_words=sum(n['differing_words'] for n in nongrip),
        source_keys=source_keys(cache,root,clip,raw,sizes,int(fword(frame_bits)) if not interpolate else fword(frame_bits)))


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('snapshots',nargs='+',type=Path)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--regression-fixture',type=Path);args=p.parse_args()
    cache=Cache(ROOT/'assets/disc/maps/mainmenu.map');records=[];fixture_rows=[];fixture_cases=[];seen=set()
    for path in args.snapshots:
      report=json.loads(path.read_text())
      for captured in report.get('objects',[report]):
        obj=dict(captured)
        obj['primary_raw_hex']=obj.get('primary_raw_hex') or obj['channels'][0]['raw_hex']
        obj['game_time_raw_hex']=obj.get('game_time_raw_hex') or report['game_time_raw_hex']
        if 'deferred_node_mask_hex' not in obj:
            definition=cache.tags[int(obj['definition'],16)]['offset']
            model=cache.tags[cache.u(definition+0x38)]['offset']
            obj['deferred_node_mask_hex']=bytes(cache.data[model+0xa4:model+0xc4]).hex()
        r=Retail();channel=bytes.fromhex(obj['primary_raw_hex'])
        graph=struct.unpack_from('<I',channel)[0];graph_setup(r,cache,graph)
        packed=struct.unpack_from('<I',channel,4)[0]
        if (graph,packed) in seen:continue
        seen.add((graph,packed))
        r.w(0x510c54,r.alloc(bytes.fromhex(obj['game_time_raw_hex'])))
        at=r.alloc(channel);dest=r.alloc(bytes.fromhex(obj['local_bank_hex']))
        r.run(0x1c6f60,stop=0x279d80,argv=[0x3f800000,0,obj['node_count'],dest],registers=[(UC_X86_REG_EBX,at)])
        argv=r.read(r.g.reg_read(UC_X86_REG_ESP)+4,'<8I');assert argv[7] in (0,1)
        masks={'full':[],'deferred':list(words(bytes.fromhex(obj['deferred_node_mask_hex'])))}
        masks['initial']=[(~v)&0xffffffff for v in masks['deferred']]
        replays={f'{stage}-interpolate-{flag}':replay(cache,obj,flag,mask) for stage,mask in masks.items() for flag in (0,1)}
        records.append(dict(snapshot=str(path.resolve()),snapshot_sha256=digest(path.read_bytes()),
            object=obj['object'],primary_raw_hex=channel.hex(),game_time_raw_hex=obj['game_time_raw_hex'],
            source_channel_argv=[f'{v:08x}' for v in argv],source_interpolate=bool(argv[7]),
            replays=replays))
        if args.regression_fixture:
            clip=bytes.fromhex(replays['full-interpolate-0']['clip_hex'])
            count=struct.unpack_from('<H',clip,0x14)[0]
            frames=[0.,.99995,1.,1.00005,fword(struct.unpack_from('<I',channel,28)[0]),float(count)-.00005]
            for frame in frames:
                frame_word=struct.unpack('<I',struct.pack('<f',frame))[0]
                raw=bytearray(channel);struct.pack_into('<I',raw,28,frame_word)
                selected=dict(obj,primary_raw_hex=raw.hex())
                for stage,mask in masks.items():
                    result=replay(cache,selected,0,mask)
                    fixture_cases.append(dict(graph=f'{graph:08x}',clip=packed>>16,
                        frame_bits=f'{frame_word:08x}',stage=stage,instructions=result['instructions'],
                        resource_sha256=result['resource_sha256'],output_sha256=digest(bytes.fromhex(result['output_hex']))))
                    fixture_rows.append(' '.join([obj['definition'],f'{graph:08x}',f'{packed>>16:x}',
                        f'{obj["node_count"]:x}',f'{frame_word:08x}',f'{len(mask):x}',
                        *(f'{v:08x}' for v in mask),*(f'{v:08x}' for v in words(bytes.fromhex(result['output_hex'])))]))
    payload=dict(xbe_sha256=PIN,cache_sha256=digest(cache.data),records=records,
        boundary='Unchanged original1C6F60 and2798A0 plus actual primary codecs execute with captured channel/clock/assets. Replays start with actual captured locals. Nondeferred production timing and physical RCP/RSQRT approximations remain separate; paired live producer captures are recorded independently.')
    if args.regression_fixture:
        assert len(seen)==3 and len(fixture_rows)==54, 'All three menu primary clips are required'
        args.regression_fixture.write_text('# Unchanged original primary sampler output; hex unit graph clip nodes frame mask-count mask output.\n'+'\n'.join(fixture_rows)+'\n')
        payload.update(fixture=str(args.regression_fixture.resolve()),fixture_sha256=digest(args.regression_fixture.read_bytes()),fixture_cases=fixture_cases)
    args.output.write_text(json.dumps(payload,indent=2)+'\n')
    for row in records:
        print(row['object'],'source interpolate',row['source_interpolate'])
        for label,result in row['replays'].items():
            print(label,result['differing_nongrip_words'],result['maximum_nongrip_quaternion_error'],result['maximum_nongrip_translation_error'])


if __name__=='__main__':main()
