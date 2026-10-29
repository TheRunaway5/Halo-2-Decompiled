//! Supplied catalog/native boundary checks, not historical Live reconstruction.
use halo2_online_protocol::{
    matchmaking_catalog as catalog,
    matchmaking_coordinator::{self, Coordinator},
    matchmaking_model::Result,
};
use serde_json::{Value, json};

fn document() -> Value {
    json!({"version":1,"playlists":[{"id":"native","revision":"declared-v1","name":"Native test","description":"Explicit native test policy",
        "ranked":true,"quickmatch":true,"team_size":0,"minimum_players":2,"maximum_players":2,"maximum_party":1,
        "games":[{"id":"slayer","map":"lockout","variant":"Slayer","weight":1}],
        "stages":[{"after_seconds":0,"rank_numerator":2,"rank_denominator":2,"minimum_connection":50}],
        "skill":{"kind":"custom","radius":6},"rank_configuration":"retail-launch-2004","required_compatibility":"halo2-native-queue-v10",
        "provenance":"Explicit native test; no downloaded historical configuration"}]})
}
fn entries(doc: &Value) -> Vec<Value> {
    catalog::parse(doc.to_string().as_bytes()).unwrap()
}
fn owner(entries: &[Value]) -> Coordinator {
    Coordinator::new(&json!({"playlists":entries.iter().map(|e|e["policy"].clone()).collect::<Vec<_>>(),"catalog":entries,
        "lease":90,"proposal_lease":30,"qos_lease":30,"capacity":1000,"search_budget":10000,"draw_seed":2,"native_seed":3})).unwrap()
}
fn send(c: &mut Coordinator, mut r: Value) -> Result<Value> {
    r["now"] = json!(100.);
    c.execute(&r)
}
fn enqueue(c: &mut Coordinator, index: usize, version: &str) -> Result<Value> {
    send(
        c,
        json!({"op":"enqueue","party_id":format!("party-{index}"),"playlist_id":"native","compatibility":version,
        "players":[{"id":format!("p{index}"),"screen_name":"Pilot","rank":1,"raw_rank":0,"guest":false,"connection_owner":null,"maps":["lockout"]}],
        "clan":null,"clan_rank":null,"leader":null,"language":0}),
    )
}
fn form(c: &mut Coordinator, a: &Value, b: &Value) -> Value {
    send(c,json!({"op":"form","playlist":"native","tickets":[a["id"],b["id"]],"game":null,"gathering_host":"p0"})).unwrap()
}
fn connection(id: &str, version: &str, guest: bool, clan: Value) -> Value {
    json!({"id":id,"compatibility":version,"maps":["lockout"],"seats":[{"id":id,"guest":guest,"clan":clan}]})
}
#[test]
fn strict_json_and_raw_catalog_validation_precede_profile_projection() {
    let base = document();
    let parsed = entries(&base);
    assert_eq!(
        parsed[0]["policy"]["stages"][0]["rank_fraction"],
        json!([1, 1])
    );
    assert!(parsed[0]["policy"]["games"][0]["settings"].is_null());
    for raw in [
        r#"{"a":1,"a":2}"#,
        r#"{"a":1,"\u0061":2}"#,
        r#"{"outer":[{"a":1,"a":2}]}"#,
        r#"{"number":NaN}"#,
        r#"{"number":1e999}"#,
        r#"{"bad":"\ud800"}"#,
        r#"{} {}"#,
    ] {
        assert!(
            matchmaking_coordinator::decode(raw.as_bytes()).is_err(),
            "{raw}"
        );
    }
    for (field, value) in [("version", json!(true)), ("version", json!(1.0))] {
        let mut bad = base.clone();
        bad[field] = value;
        assert!(catalog::parse(bad.to_string().as_bytes()).is_err());
    }
    for path in ["id", "revision", "name", "description", "provenance"] {
        let mut bad = base.clone();
        bad["playlists"][0][path] = json!("control\ncharacter");
        assert!(
            catalog::parse(bad.to_string().as_bytes()).is_err(),
            "{path}"
        );
    }
    let mut bad = base.clone();
    bad["playlists"][0]["games"][0]["weight"] = json!(true);
    assert!(catalog::parse(bad.to_string().as_bytes()).is_err());
    let mut bad = base.clone();
    bad["playlists"][0]["required_compatibility"] = json!("halo2-native-queue-v99");
    assert!(catalog::parse(bad.to_string().as_bytes()).is_err());
    let mut bad = base.clone();
    bad["playlists"]
        .as_array_mut()
        .unwrap()
        .push(base["playlists"][0].clone());
    assert!(catalog::parse(bad.to_string().as_bytes()).is_err());
    let mut bad = base.clone();
    bad["playlists"][0]["skill"]["high_rank_radius"] = json!(5);
    assert!(catalog::parse(bad.to_string().as_bytes()).is_err());
    assert!(catalog::parse(&vec![b' '; 1024 * 1024 + 1]).is_err());
}
#[test]
fn original_retail_clock_alias_and_explicit_native_clock_have_identical_owned_inputs() {
    let mut raw = document();
    let row = &mut raw["playlists"][0];
    row["ranked"] = json!(false);
    row["required_compatibility"] = json!("halo2-native-queue-v8");
    row["maximum_players"] = json!(8);
    row["minimum_players"] = json!(4);
    row["candidate_selection"] = json!("retail-candidates-native-relay-v1");
    row["gathering"] = json!({"retail_hopper":0});
    let alias = entries(&raw);
    raw["playlists"][0]["gathering"] = alias[0]["policy"]["gathering"].clone();
    assert_eq!(entries(&raw), alias);
    raw["playlists"][0]["minimum_players"] = json!(7);
    raw["playlists"][0]["gathering"] = json!({"retail_hopper":0});
    assert!(catalog::parse(raw.to_string().as_bytes()).is_err());
}
#[test]
fn protocol_guest_common_map_and_clan_gates_are_rust_owned() {
    let e = entries(&document())[0].clone();
    assert!(
        catalog::eligible(
            &e,
            "halo2-native-queue-v10",
            &json!([connection(
                "p0",
                "halo2-native-queue-v10",
                false,
                Value::Null
            )])
        )
        .unwrap()
    );
    assert!(
        !catalog::eligible(
            &e,
            "halo2-native-queue-v9",
            &json!([connection(
                "p0",
                "halo2-native-queue-v9",
                false,
                Value::Null
            )])
        )
        .unwrap()
    );
    assert!(
        !catalog::eligible(
            &e,
            "halo2-native-queue-v10",
            &json!([connection("g", "halo2-native-queue-v10", true, Value::Null)])
        )
        .unwrap()
    );
    let mut absent = connection("p0", "halo2-native-queue-v10", false, Value::Null);
    absent["maps"] = json!(["ascension"]);
    assert!(!catalog::eligible(&e, "halo2-native-queue-v10", &json!([absent])).unwrap());
    let versions = catalog::GATHERING;
    for a in versions {
        for b in versions {
            let expected =
                a == b || (!catalog::EXCLUSIVE.contains(&a) && !catalog::EXCLUSIVE.contains(&b));
            assert_eq!(catalog::same_continuity(&[a, b]), expected, "{a}/{b}");
        }
    }
    let rule = json!({"minimum":3,"maximum":4,"imbalance":0});
    assert_eq!(
        catalog::clan_group(&rule, &vec![json!({"clan":"accepted"}); 3]).unwrap(),
        "accepted"
    );
    assert!(catalog::clan_group(&rule, &vec![json!({"clan":"accepted"}); 2]).is_err());
    assert!(
        catalog::clan_group(
            &rule,
            &[
                json!({"clan":"accepted"}),
                json!({"clan":"other"}),
                json!({"clan":"accepted"})
            ]
        )
        .is_err()
    );
    assert!(
        catalog::clan_group(
            &rule,
            &[
                json!({"clan":"accepted"}),
                json!({"clan":null}),
                json!({"clan":"accepted"})
            ]
        )
        .is_err()
    );
}
#[test]
fn actual_queue_checks_client_gate_and_full_catalog_rank_identity_without_python() {
    let catalog = entries(&document());
    let mut c = owner(&catalog);
    assert_eq!(
        enqueue(&mut c, 0, "halo2-native-queue-v9").unwrap_err().0,
        409
    );
    assert_eq!(
        send(&mut c, json!({"op":"len","map":"tickets"})).unwrap(),
        0
    );
    let a = enqueue(&mut c, 0, "halo2-native-queue-v10").unwrap();
    let b = enqueue(&mut c, 1, "halo2-native-queue-v10").unwrap();
    let proposal = form(&mut c, &a, &b);
    let binding = send(&mut c, json!({"op":"admission","id":proposal["id"]})).unwrap();
    assert_eq!(
        binding["bound"]["profile"]["catalog"]["rank_configuration"],
        "retail-launch-2004"
    );
    assert_eq!(
        binding["bound"]["profile"]["rank_identity"]["table_sha256"],
        halo2_online_protocol::matchmaking_skill::RETAIL_DIGEST
    );
    assert_eq!(
        binding["bound"]["catalog_sha256"].as_str().unwrap().len(),
        64
    );
    let old = send(&mut c, json!({"op":"catalog_get","id":null})).unwrap();
    let mut changed = old.clone();
    changed[0]["rank_configuration"] = json!("title-update-1.5-defaults");
    send(&mut c, json!({"op":"catalog_set","entries":changed})).unwrap();
    assert_eq!(
        send(&mut c, json!({"op":"heartbeat","id":a["id"]}))
            .unwrap_err()
            .0,
        409
    );
    assert!(send(&mut c,json!({"op":"acknowledge","id":proposal["id"],"player":"p0","optimize":false,"quality":null})).is_err());
    assert_eq!(
        send(&mut c, json!({"op":"admission","id":proposal["id"]})).unwrap(),
        binding
    );
    let before = send(&mut c, json!({"op":"catalog_get","id":null})).unwrap();
    let mut invalid = before.clone();
    invalid[0]["rank_configuration"] = json!("not-a-policy");
    assert!(send(&mut c, json!({"op":"catalog_set","entries":invalid})).is_err());
    assert_eq!(
        send(&mut c, json!({"op":"catalog_get","id":null})).unwrap(),
        before
    );
}
#[test]
fn result_skill_policy_must_match_frozen_service_catalog() {
    let catalog = entries(&document());
    let mut c = owner(&catalog);
    let a = enqueue(&mut c, 0, "halo2-native-queue-v10").unwrap();
    let b = enqueue(&mut c, 1, "halo2-native-queue-v10").unwrap();
    let proposal = form(&mut c, &a, &b);
    let id = proposal["id"].clone();
    let mut context = json!({"proposal":id,"skill_configuration":null,"clan_configuration":null,"skills":{},"clans":{},"clan_skills":{},"created_wall":123});
    assert_eq!(
        send(
            &mut c,
            json!({"op":"result_open","id":id,"context":context})
        )
        .unwrap_err()
        .0,
        409
    );
    context["skill_configuration"] = json!("retail-launch-2004");
    for p in ["p0", "p1"] {
        context["skills"][p] = json!({"raw":0,"xp":0,"configuration":"retail-launch-2004"});
    }
    assert!(
        send(
            &mut c,
            json!({"op":"result_open","id":id,"context":context})
        )
        .is_ok()
    );
}

