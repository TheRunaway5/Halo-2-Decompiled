//! Component controls. These supply no native Ready, provider or played world.
use ed25519_dalek::SigningKey;
use halo2_online_protocol::{
    host_agreement::{Ballot, Context, Cut, Decision, Value as HostValue},
    matchmaking_native_session::*,
    matchmaking_origin::UrlParts,
};
use serde_json::{Value, json};
use sha2::{Digest, Sha256};
fn hex(bytes: &[u8]) -> String {
    bytes.iter().map(|b| format!("{b:02x}")).collect()
}
fn config(self_peer: u8, options: Options) -> Config {
    Config {
        input_valid: true,
        origin: "http://127.0.0.1:4555".into(),
        url: UrlParts {
            parsed: true,
            normalized: "http://127.0.0.1:4555/".into(),
            scheme: "http".into(),
            host: "127.0.0.1".into(),
            user: String::new(),
            password: String::new(),
            options: String::new(),
            path: "/".into(),
        },
        room: "01234567-89ab-cdef-0123-456789abcdef".into(),
        bearer: "A".repeat(48),
        loopback: true,
        self_peer,
        seats: (0..3)
            .map(|i| Seat {
                slot: i,
                owner: i,
                guest: false,
                team: -1,
                name: format!("seat{i}"),
                identity: [i; 8],
            })
            .collect(),
        team_mode: Some(false),
        protocol: 123,
        native_extension_floor: 0x80000000,
        options,
        eos_peers: Vec::new(),
    }
}
fn text(s: &mut State, v: Value) -> Effects {
    s.text(&serde_json::to_vec(&v).unwrap(), Resources::default())
        .unwrap()
}
fn running(s: &mut State, host: u8) -> Effects {
    text(s, json!({"phase":"running","host":host}))
}
fn native() -> Options {
    Options {
        direct: true,
        peer_migration: true,
        ..Options::default()
    }
}
fn decision(context: Context) -> Decision {
    let host = 1;
    Decision {
        previous: context,
        next: Context {
            epoch: context.epoch + 1,
            membership_revision: context.membership_revision + 1,
            peers: context.peers & !(1u16 << context.host),
            host,
            ..context
        },
        ballot: Ballot {
            round: 1,
            proposer: host,
        },
        value: HostValue {
            host,
            cut: Cut {
                id: 1,
                tick: 100,
                bytes: 80,
                digest: [9; 32],
            },
        },
        voters: context.peers & !(1u16 << context.host),
    }
}
fn candidate(context: Context) -> CandidateObservation {
    let cut = decision(context).value.cut;
    let electorate = context.peers & !(1u16 << context.host);
    let ready = std::array::from_fn(|p| (electorate & (1u16 << p) != 0).then_some(cut));
    CandidateObservation {
        agreement: Some((
            context,
            halo2_online_protocol::host_agreement::State {
                suspected: true,
                fenced: false,
                electorate,
                silent: 0,
                majority: electorate.count_ones() / 2 + 1,
                ready,
                decision: None,
                rejected: 0,
            },
        )),
        quality: Some(context),
        ranked: vec![2, 1],
        selection_supported: true,
        selection_complete: true,
        selection_failed: false,
    }
}
#[test]
fn peer_capability_barrier_keeps_every_physical_member_and_exact_resource_context() {
    use halo2_online_protocol::matchmaking_native_peers::{
        COMMON_FEATURE, SELECTION_FEATURE, SNAPSHOT_FEATURE,
    };
    let mut options = native();
    options.survivor_selection = true;
    options.snapshot_candidates = true;
    let mut s = State::new(config(1, options), 0).unwrap();
    let context = running(&mut s, 0).initialize.unwrap().context;
    let all = COMMON_FEATURE | SELECTION_FEATURE | SNAPSHOT_FEATURE;
    let o = PeerResources {
        authority: Some(context),
        common: Some(context),
        selection: Some(context),
        snapshot: Some(context),
        features: [all; 16],
    };
    let p = s.peer_plan(o);
    assert!(
        p.common_supported
            && p.selection_supported
            && p.snapshot_supported
            && p.handoff
            && p.native_control
    );
    let mut missing = o;
    missing.features[2] = 0;
    let p = s.peer_plan(missing);
    assert!(!p.common_supported && !p.selection_supported && !p.snapshot_supported);
    // Own endpoint and nonmembers are not remote Hello observations.
    let mut own = o;
    own.features[1] = 0;
    own.features[15] = 0;
    assert_eq!(s.peer_plan(own), s.peer_plan(o));
    let mut stale = o;
    stale.authority.as_mut().unwrap().epoch += 1;
    let p = s.peer_plan(stale);
    assert!(!p.common_supported && !p.selection_supported && !p.snapshot_supported);
    let mut no_common = o;
    no_common.features[2] &= !COMMON_FEATURE;
    let p = s.peer_plan(no_common);
    assert!(!p.common_supported && !p.snapshot_supported && p.selection_supported);
    let mut different = o;
    different.snapshot.as_mut().unwrap().membership_revision += 1;
    assert!(!s.peer_plan(different).snapshot_supported);
}
#[test]
fn peer_worker_config_excludes_couch_seats_and_preserves_epoch_negotiation() {
    let mut c = config(0, native());
    c.seats[1].owner = 0;
    let s = State::new(c, 0).unwrap();
    let c = s.peer_config().unwrap();
    assert_eq!(c.physical, 5);
    assert_eq!(c.self_peer, 0);
    assert!(c.epochs);
    let s = State::new(config(0, Options::default()), 0).unwrap();
    assert!(s.peer_config().is_err());
    let s = State::new(
        config(
            0,
            Options {
                direct: true,
                ..Options::default()
            },
        ),
        0,
    )
    .unwrap();
    assert!(!s.peer_config().unwrap().epochs);
}
#[test]
fn peer_phase_plan_does_not_turn_capabilities_into_world_readiness() {
    use halo2_online_protocol::matchmaking_native_peers::Poll;
    let mut s = State::new(config(0, native()), 0).unwrap();
    let empty = PeerResources::default();
    let p = s.peer_plan(empty);
    assert!(p.setup);
    assert!(!p.native_control && !p.common && !p.handoff);
    running(&mut s, 0);
    let p = s.peer_plan(empty);
    assert!(!p.setup && p.common && p.native_control && p.handoff);
    assert!(p.allows(Poll::Authority) && p.allows(Poll::Admissions));
    assert!(!p.allows(Poll::Common) && !p.allows(Poll::Selection) && !p.allows(Poll::Snapshot));
    s.fail("actual owner failed");
    let p = s.peer_plan(empty);
    assert!(p.failed);
    assert!(!p.handoff && !p.common && !p.native_control);
}
#[test]
fn preparation_authorization_uses_exact_native_contexts_and_retained_identity_rows() {
    use halo2_online_protocol::{
        matchmaking_native_publication as publication, peer_source_admissions::Player,
    };
    let mut options = native();
    options.survivor_selection = true;
    let mut s = State::new(config(1, options), 0).unwrap();
    let context = running(&mut s, 0).initialize.unwrap().context;
    let resources = publication::Resources {
        agreement_context: Some(context),
        authority: Some(context),
        ..publication::Resources::default()
    };
    let rows = vec![Player {
        slot: 0,
        player: 0x10000,
        identity: [3; 12],
    }];
    let admission = publication::Admission {
        accepted: Some((context, rows.clone())),
        continuity: Some(rows),
    };
    let a = s.preparation_authorization(resources, admission.clone(), true);
    assert!(a.supported);
    assert_eq!(a.admitted, Some(context));
    assert!(
        !s.preparation_authorization(resources, admission.clone(), false)
            .supported
    );
    let mut stale = resources;
    stale.authority.as_mut().unwrap().epoch += 1;
    assert!(
        !s.preparation_authorization(stale, admission.clone(), true)
            .supported
    );
    let mut changed = admission.clone();
    changed.continuity.as_mut().unwrap()[0].identity[0] ^= 1;
    assert_eq!(
        s.preparation_authorization(resources, changed, true)
            .admitted,
        None
    );
    assert!(
        !s.preparation_authorization(publication::Resources::default(), admission, true)
            .supported
    );
}
#[test]
fn host_candidate_uses_only_actual_ready_and_same_generation_reciprocal_quality() {
    let mut s = State::new(config(1, native()), 0).unwrap();
    let context = running(&mut s, 0).initialize.unwrap().context;
    assert_eq!(s.migration_candidate(candidate(context)), Some(2));
    let mut o = candidate(context);
    o.agreement.as_mut().unwrap().1.ready[2] = None;
    assert_eq!(s.migration_candidate(o), Some(1));
    let mut o = candidate(context);
    o.quality.as_mut().unwrap().membership_revision += 1;
    assert_eq!(s.migration_candidate(o), None);
    let mut o = candidate(context);
    o.agreement.as_mut().unwrap().1.electorate = 2;
    assert_eq!(s.migration_candidate(o), None);
    let mut o = candidate(context);
    o.agreement.as_mut().unwrap().1.suspected = false;
    assert_eq!(s.migration_candidate(o), None);
    let mut o = candidate(context);
    o.agreement.as_mut().unwrap().1.decision = Some(decision(context));
    assert_eq!(s.migration_candidate(o), None);
    s.fail("native owner retired");
    assert_eq!(s.migration_candidate(candidate(context)), None);
}
#[test]
fn candidate_cannot_bypass_the_actual_fixed_survivor_selection() {
    let mut options = native();
    options.survivor_selection = true;
    let mut s = State::new(config(1, options), 0).unwrap();
    let context = running(&mut s, 0).initialize.unwrap().context;
    for failure in 0..3 {
        let mut o = candidate(context);
        match failure {
            0 => o.selection_supported = false,
            1 => o.selection_complete = false,
            _ => o.selection_failed = true,
        };
        assert_eq!(s.migration_candidate(o), None);
    }
    assert_eq!(s.migration_candidate(candidate(context)), Some(2));
}
#[test]
fn sole_survivor_requires_its_exact_ready_without_inventing_a_quality_row() {
    let mut c = config(1, native());
    c.seats.pop();
    let mut s = State::new(c, 0).unwrap();
    let context = running(&mut s, 0).initialize.unwrap().context;
    let mut o = candidate(context);
    o.ranked.clear();
    assert_eq!(s.migration_candidate(o), Some(1));
    let mut o = candidate(context);
    o.agreement.as_mut().unwrap().1.ready[1] = None;
    assert_eq!(s.migration_candidate(o), None);
}
fn preset() -> Vec<u8> {
    let v: Value = serde_json::from_str(include_str!("../data/tu15-result-presets.json")).unwrap();
    let hex = v["variants"][0]["settings"].as_str().unwrap();
    (0..304)
        .map(|i| u8::from_str_radix(&hex[2 * i..2 * i + 2], 16).unwrap())
        .collect()
}

