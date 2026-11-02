use halo2_online_protocol::matchmaking_frontend_account_flow::{Entry, Frame};
use halo2_online_protocol::matchmaking_frontend_friends::*;
use halo2_online_protocol::matchmaking_frontend_party_flow::{Command, Controller, State};
use std::cell::RefCell;
fn frame(screen: u32, context: u32) -> Frame {
    Frame {
        screen,
        context,
        ..Frame::default()
    }
}
fn row(label: &[u8], destination: i32, value: i32) -> Entry {
    Entry {
        label: label.to_vec(),
        destination,
        value,
    }
}
fn friend(id: &[u8], state: &[u8]) -> Friend {
    Friend {
        id: id.to_vec(),
        name: b"same".to_vec(),
        state: state.to_vec(),
        presence: b"online".to_vec(),
        joinable: Some(true),
        context: vec![0xff, 0],
        ..Friend::default()
    }
}
struct Native<'a> {
    c: &'a Controller,
    f: RefCell<Facts>,
    next: Social,
    steam: bool,
    identity: bool,
    reply: bool,
    serial: u64,
    calls: RefCell<Vec<String>>,
    commands: RefCell<Vec<(u32, Vec<u8>, Vec<u8>)>>,
    parties: RefCell<Vec<Command>>,
    title: RefCell<Vec<u8>>,
    description: RefCell<Vec<u8>>,
    fail: Option<&'static str>,
    population: Vec<Entry>,
    after_account: Option<Vec<Frame>>,
}
impl<'a> Native<'a> {
    fn new(c: &'a Controller) -> Self {
        Self {
            c,
            f: RefCell::new(Facts {
                account_client: true,
                social_client: true,
                login_client: true,
                frame: frame(26, 0),
                frames: vec![frame(6, 0), frame(186, 0)],
                ..Facts::default()
            }),
            next: Social::default(),
            steam: false,
            identity: true,
            reply: true,
            serial: u64::MAX,
            calls: RefCell::new(vec![]),
            commands: RefCell::new(vec![]),
            parties: RefCell::new(vec![]),
            title: RefCell::new(vec![]),
            description: RefCell::new(vec![]),
            fail: None,
            population: vec![],
            after_account: None,
        }
    }
    fn call(&self, name: &str) -> Result<()> {
        let _reentry = self.c.state();
        self.calls.borrow_mut().push(name.into());
        if self.fail == Some(name) {
            Err(name.into())
        } else {
            Ok(())
        }
    }
    fn labels(&self) -> Vec<Vec<u8>> {
        self.f
            .borrow()
            .frame
            .entries
            .iter()
            .map(|p| p.label.clone())
            .collect()
    }
}
impl Io for Native<'_> {
    fn facts(&self) -> Result<Facts> {
        Ok(self.f.borrow().clone())
    }
    fn uses_steam(&self) -> Result<bool> {
        self.call("steam")?;
        Ok(self.steam)
    }
    fn identity(&self) -> Result<bool> {
        self.call("identity")?;
        Ok(self.identity)
    }
    fn pane(&self) -> Result<()> {
        self.call("pane")
    }
    fn localized(&self, index: u32) -> Result<Vec<u8>> {
        self.call("localized")?;
        Ok(vec![0xff, index as u8, 0])
    }
    fn profile_name(&self) -> Result<Vec<u8>> {
        self.call("profile")?;
        Ok(vec![0xff, 0])
    }
    fn social_snapshot(&self) -> Result<Social> {
        self.call("snapshot")?;
        Ok(self.next.clone())
    }
    fn friend_command(&self, action: &[u8], target: &[u8]) -> Result<bool> {
        self.call("friend-command")?;
        self.commands
            .borrow_mut()
            .push((0, target.to_vec(), action.to_vec()));
        Ok(self.reply)
    }
    fn party_command(&self, c: &Command) -> Result<bool> {
        self.call("party-command")?;
        self.parties.borrow_mut().push(c.clone());
        Ok(self.reply)
    }
    fn wrapped_party_command(&self, c: &Command) -> Result<()> {
        self.call("wrapped-party")?;
        self.parties.borrow_mut().push(c.clone());
        Ok(())
    }
    fn command_serial(&self) -> Result<u64> {
        self.call("serial")?;
        Ok(self.serial)
    }
    fn update_account(&self) -> Result<()> {
        self.call("update-account")?;
        if let Some(frames) = &self.after_account {
            let mut f = self.f.borrow_mut();
            f.frames = frames.clone();
            f.frame = f.frames.last().cloned().unwrap_or_default();
        }
        Ok(())
    }
    fn social_command(&self, kind: u32, target: &[u8], action: &[u8]) -> Result<bool> {
        self.call("social-command")?;
        self.commands
            .borrow_mut()
            .push((kind, target.to_vec(), action.to_vec()));
        Ok(self.reply)
    }
    fn effect(&self, e: Effect) -> Result<()> {
        self.call(match &e {
            Effect::CommitSocial => "commit",
            Effect::BindAccount => "bind",
            Effect::Presence(_) => "presence",
            Effect::Pump => "pump",
            Effect::Unavailable(_) => "unavailable",
            Effect::Back => "back",
            Effect::Revision => "revision",
            _ => "effect",
        })?;
        let mut f = self.f.borrow_mut();
        match e {
            Effect::Title(v) => *self.title.borrow_mut() = v,
            Effect::Description(v) => *self.description.borrow_mut() = v,
            Effect::Append(e) => f.frame.entries.push(e),
            Effect::Replace(rows) => f.frame.entries = rows,
            Effect::CommitSocial => f.social = self.next.clone(),
            Effect::StageEmptySocial => f.social = Social::default(),
            Effect::Open(s, c) => {
                let new = frame(s, c);
                f.frames.push(new.clone());
                f.frame = new;
            }
            Effect::Back | Effect::Pop => {
                f.frames.pop();
                if let Some(top) = f.frames.last() {
                    f.frame = top.clone();
                }
            }
            Effect::ClearIndex(i) => f.frames[i].entries.clear(),
            Effect::PopulateIndex(i) => f.frames[i].entries = self.population.clone(),
            Effect::SelectIndex(i, n) => f.frames[i].selected = n,
            _ => (),
        };
        Ok(())
    }
}
#[test]
fn provider_and_availability_short_circuits_preserve_order() {
    let c = Controller::default();
    let mut io = Native::new(&c);
    io.f.borrow_mut().account.available = true;
    assert!(native(&io).unwrap());
    assert!(io.calls.borrow().is_empty());
    io.f.borrow_mut().account.available = false;
    io.f.borrow_mut().login_client = false;
    assert!(!native(&io).unwrap());
    assert!(io.calls.borrow().is_empty());
    io.f.borrow_mut().login_client = true;
    io.steam = true;
    assert!(native(&io).unwrap());
    assert_eq!(*io.calls.borrow(), ["steam"]);
    io.calls.borrow_mut().clear();
    io.identity = false;
    io.fail = Some("steam");
    assert!(!available(&io).unwrap());
    assert_eq!(*io.calls.borrow(), ["identity"]);
    io.identity = true;
    io.f.borrow_mut().social = Social {
        available: true,
        enabled: true,
        ready: true,
        ..Social::default()
    };
    assert!(available(&io).unwrap());
}
#[test]
fn bind_targets_clear_after_snapshot_and_before_account_binding() {
    let c = Controller::from_state(State {
        friend_target: vec![0xff, 0],
        activity_target: b"activity".to_vec(),
        ..State::default()
    });
    let mut io = Native::new(&c);
    io.fail = Some("snapshot");
    assert!(bind(&c, &io).is_err());
    assert_eq!(c.state().friend_target, [0xff, 0]);
    io.fail = Some("bind");
    io.next.revision = u64::MAX;
    assert!(bind(&c, &io).is_err());
    assert!(c.state().friend_target.is_empty() && c.state().activity_target.is_empty());
    assert_eq!(io.f.borrow().social.revision, u64::MAX);
}
#[test]
fn native_lists_append_and_keep_blocked_rows_separate() {
    let c = Controller::default();
    let io = Native::new(&c);
    {
        let mut f = io.f.borrow_mut();
        f.account.available = true;
        f.account.friends = vec![friend(b"a", b"friend"), friend(b"b", b"blocked")];
        f.frame.entries = vec![row(b"existing", -1, 0)];
    }
    populate_native(&c, &io).unwrap();
    assert_eq!(
        io.labels(),
        [
            b"existing".to_vec(),
            b"Find Player by Friend Tag".to_vec(),
            b"same".to_vec()
        ]
    );
    {
        let mut f = io.f.borrow_mut();
        f.frame = frame(216, 43);
    }
    populate_native(&c, &io).unwrap();
    assert_eq!(
        io.labels(),
        [
            b"Find Player by Friend Tag".to_vec(),
            b"Blocked: same".to_vec(),
            b"Party Members".to_vec(),
            b"Back".to_vec()
        ]
    );
}
#[test]
fn native_empty_placeholder_observes_actual_appended_entry_count() {
    let c = Controller::default();
    let io = Native::new(&c);
    populate_native(&c, &io).unwrap();
    assert_eq!(io.labels()[1], b"Friends are unavailable");
    io.f.borrow_mut().frame.entries = vec![row(b"existing", -1, 0)];
    populate_native(&c, &io).unwrap();
    assert_eq!(io.labels().len(), 2);
}
#[test]
fn join_menu_requires_every_observed_gate_and_nonempty_context() {
    for bit in 0..12 {
        let c = Controller::from_state(State {
            friend_target: b"id".to_vec(),
            ..State::default()
        });
        let io = Native::new(&c);
        {
            let mut f = io.f.borrow_mut();
            f.frame = frame(31, 43);
            f.account.friends = vec![friend(b"id", b"friend")];
            match bit {
                1 => f.frame.context = 46,
                2 => f.account.busy = true,
                3 => f.account.party = true,
                4 => f.launch = true,
                5..=9 => f.status = (bit - 1) as u32,
                10 => f.account.friends[0].joinable = None,
                11 => f.account.friends[0].context.clear(),
                _ => (),
            }
        }
        populate_native(&c, &io).unwrap();
        assert_eq!(
            io.labels().iter().any(|l| l == b"Join Party"),
            bit == 0,
            "gate {bit}"
        );
    }
}
#[test]
fn search_projection_uses_first_relation_for_duplicate_identity() {
    let c = Controller::default();
    let io = Native::new(&c);
    {
        let mut f = io.f.borrow_mut();
        f.frame = frame(31, 46);
        f.account.search = Some(friend(b"id", b"friend"));
        f.account.friends = vec![friend(b"id", b"incoming"), friend(b"id", b"friend")];
    }
    populate_native(&c, &io).unwrap();
    assert_eq!(
        io.labels()[..2],
        [
            b"Accept Friend Request".to_vec(),
            b"Decline Friend Request".to_vec()
        ]
    );
    assert!(!io.labels().iter().any(|l| l == b"Join Party"));
}
#[test]
fn direct_join_carries_raw_context_and_serial_without_fabricating_membership() {
    let c = Controller::from_state(State {
        friend_target: vec![0xff, 0],
        ..State::default()
    });
    let io = Native::new(&c);
    {
        let mut f = io.f.borrow_mut();
        f.frame = frame(31, 43);
        f.frame.entries = vec![row(b"Join Party", -83, 8)];
        f.account.friends = vec![friend(&[0xff, 0], b"friend")];
    }
    assert!(accept_native(&c, &io).unwrap());
    assert_eq!(c.state().friend_join_serial, u64::MAX);
    assert!(!io.f.borrow().account.party);
    let p = io.parties.borrow();
    assert_eq!(p[0].action, b"join_friend");
    assert_eq!(p[0].join_context, [0xff, 0]);
    assert_eq!(p[0].value, [0xff, 0]);
}
#[test]
fn direct_join_retains_the_callers_distinct_empty_context_admission() {
    let c = Controller::from_state(State {
        friend_target: b"id".to_vec(),
        ..State::default()
    });
    let mut io = Native::new(&c);
    {
        let mut f = io.f.borrow_mut();
        f.frame = frame(31, 43);
        f.frame.entries = vec![row(b"Join", -83, 8)];
        let mut p = friend(b"id", b"friend");
        p.context.clear();
        f.account.friends = vec![p];
    }
    io.reply = false;
    accept_native(&c, &io).unwrap();
    assert!(io.parties.borrow()[0].join_context.is_empty());
    assert_eq!(c.state().friend_join_serial, 0);
    assert!(io.calls.borrow().iter().any(|s| s == "unavailable"));
}
#[test]
fn join_serial_write_is_after_snapshot_and_before_account_refresh() {
    let c = Controller::from_state(State {
        friend_target: b"id".to_vec(),
        friend_join_serial: 3,
        ..State::default()
    });
    let mut io = Native::new(&c);
    {
        let mut f = io.f.borrow_mut();
        f.frame = frame(31, 43);
        f.frame.entries = vec![row(b"Join", -83, 8)];
        f.account.friends = vec![friend(b"id", b"friend")];
    }
    io.fail = Some("serial");
    assert!(accept_native(&c, &io).is_err());
    assert_eq!(c.state().friend_join_serial, 3);
    io.fail = Some("update-account");
    assert!(accept_native(&c, &io).is_err());
    assert_eq!(c.state().friend_join_serial, u64::MAX);
    assert!(!io.calls.borrow().iter().any(|s| s == "revision"));
}
#[test]
fn native_friend_actions_keep_seven_commands_and_wrapped_party_invites() {
    let actions = [
        &b"request"[..],
        b"accept",
        b"decline",
        b"cancel",
        b"remove",
        b"block",
        b"unblock",
    ];
    for value in 0..8 {
        let c = Controller::from_state(State {
            friend_target: b"id".to_vec(),
            ..State::default()
        });
        let io = Native::new(&c);
        io.f.borrow_mut().frame = Frame {
            entries: vec![row(b"action", -83, value)],
            ..frame(31, 43)
        };
        accept_native(&c, &io).unwrap();
        if value == 7 {
            assert_eq!(io.parties.borrow()[0].action, b"invite_friend");
            assert!(io.calls.borrow().iter().any(|s| s == "wrapped-party"));
        } else {
            assert_eq!(io.commands.borrow()[0].2, actions[value as usize]);
            assert!(io.calls.borrow().iter().any(|s| s == "back"));
        }
    }
}
#[test]
fn selected_native_friend_target_is_owned_by_rust_before_open() {
    let c = Controller::default();
    let mut io = Native::new(&c);
    {
        let mut f = io.f.borrow_mut();
        f.frame.entries = vec![row(b"friend", 31, 0)];
        f.account.friends = vec![friend(&[0xff, 0], b"friend")];
    }
    io.fail = Some("effect");
    assert!(accept_native(&c, &io).is_err());
    assert_eq!(c.state().friend_target, [0xff, 0]);
}
#[test]
fn social_lists_preserve_invitation_encoding_and_actual_friend_indices() {
    let c = Controller::default();
    let io = Native::new(&c);
    {
        let mut f = io.f.borrow_mut();
        f.social = Social {
            available: true,
            enabled: true,
            ready: true,
            invitations: vec![Invite {
                handle: b"i".to_vec(),
                name: b"same".to_vec(),
                ..Invite::default()
            }],
            friends: vec![
                SocialFriend {
                    kind: 3,
                    ..SocialFriend::default()
                },
                SocialFriend {
                    name: b"same".to_vec(),
                    kind: 1,
                    ..SocialFriend::default()
                },
            ],
            ..Social::default()
        };
    }
    populate(&c, &io).unwrap();
    let f = io.f.borrow();
    assert_eq!(
        f.frame.entries.iter().map(|e| e.value).collect::<Vec<_>>(),
        [-1, 1]
    );
    assert_eq!(f.frame.entries[1].label, b"Request: same");
}
#[test]
fn activity_panel_bypasses_native_listing_and_retains_busy_dismiss_action() {
    let c = Controller::from_state(State {
        activity_target: b"i".to_vec(),
        ..State::default()
    });
    let mut io = Native::new(&c);
    io.steam = true;
    {
        let mut f = io.f.borrow_mut();
        f.account.available = true;
        f.frame = frame(31, 44);
        f.social.busy = true;
        f.social.invitations = vec![Invite {
            handle: b"i".to_vec(),
            name: b"same".to_vec(),
            ..Invite::default()
        }];
    }
    populate(&c, &io).unwrap();
    assert_eq!(*io.title.borrow(), b"GAME INVITATION");
    assert_eq!(io.labels()[0], b"Accepting Invitation...");
    assert_eq!(io.f.borrow().frame.entries[0].value, 0);
    assert_eq!(io.f.borrow().frame.entries[1].value, 8);
    assert_eq!(
        io.calls
            .borrow()
            .iter()
            .filter(|s| s.as_str() == "steam")
            .count(),
        1
    );
}
#[test]
fn social_presence_and_snapshot_still_run_for_unchanged_revision() {
    let c = Controller::default();
    let io = Native::new(&c);
    update(&c, &io).unwrap();
    assert_eq!(
        *io.calls.borrow(),
        ["profile", "presence", "pump", "steam", "snapshot"]
    );
    io.calls.borrow_mut().clear();
    io.f.borrow_mut().social_client = false;
    update(&c, &io).unwrap();
    assert!(io.calls.borrow().is_empty());
}
#[test]
fn roster_focus_uses_opaque_previous_identity_and_the_last_duplicate_match() {
    let c = Controller::default();
    let mut io = Native::new(&c);
    {
        let mut f = io.f.borrow_mut();
        f.social.revision = 1;
        f.social.friends = vec![SocialFriend {
            handle: vec![0xff, 0],
            ..SocialFriend::default()
        }];
        f.frames = vec![Frame {
            entries: vec![row(b"same", 31, 0)],
            ..frame(26, 0)
        }];
    }
    io.next = Social {
        revision: 2,
        friends: vec![
            SocialFriend {
                handle: b"other".to_vec(),
                ..SocialFriend::default()
            },
            SocialFriend {
                handle: vec![0xff, 0],
                ..SocialFriend::default()
            },
        ],
        ..Social::default()
    };
    io.population = vec![
        row(b"same", 31, 1),
        row(b"same", 31, 0),
        row(b"same", 31, 1),
    ];
    update(&c, &io).unwrap();
    assert_eq!(io.f.borrow().frames[0].selected, 2);
}
#[test]
fn negative_invitation_selection_and_int_min_refusal_do_not_alias() {
    let c = Controller::default();
    let io = Native::new(&c);
    {
        let mut f = io.f.borrow_mut();
        f.frame.entries = vec![row(b"invite", 31, -1)];
        f.social.invitations = vec![Invite {
            handle: vec![0xff, 0],
            ..Invite::default()
        }];
    }
    accept(&c, &io).unwrap();
    assert_eq!(c.state().activity_target, [0xff, 0]);
    io.f.borrow_mut().frame = Frame {
        entries: vec![row(b"invalid", 31, i32::MIN)],
        ..frame(26, 0)
    };
    accept(&c, &io).unwrap();
    assert_eq!(c.state().activity_target, [0xff, 0]);
    assert!(column(&io, 0, 1).unwrap().is_empty());
}
#[test]
fn native_and_social_text_read_the_pane_even_for_nonlocalized_indices() {
    let c = Controller::default();
    let mut io = Native::new(&c);
    io.f.borrow_mut().account.available = true;
    io.f.borrow_mut().account.tag = vec![0xff, 0];
    assert_eq!(text(&c, &io, 2).unwrap(), [0xff, 0]);
    assert_eq!(*io.calls.borrow(), ["pane"]);
    io.fail = Some("pane");
    assert!(text(&c, &io, 99).is_err());
    io.f.borrow_mut().account.available = false;
    assert!(text(&c, &io, 2).is_err());
}
#[test]
fn column_guards_avoid_provider_reads_and_unknown_presence_is_offline() {
    let c = Controller::default();
    let io = Native::new(&c);
    io.f.borrow_mut().frame.entries = vec![row(b"row", -1, 0)];
    assert!(column(&io, 0, 1).unwrap().is_empty());
    assert!(io.calls.borrow().is_empty());
    {
        let mut f = io.f.borrow_mut();
        f.frame.entries[0].destination = 31;
        f.social.friends = vec![SocialFriend {
            presence: -1,
            ..SocialFriend::default()
        }];
    }
    assert_eq!(column(&io, 0, 1).unwrap(), b"Offline");
    assert_eq!(*io.calls.borrow(), ["steam"]);
}
#[test]
fn social_party_invite_navigation_uses_current_stack_after_account_refresh() {
    let c = Controller::from_state(State {
        friend_target: b"id".to_vec(),
        ..State::default()
    });
    let mut io = Native::new(&c);
    {
        let mut f = io.f.borrow_mut();
        f.account.party = true;
        f.account.leader = true;
        f.social.friends = vec![SocialFriend {
            handle: b"id".to_vec(),
            ..SocialFriend::default()
        }];
        f.frame = Frame {
            entries: vec![row(b"Invite", -35, 3)],
            ..frame(31, 43)
        };
        f.frames = vec![frame(6, 0), frame(186, 0), f.frame.clone()];
    }
    io.after_account = Some(vec![frame(6, 0), frame(186, 0), frame(31, 43), frame(8, 0)]);
    accept(&c, &io).unwrap();
    assert_eq!(io.commands.borrow()[0].0, 1);
    assert_eq!(io.f.borrow().frames.last().unwrap().screen, 26);
    assert_eq!(io.f.borrow().frames.last().unwrap().context, 2);
}
#[test]
fn copied_controller_keeps_friend_and_activity_identity_independent() {
    let c = Controller::from_state(State {
        friend_target: vec![0xff, 0],
        activity_target: b"a".to_vec(),
        friend_join_serial: u64::MAX,
        ..State::default()
    });
    let copy = Controller::from_state(c.state());
    let io = Native::new(&copy);
    bind(&copy, &io).unwrap();
    assert!(copy.state().friend_target.is_empty());
    assert_eq!(c.state().friend_target, [0xff, 0]);
    assert_eq!(c.state().activity_target, b"a");
    assert_eq!(c.state().friend_join_serial, u64::MAX);
}
