use halo2_online_protocol::matchmaking_frontend_progress::*;

#[test]
fn original_complete_sort_caller_including_signed_keys_and_equal_key_swaps() {
    let fixture: serde_json::Value = serde_json::from_str(include_str!(
        "../../../tests/fixtures/matchmaking_progress_sort_tu15_original.json"
    ))
    .unwrap();
    for (i, case) in fixture["cases"].as_array().unwrap().iter().enumerate() {
        let players = case["players"]
            .as_array()
            .unwrap()
            .iter()
            .map(|p| Player {
                team: p[0].as_i64().unwrap() as i32,
                value: p[1].as_i64().unwrap() as i32,
            })
            .collect::<Vec<_>>();
        let actual = order(
            &players,
            case["ranked"].as_bool().unwrap(),
            case["teams"].as_bool().unwrap(),
        );
        let expected = case["original"]
            .as_array()
            .unwrap()
            .iter()
            .map(|v| v.as_u64().unwrap() as u32)
            .collect::<Vec<_>>();
        assert_eq!(actual, expected, "original sort control {i}: {case}");
    }
}
#[test]
fn original_cancel_clock_and_complete_leave_count_getters() {
    let fixture: serde_json::Value = serde_json::from_str(include_str!(
        "../../../tests/fixtures/matchmaking_progress_input_tu15_original.json"
    ))
    .unwrap();
    for (i, case) in fixture["cases"].as_array().unwrap().iter().enumerate() {
        let actual = if case["kind"] == "cancel" {
            u32::from(cancel_allowed(
                case["pane"].as_u64().unwrap() as u32,
                case["elapsed"].as_u64().unwrap() as u32,
            ))
        } else {
            leave_dialog(
                case["total"].as_u64().unwrap() as usize,
                case["local"].as_u64().unwrap() as usize,
            )
        };
        assert_eq!(
            actual,
            case["original"].as_u64().unwrap() as u32,
            "original input control {i}: {case}"
        );
    }
}

#[test]
fn countdown_observation_never_manufactures_readiness() {
    for ms in [0, 1, 999, 1000, 1001, 9999, 10_000, 10_001, u32::MAX] {
        let (seconds, ready) = start_countdown(ms);
        assert_eq!(ready, ms >= 10_000);
        assert!((0..=10).contains(&seconds));
        if ms <= 10_000 {
            assert_eq!(published_countdown(Some(10_000 - ms)), (seconds, ready));
        }
    }
    assert_eq!(published_countdown(None), (-1, false));
    assert_eq!(published_countdown(Some(10_001)), (-1, false));
}

#[test]
fn roster_hides_ranked_opponents_and_never_gives_placeholders_identity() {
    let mut f = Facts {
        ranked: true,
        pane: 1,
        ui: 4,
        status: 5,
        party_present: true,
        gathering_present: true,
        maximum: 4,
        viewer_level: Some(37),
        party_voice: true,
        voice_joined: true,
        party: vec![Member {
            self_player: true,
            local_controller: -1,
            handle: "self".into(),
            ..Member::default()
        }],
        gathering: vec![Member::default(); 3],
        ..Facts::default()
    };
    let r = project(&f);
    assert_eq!(
        r.iter().map(|r| r.kind).collect::<Vec<_>>(),
        [
            RowKind::Player,
            RowKind::Matched,
            RowKind::Matched,
            RowKind::Empty
        ]
    );
    assert_eq!(
        (r[0].source, r[0].rank, r[0].speaker),
        (Source::Party, Some(36), Some(1))
    );
    for row in &r[1..] {
        assert_eq!(
            (row.source, row.rank, row.speaker),
            (Source::None, None, None)
        );
    }
    f.ranked = false;
    assert_eq!(project(&f)[0].source, Source::Gathering);
    assert!(
        project(&f)
            .iter()
            .all(|r| r.rank.is_none() && r.speaker.is_none())
    );
}

#[test]
fn rank_and_voice_follow_admitted_slot_identity_even_with_duplicate_names() {
    let mut f = Facts {
        ranked: true,
        pane: 0,
        status: 5,
        party_present: true,
        viewer_level: Some(50),
        party_voice: true,
        voice_joined: true,
        talking: vec!["peer".into()],
        party: vec![
            Member {
                self_player: true,
                local_controller: -1,
                handle: "self".into(),
                ..Member::default()
            },
            Member {
                self_player: true,
                local_controller: 1,
                handle: "self".into(),
                ..Member::default()
            },
            Member {
                local_controller: -1,
                handle: "peer".into(),
                ..Member::default()
            },
            Member {
                local_controller: -2,
                handle: "peer".into(),
                ..Member::default()
            },
        ],
        ..Facts::default()
    };
    let r = project(&f);
    assert_eq!(
        r.iter().map(|r| r.rank).collect::<Vec<_>>(),
        [Some(49), None, None, None]
    );
    assert_eq!(
        r.iter().map(|r| r.speaker).collect::<Vec<_>>(),
        [Some(1), None, Some(0), None]
    );
    f.microphone_muted = true;
    f.transmitting = true;
    assert_eq!(project(&f)[0].speaker, Some(2));
    f.viewer_level = Some(51);
    f.voice_joined = false;
    assert!(
        project(&f)
            .iter()
            .all(|r| r.rank.is_none() && r.speaker.is_none())
    );
}

#[test]
fn cancellation_and_auxiliary_obey_original_pane_and_signed_timer_boundary() {
    for p in 0..=4 {
        assert_eq!(cancel_allowed(p, 59_999), p < 2);
        assert!(cancel_allowed(p, 60_000));
        assert_eq!(cancel_allowed(p, u32::MAX), p < 2);
        assert_eq!(consumes_y(p), p >= 2);
    }
    assert_eq!(leave_dialog(3, 2), 0x8c);
    assert_eq!(leave_dialog(2, 2), 0x8d);
}
#[test]
fn out_of_range_native_status_never_aliases_a_visible_rank_state() {
    let mut f = Facts {
        ranked: true,
        pane: 0,
        viewer_level: Some(37),
        local: vec![Member::default()],
        ..Facts::default()
    };
    for status in [0, 3, 9, 255, 256 + 4, 65536 + 5, u32::MAX] {
        f.status = status;
        assert!(project(&f).iter().all(|r| r.rank.is_none()));
    }
}

#[test]
fn original_tip_state_uses_signed_wrapped_elapsed_and_clones_independently() {
    let mut c = TipClock::new(u32::MAX - 10, false);
    assert!(c.update(u32::MAX - 10));
    let mut other = c.clone();
    assert!(!c.update(14_989));
    assert!(c.update(14_990));
    assert!(!other.update(0));
    assert_ne!(c, other);
    let mut seed = 0xffffffff;
    assert_eq!(tip_index(&mut seed, 33), None);
    assert_eq!(seed, 0xffffffff);
    assert_eq!(tip_index(&mut seed, 0), Some(0));
    assert_eq!(seed, 0x3c558d52);
}
