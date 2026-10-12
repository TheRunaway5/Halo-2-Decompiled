#!/usr/bin/env python3
"""Export literal source FP request outputs and declared original controls."""
import argparse, hashlib, json, struct
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
OLD=ROOT/'analysis/multiplayer/map-pack/smg-fp-sound'

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('controls',type=Path);ap.add_argument('providers',type=Path)
    ap.add_argument('output',type=Path);args=ap.parse_args()
    controls=json.loads(args.controls.read_text());providers=json.loads(args.providers.read_text())
    old=json.loads((OLD/'source-fixture.json').read_text())
    lines=['# original-fp-sound-request-v1',
        '# Source original positive sound system is unavailable; submit returns NONE.',
        '# Lower location callback rows are complete recorded original outputs, not native world pose production.']
    vectors={x['old_global']:x['raw12_hex'] for x in providers['vectors']}
    ordinary=next(x['old_address'] for x in providers['callbacks'] if x['old_table32_hex'][-8:]=='00000000')
    weapon=next(x['old_address'] for x in providers['callbacks'] if x['old_table32_hex'][-8:]!='00000000')
    lines.append('CONSTANTS '+vectors['0046fc28']+' '+vectors['0046fc48']+' '+ordinary+' '+weapon)
    lines.append('CURRENT_BSP '+controls['context']['current_bsp_word'])
    for d in controls['definitions']:
        lines.append('DEFINITION '+d['datum']+' '+d['definition20_hex']+' '+d['class92_hex'])
    for v,mode in zip(controls['context']['local_users'],controls['context']['camera_modes']):
        lines.append(f'LOCAL {v["view"]} {v["player"]} {v["unit"]} {mode:x}')
    for obj in controls['context']['objects']:
        lines.append('OBJECT '+obj['datum']+' '+obj['header12_hex'][6:8]+' '+obj['parent']+' '+
                     f'{obj["flags_12c"]:02x}'+' '+obj['associated_154'])
    for row in old['cases']:
        report=ROOT/row['source_report'];assert sha(report)==row['source_report_sha256']
        d=json.loads(report.read_text())
        event=next(x for x in d['calls'] if x['entry']=='001687c2')
        owner=next(x for x in d['calls'] if x['entry']=='0018dde0')
        submit=next(x for x in d['calls'] if x['entry']=='0021f220')
        play=bytes.fromhex(submit['play_state_before_hex'])
        marker=bytes.fromhex(row['marker_28_hex'])
        # Only fields actually written by 18AD70 and the complete 18DDE0 are
        # represented. Native pointers and unowned P-A0 padding are omitted.
        before=bytes.fromhex(owner['request_before_hex'])[:48]
        after=bytes.fromhex(owner['request_after_hex'])[:48]
        states=d['tick_states'];previous=next((x for x in states if x['tick']==row['tick']-1),None)
        beforestate=previous['after_fp']['hands'][0] if previous else {'sound_identifier':'ffffffff','sound_activity':'ffff'}
        activity=next(x for x in states if x['tick']==row['tick'])['after_fp']['hands'][0]['activity']
        assert after.hex()==row['defined_location_30_hex']
        assert struct.unpack_from('<I',play,0x50)[0]==int(row['owner_datum'],16)
        lines.append('CASE '+row['case']+' '+row['owner_datum']+' '+activity+' '+
            row['source_event_hex']+' '+beforestate['sound_identifier']+' '+beforestate['sound_activity']+' '+
            row['retained_hand_sound_identifier']+' '+row['retained_hand_activity']+f' {struct.unpack_from("<H",before,46)[0]:04x}')
        lines.append('BACKING '+marker.hex()+' '+before.hex())
        lines.append('OWNER_RESULT '+after.hex())
        lines.append('PLAY '+f'{struct.unpack_from("<I",play)[0]:08x} '+
            f'{struct.unpack_from("<I",play,4)[0]:08x} '+play[12:60].hex()+' '+
            f'{struct.unpack_from("<I",play,0x50)[0]:08x} '+
            f'{struct.unpack_from("<I",play,0x58)[0]:08x} '+play[0x5c:0x84].hex()+' '+
            f'{struct.unpack_from("<H",play,0x8c)[0]:04x} '+
            f'{struct.unpack_from("<I",play,0x98)[0]:08x}')
        lines.append('ENDCASE')
    for c in controls['cases']:
        if c['kind']=='event':
            processed=any(x['entry']=='0018aa90' for x in c['calls'])
            lines.append('GATE '+c['name']+f' {c["view"]} '+c['event_hex']+' '+
                c['hand_weapon']+' '+c['hand_activity']+' '+c['state_before']+' '+c['state_after']+f' {int(processed)}')
        elif c['kind']=='marker':
            lines.append('MARKER '+c['sound']+' '+c['owner']+' '+c['marker_before_hex']+' '+
                c['marker_after_hex']+f' {int(c["weapon_callbacks"])}')
        elif c['kind']=='audibility':
            lines.append('AUDIBILITY '+c['owner']+f' {c["requested"]:x} {c["result"]:x} '+c['resolved_view'])
    raw=('\n'.join(lines)+'\n').encode();args.output.parent.mkdir(parents=True,exist_ok=True)
    if args.output.exists():assert args.output.read_bytes()==raw,'Existing fixture differs'
    else:args.output.write_bytes(raw)
    metadata={'exporter_sha256':sha(Path(__file__)),'controls_sha256':sha(args.controls),
        'providers_sha256':sha(args.providers),'original_fixture_sha256':sha(OLD/'source-fixture.json'),
        'original_reports':{x['source_report']:x['source_report_sha256'] for x in old['cases']},
        'fixture_sha256':hashlib.sha256(raw).hexdigest(),'original_event_requests':3,
        'declared_controls':len(controls['cases']),
        'scope':'Literal source event/marker/location/play fields and source hand stores. Complete owner callback outputs supplied only as recorded test leaves. No successful admission/RNG/voice, native live pose or complete playback claim.'}
    meta=args.output.with_suffix('.json');encoded=(json.dumps(metadata,indent=2,sort_keys=True)+'\n').encode()
    if meta.exists():assert meta.read_bytes()==encoded
    else:meta.write_bytes(encoded)
    print(json.dumps(metadata))

if __name__=='__main__':main()
