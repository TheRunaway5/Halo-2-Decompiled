//! Native accepted-party policy and permit checks; no original Live claim.
use halo2_online_protocol::{
    matchmaking_catalog,
    matchmaking_coordinator::{self, Coordinator},
    matchmaking_model::Result,
};
use serde_json::{Value, json};
fn owner() -> Coordinator {
    let entries=matchmaking_catalog::parse(json!({"version":1,"playlists":[{"id":"native","revision":"v1","name":"Native party test","description":"Declared native policy",
        "ranked":false,"quickmatch":true,"team_size":0,"minimum_players":2,"maximum_players":4,"maximum_party":4,
        "games":[{"id":"slayer","map":"lockout","variant":"Slayer","weight":1}],"stages":[{"after_seconds":0,"rank_numerator":1,"rank_denominator":1,"minimum_connection":50}],
        "skill":{"kind":"custom","radius":6},"provenance":"Native authorization test"}]}).to_string().as_bytes()).unwrap();
    Coordinator::new(&json!({"catalog":entries,"playlists":entries.iter().map(|e|e["policy"].clone()).collect::<Vec<_>>(),"lease":90,"proposal_lease":30,"qos_lease":30,
        "capacity":1000,"search_budget":10000,"draw_seed":2,"native_seed":3})).unwrap()
}
fn send(c: &mut Coordinator, mut r: Value) -> Result<Value> {
    if r.get("now").is_none() {
        r["now"] = json!(100.);
    }
    c.execute(&r)
}
fn session(c: &mut Coordinator, who: &str) -> Value {
    send(c,json!({"op":"session_register","body":{"screen_name":"Pilot","maps":["lockout"],"compatibility":"test-v1"},
        "identities":[{"owner":["discord",who],"player":who,"separate_party":false}],"resolved":[who],"rooms":[]})).unwrap()["session"].clone()
}
fn facts() -> Value {
    json!({"party":{"id":"accepted","leader":"a","version":4,"members":[
    {"player":"a","handle":"ha","ready_playlist":"","ready_revision":"","ready_version":0,"consent":null},
    {"player":"b","handle":"hb","ready_playlist":"","ready_revision":"","ready_version":0,"consent":null}]},"clans":[]})
}
fn approve(c: &mut Coordinator, facts: &mut Value) {
    approve_member(c, facts, 1);
}
fn approve_member(c: &mut Coordinator, facts: &mut Value, member: usize) {
    let who = facts["party"]["members"][member]["player"].clone();
    let plan=send(c,json!({"op":"party_ready","player":who,"playlist":"native","revision":"v1","version":facts["party"]["version"],"facts":facts,"rooms":[]})).unwrap();
    let row = &mut facts["party"]["members"][member];
    row["ready_playlist"] = json!("native");
    row["ready_revision"] = json!("v1");
    row["ready_version"] = plan["consent"]["version"].clone();
    row["consent"] = plan["consent"].clone();
}
fn request_id() -> Value {
    json!("00000000-0000-0000-0000-000000000001")
}
fn body() -> Value {
    json!({"playlist_id":"native","revision":"v1","request_id":request_id()})
}
fn prepare(c: &mut Coordinator, a: &Value, facts: &Value) -> Result<Value> {
    send(
        c,
        json!({"op":"party_queue_prepare","generation":a["generation"],"body":body(),"facts":facts,"rooms":[]}),
    )
}
fn queue(a: &Value, selected: &Value, facts: &Value) -> Value {
    let players=selected["members"].as_array().unwrap().iter().flat_map(|c|c["local_players"].as_array().unwrap().iter().map(move|p|json!({"id":p["player"],"screen_name":p["screen_name"],"guest":p["guest"],"connection_owner":c["player"],"maps":c["maps"],"rank":1,"raw_rank":null}))).collect::<Vec<_>>();
    json!({"op":"session_queue_enqueue","generation":a["generation"],"body":body(),"permit":selected["permit"],"facts":facts,
        "members":selected["members"].as_array().unwrap().iter().map(|c|c["generation"].clone()).collect::<Vec<_>>(),
        "queue":{"party_id":selected["party_id"],"playlist_id":"native","compatibility":"test-v1","leader":"a","language":0,"players":players,"clan":null,"clan_rank":null}})
}
#[test]
fn profile_changes_hide_old_consent_and_require_fresh_approval() {
    let mut c = owner();
    let a = session(&mut c, "a");
    session(&mut c, "b");
    let mut f = facts();
    approve(&mut c, &mut f);
    assert_eq!(
        send(&mut c, json!({"op":"party_ready_view","facts":f})).unwrap(),
        json!(["", "native"])
    );
    assert!(prepare(&mut c, &a, &f).is_ok());
    let mut entries = send(&mut c, json!({"op":"catalog_get","id":null})).unwrap();
    entries[0]["description"] = json!("Changed full profile at the same visible revision");
    send(&mut c, json!({"op":"catalog_set","entries":entries})).unwrap();
    assert_eq!(
        send(&mut c, json!({"op":"party_ready_view","facts":f})).unwrap(),
        json!(["", ""])
    );
    assert_eq!(prepare(&mut c, &a, &f).unwrap_err().0, 409);
    approve(&mut c, &mut f);
    assert!(prepare(&mut c, &a, &f).is_ok());
}
#[test]
fn replaced_connection_invalidates_whole_roster_before_directory_notifications() {
    let mut c = owner();
    let a = session(&mut c, "a");
    session(&mut c, "b");
    let mut f = facts();
    approve(&mut c, &mut f);
    let original = f.clone();
    send(&mut c, json!({"op":"session_revoke","key":a["key"]})).unwrap();
    let replacement = session(&mut c, "a");
    assert_ne!(a["generation"], replacement["generation"]);
    assert_eq!(
        f, original,
        "Simulate delayed persistent-party notification"
    );
    assert_eq!(prepare(&mut c, &replacement, &f).unwrap_err().0, 409);
    approve(&mut c, &mut f);
    assert!(prepare(&mut c, &replacement, &f).is_ok());
}
#[test]
fn permits_reject_changed_facts_request_owner_or_expiry_without_publishing() {
    let mut c = owner();
    let a = session(&mut c, "a");
    session(&mut c, "b");
    let mut f = facts();
    approve(&mut c, &mut f);
    let selected = prepare(&mut c, &a, &f).unwrap();
    let q = queue(&a, &selected, &f);
    for (path, value) in [
        (vec!["facts", "party", "version"], json!(5)),
        (
            vec!["body", "request_id"],
            json!("00000000-0000-0000-0000-000000000002"),
        ),
        (vec!["queue", "party_id"], json!("other")),
    ] {
        let mut bad = q.clone();
        let mut target = &mut bad;
        for key in &path[..path.len() - 1] {
            target = &mut target[*key];
        }
        target[*path.last().unwrap()] = value;
        assert_eq!(send(&mut c, bad).unwrap_err().0, 409);
    }
    assert_eq!(
        send(&mut c, json!({"op":"len","map":"tickets"})).unwrap(),
        json!(0)
    );
    assert_eq!(
        send(&mut c, json!({"op":"setting_get","name":"draw_seed"})).unwrap(),
        json!(2)
    );
    let mut late = q;
    late["now"] = json!(130.);
    assert_eq!(send(&mut c, late).unwrap_err().0, 409);
}
#[test]
fn successful_publication_freezes_party_generation_and_effects_retire_it() {
    let mut c = owner();
    let a = session(&mut c, "a");
    let b = session(&mut c, "b");
    let mut f = facts();
    approve(&mut c, &mut f);
    let selected = prepare(&mut c, &a, &f).unwrap();
    let q = queue(&a, &selected, &f);
    let ticket = send(&mut c, q.clone()).unwrap();
    assert_eq!(
        send(&mut c, q).unwrap()["id"],
        ticket["id"],
        "Request retries retain the accepted ticket"
    );
    let proposal=send(&mut c,json!({"op":"form","playlist":"native","tickets":[ticket["id"]],"game":null,"gathering_host":"a"})).unwrap();
    let admitted = send(&mut c, json!({"op":"admission","id":proposal["id"]})).unwrap();
    assert_eq!(admitted["bound"]["party_bindings"][0], selected["binding"]);
    assert_eq!(
        admitted["bound"]["party_bindings"][0]["members"][1]["generation"],
        b["generation"]
    );
    send(
        &mut c,
        json!({"op":"party_effects","effects":{"parties":["accepted"]}}),
    )
    .unwrap();
    assert_eq!(
        send(&mut c, json!({"op":"len","map":"tickets"})).unwrap(),
        json!(0)
    );
    assert_eq!(
        send(&mut c, json!({"op":"len","map":"proposals"})).unwrap(),
        json!(0)
    );
}
#[test]
fn schema_migration_and_stale_ready_inputs_fail_closed() {
    let schema=matchmaking_coordinator::value(&json!({"op":"party_consent_schema","columns":["player","party","version","handle","playlist","revision","profile","session_key","generation"]})).unwrap();
    assert!(
        schema["statements"]
            .as_array()
            .unwrap()
            .iter()
            .any(|s| s["sql"].as_str().unwrap().starts_with("ALTER TABLE"))
    );
    assert_eq!(
        matchmaking_coordinator::value(&json!({"op":"party_consent_schema","columns":["player"]}))
            .unwrap_err()
            .0,
        409
    );
    let mut c = owner();
    session(&mut c, "a");
    session(&mut c, "b");
    let f = facts();
    assert_eq!(send(&mut c,json!({"op":"party_ready","player":"b","playlist":"native","revision":"v1","version":3,"facts":f,"rooms":[]})).unwrap_err().0,409);
    assert_eq!(send(&mut c,json!({"op":"party_ready","player":"outsider","playlist":"native","revision":"v1","version":4,"facts":f,"rooms":[]})).unwrap_err().0,403);
    assert_eq!(
        send(&mut c, json!({"op":"len","map":"tickets"})).unwrap(),
        json!(0)
    );
}

