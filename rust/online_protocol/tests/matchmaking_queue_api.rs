//! Queue endpoint authority over synthetic native IO; no original Live claim.
use halo2_online_protocol::{
    matchmaking_catalog, matchmaking_coordinator::Coordinator, matchmaking_model::Result,
    matchmaking_persistence::DatabaseBank,
};
use serde_json::{Value, json};
fn owner(raw: bool) -> Coordinator {
    owner_with_settings(raw, Value::Null)
}
fn owner_with_settings(raw: bool, settings: Value) -> Coordinator {
    let mut entry = json!({"id":"native","revision":"v1","name":"Native API test","description":"Declared native authorization","ranked":false,"quickmatch":true,
        "team_size":0,"minimum_players":2,"maximum_players":4,"maximum_party":4,"games":[{"id":"slayer","map":"lockout","variant":"Slayer","weight":1}],
        "stages":[{"after_seconds":0,"rank_numerator":1,"rank_denominator":1,"minimum_connection":50}],"skill":{"kind":"custom","radius":6},"provenance":"Native API test"});
    if !settings.is_null() {
        entry["required_compatibility"] = json!("halo2-native-queue-v15");
        entry["games"][0]["settings"] = settings;
    }
    if raw {
        entry["rank_configuration"] = json!(halo2_online_protocol::matchmaking_skill::RETAIL);
    }
    let entries = matchmaking_catalog::parse(
        json!({"version":1,"playlists":[entry]})
            .to_string()
            .as_bytes(),
    )
    .unwrap();
    Coordinator::new(&json!({"catalog":entries,"playlists":entries.iter().map(|e|e["policy"].clone()).collect::<Vec<_>>(),"lease":90,"proposal_lease":30,"qos_lease":30,
        "capacity":1000,"search_budget":10000,"draw_seed":2,"native_seed":3})).unwrap()
}
fn send(c: &mut Coordinator, mut r: Value) -> Result<Value> {
    if r.get("now").is_none() {
        r["now"] = json!(100.);
    }
    c.execute(&r)
}
fn session(c: &mut Coordinator, who: &str, guest: bool) -> Value {
    session_with_compat(
        c,
        who,
        guest,
        if guest {
            "halo2-native-queue-v9"
        } else {
            "test-v1"
        },
    )
}
fn session_with_compat(c: &mut Coordinator, who: &str, guest: bool, compatibility: &str) -> Value {
    let mut body = json!({"screen_name":"Same Pilot","maps":["lockout"],"language":0,"compatibility":compatibility});
    let mut identities = vec![json!({"owner":["discord",who],"player":who,"separate_party":false})];
    let mut resolved = vec![json!(who)];
    if guest {
        body["local_players"] = json!([{"screen_name":"Guest"}]);
        identities.push(Value::Null);
        resolved.push(Value::Null);
    }
    send(c,json!({"op":"session_register","body":body,"identities":identities,"resolved":resolved,"rooms":[]})).unwrap()["session"].clone()
}
fn facts() -> Value {
    json!({"party":null,"clans":[]})
}
fn begin(c: &mut Coordinator, a: &Value, request: u32) -> Value {
    send(c,json!({"op":"api_enqueue_begin","generation":a["generation"],"body":{"playlist_id":"native","revision":"v1","request_id":format!("00000000-0000-0000-0000-{request:012x}")},"facts":facts(),"match_rooms":[]})).unwrap()
}
fn skill_results(event: &Value) -> Value {
    json!(
        event["operations"]
            .as_array()
            .unwrap()
            .iter()
            .map(|op| if op["mode"] == "legacy_display" {
                json!(37)
            } else {
                json!({"raw":30,"xp":5000,"configuration":op["configuration"]})
            })
            .collect::<Vec<_>>()
    )
}
fn resume(c: &mut Coordinator, a: &Value, event: &Value, result: Value) -> Result<Value> {
    send(
        c,
        json!({"op":"api_resume","frame":event["frame"],"generation":a["generation"],"result":result,"facts":facts(),"match_rooms":[],"rooms":[]}),
    )
}
fn enqueue(c: &mut Coordinator, a: &Value, request: u32) -> Value {
    let event = begin(c, a, request);
    resume(c, a, &event, skill_results(&event)).unwrap()["value"].clone()
}
#[test]
fn native_enqueue_owns_skill_plan_guest_inheritance_and_idempotent_publication() {
    let mut c = owner(true);
    let a = session(&mut c, "a", true);
    let event = begin(&mut c, &a, 1);
    assert_eq!(
        event["operations"].as_array().unwrap().len(),
        1,
        "Guest inherits physical owner raw skill"
    );
    assert_eq!(event["operations"][0]["owner"], "a");
    let ticket = resume(&mut c, &a, &event, skill_results(&event)).unwrap()["value"].clone();
    assert_eq!(ticket["players"][0]["raw_rank"], 30);
    assert_eq!(ticket["players"][1]["raw_rank"], 30);
    assert_eq!(ticket["players"][0]["rank"], 31);
    assert_eq!(ticket["players"][1]["rank"], 31);
    assert_eq!(ticket["players"][1]["connection_owner"], "a");
    assert_eq!(ticket["players"][1]["guest"], true);
    let repeat = begin(&mut c, &a, 1);
    assert_eq!(repeat["kind"], "done");
    assert_eq!(
        send(&mut c, json!({"op":"len","map":"tickets"})).unwrap(),
        1
    );
    assert_eq!(
        resume(&mut c, &a, &event, skill_results(&event))
            .unwrap_err()
            .0,
        409
    );
}
#[test]
fn skill_io_cannot_publish_after_profile_generation_shape_or_lease_changes() {
    for case in [
        "profile",
        "generation",
        "shape",
        "expiry",
        "foreign",
        "abort",
    ] {
        let mut c = owner(false);
        let a = session(&mut c, "a", false);
        let event = begin(&mut c, &a, 1);
        let mut result = skill_results(&event);
        let mut actor = a.clone();
        match case {
            "profile" => {
                let mut entries = send(&mut c, json!({"op":"catalog_get","id":null})).unwrap();
                entries[0]["description"] = json!("Same revision changed profile");
                send(&mut c, json!({"op":"catalog_set","entries":entries})).unwrap();
            }
            "generation" => {
                session(&mut c, "a", false);
            }
            "shape" => {
                result = json!([128]);
            }
            "expiry" => {
                send(&mut c, json!({"op":"api_ticket","player":"a","now":130.})).unwrap();
            }
            "foreign" => {
                actor = session(&mut c, "b", false);
            }
            "abort" => {
                send(&mut c, json!({"op":"api_abort","frame":event["frame"]})).unwrap();
            }
            _ => unreachable!(),
        }
        assert!(resume(&mut c, &actor, &event, result).is_err(), "{case}");
        assert_eq!(
            send(&mut c, json!({"op":"len","map":"tickets"})).unwrap(),
            0,
            "{case}"
        );
        assert_eq!(
            send(&mut c, json!({"op":"setting_get","name":"draw_seed"})).unwrap(),
            2,
            "{case}"
        );
        assert_eq!(
            send(&mut c, json!({"op":"session_request_items"})).unwrap(),
            json!([]),
            "{case}"
        );
    }
}
#[test]
fn private_public_projection_keeps_legacy_shape_and_owned_optional_io() {
    let mut c = owner(false);
    let a = session(&mut c, "a", false);
    let b = session(&mut c, "b", false);
    enqueue(&mut c, &a, 1);
    enqueue(&mut c, &b, 2);
    let legacy=send(&mut c,json!({"op":"api_view_begin","generation":a["generation"],"query":{},"heartbeat":true,"rooms":[]})).unwrap()["value"].clone();
    assert_eq!(legacy.as_object().unwrap().len(), 4);
    assert_eq!(legacy["queued_players"], 2);
    assert_eq!(legacy["match"]["players"].as_array().unwrap().len(), 2);
    assert!(
        legacy["match"]["players"]
            .as_array()
            .unwrap()
            .iter()
            .all(|p| p.as_object().unwrap().len() == 3)
    );
    for private in [
        "session_key",
        "generation",
        "proposal_id",
        "raw_rank",
        "connection_owner",
    ] {
        assert!(!legacy.to_string().contains(private));
    }
    let event=send(&mut c,json!({"op":"api_view_begin","generation":a["generation"],"query":{"ranks":"1","appearance":"1","progress":"1","failure":"1"},"heartbeat":true,"rooms":[]})).unwrap();
    assert_eq!(event["action"], "database");
    assert_eq!(event["operations"].as_array().unwrap().len(), 2);
    let response =
        resume(&mut c, &a, &event, json!(["000000000000000000", null])).unwrap()["value"].clone();
    assert_eq!(response["progress"], Value::Null);
    assert_eq!(response["failure"], Value::Null);
    assert_eq!(
        response["match"]["players"][0]["level"],
        Value::Null,
        "Hidden casual artwork stays private"
    );
    assert!(
        response["match"]["players"]
            .as_array()
            .unwrap()
            .iter()
            .all(|p| p.get("appearance").is_some())
    );
}
fn room(c: &mut Coordinator, id: &Value) -> Value {
    send(c, json!({"op":"api_room_admit","id":id})).unwrap();
    json!({"id":id,"closed":false,"phase":"connecting","owners":{"a":"a","b":"b"},"active":["a","b"]})
}
fn proposal(c: &mut Coordinator, a: &Value, b: &Value, request: u32) -> Value {
    let a = enqueue(c, a, request);
    let b = enqueue(c, b, request + 1);
    send(c,json!({"op":"form","playlist":"native","tickets":[a["id"],b["id"]],"game":null,"gathering_host":"a"})).unwrap()["id"].clone()
}
fn close(c: &mut Coordinator, id: &Value) {
    send(
        c,
        json!({"op":"api_room_close","id":id,"phase":"connecting"}),
    )
    .unwrap();
    for who in ["a", "b"] {
        let ticket = send(c, json!({"op":"api_ticket","player":who})).unwrap();
        if !ticket.is_null() {
            send(c, json!({"op":"cancel","id":ticket["id"]})).unwrap();
        }
    }
}
#[test]
fn destroyed_room_content_retry_is_bound_to_latest_admission_not_only_session() {
    let mut c = owner(false);
    let a = session(&mut c, "a", false);
    let b = session(&mut c, "b", false);
    let first = proposal(&mut c, &a, &b, 1);
    room(&mut c, &first);
    close(&mut c, &first);
    let second = proposal(&mut c, &a, &b, 3);
    room(&mut c, &second);
    close(&mut c, &second);
    let failure = |id: &Value| json!({"op":"api_content_failure","generation":a["generation"],"body":{"match_id":id,"kind":"missing_content"},"rooms":[]});
    assert_eq!(send(&mut c, failure(&first)).unwrap_err().0, 409);
    assert_eq!(
        send(&mut c, json!({"op":"session_failure_get","player":"a"})).unwrap()["code"],
        "search_failed"
    );
    assert_eq!(
        send(&mut c, failure(&second)).unwrap()["response"]["accepted"],
        true
    );
    assert_eq!(
        send(&mut c, json!({"op":"session_failure_get","player":"a"})).unwrap()["code"],
        "missing_content"
    );
    assert_eq!(
        send(&mut c, json!({"op":"session_failure_get","player":"b"})).unwrap()["code"],
        "search_failed"
    );
    let mut late = failure(&second);
    late["now"] = json!(115.);
    assert_eq!(
        send(&mut c, late).unwrap_err().0,
        403,
        "Native15-second destroyed-room cache expires"
    );
}
#[test]
fn cancellation_and_start_observation_are_native_owner_scoped() {
    let mut c = owner(false);
    let a = session(&mut c, "a", false);
    let b = session(&mut c, "b", false);
    let id = proposal(&mut c, &a, &b, 1);
    let mut r = room(&mut c, &id);
    r["phase"] = json!("loading");
    let event=send(&mut c,json!({"op":"api_view_begin","generation":a["generation"],"query":{"start":"1"},"heartbeat":false,"rooms":[r]})).unwrap();
    assert_eq!(event["action"], "room_start");
    assert_eq!(event["room"], id);
    let result=send(&mut c,json!({"op":"api_resume","frame":event["frame"],"generation":a["generation"],"result":6999,"rooms":[r]})).unwrap();
    assert_eq!(result["value"]["start_countdown_ms"], 6999);
    assert!(result["value"].get("start_room").is_none());
    let idle = send(
        &mut c,
        json!({"op":"api_cancel","generation":a["generation"],"query":{"cancel":"1"},"rooms":[r]}),
    )
    .unwrap();
    assert_eq!(idle["state"], "idle");
    assert!(
        send(&mut c, json!({"op":"session_failure_get","player":"a"}))
            .unwrap()
            .is_null()
    );
    assert_eq!(
        send(&mut c, json!({"op":"session_failure_get","player":"b"})).unwrap()["code"],
        "search_failed"
    );
}

