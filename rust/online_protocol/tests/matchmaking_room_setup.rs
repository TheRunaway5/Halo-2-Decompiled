//! Original numerical controls and synthetic native room setup transitions.
use halo2_online_protocol::{
    matchmaking_catalog,
    matchmaking_coordinator::Coordinator,
    matchmaking_model::Result,
    matchmaking_readiness::{self, Seat},
    matchmaking_start::Timer,
};
use serde_json::{Value, json};
#[test]
fn all_unchanged_tu15_start_timer_and_readiness_outputs() {
    let fixture: Value = serde_json::from_str(include_str!(
        "../../../tests/fixtures/tu15_matchmaking_start_original.json"
    ))
    .unwrap();
    assert_eq!(
        fixture["original_xbe_sha256"],
        "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"
    );
    assert_eq!(fixture["all_executed_instruction_bytes_verified"], true);
    assert_eq!(fixture["controls"].as_array().unwrap().len(), 512);
    for (index, row) in fixture["controls"].as_array().unwrap().iter().enumerate() {
        let mut timer = Timer {
            clock: row["saved_clock"].as_u64().unwrap() as u32,
        };
        let output = timer.normal(row["now_ms"].as_u64().unwrap() as u32);
        assert_eq!(
            json!({"stored_clock":timer.clock,"remaining_ms":output.remaining,"seconds":output.seconds as u32,"ready":output.ready}),
            json!({"stored_clock":row["stored_clock"],"remaining_ms":row["remaining_ms"],"seconds":row["seconds"],"ready":row["ready"]}),
            "Original timer {index}: {row}"
        );
    }
    let fixture: Value = serde_json::from_str(include_str!(
        "../../../tests/fixtures/tu15_matchmaking_readiness_original.json"
    ))
    .unwrap();
    for row in fixture["readiness_controls"].as_array().unwrap() {
        let input = &row["inputs"];
        let default = json!([0, 1]);
        let teams = input.get("teams").unwrap_or(&default);
        let peers = [
            3,
            if input.get("second_connected").is_none_or(|v| v == true) {
                3
            } else {
                0
            },
        ];
        let seats = teams
            .as_array()
            .unwrap()
            .iter()
            .enumerate()
            .map(|(i, t)| Seat {
                occupied: true,
                peer: i as u32,
                team: t.as_i64().unwrap() as i32,
            })
            .collect::<Vec<_>>();
        let connected = matchmaking_readiness::connected_teams(
            &peers,
            input["current_peer"].as_u64().unwrap_or(0) as usize,
            &seats,
        );
        let online = input["online"].as_u64().unwrap_or(1) != 0;
        let debug = input["debug_policy"].as_u64().unwrap_or(0);
        let result = matchmaking_readiness::eligible(
            online && input["team_required"].as_u64().unwrap_or(1) != 0,
            connected,
            online && input["local_check"].as_u64().unwrap_or(1) != 0,
            input["logon"].as_u64().unwrap_or(1) == 1,
            input["local_flag"].as_u64().unwrap_or(0) & 0x80000000 != 0,
            debug & 255 != 0,
            debug & 0xff00 != 0,
        );
        assert_eq!(json!(result), row["ready"], "Original readiness {row}");
    }
}
fn owner(direct: bool, teams: bool) -> (Coordinator, Value) {
    owner_protocol(
        direct,
        teams,
        if direct {
            "halo2-native-queue-v12"
        } else {
            "test-v1"
        },
    )
}
fn owner_protocol(_direct: bool, teams: bool, protocol: &str) -> (Coordinator, Value) {
    owner_count(teams, protocol, 2)
}
fn owner_count(teams: bool, protocol: &str, count: usize) -> (Coordinator, Value) {
    let entries=matchmaking_catalog::parse(json!({"version":1,"playlists":[{"id":"native","revision":"v1","name":"Native setup test","description":"Declared native room policy","ranked":false,"quickmatch":true,
        "team_size":if teams{1}else{0},"minimum_players":count,"maximum_players":count,"maximum_party":1,"games":[{"id":"slayer","map":"lockout","variant":if teams{"Team Slayer"}else{"Slayer"},"weight":1}],
        "stages":[{"after_seconds":0,"rank_numerator":1,"rank_denominator":1,"minimum_connection":50}],"skill":{"kind":"custom","radius":6},"provenance":"Native synthetic setup input"}]}).to_string().as_bytes()).unwrap();
    let mut c=Coordinator::new(&json!({"catalog":entries,"playlists":entries.iter().map(|e|e["policy"].clone()).collect::<Vec<_>>(),"lease":90,"proposal_lease":30,"qos_lease":30,"capacity":1000,"search_budget":10000,"draw_seed":2,"native_seed":3})).unwrap();
    let mut tickets = Vec::new();
    for who in (0..count).map(|i| {
        if count == 2 {
            ["a", "b"][i].to_owned()
        } else {
            format!("p{i:02}")
        }
    }) {
        let t=send(&mut c,json!({"op":"enqueue","party_id":who,"playlist_id":"native","compatibility":protocol,"clan":null,"clan_rank":null,"leader":who,"language":0,
            "players":[{"id":who,"screen_name":"Same Pilot","rank":1,"raw_rank":null,"maps":["lockout"],"guest":false,"connection_owner":who}]})).unwrap();
        tickets.push(t["id"].clone());
    }
    let id = send(
        &mut c,
        json!({"op":"form","playlist":"native","tickets":tickets,"game":null,"gathering_host":if count==2{"a"}else{"p00"}}),
    )
    .unwrap()["id"]
        .clone();
    send(&mut c, json!({"op":"api_room_admit","id":id})).unwrap();
    (c, id)
}
#[test]
fn continuation_is_installed_only_by_zero_gate_from_immutable_admission() {
    let (mut c, id) = owner_protocol(false, false, "halo2-native-queue-v11");
    assert!(send(&mut c, json!({"op":"room_continuity_snapshot","id":id})).is_err());
    samples(&mut c, &id);
    step(&mut c, &id, 1000);
    loaded(&mut c, &id);
    assert!(step(&mut c, &id, 10999)["transition"].is_null());
    assert!(send(&mut c, json!({"op":"room_continuity_snapshot","id":id})).is_err());
    assert_eq!(step(&mut c, &id, 11000)["continuity"], true);
    let snapshot = send(&mut c, json!({"op":"room_continuity_snapshot","id":id})).unwrap();
    assert_eq!(snapshot["owners"], json!([0, 1]));
    let host = snapshot["host"].as_u64().unwrap() as usize;
    let who = ["a", "b"][host];
    let data = json!({"world_seal":{"epoch":0,"tick":u64::MAX,"digest":"a".repeat(64)}});
    assert_eq!(
        send(
            &mut c,
            json!({"op":"room_continuity_message","id":id,"player":"outsider","data":data})
        )
        .unwrap_err()
        .0,
        403
    );
    let effect = send(
        &mut c,
        json!({"op":"room_continuity_message","id":id,"player":who,"data":data}),
    )
    .unwrap();
    assert_eq!(effect["broadcast"], data);
    let other = 1 - host;
    let ack = json!({"world_ack":{"epoch":0,"tick":u64::MAX,"digest":"a".repeat(64)}});
    send(
        &mut c,
        json!({"op":"room_continuity_message","id":id,"player":(["a","b"][other]),"data":ack}),
    )
    .unwrap();
    send(
        &mut c,
        json!({"op":"room_continuity_disconnect","id":id,"peer":host}),
    )
    .unwrap();
    let snapshot = send(&mut c, json!({"op":"room_continuity_snapshot","id":id})).unwrap();
    assert_eq!(snapshot["epoch"], 1);
    let ignored = send(
        &mut c,
        json!({"op":"room_continuity_message","id":id,"player":(["a","b"][other]),"data":ack}),
    )
    .unwrap();
    assert_eq!(
        ignored,
        json!({"handled":true,"broadcast":null,"changed":false})
    );
    for op in [
        "room_continuity_restore",
        "room_continuity_open",
        "room_continuity_install",
    ] {
        assert!(
            send(
                &mut c,
                json!({"op":op,"id":id,"peer":other,"value":snapshot})
            )
            .is_err()
        );
    }
    send(&mut c, json!({"op":"room_setup_close","id":id})).unwrap();
    assert!(send(&mut c, json!({"op":"room_continuity_snapshot","id":id})).is_err());
}
fn send(c: &mut Coordinator, mut r: Value) -> Result<Value> {
    r["now"] = json!(100.);
    c.execute(&r)
}
fn observation(id: &Value, op: &str, clock: u32, connected: Value) -> Value {
    json!({"op":op,"id":id,"active":["a","b"],"connected":connected,"phase":"loading","closed":false,"clock_ms":clock})
}
fn step(c: &mut Coordinator, id: &Value, clock: u32) -> Value {
    send(
        c,
        observation(id, "room_setup_advance", clock, json!(["a", "b"])),
    )
    .unwrap()
}
fn samples(c: &mut Coordinator, id: &Value) {
    for who in ["a", "b"] {
        for milliseconds in [30., 20., 10.] {
            send(
                c,
                json!({"op":"room_setup_sample","id":id,"player":who,"milliseconds":milliseconds}),
            )
            .unwrap();
        }
    }
}
fn loaded(c: &mut Coordinator, id: &Value) {
    for who in ["a", "b"] {
        send(c,json!({"op":"room_setup_loaded","id":id,"player":who,"signature":"1234567890abcdef:000001a"})).unwrap();
    }
}
#[test]
fn relay_scores_and_atomic_loading_handoff_wait_for_loader_and_source_zero() {
    let (mut c, id) = owner(false, true);
    samples(&mut c, &id);
    assert_eq!(
        send(
            &mut c,
            observation(&id, "room_setup_advance", 1000, json!(["a"]))
        )
        .unwrap()["transition"],
        Value::Null
    );
    let score = send(
        &mut c,
        json!({"op":"get","map":"connections","key":["a","b"]}),
    )
    .unwrap();
    assert_eq!(score[0], 92);
    let effect = step(&mut c, &id, 1000);
    assert_eq!(effect["transition"], "loading");
    assert_eq!(effect["timeout_seconds"], 180);
    assert_eq!(
        send(&mut c, json!({"op":"len","map":"tickets"})).unwrap(),
        0
    );
    assert_eq!(
        send(&mut c, json!({"op":"room_setup_clock","id":id})).unwrap(),
        1000
    );
    loaded(&mut c, &id);
    assert!(step(&mut c, &id, 6000)["transition"].is_null());
    assert_eq!(
        send(
            &mut c,
            observation(&id, "room_setup_start", 6000, json!(["a", "b"]))
        )
        .unwrap(),
        5000
    );
    assert!(step(&mut c, &id, 10999)["transition"].is_null());
    let effect = step(&mut c, &id, 11000);
    assert_eq!(effect["transition"], "running");
    assert_eq!(effect["timeout_seconds"], 7200);
    assert!(
        step(&mut c, &id, 11000)["transition"].is_null(),
        "Publication is not repeated"
    );
}
#[test]
fn socket_readiness_loss_restarts_the_shared_source_clock_and_frozen_roster_is_owned() {
    let (mut c, id) = owner(false, true);
    samples(&mut c, &id);
    step(&mut c, &id, 1000);
    loaded(&mut c, &id);
    assert!(
        send(
            &mut c,
            observation(&id, "room_setup_start", 6000, json!(["a"]))
        )
        .unwrap()
        .is_null()
    );
    assert!(
        send(&mut c, json!({"op":"room_setup_clock","id":id}))
            .unwrap()
            .is_null()
    );
    assert_eq!(
        send(
            &mut c,
            observation(&id, "room_setup_start", 7000, json!(["a", "b"]))
        )
        .unwrap(),
        10000
    );
    assert!(step(&mut c, &id, 16999)["transition"].is_null());
    assert_eq!(step(&mut c, &id, 17000)["transition"], "running");
    assert_eq!(
        send(
            &mut c,
            json!({"op":"room_setup_restore","id":id,"value":{}})
        )
        .unwrap_err()
        .0,
        400
    );
}
#[test]
fn direct_host_upload_ranking_is_physical_reciprocal_and_never_downgrades() {
    for complete in [true, false] {
        let (mut c, id) = owner(true, false);
        samples(&mut c, &id);
        for (who, target, upload) in [("a", 1, 100000), ("b", 0, 2000000)] {
            let mut row = json!({"peer":target,"rtt_ms":30});
            if who == "a" || complete {
                row["upload_bytes_per_second"] = json!(upload);
                row["loss_per_mille"] = json!(0);
            }
            send(
                &mut c,
                json!({"op":"room_setup_direct_report","id":id,"player":who,"rows":[row]}),
            )
            .unwrap();
        }
        let effect = step(&mut c, &id, 1000);
        if complete {
            assert_eq!(effect["transition"], "loading");
            assert_eq!(effect["host"], 1);
        } else {
            assert!(effect["transition"].is_null());
            assert!(effect["close"].is_string());
        }
    }
}
#[test]
fn malformed_or_foreign_observations_do_not_partly_admit_direct_quality() {
    let (mut c, id) = owner(true, false);
    for rows in [
        json!([{"peer":1,"rtt_ms":20,"upload_bytes_per_second":true,"loss_per_mille":0}]),
        json!([{"peer":0,"rtt_ms":20}]),
        json!([{"peer":1,"rtt_ms":20,"loss_per_mille":0}]),
        json!([]),
    ] {
        assert!(
            send(
                &mut c,
                json!({"op":"room_setup_direct_report","id":id,"player":"a","rows":rows})
            )
            .is_err()
        );
        assert_eq!(
            send(&mut c, json!({"op":"room_setup_snapshot","id":id})).unwrap()["direct_ready"],
            json!({})
        );
    }
    assert_eq!(
        send(
            &mut c,
            json!({"op":"room_setup_sample","id":id,"player":"guest","milliseconds":0})
        )
        .unwrap_err()
        .0,
        403
    );
    assert!(send(&mut c,json!({"op":"room_setup_loaded","id":id,"player":"a","signature":"1234567890abcdef:000001a"})).is_err());
    send(&mut c, json!({"op":"room_setup_close","id":id})).unwrap();
    assert!(
        send(&mut c, json!({"op":"room_setup_clock","id":id}))
            .unwrap()
            .is_null()
    );
    assert!(
        send(
            &mut c,
            json!({"op":"room_setup_sample","id":id,"player":"a","milliseconds":1})
        )
        .is_err()
    );
}

