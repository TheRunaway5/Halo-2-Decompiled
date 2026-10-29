use halo2_online_protocol::team_assignment::{
    self, AssignmentPath, Hopper, HopperKind, NativePolicy,
};
use serde_json::Value;

#[test]
fn unchanged_original_assignment_corridor() {
    let fixture: Value = serde_json::from_str(include_str!(
        "../../../tests/fixtures/matchmaking_assignment_original.json"
    ))
    .unwrap();
    assert_eq!(fixture["schema"], "H2_RETAIL_ASSIGNMENT_CORRIDOR_V1");
    assert_eq!(
        fixture["original_sha256"],
        "03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d"
    );
    assert_eq!(fixture["entry"], "0x71b1e");
    let cases = fixture["cases"].as_array().unwrap();
    assert_eq!(cases.len(), 336);
    let mut paths = [false; 4];
    for case in cases {
        let values = |name: &str| -> Vec<i32> {
            case[name]
                .as_array()
                .unwrap()
                .iter()
                .map(|n| n.as_i64().unwrap() as i32)
                .collect()
        };
        let f = values("fields");
        let hopper = Hopper {
            kind: f[0].try_into().unwrap(),
            width: f[1],
            height: f[2],
            depth: f[3],
            imbalance: f[4],
            flag_enabled: f[5] != 0,
            flag_minimum: f[6],
            flag_maximum: f[7],
        };
        let (flag, min, max) = hopper.flags();
        assert_eq!(
            [
                hopper.team_count(),
                hopper.maximum_team_size(),
                i32::from(flag),
                min,
                max,
                hopper.allowed_imbalance(),
                i32::from(hopper.rank_visible()),
                i32::from(hopper.is_team())
            ],
            values("getters").as_slice(),
            "original getters: {case}"
        );
        let actual = team_assignment::assign(hopper, &values("parties"), &values("raw_weights"));
        let expected_path = case["path"].as_i64().unwrap() as i32;
        paths[(expected_path + 1) as usize] = true;
        assert_eq!(
            actual.as_ref().map_or(-1, |a| a.path as i32),
            expected_path,
            "original path: {case}"
        );
        assert_eq!(
            actual.map(|a| a.teams),
            if case["assignments"].is_null() {
                None
            } else {
                Some(values("assignments"))
            },
            "original outputs: {case}"
        );
    }
    assert_eq!(
        paths, [true; 4],
        "failure, FFA, preserved and casual-retry paths must all run"
    );
}

#[test]
fn native_adapter_uses_raw_original_assignment_for_two_teams() {
    let policy = NativePolicy {
        ranked: true,
        team_count: 2,
        team_size: 4,
        clan_minimum: 0,
        clan_imbalance: 0,
        significant_minimum: 0,
        significant_maximum: 0,
        significant_spread: 0,
    };
    let actual = policy
        .assign(&[0, 1, 2, 3, 4, 5, 6, 7], &[24, 24, 22, 22, 22, 22, 20, 20])
        .unwrap();
    assert_eq!(actual.path, AssignmentPath::PreservedParties);
    assert_eq!(actual.teams, [0, 1, 0, 1, 0, 1, 0, 1]);
}

#[test]
fn header_parsing_retains_original_words_and_rejects_wrong_owner() {
    let mut raw = vec![0; team_assignment::HOPPER_BYTES];
    for (offset, value) in [
        (0, 3i32),
        (0x5ec, 2),
        (0x5f0, 3),
        (0x5f4, 4),
        (0x5f8, 0x100),
        (0x608, 3),
        (0x60c, 4),
    ] {
        raw[offset..offset + 4].copy_from_slice(&value.to_le_bytes());
    }
    raw[0x604] = 255;
    let hopper = Hopper::from_header(&raw).unwrap();
    assert_eq!(hopper.kind, HopperKind::CasualTeams);
    assert_eq!(hopper.allowed_imbalance(), 0);
    assert_eq!(hopper.flags(), (false, 0, 0));
    raw[0x5f8] = 1;
    assert_eq!(Hopper::from_header(&raw).unwrap().allowed_imbalance(), 1);
    assert!(Hopper::from_header(&raw[..raw.len() - 1]).is_none());
    raw[0] = 6;
    assert!(Hopper::from_header(&raw).is_none());
}

#[test]
fn invalid_native_roster_or_policy_cannot_reach_formation() {
    let hopper = Hopper {
        kind: HopperKind::RankedTeams,
        width: 2,
        height: 4,
        depth: 4,
        imbalance: 0,
        flag_enabled: false,
        flag_minimum: 0,
        flag_maximum: 0,
    };
    assert!(team_assignment::assign(hopper, &[16], &[0]).is_none());
    assert!(team_assignment::assign(hopper, &[0], &[]).is_none());
    assert!(team_assignment::assign(hopper, &[0; 17], &[0; 17]).is_none());
    assert!(team_assignment::assign(Hopper { width: 0, ..hopper }, &[0, 1], &[0, 0]).is_none());
}
