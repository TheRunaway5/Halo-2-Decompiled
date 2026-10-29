//! Native session authority regressions; these are not original Live auth proof.
use halo2_online_protocol::{
    matchmaking_catalog,
    matchmaking_coordinator::{self, Coordinator},
    matchmaking_model::{Error, Result},
};
use serde_json::{Value, json};

fn owner() -> Coordinator {
    let entries=matchmaking_catalog::parse(json!({"version":1,"playlists":[{
        "id":"native","revision":"declared-v1","name":"Native sessions","description":"Explicit native service policy",
        "ranked":false,"quickmatch":true,"team_size":0,"minimum_players":2,"maximum_players":4,"maximum_party":4,
        "games":[{"id":"slayer","map":"lockout","variant":"Slayer","weight":1}],
        "stages":[{"after_seconds":0,"rank_numerator":1,"rank_denominator":1,"minimum_connection":50}],
        "skill":{"kind":"custom","radius":6},"provenance":"Native authority test; no historical service claim"}]}).to_string().as_bytes()).unwrap();
    Coordinator::new(&json!({"catalog":entries,"playlists":entries.iter().map(|e|e["policy"].clone()).collect::<Vec<_>>(),
        "lease":90,"proposal_lease":30,"qos_lease":30,"capacity":1000,"search_budget":10000,"draw_seed":2,"native_seed":3})).unwrap()
}
fn send(c: &mut Coordinator, mut request: Value) -> Result<Value> {
    if request.get("now").is_none() {
        request["now"] = json!(100.);
    }
    if request["op"] == "session_queue_enqueue" && request.get("permit").is_none() {
        let check=c.execute(&json!({"op":"session_queue_check","now":request["now"],"generation":request["generation"],"body":request["body"]}))?;
        if check["ticket"].is_null() {
            let facts = json!({"party":null,"clans":[]});
            let planned=c.execute(&json!({"op":"party_queue_prepare","now":request["now"],"generation":request["generation"],"body":request["body"],"facts":facts,"rooms":[]}))?;
            request["permit"] = planned["permit"].clone();
            request["facts"] = facts;
        }
    }
    c.execute(&request)
}
fn body() -> Value {
    json!({"screen_name":"Pilot","maps":["lockout"],"compatibility":"halo2-native-queue-v9","language":0})
}
fn fact(provider: &str, subject: &str, player: Value) -> Value {
    json!({"owner":[provider,subject],"player":player,"separate_party":false})
}
fn admission(player: &str) -> Value {
    json!({"op":"session_register","body":body(),"identities":[fact("discord",player,json!(player))],"resolved":[player],"rooms":[]})
}
fn auth(c: &mut Coordinator, token: &Value) -> Result<Value> {
    send(
        c,
        json!({"op":"session_auth","header":format!("Bearer {}",token.as_str().unwrap())}),
    )
}
fn request_id(index: usize) -> String {
    format!("00000000-0000-0000-0000-{index:012x}")
}
fn queue(admitted: &Value, index: usize) -> Value {
    let session = &admitted["session"];
    let players = session["local_players"]
        .as_array()
        .unwrap()
        .iter()
        .map(|p| {
            json!({
        "id":p["player"],"screen_name":p["screen_name"],"guest":p["guest"],"rank":1,"raw_rank":null,
        "maps":session["maps"],"connection_owner":session["player"]})
        })
        .collect::<Vec<_>>();
    json!({"op":"session_queue_enqueue","generation":session["generation"],"members":[session["generation"]],
        "body":{"playlist_id":"native","revision":"declared-v1","request_id":request_id(index)},
        "queue":{"party_id":session["player"],"playlist_id":"native","compatibility":session["compatibility"],"players":players,
            "clan":null,"clan_rank":null,"leader":session["player"],"language":session["language"]}})
}
fn count(c: &mut Coordinator, map: &str) -> Value {
    send(c, json!({"op":"len","map":map})).unwrap()
}

