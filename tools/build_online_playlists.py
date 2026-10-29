#!/usr/bin/env python3
"""Build reproducible MCC hopper catalogs using original native option setters.

Run through tools/run_test.py. The catalog and frequency manifest are
small required source data; compiler products stay in HALO2_TEST_SCRATCH.
"""
import argparse
from collections import defaultdict
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess

ROOT=Path(__file__).resolve().parents[1]
DATA=ROOT/'multiplayer/master_server/data'
EXAMPLES=ROOT/'multiplayer/master_server/examples'
SOURCE=DATA/'recovered-hopper-rows-v1.json'
MEMBERSHIP=DATA/'published-hopper-membership-v1.json'
MAPS={10:'cyclotron',20:'midship',30:'zanzibar',40:'waterworks',50:'lockout',
      60:'burial_mounds',70:'colossus',80:'ascension',100:'beavercreek',110:'coagulation',800:'headlong'}
TYPES={2:0,3:1,7:2,4:3,1:4,9:5,8:6}
LARGE=('coagulation','headlong','waterworks','burial_mounds','zanzibar','containment','dune','triplicate')
LAB_MAPS=('lockout','midship','beavercreek','foundation','warlock','deltatap',
          'derelict','highplains','zanzibar','coagulation','triplicate','waterworks')
MAP_NAMES={'Ivory Tower':'cyclotron','Sanctuary':'deltatap','Relic':'dune','Terminal':'triplicate',
           'Beaver Creek':'beavercreek','Burial Mounds':'burial_mounds','Tombstone':'highplains',
           'Sacntuary':'deltatap'}  # Preserve the published typo in evidence; explicit runtime alias only.
DESCRIPTIONS={
 'rumble-pit':'Free-for-all Slayer, Oddball and King on classic maps. Parties and guests welcome.',
 'double-team':'Two-player teams compete in ranked Slayer and objective games.',
 'team-slayer':'Ranked team Slayer with classic weapon variants.',
 'team-skirmish':'Capture flags, carry the ball and fight for territory in ranked team objectives.',
 'team-training':'Unranked team games. Bring your party and guests.',
 'team-snipers':'Ranked team battles with sniper rifle starts.',
 'team-hardcore':'Competitive ranked team games with rifle starts and no motion sensor.',
 'team-swat':'Battle rifles and magnums. No shields or motion sensor.',
 'big-team-battle':'Large unranked battles with Slayer, flags, bombs and territories.',
 'minor-clan-match':'Two clans compete in smaller ranked team games.',
 'major-clan-match':'Large ranked battles between two clans.',
 'multi-team':'Four teams compete in Slayer, Oddball, King and Territories.',
 'action-sack':'Fiesta and Infection. Random weapons, or survive the green zombies.',
 'lab-test':'Testing: 2 players to start. Slayer, Snipers, SWAT, flags, bombs, ball, hill and territories across 12 maps. No ranked progression.',
}


def sha(data):return hashlib.sha256(data).hexdigest()


def extension(base,maps,weight=4,*,name=None,writes=()):
    return [dict(base=base,map=map,weight=weight,name=name or base,writes=list(writes),
                 origin='project-tuned-extension') for map in maps]


def header(kind,teams,minimum,maximum,imbalance=0,flag=False,flag_minimum=0,flag_maximum=0,*,rank_map):
    raw=bytearray(0x614);struct.pack_into('<i',raw,0,kind)
    if len(rank_map)!=128:raise ValueError('Complete source rank-map bytes required')
    raw[0x51c:0x59c]=rank_map
    struct.pack_into('<4i',raw,0x5ec,teams,minimum,maximum,imbalance)
    raw[0x604]=flag;struct.pack_into('<2i',raw,0x608,flag_minimum,flag_maximum)
    return raw.hex()


