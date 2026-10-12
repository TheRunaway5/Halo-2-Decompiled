#!/usr/bin/env python3
"""Execute actual Oddball carrier-camouflage flag before original cap.

This supplements the immutable render-context corpus, whose options+22C is
cold zero. Whole registered slot15, inventory search, actual slot35 selector1,
cap and entity-NONE dirty wrapper execute without patches or supplied returns.
Only the declared options+22C input adds a caller branch to this corpus.
"""
import itertools
import json
from pathlib import Path

from oracle_camouflage_render_context import Context, ROOT, OUT, HudCache, G, sha


class Objective(Context):
    objective_flags = 0

    def setup(self, row):
        super().setup(row)
        self.w(G + 0x22c, self.objective_flags)


def main():
    cache = HudCache(ROOT / 'assets/disc/maps/lockout.map')
    rows = [dict(primary=p, flags=f, unit_none=n, global_flags=4, mode=m, item='ball', slot=s, objective_flags=o)
            for p, f, n, m, s, o in itertools.product((.25, .5, .75), (0, 8), (False, True), (1, 4), range(4), (0, 2))]
    versions = {}
    for name in ('retail', 'tu11', 'u5'):
        t = Objective(name, cache)
        actual = []
        for row in rows:
            t.objective_flags = row['objective_flags']
            result = t.callback(row)
            result['objective_flags_after'] = t.u(G + 0x22c)
            actual.append(result)
        versions[name] = dict(image_sha256=t.spec['pin'], owners=t.d, callbacks=actual)
    def normalize(v):
        return [{k: value for k, value in row.items() if k != 'calls'} for row in v['callbacks']]
    assert normalize(versions['retail']) == normalize(versions['tu11']) == normalize(versions['u5'])
    output = OUT / 'objective-cap'
    output.mkdir(exist_ok=True)
    fixture = output / 'fixture.json'
    fixture.write_text(json.dumps(dict(schema='original-camouflage-objective-cap-v1', boundary=__doc__, inputs=rows, versions=versions,
                                      instruction_patches=[], supplied_returns=[]), sort_keys=True, separators=(',', ':')) + '\n')
    report = dict(schema='original-camouflage-objective-cap-report-v1', boundary=__doc__, fixture_sha256=sha(fixture),
                  oracle_sha256=sha(__file__), base_oracle_sha256=sha(ROOT / 'tools/oracle_camouflage_render_context.py'),
                  base_report_sha256=sha(OUT / 'report.json'), whole_callback_calls=len(rows) * 3,
                  options_offset=0x22c, carrier_camouflage_bit=1, instruction_patches=[], supplied_returns=[],
                  limits=['Actual ball inventory is declared; objective pickup/possession producer is outside this corpus.',
                          'Only selected Oddball trait selector1 is reached; CTF callback remains static evidence.',
                          'Unit replication entity is NONE; successful replicated dirty output is outside this corpus.'])
    (output / 'report.json').write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