#[test]
fn invalid_local_plans_cannot_admit_sessions_or_trust_guest_identities() {
    let mut c = owner();
    for bad in [
        json!(32),
        json!("a".repeat(32)),
        json!("😀".repeat(16)),
        json!("control\n"),
    ] {
        let mut r = admission("p1");
        r["body"]["screen_name"] = bad;
        assert_eq!(send(&mut c, r).unwrap_err().0, 400);
    }
    for (path, bad, status) in [
        ("maps", json!(["lockout", "lockout"]), 400),
        (
            "local_players",
            json!([{"screen_name":"Guest","guest":false}]),
            400,
        ),
        ("language", json!(true), 400),
        ("appearance", json!("000000000200000000"), 400),
        ("compatibility", json!("halo2-native-queue-v6"), 409),
    ] {
        let mut r = admission("p1");
        r["body"][path] = bad;
        assert_eq!(send(&mut c, r).unwrap_err().0, status, "{path}");
    }
    let mut r = admission("p1");
    r["body"]["local_players"] = json!([{"screen_name":"Guest"}]);
    r["identities"]
        .as_array_mut()
        .unwrap()
        .push(fact("discord", "forged", json!("forged")));
    r["resolved"].as_array_mut().unwrap().push(json!("forged"));
    assert_eq!(send(&mut c, r).unwrap_err().0, 400);
    assert_eq!(
        send(&mut c, json!({"op":"session_keys"})).unwrap(),
        json!([])
    );
    assert_eq!(count(&mut c, "tickets"), json!(0));
    assert_eq!(
        matchmaking_coordinator::value(&json!({"op":"session_queue_body","body":{
            "playlist_id":"native","revision":"declared-v1","request_id":request_id(1),"clan":"forged"}})).unwrap_err().0,
        400
    );
}
#[test]
fn canonical_alias_seats_and_live_room_owners_are_checked_before_replacement() {
    let mut c = owner();
    let mut r = admission("owner");
    r["body"]["local_players"] =
        json!([{"screen_name":"Signed","identity_token":"external-proof"}]);
    r["identities"]
        .as_array_mut()
        .unwrap()
        .push(fact("discord", "subject", json!("linked")));
    r["resolved"].as_array_mut().unwrap().push(json!("linked"));
    let admitted = send(&mut c, r.clone()).unwrap();
    let mut alias = admission("linked");
    alias["identities"][0] = fact("steam", "alias", json!("linked"));
    assert_eq!(send(&mut c, alias.clone()).unwrap_err().0, 409);
    let mut duplicate = r.clone();
    duplicate["identities"][1] = fact("steam", "alias", json!("owner"));
    assert_eq!(send(&mut c, duplicate).unwrap_err().0, 400);
    // The same physical primary can replace a signed seat through a linked grant.
    r["identities"][1] = fact("steam", "alias", json!("linked"));
    let replacement = send(&mut c, r).unwrap();
    assert_ne!(
        replacement["session"]["generation"],
        admitted["session"]["generation"]
    );
    send(
        &mut c,
        json!({"op":"session_revoke","key":replacement["session"]["key"]}),
    )
    .unwrap();
    alias["rooms"] =
        json!([{"closed":false,"phase":"running","owners":{"linked":"owner"},"active":["owner"]}]);
    assert_eq!(send(&mut c, alias.clone()).unwrap_err().0, 409);
    alias["rooms"][0]["phase"] = json!("postgame");
    assert!(send(&mut c, alias).is_ok());
}
#[test]
fn unchanged_verified_renewal_keeps_guests_queue_request_and_generation() {
    let mut c = owner();
    let mut r = admission("owner");
    r["body"]["local_players"] = json!([{"screen_name":"Guest"}]);
    r["identities"].as_array_mut().unwrap().push(Value::Null);
    r["resolved"].as_array_mut().unwrap().push(Value::Null);
    let admitted = send(&mut c, r.clone()).unwrap();
    let ticket = send(&mut c, queue(&admitted, 1)).unwrap();
    r["body"]["renew_session"] = admitted["response"]["token"].clone();
    r["body"]["appearance"] = json!("010203040105060708");
    r["now"] = json!(110.);
    let renewed = send(&mut c, r.clone()).unwrap();
    assert_eq!(renewed["response"], admitted["response"]);
    assert_eq!(
        renewed["session"]["generation"],
        admitted["session"]["generation"]
    );
    assert_eq!(
        renewed["session"]["local_players"][1],
        admitted["session"]["local_players"][1]
    );
    assert_eq!(renewed["session"]["expires"], json!(1010.));
    assert_eq!(
        send(&mut c, queue(&renewed, 1)).unwrap()["id"],
        ticket["id"]
    );
    assert_eq!(
        send(&mut c, json!({"op":"session_events"})).unwrap(),
        json!([])
    );
    // A local roster/name change rotates the generation and retires its request/ticket.
    r["body"]["local_players"][0]["screen_name"] = json!("Changed guest");
    let replaced = send(&mut c, r).unwrap();
    assert_ne!(
        replaced["session"]["generation"],
        renewed["session"]["generation"]
    );
    assert_ne!(
        replaced["session"]["local_players"][1]["player"],
        renewed["session"]["local_players"][1]["player"]
    );
    assert_eq!(
        auth(&mut c, &admitted["response"]["token"]).unwrap_err().0,
        401
    );
    assert_eq!(count(&mut c, "tickets"), json!(0));
    assert_eq!(
        send(&mut c, json!({"op":"session_request_items"})).unwrap(),
        json!([])
    );
    assert_eq!(send(&mut c, queue(&renewed, 2)).unwrap_err().0, 401);
}
#[test]
fn rejected_queue_generations_and_roster_facts_do_not_record_requests_or_draw_rng() {
    let mut c = owner();
    let a = send(&mut c, admission("owner")).unwrap();
    let original = queue(&a, 1);
    for (field, value) in [
        ("id", json!("outsider")),
        ("guest", json!(true)),
        ("maps", json!(["ascension"])),
        ("connection_owner", json!("other")),
        ("screen_name", json!("Forged")),
    ] {
        let mut bad = original.clone();
        bad["queue"]["players"][0][field] = value;
        assert_eq!(send(&mut c, bad).unwrap_err().0, 409, "{field}");
    }
    let mut bad = original.clone();
    bad["members"] = json!([a["session"]["generation"], a["session"]["generation"]]);
    assert_eq!(send(&mut c, bad).unwrap_err().0, 400);
    assert_eq!(count(&mut c, "tickets"), json!(0));
    assert_eq!(
        send(&mut c, json!({"op":"session_request_items"})).unwrap(),
        json!([])
    );
    assert_eq!(
        send(&mut c, json!({"op":"setting_get","name":"draw_seed"})).unwrap(),
        json!(2)
    );
    let ticket = send(&mut c, original.clone()).unwrap();
    assert_eq!(send(&mut c, original.clone()).unwrap()["id"], ticket["id"]);
    send(&mut c, json!({"op":"cancel","id":ticket["id"]})).unwrap();
    assert_eq!(send(&mut c, original).unwrap_err().0, 409);
    for index in 2..=64 {
        send(&mut c, queue(&a, index)).unwrap();
    }
    assert_eq!(send(&mut c, queue(&a, 65)).unwrap_err().0, 429);
    assert_eq!(count(&mut c, "tickets"), json!(1));
}
#[test]
fn expiry_clears_failures_requests_and_reservations_while_time_cannot_rewind() {
    let mut c = owner();
    let a = send(&mut c, admission("a")).unwrap();
    let b = send(&mut c, admission("b")).unwrap();
    let ta = send(&mut c, queue(&a, 1)).unwrap();
    let tb = send(&mut c, queue(&b, 1)).unwrap();
    let proposal=send(&mut c,json!({"op":"form","playlist":"native","tickets":[ta["id"],tb["id"]],"game":null,"gathering_host":"a"})).unwrap();
    send(&mut c,json!({"op":"session_failure_set","generation":a["session"]["generation"],"code":"party_changed"})).unwrap();
    // Cleanup enters through the session operation without a Python/core cleanup prelude.
    send(&mut c, json!({"op":"session_cleanup","now":1000.})).unwrap();
    for map in ["tickets", "proposals", "ready"] {
        assert_eq!(count(&mut c, map), json!(0));
    }
    assert_eq!(
        send(&mut c, json!({"op":"session_request_items"})).unwrap(),
        json!([])
    );
    assert!(
        send(&mut c, json!({"op":"session_failure_get","player":"a"}))
            .unwrap()
            .is_null()
    );
    assert_eq!(
        send(&mut c, json!({"op":"session_events"}))
            .unwrap()
            .as_array()
            .unwrap()
            .len(),
        2
    );
    assert_eq!(auth(&mut c, &a["response"]["token"]).unwrap_err().0, 401);
    assert_eq!(
        send(&mut c, json!({"op":"now","now":0.})).unwrap(),
        json!(1000.)
    );
    assert_eq!(
        send(&mut c, json!({"op":"admission","id":proposal["id"]}))
            .unwrap_err()
            .0,
        404
    );
    assert_eq!(send(&mut c,json!({"op":"session_failure_set","generation":a["session"]["generation"],"code":"search_failed"})).unwrap_err().0,401);
}
#[test]
fn expired_or_foreign_renewal_cannot_extend_an_owned_session() {
    let mut c = owner();
    let admitted = send(&mut c, admission("owner")).unwrap();
    let mut foreign = admission("other");
    foreign["body"]["renew_session"] = admitted["response"]["token"].clone();
    assert_eq!(send(&mut c, foreign).unwrap_err().0, 403);
    let mut expired = admission("owner");
    expired["body"]["renew_session"] = admitted["response"]["token"].clone();
    expired["now"] = json!(1000.);
    assert_eq!(send(&mut c, expired).unwrap_err().0, 401);
    assert_eq!(
        send(&mut c, json!({"op":"session_keys"})).unwrap(),
        json!([])
    );
    assert_eq!(
        matchmaking_coordinator::value(
            &json!({"op":"session_verified_identity","owner":["client","forged"]})
        )
        .unwrap_err(),
        Error(401, "Verified account identity required")
    );
}

