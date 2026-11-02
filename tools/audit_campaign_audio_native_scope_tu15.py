#!/usr/bin/env python3
"""Current native campaign audio/speech ownership against pinned TU1.5.

This is an ownership inventory, not playback, numerical or audible parity.
Candidate source presence is kept separate from semantic qualification and
the complete group gate. No source flags or runtime selections are changed.
"""
import argparse,hashlib,json,os,re
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    assert Path(os.environ['HALO2_TEST_SCRATCH']).is_dir();assert sha(NEW_DEFAULT)==NEW_PIN
    paths=['src/audio/native_tu15_sound_campaign_audio.hpp','src/audio/native_tu15_sound_kernels_campaign_audio.cpp',
        *['src/audio/native_tu15_sound_kernels_campaign_'+part+'.cpp'for part in('playback','mix','channels')],
        *['src/audio/native_campaign_audio_'+part+'_adapter.hpp'for part in('ducking','fades','retirement','stop_callback')],
        'src/audio/native_tu15_sound_kernels_unit_speech.cpp','src/campaign/ai/source_unit_speech_tu15_generated.cpp',
        'src/physics/physics_live_world_tu15.cpp']
    pins={path:sha(ROOT/path)for path in paths};header=(ROOT/paths[0]).read_text()
    required_text=header.split('campaign_audio_required_entries{',1)[1].split('};',1)[0]
    required=[int(value,16)for value in re.findall(r'0x([0-9a-f]+)u',required_text)]
    assert len(required)==40 and len(set(required))==40
    candidates={}
    for path in paths[2:9]:
        text=(ROOT/path).read_text()
        for address,name,qualified in re.findall(r'\{0x([0-9a-f]+)u,\s*([a-zA-Z_][a-zA-Z0-9_:]*)\s*(,true)?\}',text):
            key=int(address,16)
            if key in candidates:assert candidates[key]['function']==name,(path,key)
            candidates[key]=dict(path=path,function=name,semantic_original_proven=bool(qualified))
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    def body_record(entry,listing=False):
        body=complete_body(code,entry);rows=[dict(instruction=hex(pc),bytes=code.at(pc).bytes.hex(),
            operation=code.at(pc).mnemonic,operands=code.at(pc).op_str)for pc in sorted(body.code)]
        out=dict(original_sites=len(body.code),body_sha256=hashlib.sha256(json.dumps(rows,separators=(',',':')).encode()).hexdigest(),
            calls=[dict(site=hex(pc),target=code.at(pc).op_str)for pc in sorted(body.code)if code.at(pc).mnemonic=='call'],
            direct_targets=[hex(target)for target in sorted(body.calls)],tail_targets=[hex(target)for target in sorted(body.tail_jumps)])
        if listing:out['instructions']=rows
        return out
    owners=[]
    for entry in required:
        candidate=candidates.get(entry);owners.append(dict(entry=hex(entry),candidate=candidate,
            status='semantic component proven'if candidate and candidate['semantic_original_proven']else'unqualified register-level candidate'if candidate else'absent',
            **body_record(entry,entry in(0x12ccc0,0x224c40,0x226c00))))
    speech_source=(ROOT/'src/campaign/ai/source_unit_speech_tu15_generated.cpp').read_text()
    speech=[int(value,16)for value in re.findall(r'^static void run_([0-9a-f]+)\(WorldState &s\) \{',speech_source,re.M)]
    assert len(speech)==9
    speech_registry=(ROOT/'src/audio/native_tu15_sound_kernels_unit_speech.cpp').read_text()
    registered_speech=[int(value,16)for value in re.findall(r'\{\s*0x([0-9a-f]+)u,\s*\+\[\]',speech_registry)]
    report=dict(passed=True,scope='read-only ownership and original dependency inventory',original_sha256=NEW_PIN,
        campaign_group=dict(required_count=len(required),qualified_required=[row['entry']for row in owners if row['status']=='semantic component proven'],
            unqualified_candidates=[row['entry']for row in owners if row['status']=='unqualified register-level candidate'],
            absent=[row['entry']for row in owners if row['status']=='absent'],owners=owners,
            complete_native_group_qualified=False,registry_withheld_until_complete=True,lifted_fallback_present=True),
        unit_speech=dict(native_unit_speech_registry_empty=not registered_speech,
            registered_native_entries=[hex(entry)for entry in registered_speech],
            registered_presence_is_not_semantic_qualification=True,whole_semantic_native_qualified=False,
            owners=[dict(entry=hex(entry),**body_record(entry))for entry in speech],
            reused_native_owners=['18F510 exists in native object-loop group','D3300 has a typed stop-notice dependency; unit-speech registration is inventoried above; whole speech remains open']),
        completion_requirements=['Cutscene scripted impulses, music, ambience and cinematic dialogue use complete generic native scheduling/playback and original audio clocks',
            'Sound effects preserve authored variation, gain, pitch, location, attenuation, occlusion, listener routing, looping, transitions and stop/retirement',
            'Dialogue preserves original eligibility/RNG/queues, interruption/priority/cancellation, sample progress, facial curves and authored synchronization',
            'Source decode/resample/mixing/filter/spatial/DSP behavior has independent original evidence; host audio device adaptation is qualified separately',
            'Pause/revert/checkpoint/BSP transitions and co-op listener/participant state retain one world authority',
            'No lifted/register-machine/binary-backed gameplay fallback on accepted audio paths; reject presence-only or packet-only completion claims',
            'Composed authored missions and native-only residual-identity census plus launcher qualification remain required'],
        source_sha256=pins,source_unchanged=all(sha(ROOT/path)==pin for path,pin in pins.items()),
        live_playback_qualified=False,audible_xbox_parity_qualified=False,launcher_promoted=False,
        owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({key:report['campaign_group'][key]for key in('qualified_required','unqualified_candidates','absent')}))


if __name__=='__main__':main()
