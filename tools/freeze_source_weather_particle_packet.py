#!/usr/bin/env python3
"""Freeze one source-proven immediate weather packet from the actual391F0 pair.

This checks existing unchanged-execution outputs and literal immutable banks.
It creates no guest, GPU, renderer, simulation, clock or residency outcome.
The packet is an explicit captured diagnostic input; native live producers are
separate. SDK color byte order and immediate register conventions are retained.
"""
from pathlib import Path
import collections,hashlib,json,re,struct,sys
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from differential_source_object_admission_snapshot import Snapshot,PIN,uint,words,packed
from audit_rendering import Cache

def sha(raw):return hashlib.sha256(raw).hexdigest()
def main():
    entry=ROOT/'analysis/multiplayer-shading/original-weather-20261009/session1/lockout-weather-particle-entry'
    returned=ROOT/'analysis/multiplayer-shading/original-weather-20261009/session1/lockout-weather-particle-return'
    snapshot=Snapshot(entry);result=Snapshot(returned)
    replay_path=ROOT/'analysis/multiplayer-shading/weather-particle-entry-original.json'
    sprite_path=ROOT/'analysis/multiplayer-shading/weather-sprite-particle-entry.json'
    report=json.loads(replay_path.read_text());sprite=json.loads(sprite_path.read_text())
    assert report['source_xbe_sha256']==sprite['source_xbe_sha256']==PIN
    assert report['tool_sha256']==sha((ROOT/'tools/audit_source_weather_particle_entry_original.py').read_bytes())
    assert sprite['tool_sha256']==sha((ROOT/'tools/audit_source_weather_sprite_binding.py').read_bytes())
    r=report['original_draw_entry'];comparison=r['actual_return_comparison']
    assert r['completed_whole391F0'] and r['AL']==1 and not r['boundaries']
    assert not r['services_replaced'] and not r['source_instructions_patched']
    assert not comparison['source_written_mismatches'] and not comparison['register_mismatches'] and not comparison['XMM_mismatches']
    assert comparison['appended_command_equal']
    assert all(a['source_unchanged']and a['actual_return_unchanged']for a in comparison['input_arrays'].values())
    assert sha(snapshot.ram)==report['snapshot_ram_sha256'] and sha(result.ram)==comparison['ram_sha256']
    system=int(r['system_argument'],16);system_raw=snapshot.read(system,140)
    group=uint(system_raw,0x68);count,render,state_count,state=words(snapshot.read(group,16));assert count==state_count==1024
    players=uint(snapshot.read(0x4e8c20,4));view_count=struct.unpack('<h',snapshot.read(players+8,2))[0]
    view=struct.unpack('<h',snapshot.read(0x485600,2))[0];elapsed=uint(snapshot.read(0x509418,4))
    partition_count=min(max(view_count,1),4);per=count//partition_count;first=view*per
    assert struct.unpack_from('<H',system_raw,0x6c)[0]==1
    render_raw=snapshot.read(render,count*20);state_raw=snapshot.read(state,state_count*32)
    emitted=[]
    for i in range(first,first+per):
        age=struct.unpack_from('<f',state_raw,i*32+28)[0]
        if not age>0.:continue
        raw=render_raw[i*20:i*20+20];xyz=words(raw[:12]);size=struct.unpack_from('<f',raw,12)[0]
        w=uint(struct.pack('<f',size*16777215.));color=uint(raw,16)
        sdk_color=((color&255)<<16)|((color>>16)&255)|(color&0xff00ff00)
        emitted.append(dict(source_particle_index=i,source_v0_words=[*xyz,w],logical_color_word=color,sdk_v1_word=sdk_color))
    p=r['appended_commands'];packets=p['packets'];vertices=[packet for packet in packets if packet['method']=='1518']
    colors=[packet for packet in packets if packet['method']=='1944']
    assert len(emitted)==len(vertices)==len(colors)==238
    for expected,vertex,color in zip(emitted,vertices,colors):
        assert vertex['count']==4 and color['count']==1
        assert [int(v,16)for v in vertex['words']]==expected['source_v0_words']
        assert int(color['words'][0],16)==expected['sdk_v1_word']
    for i,packet in enumerate(packets):
        if packet['method']=='1518':assert i>0 and packets[i-1]['method']=='1944'
    point=next(packet for packet in packets if packet['method']=='0A30')
    modes=next(packet for packet in packets if packet['method']=='0318')
    fixed=next(packet for packet in packets if packet['method']=='043C')
    assert point['words']==['00000000','00000000','3C91A2B4','42800000','42800000','42800000','80000000','00000000']
    assert modes['words']==['00000001','00000001'] and fixed['words']==['00000008']
    outdir=ROOT/'analysis/multiplayer-shading/weather-particle-packet';outdir.mkdir(exist_ok=True)
    artifacts={}
    def save(name,raw):
        path=outdir/name;path.write_bytes(raw);artifacts[name]=dict(path=str(path.relative_to(ROOT)),bytes=len(raw),sha256=sha(raw))
    for name,at,n in [('system.bin',system,140),('published-group.bin',group,16),('render-inputs.bin',render,count*20),
        ('state-inputs.bin',state,state_count*32),('full-source-frame.bin',0x485600,0x298),
        ('entry-texture-stage-state.bin',0x404dd0,512)]:save(name,snapshot.read(at,n))
    save('return-texture-stage-state.bin',result.read(0x404dd0,512))
    for name,payload in r['source_payloads'].items():save(name+'.bin',packed([int(v,16)for v in payload['words']]))
    save('point-parameters.bin',packed([int(v,16)for v in point['words']]))
    save('emitted-source-v0-logical-color.bin',b''.join(packed([*v['source_v0_words'],v['logical_color_word']])for v in emitted))
    save('emitted-sdk-v0-color.bin',b''.join(packed([*v['source_v0_words'],v['sdk_v1_word']])for v in emitted))
    save('appended-command-span.bin',(ROOT/p['path']).read_bytes())
    assert artifacts['appended-command-span.bin']['sha256']==p['sha256']==comparison['actual_command_sha256']
    shader_datum=uint(result.read(0x51f0f0+4,4));assert shader_datum==0xe7e4065f
    table=uint(snapshot.read(0x4e3b44,4));shaderroot=uint(snapshot.read(table+(shader_datum&65535)*16+8,4))
    header=snapshot.read(shaderroot,12);assert uint(header,4)>0
    classification=snapshot.read(uint(header,8),28)
    program_size=uint(classification,12);program=snapshot.read(uint(classification,16),program_size)
    assert program_size==148 and sha(program)=='3f9e4e6030626339db80d3bb5bf9e4103936041f9bdcfd75626f18ca2ff1a1ee'
    save('source-vs22-class0.vp',program)
    c=Cache(ROOT/'assets/disc/maps/lockout.map')
    try:
        tag,offset=c.tag(shader_datum,'vrtx');classes=c.block(offset+4,28)
        authored=bytes(c.data[c.pointer(c.u32(classes[0]+16),c.magic,program_size):c.pointer(c.u32(classes[0]+16),c.magic,program_size)+program_size])
        assert authored==program
        shader_name=tag['name']
    finally:c.close()
    partition=sprite['sprite']['partitions'][0];assert partition['ready']==1 and partition['captured_physical_resource_exact']
    assert partition['decoded_header']==dict(width=32,height=32,depth=1,mips=4,dimensions=2,hardware_format=15)
    save('sprite-resident-header.bin',result.read(int(partition['header_pointer'],16),20))
    data=sprite['sprite']['partitions'][0]
    resource=next(x for x in sprite['raw_banks']if x['name']=='partition-0-authored-resource')
    save('sprite-resident-physical-partition.bin',(ROOT/resource['file']).read_bytes())
    assert artifacts['sprite-resident-physical-partition.bin']['bytes']==1408
    counts=collections.Counter(packet['method']for packet in packets)
    packet=dict(authority=__doc__,source_xbe_sha256=PIN,entry_RAM_SHA256=sha(snapshot.ram),return_RAM_SHA256=sha(result.ram),
        full_original_replay_SHA256=sha(replay_path.read_bytes()),sprite_binding_replay_SHA256=sha(sprite_path.read_bytes()),
        shader=dict(datum=f'{shader_datum:08X}',name=shader_name,compiled_class=0,bytes=program_size,sha256=sha(program)),
        sprite=dict(datum=sprite['sprite']['datum'],name=sprite['sprite']['name'],record_index=0,
            source_handle=partition['handle'],header_words=sprite['actual_return']['selected_header_words'],
            dimensions=partition['decoded_header'],raw_resource_bytes=1408,
            captured_physical_bytes_match_authored_partition=True,mip_levels=[32,16,8,4],
            live_native_residency_policy_provided=False),
        inputs=dict(system=f'{system:08X}',initialized_count=count,source_view_count=view_count,current_view=view,
            source_elapsed_word=f'{elapsed:08X}',range_first=first,range_count=per,strict_positive_age_emissions=len(emitted)),
        proof=dict(verified_original_instructions=r['verified_original_instructions'],executed_instruction_steps=r['executed_instruction_steps'],
            restored_pages=r['restored_pages_checked'],exact_nonstack_source_written_bytes=comparison['nonstack_source_written_bytes'],
            exact_registers=not comparison['register_mismatches'],exact_XMM=not comparison['XMM_mismatches'],
            appended_commands_bytes=p['bytes'],appended_commands_words=p['word_count'],appended_command_packets=p['packet_count'],
            actual_command_equal=True,surrounding_not_source_written_differences=sum(len(v['differences'])for v in comparison['surrounding_page_differences'].values())),
        point_state=dict(parameter_words=point['words'],mode_words=modes['words'],fixed_point_size_word=fixed['words'][0],
            scope='Literal SDK command inputs; programmableoPts consumption/point coverage/orientation/hardwareprecision remain unproved.'),
        emitted=emitted,packet_methods=dict(counts),artifacts=artifacts,tool_sha256=sha(Path(__file__).read_bytes()),
        native_packet_api_status='Explicit packet contract documented; no weather draw module/runtimeactivation added.',
        native_producers_still_explicit=['actual admitted view/clippedclusters','initialized current particle arrays and38450update',
            'source elapsed/auxiliaryRNG/intensity/wind','actual resource ready/current sprite residency','current source SDK point/raster sampler state'],
        GPU=False,guest_IO=False,production_activation=False)
    output=outdir/'packet.json';output.write_text(json.dumps(packet,indent=2)+'\n')
    manifest=dict(scope='Frozen source packet artifacts; source files and tool hashes are immutable inputs to these checks.',
        packet=dict(path=str(output.relative_to(ROOT)),sha256=sha(output.read_bytes())),
        dependencies={str(path.relative_to(ROOT)):sha(path.read_bytes())for path in [replay_path,sprite_path,
            ROOT/'tools/audit_source_weather_particle_entry_original.py',ROOT/'tools/audit_source_weather_sprite_binding.py',Path(__file__),
            entry/'export.json',returned/'export.json']},artifacts=artifacts)
    (outdir/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print(json.dumps(dict(packet=manifest['packet'],proof=packet['proof'],shader=packet['shader'],artifacts=len(artifacts)),indent=2))
if __name__=='__main__':main()