#[test]
fn full_party_preflight_uses_registered_versions_maps_and_guest_policy() {
    for (first, second, expected) in [
        ("halo2-native-queue-v9", "halo2-native-queue-v10", None),
        (
            "halo2-native-queue-v11",
            "halo2-native-queue-v12",
            Some(409),
        ),
    ] {
        let mut c = owner();
        let mut ra = admission("a");
        ra["body"]["compatibility"] = json!(first);
        let mut rb = admission("b");
        rb["body"]["compatibility"] = json!(second);
        let a = send(&mut c, ra).unwrap();
        let b = send(&mut c, rb).unwrap();
        let mut plan = queue(&a, 1);
        plan["op"] = json!("session_queue_plan");
        plan["members"] = json!([a["session"]["generation"], b["session"]["generation"]]);
        match expected {
            None => assert_eq!(
                send(&mut c, plan).unwrap()["members"]
                    .as_array()
                    .unwrap()
                    .len(),
                2
            ),
            Some(status) => assert_eq!(send(&mut c, plan).unwrap_err().0, status),
        }
        assert_eq!(count(&mut c, "tickets"), json!(0));
    }
    let mut c = owner();
    let mut raw = admission("guest-owner");
    raw["body"]["local_players"] = json!([{"screen_name":"Guest"}]);
    raw["identities"].as_array_mut().unwrap().push(Value::Null);
    raw["resolved"].as_array_mut().unwrap().push(Value::Null);
    let a = send(&mut c, raw).unwrap();
    let mut entries = send(&mut c, json!({"op":"catalog_get","id":null})).unwrap();
    entries[0]["policy"]["ranked"] = json!(true);
    send(&mut c, json!({"op":"catalog_set","entries":entries})).unwrap();
    let mut plan = queue(&a, 1);
    plan["op"] = json!("session_queue_plan");
    assert_eq!(send(&mut c, plan).unwrap_err().0, 409);
    assert_eq!(
        send(&mut c, json!({"op":"session_request_items"})).unwrap(),
        json!([])
    );
}

