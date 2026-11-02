use halo2_online_protocol::{
    matchmaking_frontend_account_flow::Entry,
    matchmaking_frontend_clans::*,
    matchmaking_frontend_party_flow::{Command as PartyCommand, Controller, State},
};
use std::cell::RefCell;
struct TestIo<'a> {
    flow: &'a Controller,
    facts: RefCell<Facts>,
    events: RefCell<Vec<String>>,
    states: RefCell<Vec<State>>,
    fail: RefCell<Option<String>>,
    reply: bool,
}
impl<'a> TestIo<'a> {
    fn new(flow: &'a Controller) -> Self {
        let frame = Frame {
            screen: 26,
            context: 1,
            ..Frame::default()
        };
        Self {
            flow,
            facts: RefCell::new(Facts {
                account_client: true,
                voice_client: true,
                social_client: true,
                local_accounts: vec![true, false, true, false],
                frame: Some(frame.clone()),
                frames: vec![frame],
                ..Facts::default()
            }),
            events: RefCell::new(vec![]),
            states: RefCell::new(vec![]),
            fail: RefCell::new(None),
            reply: true,
        }
    }
    fn event(&self, s: String) -> Result<()> {
        self.states.borrow_mut().push(self.flow.state());
        self.events.borrow_mut().push(s.clone());
        if self.fail.borrow().as_ref() == Some(&s) {
            Err(s)
        } else {
            Ok(())
        }
    }
    fn events(&self) -> Vec<String> {
        self.events.borrow().clone()
    }
    fn action(&self, destination: i32, value: i32) {
        let mut f = self.facts.borrow_mut();
        let frame = Frame {
            screen: 217,
            entries: vec![Entry {
                destination,
                value,
                label: vec![],
            }],
            ..Frame::default()
        };
        f.frames = vec![frame.clone()];
        f.frame = Some(frame);
    }
    fn clan(&self) {
        self.facts.borrow_mut().account.clan = Some(Clan {
            name: b"clan".to_vec(),
            role: 3,
            members: vec![Member {
                id: b"A".to_vec(),
                name: b"name".to_vec(),
                role: 1,
                is_self: false,
                presence: 2,
            }],
            ..Clan::default()
        });
    }
}
impl Io for TestIo<'_> {
    fn facts(&self) -> Result<Facts> {
        Ok(self.facts.borrow().clone())
    }
    fn effect(&self, e: Effect) -> Result<()> {
        self.event(format!("{e:?}"))?;
        let mut f = self.facts.borrow_mut();
        match e {
            Effect::InstallAccount => f.account_client = f.candidate_account,
            Effect::ResetAccount => f.account = Account::default(),
            Effect::CreateVoice(_) | Effect::InstallVoice => f.voice_client = true,
            Effect::DropVoice => f.voice_client = false,
            Effect::ResetVoice(r) => {
                f.voice = Voice {
                    revision: r,
                    ..Voice::default()
                }
            }
            Effect::ClearLocalAccounts => f.local_accounts.fill(false),
            Effect::Open(s, c) => {
                let frame = Frame {
                    screen: s,
                    context: c,
                    ..Frame::default()
                };
                f.frame = Some(frame.clone());
                f.frames.push(frame);
            }
            Effect::Pop | Effect::Back => {
                if f.frames.len() > 1 {
                    f.frames.pop();
                }
                f.frame = f.frames.last().cloned();
            }
            Effect::Frame(target, field) => {
                let frame = match target {
                    Target::View => f.frame.as_mut().unwrap(),
                    Target::Top => f.frames.last_mut().unwrap(),
                };
                match field {
                    Field::Description(t) => frame.description = t,
                    Field::Append(e) => frame.entries.push(e),
                    Field::Replace(e) => frame.entries = e,
                    Field::Select(i) => frame.selected = i,
                    Field::Title(_) => (),
                }
            }
            _ => (),
        }
        Ok(())
    }
    fn command(&self, c: &Command) -> Result<bool> {
        self.event(format!("command:{c:?}"))?;
        Ok(self.reply)
    }
    fn party_command(&self, c: &PartyCommand) -> Result<()> {
        self.event(format!("party:{:?}", c.action))
    }
    fn update_account(&self) -> Result<()> {
        self.event("update".into())
    }
    fn select(&self, i: u32) -> Result<()> {
        self.event(format!("select:{i}"))
    }
    fn prepare_pane(&self, s: u32, p: u32) -> Result<()> {
        self.event(format!("pane:{s}:{p}"))
    }
    fn localized(&self, i: u32) -> Result<Vec<u8>> {
        self.event(format!("localized:{i}"))?;
        Ok(vec![255, 0, i as u8])
    }
    fn localized_direct(&self, s: u32, p: u32, i: u32) -> Result<Vec<u8>> {
        self.event(format!("direct:{s}:{p}:{i}"))?;
        Ok(vec![128, 0])
    }
}
#[test]
fn same_account_recreates_only_missing_voice() {
    let c = Controller::from_state(State {
        voice_party: true,
        voice_wanted: true,
        ..State::default()
    });
    let io = TestIo::new(&c);
    {
        let mut f = io.facts.borrow_mut();
        f.same_account = true;
        f.candidate_account = true;
        f.voice_client = false;
    }
    bind_account(&c, &io).unwrap();
    assert_eq!(
        io.events(),
        ["CreateVoice(true)", "ResetVoice(18446744073709551615)"]
    );
    assert!(c.state().voice_party && c.state().voice_wanted);
    assert_eq!(io.facts.borrow().voice.revision, u64::MAX);
}
#[test]
fn same_account_existing_or_null_is_noop() {
    for present in [true, false] {
        let c = Controller::default();
        let io = TestIo::new(&c);
        {
            let mut f = io.facts.borrow_mut();
            f.same_account = true;
            f.candidate_account = present;
            f.voice_client = present;
        }
        bind_account(&c, &io).unwrap();
        assert!(io.events().is_empty());
    }
}
#[test]
fn account_rebind_resets_only_owned_fields_in_order() {
    let c = Controller::from_state(State {
        account_name: b"name".to_vec(),
        friend_join_serial: 7,
        dismissed: vec![b"old".to_vec()],
        clan_target: b"keep".to_vec(),
        voice_party: true,
        voice_wanted: true,
        ..State::default()
    });
    let io = TestIo::new(&c);
    io.facts.borrow_mut().candidate_account = true;
    bind_account(&c, &io).unwrap();
    assert_eq!(
        io.events(),
        [
            "SignOutLocal(0)",
            "SignOutLocal(2)",
            "ClearLocalAccounts",
            "VoiceLeave",
            "AccountSignOut",
            "InstallAccount",
            "ResetAccount",
            "ClearSounds",
            "BindSocial",
            "CreateVoice(false)",
            "ResetVoice(0)"
        ]
    );
    let s = c.state();
    assert_eq!(s.clan_target, b"keep");
    assert!(s.voice_wanted && !s.voice_party);
    assert!(s.account_name.is_empty() && s.dismissed.is_empty());
    assert_eq!(s.friend_join_serial, 0);
}
#[test]
fn constructor_failure_keeps_late_voice_flag_and_cache() {
    let c = Controller::from_state(State {
        voice_party: true,
        ..State::default()
    });
    let io = TestIo::new(&c);
    io.facts.borrow_mut().candidate_account = true;
    io.facts.borrow_mut().voice.revision = 77;
    *io.fail.borrow_mut() = Some("CreateVoice(false)".into());
    assert!(bind_account(&c, &io).is_err());
    assert!(c.state().voice_party);
    assert_eq!(io.facts.borrow().voice.revision, 77);
    assert!(!io.events().iter().any(|e| e.starts_with("ResetVoice")));
}
#[test]
fn signout_failure_preserves_binding_and_local_array() {
    let c = Controller::default();
    let io = TestIo::new(&c);
    *io.fail.borrow_mut() = Some("SignOutLocal(2)".into());
    assert!(bind_account(&c, &io).is_err());
    assert_eq!(io.events(), ["SignOutLocal(0)", "SignOutLocal(2)"]);
    assert!(io.facts.borrow().account_client && io.facts.borrow().local_accounts[0]);
}
#[test]
fn absent_candidate_drops_voice_without_constructor() {
    let c = Controller::default();
    let io = TestIo::new(&c);
    bind_account(&c, &io).unwrap();
    assert!(io.events().contains(&"DropVoice".into()));
    assert!(!io.events().iter().any(|e| e.starts_with("CreateVoice")));
}
#[test]
fn voice_rebind_leaves_then_installs_and_resets() {
    let c = Controller::from_state(State {
        voice_party: true,
        voice_wanted: true,
        ..State::default()
    });
    let io = TestIo::new(&c);
    bind_voice(&c, &io).unwrap();
    assert_eq!(io.events(), ["VoiceLeave", "InstallVoice", "ResetVoice(0)"]);
    assert!(!c.state().voice_party && c.state().voice_wanted);
}
#[test]
fn ptt_false_does_not_require_frame() {
    let c = Controller::default();
    let io = TestIo::new(&c);
    io.facts.borrow_mut().frame = None;
    push_to_talk(&io, false).unwrap();
    assert_eq!(io.events(), ["PushToTalk(false)"]);
    assert!(push_to_talk(&io, true).is_err());
}
#[test]
fn ptt_suppressed_on_keyboard_and_absent_voice() {
    let c = Controller::default();
    let io = TestIo::new(&c);
    io.facts.borrow_mut().frame.as_mut().unwrap().screen = 167;
    push_to_talk(&io, true).unwrap();
    io.facts.borrow_mut().voice_client = false;
    push_to_talk(&io, true).unwrap();
    assert_eq!(io.events(), ["PushToTalk(false)"]);
}
#[test]
fn all_screen_routes_are_explicit() {
    for (s, ctx, want) in [
        (26, 1, true),
        (26, 0, false),
        (217, 0, true),
        (236, 99, true),
        (31, 41, true),
        (211, 41, true),
        (33, 41, true),
        (33, 0, false),
    ] {
        assert_eq!(screen(s, ctx), want);
    }
}
#[test]
fn populate_counts_preexisting_entries_and_preserves_bytes() {
    let c = Controller::default();
    let io = TestIo::new(&c);
    io.clan();
    {
        let mut f = io.facts.borrow_mut();
        let clan = f.account.clan.as_mut().unwrap();
        clan.members[0].name = vec![255, 0];
        clan.members[0].is_self = true;
        f.frame.as_mut().unwrap().entries.push(Entry::default());
    }
    populate(&c, &io).unwrap();
    let f = io.facts.borrow();
    let row = &f.frame.as_ref().unwrap().entries[1];
    assert_eq!(row.value, 1);
    assert_eq!(row.label, [255, 0, b' ', b'(', b'y', b'o', b'u', b')']);
}
#[test]
fn invite_target_precedes_member_and_commits_before_open() {
    let c = Controller::default();
    let io = TestIo::new(&c);
    io.clan();
    {
        let mut f = io.facts.borrow_mut();
        f.account.clan.as_mut().unwrap().outgoing.push(Invite {
            id: b"out".to_vec(),
            ..Invite::default()
        });
        let frame = f.frame.as_mut().unwrap();
        frame.entries = vec![Entry {
            destination: 31,
            value: 2000,
            label: vec![],
        }];
    }
    *io.fail.borrow_mut() = Some("Open(31, 41)".into());
    assert!(accept(&c, &io).is_err());
    assert_eq!(c.state().clan_target, b"out");
    assert_eq!(c.state().clan_target_kind, 2);
}
#[test]
fn clan_options_gate_voice_and_leader() {
    let c = Controller::default();
    let io = TestIo::new(&c);
    io.clan();
    {
        let mut f = io.facts.borrow_mut();
        f.frame.as_mut().unwrap().screen = 217;
        f.account.party = Some(true);
        f.voice.joined = true;
    }
    populate(&c, &io).unwrap();
    let f = io.facts.borrow();
    let entries = &f.frame.as_ref().unwrap().entries;
    assert!(entries.iter().any(|e| e.destination == -33));
    assert!(entries.iter().any(|e| e.label == b"Leave Clan Voice"));
    assert!(entries.iter().any(|e| e.label == b"Mute Microphone"));
}
#[test]
fn mute_label_precedes_talking() {
    let c = Controller::default();
    let io = TestIo::new(&c);
    io.clan();
    {
        let mut f = io.facts.borrow_mut();
        f.voice.joined = true;
        f.voice.muted = vec![b"A".to_vec()];
        f.voice.talking = vec![b"A".to_vec()];
    }
    assert_eq!(column(&c, &io, 0, 1).unwrap(), b"Member - ONLINE - muted");
    assert_eq!(column(&c, &io, 99, 1).unwrap(), b"Invited");
    assert!(column(&c, &io, 0, 0).unwrap().is_empty());
}
#[test]
fn text_prepares_pane_even_for_constant_or_unknown_index() {
    let c = Controller::default();
    let io = TestIo::new(&c);
    assert_eq!(text(&c, &io, 0).unwrap(), b"CLAN");
    assert!(text(&c, &io, 99).unwrap().is_empty());
    assert_eq!(text(&c, &io, 3).unwrap(), [255, 0, 3]);
    assert_eq!(
        io.events(),
        ["pane:26:1", "pane:26:1", "pane:26:1", "localized:3"]
    );
}
#[test]
fn role_description_and_direct_localization_clamp_selection() {
    let c = Controller::default();
    c.set_clan_target(b"A".to_vec(), 0);
    let io = TestIo::new(&c);
    io.clan();
    {
        let mut f = io.facts.borrow_mut();
        let frame = f.frame.as_mut().unwrap();
        frame.screen = 236;
        frame.selected = u32::MAX;
    }
    assert_eq!(text(&c, &io, 0).unwrap(), b"name\nCurrent role: Member");
    assert_eq!(text(&c, &io, 1).unwrap(), b"Overlord");
    assert_eq!(text(&c, &io, 2).unwrap(), [128, 0]);
    assert_eq!(io.events(), ["direct:236:3:2"]);
}
#[test]
fn message_pages_keep_utf8_boundary_and_wrap_page() {
    let c = Controller::from_state(State {
        clan_message_page: 1,
        ..State::default()
    });
    let io = TestIo::new(&c);
    io.clan();
    let mut body = vec![b'a'; 63];
    body.extend_from_slice("é".as_bytes());
    {
        let mut f = io.facts.borrow_mut();
        f.frame.as_mut().unwrap().screen = 211;
        f.account.clan.as_mut().unwrap().messages.push(Message {
            body,
            ..Message::default()
        });
    }
    assert_eq!(text(&c, &io, 1).unwrap(), "é".as_bytes());
    assert_eq!(text(&c, &io, 2).unwrap(), b"Message 2/2 - X next");
    c.advance_clan_message();
    assert_eq!(text(&c, &io, 1).unwrap(), vec![b'a'; 63]);
}
#[test]
fn message_advance_wraps_and_commits_before_revision_failure() {
    let c = Controller::from_state(State {
        clan_message_page: u32::MAX,
        ..State::default()
    });
    let io = TestIo::new(&c);
    {
        let mut f = io.facts.borrow_mut();
        f.frame.as_mut().unwrap().screen = 211;
        f.frame.as_mut().unwrap().context = 41;
    }
    *io.fail.borrow_mut() = Some("Revision".into());
    assert!(advance_message(&c, &io).is_err());
    assert_eq!(c.state().clan_message_page, 0);
}
#[test]
fn empty_and_nonprogressing_bodies_are_safe_explicit_errors() {
    let c = Controller::default();
    let io = TestIo::new(&c);
    io.clan();
    {
        let mut f = io.facts.borrow_mut();
        f.frame.as_mut().unwrap().screen = 211;
        f.account
            .clan
            .as_mut()
            .unwrap()
            .messages
            .push(Message::default());
    }
    assert!(text(&c, &io, 1).is_err());
    assert_eq!(text(&c, &io, 2).unwrap(), b"Clan message");
    io.facts
        .borrow_mut()
        .account
        .clan
        .as_mut()
        .unwrap()
        .messages[0]
        .body = vec![128; 65];
    assert!(text(&c, &io, 0).is_err());
}
#[test]
fn confirmation_is_stored_before_open_and_defaults_to_cancel() {
    let c = Controller::default();
    let io = TestIo::new(&c);
    let cmd = Command {
        action: b"remove".to_vec(),
        ..Command::default()
    };
    confirm(&c, cmd.clone(), b"message".to_vec(), &io).unwrap();
    assert_eq!(c.state().clan_confirmation, Some(cmd));
    assert_eq!(io.facts.borrow().frames.last().unwrap().selected, 1);
    assert!(io.states.borrow()[0].clan_confirmation.is_some());
}
#[test]
fn confirm_accept_takes_command_before_provider_error() {
    let c = Controller::default();
    let io = TestIo::new(&c);
    let cmd = Command {
        action: b"remove".to_vec(),
        ..Command::default()
    };
    c.set_clan_confirmation(Some(cmd.clone()));
    io.facts.borrow_mut().frame = Some(Frame {
        screen: 33,
        context: 41,
        ..Frame::default()
    });
    *io.fail.borrow_mut() = Some(format!("command:{cmd:?}"));
    assert!(accept(&c, &io).is_err());
    assert!(c.state().clan_confirmation.is_none());
    assert!(
        io.states
            .borrow()
            .last()
            .unwrap()
            .clan_confirmation
            .is_none()
    );
}
#[test]
fn leave_releases_voice_before_unavailable_account_warning() {
    let c = Controller::default();
    let io = TestIo::new(&c);
    io.facts.borrow_mut().account_client = false;
    command(
        &c,
        &Command {
            action: b"leave".to_vec(),
            ..Command::default()
        },
        &io,
    )
    .unwrap();
    assert_eq!(io.events()[0], "VoiceLeave");
    assert!(io.events()[1].starts_with("Unavailable"));
}
#[test]
fn role_three_confirms_other_roles_command_directly() {
    for rank in [0, 1, 2, 3, 4] {
        let c = Controller::default();
        let io = TestIo::new(&c);
        io.action(-30, 10 + rank);
        accept(&c, &io).unwrap();
        assert_eq!(c.state().clan_confirmation.is_some(), rank == 3);
        assert_eq!(
            io.events().iter().any(|e| e.starts_with("command:")),
            rank != 3
        );
    }
}
#[test]
fn party_voice_switch_orders_intent_before_constructor() {
    let c = Controller::from_state(State {
        voice_party: true,
        voice_wanted: true,
        ..State::default()
    });
    let io = TestIo::new(&c);
    io.action(-32, 0);
    *io.fail.borrow_mut() = Some("CreateVoice(false)".into());
    assert!(accept(&c, &io).is_err());
    assert_eq!(
        io.events(),
        ["VoiceLeave", "DropVoice", "CreateVoice(false)"]
    );
    assert!(!c.state().voice_party && !c.state().voice_wanted);
    assert!(!io.facts.borrow().voice_client);
}
#[test]
fn unknown_voice_action_still_returns_and_updates() {
    let c = Controller::default();
    let io = TestIo::new(&c);
    io.action(-32, -99);
    assert!(accept(&c, &io).unwrap());
    assert_eq!(io.events(), ["Back", "update"]);
}
#[test]
fn invalid_action_and_role_lookup_are_safe_without_changing_comparison_branches() {
    let c = Controller::default();
    let io = TestIo::new(&c);
    for value in [-1, 6, 9] {
        io.action(-30, value);
        assert!(accept(&c, &io).is_err());
    }
    io.clan();
    {
        let mut f = io.facts.borrow_mut();
        f.frame.as_mut().unwrap().screen = 26;
        f.frame.as_mut().unwrap().context = 1;
        f.account.clan.as_mut().unwrap().members[0].role = 99;
    }
    assert!(column(&c, &io, 0, 1).is_err());
    assert!(column(&c, &io, 0, 0).unwrap().is_empty());
}
#[test]
fn cloned_state_has_independent_confirmation_and_target() {
    let c = Controller::default();
    c.set_clan_target(b"first".to_vec(), 2);
    c.set_clan_confirmation(Some(Command::default()));
    let copy = Controller::from_state(c.state());
    copy.set_clan_target(b"second".to_vec(), 1);
    copy.take_clan_confirmation();
    assert_eq!(c.state().clan_target, b"first");
    assert!(c.state().clan_confirmation.is_some());
    assert!(copy.state().clan_confirmation.is_none());
}
