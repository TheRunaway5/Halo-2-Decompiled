#!/usr/bin/env python3
"""Authenticate native source particle records across a completed Needler flow.

The producer checks each live full-identity system/location chain and records
its real authored particle definition. This authenticates native draw inputs;
it does not compare every new source call or GPU output with original Xbox.
"""
import argparse
import collections
import json
from pathlib import Path
import struct

from audit_rendering import Cache,file_sha


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--flow',type=Path,required=True)
    parser.add_argument('--baseline',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();flow=args.flow.absolute()
    report=json.loads((flow/'report.json').read_text());receipt=json.loads((flow/'run.json').read_text())
    build_path=Path(receipt['command'][0]).parent/'build.json';build=json.loads(build_path.read_text())
    baseline=json.loads((args.baseline/'report.json').read_text())
    assert receipt['returncode']==0 and report['passed'] and build['build_passed']
    assert file_sha(flow/'report.json')==receipt['actual_report_sha256']
    assert file_sha(build_path)==receipt['build_sha256']
    assert file_sha(Path(receipt['command'][0]))==receipt['binary_sha256']==build['binary_sha256']
    assert file_sha(Path(build['source']))==receipt['source_main_sha256']==build['source_sha256']
    assert report['ticks']==baseline['ticks'] and len(report['ticks'])==report['completed_ticks']==240
    assert report['rounds_consumed']==11 and report['parallel_native_shots']==0
    assert not report['source_state_or_game_results_supplied']
    assert len(build['reused_objects'])==26
    assert all(row['preprocessing_and_compiler_admission_exact'] for row in build['reused_objects'])
    for name,digest in build['linked_files_sha256'].items():assert file_sha(Path(name))==digest,name
    cache=Cache(Path(receipt['command'][1]));assert file_sha(Path(receipt['command'][1]))==receipt['map_sha256']
    data=(flow/'particle-draws.bin').read_bytes();offset=0;programs={};totals=collections.Counter();particles=set()
    samples=[];checked=[]
    for sample in report['particle_draw_samples']:
        assert sample['view'] in (0,1) and 0<=sample['tick']<240
        if sample['material_owner_readback_source_unchanged_checked']:
            checked.append((sample['tick'],sample['view'],sample['first_person']))
        for draw in sample['draws']:
            assert draw['live_material_owner_verified'] and draw['record_offset']==offset and draw['record_bytes']==3424
            assert struct.unpack_from('<8I',data,offset)==(sample['tick'],sample['view'],int(sample['first_person']),
                draw['effect'],draw['owner'],draw['template'],draw['pass'],draw['vertex'])
            _,particle=cache.tag(draw['particle_datum'],'prt3')
            assert cache.u16(particle+4)+7==draw['classification']
            definitions=list(cache.block(particle+0x8c,124));assert definitions
            assert cache.u32(definitions[0])==draw['template']
            key=(draw['vertex'],draw['classification'])
            if key not in programs:
                _,root=cache.tag(key[0],'vrtx');row=list(cache.block(root+4,28))[key[1]]
                size=cache.u32(row+12);start=cache.pointer(cache.u32(row+16),cache.magic,size)
                code=bytes(cache.data[start:start+size]);path=flow/f'vertex-program-{key[0]}-class-{key[1]}.bin'
                assert path.read_bytes()==code and size==draw['source_vertex_program_bytes']
                token,count=struct.unpack_from('<2H',code);assert token==0x2078 and len(code)==4+count*16 and 0<count<=136
                programs[key]=dict(vertex_datum=key[0],classification=key[1],bytes=size,instructions=count,source_sha256=file_sha(path))
            totals[(sample['first_person'],draw['effect'])]+=1;particles.add(draw['particle_datum']);offset+=3424
        samples.append(sample)
    assert offset==len(data) and len(samples)==810
    views=[(view['tick'],view['view']) for view in report['native_view_samples']]
    assert views==[(view['tick'],view['view']) for view in baseline['native_view_samples']]
    # The opposing player dies at165;175 has only the living view. Require
    # every actual checkpoint view, without inventing a dead-player view.
    assert set(checked)=={(tick,view,fp) for tick,view in views if tick in (80,175) for fp in (False,True)}
    assert any(s['tick']==80 and s['first_person'] and s['draws'] for s in samples)
    assert any(s['tick']==175 and not s['first_person'] and s['draws'] for s in samples)
    assert totals[(True,3820225087)]>0 and totals[(False,3819242032)]>0 and totals[(False,3819438643)]>0
    result=dict(passed=True,scope=__doc__,flow=str(flow),map=str(Path(receipt['command'][1]).absolute()),map_sha256=receipt['map_sha256'],
        source_draw_file=str(flow/'particle-draws.bin'),source_draw_sha256=file_sha(flow/'particle-draws.bin'),
        source_report_sha256=file_sha(flow/'report.json'),source_receipt_sha256=file_sha(flow/'run.json'),
        source_build_sha256=file_sha(build_path),source_main_sha256=build['source_sha256'],
        all_240_gameplay_rows_unchanged=True,baseline_report_sha256=file_sha(args.baseline/'report.json'),
        source_state_mutated=False,expanded_bank_written=False,source_local_players=2,
        live_material_identity_checks=True,complete_material_observation_source_unchanged_checked_at=checked,
        supporting_objects_reused_with_exact_admission=26,samples=samples,source_programs=list(programs.values()),
        authored_particle_definitions=len(particles),draws=sum(totals.values()),
        effect_totals=[dict(first_person=fp,effect=effect,name=cache.tags[effect]['name'],draws=count) for (fp,effect),count in sorted(totals.items())],
        original_all_calls_compared=False,graphics_execution_verified=False,original_framebuffer_comparison_verified=False,
        rendered_sequence_verified=False,audible_playback_verified=False,physical_xbox_verified=False,full_weapon_parity=False,
        tool_sha256=file_sha(Path(__file__)))
    assert not args.output.exists();args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({key:result[key] for key in ('passed','draws','authored_particle_definitions','all_240_gameplay_rows_unchanged','expanded_bank_written')}))


if __name__=='__main__':main()
