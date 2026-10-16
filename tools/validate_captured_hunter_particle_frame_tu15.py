"""Authenticate actual179DF0 particle API inputs in the Hunter firing diagnostic."""
import json,struct
from pathlib import Path
from audit_rendering import Cache,file_sha
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from validate_charge_effect_bank_tu15 import SCOPE,SCOPE_PIN


def validate(case,metadata,rows):
    assert metadata['entry']==0x179df0 and metadata['actual_hunter_particle_API']
    assert metadata['declared_lower_actor_birth'] and not metadata['public_campaign_placement'] and metadata['completed']
    build=json.loads((case/'build.json').read_text());assert build['returncode']==0 and not build['scope']['public_campaign_placement']
    for name in ('before.bin','after.bin','native-input.bin','native-input-full.bin','native-fp-input.bin','native-x87-input.bin','native-exit.bin','native-trace.bin','native-stores.bin','native-cpu-inputs.bin','native.json'):
        assert file_sha(case/name)==build['files_sha256'][name]
    assert file_sha(Path(build['source']))==build['source_sha256']
    assert file_sha(Path(build['command'][0]))==build['binary_sha256']
    for path,digest in build['linked_files_sha256'].items():assert file_sha(Path(path))==digest
    for record in build['override_sources'].values():assert file_sha(Path(record['source']))==record['source_sha256']
    parent=Path(build['command'][3]);parent_build=json.loads((parent/'build.json').read_text())
    assert parent_build['returncode']==0 and file_sha(parent/'native.json')==parent_build['files_sha256']['native.json']
    observation=json.loads((parent/'native.json').read_text())
    assert observation['declared_lower_actor_birth'] and not observation['public_campaign_placement']
    assert observation['completed_ticks']==180 and observation['hunter']==metadata['unit'] and observation['weapon']==metadata['weapon']
    assert file_sha(SCOPE)==SCOPE_PIN and file_sha(NEW_DEFAULT)==NEW_PIN
    path=Path(build['command'][2]).resolve();pin=next(m['sha256'] for m in json.loads(SCOPE.read_text())['maps'] if Path(m['path'])==path);assert file_sha(path)==pin
    def read(p,n=4):
        for base,data in rows:
            if base<=p and p+n<=base+len(data):return data[p-base:p-base+n]
        raise AssertionError(hex(p))
    def u(p,n=4):return int.from_bytes(read(p,n),'little')
    raw=(case/'native-input.bin').read_bytes();assert len(raw)==168;gp=struct.unpack_from('<8I',raw)
    assert gp==(0,0,0,0,0x600f0000,0,0,0) and raw[32:160]==bytes(128) and struct.unpack_from('<II',raw,160)==(2,0x60200000)
    assert u(gp[4])==0x60300000 and u(gp[4]+4)==metadata['delta_bits']==u(u(0x51df54)+4)
    fp=(case/'native-fp-input.bin').read_bytes();depth,status,cw,mxcsr=struct.unpack('<IHHI',fp)
    assert (depth,status,cw,mxcsr)==(0,0,0x27f,0x1f80) and (case/'native-x87-input.bin').stat().st_size==0
    assert (case/'native-input-full.bin').read_bytes()==raw+fp and u(u(0x51df54)+8)==metadata['tick']
    headers=u(u(0x4ecbd0)+0x44)
    def body(datum):
        header=headers+(datum&65535)*12;assert u(header,2)==datum>>16;return u(header+8)
    hunter=body(metadata['unit']);actor=u(u(0x5020c8)+0x44)+(metadata['actor']&65535)*0x888
    assert u(actor,2)==metadata['actor']>>16 and u(actor+0x18)==metadata['unit'] and u(hunter+0x12c)==metadata['actor']
    weapon=body(metadata['weapon']);assert u(weapon+0x14)==metadata['unit']
    for slot,stride in ((0x4f720c,400),(0x51df74,84),(0x52d71c,52),(0x52d714,64)):
        pool=u(slot);assert pool and u(pool+0x24)==stride
    cache=Cache(path)
    try:
        tag=cache.tags[u(hunter)];assert tag['group']=='bipd' and tag['name']=='objects\\characters\\hunter\\hunter'
        weapon_tag=cache.tags[u(weapon)];assert weapon_tag['group']=='weap' and weapon_tag['name']=='objects\\characters\\hunter\\hunter_particle_cannon\\hunter_particle_cannon'
        return dict(map=str(path),map_sha256=pin,tick=metadata['tick'],actual_native_particle_API=0x179df0,caller_delta_bits=metadata['delta_bits'],caller_stack=gp[4],actual_api_trace_available=True,declared_lower_actor_birth=True,public_campaign_placement=False,rendered_sequence_verified=False,full_weapon_parity=False,physical_xbox_verified=False,validation_tool_sha256=file_sha(Path(__file__)))
    finally:cache.close()
