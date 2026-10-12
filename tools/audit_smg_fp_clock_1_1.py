#!/usr/bin/env python3
"""Audit frozen original FP clocks and extract their native comparison input.

No native implementation supplies expected values. Scalar arithmetic checks
operate on the captured original channel calls, while expected fixtures retain
unchanged source phase output and original callback records.
"""
import hashlib,json,struct
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,PIN,CACHE_PIN,Xbe,_Code,Cache

OUT=ROOT/'analysis/multiplayer/map-pack/smg-fp-clock'
CLOCK='7d5548e26d3503ea1057aef3202a9285d7e9fa431d0a943c372788b64282b5d6'
SUITE='d831472491423ff7a2fe9db7a20daabc2727e29a1a340a70739efed07e4ccebb'
READY='634e30e563558ffeeb42669ecf8c9fbeab72a3d64db67a6f87a3f62f7e259797'
READY_REPORT='578faf3a028b8a5843b39491f6fd57539d5e317915b87e2e3d2321a3febe5a1d'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
u=lambda b,o:struct.unpack_from('<I',b,o)[0]
h=lambda b,o:struct.unpack_from('<H',b,o)[0]
f=lambda b,o:struct.unpack_from('<f',b,o)[0]
r32=lambda x:struct.unpack('<f',struct.pack('<f',x))[0]

def scalar_clock(report):
    clips=report['authored']['clips'];delta=f(bytes.fromhex(report['external_inputs']['time_record_hex']),4)
    constants={r['name']:struct.unpack('<f',bytes.fromhex(r['raw_hex']))[0]for r in report['authored']['constants']}
    eps=constants['end_epsilon'];fps=constants['frames_per_second'];count=0
    for call in report['calls']:
        if call['entry']!='001ca2b0'or call['phase']!='original_first_person_observer_and_clock'or call.get('channel_pointer')!='07600014':continue
        old=bytes.fromhex(call['channel_before_hex']);actual=bytes.fromhex(call['channel_after_hex'])
        clip=clips[u(old,4)>>16];options=h(old,0x12);flags=old[0x11];frame=f(old,0x1C);rate=f(old,0x18)
        timing=clip['timing_events'];event_mask=0;future=0
        if old[0x10]:
            assert actual[0x10]==old[0x10]-1 and actual[0x11]==flags and f(actual,0x1C)==frame
        elif options&1 and flags&9==0:
            assert options&2 and clip['loop_frame']==0 # Only these executed authored stock cases.
            nxt=r32(r32(r32(delta*rate)*fps)+frame);threshold=r32(float(clip['frames'])-eps)
            flags&=0xF1
            def mask(a,b):
                result=0
                for e in timing:
                    data=bytes.fromhex(e['raw_hex']);kind,at=struct.unpack('<hh',data)
                    if min(a,b)<=at<max(a,b):result|=1<<(kind&31)
                return result&0xFFFF
            event_mask|=mask(frame,nxt)
            if nxt>=threshold:
                event_mask|=mask(frame,struct.unpack('<f',bytes.fromhex('ffff7f7f'))[0])
                assert u(actual,4)==u(old,4) # Source variants retain the selected stock clip.
                if options&8:rate=1.
                nxt=r32(r32(nxt-threshold)+float(clip['loop_frame']))
                event_mask|=mask(float(clip['loop_frame']),nxt);flags|=4
            frame=max(0.,min(nxt,threshold))
            look=r32(r32(r32(delta*rate)*fps)+frame)
            flags=(flags|2)if look>=threshold else(flags&0xFD)
            end=float(clip['frames'])+1. if rate>0 else 0.
            for e in timing:
                kind,at=struct.unpack('<hh',bytes.fromhex(e['raw_hex']))
                if (frame<at<=end)if frame<=end else(end<=at<frame):future|=1<<(kind&31)
            assert actual[0x11]==flags,(report['scenario'],call['tick'],'flags',actual[0x11],flags)
            assert struct.pack('<f',f(actual,0x1C))==struct.pack('<f',frame),(report['scenario'],call['tick'],'frame')
            assert struct.pack('<f',f(actual,0x18))==struct.pack('<f',rate)
        assert h(actual,0x14)==event_mask and h(actual,0x16)==future,(report['scenario'],call['tick'],'event words')
        count+=1
    return count

