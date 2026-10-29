//! Declared native relay policy, not proof of original host election/world IO.
use halo2_online_protocol::matchmaking_continuity::Continuity;
use serde_json::{Value, json};

fn send(c: &mut Continuity, mut r: Value) -> Value {
    if r.get("now").is_none() {
        r["now"] = json!(100.);
    }
    c.execute(&r).unwrap()
}
fn state(c: &mut Continuity) -> Value {
    send(c, json!({"op":"snapshot"}))
}
fn proof(op: &str, peer: usize, epoch: u32, tick: u64, digest: &str) -> Value {
    json!({"op":op,"peer":peer,"epoch":epoch,"tick":tick,"digest":digest,"now":100.})
}
fn world(owners: Value) -> Continuity {
    let mut c = Continuity::new(&json!({"owners":owners,"host":0,"timeout":15})).unwrap();
    send(&mut c, proof("seal", 0, 0, 100, &"a".repeat(64)));
    for peer in state(&mut c)["active"].as_array().unwrap() {
        send(
            &mut c,
            proof(
                "acknowledge",
                peer.as_u64().unwrap() as usize,
                0,
                100,
                &"a".repeat(64),
            ),
        );
    }
    c
}
#[test]
fn newest_world_precedes_quality_and_failed_transitions_are_atomic() {
    let mut c = world(json!([0, 1, 2]));
    for (peer, ms) in [(1, 1), (2, 500)] {
        send(
            &mut c,
            json!({"op":"quality","peer":peer,"milliseconds":ms}),
        );
    }
    send(&mut c, proof("seal", 0, 0, 101, &"b".repeat(64)));
    send(&mut c, proof("acknowledge", 2, 0, 101, &"b".repeat(64)));
    send(&mut c, json!({"op":"disconnect","peer":0}));
    assert_eq!(
        c.control(),
        json!({"epoch":1,"host":2,"members":6,"phase":"migrating","tick":101,"digest":"b".repeat(64)})
    );
    for r in [
        proof("accept", 1, 1, 101, &"b".repeat(64)),
        proof("accept", 2, 1, 100, &"a".repeat(64)),
        proof("accept", 2, 0, 101, &"b".repeat(64)),
        json!({"op":"restore","peer":2,"now":200}),
    ] {
        let before = state(&mut c);
        assert!(c.execute(&r).is_err());
        assert_eq!(state(&mut c), before);
    }
    send(&mut c, proof("accept", 2, 1, 101, &"b".repeat(64)));
    assert_eq!(state(&mut c)["phase"], "rebinding");
    send(&mut c, proof("rebind", 1, 1, 101, &"b".repeat(64)));
    assert_eq!(
        c.control(),
        json!({"epoch":1,"host":2,"members":6,"phase":"running"})
    );
}
#[test]
fn late_latest_ack_recovers_without_rollback_and_absence_expires() {
    for arrives in [false, true] {
        let mut c = world(json!([0, 1, 2]));
        send(&mut c, proof("seal", 0, 0, u64::MAX, &"b".repeat(64)));
        send(&mut c, json!({"op":"disconnect","peer":0}));
        assert_eq!(
            c.control(),
            json!({"epoch":1,"host":null,"members":6,"phase":"awaiting_world"})
        );
        let before = state(&mut c);
        assert!(
            c.execute(&proof("acknowledge", 1, 1, u64::MAX, &"b".repeat(64)))
                .is_err()
        );
        assert_eq!(state(&mut c), before);
        if arrives {
            send(
                &mut c,
                proof("acknowledge", 2, 0, u64::MAX, &"b".repeat(64)),
            );
            assert_eq!(c.control()["tick"], json!(u64::MAX));
            assert_eq!(c.control()["host"], 2);
        } else {
            assert_eq!(send(&mut c, json!({"op":"expire","now":114.999})), false);
            assert_eq!(send(&mut c, json!({"op":"expire","now":115.})), true);
            assert_eq!(c.control()["phase"], "unavailable");
            assert_eq!(c.control()["host"], Value::Null);
        }
    }
}
#[test]
fn retries_pin_world_advance_epoch_and_rebind_timeout_only_removes_absentees() {
    let mut c = world(json!([0, 1, 2, 3]));
    send(&mut c, json!({"op":"disconnect","peer":0}));
    assert_eq!(c.control()["host"], 1);
    assert_eq!(send(&mut c, json!({"op":"expire","now":115.})), true);
    assert_eq!(c.control()["host"], 2);
    assert_eq!(c.epoch(), 2);
    let before = state(&mut c);
    assert!(
        c.execute(&proof("accept", 1, 1, 100, &"a".repeat(64)))
            .is_err()
    );
    assert_eq!(state(&mut c), before);
    send(&mut c, proof("accept", 2, 2, 100, &"a".repeat(64)));
    send(&mut c, proof("rebind", 3, 2, 100, &"a".repeat(64)));
    assert_eq!(send(&mut c, json!({"op":"expire","now":130.})), true);
    assert_eq!(
        c.control(),
        json!({"epoch":2,"host":2,"members":12,"phase":"running"})
    );
    assert_eq!(state(&mut c)["failed_candidates"], json!([]));
    assert_eq!(state(&mut c)["proofs"].as_object().unwrap().len(), 2);
}
#[test]
fn all_two_to_sixteen_slot_groups_use_physical_representatives_and_whole_departures() {
    for seats in 2..=16 {
        for width in 1..=4 {
            let owners = (0..seats)
                .map(|slot| slot / width * width)
                .collect::<Vec<_>>();
            let mut c = world(json!(owners));
            send(&mut c, json!({"op":"disconnect","peer":0}));
            let expected = (0..seats)
                .filter(|slot| owners[*slot] != 0)
                .fold(0u32, |mask, slot| mask | 1 << slot);
            assert_eq!(c.control()["members"], expected);
            if seats <= width {
                assert_eq!(c.control()["phase"], "empty");
                continue;
            }
            assert_eq!(c.control()["host"], width);
            send(&mut c, proof("accept", width, 1, 100, &"a".repeat(64)));
            for peer in (width * 2..seats).step_by(width) {
                send(&mut c, proof("rebind", peer, 1, 100, &"a".repeat(64)));
            }
            assert_eq!(c.control()["phase"], "running");
            if width > 1 && seats > width + 1 {
                assert!(
                    c.execute(&proof("seal", width + 1, 1, 101, &"b".repeat(64)))
                        .is_err()
                );
            }
        }
    }
}
#[test]
fn bounded_immutable_seals_and_strict_constructor_and_operands() {
    for owners in [
        json!([true, 1]),
        json!([0, 0, 0, 0, 0]),
        json!([0, 2]),
        json!([1, 0]),
        json!([0]),
        json!([0.0, 1]),
    ] {
        assert!(Continuity::new(&json!({"owners":owners,"host":0,"timeout":15})).is_err());
    }
    let mut c = world(json!([0, 1]));
    for tick in 101..=200 {
        send(&mut c, proof("seal", 0, 0, tick, &"a".repeat(64)));
    }
    assert_eq!(state(&mut c)["seals"].as_object().unwrap().len(), 64);
    let summary = send(&mut c, json!({"op":"state"}));
    assert_eq!(summary["epoch"], c.control()["epoch"]);
    assert!(summary.get("seals").is_none() && summary.get("proofs").is_none());
    for r in [
        proof("seal", 0, 0, 199, &"a".repeat(64)),
        proof("seal", 0, 0, 200, &"b".repeat(64)),
        proof("acknowledge", 1, 0, 100, &"a".repeat(64)),
        json!({"op":"seal","peer":0,"epoch":true,"tick":201,"digest":"a".repeat(64),"now":100}),
        proof("seal", 0, 0, 201, &"A".repeat(64)),
        json!({"op":"quality","peer":true,"milliseconds":1,"now":100}),
    ] {
        let before = state(&mut c);
        assert!(c.execute(&r).is_err());
        assert_eq!(state(&mut c), before);
    }
}
#[test]
fn missing_world_candidate_exhaustion_and_zero_quality_have_explicit_outcomes() {
    let mut c = Continuity::new(&json!({"owners":[0,1],"host":0,"timeout":15})).unwrap();
    send(&mut c, json!({"op":"disconnect","peer":0}));
    assert_eq!(c.control()["phase"], "unavailable");
    let mut c = world(json!([0, 1, 2]));
    for (peer, ms) in [(1, 0.0), (2, -0.0)] {
        send(
            &mut c,
            json!({"op":"quality","peer":peer,"milliseconds":ms}),
        );
    }
    send(&mut c, json!({"op":"disconnect","peer":0}));
    assert_eq!(c.control()["host"], 1);
    send(&mut c, json!({"op":"expire","now":115.}));
    assert_eq!(c.control()["host"], 2);
    send(&mut c, json!({"op":"expire","now":130.}));
    assert_eq!(c.control()["phase"], "unavailable");
    assert_eq!(c.control()["host"], Value::Null);
    assert_eq!(state(&mut c)["failed_candidates"], json!([1, 2]));
}
