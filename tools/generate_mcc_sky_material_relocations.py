#!/usr/bin/env python3
"""Retain exact sky shader relocation receipts for the derived V8 input."""
import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE_LIBRARY = "836dc371006640ddaf22b931904c88ee1a9602702fa69c9216b72100ac6cdb84"
MODEL_INPUT = "09c1b41886dea36572f1bbe8aad2401796c8951dfe8b9044690e033ecfe263ca"
BEFORE_MAP = "2751a31d21e01ff516f4a4300602e101579a7df59364716e884f8f7c28bfa080"
AFTER_MAP = "01311e3a20af3ad4638471bd68ff505f64525397950d92bb2417697da41c67d3"


def require(ok, message):
    if not ok:
        raise ValueError("Sky material relocation receipt: " + message)


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode()


def digest(value):
    return hashlib.sha256(value).hexdigest()


def generate(evidence, native):
    original = json.loads((native / "zanzibar/library.json").read_bytes())
    require(digest(canonical(original)) == SOURCE_LIBRARY, "original source library differs")
    records = {r["name"]: r for r in original["shader_tags"] if r["group"] == "shad"}
    rows = []
    for before, after in zip(evidence["v7"]["sky"]["skies"], evidence["v8"]["sky"]["skies"]):
        require(before["input"] == after["input"] == MODEL_INPUT, "complete model input differs")
        require(len(before["materials"]) == len(after["materials"]) == 3, "complete sky material bank differs")
        for a, b in zip(before["materials"], after["materials"]):
            require(a["datum"] == b["datum"] and a["name"] == b["name"], "typed shader owner differs")
            require(a["semantic_sha"] == a["expected_semantic_sha"] == b["semantic_sha"] == b["expected_semantic_sha"], "original sky semantics differ")
            require(a["payload_sha"] == a["expected_payload_sha"] == b["expected_payload_sha"], "original catalog payload differs")
            source = records[a["name"]]
            raw = (native / "zanzibar/payloads" / source["payload"]).read_bytes()
            require(len(raw) == source["bytes"] and digest(raw) + ".bin" == source["payload"], "original complete shader payload differs")
            delta = b["compared_to_v7"]
            require(delta["same_length"] and {v["offset"] for v in delta["different_words"]} == {v[0] for v in source["pointers"]}, "changed words differ from complete declared original pointer set")
            require(all(v["new"] - v["old"] == 2224 for v in delta["different_words"]), "nonliteral allocation shift")
            rows.append({"model_input_sha256": MODEL_INPUT, "shader": a["datum"], "name": a["name"],
                         "map": "zanzibar", "allocation_delta": 2224,
                         "before_payload_sha256": a["payload_sha"], "after_payload_sha256": b["payload_sha"],
                         "semantic_sha256": a["semantic_sha"], "source_record": source,
                         "pointer_words": delta["different_words"]})
    require(len(rows) == 3, "incomplete derived sky input receipt")
    return {"schema": "H2_MCC_SKY_MATERIAL_RELOCATIONS_V1", "before_map_sha256": BEFORE_MAP,
            "after_map_sha256": AFTER_MAP, "source_library_sha256": SOURCE_LIBRARY,
            "cause": "Exact appended granite bitmap owner shifts native projection allocations by2224bytes; complete model and every material semantic digest remain unchanged",
            "materials": rows}


