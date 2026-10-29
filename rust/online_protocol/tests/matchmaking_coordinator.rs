//! Native replacement service authority tests, not original Live proof.
use halo2_online_protocol::matchmaking_coordinator::Coordinator;
use halo2_online_protocol::matchmaking_model::{self as model, Error};
use serde_json::{Value, json};

fn policy() -> Value {
    let mut options = [0u8; 304];
    options[0x44] = 2;
    options[0x50] = 17;
    let settings = options
        .iter()
        .map(|n| format!("{n:02x}"))
        .collect::<String>();
    json!({"id":"declared-native","revision":"explicit-v1","ranked":false,"team_size":0,
        "minimum_players":2,"maximum_players":4,"maximum_party":4,
        "games":[{"id":"first","map":"lockout","variant":"Declared Slayer","weight":3,"settings":settings},
                 {"id":"second","map":"ascension","variant":"King","weight":1,"settings":null}],
        "stages":[{"after_seconds":0,"rank_fraction":[1,1],"minimum_connection":80,"unbounded_skill":false}],
        "provenance":"Native authority test; no historical service claim", "party_balance":null,
        "use_2007_rank_limits":false,"high_rank_radius":null,"mixed_party_floor":false,"custom_rank_radius":6,
        "clan_match":null,"candidate_selection":null,"gathering":null,"team_count":2,"hopper":null})
}
fn owner(p: &Value) -> Coordinator {
    Coordinator::new(
        &json!({"playlists":[p],"lease":90,"proposal_lease":30,"qos_lease":30,
        "capacity":1000,"search_budget":10000,"draw_seed":2,"native_seed":3}),
    )
    .unwrap()
}
fn send(c: &mut Coordinator, mut r: Value) -> model::Result<Value> {
    if r.get("now").is_none() {
        r["now"] = json!(100.);
    }
    c.execute(&r)
}
fn queue(c: &mut Coordinator, index: usize, count: usize, compatibility: &str) -> Value {
    let players=(0..count).map(|seat| json!({"id":format!("p{index}-{seat}"),"screen_name":"Pilot",
        "rank":10,"raw_rank":9,"guest":false,"connection_owner":null,"maps":["ascension","lockout"]})).collect::<Vec<_>>();
    send(c,json!({"op":"enqueue","party_id":format!("party-{index}"),"playlist_id":"declared-native",
        "compatibility":compatibility,"players":players,"clan":null,"clan_rank":null,"leader":null,"language":0})).unwrap()
}
fn form(c: &mut Coordinator, ids: &[Value]) -> model::Result<Value> {
    send(
        c,
        json!({"op":"form","playlist":"declared-native","tickets":ids,"game":null,"gathering_host":"p0-0"}),
    )
}
fn links(c: &mut Coordinator) {
    for (a, b) in [("p0-0", "p1-0"), ("p1-0", "p0-0")] {
        send(c, json!({"op":"connection","host":a,"player":b,"score":90})).unwrap();
    }
}
fn acknowledge(c: &mut Coordinator, id: &Value, player: &str) -> model::Result<Value> {
    send(
        c,
        json!({"op":"acknowledge","id":id,"player":player,"optimize":false,"quality":null}),
    )
}
fn size(c: &mut Coordinator, map: &str) -> Value {
    send(c, json!({"op":"len","map":map})).unwrap()
}

#[test]
fn reservations_readiness_and_consumption_have_one_owner() {
    let p = policy();
    let mut c = owner(&p);
    let a = queue(&mut c, 0, 1, "build-a");
    let b = queue(&mut c, 1, 1, "build-a");
    let ids = [a["id"].clone(), b["id"].clone()];
    let proposal = form(&mut c, &ids).unwrap();
    let id = &proposal["id"];
    assert_eq!(form(&mut c, &ids).unwrap_err().0, 409);
    let binding = send(&mut c, json!({"op":"admission","id":id})).unwrap();
    assert_eq!(binding["bound"]["game"], proposal["game"]);
    assert_eq!(binding["bound"]["tickets"], json!([a, b]));
    assert_eq!(binding["digest"].as_str().unwrap().len(), 64);
    links(&mut c);
    assert_eq!(acknowledge(&mut c, id, "outsider").unwrap_err().0, 403);
    assert!(
        send(&mut c, json!({"op":"get","map":"proposals","key":id})).unwrap()["host"].is_null(),
        "An unauthorized acknowledgment must not select a host or mutate the proposal"
    );
    assert!(
        send(&mut c, json!({"op":"get","map":"ready","key":id}))
            .unwrap()
            .as_array()
            .unwrap()
            .is_empty()
    );
    assert!(acknowledge(&mut c, id, "p0-0").unwrap().is_null());
    assert!(acknowledge(&mut c, id, "p0-0").unwrap().is_null());
    assert_eq!(
        send(&mut c, json!({"op":"get","map":"ready","key":id})).unwrap(),
        json!(["p0-0"])
    );
    assert_eq!(acknowledge(&mut c, id, "p1-0").unwrap()["id"], *id);
    assert_eq!(acknowledge(&mut c, id, "p1-0").unwrap_err().0, 404);
    for map in ["tickets", "proposals", "ready", "connections"] {
        assert_eq!(size(&mut c, map), json!(0));
    }
    assert_eq!(
        send(&mut c, json!({"op":"admission","id":id}))
            .unwrap_err()
            .0,
        404
    );
}

