"""Install the qualified frozen wreck source into an owned Classic scenario.

This authoring seam copies exactly four qualified wreck_static tags and adds
one typed SCEN placement. It never edits installed tags, geometry, or engines.
"""
from __future__ import annotations
import copy,hashlib,json,os,subprocess,sys,tempfile
from pathlib import Path

PINNED_INTERFACE='7b01076b485cb851277c14defb71c8f88e214de4'
SCENARIO='scenarios/multi/dust2_mombasa/dust2_mombasa'
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def _json(value):return json.loads(json.dumps(value))

def omit_wreck_from_mixed_props(source_ass:Path,source_report:Path,output:Path,report_path:Path)->dict:
    """Remove exactly the proven Warthog owner from the stable mixed-prop output.

    All remaining material IDs, records, faces and their order are untouched.
    The returned excluded source object IDs let the parent assembler omit those
    original visible faces while adding the real SCEN prefab separately.
    """
    from prepare_dust2_h2ek import source_meshes,IDENTITY
    source_ass,source_report,output,report_path=map(Path,(source_ass,source_report,output,report_path))
    proof=json.loads(source_report.read_text())
    if proof.get('schema')!='DESERTED_MIXED_RETAIL_PROP_OWNERS_V1' or proof.get('passed') is not True or sha(source_ass)!=proof['output_sha256']:raise ValueError('Unqualified mixed retail source')
    rows=proof['rows'];removed=[i for i,r in enumerate(rows) if r['collider_source_node']=='permanent_retail_warthog_hull']
    if len(removed)!=1:raise ValueError('Expected exactly one frozen retail Warthog owner')
    index=removed[0];row=rows[index]
    if row['render_triangles']!=1770 or row['collision_triangles']!=270 or len(row['render_objects'])!=5:raise ValueError('Unexpected Warthog owner geometry')
    iterator=source_meshes(source_ass);palette=next(iterator);bodies=list(iterator)
    if len(bodies)!=len(rows) or len(bodies[index][1])!=2040:raise ValueError('Mixed Warthog owner count mismatch')
    retained=[b for i,b in enumerate(bodies) if i!=index]
    if output.exists():raise ValueError('Refuse to replace a mixed source output')
    with output.open('wb') as f:
        f.write(('2\n"DESERTED_MIXED_PALMS_STATIC_WRECK_SEPARATE"\n"1"\n"Eric"\n"LOCAL"\n'+str(len(palette))+'\n').encode())
        for name,variant in palette:f.write((name+'\n'+variant+'\n').encode())
        f.write(f';### OBJECTS ###\n{len(retained)}\n'.encode())
        for records,faces in retained:
            f.write(f'"MESH"\n""\n""\n{len(records)}\n'.encode());f.writelines(records);f.write((str(len(faces))+'\n').encode())
            for face in faces:f.write(('\n'.join(map(str,face))+'\n').encode())
        f.write(f';### INSTANCES ###\n{len(retained)+1}\n-1\n"Scene Root"\n-1\n-1\n0\n{IDENTITY}'.encode())
        for i in range(len(retained)):f.write(f'{i}\n"%mixed_owner_{i:03}"\n{i+1}\n0\n0\n{IDENTITY}'.encode())
    iterator=source_meshes(output);assert next(iterator)==palette;back=list(iterator)
    if len(back)!=len(retained) or any(a[0]!=b[0] or not (a[1]==b[1]).all() for a,b in zip(retained,back)):raise ValueError('Retained palm records/indices changed')
    result=copy.deepcopy(proof);result.update(output_sha256=sha(output),output_bytes=output.stat().st_size,rows=[r for i,r in enumerate(rows) if i!=index],source_render_object_ids=[i for i in proof['source_render_object_ids'] if i not in row['render_objects']],static_wreck_omitted=True,excluded_scenery_render_object_ids=row['render_objects'],excluded_scenery_source_node=row['collider_source_node'],removed_wreck_render_triangles=1770,removed_wreck_collision_triangles=270,source_mixed_sha256=sha(source_ass),retained_palms_records_and_indices_exact=True,all_source_render_corners_normals_uv_indices_preserved=False,all_retained_source_render_corners_normals_uv_indices_preserved=True,all_source_hull_geometry_preserved=False,all_retained_source_hull_geometry_preserved=True)
    report_path.write_text(json.dumps(result,indent=2)+'\n');return result

