#!/usr/bin/env python3
"""Generate guarded Cairo authoring facts; never read executable/reference files."""
import argparse
import hashlib
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def generated(p):
    if (p['schema'],p['scenario_path'],p['script_count_before'],p['script_count_after'])!=(6,r'scenarios\solo\01b_spacestation\01b_spacestation',420,418):
        raise ValueError('unverified Cairo authoring identity')
    if [(r['index'],r['name'],r['type'],r['return_type'],r['root'])for r in p['removed_scripts']] != [(136,'bring_back_brand',1,4,0xf92a15b7),(330,'breadcrumbs_nav_points_01b',1,4,0x9b6137ed)]:
        raise ValueError('unverified Cairo removed declarations')
    if len(p['banks'])!=180 or len(p['script_references'])!=461 or len(p['retired_nodes'])!=626 or len(set(p['retired_nodes']))!=626 or len(p['scenario_ordinals'])!=17 or len(p['patches'])!=20:
        raise ValueError('Cairo authoring proof extent differs')
    if p['mcc_dll_sha256']!='de65b4f4fdbf3f0a5eab7431fe530da17dd815599182dfd6ae9b7e21cf171946' or p['xbox_tu15_xbe_sha256']!='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd':
        raise ValueError('unverified Cairo executable authority')
    names={b['name'] for b in p['banks']}
    if len(names)!=180 or not {'scenario','block_568','block_432','block_440','block_448'}<=names:
        raise ValueError('missing or duplicate guarded Cairo bank')
    namespace_banks=[b for b in p['banks']if b.get('source')=='cache_namespace']
    if [(b['name'],b['token'],b['size'])for b in namespace_banks] != [('branding_placement',247356,92),('branding_palette',300600,40),('branding_object_header',5132320,64),('branding_model_header',5132636,96),('branding_render_header',5132980,96),('branding_nodes',5133432,96),('branding_marker_groups',5133528,24),('branding_markers_0',5133552,36),('branding_markers_1',5133588,36)]:
        raise ValueError('unverified Cairo branding namespace owners')
    if next(b['token'] for b in p['banks'] if b['name']=='scenario')!=p['scenario_address']:
        raise ValueError('Cairo primary scenario owner differs')
    if [(x['predecessor'],x['removed_call'],x['last_removed'],x['next'])for x in p['patches']] != [(3928491701, 3928688312, 3928688312, 3928884923), (2177113680, 2177310291, 2177310291, 2177506902), (4157084757, 4157215831, 4157215831, 4157346905), (4165735641, 4165932252, 4165932252, 4166063326), (2935114622, 2935245696, 2935245696, 2935442307), (4083159021, 4083355632, 4083552243, 4083748854), (4111012246, 4111405468, 4111602079, 4111798690), (4192867959, 4193261181, 4193523329, 4193719940), (2652453541, 2652715689, 2652715689, 2652977837), (2663529294, 2664053590, 2664053590, 2664315738), (2679979081, 2680437840, 2680437840, 2680699988), (2720743095, 2720939706, 2720939706, 4294967295), (2855421630, 2855683778, 2855683778, 2855945926), (2614376544,2614900840,2614900840,2615359599), (2678799415,2679127100,2679127100,2679454785), (2680699988,2680896599,2680896599,2681224284), (2706783714,2707045862,2707373547,2707701232), (2716221042,2716417653,2716417653,2716745338), (2717073023,2717269634,2717269634,2717597319), (2623682798,2624272631,2624469242,2624796927)]:
        raise ValueError('unverified Cairo control-flow delta')
    # Pin every authored typed field, including salts, debug labels and null
    # emitter bits. A same-length edit is not additional retail evidence.
    canonical=lambda value:hashlib.sha256(json.dumps(value,sort_keys=True,separators=(',',':')).encode()).hexdigest()
    sounds=p['sound_removals']
    if canonical(sounds)!='f6c6eb1307dbd2f09ad52ef81bf31fdb24173d6ae72c1d9df309c67fc239e7d2' or canonical(p['retired_nodes'])!='506e35cab371cae822459a94a1522f0265e9ea8e8f75a675ec9da862f67bdae8':
        raise ValueError('unverified Cairo remastered sound closure or retirement')
    sound_rows=[r for sound in sounds for r in sound['rows']]
    if len(sounds)!=8 or len(sound_rows)!=42 or len({r['datum']for r in sound_rows})!=42 or any(r['datum']not in p['retired_nodes']for r in sound_rows):
        raise ValueError('Cairo sound retirement ownership differs')
    if len({r['source_token']for r in p['scenario_ordinals']})!=17:
        raise ValueError('duplicate Cairo scenario ordinal write')
    if [(r['datum'],r['parent_datum'],r['argument'],r['before'],r['after'],r['function'])for r in p['literal_restores']] != [(2833597809,2833204587,4,1000,1200,'set_global_sound_environment'),(2874492897,2874099675,4,1000,1200,'set_global_sound_environment')]:
        raise ValueError('unverified Cairo environment word restoration')
    clock=p['clock_restore']
    if (clock['global_index'],clock['before_name'],clock['after_name'],clock['label_offset'],clock['root'],clock['function'],clock['literal_value'])!=(18,'tick_seconds','seconds',23,2420190413,2420255950,0xffff001e) or clock['references']!=[2422090986,2440310272,2441752086,2443193900,2444635714,2446077528,2447519342,2448961156,2450402970,2452238006,2453679820,2455449319,2457415429,2458791706,2461019964,2463641444,2465083258,2466525072,2467966886,2469408700,2470850514,2479632472,2479894620,2480746601,2481008749] or clock['function']not in p['retired_nodes'] or clock['root']in p['retired_nodes']:
        raise ValueError('unverified Cairo seconds global restoration')
    if [tuple(r[k]for k in ['datum','parent','other','parent_context','other_value'])for r in p['clock_literal_restores']] != [(2493919538,2493722927,2493854001,7,1084227584),(2494771519,2494574908,2494705982,6,1084227584),(2500211090,2500014479,2500145553,7,1077936128),(2500473238,2500276627,2500407701,7,1088421888),(2700033403,2699836792,2699967866,7,1097859072),(2740076510,2739879899,2740010973,7,1092616192),(2823701722,2823505111,2823636185,7,1128792064),(2855356093,2855159482,2855290556,8,1101004800),(2856732370,2856535759,2856666833,8,1123024896),(2866366309,2866169698,2866300772,8,1097859072)]:
        raise ValueError('unverified Cairo mutable seconds reference restoration')
    brand=p['effect_brand_restore']
    if tuple(brand[k]for k in ['parent','function','effect','object','marker','old_object_index','new_object_index','old_label_offset','new_label_offset','old_marker','new_marker','old_marker_offset','new_marker_offset'])!=(4174386525,4174452062,4174517599,4174583136,4174648673,25,24,13938,13979,83888633,0,13955,0) or brand['effect_path']!=r'effects\objects\cinematics\branding_iron\branding_iron_burn' or brand['object_path']!=r'objects\cinematics\covenant\branding_iron\branding_iron':
        raise ValueError('unverified Cairo original branding effect binding')
    final_global=p['removed_final_global']
    if (final_global['index'],final_global['name'],final_global['type'],final_global['kind'],final_global['root'])!=(51,'g_breadcrumb_nav_index',7,0,2431266166) or [(r['datum'],r['opcode'])for r in final_global['references']] != [(2573481456,0),(2575382029,0),(2576037399,0),(2576430621,0),(2578331194,0),(2578855490,65535),(2579248712,0),(2581149285,0),(2581673581,65535),(2582066803,0),(2583836302,0),(2584491672,0),(2584884894,0),(2586785467,0),(2587440837,0),(2587834059,0),(2590979835,0),(2591766279,65535),(2592159501,0),(2593142556,0),(2594912055,0),(2595436351,65535),(2595829573,0),(2597730146,0),(2598254442,65535),(2598647664,0),(2599630719,0),(2601072533,0),(2601727903,0),(2602121125,0),(2603562939,0),(2604087235,65535),(2604480457,0),(2652846763,65535),(2664184664,65535),(2680568914,65535),(2721070780,65535),(2855814852,65535)] or any(r['datum']not in p['retired_nodes']for r in final_global['references']) or final_global['root']in p['retired_nodes']:
        raise ValueError('unverified Cairo final unused global removal')
    restoration=p['final_holder_restoration']
    if canonical(restoration)!='6b920f65527fe43b21ed5aced43ebc22a74f2c98d68763e5f12c57155ef53ca4':
        raise ValueError('unverified Cairo original timing, flush allocation or string ownership')
    q=json.dumps;lines=['// Generated from tools/mcc-cairo-vanilla-authoring.json.',f'constexpr std::uint32_t cairo_scenario_address={p["scenario_address"]}u;','constexpr std::array<RemovedScript,2> removed_scripts{{{136,1,4,0xf92a15b7u,"bring_back_brand"},{330,1,4,0x9b6137edu,"breadcrumbs_nav_points_01b"}}};',f'constexpr std::array<GuardBank,{len(p["banks"])}> guard_banks{{{{']
    for b in p['banks']:
        if len(bytes.fromhex(b['sha256']))!=32 or not b['size'] or b['token']+b['size']>2**32:raise ValueError('invalid guarded source extent')
        digest=','.join('std::byte{0x'+b['sha256'][i:i+2]+'}'for i in range(0,64,2));lines.append(f'    {{{q(b["name"])},{b["token"]},{b["size"]},{{{{{digest}}}}}}},')
    lines+=['}};',f'constexpr std::array<ScriptReference,{len(p["script_references"])}> script_references{{{{']
    if len({r['datum']for r in p['script_references']})!=461:raise ValueError('duplicate script reference')
    for r in p['script_references']:
        kind={'static':0,'script':10,'command_script':21}[r['kind']]
        if not 0<=r['index']<420:raise ValueError('script ordinal escapes declaration bank')
        if r['index']in(136,330) and r['datum']not in(2935376770,4193654403):raise ValueError('breadcrumb has an unverified incoming script reference')
        lines.append(f'    {{{r["datum"]},{r["index"]},{kind},{r.get("function_datum",0)},{q(r["name"])}}},')
    lines+=['}};',f'constexpr std::array<AuthoringPatch,{len(p["patches"])}> authoring_patches{{{{']
    for r in p['patches']:lines.append(f'    {{{r["predecessor"]},{r["removed_call"]},{r["last_removed"]},{r["next"]}}},')
    lines+=['}};',f'constexpr std::array<std::uint32_t,{len(p["retired_nodes"])}> authoring_retired{{{{'+','.join(map(str,p['retired_nodes']))+'}};',f'constexpr std::array<ScenarioOrdinal,{len(p["scenario_ordinals"])}> scenario_ordinals{{{{']
    for r in p['scenario_ordinals']:
        if not 136<r['index']<420 or r['index']==330:raise ValueError('scenario ordinal is not in the shifted suffix')
        lines.append(f'    {{{r["source_token"]},{r["index"]},{q(r["script_name"])}}},')
    lines+=['}};','constexpr std::array<LiteralRestore,2> literal_restores{{']
    for r in p['literal_restores']:
        lines.append(f'    {{{r["datum"]},{r["parent_datum"]},{r["argument"]},{r["before"]},{r["after"]}}},')
    lines+=['}};',f'constexpr ClockInitializer clock_initializer{{18u,{clock["root"]}u,{clock["function"]}u,23u}};',f'constexpr std::array<std::uint32_t,25> clock_references{{{{'+','.join(map(str,clock['references']))+'}};','constexpr std::array<ClockLiteralRestore,10> clock_literal_restores{{']
    for r in p['clock_literal_restores']:lines.append('    {'+','.join(str(r[k])+'u'for k in ['datum','parent','other','parent_context','other_value'])+'},')
    lines+=['}};','constexpr BrandRestore brand_restore{'+','.join(str(brand[k])+'u'for k in ['parent','function','effect','object','marker'])+','+q(brand['effect_path'])+','+q(brand['object_path'])+'};']
    lines+=['constexpr RemovedGlobal removed_final_global{51u,7u,0u,2431266166u,"g_breadcrumb_nav_index"};','constexpr std::array<RemovedGlobalReference,38> removed_global_references{{']
    for r in final_global['references']:lines.append(f'    {{{r["datum"]}u,{r["opcode"]}u}},')
    lines+=['}};']
    lines+=['constexpr std::array<SoundRemoval,8> sound_removals{{']
    for r in sounds:
        keys=['root','function','sound','emitter','volume','ai_function','ai_literal','predecessor','opcode','xbox_opcode','sound_type','sound_handle']
        lines.append('    {'+','.join(str(r[k])+'u'for k in keys)+','+q(r['path'])+','+q(r['function_name'])+'},')
    lines+=['}};','constexpr std::array<SoundRow,42> sound_rows{{']
    for r in sound_rows:lines.append('    {'+','.join(str(r[k])+'u'for k in ['datum','salt','opcode','type','flags','next','label','value'])+'},')
    lines+=['}};']
    def typed(r):return '{'+','.join(str(r[k])+'u'for k in ['datum','salt','opcode','type','flags','next','label','value'])+'}'
    lines+=['constexpr std::array<TypedRestore,14> final_row_restores{{']
    for r in restoration['rewrites']:lines.append('    {'+('true'if r['allocated']else'false')+','+typed(r['before'])+','+typed(r['after'])+'},')
    lines+=['}};','constexpr std::array<IncomingOwner,17> timing_incoming{{']
    for r in restoration['incoming']:lines.append('    {'+str(r['datum'])+'u,'+str(r['from'])+'u,'+('true'if r['child']else'false')+'},')
    lines+=['}};','constexpr std::array<FinalLink,2> flush_links{{']
    for r in restoration['links']:lines.append('    {'+','.join(str(r[k])+'u'for k in ['datum','before','after'])+'},')
    lines+=['}};','constexpr std::array<FlushCall,4> flush_calls{{']
    for r in restoration['flush_calls']:lines.append('    {'+','.join(str(r[k])+'u'for k in ['root','function','opcode','xbox_opcode'])+','+q(r['name'])+'},')
    lines+=['}};','constexpr std::array<FunctionSlot,2> final_function_slots{{']
    for r in restoration['function_slots']:lines.append('    {'+','.join(str(r[k])+'u'for k in ['offset','size','before_owner'])+',{'+','.join(str(x)+'u'for x in r['after_owners'])+'},'+q(bytes.fromhex(r['before']).rstrip(b'\0').decode())+','+q(r['after_name'])+'},')
    lines+=['}};','constexpr std::array<SoundRow,8> preserved_string_debug_rows{{']
    for r in restoration['function_slots']:
        for d in r['preserved_debug_rows']:lines.append('    '+typed(d['row'])+',')
    lines+=['}};','constexpr std::array<RemovedScript,3> final_holders{{']
    for r in restoration['holders']:lines.append('    {'+','.join(str(r[k])+'u'for k in ['index','type','return_type','root'])+','+q(r['name'])+'},')
    lines+=['}};']
    for name,bank in [('nodes','block_568'),('strings','block_432')]:
        owner=next(b for b in p['banks']if b['name']==bank);h=restoration['postimages'][name];digest=','.join('std::byte{0x'+h[i:i+2]+'}'for i in range(0,64,2));lines.append('constexpr GuardBank final_'+name+'_postimage{'+q(bank)+','+str(owner['token'])+'u,'+str(owner['size'])+'u,{{'+digest+'}}};')
    return '\n'.join(lines)+'\n'
def main():
    a=argparse.ArgumentParser(description=__doc__);a.add_argument('--check',action='store_true');o=a.parse_args();result=generated(json.loads((ROOT/'tools/mcc-cairo-vanilla-authoring.json').read_text()));output=ROOT/'src/campaign/mcc_cairo_vanilla_authoring.inc'
    if o.check:
        if output.read_text()!=result:raise SystemExit('compiled Cairo authoring profile is stale')
    else:output.write_text(result)
if __name__=='__main__':main()
