#!/usr/bin/env python3
"""Export/reexecute actual captured contact construction, never create contacts.

The full caller input is extracted from the immutable source-created power
module contact. Both images run their unchanged complete constructor on those
literal buffers. This is stored-state kernel replay, not new world admission.
"""
import argparse, csv, hashlib, json, struct
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_MEM
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn import x86_const as reg
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code, _trace, PIN
from oracle_mp_item_current_producers_1_1 import frozen_write, encoded
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN, full_tokens

SOURCE = ROOT / 'analysis/multiplayer/sandbox-source/power-module-schedule/frozen'
SOURCE_PIN = '948723b3077688265b4392ef130c679deb4744fdb41c6558f260520150d9185a'
OUT = ROOT / 'analysis/multiplayer/sandbox-source/contact-record'
OLD_ENTRY = 0x1d2df0
STOP = 0x3000000
STACK = 0x70008000

def sha(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def verify():
    assert sha(SOURCE/'manifest.json') == SOURCE_PIN
    manifest = json.loads((SOURCE/'manifest.json').read_text())
    for path, value in manifest['files'].items(): assert sha(ROOT/path) == value, path
    return len(manifest['files'])

def source_inputs():
    report = json.loads((SOURCE/'measured/report.json').read_text())
    call = next(c for c in report['calls'] if c['entry'] == '001d2df0')
    assert call['return_esp_delta'] == 40
    assert len(bytes.fromhex(call['contact48_before_hex'])) == 72
    buffers = [(int(call['contact_record_pointer'],16), call['contact48_before_hex'])]
    for body in call['source_entities']:
        buffers += [(int(body['pointer'],16),body['prefix80_hex']),
                    (int(body['property_pairs_pointer'],16),body['property_pairs_hex'])]
    expected = [s for s in report['stores'] if call['order'] < s['order'] < call['return_order']]
    return call, buffers, expected

def correspondence(old, new, out):
    oc, nc = _Code(old,'.text'), _Code(new,'.text')
    starts = {int(r['address'],16) for r in csv.DictReader(
        (ROOT/'analysis/map-pack-update/functions.tsv').open(),delimiter='\t')}
    f = _trace(oc, OLD_ENTRY, starts | {OLD_ENTRY})
    oi = [oc.at(p) for p in sorted(f.code)]
    # Exact first16 instructions contain no image-address operand. This only
    # finds candidates; the complete body/topology is verified below.
    prefix = b''.join(i.bytes for i in oi[:16])
    section = new.section('.text'); raw = new.section_bytes(section)
    hits=[]; cursor=0
    while True:
        cursor=raw.find(prefix,cursor)
        if cursor<0:break
        hits.append(section.va+cursor); cursor+=1
    assert len(hits)==1, hits
    target=hits[0]
    nf=_trace(nc,target,{target})
    ni=[nc.at(p) for p in sorted(nf.code)]
    assert full_tokens(oi,old)==full_tokens(ni,new)
    assert not f.sites and not nf.sites and not f.tables and not nf.tables
    assert len(oi)==len(ni)
    delta=target-OLD_ENTRY; refs=[]; pairs=[]
    for a,b in zip(oi,ni):
        assert b.address-a.address==delta
        pairs.append({'old_site':f'{a.address:08x}','later_site':f'{b.address:08x}'})
        for x,y in zip(a.operands,b.operands):
            if x.type==y.type==X86_OP_MEM and not x.mem.base and not y.mem.base:
                assert x.size==y.size
                refs.append(dict(old_site=f'{a.address:08x}',later_site=f'{b.address:08x}',
                    old_address=f'{x.mem.disp:08x}',later_address=f'{y.mem.disp:08x}',
                    width=x.size,old_hex=old.read(x.mem.disp,x.size).hex(),
                    later_hex=new.read(y.mem.disp,y.size).hex()))
        if a.bytes != b.bytes:
            assert a.address == 0x1d2e62 and len(refs)==1
            assert a.bytes[:2]==b.bytes[:2] and a.bytes[6:]==b.bytes[6:]
    assert len(refs)==1 and refs[0]['old_address']=='0052625c'
    rows=[]
    for label,ins,image in [('map_pack_1_1',oi,old),('title_update_1_5',ni,new)]:
        payload=''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in ins).encode()
        for i in ins:assert image.read(i.address,len(i.bytes))==i.bytes
        path=out/f'{label}.asm';frozen_write(path,payload)
        rows.append(dict(image=label,file=path.name,sha256=sha(path),rows=len(ins)))
    return dict(old_entry=f'{OLD_ENTRY:08x}',later_entry=f'{target:08x}',
        unique_exact_prefix_candidates=[f'{p:08x}'for p in hits],
        whole_normalized_body_and_internal_branch_topology_equal=True,
        every_instruction_offset_equal=True,paired_sites=pairs,
        paired_memory_operands=refs,listings=rows),f,nf