fn result_admission(c: &mut Coordinator, id: &Value) {
    send(c,json!({"op":"result_open","id":id,"context":{"proposal":id,"skill_configuration":null,"clan_configuration":null,
        "skills":{},"clans":{},"clan_skills":{},"created_wall":123}})).unwrap();
}
fn lifecycle(c: &mut Coordinator, id: &Value, op: &str, extra: Value) -> Result<Value> {
    let mut r = extra;
    r["op"] = json!(op);
    r["id"] = id.clone();
    send(c, r)
}
#[test]
fn room_deadlines_and_phase_cannot_be_replaced_by_caller_projections() {
    let (mut c, id) = owner(false, false);
    let initial = lifecycle(&mut c, &id, "room_state_get", json!({})).unwrap();
    assert_eq!(initial["phase"], "connecting");
    assert_eq!(initial["deadline"], 125.);
    samples(&mut c, &id);
    step(&mut c, &id, 1000);
    assert_eq!(
        lifecycle(&mut c, &id, "room_state_get", json!({})).unwrap()["deadline"],
        280.
    );
    loaded(&mut c, &id);
    step(&mut c, &id, 11000);
    let state = lifecycle(&mut c, &id, "room_state_get", json!({})).unwrap();
    assert_eq!(state["phase"], "running");
    assert_eq!(state["deadline"], 7300.);
    for op in ["room_state_restore", "room_state_install", "room_state_set"] {
        assert!(
            lifecycle(
                &mut c,
                &id,
                op,
                json!({"phase":"postgame","host":9,"closed":true,"deadline":0})
            )
            .is_err()
        );
        assert_eq!(
            lifecycle(&mut c, &id, "room_state_get", json!({})).unwrap(),
            state
        );
    }
    assert_eq!(
        lifecycle(&mut c, &id, "room_state_expired", json!({"io_now":7300.})).unwrap(),
        false
    );
    assert_eq!(
        lifecycle(
            &mut c,
            &id,
            "room_state_expired",
            json!({"io_now":7300.001})
        )
        .unwrap(),
        true
    );
    // A regressing IO observation cannot revive the expired room.
    assert_eq!(
        lifecycle(&mut c, &id, "room_state_expired", json!({"io_now":100.})).unwrap(),
        true
    );
}
#[test]
fn report_authority_is_native_before_report_validation_and_socket_loss_is_not_direct_departure() {
    let (mut c, id) = owner(true, false);
    result_admission(&mut c, &id);
    samples(&mut c, &id);
    for (who, other) in [("a", 1), ("b", 0)] {
        send(&mut c,json!({"op":"room_setup_direct_report","id":id,"player":who,"rows":[{"peer":other,"rtt_ms":20}]})).unwrap();
    }
    step(&mut c, &id, 1000);
    loaded(&mut c, &id);
    step(&mut c, &id, 11000);
    let state = lifecycle(&mut c, &id, "room_state_get", json!({})).unwrap();
    let host = state["host"].as_u64().unwrap() as usize;
    let other = 1 - host;
    let peers = json!([{"player":"a","native_handoff":false,"departure_game_over":true},{"player":"b","native_handoff":false,"departure_game_over":true}]);
    let extra = json!({"player":(["a","b"][other]),"report":"not a PCR","peers":peers,"host":other,"phase":"running"});
    assert_eq!(
        lifecycle(&mut c, &id, "room_state_submit", extra)
            .unwrap_err()
            .0,
        403
    );
    assert_eq!(
        lifecycle(&mut c, &id, "room_state_get", json!({})).unwrap(),
        state
    );
    let lost = lifecycle(
        &mut c,
        &id,
        "room_state_socket_lost",
        json!({"player":(["a","b"][other]),"peers":peers}),
    )
    .unwrap();
    assert!(lost["close"].is_null());
    assert_eq!(lost["retire"], json!([]));
    assert_eq!(
        lifecycle(&mut c, &id, "room_state_get", json!({})).unwrap(),
        state
    );
    let leave = lifecycle(
        &mut c,
        &id,
        "room_state_depart",
        json!({"player":(["a","b"][host]),"peers":peers,"direct":true}),
    )
    .unwrap();
    assert!(leave["close"].is_null());
    assert_eq!(leave["notice"]["departed_players"], 1u32 << host);
    let after = lifecycle(&mut c, &id, "room_state_get", json!({})).unwrap();
    assert_eq!(after["phase"], "running");
    assert_eq!(after["host"], host);
    assert_eq!(after["active"], json!([(["a", "b"][other])]));
    assert_eq!(
        after["setup_host_departed"], false,
        "Last-opponent GAME OVER does not grant negotiated host authority"
    );
}
#[test]
fn closing_uses_frozen_ticket_ids_and_cannot_cancel_an_unrelated_search() {
    let (mut c, id) = owner(false, false);
    let ticket=send(&mut c,json!({"op":"enqueue","party_id":"foreign","playlist_id":"native","compatibility":"test-v1","clan":null,"clan_rank":null,"leader":"c","language":0,
        "players":[{"id":"c","screen_name":"Other Pilot","rank":1,"raw_rank":null,"maps":["lockout"],"guest":false,"connection_owner":"c"}]})).unwrap();
    let closed = lifecycle(
        &mut c,
        &id,
        "room_state_close",
        json!({"tickets":[ticket["id"]]}),
    )
    .unwrap();
    assert_eq!(closed["closed"], true);
    assert_eq!(
        send(&mut c, json!({"op":"len","map":"tickets"})).unwrap(),
        1
    );
    assert_eq!(
        send(
            &mut c,
            json!({"op":"get","map":"tickets","key":ticket["id"]})
        )
        .unwrap()["id"],
        ticket["id"]
    );
}

