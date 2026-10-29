use halo2_online_protocol::search_lifecycle::{
    self as core, GatheringClock, GatheringConfiguration, Object, SearchProgress, SeenSessions,
};
use serde_json::{Value, json};

fn fixture() -> Value {
    let value: Value = serde_json::from_str(include_str!(
        "../../../tests/fixtures/matchmaking_lifecycle_original.json"
    ))
    .unwrap();
    assert_eq!(
        value["original_sha256"],
        "03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d"
    );
    value
}
fn i(value: &Value) -> i32 {
    i32::try_from(value.as_i64().unwrap()).unwrap()
}
fn u(value: &Value) -> u32 {
    u32::try_from(value.as_u64().unwrap()).unwrap()
}

#[test]
fn original_signed_jitter_elapsed_and_role_entry_boundaries() {
    let fixture = fixture();
    for row in fixture["jitter"].as_array().unwrap() {
        let actual = core::jitter(
            u(&row["seed"]),
            i(&row["search_spread"]),
            i(&row["gather_spread"]),
        );
        assert_eq!(
            json!([actual.0, actual.1, actual.2]),
            row["expected"],
            "{row}"
        );
    }
    for row in fixture["elapsed"].as_array().unwrap() {
        assert_eq!(
            core::elapsed(u(&row["now"]), u(&row["start"])),
            i(&row["expected"])
        );
    }
    for row in fixture["roles"].as_array().unwrap() {
        assert_eq!(
            core::role(
                u(&row["now"]),
                u(&row["search_start"]),
                u(&row["gather_start"]),
                i(&row["capacity"])
            ),
            row["expected"]
        );
    }
}

#[test]
fn original_completion_handles_selection_signed_totals_and_casual_expansion() {
    let fixture = fixture();
    for row in fixture["progress"].as_array().unwrap() {
        let mut progress = SearchProgress::new(
            i(&row["maximum"]),
            row["ranked"].as_bool().unwrap(),
            i(&row["radius"]),
            i(&row["total"]),
            true,
        )
        .unwrap();
        let pending: [bool; 4] =
            std::array::from_fn(|index| row["pending"][index].as_bool().unwrap());
        let again = progress.complete(i(&row["count"]), pending).unwrap();
        assert_eq!(
            json!([progress.radius, progress.total, progress.active, again]),
            row["expected"],
            "{row}"
        );
    }
}

#[test]
fn original_growth_clocks_countdown_sentinels_and_timeout_edges() {
    let fixture = fixture();
    for row in fixture["growth"].as_array().unwrap() {
        let mut clock = GatheringClock::new(0, u(&row["start"]), i(&row["peak"])).unwrap();
        let elapsed = clock.observe(u(&row["now"]), i(&row["players"])).unwrap();
        assert_eq!(
            json!([clock.peak, clock.growth, elapsed]),
            row["expected"],
            "{row}"
        );
    }
    let mut configs = Vec::new();
    for row in fixture["hoppers"].as_array().unwrap() {
        let native =
            core::value(&json!({"op":"retail_configuration","index":row["index"]})).unwrap();
        assert_eq!(native[0], row["minimum"]);
        assert_eq!(native[1], row["maximum"]);
        let config = GatheringConfiguration::from_json(&native[2]).unwrap();
        assert_eq!(config.timeout_ms, i(&row["timeout"]));
        for (players, expected) in row["countdown"].as_array().unwrap().iter().enumerate() {
            assert_eq!(
                config
                    .delay(players as i32, i(&row["minimum"]), i(&row["maximum"]))
                    .unwrap(),
                i(expected)
            );
        }
        configs.push((i(&row["minimum"]), i(&row["maximum"]), config));
    }
    for row in fixture["countdown_gates"].as_array().unwrap() {
        let (minimum, maximum, config) = &configs[row["index"].as_u64().unwrap() as usize];
        let players = i(&row["players"]);
        let mut clock = GatheringClock::new(0, 0, players).unwrap();
        assert_eq!(
            clock
                .ready(
                    i(&row["waited"]) as u32,
                    config,
                    players,
                    *minimum,
                    *maximum
                )
                .unwrap(),
            row["expected"]
        );
    }
    for row in fixture["gather_timeouts"].as_array().unwrap() {
        let config = &configs[row["index"].as_u64().unwrap() as usize].2;
        assert_eq!(
            GatheringClock::new(0, 0, 0).unwrap().expired(
                i(&row["waited"]) as u32,
                config.timeout_ms,
                i(&row["offset"])
            ),
            row["expected"]
        );
    }
    for row in fixture["search_timeouts"].as_array().unwrap() {
        assert_eq!(
            core::search_expired(u(&row["now"]), u(&row["start"]), i(&row["offset"])),
            row["expected"]
        );
    }
}