#[test]
fn start_view_uses_current_admitted_room_when_an_old_loading_observation_remains() {
    let mut c = owner(false);
    let a = session(&mut c, "a", false);
    let b = session(&mut c, "b", false);
    let first = proposal(&mut c, &a, &b, 1);
    let mut old = room(&mut c, &first);
    old["phase"] = json!("loading");
    close(&mut c, &first);
    let second = proposal(&mut c, &a, &b, 3);
    let mut current = room(&mut c, &second);
    current["phase"] = json!("loading");
    let query = json!({"start":"1","start_room":"1"});
    let view = |rooms: Value| json!({"op":"api_view_begin","generation":a["generation"],"query":query,"heartbeat":false,"rooms":rooms});
    let event = send(&mut c, view(json!([old, current]))).unwrap();
    assert_eq!(event["action"], "room_start");
    assert_eq!(event["room"], second);
    let result = send(
        &mut c,
        json!({"op":"api_resume","frame":event["frame"],"generation":a["generation"],
            "result":6999,"rooms":[old,current]}),
    )
    .unwrap();
    assert_eq!(result["value"]["start_countdown_ms"], 6999);
    assert_eq!(result["value"]["start_room"], second);
}

#[test]
fn start_read_is_discarded_when_its_room_is_replaced_during_io() {
    let mut c = owner(false);
    let a = session(&mut c, "a", false);
    let b = session(&mut c, "b", false);
    let first = proposal(&mut c, &a, &b, 1);
    let mut old = room(&mut c, &first);
    old["phase"] = json!("loading");
    let query = json!({"start":"1","start_room":"1"});
    let view = |rooms: Value| json!({"op":"api_view_begin","generation":a["generation"],"query":query,"heartbeat":false,"rooms":rooms});
    let event = send(&mut c, view(json!([old]))).unwrap();
    assert_eq!(event["room"], first);
    close(&mut c, &first);
    let second = proposal(&mut c, &a, &b, 3);
    let mut current = room(&mut c, &second);
    current["phase"] = json!("loading");
    let stale = send(
        &mut c,
        json!({"op":"api_resume","frame":event["frame"],"generation":a["generation"],
            "result":0,"rooms":[current]}),
    )
    .unwrap();
    assert_eq!(stale["kind"], "done");
    assert_eq!(stale["value"]["start_countdown_ms"], Value::Null);
    assert_eq!(stale["value"]["start_room"], Value::Null);
    let fresh = send(&mut c, view(json!([current]))).unwrap();
    assert_eq!(fresh["room"], second);
}