fn wire(c: &mut Coordinator, id: &Value, op: &str, mut r: Value) -> Result<Value> {
    r["op"] = json!(op);
    r["id"] = id.clone();
    if r.get("peers").is_none() {
        r["peers"] = json!([]);
    }
    send(c, r)
}
fn bearer(c: &mut Coordinator, id: &Value, who: &str) -> String {
    format!(
        "Bearer {}",
        wire(c, id, "room_wire_snapshot", json!({})).unwrap()["tokens"][who]
            .as_str()
            .unwrap()
    )
}
fn attach(c: &mut Coordinator, id: &Value, who: &str) -> Value {
    let authorization = bearer(c, id, who);
    wire(c,id,"room_wire_attach",json!({"route":"signal","authorization":authorization,"prior":null,"completion":"1","native_handoff":"1","departure_game_over":null})).unwrap()
}
#[test]
fn wire_auth_rotation_expiry_and_prepare_race_are_owned_generations() {
    let (mut c, id) = owner(true, false);
    result_admission(&mut c, &id);
    let header = bearer(&mut c, &id, "a");
    let auth = wire(
        &mut c,
        &id,
        "room_wire_authorize",
        json!({"route":"signal","authorization":header,"prior":null}),
    )
    .unwrap();
    let request = json!({"route":"signal","authorization":header,"prior":auth});
    let first = wire(&mut c, &id, "room_wire_attach", request.clone()).unwrap();
    assert_eq!(
        wire(&mut c, &id, "room_wire_attach", request)
            .unwrap_err()
            .0,
        409,
        "Two preflight grants cannot both attach a connecting peer"
    );
    assert_eq!(
        wire(
            &mut c,
            &id,
            "room_wire_detach",
            json!({"player":"a","connection":first["connection"]})
        )
        .unwrap(),
        true
    );
    let second = attach(&mut c, &id, "a");
    assert_ne!(first["connection"], second["connection"]);
    assert_eq!(
        wire(
            &mut c,
            &id,
            "room_wire_detach",
            json!({"player":"a","connection":first["connection"]})
        )
        .unwrap(),
        false
    );
    assert_eq!(
        wire(
            &mut c,
            &id,
            "room_wire_current",
            json!({"player":"a","connection":second["connection"]})
        )
        .unwrap(),
        true
    );
    let before = wire(&mut c, &id, "room_wire_snapshot", json!({})).unwrap();
    for op in ["room_wire_restore", "room_wire_install"] {
        assert!(wire(&mut c, &id, op, json!({"value":before})).is_err());
    }
    assert_eq!(
        wire(&mut c, &id, "room_wire_snapshot", json!({})).unwrap(),
        before
    );
    wire(&mut c, &id, "room_wire_rotate", json!({"player":"a"})).unwrap();
    assert_eq!(
        wire(
            &mut c,
            &id,
            "room_wire_authorize",
            json!({"route":"signal","authorization":header,"prior":null})
        )
        .unwrap_err()
        .0,
        401
    );
    let new_header = bearer(&mut c, &id, "a");
    assert_eq!(
        wire(
            &mut c,
            &id,
            "room_wire_authorize",
            json!({"route":"signal","authorization":new_header,"prior":auth})
        )
        .unwrap_err()
        .0,
        401
    );
    assert_eq!(
        wire(
            &mut c,
            &id,
            "room_wire_authorize",
            json!({"route":"signal","authorization":new_header,"prior":null,"io_now":126.})
        )
        .unwrap_err()
        .0,
        404
    );
}
#[test]
fn wire_signaling_is_atomic_bounded_and_replacement_fenced() {
    let (mut c, id) = owner(true, false);
    result_admission(&mut c, &id);
    let a = attach(&mut c, &id, "a");
    let grant = a["connection"].clone();
    let signal =
        |kind: &str, payload: &str| json!({"peer":1,"type":kind,"payload":payload,"mid":""});
    let before = wire(&mut c, &id, "room_wire_snapshot", json!({})).unwrap();
    for malformed in [
        json!({"peer":true,"type":"offer","payload":"x","mid":""}),
        json!({"peer":1,"type":"offer","payload":"x","mid":"","source":0}),
        signal("answer", "x"),
    ] {
        assert!(
            wire(
                &mut c,
                &id,
                "room_wire_signal",
                json!({"player":"a","connection":grant,"signal":malformed})
            )
            .is_err()
        );
        assert_eq!(
            wire(&mut c, &id, "room_wire_snapshot", json!({})).unwrap(),
            before
        );
    }
    let offer = wire(
        &mut c,
        &id,
        "room_wire_signal",
        json!({"player":"a","connection":grant,"signal":signal("offer","fixture SDP\r\n")}),
    )
    .unwrap();
    assert!(offer["deliver"].is_null());
    let b = attach(&mut c, &id, "b");
    assert_eq!(b["publications"].as_array().unwrap().len(), 1);
    let pending = wire(
        &mut c,
        &id,
        "room_wire_drain",
        json!({"player":"b","connection":b["connection"]}),
    )
    .unwrap();
    assert_eq!(pending[0]["peer_signal"]["peer"], 0);
    assert_eq!(
        wire(
            &mut c,
            &id,
            "room_wire_drain",
            json!({"player":"b","connection":b["connection"]})
        )
        .unwrap(),
        json!([])
    );
    let peers = json!([{"player":"a"},{"player":"b"}]);
    for _ in 0..64 {
        assert_eq!(wire(&mut c,&id,"room_wire_signal",json!({"player":"a","connection":grant,"peers":peers,"signal":signal("candidate","candidate:fixture")})).unwrap()["deliver"],"b");
    }
    let before = wire(&mut c, &id, "room_wire_snapshot", json!({})).unwrap();
    assert!(wire(&mut c,&id,"room_wire_signal",json!({"player":"a","connection":grant,"peers":peers,"signal":signal("candidate","over quota")})).is_err());
    assert_eq!(
        wire(&mut c, &id, "room_wire_snapshot", json!({})).unwrap(),
        before
    );
    assert!(
        wire(
            &mut c,
            &id,
            "room_wire_signal",
            json!({"player":"a","connection":999,"signal":signal("candidate","old connection")})
        )
        .is_err()
    );
    assert_eq!(
        wire(&mut c, &id, "room_wire_snapshot", json!({})).unwrap(),
        before
    );
}
#[test]
fn wire_eos_proof_fences_precede_signaling_and_descriptor_is_admission_bound() {
    let (mut c, id) = owner_protocol(true, false, "halo2-native-queue-v14");
    result_admission(&mut c, &id);
    let a = attach(&mut c, &id, "a");
    let header = bearer(&mut c, &id, "a");
    let auth = wire(
        &mut c,
        &id,
        "room_wire_authorize",
        json!({"route":"eos","authorization":header,"prior":null}),
    )
    .unwrap();
    let signal = json!({"peer":1,"type":"eos","payload":"1".repeat(32),"mid":""});
    assert_eq!(
        wire(
            &mut c,
            &id,
            "room_wire_signal",
            json!({"player":"a","connection":a["connection"],"signal":signal})
        )
        .unwrap_err()
        .0,
        403
    );
    let proof = json!({"route":"eos","authorization":header,"prior":auth,"account":"a","puid":"1".repeat(32)});
    wire(&mut c, &id, "room_wire_proof_validate", proof.clone()).unwrap();
    assert_eq!(
        wire(&mut c, &id, "room_wire_snapshot", json!({})).unwrap()["eos_verified"],
        json!({})
    );
    wire(&mut c, &id, "room_wire_proof_commit", proof).unwrap();
    wire(
        &mut c,
        &id,
        "room_wire_signal",
        json!({"player":"a","connection":a["connection"],"signal":signal}),
    )
    .unwrap();
    let descriptor = wire(
        &mut c,
        &id,
        "room_wire_descriptor",
        json!({"player":"a","stun_servers":[]}),
    )
    .unwrap();
    assert_eq!(descriptor["slot"], 0);
    assert_eq!(descriptor["players"][0]["name"], "Same Pilot");
    assert_eq!(descriptor["transport"]["kind"], "eos-p2p-v1");
    assert_eq!(descriptor["transport"]["session"]["map"], "lockout");
    assert_eq!(
        descriptor["transport"]["peers"].as_array().unwrap().len(),
        1
    );
    let bheader = bearer(&mut c, &id, "b");
    assert_eq!(wire(&mut c,&id,"room_wire_proof_commit",json!({"route":"eos","authorization":bheader,"prior":null,"account":"b","puid":"1".repeat(32)})).unwrap_err().0,409);
}

