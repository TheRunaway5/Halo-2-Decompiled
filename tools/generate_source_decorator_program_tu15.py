#!/usr/bin/env python3
"""Lower the source-owned authenticated TU1.5 decorator class0 program."""
import argparse
import hashlib
import json
from pathlib import Path
from generate_particle_graphics_program import HELPERS
from generate_particle_vertex_probe import translate


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--program',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    data=json.loads(args.program.read_text())
    code=bytes.fromhex(data['vertex_program_hex'])
    if (len(code)!=212 or data['classification']!=0 or data['semantics']!=[0,3,9,14] or
        hashlib.sha256(code).hexdigest()!='fb001da8b50fcb09257ce5e079fb43813285772658cecfba827c9194e289bd4d'):
        raise ValueError('Source decorator program or declaration differs from authenticated TU1.5')
    output=HELPERS+translate(code,'decoratorProgram',
        attribute_expression='decoratorAttributes[i]',corner_expression=None)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(output)
    args.output.with_suffix('.json').write_text(json.dumps({
        'source_program_sha256':hashlib.sha256(code).hexdigest(),
        'generated_source_sha256':hashlib.sha256(output.encode()).hexdigest(),
        'runtime_instruction_bank':False,'expected_vertices_embedded':False,
        'authority':'Original TU1.5 class0 decorator_decal; arithmetic follows the existing source shader backend',
    },indent=2)+'\n')


if __name__=='__main__':main()