def main():
    assert sha(ROOT/'tools/oracle_smg_fp_clock_1_1.py')==CLOCK
    assert sha(ROOT/'tools/oracle_smg_fp_ready_producer_1_1.py')==READY
    assert sha(OUT/'frozen/suite.json')==SUITE
    suite=json.loads((OUT/'frozen/suite.json').read_text())
    expected=[r for r in suite['cases']if not r['repeat']]
    x=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');assert hashlib.sha256(x.data).hexdigest()==PIN
    code=_Code(x,'.text');c=Cache(ROOT/'assets/disc/maps/lockout.map')
    assert sha(ROOT/'assets/disc/maps/lockout.map')==CACHE_PIN
    listings=[]
    for inventory in(OUT/'frozen/instructions/inventory.json',OUT/'ready-producer/instructions/inventory.json'):
        inv=json.loads(inventory.read_text())
        for row in inv['routines']:
            p=ROOT/row['path'];assert sha(p)==row['sha256'];n=0
            for line in p.read_text().splitlines():
                address,raw,*_=line.split();assert code.at(int(address,16)).bytes.hex()==raw;n+=1
            listings.append({'entry':row['entry'],'path':row['path'],'sha256':row['sha256'],'instructions_checked':n})
    cases=[];stats=[];all_events=[];scalar=0
    # The ready-producer extension replaces the earlier supplied-request probe
    # in the consumer fixture; that first frozen report remains unchanged.
    inputs=[(r['name'],ROOT/r['path'],r['sha256'])for r in expected if r['name']!='ready']
    inputs.append(('normal_wrapper_ready',OUT/'ready-producer/report.json',READY_REPORT))
    for name,p,pin in inputs:
        assert sha(p)==pin
        repeat=p.with_name(p.stem+'-reexecution.json');assert sha(repeat)==pin
        report=json.loads(p.read_text());assert report['xbe_sha256']==PIN and report['cache_sha256']==CACHE_PIN
        assert report['oracle_sha256']==(READY if name=='normal_wrapper_ready'else CLOCK)
        assert '00168866'not in report['supplied_services']and'0016a356'not in report['supplied_services']
        assert report['actual_first_person_clock_executed']and report['actual_first_person_decode_executed']
        for record in report['authored']['constants']:
            assert x.read(int(record['address'],16),4).hex()==record['raw_hex']
        raw_records=[]
        for clip in report['authored']['clips']:
            raw_records.extend([clip['record'],*clip['timing_events'],*clip['sound_events'],*clip['effect_events']])
        raw_records.extend(report['authored']['graph_sound_references']);raw_records.extend(report['authored']['graph_effect_references'])
        for rec in raw_records:
            raw=bytes.fromhex(rec['raw_hex']);off=rec['file_offset'];assert bytes(c.data[off:off+len(raw)])==raw
        with(ROOT/'assets/disc/maps/shared.map').open('rb')as shared:
            for res in report['resources']:
                assert res['location']==2;shared.seek(res['file_offset']);raw=shared.read(res['size'])
                assert hashlib.sha256(raw).hexdigest()==res['raw_sha256']
        events=[{'tick':a['tick'],'phase':a['phase'],'registers':a['registers'],
            'stack':a['stack'],'source_event_hex':a['source_event_hex'],'return_eax':a['return_eax']}
            for a in report['calls']if a['entry']=='001687c2']
        requests=[{'tick':a['tick'],'phase':a['phase'],'entry':a['entry'],'registers':a['registers'],
            'stack':a['stack'],'before':a['before'],'after':a['after']}
            for a in report['calls']if a['entry']in('0016a356','00168483','000e9d50','000efe30')]
        updates=[a for a in report['calls']if a['entry']in('001ca2b0','001ca090','001c9f50')and a['phase']=='original_first_person_observer_and_clock']
        for row in report['rows']:
            for phase in('before','after_unit','after_weapon','after_postpass','after_fp'):
                state=row[phase]
                for hand in state['hands']:
                    m=bytes.fromhex(hand['manager_hex']);assert len(m)==0x90
                    assert f'{u(m,4):08x}'==hand['channel_identifier']and f'{u(m,0x1C):08x}'==hand['frame_bits']
            full=bytes.fromhex(row['after_fp']['view_0_hex']);assert len(full)==0x20CC
            for i,hand in enumerate(row['after_fp']['hands']):
                assert full[0x14+i*0x1010:0xA4+i*0x1010].hex()==hand['manager_hex']
            assert len(bytes.fromhex(row['after_fp']['source_primary_scratch_1000_hex']))==0x1000
        scalar+=scalar_clock(report)
        cases.append({'name':name,'source_report_path':str(p.relative_to(ROOT)),'source_report_sha256':pin,
            'oracle_sha256':report['oracle_sha256'],'external_inputs':report['external_inputs'],
            'supplied_admission_boundary':report.get('supplied_admission_boundary'),
            'initial':report.get('initial_fp_state',report.get('initial_registered_instant_loadout')),
            'rows':report['rows'],'source_activity_requests':requests,'source_clock_updates':updates,
            'source_event_callbacks':events,'source_view_writes':report['writes'],
            'local_rng_stores':report['local_rng_stores'],'supplied_services':report['supplied_services'],
            'projectile_request_ticks':[a['tick']for a in report['projectile_requests']]})
        stats.append({'name':name,'source_report_sha256':pin,'ticks':len(report['rows']),
            'calls':len(report['calls']),'view_writes':len(report['writes']),'event_callbacks':len(events),
            'projectile_requests':len(report['projectile_requests'])})
        all_events.extend({'case':name,**e}for e in events)
        authored=report['authored'];resources=report['resources']
    compact=OUT/'source-fixture.json'
    compact.write_text(json.dumps({'xbe_sha256':PIN,'cache_sha256':CACHE_PIN,
        'frozen_suite_sha256':SUITE,'authored':authored,'resources':resources,'cases':cases,
        'scope':'Measured registered source clock/request/event/decode state. Canonical ready case is original C7E90 producer with supplied current-index NONE. Original data residency, world/audio/failed projectile leaves remain supplied; Xbox RCPSS matrix bit parity is unresolved.',
        'native_gameplay_update_applied':False},indent=2)+'\n')
    audit=OUT/'audit.json'
    audit.write_text(json.dumps({'oracle_sha256':CLOCK,'ready_producer_oracle_sha256':READY,
        'suite_sha256':SUITE,'independent_reexecution_byte_identical':True,'cases':stats,
        'total_ticks':sum(s['ticks']for s in stats),'raw_phase_snapshots':sum(s['ticks']for s in stats)*5,
        'primary_fp_scalar_clock_calls_verified':scalar,'original_event_callbacks':all_events,
        'instruction_listings':listings,'instructions_checked':sum(s['instructions_checked']for s in listings),
        'source_fixture_sha256':sha(compact),'prior_registration_tool_sha256':sha(ROOT/'tools/oracle_smg_fp_registration_1_1.py'),
        'xbox_matrix_bit_parity_verified':False,'native_gameplay_update_applied':False},indent=2)+'\n')
    print(json.dumps({'audit_sha256':sha(audit),'fixture_sha256':sha(compact),'primary_clocks':scalar,
        'ticks':sum(s['ticks']for s in stats),'instructions':sum(s['instructions_checked']for s in listings)}))

if __name__=='__main__':main()