#[test]
fn full_room_pending_signals_have_bounded_replies_and_retired_senders_never_drain() {
    let (mut c, id) = owner_count(false, "halo2-native-queue-v12", 16);
    result_admission(&mut c, &id);
    let proposal = send(&mut c, json!({"op":"get","map":"proposals","key":id})).unwrap();
    let owners = proposal["teams"]
        .as_array()
        .unwrap()
        .iter()
        .flat_map(|team| team.as_array().unwrap())
        .map(|id| id.as_str().unwrap().to_owned())
        .collect::<Vec<_>>();
    let destination = &owners[15];
    for who in &owners[..15] {
        let connection = attach(&mut c, &id, who)["connection"].clone();
        wire(&mut c,&id,"room_wire_signal",json!({"player":who,"connection":connection,"signal":{"peer":15,"type":"offer","payload":"\0".repeat(16384),"mid":""}})).unwrap();
    }
    let metadata = wire(&mut c, &id, "room_wire_snapshot", json!({})).unwrap();
    assert_eq!(metadata["pending_signals"][destination], 15);
    assert!(
        metadata.to_string().len() < 65536,
        "Snapshots carry counts, never pending payload copies"
    );
    lifecycle(
        &mut c,
        &id,
        "room_state_retire",
        json!({"player":owners[0]}),
    )
    .unwrap();
    let a = attach(&mut c, &id, destination);
    assert_eq!(a["publications"].as_array().unwrap().len(), 1);
    let frames = wire(
        &mut c,
        &id,
        "room_wire_drain",
        json!({"player":destination,"connection":a["connection"]}),
    )
    .unwrap();
    assert_eq!(frames.as_array().unwrap().len(), 14);
    assert!(
        frames
            .as_array()
            .unwrap()
            .iter()
            .all(|p| p["peer_signal"]["peer"] != 0)
    );
    assert!(
        frames.to_string().len() < 2 * 1024 * 1024 - 1024,
        "Worst-case escaped payloads fit the private response envelope"
    );
    assert_eq!(
        wire(
            &mut c,
            &id,
            "room_wire_drain",
            json!({"player":destination,"connection":a["connection"]})
        )
        .unwrap(),
        json!([])
    );
}

