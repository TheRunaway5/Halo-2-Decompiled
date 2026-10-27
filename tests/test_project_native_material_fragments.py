import copy
import hashlib
import json
from pathlib import Path
import sys
import unittest

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from project_native_material_fragments import project


def corpus():
    entry=copy.deepcopy(json.loads((ROOT/'shaders/native-programs/world/programs.json').read_text())['entries'][0])
    entry['aliases']=[{'kind':'material-pass','source':'original-disc','map':'source.map','datum':17,
        'pass':0,'implementation':0,'vertex_shader':'shaders\\vertex_shaders\\stencil_texture',
        'template_name':'shaders\\shader_templates\\opaque\\emblem_overlay_simple'}]
    other=[0]*60;other[53]=2
    return {'schema':'HALO2_NATIVE_WORLD_KERNEL_PROGRAMS_V1','abi':1,'target':'vulkan1.3','all_materials':True,
        'entries':[entry],'maps':[{'map':'source.map'}],
        'prepared_inventories':[{'complete':True,'declared_maps':1}],
        'fragment_programs':[{'controls':other,'aliases':[{'kind':'raw-pass-implementation','group':'spas','name':'unused'}]}]}


class NativeMaterialProjectionTests(unittest.TestCase):
    def test_roles_follow_actual_source_owners_not_a_cross_product(self):
        value=corpus();entry=value['entries'][0]
        digest=hashlib.sha256(bytes.fromhex(entry['vertex_program_hex'])).hexdigest()
        result=project(value,'a'*64,[digest])
        families=result['source_census']['families']
        self.assertEqual(set(families),{'world','menu','world-emblem','pstencil-texture'})
        self.assertEqual(len(result['source_census']['raw_source_programs_without_a_proved_runtime_family']),1)
        for row in result['entries']:
            self.assertTrue(row['provenance']['representative_source_owners'])
            self.assertEqual(len(row['provenance']['complete_source_evidence_sha256']),64)

    def test_typed_hud_particle_and_explicit_constructor_roles(self):
        value=corpus();program=[0]*60
        value['fragment_programs']=[
            {'controls':program,'aliases':[{'kind':'particle-pass','group':'prt3','datum':19}]},
            {'controls':program,'aliases':[{'kind':'hud-material-pass','family':'hud','typed_owners':[{'group':'nhdt','owner':20}]}]},
            {'controls':program,'aliases':[{'kind':'native-source-constructor','family':'pstencil-light-utility','also_family':'pstencil-normal-utility'}]},
            {'controls':program,'aliases':[{'kind':'native-fixed-procedural','family':'world-fixed'}]}]
        result=project(value,'b'*64)
        families=result['source_census']['families']
        self.assertTrue({'particle','hud','pstencil-light-utility','pstencil-normal-utility','world-fixed'}<=set(families))
        self.assertNotIn('bloom',families)

    def test_incomplete_inventory_partial_export_and_unknown_roles_reject(self):
        value=corpus();value['all_materials']=False
        with self.assertRaises(ValueError):project(value,'c'*64)
        value=corpus();value['prepared_inventories'][0]['complete']=False
        with self.assertRaises(ValueError):project(value,'c'*64)
        value=corpus();value['fragment_programs'][0]['aliases'][0]['family']='arbitrary-guess'
        with self.assertRaises(ValueError):project(value,'c'*64)

    def test_bounded_aliases_preserve_complete_owner_digest_and_count(self):
        value=corpus();entry=value['entries'][0]
        entry['source_owner_evidence']={'count':100000,'sha256':'d'*64,
            'retention':'One representative per routing signature'}
        result=project(value,'e'*64)
        row=next(row for row in result['entries'] if row['family']=='world')
        provenance=row['provenance']
        self.assertEqual(provenance['source_evidence_count'],1)
        complete=list(provenance['complete_source_identity_evidence'].values())
        self.assertEqual(len(complete),1)
        self.assertEqual(complete[0]['count'],100000)
        self.assertEqual(complete[0]['sha256'],'d'*64)
        entry['source_owner_evidence']['sha256']='z'*64
        with self.assertRaises(ValueError):project(value,'e'*64)


if __name__=='__main__':unittest.main()