#[test]
fn gathering_io_and_query_state_share_the_enriched_catalog_identity() {
    let mut doc = document();
    doc["playlists"][0]["candidate_selection"] = json!("retail-candidates-native-relay-v1");
    doc["playlists"][0]["gathering"] = json!({"countdown_ms":vec![0;16],"timeout_ms":150000,"provenance":"Declared integration test clock"});
    let catalog = entries(&doc);
    let mut c = owner(&catalog);
    send(&mut c, json!({"op":"gather_init","seed":1})).unwrap();
    enqueue(&mut c, 0, "halo2-native-queue-v10").unwrap();
    enqueue(&mut c, 1, "halo2-native-queue-v10").unwrap();
    let mut selected = Value::Null;
    for _ in 0..10 {
        let mut event = send(
            &mut c,
            json!({"op":"gather_advance","playlist":"native","wall":1000.}),
        )
        .unwrap();
        for _ in 0..100 {
            if event["kind"] == "done" {
                selected = event["value"].clone();
                break;
            }
            assert_eq!(event["kind"], "io");
            let facts = if event["action"] == "facts" {
                json!({"ready":vec![true;event["players"].as_array().unwrap().len()],
                    "pairs":event["pairs"].as_array().unwrap().iter().map(|_|json!({"milliseconds":10,"pending":false})).collect::<Vec<_>>()})
            } else {
                json!(vec![true; event["pairs"].as_array().unwrap().len()])
            };
            event = send(
                &mut c,
                json!({"op":"gather_resume","frame":event["frame"],"result":facts}),
            )
            .unwrap();
        }
        if !selected.is_null() {
            break;
        }
    }
    assert!(
        !selected.is_null(),
        "The real Rust gathering finalizer must publish an admission with catalog metadata bound"
    );
    let binding = send(&mut c, json!({"op":"admission","id":selected["id"]})).unwrap();
    assert_eq!(
        binding["bound"]["profile"]["rank_identity"]["name"],
        "retail-launch-2004"
    );
    // A separate live query cannot resume after a metadata-only config change.
    let mut c = owner(&catalog);
    send(&mut c, json!({"op":"gather_init","seed":1})).unwrap();
    enqueue(&mut c, 0, "halo2-native-queue-v10").unwrap();
    let event = send(
        &mut c,
        json!({"op":"gather_advance","playlist":"native","wall":1000.}),
    )
    .unwrap();
    assert_eq!(event["kind"], "io");
    let mut changed = catalog;
    changed[0]["rank_configuration"] = json!("title-update-1.5-defaults");
    send(&mut c, json!({"op":"catalog_set","entries":changed})).unwrap();
    assert_eq!(send(&mut c,json!({"op":"gather_resume","frame":event["frame"],"result":{"ready":[true],"pairs":[]}})).unwrap_err().0,409);
}