def published_pools(source,swat_writes):
    published=json.loads(MEMBERSHIP.read_bytes())['sources'][-1]['playlists']
    by_name={p['name']:p for p in published}
    aliases={'Rumble Slayer':'Slayer','Rumble Rockets':'Rockets','Classic CTF':'CTF Classic',
             'Teamball':'Team Ball','Three Plots':'3 Plots'}
    recovered={}
    for name,index in (('Rumble Pit',0),('Team Skirmish',1),('Big Team Battle',3)):
        rows=defaultdict(list)
        for row in source['headers'][index]['entries']:
            rows[(MAPS[row['map_id']],aliases.get(row['name'],row['name']))].append(row)
        recovered[name]=rows
    presets={row['index']:row for row in source['retail_executable_presets']}
    table=next(t for t in source['literal_tables'] if t['retail_address'].lower()=='0x445218')
    recovered['Team Slayer']=defaultdict(list)
    for ordinal,row in enumerate(table['rows']):
        preset=presets[row['preset']];raw=bytearray.fromhex(preset['raw'])
        struct.pack_into('<I',raw,0x48,struct.unpack_from('<I',raw,0x48)[0]&~0x30)
        struct.pack_into('<I',raw,0x58,0)
        recovered['Team Slayer'][(MAPS[row['map_id']],preset['name'])].append(
            dict(raw=raw.hex(),ordinal=ordinal,weight=row['weight']))
    # Explicit authored equivalents. Whole downloaded options remain absent.
    # Weapon/CTF/bomb/ball fields use the source sily/descriptor receipts.
    recipes={
      'Duals':('Slayer',[(83,9)]), 'Rifles':('Slayer',[(83,11)]),
      'Slayer BR':('Slayer',[(89,3)]),'Slayer Covenant':('Slayer',[(83,17)]),
      '2v2 Duals Only':('Team Slayer',[(83,9)]),'2v2 Slayer':('Team Slayer',[]),
      'CTF Human':('CTF Classic',[(83,16)]),'2v2 Rifles':('Team Slayer',[(83,11),(89,3)]),
      '2v2 Pistols':('Team Slayer',[(83,14),(89,4),(94,1)]),'2v2 Rockets':('Rockets',[(91,1),(2,50)]),
      'Team Duals':('Team Slayer',[(83,9)]),'Team Rifles':('Team Slayer',[(83,11),(89,3)]),
      'Team Slayer BR':('Team Slayer',[(89,3)]),'Team Slayer Human BR':('Team Slayer',[(89,3),(83,16)]),
      'Team Snipers':('Snipers',[(91,1),(2,50)]),'Team Rockets':('Rockets',[(91,1),(2,50)]),
      'Assault':('Single Bomb',[]),'Assault BR':('Single Bomb',[(89,3)]),
      'Assault Covenant':('Single Bomb',[(83,17)]),'1 Flag CTF BR':('1 Flag CTF',[(89,3)]),
      'Multi Flag CTF Human':('Multi Flag CTF',[(83,16)]),'Neutral Bomb BR':('Neutral Bomb',[(89,3)]),
      'Neutral Bomb Fast':('Single Bomb Fast',[(59,2)]),
      'Rapid Assault':('Rapid Assault',[]),'Neutral Flag':('Multi Flag CTF',[(50,2)]),
      '1 Flag CTF Covenant':('1 Flag CTF',[(83,17)]),'Shotgun CTF':('CTF Classic',[(83,3),(89,9)]),
      'Team SWAT':('Team Slayer',list(swat_writes)),
      'Team Plasma Wars':('Team Slayer',[(83,15),(89,7),(94,6)]),
      'Team Brutes':('Team Slayer',[(83,5),(89,11),(94,18)]),
      'Team Slayer Pro':('Team Slayer',[(20,0),(89,3)]),
      'Team Shotguns':('Team Slayer',[(83,3),(89,9)]),'Team Swords':('Team Slayer',[(83,4),(89,17)]),
      'Pro CTF Classic 5':('CTF Classic',[(20,0),(89,3),(2,5)]),
      'Pro CTF Classic 3':('CTF Classic',[(20,0),(89,3),(2,3)]),
      'Pro Team Slayer':('Team Slayer',[(20,0),(89,3)]),
      'Pro Team Slayer Cov':('Team Slayer',[(20,0),(89,13),(83,17)]),
      'Pro Team Ball':('Team Ball',[(20,0),(89,3)]),'Pro Neutral Bomb':('Neutral Bomb',[(20,0),(89,3)]),
      'Pro Team Rifles':('Team Slayer',[(20,0),(89,3),(83,11)]),
      'Pro Team Snipers':('Snipers',[(20,0),(91,1),(2,50)]),
      'Pro Team Crazy King':('Team Crazy King',[(20,0),(89,3)]),
      'Pro Multi-Flag CTF':('Multi Flag CTF',[(20,0),(89,3)]),
      'Multi Bomb BR':('Multi Bomb',[(89,3)]),
      'Multi Flag CTF Cov Carbine':('Multi Flag CTF',[(83,17),(89,13)]),
      'Multi Flag CTF Covenant':('Multi Flag CTF',[(83,17)]),
      'Team Multiballs':('Team Ball',[(43,3)]),
      '3 Plots BR':('3 Plots',[(89,3)]),'Land Grab BR':('Land Grab',[(89,3)]),
      'Team Crazy King BR':('Team Crazy King',[(89,3)]),
    }
    builtins={p['name'] for p in source['builtins']}|{p['name'] for p in source['retail_executable_presets']}
    skipped=[];result={}
    slayer_names={'Slayer','2v2 Slayer','Team Slayer','Team Slayer BR','Pro Team Slayer','Team Slayer Pro',
                  'Pro Team Slayer Cov','Team Duals','2v2 Duals Only','Duals','Slayer BR','Slayer Covenant',
                  'Rifles','Team Rifles','2v2 Rifles','Pro Team Rifles','Team Slayer Human BR'}
    for name in ('Rumble Pit','Double Team','Team Slayer','Team Skirmish','Team Training','Team Snipers','Team Hardcore','Big Team Battle','Multi-Team'):
        inputs=list(by_name[name]['rows'])
        if name=='Team Training':inputs=[*[dict(r,inherited_pool='Team Skirmish') for r in by_name['Team Skirmish']['rows']],
                                         *[dict(r,inherited_pool='Team Slayer') for r in by_name['Team Slayer']['rows']],*inputs]
        rows=[]
        for row in inputs:
            variant=row['variant']
            # A name with unresolved rules is not silently ordinary Slayer.
            if variant in ('Fiesta Team Slayer','Team Showers','Team Gulch Wars'):
                skipped.append(dict(playlist=name,variant=variant,maps=row['maps'],reason='Complete rules/native team extension not admitted'))
                continue
            base,writes=recipes.get(variant,(variant,[]))
            if base not in builtins:raise ValueError('Unresolved published mode: '+variant)
            if name=='Multi-Team' and base in ('CTF Classic','Multi Flag CTF','Neutral Bomb'):
                skipped.append(dict(playlist=name,variant=variant,maps=row['maps'],reason='Two-team objective row requires separate team-count admission'))
                continue
            for title in row['maps']:
                map=MAP_NAMES.get(title,title.lower().replace(' ','_'))
                source_pool=row.get('inherited_pool',name)
                exact=recovered.get(source_pool,{}).get((map,variant),[])
                if exact:
                    for original in exact:
                        rows.append(dict(map=map,name=variant,settings=original['raw'],weight=original['weight'],
                                         origin=f'recovered-retail-{source_pool}-row-{original["ordinal"]}-published-pair'))
                    continue
                # Declared project tuning, not inferred from published order.
                weight=8 if variant in slayer_names else 2
                if name in ('Team Snipers','Team Hardcore'):weight=8 if variant in slayer_names else 4
                if name=='Big Team Battle' and variant=='Team Slayer BR':weight=12
                rows+=extension(base,(map,),weight,name=variant,writes=writes)
                rows[-1]['origin']='published-2005-pair/project-tuned-weight-and-options'
        result[name]=rows
    result['Team SWAT']=extension('Team Slayer',('elongation','cyclotron','lockout','turf','highplains','derelict'),4,
                                   name='Team SWAT',writes=swat_writes)
    return result,skipped


