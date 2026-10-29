//! Native result/transaction authority; this is not original Live arbitration.
use halo2_online_protocol::matchmaking_coordinator::Coordinator;
use halo2_online_protocol::matchmaking_model::Result;
use halo2_online_protocol::matchmaking_persistence::Database;
use halo2_online_protocol::security_policy::PCR_SIZE;
use serde_json::{Value, json};

fn hex(bytes: &[u8]) -> String {
    bytes.iter().map(|b| format!("{b:02x}")).collect()
}
fn send(c: &mut Coordinator, mut r: Value) -> Result<Value> {
    r["now"] = json!(100.);
    c.execute(&r)
}
fn record() -> (Coordinator, Value, Vec<u8>) {
    let mut options = [0u8; 304];
    options[0x44] = 2;
    let game = json!({"id":"slayer","map":"lockout","variant":"Explicit custom Slayer","weight":1,"settings":hex(&options)});
    let policy = json!({"id":"results","revision":"declared-v1","ranked":false,"team_size":0,
        "minimum_players":3,"maximum_players":3,"maximum_party":2,"games":[game],
        "stages":[{"after_seconds":0,"rank_fraction":[1,1],"minimum_connection":0,"unbounded_skill":false}],
        "provenance":"Declared native test; no historical service claim","party_balance":null,
        "use_2007_rank_limits":false,"high_rank_radius":null,"mixed_party_floor":false,"custom_rank_radius":6,
        "clan_match":null,"candidate_selection":null,"gathering":null,"team_count":2,"hopper":null});
    let mut c = Coordinator::new(
        &json!({"playlists":[policy],"lease":90,"proposal_lease":30,"qos_lease":30,
        "capacity":1000,"search_budget":10000,"draw_seed":2,"native_seed":3}),
    )
    .unwrap();
    let player = |id: &str, guest: bool, owner: Option<&str>| {
        json!({"id":id,"screen_name":"Pilot","rank":1,"raw_rank":0,
        "guest":guest,"connection_owner":owner,"maps":["lockout"]})
    };
    let mut tickets = Vec::new();
    for (party, players) in [
        (
            "a",
            json!([
                player("p0", false, None),
                player("guest-0", true, Some("p0"))
            ]),
        ),
        ("b", json!([player("p1", false, None)])),
    ] {
        tickets.push(send(&mut c,json!({"op":"enqueue","party_id":party,"playlist_id":"results","compatibility":"halo2-native-queue-v16",
            "players":players,"clan":null,"clan_rank":null,"leader":null,"language":0})).unwrap()["id"].clone());
    }
    let proposal=send(&mut c,json!({"op":"form","playlist":"results","tickets":tickets,"game":null,"gathering_host":"p0"})).unwrap();
    let id = proposal["id"].clone();
    let skill = json!({"raw":0,"xp":0,"configuration":"retail-launch-2004"});
    let meta=send(&mut c,json!({"op":"result_open","id":id,"context":{"proposal":id,"skill_configuration":"retail-launch-2004",
        "clan_configuration":null,"skills":{"p0":skill,"p1":skill},"clans":{},"clan_skills":{},"created_wall":123}})).unwrap();
    assert_eq!(meta["admitted_owners"], json!(["p0", "p1"]));
    let mut raw = vec![0u8; PCR_SIZE];
    raw[0..2].copy_from_slice(&[1, 1]);
    raw[0x10..0x12].copy_from_slice(&[1, 1]);
    raw[0x20..0x150].copy_from_slice(&options);
    for (slot, member) in proposal["teams"]
        .as_array()
        .unwrap()
        .iter()
        .flat_map(|t| t.as_array().unwrap())
        .enumerate()
    {
        let at = 0x370 + slot * 0x94;
        raw[at] = 1;
        let identity = meta["identities"][member.as_str().unwrap()]
            .as_str()
            .unwrap();
        for i in 0..8 {
            raw[at + 2 + i] = u8::from_str_radix(&identity[i * 2..i * 2 + 2], 16).unwrap();
        }
        let name = "Pilot"
            .encode_utf16()
            .flat_map(u16::to_le_bytes)
            .collect::<Vec<_>>();
        raw[at + 12..at + 12 + name.len()].copy_from_slice(&name);
        raw[at + 0x88] = 255;
        raw[at + 0x90] = slot as u8;
    }
    (c, id, raw)
}
fn snapshot(c: &mut Coordinator, id: &Value) -> Value {
    send(c, json!({"op":"result_snapshot","id":id})).unwrap()
}
fn submit(c: &mut Coordinator, id: &Value, raw: &[u8]) -> Value {
    send(
        c,
        json!({"op":"result_submit","id":id,"actor":"p0","report":hex(raw)}),
    )
    .unwrap()
}
fn confirm(c: &mut Coordinator, id: &Value, owner: &str, digest: &Value) -> Result<Value> {
    send(
        c,
        json!({"op":"result_confirm","id":id,"actor":owner,"digest":digest}),
    )
}

