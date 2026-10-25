#!/usr/bin/env python3
"""Whole Xbox callback references for semantic CTF/Assault owner conversion.

Use the existing authenticated source CPU and declared carrier/roster banks.
All service bodies execute normally; a missing owner fails generation.
"""
from copy import deepcopy
import struct
import oracle_multiplayer_engines_tu15 as original

BASE_CASES=original.cases


def cases(image,engines,immutable,variants):
    base=BASE_CASES(image,engines,immutable,variants);result=[]
    # Retain one actual partial-arm/decay sequence, so the existing shared
    # engine regression also verifies execution of both scalar kernels.
    result.append(next(deepcopy(row) for row in base if row['source_type']==9 and
        row['entry']==0x2496b0 and row['move_at_tick'] is not None))
    for variant in variants:
        kind=variant['source_type']
        if kind not in (1,9):continue
        template=next(row for row in base if row['variant_name']==variant['name'] and
            row['entry']==0x2496b0 and '-carrier-' in row['name'] and row['name'].endswith('-1-1'))
        methods=next(methods for source,_,_,methods in engines if source==kind)
        def add(slot,label,args,setup=None,*,entry=None,registers=None):
            row=deepcopy(template);row.update(name=f'{kind}-native-{variant["name"]}-{label}',slot=slot,
                entry=methods[slot] if entry is None else entry,ticks=1,objective_index=None,move_at_tick=None,position_hex=None,
                observe_scores=False,observe_objects=False,observe_banks=(),initialize=False)
            for n,value in (registers or {}).items():row['gp'][n]=value
            for n in range(16):row['banks'].w(original.STACK+4+n*4,args[n] if n<len(args) else 0)
            if setup:setup(row['banks'])
            result.append(row)
        for speed in range(4):
            add(19,f'speed-{speed}',(0x1010000,),lambda b,speed=speed:b.w(original.G+0x234,speed))
        for suppressed in (0,1):
            add(30,f'carrier-statistic-{suppressed}',(0x1010000,0x1010000,suppressed,0))
            add(31,f'carrier-event-{suppressed}',(0x1010000,0x1010000,suppressed))
        add(37,'carrier-indicator',(0x1010000,original.ROWS+0x900))
        for relation in (-1,0,1):add(34,f'relationship-{relation}',(relation,))
        # Spawn input bit combinations are caller-owned authored placement
        # flags. The original registry walk resolves the actual declared row.
        for flags in range(16):
            for home in (False,True):
                def setup(b,flags=flags,home=home):
                    b.w(original.ROWS+0x90c,flags);b.w(original.E+0xc18,1)
                    b.w(original.E+0xc1c,0x1030002);b.w(original.E+0xc20,0,2)
                    b.w(original.K+0x1f4,0);b.w(original.K+0x170,int(home),1)
                add(33,f'spawn-filter-{flags:x}-{int(home)}',(0x1010000,original.ROWS+0x900),setup)
        # The real notification producer is covered in source-world startup;
        # this exact client guard additionally qualifies its complete ABI.
        add(6,'spawn-client-guard',(0x1010000,),lambda b:b.w(original.G+0xc,4,1))
        def hud_without_active_unit(b):
            b.w(original.LU,1);b.w(original.LU+0xc,0x1010000)
            b.w(original.PR+0x2c,-1);b.w(original.G+0x184,0,1)
            # Explicit custom option inputs: no team-marker pass and no
            # neutral bank. The actual inventory/relationship guards execute.
            b.w(original.G+0x240,0)
        add(14,'hud-no-active-unit',(0,),hud_without_active_unit)
        def explicit_object_state(b):
            original.placed_bomb_banks(b,variant,seconds=5)
        def registered_object_state(b):
            explicit_object_state(b)
            # Keep the declared side0 item eligible in one-sided presets.
            # The unchanged registry owner still performs its own checks.
            b.w(original.K+0x1f4,1 if kind==9 else 0)
        add(40,'validate-existing-objectives',(),registered_object_state)
        # Original24D6C5..24D6D8 selects opposing CTF players and owning
        # Assault players; neutral checks either side. Exercise both real
        # occupied player rows at the strict two-unit radius boundary.
        for player_team in (-1,0,1):
            for distance in (0.0,1.9999,2.0,2.0001,100.0):
                def nearby_players(b,player_team=player_team,distance=distance):
                    explicit_object_state(b)
                    original.occupied_players(b,teams=True,alive=True)
                    b.raw(original.OB+0xa00+0x64,struct.pack('<3f',0.0,0.0,0.0))
                    for slot in range(2):
                        b.w(original.PR+slot*0x21c+0xc0,player_team,1)
                        b.raw(original.OB+slot*0x500+0x64,struct.pack('<3f',distance,0.0,0.0))
                add(-1,f'nearby-team-{player_team}-distance-{distance}',(original.ROWS+0xc00,),
                    nearby_players,entry=0x24d5f0,registers={0:0x1030002})
        add(-1,'region-constructor',(0x1030002,),explicit_object_state,entry=0x24d530,
            registers={6:original.ROWS+0xc00,7:original.ROWS+0x900})
        for touch_return in (False,True):
            def setup(b,touch_return=touch_return):
                explicit_object_state(b)
                b.w(original.OB+0xa00+0x17e,1,2)
                b.w(original.PR+0xc0,0 if kind==9 else 1,1)
                flags=struct.unpack('<I',variant['raw'][0xf0:0xf4])[0]
                b.w(original.G+0x22c,(flags|4) if touch_return else (flags&~4),1)
                b.w(original.K+0x171,1,1);b.w(original.T+8,200)
                # NONE home/effect/damage tags are explicit input contracts.
                # Real announcement/statistics/timer owners still execute;
                # this case does not certify a physical home placement.
            add(17,f'touch-return-{int(touch_return)}',(0x1010000,0x1030002),setup)
    return result


if __name__=='__main__':
    original.CONFIG_ENTRIES=(*original.CONFIG_ENTRIES,0x249600,0x24a6f0)
    original.cases=cases
    original.main()