// These fixtures use the public production queue/setup/wire path. They never
// install packet state or replace a live connection's budget/key/roster.
fn packet_room(protocol: &str) -> (Coordinator, Value, Vec<Value>, usize) {
    let (mut c, id) = owner_protocol(false, false, protocol);
    let mut attached = Vec::new();
    for who in ["a", "b"] {
        let token = bearer(&mut c, &id, who);
        attached.push(
            wire(
                &mut c,
                &id,
                "room_wire_attach",
                json!({"route":"relay","authorization":token,"prior":null}),
            )
            .unwrap(),
        );
    }
    samples(&mut c, &id);
    step(&mut c, &id, 1000);
    loaded(&mut c, &id);
    let host = step(&mut c, &id, 11000)["host"].as_u64().unwrap() as usize;
    (c, id, attached, host)
}
fn packet_request(id: &Value, attached: &[Value], source: usize, packet: Vec<u8>) -> Value {
    json!({"op":"room_packets_route","id":id,"player":(["a","b"][source]),"connection":attached[source]["connection"],
        "packet":packet.iter().map(|b|format!("{b:02x}")).collect::<String>(),"packet_now":0.,"peers":[{"player":"a"},{"player":"b"}],"full":[]})
}
fn game_packet(target: usize, kind: u8) -> Vec<u8> {
    let mut p = vec![target as u8];
    p.extend_from_slice(b"H2N2\x1a\x00");
    p.push(kind);
    p
}
fn facts(plan: &Value, allowed: bool) -> Value {
    json!(
        plan["pairs"]
            .as_array()
            .unwrap()
            .iter()
            .map(|p| json!({"a":p[0],"b":p[1],"allowed":allowed}))
            .collect::<Vec<_>>()
    )
}
fn signed_audio(id: &Value, attached: &Value, source: usize, target: usize, epoch: u32) -> Vec<u8> {
    use ed25519_dalek::{Signer, SigningKey};
    let seed = halo2_online_protocol::matchmaking_model::hex(
        &attached["voice_auth"]["voice_auth"]["seed"],
        32,
    )
    .unwrap();
    let key = SigningKey::from_bytes(&seed.try_into().unwrap());
    let mut body = b"H2N2\x1a\x00\x06\x00".to_vec();
    body.push(source as u8);
    body.extend_from_slice(&[0; 14]);
    let mut message = b"H2VOICE2\0".to_vec();
    message.extend_from_slice(id.as_str().unwrap().as_bytes());
    message.extend_from_slice(&epoch.to_le_bytes());
    message.push(target as u8);
    message.extend_from_slice(&body);
    body.extend_from_slice(&key.sign(&message).to_bytes());
    let mut p = vec![target as u8];
    p.extend_from_slice(&body);
    p
}
#[test]
fn relay_protocol_roles_datagram_boundaries_and_connection_budget_are_native() {
    let (mut c, id, a, host) = packet_room("test-v1");
    let source = 1 - host;
    let request = packet_request(&id, &a, source, game_packet(host, 2));
    let effect = send(&mut c, request.clone()).unwrap();
    assert_eq!(effect["kind"], "route");
    assert_eq!(effect["deliver"], ["a", "b"][host]);
    assert!(
        effect["packet"]
            .as_str()
            .unwrap()
            .starts_with(&format!("{source:02x}"))
    );
    let mut wrong = game_packet(host, 2);
    wrong[5] = 0x1b;
    assert_eq!(
        send(&mut c, packet_request(&id, &a, source, wrong))
            .unwrap_err()
            .0,
        400
    );
    assert_eq!(
        send(
            &mut c,
            packet_request(&id, &a, source, game_packet(host, 3))
        )
        .unwrap_err()
        .0,
        400
    );
    let mut max = game_packet(host, 2);
    max.resize(1401, 0);
    send(&mut c, packet_request(&id, &a, source, max.clone())).unwrap();
    max.push(0);
    assert_eq!(
        send(&mut c, packet_request(&id, &a, source, max))
            .unwrap_err()
            .0,
        400
    );
    let mut stale = request.clone();
    stale["connection"] = json!(999);
    assert_eq!(send(&mut c, stale).unwrap_err().0, 409);
    for _ in 0..510 {
        send(&mut c, request.clone()).unwrap();
    }
    assert_eq!(send(&mut c, request).unwrap_err().0, 429);
    // Exhaustion does not consume the independent control bucket.
    send(&mut c,json!({"op":"room_packets_control","id":id,"player":(["a","b"][source]),"connection":a[source]["connection"],"packet_now":0.})).unwrap();
    for op in [
        "room_packets_restore",
        "room_packets_install",
        "room_packets_budget_reset",
    ] {
        assert_eq!(send(&mut c,json!({"op":op,"id":id,"player":(["a","b"][source]),"connection":a[source]["connection"],"value":{}})).unwrap_err().0,400);
    }
}
#[test]
fn voice_proof_binds_frozen_owner_room_recipient_epoch_and_exact_body() {
    let (mut c, id, a, host) = packet_room("test-v1");
    let source = 1 - host;
    assert_ne!(
        a[0]["voice_auth"]["voice_auth"]["seed"],
        a[1]["voice_auth"]["voice_auth"]["seed"]
    );
    for bad in [
        signed_audio(&id, &a[source], source, source, 0),
        signed_audio(&json!("other-room"), &a[source], source, host, 0),
        signed_audio(&id, &a[source], source, host, 1),
        signed_audio(&id, &a[host], source, host, 0),
    ] {
        let mut bad = bad;
        bad[0] = host as u8;
        assert_eq!(
            send(&mut c, packet_request(&id, &a, source, bad))
                .unwrap_err()
                .0,
            400
        );
    }
    let packet = signed_audio(&id, &a[source], source, host, 0);
    let mut request = packet_request(&id, &a, source, packet.clone());
    let plan = send(&mut c, request.clone()).unwrap();
    assert_eq!(plan["kind"], "privacy");
    request["facts"] = facts(&plan, true);
    assert_eq!(send(&mut c, request.clone()).unwrap()["kind"], "route");
    for at in [10, packet.len() - 1] {
        let mut bad = packet.clone();
        bad[at] ^= 1;
        let mut r = request.clone();
        r["packet"] = json!(bad.iter().map(|b| format!("{b:02x}")).collect::<String>());
        assert_eq!(send(&mut c, r).unwrap_err().0, 400);
    }
}
#[test]
fn privacy_plans_do_not_charge_twice_and_denied_audio_still_has_a_finite_budget() {
    let (mut c, id, a, host) = packet_room("test-v1");
    let source = 1 - host;
    let mut r = packet_request(
        &id,
        &a,
        source,
        signed_audio(&id, &a[source], source, host, 0),
    );
    let plan = send(&mut c, r.clone()).unwrap();
    for _ in 0..76 {
        assert_eq!(send(&mut c, r.clone()).unwrap(), plan);
    }
    r["facts"] = json!([]);
    assert_eq!(send(&mut c, r.clone()).unwrap_err().0, 400);
    r["facts"] = json!([{"a":"outsider","b":"b","allowed":true}]);
    assert_eq!(send(&mut c, r.clone()).unwrap_err().0, 400);
    r["facts"] = facts(&plan, false);
    for _ in 0..75 {
        assert_eq!(send(&mut c, r.clone()).unwrap()["kind"], "drop");
    }
    assert_eq!(send(&mut c, r).unwrap_err().0, 429);
}
#[test]
fn queued_delivery_rechecks_privacy_and_both_native_connection_generations() {
    let (mut c, id, a, host) = packet_room("test-v1");
    let target = 1 - host;
    let mut r = packet_request(&id, &a, host, signed_audio(&id, &a[host], host, target, 0));
    let plan = send(&mut c, r.clone()).unwrap();
    r["facts"] = facts(&plan, true);
    let effect = send(&mut c, r).unwrap();
    assert_eq!(effect["kind"], "route");
    let mut delivery = json!({"op":"room_packets_delivery","id":id,"player":(["a","b"][target]),"connection":a[target]["connection"],
        "packet":effect["packet"],"target":target,"origin_connection":effect["origin_connection"],"recipient_connection":effect["recipient_connection"]});
    let plan = send(&mut c, delivery.clone()).unwrap();
    delivery["facts"] = facts(&plan, false);
    assert_eq!(send(&mut c, delivery.clone()).unwrap()["kind"], "drop");
    delivery["facts"] = facts(&plan, true);
    assert_eq!(send(&mut c, delivery.clone()).unwrap()["kind"], "send");
    let mut bad = delivery.clone();
    bad["recipient_connection"] = json!(999);
    assert_eq!(send(&mut c, bad).unwrap()["kind"], "drop");
    wire(
        &mut c,
        &id,
        "room_wire_detach",
        json!({"player":(["a","b"][host]),"connection":a[host]["connection"]}),
    )
    .unwrap();
    let token = bearer(&mut c, &id, ["a", "b"][host]);
    let next = wire(
        &mut c,
        &id,
        "room_wire_attach",
        json!({"route":"relay","authorization":token,"prior":null}),
    )
    .unwrap();
    assert_ne!(next["connection"], a[host]["connection"]);
    assert_eq!(send(&mut c, delivery).unwrap()["kind"], "drop");
}