#[test]
fn frozen_admission_and_complete_options_survive_queue_consumption_and_profile_change() {
    let (mut c, id, raw) = record();
    let before = snapshot(&mut c, &id);
    for offset in 0..304 {
        let mut changed = raw.clone();
        changed[0x20 + offset] ^= 1;
        assert!(
            send(
                &mut c,
                json!({"op":"result_validate","id":id,"report":hex(&changed)})
            )
            .is_err(),
            "option byte {offset}"
        );
    }
    assert_eq!(snapshot(&mut c, &id), before);
    let mut replacement = send(
        &mut c,
        json!({"op":"get","map":"playlists","key":"results"}),
    )
    .unwrap();
    replacement["games"][0]["weight"] = json!(2);
    send(&mut c, json!({"op":"policies_set","values":[replacement]})).unwrap();
    let digest = submit(&mut c, &id, &raw);
    confirm(&mut c, &id, "p0", &digest).unwrap();
    confirm(&mut c, &id, "p1", &digest).unwrap();
    let bound = c.result_commit_context(id.as_str().unwrap()).unwrap();
    assert_eq!(bound["admission"]["proposal"]["playlist"], "results");
    assert_eq!(
        bound["admission"]["source_binding"]["bound"]["game"]["settings"],
        bound["admission"]["proposal"]["game"]["settings"]
    );
    assert_eq!(bound["admission"]["namespace"], "native-matchmaking-v1");
    assert_eq!(bound["admission"]["skills"].as_object().unwrap().len(), 2);
}
#[test]
fn membership_precedes_mutation_and_conflicts_quarantine_the_original_electorate() {
    let (mut c, id, raw) = record();
    let digest = submit(&mut c, &id, &raw);
    let before = snapshot(&mut c, &id);
    assert_eq!(
        confirm(&mut c, &id, "outsider", &json!("f".repeat(64)))
            .unwrap_err()
            .0,
        403
    );
    assert_eq!(confirm(&mut c, &id, "guest-0", &digest).unwrap_err().0, 403);
    assert_eq!(snapshot(&mut c, &id), before);
    confirm(&mut c, &id, "p0", &digest).unwrap();
    confirm(&mut c, &id, "p0", &digest).unwrap();
    assert_eq!(snapshot(&mut c, &id)["confirmations"], json!(["p0"]));
    assert_eq!(
        confirm(&mut c, &id, "p1", &json!("f".repeat(64)))
            .unwrap_err()
            .0,
        409
    );
    assert_eq!(
        snapshot(&mut c, &id)["quarantined"],
        "conflicting-confirmation"
    );
    assert!(confirm(&mut c, &id, "p1", &digest).is_err());
    assert!(c.result_commit_context(id.as_str().unwrap()).is_err());
}
#[test]
fn a_departed_owner_keeps_cast_votes_but_abstention_never_becomes_consent() {
    for cast in [false, true] {
        let (mut c, id, raw) = record();
        let digest = submit(&mut c, &id, &raw);
        if cast {
            confirm(&mut c, &id, "p1", &digest).unwrap();
        }
        send(&mut c, json!({"op":"result_revoke","id":id,"actor":"p1"})).unwrap();
        assert_eq!(confirm(&mut c, &id, "p1", &digest).unwrap_err().0, 403);
        confirm(&mut c, &id, "p0", &digest).unwrap();
        let review = send(&mut c, json!({"op":"result_review","id":id})).unwrap();
        assert_eq!(review, json!({"present":true,"recordable":cast}));
        let bound = c.result_commit_context(id.as_str().unwrap());
        assert_eq!(bound.is_ok(), cast);
        if let Ok(bound) = bound {
            assert_ne!(bound["departed_slots"], 0);
            assert_eq!(bound["authority_epoch"], 0);
        }
    }
}
#[test]
fn changed_report_quarantines_before_commit_and_cannot_rewrite_a_committed_record() {
    for committed in [false, true] {
        let (mut c, id, raw) = record();
        let digest = submit(&mut c, &id, &raw);
        confirm(&mut c, &id, "p0", &digest).unwrap();
        confirm(&mut c, &id, "p1", &digest).unwrap();
        if committed {
            send(
                &mut c,
                json!({"op":"result_committed","id":id,"digest":digest}),
            )
            .unwrap();
        }
        let mut changed = raw.clone();
        changed[0x370 + 0x92] ^= 1;
        assert_eq!(
            send(
                &mut c,
                json!({"op":"result_submit","id":id,"actor":"p0","report":hex(&changed)})
            )
            .unwrap_err()
            .0,
            409
        );
        assert_eq!(snapshot(&mut c, &id)["quarantined"].is_null(), committed);
        assert_eq!(
            c.result_commit_context(id.as_str().unwrap()).is_ok(),
            committed
        );
    }
}
#[test]
fn database_continuations_reject_stale_partial_and_replayed_facts_without_advancing() {
    let mut db=Database::new(&json!({"mode":"player_snapshot","owner":"p0","playlist":"ranked","configuration":"retail-launch-2004"})).unwrap();
    let lock = db.execute(&json!({"op":"begin"})).unwrap();
    assert_eq!(lock["atomic"], true);
    let reply = |frame: Value, rows: Value, changes: Value| json!({"op":"reply","frame":frame,"rows":rows,"changes":changes});
    assert_eq!(
        db.execute(&reply(lock["frame"].clone(), json!([]), json!([0])))
            .unwrap_err()
            .0,
        409
    );
    let read = db
        .execute(&reply(lock["frame"].clone(), json!([]), json!([1])))
        .unwrap();
    assert_eq!(
        db.execute(&reply(lock["frame"].clone(), json!([]), json!([1])))
            .unwrap_err()
            .0,
        409
    );
    assert!(
        db.execute(&reply(read["frame"].clone(), json!([[0, 0]]), json!([-1])))
            .is_err()
    );
    let done = db
        .execute(&reply(
            read["frame"].clone(),
            json!([[0, 0, "retail-launch-2004"]]),
            json!([-1]),
        ))
        .unwrap();
    assert_eq!(
        done["value"],
        json!({"raw":0,"xp":0,"configuration":"retail-launch-2004"})
    );
    assert_eq!(
        db.execute(&reply(read["frame"].clone(), json!([]), json!([-1])))
            .unwrap_err()
            .0,
        409
    );
    assert!(Database::new(&json!({"mode":"result_commit","report":"forged"})).is_err());
}
#[test]
#[cfg(not(feature = "matchmaking-test-inputs"))]
fn production_abi_rejects_synthetic_result_admissions_and_accuracy_overrides() {
    let (mut c, id, _) = record();
    assert!(send(&mut c, json!({"op":"result_fixture_accuracy","id":id})).is_err());
    assert!(
        halo2_online_protocol::matchmaking_coordinator::value(
            &json!({"op":"result_validate_fixture","context":{},"report":""})
        )
        .is_err()
    );
}