#[test]
fn full_profile_change_blocks_same_revision_ticket_and_ready_reuse() {
    let mut p = policy();
    let mut c = owner(&p);
    let a = queue(&mut c, 0, 1, "build-a");
    let b = queue(&mut c, 1, 1, "build-a");
    let proposal = form(&mut c, &[a["id"].clone(), b["id"].clone()]).unwrap();
    links(&mut c);
    assert!(
        acknowledge(&mut c, &proposal["id"], "p0-0")
            .unwrap()
            .is_null()
    );
    let mut bytes = model::hex(&p["games"][0]["settings"], 304).unwrap();
    bytes[0x50] ^= 1;
    p["games"][0]["settings"] = json!(bytes.iter().map(|n| format!("{n:02x}")).collect::<String>());
    send(&mut c, json!({"op":"policy_put","key":p["id"],"value":p})).unwrap();
    assert_eq!(
        send(&mut c, json!({"op":"heartbeat","id":a["id"]}))
            .unwrap_err()
            .0,
        409
    );
    assert_eq!(
        acknowledge(&mut c, &proposal["id"], "p1-0").unwrap_err().0,
        409
    );
    assert_eq!(size(&mut c, "tickets"), json!(2));
    send(&mut c, json!({"op":"cancel","id":a["id"]})).unwrap();
    assert_eq!(size(&mut c, "proposals"), json!(0));
    assert_eq!(size(&mut c, "ready"), json!(0));
    let replacement = queue(&mut c, 0, 1, "build-a");
    assert_ne!(replacement["id"], a["id"]);
}

#[test]
fn mismatched_build_or_duplicate_ticket_cannot_bypass_admission() {
    let p = policy();
    let mut c = owner(&p);
    let a = queue(&mut c, 0, 1, "build-a");
    let b = queue(&mut c, 1, 1, "build-b");
    assert_eq!(
        form(&mut c, &[a["id"].clone(), b["id"].clone()])
            .unwrap_err()
            .0,
        409
    );
    assert_eq!(
        form(&mut c, &[a["id"].clone(), a["id"].clone()])
            .unwrap_err()
            .0,
        400
    );
    assert_eq!(size(&mut c, "proposals"), json!(0));
    assert_eq!(
        send(&mut c, json!({"op":"setting_get","name":"draw_seed"})).unwrap(),
        json!(2)
    );
    assert_eq!(
        send(
            &mut c,
            json!({"op":"fits","playlist":p["id"],"tickets":[a["id"]],"ticket":b["id"],"game":{}})
        )
        .unwrap_err()
        .0,
        400
    );
}

#[test]
fn failed_assignment_leaves_hopper_seed_and_reservations_unchanged() {
    let mut p = policy();
    p["ranked"] = json!(true);
    p["team_size"] = json!(3);
    p["minimum_players"] = json!(6);
    p["maximum_players"] = json!(6);
    p["maximum_party"] = json!(3);
    p["games"][0]["settings"] = Value::Null;
    p["games"][0]["variant"] = json!("Slayer");
    let mut c = owner(&p);
    let ids = (0..3)
        .map(|i| queue(&mut c, i, 2, "build-a")["id"].clone())
        .collect::<Vec<_>>();
    assert!(form(&mut c, &ids).unwrap().is_null());
    assert_eq!(
        send(&mut c, json!({"op":"setting_get","name":"draw_seed"})).unwrap(),
        json!(2)
    );
    assert_eq!(size(&mut c, "proposals"), json!(0));
    assert_eq!(size(&mut c, "tickets"), json!(3));
}

#[test]
fn leases_and_qos_use_a_monotone_clock_and_expire_reservations() {
    let p = policy();
    let mut c = owner(&p);
    let a = queue(&mut c, 0, 1, "build-a");
    let b = queue(&mut c, 1, 1, "build-a");
    let proposal = form(&mut c, &[a["id"].clone(), b["id"].clone()]).unwrap();
    links(&mut c);
    send(&mut c, json!({"op":"cleanup","now":130.})).unwrap();
    assert_eq!(size(&mut c, "connections"), json!(0));
    assert_eq!(size(&mut c, "proposals"), json!(0));
    assert_eq!(
        acknowledge(&mut c, &proposal["id"], "p0-0").unwrap_err(),
        Error(404, "Proposal not found")
    );
    assert_eq!(
        send(&mut c, json!({"op":"now","now":1.})).unwrap(),
        json!(130.)
    );
    send(&mut c, json!({"op":"cleanup","now":190.})).unwrap();
    assert_eq!(size(&mut c, "tickets"), json!(0));
}