#[test]
fn continuity_packet_epochs_are_fenced_before_routing_and_stalled_readers_are_bounded() {
    let (mut c, id, a, host) = packet_room("halo2-native-queue-v11");
    let source = 1 - host;
    let mut packet = game_packet(host, 2);
    packet.splice(1..1, [0; 4]);
    let mut r = packet_request(&id, &a, source, packet);
    r["peers"] = json!([]);
    assert_eq!(send(&mut c, r.clone()).unwrap()["kind"], "drop");
    r["peers"] = json!([{"player":"a"},{"player":"b"}]);
    r["full"] = json!([(["a", "b"][host])]);
    assert_eq!(
        send(&mut c, r.clone()).unwrap()["close_peer"],
        ["a", "b"][host]
    );
    r["full"] = json!([]);
    let routed = send(&mut c, r.clone()).unwrap();
    assert_eq!(routed["kind"], "route");
    send(
        &mut c,
        json!({"op":"room_continuity_disconnect","id":id,"peer":host}),
    )
    .unwrap();
    assert_eq!(send(&mut c, r).unwrap()["kind"], "drop");
    assert_eq!(send(&mut c,json!({"op":"room_packets_delivery","id":id,"player":(["a","b"][host]),"connection":a[host]["connection"],
        "packet":routed["packet"],"target":host,"origin_connection":routed["origin_connection"],"recipient_connection":routed["recipient_connection"]})).unwrap()["kind"],"drop");
}
#[test]
fn loaded_relay_protocol_is_validated_before_any_packet_can_use_it() {
    let (mut c, id) = owner(false, false);
    samples(&mut c, &id);
    step(&mut c, &id, 1000);
    for signature in [
        "0".repeat(24),
        "1234567890abcdef:00:001a".to_owned(),
        "1234567890abcdef:0010000".to_owned(),
    ] {
        assert_eq!(
            send(
                &mut c,
                json!({"op":"room_setup_loaded","id":id,"player":"a","signature":signature})
            )
            .unwrap_err()
            .0,
            400
        );
    }
    assert_eq!(
        send(&mut c, json!({"op":"room_setup_snapshot","id":id})).unwrap()["loaded"],
        json!({})
    );
    loaded(&mut c, &id);
    assert_eq!(step(&mut c, &id, 11000)["transition"], "running");
}

