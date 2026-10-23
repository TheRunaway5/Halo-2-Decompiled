#!/usr/bin/env python3
"""Check the compiled skin kernel's explicit float controls and contraction policy."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--disassembler', required=True)
    parser.add_argument('--shader', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    text = subprocess.check_output([args.disassembler, str(args.shader)], text=True)
    arithmetic = re.findall(r'^\s*(%\S+) = OpF(?:Mul|Add) ', text, re.M)
    precise = set(re.findall(r'OpDecorate (%\S+) NoContraction', text))
    modes = re.findall(r'OpExecutionMode %\S+ (\S+) 32', text)
    if not {'RoundingModeRTE', 'SignedZeroInfNanPreserve'} <= set(modes):
        raise RuntimeError('Skin shader lacks explicit float32 RTE/signed-zero controls')
    if len(arithmetic) != 15 or not set(arithmetic) <= precise:
        raise RuntimeError('Skin shader lost its fifteen separately rounded MUL/ADD instructions')
    if any(x in text for x in [' Fma ', 'OpDot ', 'OpMatrixTimes', 'OpVectorTimesMatrix',
                               'RelaxedPrecision', 'FPFastMathMode']):
        raise RuntimeError('Skin shader contains an unqualified floating-point optimization')
    report = {'passed': True, 'shader_sha256': hashlib.sha256(args.shader.read_bytes()).hexdigest(),
              'float32_modes': modes, 'separate_no_contraction_arithmetic': len(arithmetic)}
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
