"""Authenticate a cold actor constructor's real source bank and authored character."""
import json,struct
from pathlib import Path
from audit_rendering import Cache,file_sha
from validate_charge_effect_bank_tu15 import SCOPE,SCOPE_PIN

def validate(case,metadata,rows):
    entry=metadata['entry'];binding=entry==0x1e9000;birth=entry==0x1e8410;registration=entry==0x1d03a0
    assert entry in (0x1e7e40,0x1e9000,0x1e8410,0x1d03a0)
    if not birth:assert metadata['actor_bound']==binding
    assert file_sha(SCOPE)==SCOPE_PIN
    build=json.loads((case/'build.json').read_text())
    # A retained failed native birth is a valid input for discovering the
    # original continuation. It is never accepted as a passing native proof.
    following_ai=False
    if birth and (case/'ai-frame1/native.json').is_file():
        following=json.loads((case/'ai-frame1/native.json').read_text())
        assert following['entry']==0x1d0670 and following['ordinal']==1 and following['tick']==60
        assert all(following[k]==metadata[k] for k in ('actor','unit','weapon','character'))
        assert build['scope']['ai_frame_enabled_after_actor_birth']
        assert not (case/'actor-failure.txt').exists()
        following_ai=True # The retained next call is reached only after birth returns.
    assert build['returncode']==0 or (birth and build['returncode']==1 and
        ((case/'actor-failure.txt').is_file() or following_ai))
    for name in ['before.bin','after.bin','native-input.bin','native-exit.bin','native-stores.bin','native-trace.bin','native.json']:
        assert file_sha(case/name)==build['files_sha256'][name]
    path=Path(build['command'][2]);scope=json.loads(SCOPE.read_text())
    expected=next(m['sha256'] for m in scope['maps'] if Path(m['path'])==path);assert file_sha(path)==expected
    def read(address,size):
        for p,b in rows:
            if p<=address and address+size<=p+len(b):return b[address-p:address-p+size]
        raise AssertionError(hex(address))
    def u(p,n=4):return int.from_bytes(read(p,n),'little')
    pool=u(0x5020c8);assert pool and u(pool+0x20)==256 and u(pool+0x24)==0x888
    assert u(pool+0x29,1)==1 and u(pool+0x3c)==(1 if binding else 0)
    assert u(u(0x5020a8)+1,1)==1 and u(0x5112d4)!=0
    assert u(u(0x51df54)+8)==(0 if registration else 60) and u(u(0x51df54)+2,2)==30
    raw=(case/'native-input.bin').read_bytes();assert len(raw)==168
    gp=struct.unpack_from('<8I',raw);assert gp[4]==0x600f0000 and u(gp[4])==0x60300000
    if binding:
        actor=metadata['actor'];assert gp[7]==actor and u(gp[4]+4)==metadata['unit']
        record=u(pool+0x44)+(actor&65535)*0x888
        assert u(record,2)==actor>>16 and u(record+0x18)==0xffffffff
        objects=u(0x4ecbd0);unit=metadata['unit'];header=u(objects+0x44)+(unit&65535)*12
        assert u(header,2)==unit>>16 and u(u(header+8)+0x12c)==0xffffffff
    elif birth:
        assert gp[0]==metadata['declared_squad'] and gp[2]==metadata['character']
        assert u(gp[4]+4)==metadata['unit'] and u(gp[4]+8)==0
    elif registration:
        assert gp[6]==metadata['player']
        players=u(0x4f54f4);assert u(players+0x24)==0x21c and u(players+0x3c)==1
        player=u(players+0x44)+(metadata['player']&65535)*0x21c
        assert u(player,2)==metadata['player']>>16 and u(player+0x2c)==metadata['player_unit']
        assert u(u(0x5020a4))==u(u(0x5020a4)+0x1c)==0xffffffff
    else:assert u(gp[4]+4)==metadata['character']
    cache=Cache(path)
    try:
        squad=None
        if birth and metadata['declared_squad']!=0xffffffff:
            index=metadata['declared_squad'];scenario=u(0x4ecc20)
            authored=next(t for t in cache.tags.values() if t['group']=='scnr')
            assert scenario==authored['address'] and index<u(scenario+0x160)
            definition=u(scenario+0x164)+index*0x74
            assert read(definition,0x74)==bytes(cache.data[definition-cache.magic:definition-cache.magic+0x74])
            squads=u(0x52be7c);assert u(squads+0x29,1)==1 and u(squads+0x24)==0x98
            assert u(squads+0x3c)==u(scenario+0x160)
            record=u(squads+0x44)+index*0x98
            assert u(record,2)!=0 and u(record+0x68)==0xffffffff
            squad=dict(index=index,name=read(definition,32).split(b'\0')[0].decode(),
                full_datum=(u(record,2)<<16)|index,definition=definition,record=record)
            assert squad['name']=='e2_cov_hunters0'
        character=metadata['character'];tag=cache.tags[character];assert tag['group']=='char'
        root=u(u(0x4f0414)+(character&65535)*16+8);assert root==tag['address']
        assert read(root,0x40)==bytes(cache.data[root-cache.magic:root-cache.magic+0x40])
        inherited=[];current=character
        while True:
            assert current not in inherited and len(inherited)<64;inherited.append(current)
            t=cache.tags[current];assert t['group']=='char';at=u(u(0x4f0414)+(current&65535)*16+8);assert at==t['address']
            if u(at+0x34):
                general=u(at+0x38);assert read(general,12)==bytes(cache.data[general-cache.magic:general-cache.magic+12]);break
            current=u(at+8);assert current!=0xffffffff
        return dict(map=str(path),map_sha256=expected,character=tag['name'],character_datum=character,inheritance=inherited,general_flags=u(general),actor_type=u(general+4,2),actor_pool=pool,actual_ai_storage_initialized=True,actual_ai_new_map_initialized=True,actor_bound_before=False,entry=entry,authored_squad=squad,native_completed=build['returncode']==0 or following_ai,whole_case_completed=build['returncode']==0,following_ai_call_captured=following_ai,complete_campaign_boot_verified=False,validation_tool_sha256=file_sha(Path(__file__)))
    finally:cache.close()
