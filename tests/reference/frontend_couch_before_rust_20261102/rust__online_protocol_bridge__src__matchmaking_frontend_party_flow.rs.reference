use crate::ffi;
use halo2_online_protocol::matchmaking_frontend_party_flow as core;
pub struct FrontendPartyFlow(core::Controller);
impl FrontendPartyFlow {
    pub(crate) fn controller(&self) -> &core::Controller {
        &self.0
    }
}
pub fn new_frontend_party_flow() -> Box<FrontendPartyFlow> {
    Box::new(FrontendPartyFlow(core::Controller::default()))
}
pub fn clone_frontend_party_flow(c: &FrontendPartyFlow) -> Box<FrontendPartyFlow> {
    Box::new(FrontendPartyFlow(core::Controller::from_state(c.0.state())))
}
pub fn frontend_party_flow_state(c: &FrontendPartyFlow) -> ffi::PartyFlowState {
    let s = c.0.state();
    let p = s.pending;
    let confirmation = s.clan_confirmation;
    ffi::PartyFlowState {
        target: s.target,
        clan_target: s.clan_target,
        clan_target_kind: s.clan_target_kind,
        clan_message_page: s.clan_message_page,
        clan_confirmation_present: confirmation.is_some(),
        clan_confirmation: confirmation
            .as_ref()
            .map(super::matchmaking_frontend_clans::encoded_command)
            .unwrap_or_else(|| ffi::ClansFlowCommand {
                action: vec![],
                value: vec![],
                role: 0,
                confirmed: false,
            }),
        friend_target: s.friend_target,
        activity_target: s.activity_target,
        friend_join_serial: s.friend_join_serial,
        account_name: s.account_name,
        voice_party: s.voice_party,
        voice_wanted: s.voice_wanted,
        kind: s.kind,
        version: s.version,
        tab: s.tab,
        dismissed: s
            .dismissed
            .into_iter()
            .map(|value| ffi::PartyBytesData { value })
            .collect(),
        pending: p.is_some(),
        frame_instance: p.as_ref().map_or(0, |p| p.frame_instance),
        command_serial: p.as_ref().map_or(0, |p| p.command_serial),
        revision: p.as_ref().map_or(0, |p| p.revision),
        mode: p.as_ref().map_or(0, |p| p.mode),
        context: p.map_or_else(Vec::new, |p| p.context),
    }
}
pub fn frontend_party_flow_clear_dismissed(c: &FrontendPartyFlow) {
    c.0.clear_dismissed()
}
pub fn frontend_party_flow_prune(c: &FrontendPartyFlow, current: &[ffi::PartyBytesData]) {
    c.0.prune_dismissed(&current.iter().map(|b| b.value.clone()).collect::<Vec<_>>())
}
pub fn frontend_party_flow_screen(mode: i32, screen: u32, context: u32) -> bool {
    core::screen(mode, screen, context)
}
pub fn frontend_party_flow_picker(screen: u32, context: u32) -> bool {
    core::picker(screen, context)
}
fn command(c: &ffi::PartyFlowCommand) -> core::Command {
    core::Command {
        action: c.action.clone(),
        value: c.value.clone(),
        playlist: c.playlist.clone(),
        revision: c.revision.clone(),
        version: c.version,
        privacy: c.privacy_present.then_some(c.privacy),
        privacy_revision: c.privacy_revision,
        privacy_context: c.privacy_context.clone(),
        join_context: c.join_context.clone(),
    }
}
fn encoded(c: &core::Command) -> ffi::PartyFlowCommand {
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
struct Io<'a>(&'a ffi::NativeFrontendPartyFlowIo);
impl core::Io for Io<'_> {
    fn facts(&self) -> core::Result<core::Facts> {
        let f = self.0.party_flow_facts().map_err(|e| e.to_string())?;
        Ok(core::Facts {
            mode: f.mode,
            frames: f
                .frames
                .into_iter()
                .map(|f| core::Frame {
                    screen: f.screen,
                    context: f.context,
                    instance: f.instance,
                    selected: f.selected,
                    entries: f
                        .entries
                        .into_iter()
                        .map(|e| core::Entry {
                            destination: e.destination,
                            value: e.value,
                        })
                        .collect(),
                })
                .collect(),
            account_client: f.account_client,
            queue_client: f.queue_client,
            voice_client: f.voice_client,
            authenticated: f.authenticated,
            busy: f.busy,
            party: f.party.then(|| core::Party {
                leader: f.leader,
                version: f.version,
                privacy: f.privacy_present.then(|| core::Privacy {
                    mode: f.privacy.mode,
                    revision: f.privacy.revision,
                    context: f.privacy.context,
                }),
                outgoing: f.outgoing.into_iter().map(|p| p.value).collect(),
            }),
            invitations: f
                .invitations
                .into_iter()
                .map(|p| core::Invite {
                    id: p.id,
                    name: p.name,
                    version: p.version,
                })
                .collect(),
            friends: f
                .friends
                .into_iter()
                .map(|p| core::Friend {
                    id: p.id,
                    state: p.state,
                })
                .collect(),
            clan_members: f
                .clan
                .then(|| f.clan_members.into_iter().map(|p| p.value).collect()),
            signed_in: f.signed_in,
            launch: f.launch,
            status: f.status,
            party_message: f.party_message,
            command_serial: f.command_serial,
            successful: f.successful_present.then_some(f.successful),
            voice_party: f.voice_party,
            voice_joined: f.voice_joined,
            voice_connecting: f.voice_connecting,
            microphone_muted: f.microphone_muted,
            muted: f.muted.into_iter().map(|p| p.value).collect(),
        })
    }
    fn members(
        &self,
    ) -> core::Result<Vec<halo2_online_protocol::matchmaking_frontend_party::Member>> {
        self.0
            .party_flow_members()
            .map(|v| {
                v.iter()
                    .map(super::matchmaking_frontend_party::member)
                    .collect()
            })
            .map_err(|e| e.to_string())
    }
    fn command(&self, c: &core::Command) -> core::Result<bool> {
        self.0
            .party_flow_command(&encoded(c))
            .map_err(|e| e.to_string())
    }
    fn command_serial(&self) -> core::Result<u64> {
        self.0.party_flow_serial().map_err(|e| e.to_string())
    }
    fn update_account(&self) -> core::Result<()> {
        self.0.party_flow_update().map_err(|e| e.to_string())
    }
    fn cancel_queue(&self) -> core::Result<()> {
        self.0.party_flow_cancel().map_err(|e| e.to_string())
    }
    fn lobby_input(&self, d: i32) -> core::Result<bool> {
        self.0.party_flow_lobby(d).map_err(|e| e.to_string())
    }
    fn ui(&self, e: core::Ui) -> core::Result<()> {
        let mut f = ffi::PartyFlowUi {
            kind: 0,
            a: 0,
            b: 0,
            flag: false,
            text: Vec::new(),
        };
        match e {
            core::Ui::Revision => f.kind = 0,
            core::Ui::Pop => f.kind = 1,
            core::Ui::Open(a, b) => {
                f.kind = 2;
                f.a = a;
                f.b = b
            }
            core::Ui::Back => f.kind = 3,
            core::Ui::RebuildPicker => f.kind = 4,
            core::Ui::ClearKeyboard => f.kind = 5,
            core::Ui::LocalPlayer(a) => {
                f.kind = 6;
                f.a = a
            }
            core::Ui::Unavailable(text) => {
                f.kind = 7;
                f.text = text
            }
            core::Ui::FormatError(text) => {
                f.kind = 8;
                f.text = text
            }
            core::Ui::VoiceLeave => f.kind = 9,
            core::Ui::VoiceRecreate => f.kind = 10,
            core::Ui::VoiceJoin => f.kind = 11,
            core::Ui::VoiceWanted(flag) => {
                f.kind = 12;
                f.flag = flag
            }
            core::Ui::VoiceMicrophone(flag) => {
                f.kind = 13;
                f.flag = flag
            }
            core::Ui::VoiceMute(text, flag) => {
                f.kind = 14;
                f.text = text;
                f.flag = flag
            }
            core::Ui::VoiceSnapshot => f.kind = 15,
        }
        self.0.party_flow_ui(&f).map_err(|e| e.to_string())
    }
}
pub fn frontend_party_flow_run(
    c: &FrontendPartyFlow,
    action: u32,
    value: i32,
    io: &ffi::NativeFrontendPartyFlowIo,
) -> core::Result<bool> {
    let io = Io(io);
    match action {
        0 => c.0.accept(&io),
        1 => c.0.notice(value != 0, &io),
        2 => c.0.switch_tab(value, &io),
        3 => c.0.opened_privacy(&io).map(|_| true),
        4 => c.0.select_privacy(value, &io),
        5 => c.0.observe_privacy(&io).map(|_| true),
        _ => Err("Invalid native party flow action".into()),
    }
}
pub fn frontend_party_flow_command(
    c: &FrontendPartyFlow,
    data: &ffi::PartyFlowCommand,
    io: &ffi::NativeFrontendPartyFlowIo,
) -> core::Result<()> {
    c.0.command(command(data), &Io(io))
}

pub fn frontend_party_flow_join_serial(c: &FrontendPartyFlow, v: u64) {
    c.0.set_friend_join_serial(v)
}
pub fn frontend_party_flow_account_name(c: &FrontendPartyFlow, v: &[u8]) {
    c.0.set_account_name(v.to_vec())
}
pub fn frontend_party_flow_voice_party(c: &FrontendPartyFlow, v: bool) {
    c.0.set_voice_party(v)
}
pub fn frontend_party_flow_voice_wanted(c: &FrontendPartyFlow, v: bool) {
    c.0.set_voice_wanted(v)
}
pub fn frontend_party_flow_match_voice(c: &FrontendPartyFlow, v: bool, joined: bool) -> bool {
    c.0.match_voice(v, joined)
}