fn control_request(id: &Value, attached: &[Value], source: usize, op: &str, now: f64) -> Value {
    json!({"id":id,"op":op,"player":(["a","b"][source]),"connection":attached[source]["connection"],"control_now":now,"packet_now":now,"clock_ms":11000,
        "peers":attached.iter().map(|a|json!({"player":a["player"],"native_handoff":a["features"]["native_handoff"],"departure_game_over":a["features"]["departure_game_over"]})).collect::<Vec<_>>(),"full":[]})
}
fn control_tick(
    c: &mut Coordinator,
    id: &Value,
    attached: &[Value],
    source: usize,
    now: f64,
) -> Value {
    let mut r = control_request(id, attached, source, "room_control_tick", now);
    let plan = send(c, r.clone()).unwrap();
    assert_eq!(plan["kind"], "privacy");
    r["facts"] = facts(&plan, true);
    send(c, r).unwrap()
}
fn nonce(e: &Value) -> String {
    e["publications"]
        .as_array()
        .unwrap()
        .iter()
        .find_map(|p| p["packet"]["ping"].as_str().map(str::to_owned))
        .unwrap()
}
#[test]
fn room_control_strict_decoding_and_owned_nonce_produce_only_bounded_observations() {
    let (mut c, id, a, host) = packet_room("test-v1");
    let source = 1 - host;
    let before = send(&mut c, json!({"op":"room_setup_snapshot","id":id})).unwrap();
    for text in [
        "true",
        "[]",
        "{}",
        "{\"pong\":1,\"loaded\":\"x\"}",
        "{\"loaded\":\"x\",\"lo\\u0061ded\":\"y\"}",
        "{\"pong\":null}",
    ] {
        let mut r = control_request(&id, &a, source, "room_control_receive", 0.);
        r["text"] = json!(text);
        assert_eq!(send(&mut c, r).unwrap_err().0, 400);
    }
    assert_eq!(
        send(&mut c, json!({"op":"room_setup_snapshot","id":id})).unwrap(),
        before
    );
    let ping = nonce(&control_tick(&mut c, &id, &a, source, 0.));
    let mut r = control_request(&id, &a, source, "room_control_receive", 0.01);
    r["text"] = json!(json!({"pong":ping}).to_string());
    assert_eq!(send(&mut c, r.clone()).unwrap()["kind"], "effects");
    assert_eq!(
        send(&mut c, json!({"op":"room_setup_snapshot","id":id})).unwrap()["samples"]
            [["a", "b"][source]],
        json!([20., 10., 10.])
    );
    assert_eq!(
        send(&mut c, r).unwrap_err().0,
        400,
        "A consumed nonce is not replayable"
    );
    for op in [
        "room_control_restore",
        "room_control_install",
        "room_control_challenge_reset",
    ] {
        assert_eq!(
            send(&mut c, control_request(&id, &a, source, op, 0.01))
                .unwrap_err()
                .0,
            400
        );
    }
}
#[test]
fn replaced_control_generation_cannot_consume_a_probe_or_advance_its_clock() {
    let (mut c, id, mut a, host) = packet_room("test-v1");
    let source = 1 - host;
    let old = a.clone();
    control_tick(&mut c, &id, &a, source, 0.);
    wire(
        &mut c,
        &id,
        "room_wire_detach",
        json!({"player":(["a","b"][source]),"connection":a[source]["connection"]}),
    )
    .unwrap();
    let bearer = bearer(&mut c, &id, ["a", "b"][source]);
    a[source] = wire(
        &mut c,
        &id,
        "room_wire_attach",
        json!({"route":"relay","authorization":bearer,"prior":null}),
    )
    .unwrap();
    let ping = nonce(&control_tick(&mut c, &id, &a, source, 0.));
    let loss = send(
        &mut c,
        control_request(&id, &old, source, "room_control_socket_lost", 1e12),
    )
    .unwrap();
    assert!(loss["remove"].as_array().unwrap().is_empty());
    assert!(
        loss["close"].is_null(),
        "Old detached generations cannot end their replacement's room"
    );
    let mut rejected = control_request(&id, &old, source, "room_control_rejected", 1e12);
    rejected["reason"] = json!("Stale callback");
    assert_eq!(send(&mut c, rejected).unwrap()["close"]["scope"], "peer");
    let mut stale = control_request(&id, &old, source, "room_control_receive", 1e12);
    stale["text"] = json!(json!({"pong":ping}).to_string());
    assert_eq!(send(&mut c, stale).unwrap_err().0, 409);
    let mut fresh = control_request(&id, &a, source, "room_control_receive", 0.01);
    fresh["text"] = json!(json!({"pong":ping}).to_string());
    assert_eq!(send(&mut c, fresh).unwrap()["kind"], "effects");
    let before = send(&mut c, json!({"op":"room_setup_snapshot","id":id})).unwrap();
    let ping = nonce(&control_tick(&mut c, &id, &a, source, 6.));
    let mut late = control_request(&id, &a, source, "room_control_receive", 21.001);
    late["text"] = json!(json!({"pong":ping}).to_string());
    assert_eq!(send(&mut c, late).unwrap()["close"]["scope"], "room");
    assert_eq!(
        send(&mut c, json!({"op":"room_setup_snapshot","id":id})).unwrap(),
        before
    );
}

