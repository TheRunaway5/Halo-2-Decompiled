#!/usr/bin/env python3
"""Compare native value proposals with retained, test-only original values."""
from pathlib import Path
import argparse
import gzip
import json
import subprocess
import tempfile

p = argparse.ArgumentParser()
kind = p.add_mutually_exclusive_group(required=True)
kind.add_argument('--math-probe', type=Path)
kind.add_argument('--definition-probe', type=Path)
p.add_argument('--maps', type=Path)
p.add_argument('--vectors', type=Path, required=True)
p.add_argument('--report', type=Path, required=True)
a = p.parse_args()
vectors = json.loads(gzip.decompress(a.vectors.read_bytes()))
assert len(vectors['conversion_controls']) == 1779
assert len(vectors['quantization_controls']) == 840
if a.math_probe:
    data = ''.join('0 ' + str(r['cw']) + ' ' + ' '.join(map(str, r['input_bits'])) + '\n' for r in vectors['conversion_controls'])
    data += ''.join('1 ' + str(r['cw']) + ' ' + str(r['input_bits']) + '\n' for r in vectors['quantization_controls'])
    result = subprocess.run([str(a.math_probe)], input=data, text=True, capture_output=True, check=True)
    assert not result.stderr, result.stderr
    actual = [list(map(int, line.split())) for line in result.stdout.splitlines()]
    expected = [r['words'] for r in vectors['conversion_controls']] + [[r['word']] for r in vectors['quantization_controls']]
    assert actual == expected, 'Native parameter proposal differs from original values'
    report = {'passed': True, 'hardware_original_proposals': 1779, 'original_quantizer_controls': 840,
              'install_ack': False, 'full_live_caller_FP_proven': False, 'physical_Xbox_parity_proven': False}
else:
    assert a.maps and all((a.maps / name).is_file() for name in ['mainmenu.map', '00a_introduction.map', 'lockout.map']), 'Actual imported MCC caches are required'
    with tempfile.TemporaryDirectory(prefix='halo2-mono-definitions-') as temporary:
        root = Path(temporary)
        reference = root / 'values.json'
        output = root / 'report.json'
        reference.write_text(json.dumps(vectors))
        subprocess.run([str(a.definition_probe), str(a.maps), str(reference), str(output)], check=True)
        report = json.loads(output.read_text())
        assert report['passed'] and report['guards'] == 81
        assert report['original_parameter_proposals_and_controls'] == 50
        assert not report['graph_ready'] and not report['install_ack']
        assert report['authored_initial_gain'] == 3000000 and report['converted_default_gain'] == 3003121
a.report.write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report))
