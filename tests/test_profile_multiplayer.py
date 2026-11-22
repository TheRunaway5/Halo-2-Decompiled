from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

from tools import profile_multiplayer as profile


class CpuSubtreeProfileTests(unittest.TestCase):
    def test_symbolized_stack_partitions(self):
        base = 0x700000
        outer, anchor, child, leaf = (base + offset for offset in (0x100, 0x200, 0x300, 0x400))
        library = 0x900000
        stacks = [(anchor, outer), (library, anchor, outer), (child, anchor, outer),
                  (leaf, child, anchor, outer), (library, leaf, child, anchor, outer),
                  (anchor, anchor, outer), (library,), (outer,)]
        symbols = '00000100 T Outer\n00000200 T run_1eb790\n00000300 T Child\n00000400 T Leaf\n'
        depth_limit = len(profile.RECORD.unpack(bytes(profile.RECORD.size))) - 4
        with tempfile.TemporaryDirectory() as scratch_path:
            scratch = Path(scratch_path)
            (scratch / 'modules.tsv').write_text(f'{base:x}\t{base + 0x500:x}\t/proc/self/exe\n'
                                                 '# presented=4 begin_ns=100 end_ns=200\n')
            (scratch / 'samples.bin').write_bytes(b''.join(
                profile.RECORD.pack(0, 0, 1 + index % 3, len(stack), *stack,
                                    *([0] * (depth_limit - len(stack))))
                for index, stack in enumerate(stacks)))
            with patch.object(profile.subprocess, 'run',
                              return_value=subprocess.CompletedProcess([], 0, symbols, '')):
                baseline = profile.cpu_functions(Path('synthetic'), scratch, 1, 4)
                focused = profile.cpu_functions(Path('synthetic'), scratch, 1, 4,
                                                focus_functions=('run_1eb790', 'Child', 'absent', 'Child'))

        owner, nested, missing = focused.pop('function_subtrees')['focuses']
        self.assertEqual(baseline, focused)
        self.assertEqual(baseline['external_only_samples'], 1)
        self.assertEqual(baseline['sampled_frame_count'], 3)
        fields = ('inclusive_samples', 'self_samples', 'external_below_anchor_samples',
                  'executable_descendant_samples')
        self.assertEqual(tuple(owner[field] for field in fields), (6, 2, 1, 3))
        self.assertEqual(tuple(nested[field] for field in fields), (3, 1, 0, 2))
        self.assertEqual(tuple(missing[field] for field in fields), (0, 0, 0, 0))
        self.assertEqual([(row['function'], row['samples']) for row in owner['direct_executable_children']],
                         [('Child', 3)])
        self.assertEqual([(row['function'], row['samples']) for row in owner['descendant_leaf_or_external_caller']],
                         [('Leaf', 2), ('Child', 1)])

    def test_output_and_request_bounds(self):
        subtree = profile.CpuSubtreeProfile('anchor')
        for index in range(30):
            subtree.sample((f'leaf-{index}-' + 'x' * 310, f'child-{index}', 'anchor'), True)
        output = subtree.report(30)
        self.assertEqual(output['executable_descendant_samples'], 30)
        self.assertEqual(len(output['direct_executable_children']), 16)
        self.assertEqual(len(output['descendant_leaf_or_external_caller']), 16)
        leaf = output['descendant_leaf_or_external_caller'][0]
        self.assertEqual(len(leaf['function']), 300)
        self.assertTrue(leaf['name_truncated'])
        for invalid in (tuple(str(index) for index in range(9)), ('   ',), ('x' * 161,)):
            with self.assertRaises(ValueError):
                profile.cpu_functions(Path('synthetic'), Path('unused'), 1, 4, focus_functions=invalid)


if __name__ == '__main__':
    unittest.main()