fn countdown_body(action: &str) -> Value {
    let mut body = json!({"action":action,"request_id":request_id(),"version":4});
    if action == "countdown_start" {
        body["playlist_id"] = json!("native");
        body["revision"] = json!("v1");
    }
    body
}
fn countdown_prepare(
    c: &mut Coordinator,
    f: &Value,
    who: &str,
    action: &str,
    clock: u32,
) -> Result<Value> {
    send(
        c,
        json!({"op":"party_countdown_prepare","body":countdown_body(action),"player":who,"facts":f,"rooms":[],
        "clock_ms":clock,"wall_seconds":1000,"requests":{"payload":null,"count":0}}),
    )
}
fn countdown_commit(c: &mut Coordinator, f: &Value, plan: &Value) -> Result<Value> {
    send(
        c,
        json!({"op":"party_countdown_commit","token":plan["token"],"facts":f,"rooms":[]}),
    )
}
fn countdown_get(c: &mut Coordinator) -> Value {
    send(c, json!({"op":"party_countdown_get","party":"accepted"})).unwrap()
}
fn countdown_command(c: &mut Coordinator, f: &Value, who: &str, action: &str, clock: u32) -> Value {
    let plan = countdown_prepare(c, f, who, action, clock).unwrap();
    countdown_commit(c, f, &plan).unwrap();
    countdown_get(c)
}
fn countdown_tick(c: &mut Coordinator, clock: u32) {
    send(c,json!({"op":"party_countdown_tick","clock_ms":clock,"versions":[{"id":"accepted","version":4}]})).unwrap();
}
fn countdown_pair() -> (Coordinator, Value) {
    let mut c = owner();
    session(&mut c, "a");
    session(&mut c, "b");
    let mut f = facts();
    approve(&mut c, &mut f);
    (c, f)
}
#[test]
fn countdown_publishes_original_tuples_and_consumes_zero_without_restarting() {
    let (mut c, mut f) = countdown_pair();
    session(&mut c, "c");
    f["party"]["members"].as_array_mut().unwrap().push(json!({"player":"c","handle":"hc","ready_playlist":"","ready_revision":"","ready_version":0,"consent":null}));
    approve_member(&mut c, &mut f, 1);
    approve_member(&mut c, &mut f, 2);
    assert_eq!(
        countdown_command(&mut c, &f, "a", "countdown_start", 0)["seconds"],
        10
    );
    let request = countdown_command(&mut c, &f, "b", "countdown_request", 0);
    assert_eq!(request["mode"], 1);
    assert_eq!(request["requester"], "b");
    assert_eq!(
        countdown_command(&mut c, &f, "c", "countdown_request", 900),
        request,
        "Identical tuple preserves first requester, clock and serial"
    );
    let public = send(
        &mut c,
        json!({"op":"party_countdown_view","player":"c","facts":f}),
    )
    .unwrap();
    assert_eq!(public["countdown"]["requester"], "hb");
    assert_eq!(public["countdown"].as_object().unwrap().len(), 5);
    countdown_tick(&mut c, 1000);
    let ticked = countdown_get(&mut c);
    assert_eq!(
        (
            ticked["seconds"].clone(),
            ticked["mode"].clone(),
            ticked["requester"].clone()
        ),
        (json!(9), json!(0), Value::Null)
    );
    countdown_tick(&mut c, 20000);
    assert_eq!(
        countdown_get(&mut c)["seconds"],
        8,
        "Stalled pump decrements once"
    );
    for _ in 0..8 {
        countdown_command(&mut c, &f, "a", "countdown_start", 20000);
    }
    let minimum = countdown_get(&mut c);
    assert_eq!(minimum["seconds"], 3);
    assert_eq!(
        countdown_command(&mut c, &f, "a", "countdown_start", 20900),
        minimum,
        "No change leaves clock and serial alone"
    );
    for ms in [21000, 22000, 23000] {
        countdown_tick(&mut c, ms);
    }
    let zero = countdown_get(&mut c);
    assert_eq!(zero["seconds"], 0);
    assert_eq!(zero["complete"], true);
    assert_eq!(
        countdown_command(&mut c, &f, "a", "countdown_start", 24000),
        zero
    );
    send(
        &mut c,
        json!({"op":"party_select","player":"a","playlist":"native","facts":f,"rooms":[]}),
    )
    .unwrap();
    let consumed = countdown_get(&mut c);
    assert!(consumed["seconds"].is_null());
    assert_eq!(consumed["serial"], zero["serial"]);
    assert_eq!(consumed["complete"], true);
    assert_eq!(
        countdown_prepare(&mut c, &f, "b", "countdown_request", 24000)
            .unwrap_err()
            .0,
        409
    );
    let fresh = countdown_command(&mut c, &f, "a", "countdown_start", 24000);
    assert_eq!(fresh["seconds"], 10);
    assert!(fresh["serial"].as_u64().unwrap() > zero["serial"].as_u64().unwrap());
}
#[test]
fn countdown_clock_wrap_is_signed_and_each_tick_restarts_the_clock() {
    let (mut c, f) = countdown_pair();
    countdown_command(&mut c, &f, "a", "countdown_start", 0xfffffff0);
    countdown_tick(&mut c, 983);
    assert_eq!(countdown_get(&mut c)["seconds"], 10);
    countdown_tick(&mut c, 984);
    assert_eq!(countdown_get(&mut c)["seconds"], 9);
    countdown_tick(&mut c, 984u32.wrapping_add(0x80000000));
    assert_eq!(countdown_get(&mut c)["seconds"], 9);
    countdown_tick(&mut c, 1984);
    assert_eq!(countdown_get(&mut c)["seconds"], 8);
}
#[test]
fn countdown_staged_publication_abort_epoch_and_cancel_fail_closed() {
    let (mut c, f) = countdown_pair();
    let aborted = countdown_prepare(&mut c, &f, "a", "countdown_start", 0).unwrap();
    assert!(countdown_get(&mut c).is_null(), "SQL has not committed");
    send(
        &mut c,
        json!({"op":"party_countdown_abort","token":aborted["token"]}),
    )
    .unwrap();
    assert_eq!(countdown_commit(&mut c, &f, &aborted).unwrap_err().0, 409);
    let pending = countdown_prepare(&mut c, &f, "a", "countdown_start", 0).unwrap();
    countdown_command(&mut c, &f, "a", "countdown_cancel", 0);
    assert_eq!(
        countdown_commit(&mut c, &f, &pending).unwrap_err().0,
        409,
        "Cancellation retires even an unpublished start"
    );
    countdown_command(&mut c, &f, "a", "countdown_start", 0);
    let pending = countdown_prepare(&mut c, &f, "a", "countdown_start", 900).unwrap();
    countdown_tick(&mut c, 1000);
    assert_eq!(
        countdown_commit(&mut c, &f, &pending).unwrap_err().0,
        409,
        "A later native tick defeats the stale prepared write"
    );
    assert_eq!(countdown_get(&mut c)["seconds"], 9);
}
#[test]
fn countdown_profile_roster_and_connection_generation_fence_prepared_and_live_rows() {
    for case in ["profile", "connection", "facts", "version", "expiry"] {
        let (mut c, mut f) = countdown_pair();
        let pending = countdown_prepare(&mut c, &f, "a", "countdown_start", 0).unwrap();
        match case {
            "profile" => {
                let mut entries = send(&mut c, json!({"op":"catalog_get","id":null})).unwrap();
                entries[0]["description"] = json!("Same revision, different complete profile");
                send(&mut c, json!({"op":"catalog_set","entries":entries})).unwrap();
            }
            "connection" => {
                session(&mut c, "b");
            }
            "facts" => {
                f["party"]["members"][1]["handle"] = json!("replacement-handle");
            }
            "version" => {
                f["party"]["version"] = json!(5);
            }
            "expiry" => {
                send(&mut c, json!({"op":"party_countdown_keys","now":130.})).unwrap();
            }
            _ => unreachable!(),
        }
        assert_eq!(
            countdown_commit(&mut c, &f, &pending).unwrap_err().0,
            409,
            "{case}"
        );
        assert!(countdown_get(&mut c).is_null(), "{case}");
    }
    let (mut c, mut f) = countdown_pair();
    countdown_command(&mut c, &f, "a", "countdown_start", 0);
    f["party"]["version"] = json!(5);
    assert!(
        send(
            &mut c,
            json!({"op":"party_countdown_view","player":"a","facts":f})
        )
        .unwrap()["countdown"]
            .is_null()
    );
}
#[test]
fn countdown_native_request_replay_authority_and_storage_guards() {
    let (mut c, f) = countdown_pair();
    assert_eq!(
        countdown_prepare(&mut c, &f, "b", "countdown_start", 0)
            .unwrap_err()
            .0,
        403
    );
    assert_eq!(
        countdown_prepare(&mut c, &f, "a", "countdown_request", 0)
            .unwrap_err()
            .0,
        409
    );
    assert_eq!(
        countdown_prepare(&mut c, &f, "outsider", "countdown_cancel", 0)
            .unwrap_err()
            .0,
        403
    );
    let body = countdown_body("countdown_start");
    let replay=send(&mut c,json!({"op":"party_countdown_prepare","body":body,"player":"a","requests":{"payload":body.to_string(),"count":256}})).unwrap();
    assert!(replay["token"].is_null());
    assert_eq!(replay["statements"], json!([]));
    assert!(
        countdown_get(&mut c).is_null(),
        "Replay cannot install or restart a native row"
    );
    let mut bad = body.clone();
    bad["version"] = json!(5);
    assert_eq!(send(&mut c,json!({"op":"party_countdown_prepare","body":bad,"player":"a","requests":{"payload":body.to_string(),"count":1}})).unwrap_err().0,409);
    assert_eq!(send(&mut c,json!({"op":"party_countdown_prepare","body":body,"player":"a","requests":{"payload":null,"count":256}})).unwrap_err().0,429);
    let plan = countdown_prepare(&mut c, &f, "a", "countdown_start", 0).unwrap();
    assert_eq!(plan["statements"].as_array().unwrap().len(), 3);
    assert!(
        plan["statements"]
            .as_array()
            .unwrap()
            .iter()
            .all(|s| s["changes"] == 1)
    );
    assert!(
        plan["statements"][2]["sql"]
            .as_str()
            .unwrap()
            .contains("NOT EXISTS")
    );
    assert_eq!(
        send(
            &mut c,
            json!({"op":"party_countdown_restore","party":"accepted","value":{}})
        )
        .unwrap_err()
        .0,
        400
    );
    assert!(countdown_get(&mut c).is_null());
}
