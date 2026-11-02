use crate::ffi;
use crate::matchmaking_frontend_party_flow::FrontendPartyFlow;
use halo2_online_protocol::matchmaking_frontend_account_flow as account;
use halo2_online_protocol::matchmaking_frontend_friends as core;
use halo2_online_protocol::matchmaking_frontend_party_flow::Command;
fn friend(f: ffi::FriendsFlowFriend) -> core::Friend {
    core::Friend {
        id: f.id,
        name: f.name,
        tag: f.tag,
        state: f.state,
        presence: f.presence,
        joinable: f.joinable.present.then_some(f.joinable.value),
        context: f.context,
    }
}
fn social(s: ffi::FriendsFlowSocial) -> core::Social {
    core::Social {
        available: s.available,
        enabled: s.enabled,
        connecting: s.connecting,
        ready: s.ready,
        busy: s.busy,
        message: s.message,
        revision: s.revision,
        friends: s
            .friends
            .into_iter()
            .map(|p| core::SocialFriend {
                handle: p.handle,
                name: p.name,
                kind: p.kind,
                presence: p.presence,
            })
            .collect(),
        invitations: s
            .invitations
            .into_iter()
            .map(|p| core::Invite {
                handle: p.handle,
                name: p.name,
                status: p.status,
            })
            .collect(),
    }
}
fn entry(e: ffi::AccountFlowEntry) -> account::Entry {
    account::Entry {
        label: e.label,
        destination: e.destination,
        value: e.value,
    }
}
fn frame(f: ffi::AccountFlowFrame) -> account::Frame {
    account::Frame {
        screen: f.screen,
        context: f.context,
        selected: f.selected,
        pcr: f.pcr,
        entries: f.entries.into_iter().map(entry).collect(),
    }
}
fn encoded_entry(e: account::Entry) -> ffi::AccountFlowEntry {
    ffi::AccountFlowEntry {
        label: e.label,
        destination: e.destination,
        value: e.value,
    }
}
fn command(c: &Command) -> ffi::PartyFlowCommand {
    ffi::PartyFlowCommand {
        action: c.action.clone(),
        value: c.value.clone(),
        playlist: c.playlist.clone(),
        revision: c.revision.clone(),
        version: c.version,
        privacy_present: c.privacy.is_some(),
        privacy: c.privacy.unwrap_or_default(),
        privacy_revision: c.privacy_revision,
        privacy_context: c.privacy_context.clone(),
        join_context: c.join_context.clone(),
    }
}
struct Io<'a>(&'a ffi::NativeFrontendFriendsIo);
impl core::Io for Io<'_> {
    fn facts(&self) -> core::Result<core::Facts> {
        let f = self.0.friends_flow_facts().map_err(|e| e.to_string())?;
        let a = f.account;
        Ok(core::Facts {
            account_client: f.account_client,
            social_client: f.social_client,
            login_client: f.login_client,
            account: core::Account {
                authenticated: a.authenticated,
                busy: a.busy,
                party: a.party,
                leader: a.leader,
                available: a.available,
                tag: a.tag,
                message: a.message,
                friends: a.friends.into_iter().map(friend).collect(),
                search: a.search_present.then(|| friend(a.search)),
            },
            social: social(f.social),
            frame: frame(f.frame),
            frames: f.frames.into_iter().map(frame).collect(),
            launch: f.launch,
            status: f.status,
        })
    }
    fn uses_steam(&self) -> core::Result<bool> {
        self.0.friends_flow_steam().map_err(|e| e.to_string())
    }
    fn identity(&self) -> core::Result<bool> {
        self.0.friends_flow_identity().map_err(|e| e.to_string())
    }
    fn pane(&self) -> core::Result<()> {
        self.0.friends_flow_pane().map_err(|e| e.to_string())
    }
    fn localized(&self, index: u32) -> core::Result<Vec<u8>> {
        self.0
            .friends_flow_localized(index)
            .map_err(|e| e.to_string())
    }
    fn profile_name(&self) -> core::Result<Vec<u8>> {
        self.0
            .friends_flow_profile_name()
            .map_err(|e| e.to_string())
    }
    fn social_snapshot(&self) -> core::Result<core::Social> {
        self.0
            .friends_flow_social_snapshot()
            .map(social)
            .map_err(|e| e.to_string())
    }
    fn friend_command(&self, action: &[u8], target: &[u8]) -> core::Result<bool> {
        self.0
            .friends_flow_friend_command(action, target)
            .map_err(|e| e.to_string())
    }
    fn party_command(&self, c: &Command) -> core::Result<bool> {
        self.0
            .friends_flow_party_command(&command(c))
            .map_err(|e| e.to_string())
    }
    fn wrapped_party_command(&self, c: &Command) -> core::Result<()> {
        self.0
            .friends_flow_wrapped_party_command(&command(c))
            .map_err(|e| e.to_string())
    }
    fn command_serial(&self) -> core::Result<u64> {
        self.0.friends_flow_serial().map_err(|e| e.to_string())
    }
    fn update_account(&self) -> core::Result<()> {
        self.0
            .friends_flow_update_account()
            .map_err(|e| e.to_string())
    }
    fn social_command(&self, kind: u32, target: &[u8], action: &[u8]) -> core::Result<bool> {
        self.0
            .friends_flow_social_command(kind, target, action)
            .map_err(|e| e.to_string())
    }
    fn effect(&self, e: core::Effect) -> core::Result<()> {
        use core::Effect::*;
        let mut out = ffi::FriendsFlowEffect {
            kind: 0,
            index: 0,
            a: 0,
            b: 0,
            text: vec![],
            entry: ffi::AccountFlowEntry {
                label: vec![],
                destination: 0,
                value: 0,
            },
            entries: vec![],
        };
        out.kind = match e {
            Title(t) => {
                out.text = t;
                0
            }
            Description(t) => {
                out.text = t;
                1
            }
            Append(e) => {
                out.entry = encoded_entry(e);
                2
            }
            Replace(rows) => {
                out.entries = rows.into_iter().map(encoded_entry).collect();
                3
            }
            ClearKeyboard => 4,
            Open(a, b) => {
                out.a = a;
                out.b = b;
                5
            }
            Back => 6,
            Pop => 7,
            Revision => 8,
            Unavailable(t) => {
                out.text = t;
                9
            }
            StageEmptySocial => 10,
            CommitSocial => 11,
            BindAccount => 12,
            Enable => 13,
            Disconnect => 14,
            Pump => 15,
            Presence(t) => {
                out.text = t;
                16
            }
            ClearIndex(i) => {
                out.index = i as u64;
                17
            }
            PopulateIndex(i) => {
                out.index = i as u64;
                18
            }
            SelectIndex(i, n) => {
                out.index = i as u64;
                out.a = n;
                19
            }
        };
        self.0.friends_flow_effect(&out).map_err(|e| e.to_string())
    }
}
pub fn frontend_friends_screen(screen: u32, context: u32) -> bool {
    core::screen(screen, context)
}
pub fn frontend_friends_flow(
    controller: &FrontendPartyFlow,
    action: u32,
    a: u32,
    b: u32,
    native: &ffi::NativeFrontendFriendsIo,
) -> core::Result<ffi::FriendsFlowResult> {
    let c = controller.controller();
    let io = Io(native);
    let mut out = ffi::FriendsFlowResult {
        handled: true,
        text: vec![],
    };
    match action {
        0 => core::bind(c, &io)?,
        1 => out.handled = core::native(&io)?,
        2 => out.handled = core::available(&io)?,
        3 => core::populate_native(c, &io)?,
        4 => core::populate(c, &io)?,
        5 => out.handled = core::accept_native(c, &io)?,
        6 => out.handled = core::accept(c, &io)?,
        7 => core::update(c, &io)?,
        8 => out.text = core::text(c, &io, a)?,
        9 => out.text = core::column(&io, a, b)?,
        _ => return Err("Invalid native friends flow action".into()),
    };
    Ok(out)
}