#[test]
fn loading_room_keeps_full_match_and_clock_after_final_ack_consumes_the_proposal() {
    let stock: Value =
        serde_json::from_str(include_str!("../data/tu15-result-presets.json")).unwrap();
    let options = stock["variants"][0]["settings"].clone();
    let mut c = owner_with_settings(false, options.clone());
    let a = session_with_compat(&mut c, "a", false, "halo2-native-queue-v16");
    let b = session_with_compat(&mut c, "b", false, "halo2-native-queue-v16");
    let id = proposal(&mut c, &a, &b, 1);
    let mut r = room(&mut c, &id);
    let read = |query: Value, generation: &Value, rooms: Value| {
        json!({
            "op":"api_view_begin","generation":generation,"query":query,"heartbeat":false,"rooms":rooms
        })
    };
    let generation = &a["generation"];
    let queued = send(&mut c, read(json!({}), generation, json!([r]))).unwrap()["value"].clone();
    assert_eq!(queued["match"]["settings"], options);
    for (host, peer) in [("a", "b"), ("b", "a")] {
        send(
            &mut c,
            json!({"op":"connection","host":host,"player":peer,"score":100}),
        )
        .unwrap();
    }
    for who in ["a", "b"] {
        send(
            &mut c,
            json!({"op":"acknowledge","id":id,"player":who,"optimize":false,"quality":null}),
        )
        .unwrap();
    }
    r["phase"] = json!("loading");
    assert!(
        send(&mut c, json!({"op":"api_ticket","player":"a"}))
            .unwrap()
            .is_null()
    );
    let finish = |c: &mut Coordinator, query: Value| {
        let event = send(c, read(query, generation, json!([r]))).unwrap();
        assert_eq!(event["action"], "room_start");
        assert_eq!(event["room"], id);
        send(
            c,
            json!({"op":"api_resume","frame":event["frame"],"generation":generation,
            "result":0,"rooms":[r]}),
        )
        .unwrap()["value"]
            .clone()
    };
    let bound = finish(&mut c, json!({"start":"1","start_room":"1"}));
    assert_eq!(bound["start_room"], id);
    assert_eq!(bound["start_countdown_ms"], 0);
    assert_eq!(bound["state"], "awaiting_transport");
    assert_eq!(bound["playlist_id"], queued["playlist_id"]);
    assert_eq!(bound["match"], queued["match"]);
    let scalar = finish(&mut c, json!({"start":"1"}));
    assert_eq!(
        scalar,
        json!({"state":"idle","playlist_id":"","queued_players":0,
        "match":null,"start_countdown_ms":0})
    );
}