#[test]
fn native_presence_uses_frozen_seats_and_cannot_renew_or_revive_an_expired_probe() {
    let (mut c, id, a, host) = packet_room("test-v1");
    let source = 1 - host;
    let ping = nonce(&control_tick(&mut c, &id, &a, source, 0.));
    let present = json!([{"player":(["a","b"][source]),"guest":false}]);
    assert_eq!(
        send(
            &mut c,
            control_request(&id, &a, source, "room_control_presence", 15.)
        )
        .unwrap(),
        present
    );
    assert_eq!(
        send(
            &mut c,
            control_request(&id, &a, source, "room_control_presence", 15.001)
        )
        .unwrap(),
        json!([])
    );
    let mut late = control_request(&id, &a, source, "room_control_receive", 15.);
    late["text"] = json!(json!({"pong":ping}).to_string());
    assert_eq!(send(&mut c, late).unwrap()["close"]["scope"], "room");
    let state = send(&mut c, json!({"op":"room_state_get","id":id})).unwrap();
    let mut r = control_request(&id, &a, host, "room_control_presence", 0.);
    r["io_now"] = state["deadline"].clone();
    assert_eq!(send(&mut c, r).unwrap(), json!([]));
    assert_eq!(
        send(
            &mut c,
            json!({"op":"room_state_member","id":id,"player":(["a","b"][host])})
        )
        .unwrap(),
        true,
        "Presence excludes the exact boundary without inventing a lease extension or a departure"
    );
}

#[test]
fn legacy_voice_observations_exclude_a_detached_generation_before_io_removes_its_handle() {
    let (mut c, id, a, host) = packet_room("test-v1");
    let source = 1 - host;
    let r = control_request(&id, &a, host, "room_control_voice", 0.);
    assert_eq!(send(&mut c, r.clone()).unwrap()["voice_members"], 3);
    wire(
        &mut c,
        &id,
        "room_wire_detach",
        json!({"player":(["a","b"][source]),"connection":a[source]["connection"]}),
    )
    .unwrap();
    assert_eq!(
        send(&mut c, r).unwrap()["voice_members"],
        1 << host,
        "Stale IO handles cannot keep an absent relay peer's microphone admitted"
    );
}
