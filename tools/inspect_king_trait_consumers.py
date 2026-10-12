#!/usr/bin/env python3
"""Literal King membership/team-bit contracts and compiled trait float words.

Actual registered King pointer and whole team-mode query are original source.
Options/engine presence are declared inputs, no patched instructions or returns.
"""
import json,struct
from unicorn.x86_const import UC_X86_REG_EAX
from oracle_king_trait_consumers import TraitTrace
from oracle_mp_full_scoreboard_1_5 import HudCache,ROOT,literal,sha
OUT=ROOT/'analysis/multiplayer/game-variants/king-traits-20261011'
cache=HudCache(ROOT/'assets/disc/maps/lockout.map');out=[]
for name,team in [('retail',0x15eaf0),('tu11',0x160150),('u5',0x161530)]:
 t=TraitTrace(name,cache);t.setup({'global_flags':1})
 engine=t.u(t.spec['table']+4*4);vtable=t.u(engine);predicate=t.u(vtable+0x6c)
 _,damage=literal(t.code,t.owners['damage'],t.starts)
 wrapper=int(next(i.op_str for i in reversed(damage)if i.mnemonic=='call'),16)
 row=dict(image=name,image_sha256=t.spec['pin'],selected_engine_type=4,engine_pointer=engine,vtable=vtable,
     membership_callback=predicate,membership_dispatcher=wrapper,team_mode_query=team,owners=[],team_query_calls=[])
 for key,entry in [('membership',predicate),('dispatcher',wrapper),('team_mode_query',team)]:
  _,ins=literal(t.code,entry,t.starts)
  row['owners'].append(dict(owner=key,entry=entry,instructions=[dict(address=i.address,bytes=i.bytes.hex(),mnemonic=i.mnemonic,operands=i.op_str)for i in ins]))
 for present in (False,True):
  for flags in (0,1,128,129):
   t.setup(dict(global_flags=flags,engine_present=present));t.call(team)
   row['team_query_calls'].append(dict(engine_present=present,global_flags=flags,al=t.g.reg_read(UC_X86_REG_EAX)&255))
 out.append(row)
 if name=='tu11':
  constants={f'{p:08x}':dict(bytes_hex=t.image.read(p,4).hex(),binary32=struct.unpack('<f',t.image.read(p,4))[0])for p in (0x465030,0x46509c,0x46502c,0x465090,0x446548,0x4650d8,0x4650d4)}
assert out[0]['team_query_calls']==out[1]['team_query_calls']==out[2]['team_query_calls']
(OUT/'team-friendly-source-inspection.json').write_text(json.dumps(dict(generator_sha256=sha(__file__),versions=out,instruction_patches=[],supplied_returns=[]),sort_keys=True,indent=2)+'\n')
(OUT/'compiled-constants.json').write_text(json.dumps(constants,sort_keys=True,indent=2)+'\n')
print(json.dumps(dict(whole_team_query_calls=24,images_agree=True),indent=2))