#[test]
fn database_bank_frames_are_monotonic_and_cannot_admit_other_sql_work() {
    assert!(
        DatabaseBank::new(&json!({"mode":"snapshot_bank","operations":[{"mode":"result_schema"}]}))
            .is_err()
    );
    let mut bank=DatabaseBank::new(&json!({"mode":"snapshot_bank","operations":[{"mode":"player_appearance","owner":"a"},{"mode":"player_appearance","owner":"b"}]})).unwrap();
    let first = bank.execute(&json!({"op":"begin"})).unwrap();
    assert_eq!(first["frame"], 1);
    assert_eq!(first["atomic"], true);
    assert!(
        bank.execute(&json!({"op":"reply","frame":0,"rows":[],"changes":[-1]}))
            .is_err()
    );
    let second = bank
        .execute(&json!({"op":"reply","frame":1,"rows":[],"changes":[-1]}))
        .unwrap();
    assert_eq!(second["frame"], 2);
    assert!(
        bank.execute(&json!({"op":"reply","frame":1,"rows":[],"changes":[-1]}))
            .is_err()
    );
    let done = bank
        .execute(&json!({"op":"reply","frame":2,"rows":[["000000000000000000"]],"changes":[-1]}))
        .unwrap();
    assert_eq!(done["value"], json!([null, "000000000000000000"]));
    assert!(
        bank.execute(&json!({"op":"reply","frame":2,"rows":[],"changes":[-1]}))
            .is_err()
    );
}