def execute(image, entry, call, buffers):
    guest=Uc(UC_ARCH_X86,UC_MODE_32); mapped=set()
    def backing(address,raw):
        if not raw:return
        end=address+len(raw)
        for page in range(address&~0xfff,(end+0xfff)&~0xfff,0x1000):
            if page not in mapped:guest.mem_map(page,0x1000);mapped.add(page)
        guest.mem_write(address,raw)
    for s in image.sections:
        backing(s.va, image.section_bytes(s)+bytes(max(0,s.vsize-len(image.section_bytes(s)))))
    for address,value in buffers:backing(address,bytes.fromhex(value))
    words=[STOP]+[int(v,16)for v in call['stack'][:9]]
    backing(STACK-0x4000,bytes(0x8000))
    backing(STACK,struct.pack('<10I',*words));backing(STOP,b'\xcc')
    for name,value in call['registers'].items():
        guest.reg_write(getattr(reg,'UC_X86_REG_'+name.upper()),int(value,16))
    guest.reg_write(reg.UC_X86_REG_ESP,STACK)
    stores=[];executed=[]
    def code(g,a,size,user):executed.append(a)
    def write(g,access,a,size,value,user):
        if not STACK-0x4000<=a<STACK+0x4000:
            stores.append(dict(instruction=f'{g.reg_read(reg.UC_X86_REG_EIP):08x}',
                address=f'{a:08x}',size=size,before_hex=bytes(g.mem_read(a,size)).hex(),
                written_hex=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()))
    guest.hook_add(UC_HOOK_CODE,code);guest.hook_add(UC_HOOK_MEM_WRITE,write)
    guest.emu_start(entry,STOP,count=100000)
    assert guest.reg_read(reg.UC_X86_REG_EIP)==STOP
    assert guest.reg_read(reg.UC_X86_REG_ESP)-STACK==40
    result=dict(entry=f'{entry:08x}',initial_registers=call['registers'],
        nine_stack_words=call['stack'][:9],declared_replay_stack=f'{STACK:08x}',
        input_buffers=[dict(address=f'{a:08x}',raw_hex=h)for a,h in buffers],
        output_buffers=[dict(address=f'{a:08x}',raw_hex=bytes(guest.mem_read(a,len(bytes.fromhex(h)))).hex())for a,h in buffers],
        returned_eax=f'{guest.reg_read(reg.UC_X86_REG_EAX):08x}',return_esp_delta=40,
        ordered_stores=stores,executed_addresses=[f'{a:08x}'for a in executed],supplied_services=[])
    assert result['returned_eax']==call['returned_eax']
    assert result['output_buffers'][0]['raw_hex']==call['contact48_after_hex']
    assert result['output_buffers'][1:]==result['input_buffers'][1:]
    return result

def fixtures(cases):
    rows=[]
    for label,c in cases:
        r=c['initial_registers']
        rows.append(' '.join(['C',label,c['entry'],r['eax'],r['ecx'],r['edx'],c['returned_eax']]+c['nine_stack_words']))
        for b in c['input_buffers']:rows.append(f'M {b["address"]} {b["raw_hex"]}')
        for b in c['output_buffers']:rows.append(f'A {b["address"]} {b["raw_hex"]}')
        for s in c['ordered_stores']:
            rows.append(f'W {s["instruction"]} {s["address"]} {s["before_hex"]} {s["written_hex"]}')
    return ('\n'.join(rows)+'\n').encode()

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,default=OUT/'measured/report.json');args=ap.parse_args()
    count=verify();oldpath=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert sha(oldpath)==PIN and sha(NEW_DEFAULT)==NEW_PIN
    old,new=Xbe(oldpath),Xbe(NEW_DEFAULT);call,buffers,expected=source_inputs()
    mapped,ot,nt=correspondence(old,new,args.output.parent)
    a=execute(old,OLD_ENTRY,call,buffers);b=execute(new,int(mapped['later_entry'],16),call,buffers)
    assert a['ordered_stores']==[{k:s[k]for k in ('instruction','address','size','before_hex','written_hex')}for s in expected]
    source_sites={p['old_site']:p['later_site']for p in mapped['paired_sites']}
    assert b['ordered_stores']==[dict(s,instruction=source_sites[s['instruction']])for s in a['ordered_stores']]
    assert a['output_buffers']==b['output_buffers']
    for record,trace in ((a,ot),(b,nt)):
        assert {int(v,16)for v in record['executed_addresses']}<=set(trace.code)
    fixture=args.output.parent/'fixtures.tsv';frozen_write(fixture,fixtures([('map_pack_1_1',a),('title_update_1_5',b)]))
    result=dict(schema=1,exporter_sha256=sha(__file__),source_manifest_sha256=SOURCE_PIN,
        source_manifest_verified_files=count,original_captured_call=call,
        old_image_sha256=PIN,later_owned_image_sha256=NEW_PIN,correspondence=mapped,
        cases=[a,b],fixture_file='fixtures.tsv',fixture_sha256=sha(fixture),
        source_original_outputs_equal=True,native_expected_values_used=False,
        bounds=['Captured original source-owned constructor input is replayed whole; no new live contact/world/solver admission is claimed.',
        'Original corresponding1.5 body is executed with the exact captured1.1 stored inputs; full latest-update world setup is not executed.',
        'Encoded counterpart class3/4 lookup via the image-specific global is source-listed but unexecuted; native module must reject that unavailable owner.',
        'No point/normal/impulse/impact update, bank allocation/free, damage/destruction or native runtime integration is produced by this constructor.'])
    verify();frozen_write(args.output,encoded(result))
    print(json.dumps(dict(report=sha(args.output),later_entry=mapped['later_entry'],stores=len(a['ordered_stores']),fixture=sha(fixture))))

if __name__=='__main__':main()
