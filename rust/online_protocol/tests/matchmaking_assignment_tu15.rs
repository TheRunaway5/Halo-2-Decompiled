//! Replay the real TU1.5 caller corridor through the production Rust owner.
//! Synthetic ready-summary operands do not qualify upstream session producers.
use halo2_online_protocol::team_assignment::{Hopper, assign};
use serde_json::Value;

fn words(value: &Value) -> Vec<i32> {
    value
        .as_array()
        .unwrap()
        .iter()
        .map(|v| v.as_i64().unwrap() as i32)
        .collect()
}

#[test]
fn unchanged_tu15_assignment_corridor_and_getters() {
    let fixture: Value = serde_json::from_str(include_str!(
        "../../../tests/fixtures/matchmaking_assignment_tu15_original.json"
    ))
    .unwrap();
    assert_eq!(
        fixture["original_sha256"],
        "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"
    );
    assert_eq!(fixture["epoch"], "tu15");
    let cases = fixture["cases"].as_array().unwrap();
    assert_eq!(cases.len(), 336);
    for (index, case) in cases.iter().enumerate() {
        let fields = words(&case["fields"]);
        let mut header = [0u8; 0x614];
        for (offset, value) in [0, 0x5ec, 0x5f0, 0x5f4, 0x5f8, 0x604, 0x608, 0x60c]
            .into_iter()
            .zip(&fields)
        {
            header[offset..offset + 4].copy_from_slice(&value.to_le_bytes());
        }
        let hopper = Hopper::from_header(&header).unwrap();
        let (flag, minimum, maximum) = hopper.flags();
        let getters = vec![
            hopper.team_count(),
            hopper.maximum_team_size(),
            i32::from(flag),
            minimum,
            maximum,
            hopper.allowed_imbalance(),
            i32::from(hopper.rank_visible()),
            i32::from(hopper.is_team()),
        ];
        assert_eq!(
            getters,
            words(&case["getters"]),
            "TU1.5 getters case {index}"
        );
        let actual = assign(
            hopper,
            &words(&case["parties"]),
            &words(&case["raw_weights"]),
        );
        let expected = (!case["assignments"].is_null()).then(|| words(&case["assignments"]));
        assert_eq!(
            actual.as_ref().map(|r| &r.teams),
            expected.as_ref(),
            "TU1.5 assignment case {index}"
        );
        assert_eq!(
            actual.as_ref().map_or(-1, |r| r.path as i32),
            case["path"].as_i64().unwrap() as i32,
            "TU1.5 retry path case {index}"
        );
    }
}
