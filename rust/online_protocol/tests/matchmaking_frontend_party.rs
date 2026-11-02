use halo2_online_protocol::matchmaking_frontend_party::*;
use std::cell::RefCell;
use std::collections::BTreeMap;
#[derive(Default)]
struct Reads {
    calls: RefCell<Vec<String>>,
    seats: Vec<Seat>,
    grants: BTreeMap<u32, Option<bool>>,
    fail: Option<usize>,
}
impl Reads {
    fn read(&self, label: String) -> Result<()> {
        let mut calls = self.calls.borrow_mut();
        let n = calls.len();
        calls.push(label);
        if self.fail == Some(n) {
            return Err(format!("native read {n}"));
        }
        Ok(())
    }
}
impl Io for Reads {
    fn primary_appearance(&self) -> Result<Appearance> {
        self.read("profile".into())?;
        // Reentry does not observe a borrowed mutable Rust state guard.
        assert!(members(None, self)?.is_empty());
        Ok([0xff; 9])
    }
    fn local_players(&self) -> Result<Vec<Seat>> {
        self.read("locals".into())?;
        Ok(self.seats.clone())
    }
    fn authorization(&self, controller: u32) -> Result<Option<bool>> {
        self.read(format!("auth:{controller}"))?;
        Ok(self.grants.get(&controller).copied().flatten())
    }
}
#[test]
fn no_party_reads_no_profile_or_login_and_remote_payloads_are_preserved() {
    let io = Reads::default();
    assert!(members(None, &io).unwrap().is_empty());
    let p = Member {
        handle: vec![0xff, 0],
        name: b"Same".to_vec(),
        ready: vec![0x80, 0],
        leader: true,
        online: true,
        controller: -100,
        postgame: Some(false),
        appearance: Some([0x80; 9]),
        locals: vec![
            Local {
                name: vec![0xfe, 0],
                guest: true,
                appearance: None,
            },
            Local {
                name: b"Same".to_vec(),
                guest: false,
                appearance: Some([0; 9]),
            },
        ],
        ..Member::default()
    };
    let rows = members(Some(&[p.clone()]), &io).unwrap();
    assert_eq!(rows.len(), 3);
    assert_eq!(rows[0], p);
    for (row, local) in rows[1..].iter().zip(&p.locals) {
        assert_eq!(row.handle, p.handle);
        assert_eq!(row.name, local.name);
        assert_eq!(row.guest, local.guest);
        assert_eq!(row.appearance, local.appearance);
        assert_eq!(row.controller, -2);
        assert!(!row.leader);
        assert!(row.locals.is_empty());
        assert_eq!(row.ready, p.ready);
        assert_eq!(row.postgame, Some(false));
    }
    assert!(io.calls.borrow().is_empty());
}
#[test]
fn actual_signed_seats_use_their_own_appearance_and_current_login_resource() {
    let io = Reads {
        seats: (0..4)
            .map(|controller| Seat {
                controller,
                name: vec![controller as u8, 0xff],
                appearance: [controller as u8; 9],
            })
            .collect(),
        grants: BTreeMap::from([(1, Some(true)), (2, Some(false)), (3, None)]),
        ..Reads::default()
    };
    let p = Member {
        handle: b"physical owner".to_vec(),
        name: b"Primary".to_vec(),
        is_self: true,
        leader: true,
        online: true,
        ready: b"hopper".to_vec(),
        postgame: Some(true),
        locals: vec![Local {
            name: b"unselected remote declaration".to_vec(),
            ..Local::default()
        }],
        ..Member::default()
    };
    let rows = members(Some(&[p.clone()]), &io).unwrap();
    assert_eq!(rows.len(), 4);
    assert_eq!(rows[0].appearance, Some([0xff; 9]));
    assert_eq!(rows[0].locals, p.locals);
    for (index, row) in rows[1..].iter().enumerate() {
        let controller = index + 1;
        assert_eq!(row.controller, controller as i32);
        assert_eq!(row.handle, p.handle);
        assert!(row.is_self);
        assert!(!row.leader);
        assert_eq!(row.guest, controller != 1);
        assert!(row.locals.is_empty());
        assert_eq!(row.appearance, Some([controller as u8; 9]));
        assert_eq!(row.postgame, Some(true));
    }
    assert_eq!(
        *io.calls.borrow(),
        ["profile", "locals", "auth:1", "auth:2", "auth:3"]
    );
}
#[test]
fn native_read_failures_stop_projection_without_later_reads() {
    for fail in 0..3 {
        let io = Reads {
            fail: Some(fail),
            seats: vec![Seat {
                controller: 1,
                ..Seat::default()
            }],
            ..Reads::default()
        };
        assert_eq!(
            members(
                Some(&[Member {
                    is_self: true,
                    ..Member::default()
                }]),
                &io
            ),
            Err(format!("native read {fail}"))
        );
        assert_eq!(io.calls.borrow().len(), fail + 1);
    }
}
#[test]
fn column_keeps_guest_voice_offline_ready_and_leader_precedence_and_raw_handles() {
    let mut p = Member {
        handle: vec![0xff, 0],
        online: true,
        leader: true,
        ready: b"hopper".to_vec(),
        ..Member::default()
    };
    let mut voice = Voice {
        party: true,
        joined: true,
        muted: vec![p.handle.clone()],
        talking: vec![p.handle.clone()],
        ..Voice::default()
    };
    let label = |p: &Member, v: &Voice| column(&[p.clone()], v, 26, 0, 1);
    p.guest = true;
    assert_eq!(label(&p, &voice), b"Guest");
    p.guest = false;
    assert_eq!(label(&p, &voice), b"Muted");
    voice.muted.clear();
    assert_eq!(label(&p, &voice), b"Talking");
    voice.talking.clear();
    p.is_self = true;
    voice.transmitting = true;
    assert_eq!(label(&p, &voice), b"Talking");
    voice.joined = false;
    p.online = false;
    assert_eq!(label(&p, &voice), b"Offline");
    p.online = true;
    assert_eq!(label(&p, &voice), b"Ready");
    p.ready.clear();
    assert_eq!(label(&p, &voice), b"Leader");
    p.leader = false;
    assert_eq!(label(&p, &voice), b"ONLINE");
    for (screen, row, cell) in [(0xffffffff, 0, 1), (26, 0xffffffff, 1), (26, 0, 0xffffffff)] {
        assert!(column(&[p.clone()], &voice, screen, row, cell).is_empty());
    }
}
#[test]
fn membership_notifications_use_first_handles_and_full_width_couch_counts() {
    let before = Notification {
        authenticated: true,
        busy: true,
        party: Some(vec![Membership {
            handle: b"a".to_vec(),
            local_count: 0x1_0000_0000,
        }]),
        message_empty: true,
        ..Notification::default()
    };
    let after = Notification {
        authenticated: true,
        busy: false,
        party: Some(vec![Membership {
            handle: b"b".to_vec(),
            local_count: 1,
        }]),
        invitations: vec![vec![0xff, 0]],
        message_empty: false,
    };
    assert_eq!(sounds(&before, &after), [4, 3, 8, 2]);
    assert!(sounds(&Notification::default(), &after).is_empty());
    let mut shrink = before.clone();
    shrink.busy = true;
    shrink.party.as_mut().unwrap()[0].local_count -= 1;
    assert_eq!(sounds(&before, &shrink), [4]);
    assert_eq!(sounds(&shrink, &before), [3]);
    assert!(sounds(&before, &before).is_empty());
}
#[test]
fn notice_selects_the_first_undismissed_invite_and_preserves_full_width_status() {
    let mut f = NoticeFacts {
        authenticated: true,
        signed_in: vec![0],
        invitations: vec![
            Invitation {
                id: b"old".to_vec(),
                name: b"Old".to_vec(),
            },
            Invitation {
                id: vec![0xff, 0],
                name: vec![0x80, 0],
            },
        ],
        dismissed: vec![b"old".to_vec()],
        ..NoticeFacts::default()
    };
    for status in [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 262, 263, 264, 0xffffffff] {
        f.status = status;
        let n = notice(&f).unwrap();
        assert_eq!(n.invitation, f.invitations[1]);
        assert_eq!(n.can_accept, !matches!(status, 6 | 7 | 8));
    }
    f.status = 3;
    for (busy, party, launch) in [
        (true, false, false),
        (false, true, false),
        (false, false, true),
    ] {
        f.busy = busy;
        f.party = party;
        f.launch = launch;
        let n = notice(&f).unwrap();
        assert_eq!(n.busy, busy);
        assert!(!n.can_accept);
    }
    f.authenticated = false;
    assert!(notice(&f).is_none());
    f.authenticated = true;
    f.signed_in.clear();
    assert!(notice(&f).is_none());
}