def install_static_wreck(kit:Path,manifest_path:Path,interface_path:Path,report_path:Path|None=None,*,scenario:str=SCENARIO)->dict:
    """Validate a source manifest, copy owned prefab tags, append one placement.

    ``interface_path`` is the pinned checkout's tag_interface directory. Caller
    runs under tools/run_test.py; kit/scenario writes must resolve inside that
    owner's HALO2_TEST_SCRATCH. Return source/readback hashes and typed fields.
    """
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    kit=Path(kit).resolve(strict=True);manifest_path=Path(manifest_path).resolve(strict=True);interface_path=Path(interface_path).resolve(strict=True)
    if not kit.is_relative_to(scratch):raise ValueError('Static scenery authoring requires an owned private kit')
    revision=subprocess.run(['git','-C',str(interface_path.parent),'rev-parse','HEAD'],capture_output=True,text=True,check=True).stdout.strip()
    if revision!=PINNED_INTERFACE:raise ValueError('Unexpected source tag interface revision')
    manifest=json.loads(manifest_path.read_text());bundle=manifest_path.parent.parent
    if manifest.get('schema')!='DESERTED_STATIC_WRECK_PREFAB_SOURCE_V1' or manifest.get('passed') is not True:raise ValueError('Unqualified frozen wreck source manifest')
    if manifest['source_render_triangles']!=1770 or manifest['source_collision_triangles']!=270:raise ValueError('Unexpected frozen wreck geometry')
    expected='scenarios/multi/dust2_mombasa/wreck_static/wreck_static'
    if manifest['scenery_tag_path']!=expected:raise ValueError('Unexpected prefab tag namespace')
    placement=manifest['placement']
    if placement['rotation_degrees']!=[0,0,0] or placement['scale']!=1. or placement['variant']!='':raise ValueError('Unexpected frozen wreck placement transform')
    for name,row in manifest['files'].items():
        path=(bundle/name).resolve(strict=True)
        if not path.is_relative_to(bundle) or path.stat().st_size!=row['bytes'] or sha(path)!=row['sha256']:raise ValueError('Wreck source hash mismatch: '+name)
    for name,row in manifest['receipts'].items():
        path=bundle/'validation'/name
        if sha(path)!=row['sha256'] or json.loads(path.read_text()).get('passed') is not True:raise ValueError('Wreck source receipt mismatch: '+name)
    scenario_path=kit/'tags'/(scenario+'.scenario')
    if scenario_path.is_symlink() or not scenario_path.resolve(strict=True).is_relative_to(scratch):raise ValueError('Scenario must be a private writable source tag')
    target=kit/'tags'/Path(expected).parent
    target.mkdir(exist_ok=True)
    if not target.resolve().is_relative_to(scratch):raise ValueError('Prefab destination escapes private kit')
    copied=[]
    for extension in ('render_model','collision_model','model','scenery'):
        name='production/tags/'+expected+'.'+extension;source=bundle/name;destination=kit/'tags'/(expected+'.'+extension)
        if destination.exists():raise ValueError('Refuse to replace an existing prefab source tag')
        destination.write_bytes(source.read_bytes());copied.append(dict(path=str(destination.relative_to(kit/'tags')),sha256=sha(destination),bytes=destination.stat().st_size))
    sys.path.insert(0,str(interface_path));import tag_interface as ti
    game,engine,defs,groups,extensions,post,_=ti.get_version_args('halo2',0)
    with tempfile.TemporaryDirectory(prefix='static-scenery-defs-',dir=scratch) as temp:
        temp=Path(temp);merged=game.generate_defs(defs,str(temp/'defs'),groups,extensions)
        def read(path):return ti.read_file(merged,str(kit/'tags'),str(kit/'tags'/path),0,engine_tag=engine)
        document=read(scenario+'.scenario');data=document['Data'];previous_palette=_json(data['scenery palette']);previous_instances=_json(data['scenery'])
        if any(r['name']['path'].replace('\\','/').lower()==expected for r in previous_palette):raise ValueError('Prefab already exists in scenario palette')
        # Zanzibar's palms are BSP instances, so it has no typed palmtree SCEN
        # placement to clone. Use the same authentic Coagulation SCEN template
        # already qualified by this repository's source scenery authoring.
        reference=read('scenarios/multi/halo/coagulation/coagulation.scenario')['Data'];palette=reference['scenery palette'];prototype=reference['scenery'][0];palette_item=copy.deepcopy(palette[prototype['type']]);palette_item['name'].update(path=expected,length=len(expected));palette_index=len(data['scenery palette']);data['scenery palette'].append(palette_item)
        value=copy.deepcopy(prototype);value.update(type=palette_index,name=-1,position=placement['position_wu'],rotation=placement['rotation_degrees'],scale=1.,**{'variant name':'','variant name_pad':0,'placement flags':0,'unique id':0x57000000+len(data['scenery']),'origin bsp index':0,'manual bsp flags':0,'valid multiplayer games':0})
        if isinstance(value.get('bsp policy'),dict):value['bsp policy']['value']=0
        data['scenery'].append(value);before=sha(scenario_path);written=temp/'written.scenario';ti.write_file(merged,document,ti.obfuscation_buffer_prepare(),str(written),engine_tag=engine,tag_groups=groups,tag_extensions=extensions,postprocess_functions=post);scenario_path.write_bytes(written.read_bytes());back=read(scenario+'.scenario')['Data']
        if _json(back['scenery palette'][:-1])!=previous_palette or _json(back['scenery'][:-1])!=previous_instances:raise ValueError('Unrelated scenario palette/placements changed')
        actual=back['scenery'][-1]
        if actual['type']!=palette_index or list(actual['rotation'])!=[0.,0.,0.] or actual['scale']!=1. or max(abs(a-b) for a,b in zip(actual['position'],placement['position_wu']))>1e-6:raise ValueError('Static wreck placement readback mismatch: '+json.dumps(dict(actual=actual,expected=placement,palette_index=palette_index)))
        if back['scenery palette'][palette_index]['name']['path']!=expected:raise ValueError('Static wreck palette readback mismatch')
        report=dict(schema='DESERTED_STATIC_WRECK_SCENARIO_AUTHORING_V1',passed=True,manifest_sha256=sha(manifest_path),interface_revision=revision,source_scenario_sha256=before,authored_scenario_sha256=sha(scenario_path),copied_tags=copied,palette_index=palette_index,placement_index=len(previous_instances),placement=actual,unchanged_prior_scenery_and_palette=True,prototype='Original Coagulation typed SCEN placement',scope='Source scenario authoring/readback only. Cache postprocessing, native factory/render/collision/contact-lighting/persistence remain separate.')
    if report_path:Path(report_path).write_text(json.dumps(report,indent=2)+'\n')
    return report
