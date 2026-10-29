//! Native registry/IO authority tests. No original Live/relay parity claim.
use halo2_online_protocol::matchmaking_coordinator::Coordinator;
use halo2_online_protocol::matchmaking_model::Result;
use serde_json::{Value, json};

fn policy() -> Value {
    json!({"id":"gather","revision":"declared-v1","ranked":true,"team_size":0,
        "minimum_players":2,"maximum_players":2,"maximum_party":1,
        "games":[{"id":"slayer","map":"lockout","variant":"Slayer","weight":1,"settings":null}],
        "stages":[{"after_seconds":0,"rank_fraction":[1,1],"minimum_connection":50,"unbounded_skill":false}],
        "provenance":"Declared native service inputs; not an original capture","party_balance":null,
        "use_2007_rank_limits":false,"high_rank_radius":null,"mixed_party_floor":false,"custom_rank_radius":6,
        "clan_match":null,"candidate_selection":"retail-candidates-native-relay-v1",
        "gathering":{"countdown_ms":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],"timeout_ms":150000,"provenance":"Declared test timing"},
        "team_count":2,"hopper":null})
}
fn owner(p: &Value) -> Coordinator {
    let mut c = Coordinator::new(
        &json!({"playlists":[p],"lease":90,"proposal_lease":30,"qos_lease":30,
        "capacity":1000,"search_budget":10000,"draw_seed":2,"native_seed":3}),
    )
    .unwrap();
    send(&mut c, json!({"op":"gather_init","seed":1})).unwrap();
    c
}
fn send(c: &mut Coordinator, mut r: Value) -> Result<Value> {
    r["now"] = json!(100.);
    c.execute(&r)
}
fn queue(c: &mut Coordinator, index: usize) -> Value {
    send(c,json!({"op":"enqueue","party_id":format!("party-{index}"),"playlist_id":"gather","compatibility":"native-declared",
        "players":[{"id":format!("p{index}"),"screen_name":"Pilot","rank":2,"raw_rank":1,"guest":false,"connection_owner":null,"maps":["lockout"]}],
        "clan":null,"clan_rank":null,"leader":null,"language":0})).unwrap()
}
fn pump(c: &mut Coordinator, mut event: Value, measured: bool, allocate: bool) -> Value {
    for _ in 0..1000 {
        if event["kind"] == "done" {
            return event["value"].clone();
        }
        assert_eq!(event["kind"], "io");
        let result = if event["action"] == "facts" {
            json!({"ready":vec![true;event["players"].as_array().unwrap().len()],
                "pairs":event["pairs"].as_array().unwrap().iter().map(|_|json!({"milliseconds":if measured{json!(10)}else{Value::Null},"pending":allocate})).collect::<Vec<_>>()})
        } else {
            json!(vec![allocate; event["pairs"].as_array().unwrap().len()])
        };
        event = send(
            c,
            json!({"op":"gather_resume","frame":event["frame"],"result":result}),
        )
        .unwrap();
    }
    panic!("Declared work budget did not terminate the IO round")
}
fn begin(c: &mut Coordinator) -> Value {
    send(
        c,
        json!({"op":"gather_advance","playlist":"gather","wall":1000.}),
    )
    .unwrap()
}
fn keys(c: &mut Coordinator, map: &str) -> Vec<Value> {
    send(c, json!({"op":"gather_keys","map":map}))
        .unwrap()
        .as_array()
        .unwrap()
        .clone()
}