#[test]
fn playlist_response_owns_rank_visibility_and_rejects_unrequested_private_level_inputs() {
    let mut all = entries(&document());
    let mut casual = all[0].clone();
    casual["policy"]["id"] = json!("casual");
    casual["policy"]["ranked"] = json!(false);
    casual["quickmatch"] = json!(false);
    all.push(casual);
    let catalog = catalog::Catalog::new(&json!(all)).unwrap();
    let facts = json!([connection(
        "p0",
        "halo2-native-queue-v10",
        false,
        Value::Null
    )]);
    let view = catalog
        .public_view("halo2-native-queue-v10", &facts, &json!({"native":4}))
        .unwrap();
    assert_eq!(catalog.display_requests(), vec![json!("native")]);
    assert_eq!(view["playlists"][0]["level"], 4);
    assert!(view["playlists"][1]["level"].is_null());
    assert!(view["playlists"][0]["eligible"].as_bool().unwrap());
    let text = view.to_string();
    assert!(!text.contains("p0"));
    for row in view["playlists"].as_array().unwrap() {
        for private in [
            "raw_rank",
            "xp",
            "rank_configuration",
            "table_sha256",
            "policy",
        ] {
            assert!(row.get(private).is_none(), "{private}");
        }
    }
    for levels in [
        json!({}),
        json!({"native":null}),
        json!({"native":4,"casual":4}),
        json!({"native":51}),
    ] {
        assert!(
            catalog
                .public_view("halo2-native-queue-v10", &facts, &levels)
                .is_err()
        );
    }
}
