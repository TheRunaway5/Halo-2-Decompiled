#!/usr/bin/env python3
"""Import paired immutable sound-create words from the actual owned executables."""
import argparse,json
from pathlib import Path
from oracle_mp_title_update_display import OLD,NEW,PINS,Xbe,sha,save,encoded

FIELDS=('default_extra_gain_bits','minimum_gain_bits','maximum_float_bits',
        'rng_scale_bits','unsigned_conversion_correction_bits','one_bits',
        'upper_gain_bits','db_exponent_scale_bits','db_exponent_log_bits',
        'linear_db_scale_bits','fade_epsilon_bits')

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--providers',type=Path,required=True)
    parser.add_argument('--providers-sha256',required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    assert sha(args.providers)==args.providers_sha256
    providers=json.loads(args.providers.read_text())
    assert providers['mutable_delay_scale_excluded']is True and len(providers['words'])==len(FIELDS)
    lines=[];images=[]
    for index,(side,path)in enumerate([('old',OLD),('u5',NEW)],1):
        assert sha(path)==PINS[str(path)];xbe=Xbe(path);words=[];operands=[]
        for field,item in zip(FIELDS,providers['words'],strict=True):
            address=int(item[side+'_va'],16);raw=xbe.read(address,4)
            assert raw.hex()==item['raw_hex']
            words.append(f'{int.from_bytes(raw,"little"):08x}')
            operands.append(dict(field=field,address=f'{address:08x}',raw_hex=raw.hex()))
        lines.append('\t'.join(['C',str(index),*words]))
        images.append(dict(source_path=str(path),source_sha256=sha(path),operands=operands))
    fixture=args.output/'constants.tsv';save(fixture,('\n'.join(lines)+'\n').encode())
    report=dict(schema='owned-sound-create-immutable-operands-v1',
                exporter_sha256=sha(__file__),providers_path=str(args.providers),
                providers_sha256=args.providers_sha256,images=images,immutable_bytes=88,
                fixture_sha256=sha(fixture),mutable_state_imported=False,
                limits=['Base-image operand mapping remains unverified and absent.',
                        'Mutable delay scale, class fades, source/controller pools, local RNG and cache/admission retain actual owners.'])
    save(args.output/'report.json',encoded(report))
    print(json.dumps(dict(fixture_sha256=sha(fixture),report_sha256=sha(args.output/'report.json'),immutable_bytes=88)))
if __name__=='__main__':main()