def plans(source):
    original=source['headers']
    def pool(index):
        return [dict(map=MAPS[row['map_id']],name=row['name'],settings=row['raw'],weight=row['weight'],
                     origin=f'original-retail-hopper-{index}-row-{row["ordinal"]}') for row in original[index]['entries']]
    def plan(id,name,team_size,minimum,maximum,party,ranked,rows,clock,**extra):
        return dict(id=id,name=name,team_size=team_size,minimum_players=minimum,maximum_players=maximum,
                    maximum_party=party,ranked=ranked,rows=rows,clock=clock,**extra)
    # Authored from source options: BR + magnum, no shields/radar/grenades,
    # map weapons/powerups/vehicles. Not an authenticated SWAT download.
    swat_writes=((91,1),(2,50),(19,1),(20,0),(89,3),(94,4),(88,0),(86,0),(83,1),
                 (84,0),(85,0),(76,7),(77,7),(78,4),(80,4),(79,4),(81,6),(82,6))
    minor=pool(4)+extension('Team Slayer',('deltatap','warlock','highplains'),4)+extension('Multi Flag CTF',('highplains',),2)
    major=pool(5)+extension('Team Slayer',LARGE,5,writes=((2,100),))+extension('Team Slayer',('highplains',),4)
    # Native Fiesta/Infection are authored Slayer extensions. Equal per-mode
    # totals prevent map-count changes silently making one mode more frequent.
    sack=[dict(map=m,name=n,weight=1,origin='authored-slayer-extension',custom=True)
          for n in ('Fiesta Slayer','Infection Slayer')
          for m in ('lockout','foundation','warlock','elongation','derelict','highplains')]
    dated,skipped=published_pools(source,swat_writes)
    # New MCC map additions are separate from the dated membership evidence.
    dated['Rumble Pit']+=extension('Slayer',('highplains','derelict'),4)
    dated['Team Slayer']+=extension('Team Slayer',('highplains','derelict'),4)
    dated['Double Team']+=extension('Team Slayer',('highplains',),4,name='2v2 Slayer')
    dated['Team Skirmish']+=extension('Multi Flag CTF',('highplains',),3)+extension('Single Bomb',('highplains',),2)
    dated['Team Training']+=extension('Team Slayer',('highplains',),4)+extension('Multi Flag CTF',('highplains',),2)+extension('Single Bomb',('highplains',),2)
    dated['Team Snipers']+=extension('Snipers',('highplains','derelict'),4,name='Team Snipers',writes=((91,1),(2,50)))
    dated['Big Team Battle']+=extension('Team Slayer',('highplains',),8,writes=((89,3),(2,100),))+extension('Multi Flag CTF',('highplains',),2)
    dated['Multi-Team']+=extension('Team Slayer',('highplains',),4)
    rows=[
        plan('rumble-pit','Rumble Pit',0,6,8,8,False,dated['Rumble Pit'],0,quickmatch=True),
        plan('double-team','Double Team',2,4,4,2,True,dated['Double Team'],1),
        plan('team-slayer','Team Slayer',4,6,8,4,True,dated['Team Slayer'],1),
        plan('team-skirmish','Team Skirmish',4,6,8,4,True,dated['Team Skirmish'],1),
        plan('team-training','Team Training',4,4,8,8,False,dated['Team Training'],6),
        plan('team-snipers','Team Snipers',4,6,8,4,True,dated['Team Snipers'],1),
        plan('team-hardcore','Team Hardcore',4,6,8,4,True,dated['Team Hardcore'],1),
        plan('team-swat','Team SWAT',4,6,8,8,False,dated['Team SWAT'],1),
        plan('big-team-battle','Big Team Battle',8,10,16,16,False,dated['Big Team Battle'],3),
        plan('minor-clan-match','Minor Clan Match',4,6,8,4,True,minor,4,clan_match='minor-retail-launch-2004'),
        plan('major-clan-match','Major Clan Match',8,12,16,8,True,major,5,clan_match='major-retail-launch-2004'),
        plan('multi-team','Multi-Team',3,12,12,12,False,dated['Multi-Team'],1,team_count=4),
        plan('action-sack','Action Sack',0,6,8,8,False,sack,0)]
    # Explicit user-requested testing exception. Keep the historical pools,
    # population rules, Quickmatch default and their progression unchanged.
    lab=[]
    for base,name,writes in (
            ('Team Slayer','Team Slayer',()),
            ('Snipers','Team Snipers',((91,1),(2,50))),
            ('Team Slayer','Team SWAT',swat_writes),
            ('Multi Flag CTF','Multi Flag CTF',()),
            ('Single Bomb','Single Bomb',()),
            ('Team Ball','Team Ball',()),
            ('Team King','Team King',()),
            ('3 Plots','3 Plots',())):
        lab+=extension(base,LAB_MAPS,1,name=name,writes=writes)
    for row in lab:row['origin']='authored-lab-test'
    rows.append(plan('lab-test','Lab Test',8,2,16,16,False,lab,6,
        revision='lab-test-v1',rank_configuration=None,
        gathering=dict(countdown_ms=[1000]*16,timeout_ms=120000,
            provenance='Authored Lab Test: two-player minimum, one-second population countdown and two-minute idle timeout.'),
        provenance='User-requested test rotation. Equal row weights across eight team variants and twelve MCC maps; two-player minimum. No historical rotation claim or ranked/hidden XP progression. Native connection/readiness requirements still apply.'))
    return rows,skipped


