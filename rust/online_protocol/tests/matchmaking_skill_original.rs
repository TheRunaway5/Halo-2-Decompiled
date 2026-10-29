//! Comparison inputs are synthetic; each expected result is unchanged XBE output.
use halo2_online_protocol::matchmaking_skill as skill;
use serde_json::{Value, json};
use sha2::{Digest, Sha256};

#[test]
fn unchanged_retail_and_tu15_pairs_collections_clans_and_hysteresis_match_rust() {
    let bytes = include_bytes!("../../../tests/fixtures/matchmaking_skill_original.json");
    let f: Value = serde_json::from_slice(bytes).unwrap();
    assert_eq!(f["schema"], "H2_MATCHMAKING_SKILL_ORIGINAL_V1");
    assert_eq!(f["policies"].as_array().unwrap().len(), 2);
    let mut comparisons = 0;
    for original in f["policies"].as_array().unwrap() {
        let (tables, digest) = skill::policy(original["policy"].as_str().unwrap()).unwrap();
        assert_eq!(original["table_sha256"], digest);
        assert_eq!(original["tables"], tables.value());
        assert_eq!(original["controls"].as_array().unwrap().len(), 955);
        for case in original["controls"].as_array().unwrap() {
            let mut request = case["input"].clone();
            request["op"] = case["op"].clone();
            request["tables"] = tables.value();
            if !case["limits"].is_null() {
                request["tables"]["delta_bounds"] = case["limits"].clone();
            }
            assert_eq!(
                skill::value(&request).unwrap(),
                case["actual"],
                "{} {}",
                original["policy"],
                case
            );
            comparisons += 1;
        }
    }
    assert_eq!(comparisons, 1910);
    assert_eq!(
        skill::value(&json!({"op":"rank_display","rank":127})).unwrap(),
        50
    );
    // Keep the fixture hash available in normal test receipts as provenance.
    println!(
        "original fixture sha256={:x}; comparisons={comparisons}",
        Sha256::digest(bytes)
    );
}
