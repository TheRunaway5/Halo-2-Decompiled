"""Authenticate a captured actual Hunter AI call in a source-world tick.

The diagnostic warms up60 object ticks, performs authored squad birth and
then enables AI/object/particle ticks. A failed native continuation is allowed
as original-only discovery input; it is never certified as a passing replay.
"""
import json,struct
from pathlib import Path
from audit_rendering import Cache,file_sha
from validate_charge_effect_bank_tu15 import SCOPE,SCOPE_PIN

def validate(case,metadata,rows):
    assert metadata['entry']==0x1d0670 and metadata['ordinal']>=1
    assert file_sha(SCOPE)==SCOPE_PIN
    build=json.loads((case/'build.json').read_text())
    assert build['returncode']==(0 if metadata['completed'] else 1)
    assert build['scope']['ai_frame_enabled_after_actor_birth']
    for name in ['before.bin','after.bin','native-input.bin','native-exit.bin','native-stores.bin','native-trace.bin','native.json']:
        assert file_sha(case/name)==build['files_sha256'][name]
    path=Path(build['command'][2]);scope=json.loads(SCOPE.read_text())
    expected=next(m['sha256'] for m in scope['maps'] if Path(m['path'])==path)
    assert file_sha(path)==expected
    def read(address,size):
        for p,b in rows:
            if p<=address and address+size<=p+len(b):return b[address-p:address-p+size]
        raise AssertionError(hex(address))
    def u(p,n=4):return int.from_bytes(read(p,n),'little')
    clock=u(0x51df54);assert u(clock+8)==metadata['tick'] and 60<=metadata['tick']<150
    assert u(clock+2,2)==30 and u(u(0x4f3218)+8)==1
    assert u(u(0x5020a8)+1,1)==1 and u(0x5020c4)!=0
    raw=(case/'native-input.bin').read_bytes();assert len(raw)==168
    assert struct.unpack_from('<8I',raw)==(0,0,0,0,0x600f0000,0,0,0)
    assert raw[32:160]==bytes(128) and struct.unpack_from('<II',raw,160)==(2,0x60200000)
    assert u(0x600f0000)==0x60300000
    actors=u(0x5020c8);assert u(actors+0x20)==256 and u(actors+0x24)==0x888 and u(actors+0x29,1)==1
    actor=metadata['actor'];record=u(actors+0x44)+(actor&65535)*0x888
    assert u(record,2)==actor>>16 and u(record+0x54)==metadata['character']
    assert u(record+0x18)==metadata['unit'] and u(record+0x30)==31
    objects=u(0x4ecbd0);assert u(objects+0x24)==12
    def object(datum):
        header=u(objects+0x44)+(datum&65535)*12
        assert u(header,2)==datum>>16
        return u(header+8)
    hunter=object(metadata['unit']);weapon=object(metadata['weapon']);player_unit=object(metadata['player_unit'])
    assert u(hunter+0x12c)==actor and u(weapon+0x14)==metadata['unit']
    players=u(0x4f54f4);assert u(players+0x24)==0x21c and u(players+0x3c)==1
    player=metadata['player'];player_record=u(players+0x44)+(player&65535)*0x21c
    assert u(player_record,2)==player>>16 and u(player_record+0x2c)==metadata['player_unit']
    assert u(player_unit+0x13c)==player and u(u(0x5020a4))==player
    cache=Cache(path)
    try:
        character=cache.tags[metadata['character']];assert character['group']=='char'
        assert character['name']=='objects\\characters\\hunter\\ai\\hunter'
        for body,group,name in [(hunter,'bipd','objects\\characters\\hunter\\hunter'),
                (weapon,'weap','objects\\characters\\hunter\\hunter_particle_cannon\\hunter_particle_cannon')]:
            tag=cache.tags[u(body)];assert tag['group']==group and tag['name']==name
            root=u(u(0x4f0414)+(u(body)&65535)*16+8);assert root==tag['address']
        return dict(map=str(path),map_sha256=expected,character=character['name'],character_datum=metadata['character'],
            tick=metadata['tick'],ordinal=metadata['ordinal'],actor=actor,unit=metadata['unit'],weapon=metadata['weapon'],
            player=player,authored_squad=31,actual_ai_frame_caller=True,native_completed=metadata['completed'],
            warmup_without_ai_frames=True,complete_campaign_boot_verified=False,validation_tool_sha256=file_sha(Path(__file__)))
    finally:cache.close()
