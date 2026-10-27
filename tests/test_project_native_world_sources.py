import copy
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
from project_native_world_sources import project


def corpus():
    actual = json.loads((ROOT/'shaders/native-programs/world/programs.json').read_bytes())
    result = {key: copy.deepcopy(actual[key]) for key in ('schema', 'abi', 'target', 'source_authority',
        'dynamic_color_words', 'all_materials', 'prepared_inventories', 'rejected')}
    result['entries'] = [copy.deepcopy(actual['entries'][0])]
    result['cpu_vertex_programs'] = [copy.deepcopy(actual['cpu_vertex_programs'][0])]
    row = {'map': 'source.map', 'source': 'original-disc', 'sha256': 'a'*64}
    identity = row['source']+':'+row['sha256']+':'+row['map']
    result['maps'] = [row]
    for kind, count in (('entries', 7), ('cpu_vertex_programs', 11)):
        result[kind][0]['aliases'] = result[kind][0]['aliases'][:1]
        result[kind][0]['source_owner_evidence'] = {'count': count, 'sha256': 'b'*64,
            'map_counts': {identity: count}}
    result.update(fragment_programs=[], metadata_rejected=[], non_world_inputs=[])
    return result


class NativeWorldSourceProjection(unittest.TestCase):
    def test_full_native_identities_and_owner_totals_survive_compaction(self):
        source = corpus();before = copy.deepcopy(source)
        result = project(source, {'path': 'evidence.json.xz', 'decoded_sha256': 'c'*64})
        self.assertEqual(source, before)
        for kind in ('entries', 'cpu_vertex_programs'):
            self.assertEqual(result[kind][0]['vertex_program_hex'], source[kind][0]['vertex_program_hex'])
            self.assertEqual(result[kind][0]['vertex_instructions'], source[kind][0]['vertex_instructions'])
            self.assertNotIn('map_counts', result[kind][0]['source_owner_evidence'])
            self.assertEqual(result[kind][0]['source_owner_evidence']['sha256'], 'b'*64)
        self.assertEqual(result['maps'][0]['native_world_tuple_owner_records'], 7)
        self.assertEqual(result['maps'][0]['native_cpu_vertex_owner_records'], 11)

    def test_partial_inventory_changed_words_and_inconsistent_owner_counts_reject(self):
        source = corpus();source['prepared_inventories'][0]['complete'] = False
        with self.assertRaises(ValueError):project(source, {})
        source = corpus();source['entries'][0]['vertex_instructions'][0][0] ^= 1
        with self.assertRaises(ValueError):project(source, {})
        source = corpus();source['entries'][0]['source_owner_evidence']['count'] += 1
        with self.assertRaises(ValueError):project(source, {})


if __name__ == '__main__':
    unittest.main()
