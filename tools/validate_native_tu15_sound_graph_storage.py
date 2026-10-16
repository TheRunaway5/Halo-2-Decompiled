#!/usr/bin/env python3
"""Compare typed GP constructor fields without an original executable or bank."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--probe', type=Path, required=True)
parser.add_argument('--vectors', type=Path, required=True)
parser.add_argument('--report', type=Path, required=True)
args = parser.parse_args()
data = args.vectors.read_bytes()
vectors = json.loads(data)
assert vectors['schema'] == 'H2_TU15_NATIVE_GP_GRAPH_LITERAL_FIELDS_V1'
assert vectors['records'] == 15 and vectors['fields'] == 462
completed = subprocess.run([str(args.probe.resolve())], capture_output=True, text=True, check=True)
assert not completed.stderr
actual = json.loads(completed.stdout)
assert actual['passed'] and actual['literal_nodes'] == vectors['literal_nodes']
assert actual['scratch_bytes'] == 774144 and actual['rejections'] == 13
assert actual['cached_chorus_calls'] == 2 and not actual['runtime_ready']
report = {'passed': True, 'literal_records': 15, 'literal_fields': 462,
          'logical_title_scratch_bytes': 774144, 'constructor_guards': 13,
          'actual_retained_chorus_calls': 2,
          'runtime_original_program_or_executable_loaded': False,
          'physical_DMA_byte_image_verified': False, 'whole_native_DSP_ready': False,
          'initializer45_complete': False,
          'fixture_sha256': hashlib.sha256(data).hexdigest(),
          'probe_sha256': hashlib.sha256(args.probe.read_bytes()).hexdigest()}
args.report.write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report))
