#!/usr/bin/env python3
"""Seal the additive private-lifetime API only after exact-source proofs."""
from __future__ import annotations
import argparse
import difflib
import hashlib
import json
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[1]


def sha(p: Path) -> str:
    return hashlib.sha256(p.read_bytes()).hexdigest()


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--stage", type=Path, required=True)
    p.add_argument("--private-proof", type=Path, required=True)
    p.add_argument("--public-proof", type=Path, required=True)
    p.add_argument("--lifetime-proof", type=Path, required=True)
    a = p.parse_args()
    stage = a.stage
    manifest = json.loads((stage / "source-pins.json").read_text())
    parent_path = ROOT / manifest["parent_manifest"]
    assert sha(parent_path) == manifest["parent_manifest_sha256"]
    parent = parent_path.parent
    parent_peer = ROOT / "analysis/physics/native-contact-private-data-independent-peer-review-v6-20261102/review.json"
    peer = json.loads(parent_peer.read_text())
    assert peer.get("passed", peer.get("review_passed")) and not peer.get("findings")
    changed = []
    patch = []
    for rel, pin in manifest["source_sha256"].items():
        current = stage / "staged" / rel
        assert sha(current) == pin, rel
        before = parent / "staged" / rel
        if current.read_bytes() != before.read_bytes():
            changed.append(rel)
            patch.extend(difflib.unified_diff(before.read_text().splitlines(True), current.read_text().splitlines(True), "a/" + rel, "b/" + rel))
    assert sorted(changed) == ["include/halo2/native_tu15_contact_candidate_private_data.hpp", "include/halo2/native_tu15_retained_store_observation.hpp"]
    proofs = []
    for path, public in ((a.private_proof, False), (a.public_proof, True)):
        report = json.loads(path.read_text())
        assert report["passed"] and not Path(report["owned_scratch"]).exists()
        assert not report["captured_bank_exclusions"] and not report["captured_gp_or_after_consumed"]
        assert report["public_shared_algorithm_default_domain"] == public
        assert sorted((r["compiler"], r["exit_code"]) for r in report["results"]) == [("clang++", 0), ("g++", 0)]
        for rel, pin in manifest["source_sha256"].items():
            key = rel if rel.startswith("include/") else Path(rel).name
            expected = pin
            if public and rel.endswith("native_tu15_contact_candidates_binding.hpp"):
                text = (stage / "staged" / rel).read_text()
                old = "        native_tu15_update_contact_candidates_defined_data(memory,producer"
                assert text.count(old) == 1
                text = text.replace(old, "        native_tu15_update_contact_candidates(memory,producer")
                expected = hashlib.sha256(text.encode()).hexdigest()
            assert report["source_hashes"][key] == expected, (str(path), rel)
        for compiler in ("g++", "clang++"):
            evidence = json.loads((path.parent / compiler / "evidence/actual-report.json").read_text())
            assert evidence["passed"] and len(evidence["profiles"]) == 4
            if not public:
                assert evidence["private_data_controls"] == 57
            for profile in evidence["profiles"]:
                assert profile["passed"] and profile["compared_bytes"] == 89795481 and profile["different_bytes"] == 0
                assert profile["read_order_equal"] and profile["store_order_equal"]
                assert len(profile["immutable_dispatch_CODE_reads"]) == 114
        proofs.append({"path": str(path), "sha256": sha(path), "passed": True,
                       "owner_absent": True, "public_default_call_override": public,
                       "removed_allocated_bytes": report["temporary_allocated_bytes_before_cleanup"]})
    life = json.loads(a.lifetime_proof.read_text())
    assert life["passed"] and not Path(life["owner"]).exists() and len(life["results"]) == 10
    assert all(r["passed"] for r in life["results"])
    assert {r["diagnostics"].strip() for r in life["results"] if r["variant"] == "baseline"} == {"19"}
    assert life["header_sha256"] == manifest["source_sha256"]["include/halo2/native_tu15_contact_candidate_private_data.hpp"]
    proofs.append({"path": str(a.lifetime_proof), "sha256": sha(a.lifetime_proof),
                   "passed": True, "owner_absent": True, "controls_per_compiler": 19,
                   "mutants_rejected_per_compiler": 4})
    (stage / "native-private-lifetime-from-v6.diff").write_text("".join(patch))
    frozen = stage / "proof-source/tools"
    frozen.mkdir(parents=True, exist_ok=True)
    for name in ("validate_native_contact_private_lifetime_tu15.py", "native_contact_candidate_private_data_checks_v7_tu15.hpp",
                 "validate_native_contact_candidates_bound_actual_tu15.py", "observe_native_contact_candidates_bound_actual_tu15.cpp",
                 "seal_native_contact_private_lifetime_tu15.py"):
        shutil.copyfile(ROOT / "tools" / name, frozen / name)
    manifest.update(qualified=True, independent_peer_reviewed=False, root_reviewed=False,
                    world_or_live_admission=False, actual_private_and_public_default_qualified=True,
                    qualification_scope="Additive bounded private-lifetime metadata API. Same native2F candidate algorithms; both compilers/fourFP controls/all89795481 captured bytes/ordered physics DATA and retained stores/FP. Native private path57 controls, independent lifetime model19 controls/four mutants per compiler. No new full304, World, live, raw-private observation or retained-byte exception.",
                    changed_files=changed, inherited_v6_peer={"path": str(parent_peer.relative_to(ROOT)), "sha256": sha(parent_peer)},
                    proofs=proofs,
                    proof_source_sha256={str(x.relative_to(stage)): sha(x) for x in frozen.iterdir() if x.is_file()},
                    remaining=["Final independent additive source/spec/pins peer", "Root full304 lifetime/defined swap/provider proof", "Coherent source/World/counters/live integration", "Historical retained-record growth/heap mismatches and raw-private observation domains remain open"])
    (stage / "source-pins.json").write_text(json.dumps(manifest, indent=2) + "\n")
    (stage / "QUALIFICATION.md").write_text("""# Additive lifetime metadata qualification

The23-source candidate inherits peer-reviewed v6. Only the shared optional invalidation interface and private arena header change: owned-range invalidation propagates through the actual outer domain, copy/move is prohibited, and extent admission checks the size before addition. Invalidated bytes never acquire numerical values; undefined reads and retained escapes still refuse.

Both compilers pass19 independent lifetime controls and reject four mutants, including the near-SIZE_MAX extent case. Exact selected native2F and public-default replays each pass fourFP profiles and all89,795,481 captured bytes, ordered physics DATA/retained stores and FP. The native path also passes57 ownership/lifetime controls. Earlier mutation-construction and unsealed-draft overflow findings remain preserved.

Every unit must use the same shared header in a new coherent source build. This is not binary compatibility with an older Domain vtable. Root full304, World/counters/live integration, generic raw private observations and historical retained-record heap failures remain outside this component qualification.
""")
    print(json.dumps({"qualified": True, "manifest": str(stage / "source-pins.json"),
                      "manifest_sha256": sha(stage / "source-pins.json")}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