#[test]
fn admitted_constructor_preserves_signed_ffa_teams_and_rejects_invalid_ownership() {
    let s = State::new(config(1, Options::default()), 100).unwrap();
    assert_eq!(s.view().voice_members, 7);
    assert_eq!(s.seats()[0].team, -1);
    assert!(
        s.socket_url()
            .ends_with("/relay?completion=1&departure_game_over=1")
    );
    let mut c = config(1, Options::default());
    c.seats[2].owner = 0;
    c.seats[2].guest = true;
    assert!(State::new(c, 0).is_ok());
    for bad in 0..7 {
        let mut c = config(1, Options::default());
        match bad {
            0 => c.seats[1].guest = true,
            1 => c.seats[2].slot = 8,
            2 => c.seats[2].owner = 8,
            3 => c.bearer.push('\n'),
            4 => {
                c.origin.push_str("/other");
                c.url.path = "/other".into();
            }
            5 => c.options.peer_migration = true,
            _ => c.options.eos = true,
        };
        assert!(State::new(c, 0).is_err());
    }
    let mut c = config(0, Options::default());
    c.team_mode = None;
    c.seats[2].owner = 0;
    c.seats[2].team = 1;
    assert!(State::new(c, 0).is_err());
}
#[test]
fn decoded_duplicate_fields_and_partial_controls_cannot_mutate_session() {
    let mut s = State::new(config(1, Options::default()), 0).unwrap();
    running(&mut s, 0);
    let before = s.view();
    for bytes in [
        br#"{"phase":"loading","ph\u0061se":"running","host":2}"#.as_slice(),
        br#"{"phase":"running","host":2,"extra":0}"#,
        br#"{"phase":"running","host":2.5}"#,
        br#"{"voice_auth":{"version":1,"version":1,"keys":[],"seed":""}}"#,
        br#"{"phase":"running","host":2} true"#,
    ] {
        assert!(s.text(bytes, Resources::default()).is_err());
        assert_eq!(s.view(), before);
    }
}
#[test]
fn native_session_phase_ignores_late_master_and_refuses_stale_fence_atomically() {
    let mut s = State::new(config(1, native()), 0).unwrap();
    let ctx = running(&mut s, 0).initialize.unwrap().context;
    let before = s.view();
    text(&mut s, json!({"phase":"loading","host":2}));
    assert_eq!(s.view(), before);
    let mut wrong = ctx;
    wrong.membership_revision += 1;
    assert!(s.native_fence(wrong, true, None, true, false).is_err());
    assert_eq!(s.view(), before);
    let mut bad = decision(ctx);
    bad.next.peers = 2;
    assert!(s.native_fence(ctx, true, Some(bad), false, false).is_err());
    assert_eq!(s.view(), before);
    bad = decision(ctx);
    bad.voters = 2;
    assert!(s.native_fence(ctx, true, Some(bad), false, false).is_err());
    assert_eq!(s.view(), before);
    let d = decision(ctx);
    assert!(
        s.native_fence(ctx, true, Some(d), false, false)
            .unwrap()
            .audio_fence
    );
    assert_eq!(s.view().phase, Phase::Migrating);
    assert!(s.commit_allowed(d));
    s.committed(d).unwrap();
    assert_eq!(s.view().continuity.members, 6);
    assert_eq!(s.view().continuity.epoch, 1);
    s.send(2, &packet(1));
    let current = s.take_outgoing().remove(0);
    let before = s.view();
    assert!(s.native_fence(ctx, true, None, false, false).is_err());
    assert_eq!(s.view(), before);
    assert!(s.packet_current(&current));
    s.fail("closed");
    assert!(!s.commit_allowed(d));
    text(&mut s, json!({"phase":"running","host":1}));
    assert_eq!(s.view().phase, Phase::Failed);
}
#[test]
fn continuation_requires_whole_couch_groups_and_clears_old_media() {
    let mut c = config(
        1,
        Options {
            continuity: true,
            ..Options::default()
        },
    );
    c.seats[2].owner = 0;
    c.seats[2].guest = true;
    let mut s = State::new(c, 0).unwrap();
    text(
        &mut s,
        json!({"phase":"running","host":0,"continuity":{"phase":"running","host":0,"epoch":0,"members":7}}),
    );
    let before = s.view();
    assert!(s.text(br#"{"phase":"running","host":0,"continuity":{"phase":"running","host":0,"epoch":1,"members":3}}"#,Resources::default()).is_err());
    assert_eq!(s.view(), before);
    s.send(0, &packet(1));
    text(
        &mut s,
        json!({"phase":"awaiting_world","host":null,"continuity":{"phase":"awaiting_world","host":null,"epoch":1,"members":2}}),
    );
    assert!(s.take_outgoing().is_empty());
    assert!(
        s.queue_world(
            1,
            Seal {
                epoch: 0,
                tick: 100,
                digest: "aa".repeat(32)
            }
        )
        .is_ok()
    );
    let queued: Value = serde_json::from_str(&s.take_commands()[0]).unwrap();
    assert!(queued.get("world_ack").is_some());
}
#[test]
fn world_request_uses_owned_epoch_selected_world_and_refuses_wrap_or_foreign_outcomes() {
    // Service phase/seal inputs are declared metadata, not a prepared world.
    let options = Options {
        continuity: true,
        ..Options::default()
    };
    let mut host = State::new(config(0, options), 0).unwrap();
    text(
        &mut host,
        json!({"phase":"running","host":0,"continuity":{"phase":"running","host":0,"epoch":9,"members":7}}),
    );
    let request = Seal {
        epoch: u32::MAX,
        tick: 100,
        digest: "aa".repeat(32),
    };
    host.queue_world_request(0, request.clone()).unwrap();
    let queued: Value = serde_json::from_str(&host.take_commands()[0]).unwrap();
    assert_eq!(queued["world_seal"]["epoch"], 9);
    assert!(host.queue_world_request(1, request.clone()).is_err());
    assert!(host.queue_world_request(2, request.clone()).is_err());
    assert!(host.take_commands().is_empty());
    for (self_peer, phase, kind) in [(1, "migrating", 2), (2, "rebinding", 3)] {
        let mut s = State::new(config(self_peer, options), 0).unwrap();
        text(
            &mut s,
            json!({"phase":phase,"host":1,"continuity":{"phase":phase,"host":1,"epoch":10,"members":6,"tick":100,"digest":"aa".repeat(32)}}),
        );
        let mut wrong = request.clone();
        wrong.digest = "bb".repeat(32);
        assert!(s.queue_world_request(kind, wrong).is_err());
        assert!(
            s.queue_world_request(if kind == 2 { 3 } else { 2 }, request.clone())
                .is_err()
        );
        assert!(s.take_commands().is_empty());
        s.queue_world_request(kind, request.clone()).unwrap();
        let queued: Value = serde_json::from_str(&s.take_commands()[0]).unwrap();
        assert_eq!(
            queued[if kind == 2 {
                "migration_accept"
            } else {
                "migration_rebind"
            }]["epoch"],
            10
        );
    }
    let mut s = State::new(config(1, options), 0).unwrap();
    text(
        &mut s,
        json!({"phase":"awaiting_world","host":null,"continuity":{"phase":"awaiting_world","host":null,"epoch":10,"members":6}}),
    );
    assert!(
        s.queue_world_request(
            1,
            Seal {
                epoch: u32::MAX,
                ..request.clone()
            }
        )
        .is_err()
    );
    assert!(
        s.queue_world_request(
            1,
            Seal {
                epoch: 9,
                ..request
            }
        )
        .is_ok()
    );
}
#[test]
fn exchange_session_projects_authoritative_phase_host_and_fixed_configuration() {
    let mut s = State::new(config(1, native()), 0).unwrap();
    let before = s.exchange_session();
    assert!(before.direct && !before.running && !before.candidate_phase && !before.common_phase);
    let context = running(&mut s, 0).initialize.unwrap().context;
    let active = s.exchange_session();
    assert!(active.running && active.common_phase && active.candidate_phase);
    assert_eq!((active.self_peer, active.host), (1, 0));
    assert!(!active.terminal_recovery);
    s.native_fence(context, true, None, false, false).unwrap();
    let fenced = s.exchange_session();
    assert!(!fenced.running && fenced.common_phase && fenced.candidate_phase);
    s.fail("retired");
    let failed = s.exchange_session();
    assert!(!failed.running && !failed.common_phase && !failed.candidate_phase);
}
#[test]
fn loaded_admission_hashes_exact_full_variant_and_sorted_signed_extensions() {
    let mut s = State::new(config(1, Options::default()), 0).unwrap();
    let settings = preset();
    let map = 0x1122334455667788;
    let params = [(0x80000002, -7), (4, 88), (0x80000000, 9)];
    s.loaded(map, &settings, &params).unwrap();
    let mut bytes = map.to_le_bytes().to_vec();
    bytes.extend_from_slice(&settings);
    for (id, value) in [(0x80000000u32, 9i32), (0x80000002, -7)] {
        bytes.extend_from_slice(&id.to_le_bytes());
        bytes.extend_from_slice(&value.to_le_bytes());
    }
    assert_eq!(
        s.loaded_text(),
        format!("{}:000007b", &hex(&Sha256::digest(&bytes))[..16])
    );
    let before = s.loaded_text().to_owned();
    assert!(s.loaded(map, &settings[..303], &[]).is_err());
    assert_eq!(s.loaded_text(), before);
    assert!(
        s.loaded(map, &settings, &[(0x80000000, 1), (0x80000000, 2)])
            .is_err()
    );
    assert_eq!(s.loaded_text(), before);
    assert_eq!(s.view().native_fingerprint, Some(map));
}
#[test]
fn full_result_receipt_requires_review_confirmation_and_matching_digest() {
    let mut s = State::new(config(1, Options::default()), 0).unwrap();
    running(&mut s, 1);
    let report = vec![0x91; REPORT_BYTES];
    assert!(s.complete(&report[..REPORT_BYTES - 1]).is_err());
    s.complete(&report).unwrap();
    assert!(s.confirm().is_err());
    text(&mut s, json!({"postgame":hex(&report)}));
    assert_eq!(s.postgame(), report);
    let before = s.view();
    assert!(
        s.text(
            &serde_json::to_vec(&json!({"completed":hex(&Sha256::digest(&report))})).unwrap(),
            Resources::default()
        )
        .is_err()
    );
    assert_eq!(s.view(), before);
    s.confirm().unwrap();
    text(
        &mut s,
        json!({"completed":hex(&Sha256::digest(&report)),"recorded":false}),
    );
    assert_eq!(s.view().phase, Phase::Postgame);
    assert_eq!(s.view().result_recorded, Some(false));
    text(&mut s, json!({"voice_host":1,"voice_members":6}));
    assert_eq!(s.view().presence, Some(6));
    s.close_postgame();
    assert_eq!(s.view().presence, None);
}
#[test]
fn reconnect_preserves_admitted_direct_world_but_never_revives_failed_session() {
    let mut s = State::new(config(1, native()), 0).unwrap();
    running(&mut s, 0);
    assert!(s.connection_failure(true, "expired").0);
    assert!(s.connection_failure(false, "offline").0);
    assert_eq!(s.view().phase, Phase::Running);
    s.fail("world retired");
    assert!(!s.connection_failure(true, "expired").0);
    assert_eq!(s.view().phase, Phase::Failed);
    let mut relay = State::new(config(1, Options::default()), 0).unwrap();
    running(&mut relay, 0);
    assert!(!relay.connection_failure(false, "offline").0);
    assert_eq!(relay.view().phase, Phase::Failed);
}
fn packet(kind: u8) -> Vec<u8> {
    vec![b'H', b'2', b'N', b'2', 123, 0, kind]
}
#[test]
fn bounded_gameplay_queue_host_role_epoch_and_audio_generation() {
    let mut s = State::new(config(1, native()), 0).unwrap();
    let ctx = running(&mut s, 0).initialize.unwrap().context;
    assert!(s.datagram(2, &packet(3), 0).is_err());
    assert!(s.datagram(0, &packet(3), 0).unwrap());
    assert!(!s.datagram(0, &packet(3), 1).unwrap());
    assert_eq!(s.receive().unwrap().peer, 0);
    assert!(s.receive().is_none());
    for _ in 0..512 {
        assert!(s.send(2, &packet(1)));
    }
    assert!(!s.send(2, &packet(1)));
    let frames = s.take_outgoing();
    assert_eq!(frames.len(), 512);
    assert!(
        s.native_fence(ctx, true, None, false, false)
            .unwrap()
            .audio_fence
    );
    assert!(!s.packet_current(&frames[0]));
    assert!(s.take_outgoing().is_empty());
    assert!(s.datagram(0, &packet(3), 0).is_ok());
    assert!(s.receive().is_none());
}
fn authenticate(s: &mut State, seed: [u8; 32]) {
    let public = hex(&SigningKey::from_bytes(&seed).verifying_key().to_bytes());
    text(
        s,
        json!({"voice_auth":{"version":1,"seed":hex(&seed),"keys":[public,public,public]}}),
    );
}
fn audio_frame() -> Audio {
    Audio {
        source: 1,
        sequence: 1,
        radio: false,
        left: 1.,
        right: 1.,
        opus: vec![8, 9],
        signature: None,
        generation: 0,
    }
}
#[test]
fn signed_voice_is_bound_to_actual_recipient_and_epoch_without_an_audio_device() {
    let mut sender = State::new(config(1, Options::default()), 0).unwrap();
    let mut receiver = State::new(config(2, Options::default()), 0).unwrap();
    let mut other = State::new(config(0, Options::default()), 0).unwrap();
    for s in [&mut sender, &mut receiver, &mut other] {
        running(s, 0);
        authenticate(s, [7; 32]);
    }
    assert!(sender.voice(2, &audio_frame(), false));
    let wire = sender.take_outgoing().remove(0);
    assert!(receiver.datagram(1, &wire.bytes, 0).is_ok());
    assert_eq!(receiver.take_voices()[0].audio.opus, vec![8, 9]);
    assert!(other.datagram(1, &wire.bytes, 0).is_err());
    assert!(receiver.datagram(1, &wire.bytes, 1).is_err());
    sender.bind_audio();
    sender.fence_audio(77);
    assert!(!sender.voice(2, &audio_frame(), true));
    let mut f = audio_frame();
    f.generation = 77;
    assert!(!sender.voice(2, &f, false));
    assert!(sender.voice(2, &f, true));
    sender.fence_audio(78);
    assert!(sender.take_outgoing().is_empty());
}
#[test]
fn postgame_passive_voice_uses_membership_actual_preferences_sequence_and_credit() {
    let mut s = State::new(config(1, Options::default()), 0).unwrap();
    running(&mut s, 0);
    authenticate(&mut s, [7; 32]);
    text(&mut s, json!({"postgame":hex(&vec![0;REPORT_BYTES])}));
    let mut preferences = packet(6);
    preferences.extend_from_slice(&[1, 1, 0, 0]);
    s.datagram(2, &preferences, 0).unwrap();
    s.postgame_voice(201, true, 0, 0, vec![audio_frame()], true)
        .unwrap();
    let outgoing = s.take_outgoing();
    assert_eq!(
        outgoing
            .iter()
            .filter(|p| p.bytes.get(7) == Some(&0))
            .count(),
        1
    );
    s.postgame_voice(201, true, 0, 0, vec![audio_frame()], true)
        .unwrap();
    assert!(s.take_outgoing().is_empty());
    let frames = (2..=20)
        .map(|n| Audio {
            sequence: n,
            ..audio_frame()
        })
        .collect();
    s.postgame_voice(201, true, 0, 0, frames, true).unwrap();
    assert_eq!(s.take_outgoing().len(), 9);
    s.postgame_voice(
        221,
        true,
        0,
        0,
        vec![Audio {
            sequence: 21,
            ..audio_frame()
        }],
        true,
    )
    .unwrap();
    assert_eq!(s.take_outgoing().len(), 1);
    let frames = vec![Audio {
        sequence: 22,
        ..audio_frame()
    }];
    s.postgame_voice(501, true, 1 << 2, 0, frames, true)
        .unwrap();
    assert!(s.take_outgoing().iter().all(|p| p.bytes[7] == 1));
}
#[test]
fn direct_readiness_and_terminal_retirement_use_real_observation_sets() {
    let options = Options {
        direct: true,
        terminal_recovery: true,
        ..Options::default()
    };
    let mut s = State::new(config(1, options), 0).unwrap();
    let ctx = running(&mut s, 0).initialize.unwrap().context;
    let observe = |now, failed, admitted| DirectObservation {
        now,
        authenticated: true,
        ready: 1,
        failed,
        links: vec![],
        authority: Some(ctx),
        admitted,
    };
    s.direct_observation(observe(0, 0, None)).unwrap();
    assert!(s.take_commands().is_empty());
    s.direct_observation(observe(9999, 0, None)).unwrap();
    assert!(s.take_commands().is_empty());
    s.direct_observation(observe(10000, 0, None)).unwrap();
    assert_eq!(s.take_commands().len(), 1);
    let partial = s.direct_observation(observe(10001, 1, Some(ctx))).unwrap();
    assert!(!partial.failed);
    assert!(partial.terminal.is_none());
    assert_eq!(s.view().phase, Phase::Running);
    let terminal = s.direct_observation(observe(10002, 5, Some(ctx))).unwrap();
    assert!(terminal.failed);
    assert_eq!(terminal.terminal, Some(ctx));
    assert_eq!(s.view().phase, Phase::Failed);
    assert_eq!(s.view().departed & 1, 1);
}

mod worker {
    use super::*;
    use halo2_online_protocol::{
        matchmaking_native_control as control,
        matchmaking_native_qos::{Frame, Open, Receive},
        matchmaking_native_socket::{Error, Result, Sent},
    };
    use std::{
        cell::{Cell, RefCell},
        collections::VecDeque,
    };
    struct Io {
        state: RefCell<State>,
        frames: RefCell<VecDeque<Receive>>,
        clock: Cell<u64>,
        stop: Cell<bool>,
        stop_at: Cell<u64>,
        writes: RefCell<Vec<(control::SendKind, Vec<u8>)>>,
        failures: RefCell<Vec<(bool, String)>>,
        pong_again: Cell<bool>,
        opens: Cell<usize>,
        closes: Cell<usize>,
        retry: Cell<bool>,
        texts: RefCell<Vec<Vec<u8>>>,
        binary: RefCell<Vec<Vec<u8>>>,
        fence_on_flush: Cell<bool>,
    }
    impl Io {
        fn new(direct: bool, frames: Vec<Frame>) -> Self {
            Self {
                state: RefCell::new(
                    State::new(
                        config(
                            1,
                            Options {
                                direct,
                                ..Options::default()
                            },
                        ),
                        0,
                    )
                    .unwrap(),
                ),
                frames: RefCell::new(frames.into_iter().map(Receive::Frame).collect()),
                clock: Cell::new(0),
                stop: Cell::new(false),
                stop_at: Cell::new(30),
                writes: RefCell::new(Vec::new()),
                failures: RefCell::new(Vec::new()),
                pong_again: Cell::new(false),
                opens: Cell::new(0),
                closes: Cell::new(0),
                retry: Cell::new(false),
                texts: RefCell::new(Vec::new()),
                binary: RefCell::new(Vec::new()),
                fence_on_flush: Cell::new(false),
            }
        }
        fn run(&self) {
            let worker = control::Worker::new(self.state.borrow().control_config());
            worker.run(self);
        }
    }
    impl control::Io for Io {
        fn now_ms(&self) -> u64 {
            self.clock.get()
        }
        fn cancelled(&self) -> bool {
            self.stop.get()
        }
        fn wait(&self, ms: u32) {
            self.clock.set(self.clock.get() + ms as u64);
            if self.clock.get() >= self.stop_at.get() {
                self.stop.set(true);
            }
        }
        fn open(&self, o: Open<'_>) -> Result<()> {
            assert_eq!((o.connect_ms, o.timeout_ms), (2000, 3000));
            self.opens.set(self.opens.get() + 1);
            Ok(())
        }
        fn receive(&self) -> Result<Receive> {
            Ok(self
                .frames
                .borrow_mut()
                .pop_front()
                .unwrap_or(Receive::Again))
        }
        fn send(&self, kind: control::SendKind, bytes: &[u8]) -> Result<Sent> {
            self.writes.borrow_mut().push((kind, bytes.to_vec()));
            if kind == control::SendKind::Pong && self.pong_again.replace(false) {
                return Ok(Sent {
                    again: true,
                    count: 0,
                });
            }
            Ok(Sent {
                again: false,
                count: bytes.len(),
            })
        }
        fn close(&self) {
            self.closes.set(self.closes.get() + 1);
        }
        fn text(&self, bytes: &[u8]) -> Result<Option<String>> {
            self.texts.borrow_mut().push(bytes.to_vec());
            let effects = self
                .state
                .borrow_mut()
                .text(bytes, Resources::default())
                .map_err(|_| Error::Invalid)?;
            Ok(effects.reply)
        }
        fn datagram(&self, peer: u8, epoch: u32, bytes: &[u8]) -> Result<()> {
            self.binary.borrow_mut().push(bytes.to_vec());
            self.state
                .borrow_mut()
                .datagram(peer, bytes, epoch)
                .map(|_| ())
                .map_err(|_| Error::Invalid)
        }
        fn pump(&self) {}
        fn flush(&self) -> Result<control::Flush> {
            let flush = self.state.borrow_mut().control_flush();
            if self.fence_on_flush.replace(false) {
                self.state.borrow_mut().fence_audio(9);
            }
            Ok(flush)
        }
        fn packet_current(&self, p: &Packet) -> bool {
            self.state.borrow().packet_current(p)
        }
        fn failure(&self, terminal: bool, message: &str) -> bool {
            self.failures.borrow_mut().push((terminal, message.into()));
            self.retry.get()
        }
    }
    fn text_frame(bytes: &[u8]) -> Frame {
        Frame {
            text: true,
            data: bytes.into(),
            ..Frame::default()
        }
    }
    #[test]
    fn empty_pong_again_keeps_the_unacknowledged_zero_length_write() {
        let io = Io::new(
            false,
            vec![
                Frame {
                    ping: true,
                    ..Frame::default()
                },
                text_frame(br#"{"ping":"0123456789abcdef"}"#),
            ],
        );
        io.pong_again.set(true);
        io.run();
        let writes = io.writes.borrow();
        assert_eq!(
            writes
                .iter()
                .filter(|(k, b)| *k == control::SendKind::Pong && b.is_empty())
                .count(),
            2
        );
        assert!(
            writes
                .iter()
                .any(|(k, b)| *k == control::SendKind::Text
                    && b == br#"{"pong":"0123456789abcdef"}"#)
        );
        assert!(io.failures.borrow().is_empty());
        assert_eq!(io.closes.get(), 1);
    }
    #[test]
    fn relay_drain_rechecks_actual_audio_generation_immediately_before_write() {
        let io = Io::new(false, vec![]);
        running(&mut io.state.borrow_mut(), 0);
        assert!(io.state.borrow_mut().voice_preferences(2, true, 0));
        io.fence_on_flush.set(true);
        io.run();
        assert!(
            io.writes
                .borrow()
                .iter()
                .all(|(kind, _)| *kind != control::SendKind::Binary)
        );
    }
    #[test]
    fn complete_fragmented_room_control_survives_interleaved_ping() {
        let io = Io::new(
            false,
            vec![
                Frame {
                    continued: true,
                    ..text_frame(br#"{"phase":"run"#)
                },
                Frame {
                    ping: true,
                    data: vec![1, 2],
                    ..Frame::default()
                },
                text_frame(br#"ning","host":0}"#),
            ],
        );
        io.run();
        assert_eq!(io.state.borrow().view().phase, Phase::Running);
        assert_eq!(io.texts.borrow().len(), 1);
        assert!(io.failures.borrow().is_empty());
    }
    #[test]
    fn mixed_fragment_kinds_oversize_control_and_duplicate_json_stop_before_admission() {
        let cases = vec![
            vec![
                Frame {
                    continued: true,
                    ..text_frame(b"{")
                },
                Frame {
                    binary: true,
                    data: vec![1],
                    ..Frame::default()
                },
            ],
            vec![Frame {
                bytes_left: MESSAGE_BYTES as i64 + 1,
                ..text_frame(b"{")
            }],
            vec![Frame {
                ping: true,
                continued: true,
                ..Frame::default()
            }],
            vec![Frame {
                pong: true,
                data: vec![0; 126],
                ..Frame::default()
            }],
            vec![text_frame(br#"{"phase":"running","host":0,"host":2}"#)],
        ];
        for frames in cases {
            let io = Io::new(false, frames);
            io.run();
            assert_eq!(io.failures.borrow().len(), 1);
            assert_eq!(io.state.borrow().view().phase, Phase::Connecting);
            assert_eq!(io.closes.get(), 1);
        }
    }
    #[test]
    fn actual_idle_budget_and_reconnect_are_distinct_from_cancel() {
        let io = Io::new(false, vec![]);
        io.stop_at.set(30_000);
        io.run();
        assert_eq!(io.clock.get(), 20_000);
        assert_eq!(io.failures.borrow().len(), 1);
        let io = Io::new(
            true,
            vec![Frame {
                close: true,
                data: vec![3, 233],
                ..Frame::default()
            }],
        );
        io.retry.set(true);
        io.stop_at.set(1010);
        io.run();
        assert_eq!(io.opens.get(), 2);
        assert_eq!(io.failures.borrow()[0].0, false);
        assert_eq!(io.closes.get(), 2);
        let io = Io::new(false, vec![]);
        io.stop.set(true);
        io.run();
        assert_eq!(io.opens.get(), 0);
        assert!(io.failures.borrow().is_empty());
    }
    #[test]
    fn terminal_close_reason_is_bounded_and_direct_only() {
        for direct in [false, true] {
            let mut data = vec![15, 161];
            data.extend_from_slice(b"expired");
            let io = Io::new(
                direct,
                vec![Frame {
                    close: true,
                    data,
                    ..Frame::default()
                }],
            );
            io.run();
            assert_eq!(io.failures.borrow()[0], (direct, "expired".into()));
        }
    }
    #[test]
    fn relay_binary_reaches_the_rust_host_role_queue_and_loaded_replays_once_per_connection() {
        let mut binary = vec![0];
        binary.extend_from_slice(&packet(3));
        let io = Io::new(
            false,
            vec![
                text_frame(br#"{"phase":"running","host":0}"#),
                Frame {
                    binary: true,
                    data: binary,
                    ..Frame::default()
                },
            ],
        );
        io.run();
        assert_eq!(io.state.borrow_mut().receive().unwrap().peer, 0);
        let io = Io::new(false, vec![text_frame(br#"{"phase":"loading","host":0}"#)]);
        io.state.borrow_mut().loaded(5, &preset(), &[]).unwrap();
        io.run();
        assert_eq!(
            io.writes
                .borrow()
                .iter()
                .filter(|(_, b)| b.starts_with(b"{\"loaded\":"))
                .count(),
            1
        );
    }
}

#[test]
fn immutable_local_seats_physical_peers_and_presence_views_keep_native_owner_groups() {
    let mut c = config(0, native());
    c.seats = (0..8)
        .map(|slot| Seat {
            slot,
            owner: if slot < 4 { 0 } else { 4 },
            guest: slot % 4 != 0,
            team: -1,
            name: format!("seat{slot}"),
            identity: [slot; 8],
        })
        .collect();
    let mut s = State::new(c, 0).unwrap();
    assert_eq!(s.local_slots(), [0, 1, 2, 3]);
    assert_eq!(s.physical_peers(), [0, 4]);
    assert_eq!(s.presence(), None);
    assert!(s.continuity_view().is_some());
    running(&mut s, 4);
    text(&mut s, json!({"postgame":hex(&vec![0;REPORT_BYTES])}));
    text(&mut s, json!({"voice_host":4,"voice_members":255}));
    assert_eq!(s.presence(), Some(255));
    assert_eq!(s.view().presence, s.presence());
    s.close_postgame();
    assert_eq!(s.presence(), None);
    assert_eq!(s.local_slots(), [0, 1, 2, 3]);
    assert_eq!(s.physical_peers(), [0, 4]);
    let legacy = State::new(config(0, Options::default()), 0).unwrap();
    assert!(legacy.continuity_view().is_none());
}
#[test]
fn initial_transfer_feature_contract_distinguishes_terminal_and_peer_candidates() {
    for opts in [
        native(),
        Options {
            snapshot_candidates: true,
            survivor_selection: true,
            ..native()
        },
        Options {
            direct: true,
            terminal_recovery: true,
            ..Options::default()
        },
    ] {
        let s = State::new(config(0, opts), 0).unwrap();
        let l = s.authority_limits();
        assert_eq!(l.features & 1024 != 0, opts.snapshot_candidates);
        assert_eq!(l.features & 2048 != 0, opts.survivor_selection);
        assert_eq!(l.features & 512 == 0, opts.terminal_recovery);
        assert_eq!(l.features & 4096 != 0, opts.terminal_recovery);
    }
}
