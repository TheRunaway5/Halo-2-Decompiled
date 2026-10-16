"""Authenticate the real nested Hunter hit-effect creator and its full caller."""
import json,struct
from pathlib import Path
from audit_rendering import Cache,file_sha
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from validate_charge_effect_bank_tu15 import SCOPE,SCOPE_PIN

def validate(case,metadata,rows):
    assert metadata['entry']==0x178e30 and metadata['actual_nested_projectile_effect_caller'] and metadata['declared_lower_actor_birth']
    build=json.loads((case/'build.json').read_text());assert build['returncode']==0 and metadata['completed']
    for name in ('before.bin','after.bin','native-input.bin','native-input-full.bin','native-fp-input.bin','native-x87-input.bin','native-exit.bin','native-trace.bin','native-stores.bin','native.json'):
        assert file_sha(case/name)==build['files_sha256'][name]
    assert file_sha(Path(build['command'][0]))==build['binary_sha256']
    for path,digest in build['linked_files_sha256'].items():assert file_sha(Path(path))==digest
    for record in build['override_sources'].values():assert file_sha(Path(record['source']))==record['source_sha256']
    assert file_sha(SCOPE)==SCOPE_PIN and file_sha(NEW_DEFAULT)==NEW_PIN
    path=Path(build['command'][2]).resolve();scope=json.loads(SCOPE.read_text())
    pin=next(m['sha256'] for m in scope['maps'] if Path(m['path'])==path);assert file_sha(path)==pin
    def read(p,n=4):
        for base,data in rows:
            if base<=p and p+n<=base+len(data):return data[p-base:p-base+n]
        raise AssertionError(hex(p))
    def u(p,n=4):return int.from_bytes(read(p,n),'little')
    raw=(case/'native-input.bin').read_bytes();assert len(raw)==168
    gp=struct.unpack_from('<8I',raw);assert 0x60000000<=gp[4]<=0x601ffff8
    assert struct.unpack_from('<I',raw,164)[0]==0x60200000
    call=_Code(Xbe(NEW_DEFAULT),'.text').at(metadata['callsite'])
    assert call.mnemonic=='call' and call.operands[0].imm==0x178e30
    assert u(gp[4])==call.address+call.size and u(gp[4]+4)==metadata['request']
    request=metadata['request'];assert u(request+4)==metadata['effect_definition']
    fp=(case/'native-fp-input.bin').read_bytes();depth,status,cw,mxcsr=struct.unpack('<IHHI',fp)
    assert depth==0 and (case/'native-x87-input.bin').stat().st_size==0
    assert (case/'native-input-full.bin').read_bytes()==raw+fp
    assert cw&0x3f==0x3f and mxcsr&0x1f80==0x1f80
    assert u(u(0x51df54)+8)==metadata['tick']
    parent=Path(build['command'][3]);parent_report=json.loads((parent/'build.json').read_text())
    assert parent_report['returncode']==0
    assert file_sha(parent/'events.jsonl')==parent_report['files_sha256']['events.jsonl']
    instance=struct.unpack_from('<I',(case/'native-exit.bin').read_bytes())[0]
    matches=[json.loads(line) for line in (parent/'events.jsonl').read_text().splitlines() if json.loads(line)['kind']=='projectile-detonation' and json.loads(line)['effect_instance']==instance]
    assert len(matches)==1;event=matches[0]
    assert event['effect']==metadata['effect_definition'] and event['definition']==metadata['projectile_definition']
    headers=u(u(0x4ecbd0)+0x44)
    def body(datum):
        header=headers+(datum&65535)*12;assert u(header,2)==datum>>16;return u(header+8)
    projectile=body(event['projectile']);assert u(projectile)==metadata['projectile_definition']
    assert u(projectile+0xc8)==metadata['unit']
    actor=u(u(0x5020c8)+0x44)+(metadata['actor']&65535)*0x888
    assert u(actor,2)==metadata['actor']>>16 and u(actor+0x18)==metadata['unit']
    weapon=body(metadata['weapon']);assert u(weapon+0x14)==metadata['unit']
    cache=Cache(path)
    try:
        effect=cache.tags[metadata['effect_definition']];assert effect['group']=='effe' and effect['name']=='effects\\objects\\characters\\hunter\\hunter_beam_hit'
        definition=cache.tags[metadata['projectile_definition']];assert definition['group']=='proj' and definition['name']=='objects\\characters\\hunter\\hunter_particle_cannon\\projectiles\\particle_cannon_component'
        return dict(map=str(path),map_sha256=pin,tick=metadata['tick'],actual_projectile=event['projectile'],actual_nested_caller=metadata['callsite'],
            creator_request=request,effect=effect['name'],effect_instance=instance,caller_stack=gp[4],x87_depth=depth,x87_status=status,
            x87_control_word=cw,mxcsr=mxcsr,declared_lower_actor_birth=True,public_campaign_placement=False,
            full_flight_or_damage_verified=False,physical_xbox_verified=False,full_weapon_parity=False,validation_tool_sha256=file_sha(Path(__file__)))
    finally:cache.close()