def add_oldmombasa(policy, proof_path, native):
    proof = json.loads(proof_path.read_bytes())
    before = json.loads((proof_path.parent / 'oldmombasa-sky-before.json').read_bytes())
    after = json.loads((proof_path.parent / 'oldmombasa-sky-after.json').read_bytes())
    require(digest((proof_path.parent / 'oldmombasa-sky-before.json').read_bytes()) == proof['before_payload_report_sha256'] and
            digest((proof_path.parent / 'oldmombasa-sky-after.json').read_bytes()) == proof['after_payload_report_sha256'],
            'complete before/after OldMombasa source observation differs')
    library = json.loads((native / '03a_oldmombasa/library.json').read_bytes())
    expected_library = 'a249dafbf4bf99413cc4470c65e83e66ba1c1c94d8fc5bf4deffb4870e53f519'
    require(digest(canonical(library)) == expected_library, 'complete original OldMombasa bank differs')
    records = {r['name']: r for r in library['shader_tags'] if r['group'] == 'shad'}
    observed = {}
    require(len(before['skies']) == len(after['skies']) == 7, 'complete OldMombasa sky census differs')
    for a, b in zip(before['skies'], after['skies']):
        if a.get('authored_model_absent') or b.get('authored_model_absent'):
            require(a == b, 'authored sky model absence differs')
            continue
        require((a['ordinal'], a['model'], a['input']) == (b['ordinal'], b['model'], b['input']), 'complete sky model input differs')
        require(len(a['materials']) == len(b['materials']), 'complete source material bank differs')
        for x, y in zip(a['materials'], b['materials']):
            require((x['index'], x['datum'], x['name'], x['bytes']) == (y['index'], y['datum'], y['name'], y['bytes']), 'typed material owner differs')
            require(x['semantic_sha'] == y['semantic_sha'] == x['expected_semantic_sha'] == y['expected_semantic_sha'], 'original complete material semantics differ')
            require(x['payload_sha'] == x['expected_payload_sha'] == y['expected_payload_sha'], 'original sky catalog beforeimage differs')
            old, new = bytes.fromhex(x['payload_hex']), bytes.fromhex(y['payload_hex'])
            require(digest(old) == x['payload_sha'] and digest(new) == y['payload_sha'], 'complete observed payload differs')
            observed[a['input'], x['datum']] = (x, y, old, new)
    require(len(proof['materials']) == len(observed) == 45, 'complete model/material input extent differs')
    for row in proof['materials']:
        record = records[row['name']]
        require(row['source_record'] == record and row['allocation_delta'] == 992032, 'literal original allocation source differs')
        original = (native / '03a_oldmombasa/payloads' / record['payload']).read_bytes()
        require(len(original) == record['bytes'] and digest(original) + '.bin' == record['payload'], 'full original source shader payload differs')
        x, y, old, new = observed[row['model_input_sha256'], row['shader']]
        require(row['before_payload_sha256'] == x['payload_sha'] and row['after_payload_sha256'] == y['payload_sha'], 'relocation before/after owner differs')
        require(len(old) == len(new) == record['bytes'] and
                {v['offset'] for v in row['pointer_words']} == {v[0] for v in record['pointers']}, 'complete declared source pointer set differs')
        replay = bytearray(new)
        for word in row['pointer_words']:
            at = word['offset']
            require(int.from_bytes(old[at:at+4], 'little') == word['old'] and
                    int.from_bytes(new[at:at+4], 'little') == word['new'] and
                    word['new'] - word['old'] == 992032, 'literal source pointer words differ')
            replay[at:at+4] = old[at:at+4]
        require(bytes(replay) == old, 'non-pointer source word changed')
        policy['materials'].append({'map': '03a_oldmombasa', **row})
    policy['additional_source_transitions'] = [{'map': '03a_oldmombasa',
        'before_map_sha256': '395ad7bccefdb3d50950dc911e8adeee770a75e6ad109c3979d6a641fd648475',
        'after_map_sha256': '5941140f8d8883f99c0efbeca5ac1d7a1d7e14c20a76bc40e419604d33e401ee',
        'source_library_sha256': expected_library, 'allocation_delta': 992032,
        'models': proof['skies'], 'proof_sha256': digest(proof_path.read_bytes()),
        'scope': 'Original 17-frame bitmap owner append; all original model inputs, complete material semantics and every non-pointer byte unchanged'}]
    return policy


def render(policy):
    text = json.dumps(policy, indent=2) + "\n"
    code = '// Generated exact derived-input receipt; original sky catalog remains immutable.\n'
    code += '// Data SHA256: ' + digest(text.encode()) + '\n'
    code += 'constexpr char sky_material_relocation_storage[]=R"H2SKYRELOC(' + canonical(policy).decode() + ')H2SKYRELOC";\n'
    code += 'constexpr std::string_view sky_material_relocation_policy{sky_material_relocation_storage,sizeof(sky_material_relocation_storage)-1};\n'
    return text, code


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--evidence', type=Path, required=True)
    p.add_argument('--native', type=Path, required=True)
    p.add_argument('--check', action='store_true')
    p.add_argument('--oldmombasa-proof', type=Path)
    a = p.parse_args()
    policy = generate(json.loads(a.evidence.read_bytes()), a.native)
    if a.oldmombasa_proof:
        policy = add_oldmombasa(policy, a.oldmombasa_proof, a.native)
    for path, value in zip((ROOT / 'tools/data/mcc-sky-material-relocations.json', ROOT / 'src/rendering/models/mcc_sky_material_relocations.inc'), render(policy)):
        if a.check:
            require(path.read_text() == value, 'stale generated source')
        else:
            path.write_text(value)
    print(str(len(policy['materials'])) + ' exact source-owned sky payload relocation receipts authenticated')


if __name__ == '__main__':
    main()
