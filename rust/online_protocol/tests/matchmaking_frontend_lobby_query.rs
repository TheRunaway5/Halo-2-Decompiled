use halo2_online_protocol::matchmaking_frontend_lobby_query::*;
use std::cell::RefCell;

#[derive(Default)]
struct Mock {
    f: Facts,
    calls: RefCell<Vec<String>>,
    member_reads: RefCell<Vec<Vec<Member>>>,
    local_reads: RefCell<Vec<Vec<i32>>>,
    values: std::collections::BTreeMap<u32, i32>,
    seconds: i32,
    error: Vec<u8>,
    stats: bool,
    prior: i32,
    custom: bool,
    defusal: bool,
    fail: Option<String>,
    quick_values: RefCell<Vec<(u32, i32)>>,
}
impl Mock {
    fn call(&self, s: String) -> Result<()> {
        self.calls.borrow_mut().push(s.clone());
        if self.fail.as_ref() == Some(&s) {
            Err(s)
        } else {
            Ok(())
        }
    }
    fn local() -> Self {
        let mut m = Self::default();
        m.f.mode = 1;
        m.f.multiplayer_maps.push(Map {
            counts: vec![0, 0, 2],
            ..Map::default()
        });
        m.local_reads = RefCell::new(vec![vec![0], vec![0]]);
        m
    }
}
impl Io for Mock {
    fn facts(&self) -> Result<Facts> {
        Ok(self.f.clone())
    }
    fn start_error(&self) -> Result<Vec<u8>> {
        self.call("error".into())?;
        Ok(self.error.clone())
    }
    fn seconds(&self) -> Result<i32> {
        self.call("seconds".into())?;
        Ok(self.seconds)
    }
    fn members(&self) -> Result<Vec<Member>> {
        self.call("members".into())?;
        Ok(if self.member_reads.borrow().is_empty() {
            vec![]
        } else {
            self.member_reads.borrow_mut().remove(0)
        })
    }
    fn locals(&self) -> Result<Vec<i32>> {
        self.call("locals".into())?;
        Ok(if self.local_reads.borrow().is_empty() {
            vec![]
        } else {
            self.local_reads.borrow_mut().remove(0)
        })
    }
    fn has_stats(&self) -> Result<bool> {
        self.call("stats".into())?;
        Ok(self.stats)
    }
    fn prior_count(&self) -> Result<i32> {
        self.call("prior".into())?;
        Ok(self.prior)
    }
    fn campaign_file(&self, n: u32) -> Result<Vec<u8>> {
        self.call(format!("campaign:{n}"))?;
        if n >= 15 {
            Err("campaign bounds".into())
        } else {
            Ok(b"map".to_vec())
        }
    }
    fn profile_option(&self, s: u32, p: u32) -> Result<i32> {
        self.call(format!("option:{s}:{p}"))?;
        Ok(-19)
    }
    fn variant(&self) -> Result<()> {
        self.call("variant".into())
    }
    fn defusal(&self) -> Result<bool> {
        self.call("defusal".into())?;
        Ok(self.defusal)
    }
    fn value(&self, p: u32) -> Result<i32> {
        self.call(format!("value:{p}"))?;
        Ok(*self.values.get(&p).unwrap_or(&0))
    }
    fn appearance(&self) -> Result<()> {
        self.call("appearance".into())
    }
    fn custom(&self) -> Result<bool> {
        self.call("custom".into())?;
        Ok(self.custom)
    }
    fn capacity(&self, p: u32) -> Result<i32> {
        self.call(format!("capacity:{p}"))?;
        Ok(i32::MIN)
    }
    fn quick(&self, e: u32, v: &[(u32, i32)], s: u32, t: u32) -> Result<()> {
        self.call(format!("quick:{e}:{s}:{t}"))?;
        *self.quick_values.borrow_mut() = v.to_vec();
        Ok(())
    }
}
fn online() -> Mock {
    let mut m = Mock::default();
    m.f.mode = 3;
    m.seconds = -1;
    m
}
#[test]
fn unsupported_modes_observe_no_services() {
    for mode in [i32::MIN, -1, 0, 5, 65539, i32::MAX] {
        let mut m = Mock::default();
        m.f.mode = mode;
        assert!(query(&m).unwrap().is_none());
        assert!(m.calls.borrow().is_empty());
    }
}
#[test]
fn online_keeps_unknown_idle_and_ordered_duplicate_requester() {
    let mut m = online();
    m.f.party = true;
    m.f.leader = true;
    m.f.privacy = Some(u32::MAX);
    m.f.requester = vec![0xff, 0];
    m.f.members = vec![
        Member {
            handle: vec![0xff, 0],
            leader: false,
            ..Member::default()
        },
        Member {
            handle: vec![0xff, 0],
            leader: true,
            ..Member::default()
        },
    ];
    let v = query(&m).unwrap().unwrap();
    assert_eq!(
        (
            v.input.status,
            v.input.delayed_player,
            v.input.countdown_cancelled
        ),
        (-1, 0, false)
    );
    assert_eq!(v.input.privacy, -1);
    assert_eq!(v.machines, 2);
    assert_eq!(
        *m.calls.borrow(),
        ["error", "seconds", "members", "stats", "members"]
    );
}
#[test]
fn online_roster_count_and_render_cap_have_distinct_reads() {
    let mut m = online();
    m.f.party = true;
    m.member_reads = RefCell::new(vec![
        vec![Member::default(); 21],
        (0..19)
            .map(|i| Member {
                name: vec![i],
                appearance: i % 2 == 0,
                ..Member::default()
            })
            .collect(),
    ]);
    let v = query(&m).unwrap().unwrap();
    assert_eq!(v.input.player_count, 21);
    assert_eq!(v.rows.len(), 16);
    assert_eq!(v.names.unwrap().len(), 16);
    assert_eq!(v.rows[15].source, 15);
    assert!(!v.rows[15].appearance);
    assert!(v.rows[0].appearance);
}
#[test]
fn no_party_uses_actual_local_presence_and_late_stats() {
    let mut m = online();
    m.local_reads = RefCell::new(vec![vec![7, 8, 9]]);
    m.stats = true;
    m.prior = i32::MIN;
    let v = query(&m).unwrap().unwrap();
    assert_eq!(v.machines, 1);
    assert_eq!(v.input.prior_stats, i32::MIN);
    assert_eq!(
        *m.calls.borrow(),
        [
            "error", "seconds", "members", "locals", "stats", "prior", "members"
        ]
    );
}
#[test]
fn online_countdown_and_permission_are_independent() {
    let mut m = online();
    m.seconds = 0;
    m.error = b"blocked".to_vec();
    let v = query(&m).unwrap().unwrap();
    assert_eq!(v.input.status, 10);
    assert!(v.input.countdown);
    assert!(!v.input.start_permission);
}
#[test]
fn speakers_preserve_remote_unknown_and_real_mute_facts() {
    for bits in 0..64 {
        let mut m = online();
        m.f.party = true;
        m.f.voice_party = bits & 1 != 0;
        m.f.joined = bits & 2 != 0;
        m.f.transmitting = bits & 4 != 0;
        m.f.microphone_muted = bits & 8 != 0;
        let own = bits & 16 != 0;
        let muted = bits & 32 != 0;
        if muted {
            m.f.muted.push(vec![0xff, 0]);
        }
        m.member_reads = RefCell::new(vec![
            vec![],
            vec![Member {
                handle: vec![0xff, 0],
                is_self: own,
                ..Member::default()
            }],
        ]);
        let talking = m.f.voice_party && m.f.joined && own && m.f.transmitting;
        let expected = if !own && !talking && !muted {
            3
        } else if m.f.voice_party && m.f.joined && !muted && !(own && m.f.microphone_muted) {
            u32::from(!talking)
        } else {
            2
        };
        assert_eq!(query(&m).unwrap().unwrap().speakers, [expected]);
    }
}
#[test]
fn campaign_lookups_remain_lazy_and_accept_both_path_separators() {
    let mut m = Mock::local();
    m.f.mode = 4;
    m.f.level = u32::MAX;
    assert!(query(&m).is_ok());
    assert!(!m.calls.borrow().iter().any(|s| s.starts_with("campaign:")));
    m.f.campaign_maps.push(Map {
        scenario: b"folder\\map".to_vec(),
        campaign_id: 19,
        ..Map::default()
    });
    assert!(query(&m).is_err());
    m.f.level = 0;
    let v = query(&m).unwrap().unwrap();
    assert_eq!(
        v.map,
        Some(MapRef {
            campaign: true,
            index: 0
        })
    );
    assert_eq!(v.input.max_players, 2);
    assert!(
        !m.calls
            .borrow()
            .iter()
            .any(|s| s == "defusal" || s == "value:91" || s == "custom")
    );
}
#[test]
fn local_reads_do_not_merge_team_observation_into_count() {
    let mut m = Mock::local();
    m.values.insert(91, 1);
    m.local_reads = RefCell::new(vec![vec![0, 1, 1], vec![-1]]);
    let v = query(&m).unwrap().unwrap();
    assert_eq!(v.input.player_count, 3);
    assert_eq!(v.input.status, 7);
    assert_eq!(
        m.calls
            .borrow()
            .iter()
            .filter(|s| s.as_str() == "locals")
            .count(),
        2
    );
}
#[test]
fn primary_edit_overwrites_only_primary_local_edit() {
    let mut m = Mock::local();
    m.f.edits[0] = Edit {
        valid: true,
        team: 3,
    };
    m.f.edits[3] = Edit {
        valid: true,
        team: -123,
    };
    m.f.edit = Edit {
        valid: false,
        team: -1,
    };
    let v = query(&m).unwrap().unwrap();
    assert_eq!(v.edits[0], Edit::EMPTY);
    assert_eq!(v.edits[3].team, -123);
    assert!(v.input.local_team_selection);
}
#[test]
fn negative_present_countdown_preserves_system_link_presence_guard() {
    let mut m = Mock::local();
    m.f.mode = 2;
    m.f.system_host = Some(true);
    m.f.countdown = Some(-1);
    let v = query(&m).unwrap().unwrap();
    assert!(!v.input.countdown);
    assert!(!v.input.start_permission);
    assert_eq!(v.input.status, 9);
    assert_eq!(v.input.player_count, 0);
}
#[test]
fn system_link_keeps_all_native_rows_and_machine_width() {
    let mut m = Mock::local();
    m.f.mode = 2;
    m.f.system_host = Some(false);
    m.f.system_machines = u32::MAX;
    m.f.system_players = (0..19)
        .map(|i| SystemPlayer {
            name: vec![i],
            team: i as i32 % 8,
            session_index: 100 + i as i32,
        })
        .collect();
    let v = query(&m).unwrap().unwrap();
    assert_eq!(v.rows.len(), 19);
    assert_eq!(v.speakers, vec![2; 19]);
    assert!(v.rows.iter().all(|r| r.system));
    assert_eq!(v.machines, -1);
    assert_eq!(v.input.player_count, 19);
}
#[test]
fn custom_capacity_overrides_exactly_parameter_ninety_after_all_reads() {
    let mut m = Mock::local();
    m.custom = true;
    m.values.insert(90, 9);
    let v = query(&m).unwrap().unwrap();
    assert!(v.quick);
    let values = m.quick_values.borrow();
    assert_eq!(values.len(), 112);
    assert_eq!(values[90], (90, i32::MIN));
    let calls = m.calls.borrow();
    let at = calls.iter().position(|s| s == "custom").unwrap();
    assert_eq!(
        &calls[at - 1..at + 3],
        ["value:111", "custom", "capacity:90", "quick:0:14:436213792"]
    );
}
#[test]
fn defusal_owns_two_sides_but_preserves_native_getter_reads() {
    let mut m = Mock::local();
    m.defusal = true;
    m.values.insert(13, 1);
    let v = query(&m).unwrap().unwrap();
    assert_eq!(v.team_count, 2);
    assert!(v.input.team_mode);
    assert!(!v.allow_none);
    assert!(m.calls.borrow().iter().any(|s| s == "value:13"));
}
#[test]
fn failed_native_queries_do_not_continue() {
    for failure in [
        "variant",
        "defusal",
        "value:91",
        "locals",
        "custom",
        "quick:0:14:436213792",
    ] {
        let mut m = Mock::local();
        m.fail = Some(failure.into());
        assert_eq!(query(&m).unwrap_err(), failure);
        assert_eq!(m.calls.borrow().last().unwrap(), failure);
    }
}
#[test]
fn original_ready_fixture_is_unchanged() {
    let fixture = include_str!("../../../tests/fixtures/frontend_lobby_ready_original.tsv");
    let mut n = 0;
    for line in fixture.lines() {
        let v: Vec<i32> = line
            .split_whitespace()
            .map(|v| v.parse().unwrap())
            .collect();
        assert_eq!(v.len(), 7);
        assert_eq!(ready(v[0], v[1], v[2], v[3] != 0, v[4], v[5]), v[6]);
        n += 1;
    }
    assert_eq!(n, 1014);
}
#[test]
fn team_counts_preserve_full_source_type_and_raw_byte() {
    for kind in [i32::MIN, -1, 0, 1, 2, 9, 10, 65538, i32::MAX] {
        for byte in [0, 1, 2, 8, 16, 255] {
            let mut counts = vec![byte; 16];
            for valid in [false, true] {
                assert_eq!(
                    team_count(&counts, kind, valid),
                    if valid && (1..10).contains(&kind) && byte > 1 {
                        byte as u32
                    } else {
                        1
                    }
                );
            }
            counts.clear();
            assert_eq!(team_count(&counts, kind, true), 1);
        }
    }
}