def build(scratch):
    source=json.loads(SOURCE.read_bytes())
    policy,skipped=plans(source)
    builtins={row['name']:row for row in source['builtins']}
    for row in source['retail_executable_presets']:
        builtins.setdefault(row['name'],dict(raw=row['raw'],frontend_engine=TYPES[struct.unpack_from('<i',bytes.fromhex(row['raw']),0x44)[0]]))
    requests=[]
    for plan in policy:
        for row in plan['rows']:
            if row.get('custom'):continue
            if 'settings' in row:
                raw=row['settings'];engine=TYPES[struct.unpack_from('<i',bytes.fromhex(raw),0x44)[0]]
                request=dict(name=row['name'],engine=engine,settings=raw,preserve_title=True)
            else:
                base=builtins[row['base']]
                request=dict(name=row['name'],engine=base['frontend_engine'],settings=base['raw'],writes=row['writes'])
            row['request']=len(requests);requests.append(request)
    binary=scratch/'hopper-options'
    command=['c++','-std=c++20','-O1','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),
             str(ROOT/'tools/build_online_hopper_variants.cpp'),str(ROOT/'src/gameplay/multiplayer/source_game_variant.cpp'),'-o',str(binary)]
    subprocess.run(command,check=True)
    result=subprocess.run([str(binary)],input=json.dumps(requests),capture_output=True,text=True,check=True)
    options=json.loads(result.stdout)
    clocks=json.loads((ROOT/'multiplayer/master_server/retail_gathering.json').read_text())['hoppers']
    catalog={'version':1,'playlists':[]};manifest={'schema':'H2_HOPPER_ROTATIONS_V1',
        'profile':'final-era-reconstruction-v1','source_sha256':sha(SOURCE.read_bytes()),
        'membership_sha256':sha(MEMBERSHIP.read_bytes()),'historical_2010_download_recovered':False,
        'frequency_domain':dict(rng_high16_values=65536,
            map_availability='All catalog maps available to every group member',
            runtime_unavailable_maps='Zero weight; remaining ordered intervals use their new total'),
        'unadmitted_published_rows':skipped,'playlists':[]}
    map_path=ROOT/'multiplayer/docs/evidence/playlist-hoppers-20261028/mcc-map-identities.json'
    map_metadata={m['key']:m for m in json.loads(map_path.read_bytes())['maps']}
    manifest['mcc_map_identity_sha256']=sha(map_path.read_bytes())
    for plan in policy:
        team=plan['team_size'];count=plan.get('team_count',2);clan=plan.get('clan_match')
        kind=5 if clan else (4 if plan['ranked'] else 3) if team else (2 if plan['ranked'] else 1)
        minimum=plan['minimum_players']//count if team else plan['minimum_players']
        maximum=team if team else plan['maximum_players']
        ranked_team=kind==4
        # The reconstructed profile explicitly retains the executable-default
        # rank table. A dated map/mode list does not recover downloaded ranks.
        rank_source=source['headers'][plan['clock']]
        rank_map=bytes.fromhex(rank_source['raw'])[0x51c:0x59c]
        raw_header=header(kind,count if team else minimum,minimum if team else maximum,
                          maximum if team else 0,2 if clan and team==8 else 0 if ranked_team or clan else 1,
                          ranked_team and team==4,3 if ranked_team and team==4 else 0,4 if ranked_team and team==4 else 0,
                          rank_map=rank_map)
        clock=clocks[plan['clock']]
        games=[];details=[]
        for index,row in enumerate(plan['rows']):
            game=dict(id=f'{plan["id"]}-{index:03}',map=row['map'],variant=row['name'],weight=row['weight'])
            if not row.get('custom'):
                emitted=options[row['request']]
                if emitted['team'] != bool(team):raise ValueError('Variant team mismatch: '+row['name'])
                game['settings']=emitted['settings']
            engine=struct.unpack_from('<i',bytes.fromhex(game['settings']),0x44)[0] if 'settings' in game else 2
            metadata=map_metadata[row['map']]
            capacity=metadata.get('scenario_definition',metadata)['team_counts'][engine]
            if team and capacity<count:raise ValueError('Map cannot host selected engine/team count: '+str((row['map'],engine,count)))
            games.append(game);details.append(dict(id=game['id'],map=row['map'],variant=row['name'],weight=row['weight'],origin=row['origin'],
                 source_engine=engine,settings_sha256=sha(bytes.fromhex(game['settings'])) if 'settings' in game else None))
        entry={k:plan[k] for k in ('id','name','ranked','team_size','minimum_players','maximum_players','maximum_party')}
        entry.update(revision=plan.get('revision','h2f-final-era-v1'),description=DESCRIPTIONS[plan['id']],
            quickmatch=plan.get('quickmatch',False),games=games,stages=[dict(after_seconds=0,rank_numerator=1,rank_denominator=1,minimum_connection=50)],
            skill=dict(kind='custom',radius=0),rank_configuration=plan.get('rank_configuration','title-update-1.5-defaults'),required_compatibility='halo2-native-queue-v16',
            candidate_selection='retail-candidates-native-relay-v1',hopper=raw_header,
            gathering=plan.get('gathering',dict(countdown_ms=clock['countdown_ms'],timeout_ms=clock['timeout_ms'],
                provenance='Recovered retail clock table '+str(plan['clock'])+'; explicit reconstructed playlist population bounds.')),
            provenance=plan.get('provenance','Final-era reconstruction. Exact recovered row weights/options are retained where identified in the frequency manifest; additions and unrecovered SWAT/Hardcore/Doubles/Snipers rotations are project-tuned. Rank maps explicitly retain retail executable defaults. Later downloaded rank maps remain unknown. Not a recovered final-2010 download.'))
        if count!=2:entry['team_count']=count
        if clan:entry['clan_match']=clan
        catalog['playlists'].append(entry)
        total=sum(max(1,min(1000,g['weight'])) for g in games)
        cumulative=0;by_map=defaultdict(int);by_mode=defaultdict(int);by_engine=defaultdict(int)
        for row in details:
            before=(cumulative*65536+total-1)//total;cumulative+=max(1,min(1000,row['weight']))
            row['draws_of_65536']=(cumulative*65536+total-1)//total-before
            by_map[row['map']]+=row['draws_of_65536'];by_mode[row['variant']]+=row['draws_of_65536']
            by_engine[str(row['source_engine'])]+=row['draws_of_65536']
        manifest['playlists'].append(dict(id=entry['id'],name=entry['name'],weight_total=total,rows=details,
            map_draws_of_65536=dict(by_map),variant_draws_of_65536=dict(by_mode),engine_draws_of_65536=dict(by_engine),
            rank_map_source=dict(profile='retail-launch-executable-default',header_index=rank_source['index'],
                source_header_sha256=rank_source['sha256'],source_xbe_sha256=source['retail_xbe_sha256'],
                offset='0x51c',bytes=128,sha256=sha(rank_map),downloaded_historical_values_recovered=False)))
    return catalog,manifest


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check',action='store_true')
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    catalog,manifest=build(scratch)
    outputs={EXAMPLES/'matchmaking-online.json':catalog,DATA/'online-hopper-frequencies-v1.json':manifest}
    for path,value in outputs.items():
        raw=(json.dumps(value,indent=2)+'\n').encode()
        if args.check:
            if not path.is_file() or path.read_bytes()!=raw:raise ValueError('Stale generated catalog: '+str(path))
        else:path.write_bytes(raw)
        print(path.relative_to(ROOT),len(raw),sha(raw))


if __name__=='__main__':main()