#[test]
fn live_session_capacity_is_checked_before_resolution_and_allows_owned_replacement() {
    let mut c = owner();
    for index in 0..1000 {
        send(&mut c, admission(&format!("p{index}"))).unwrap();
    }
    let mut denied = admission("new");
    denied["op"] = json!("session_check");
    denied["identities"][0]["player"] = Value::Null;
    assert_eq!(send(&mut c, denied).unwrap_err().0, 503);
    assert_eq!(
        send(&mut c, json!({"op":"session_keys"}))
            .unwrap()
            .as_array()
            .unwrap()
            .len(),
        1000
    );
    let mut replacement = admission("p0");
    replacement["body"]["screen_name"] = json!("Fresh profile");
    let replaced = send(&mut c, replacement).unwrap();
    assert_eq!(replaced["session"]["generation"], json!(1001));
    assert_eq!(
        send(&mut c, json!({"op":"session_keys"}))
            .unwrap()
            .as_array()
            .unwrap()
            .len(),
        1000
    );
    send(
        &mut c,
        json!({"op":"session_revoke","key":replaced["session"]["key"]}),
    )
    .unwrap();
    assert_eq!(
        send(&mut c, admission("new")).unwrap()["session"]["generation"],
        json!(1002)
    );
    assert_eq!(
        send(&mut c, json!({"op":"session_keys"}))
            .unwrap()
            .as_array()
            .unwrap()
            .len(),
        1000
    );
}