#[test]
fn original_eight_byte_seen_ids_duplicate_saturation_lookup_and_new_search_clear() {
    let fixture = fixture();
    let mut seen = SeenSessions::<u64>::default();
    for row in fixture["seen"].as_array().unwrap() {
        seen.reject(row["identity"].as_u64().unwrap());
        assert_eq!(seen.values().len(), row["count"].as_u64().unwrap() as usize);
        assert_eq!(row["eligible"], 0);
        assert_eq!(row["seen"], 1);
    }
    assert_eq!(json!(seen.values()), fixture["seen_final"]);
    for row in fixture["seen_lookups"].as_array().unwrap() {
        assert_eq!(
            seen.contains(&row["identity"].as_u64().unwrap()),
            row["expected"]
        );
    }
    seen.clear();
    assert_eq!(
        seen.values().len(),
        fixture["new_search_seen_count"].as_u64().unwrap() as usize
    );
}

fn command(state: &mut Object, request: Value) -> Value {
    let (next, result) = state.command(&request).unwrap();
    *state = next;
    result
}

#[test]
fn rust_party_state_preserves_seen_across_radius_queries_and_rejects_stale_views() {
    let mut state =
        Object::new(&json!({"kind":"party","search_offset":-1307,"gather_offset":-1543})).unwrap();
    command(
        &mut state,
        json!({"op":"party_start_search","now":10000,"maximum":2,"ranked":false}),
    );
    command(
        &mut state,
        json!({"op":"seen_reject","generation":1,"identity":"session-a"}),
    );
    assert_eq!(
        command(
            &mut state,
            json!({"op":"party_complete_query","generation":1,"count":0})
        ),
        true
    );
    let before = command(&mut state, json!({"op":"snapshot"}));
    assert_eq!(before["progress"]["radius"], 1);
    assert_eq!(before["seen"], json!(["session-a"]));
    command(&mut state, json!({"op":"party_start_gather","now":11000}));
    command(&mut state, json!({"op":"party_joined"}));
    command(&mut state, json!({"op":"party_choose"}));
    command(
        &mut state,
        json!({"op":"party_start_search","now":12000,"maximum":0,"ranked":true}),
    );
    let current = command(&mut state, json!({"op":"snapshot"}));
    assert_eq!(current["generation"], 2);
    assert_eq!(current["seen"], json!([]));
    assert!(
        state
            .command(&json!({"op":"progress_complete","generation":1,"count":50,
        "query_pending":false,"full_pending":false,"brief_pending":false,"selected":false}))
            .is_err()
    );
    assert!(
        state
            .command(&json!({"op":"seen_reject","generation":1,"identity":"stale"}))
            .is_err()
    );
    assert_eq!(command(&mut state, json!({"op":"snapshot"})), current);
    assert_eq!(
        command(
            &mut state,
            json!({"op":"party_complete_query","generation":2,"count":0})
        ),
        false
    );
    assert_eq!(
        command(&mut state, json!({"op":"snapshot"}))["mode"],
        "choose"
    );
}

#[test]
fn explicit_native_provider_preserves_birth_id_order_and_fifty_record_cap() {
    let input = (1..=60)
        .rev()
        .map(|n| (100 + i64::from(n % 3), format!("{n:03}")))
        .collect();
    let actual = core::native_birth_id_order(input).unwrap();
    let expected = (3..=60)
        .step_by(3)
        .chain((1..=60).step_by(3))
        .chain((2..=30).step_by(3))
        .map(|n| format!("{n:03}"))
        .collect::<Vec<_>>();
    assert_eq!(actual, expected);
    assert_eq!(actual.len(), 50);
    assert!(core::native_birth_id_order(vec![(1, "same".into()), (2, "same".into())]).is_err());
    assert!(core::native_birth_id_order((0..1001).map(|n| (n, n.to_string())).collect()).is_err());
    assert!(
        core::value(&json!({"op":"native_birth_id_order","rows":[{"id":"a","born":true}]}))
            .is_err()
    );
}