#[test]
fn original_team_count_records_are_unchanged() {
    let mut count = 0;
    for line in include_str!("../../../tests/fixtures/frontend_team_original.tsv")
        .lines()
        .filter(|line| line.starts_with("N "))
    {
        let values: Vec<i32> = line
            .split_whitespace()
            .skip(1)
            .map(|v| v.parse().unwrap())
            .collect();
        assert_eq!(values.len(), 19);
        let bytes: Vec<u8> = values[2..18]
            .iter()
            .map(|&v| u8::try_from(v).unwrap())
            .collect();
        assert_eq!(team_count(&bytes, values[1], true), values[18] as u32);
        count += 1;
    }
    assert_eq!(count, 143);
}

#[test]
fn original_signed_team_limits_cannot_alias_a_native_shift() {
    let fixture: serde_json::Value = serde_json::from_str(include_str!(
        "../../../tests/fixtures/frontend_lobby_signed_team_original.json"
    ))
    .unwrap();
    assert_eq!(fixture["function"], "19A2CE");
    let cases = fixture["cases"].as_array().unwrap();
    assert_eq!(cases.len(), 9);
    for row in cases {
        let team = row["team"].as_i64().unwrap() as i32;
        let mut m = Mock::local();
        m.f.multiplayer_maps[0].counts[2] = row["maximum"].as_u64().unwrap() as u8;
        m.values.insert(91, 1);
        m.local_reads = RefCell::new(vec![vec![team], vec![team]]);
        let result = query(&m).unwrap().unwrap();
        assert_eq!(
            result.input.status,
            row["status"].as_i64().unwrap() as i32,
            "team {team}"
        );
    }
}