#[test]
fn native_registry_query_cache_and_complete_selected_admission_share_the_owner() {
    let mut c = owner(&policy());
    let a = queue(&mut c, 0);
    let b = queue(&mut c, 1);
    let event = begin(&mut c);
    assert!(pump(&mut c, event, false, true).is_null());
    assert_eq!(keys(&mut c, "sessions").len(), 1);
    assert_eq!(keys(&mut c, "queries").len(), 1);
    let key = keys(&mut c, "queries")[0].clone();
    let query = send(&mut c, json!({"op":"gather_get","map":"queries","key":key})).unwrap();
    let event = begin(&mut c);
    assert!(pump(&mut c, event, false, true).is_null());
    let still = send(&mut c, json!({"op":"gather_get","map":"queries","key":key})).unwrap();
    assert_eq!(query["generation"], still["generation"]);
    assert_eq!(query["started"], still["started"]);
    let event = begin(&mut c);
    let proposal = pump(&mut c, event, true, true);
    assert!(!proposal.is_null());
    assert_eq!(proposal["game"]["id"], "slayer");
    let mut ids = proposal["tickets"].as_array().unwrap().clone();
    ids.sort_by_key(Value::to_string);
    let mut expected = vec![a["id"].clone(), b["id"].clone()];
    expected.sort_by_key(Value::to_string);
    assert_eq!(ids, expected);
    assert!(keys(&mut c, "sessions").is_empty());
    assert!(keys(&mut c, "queries").is_empty());
    let binding = send(&mut c, json!({"op":"admission","id":proposal["id"]})).unwrap();
    assert_eq!(binding["bound"]["game"], proposal["game"]);
}

#[test]
fn malformed_replayed_and_canceled_io_cannot_advance_a_new_search() {
    let mut c = owner(&policy());
    let ticket = queue(&mut c, 0);
    let event = begin(&mut c);
    let token = event["frame"].as_u64().unwrap();
    assert_eq!(
        send(
            &mut c,
            json!({"op":"gather_resume","frame":token+1,"result":{"ready":[true],"pairs":[]}})
        )
        .unwrap_err()
        .0,
        409
    );
    assert_eq!(
        send(
            &mut c,
            json!({"op":"gather_resume","frame":token,"result":{"ready":[],"pairs":[]}})
        )
        .unwrap_err()
        .0,
        400
    );
    send(&mut c, json!({"op":"cancel","id":ticket["id"]})).unwrap();
    assert_eq!(
        send(
            &mut c,
            json!({"op":"gather_resume","frame":token,"result":{"ready":[true],"pairs":[]}})
        )
        .unwrap_err()
        .0,
        409
    );
    assert!(keys(&mut c, "states").is_empty());
    assert!(keys(&mut c, "queries").is_empty());
    let replacement = queue(&mut c, 0);
    assert_ne!(replacement["id"], ticket["id"]);
    let event = begin(&mut c);
    assert_ne!(event["frame"], json!(token));
    let token2 = event["frame"].clone();
    assert_eq!(
        send(&mut c, json!({"op":"gather_abort","frame":token}))
            .unwrap_err()
            .0,
        409
    );
    send(&mut c, json!({"op":"gather_abort","frame":token2})).unwrap();
    let event = begin(&mut c);
    assert!(pump(&mut c, event, true, true).is_null());
    assert_eq!(
        send(
            &mut c,
            json!({"op":"gather_state_command","ticket":ticket["id"],"command":{"op":"snapshot"}})
        )
        .unwrap_err()
        .0,
        404
    );
}

#[test]
fn same_revision_profile_change_retires_registry_states_and_inflight_work() {
    let mut p = policy();
    let mut c = owner(&p);
    let ticket = queue(&mut c, 0);
    let event = begin(&mut c);
    pump(&mut c, event, true, true);
    assert_eq!(keys(&mut c, "sessions").len(), 1);
    queue(&mut c, 1);
    let event = begin(&mut c);
    assert_eq!(event["kind"], "io");
    p["games"][0]["weight"] = json!(2);
    send(&mut c, json!({"op":"policy_put","key":"gather","value":p})).unwrap();
    assert!(keys(&mut c, "sessions").is_empty());
    assert!(keys(&mut c, "states").is_empty());
    assert!(keys(&mut c, "queries").is_empty());
    assert_eq!(send(&mut c,json!({"op":"gather_resume","frame":event["frame"],"result":{"ready":[true],"pairs":[]}})).unwrap_err().0,409);
    assert_eq!(
        send(&mut c, json!({"op":"gather_state","ticket":ticket["id"]}))
            .unwrap_err()
            .0,
        409
    );
}

#[test]
#[cfg(not(feature = "matchmaking-test-inputs"))]
fn registry_restore_is_absent_from_the_production_command_surface() {
    let mut c = owner(&policy());
    assert_eq!(
        send(
            &mut c,
            json!({"op":"gather_fixture","action":"insert","value":{}})
        )
        .unwrap_err()
        .0,
        400
    );
}
