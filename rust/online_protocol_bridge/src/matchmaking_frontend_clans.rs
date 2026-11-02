use crate::{ffi, matchmaking_frontend_party_flow::FrontendPartyFlow};
use halo2_online_protocol::matchmaking_frontend_clans as core;
use halo2_online_protocol::matchmaking_frontend_party_flow::Command as PartyCommand;
fn entry(
    e: ffi::AccountFlowEntry,
) -> halo2_online_protocol::matchmaking_frontend_account_flow::Entry {
    halo2_online_protocol::matchmaking_frontend_account_flow::Entry {
        label: e.label,
        destination: e.destination,
        value: e.value,
    }
}
fn encoded_entry(
    e: halo2_online_protocol::matchmaking_frontend_account_flow::Entry,
) -> ffi::AccountFlowEntry {
    ffi::AccountFlowEntry {
        label: e.label,
        destination: e.destination,
        value: e.value,
    }
}
fn frame(f: ffi::ClansFlowFrame) -> core::Frame {
    core::Frame {
        screen: f.screen,
        context: f.context,
        selected: f.selected,
        description: f.description,
        entries: f.entries.into_iter().map(entry).collect(),
    }
}
fn command(c: &ffi::ClansFlowCommand) -> core::Command {
    core::Command {
        action: c.action.clone(),
        value: c.value.clone(),
        role: c.role,
        confirmed: c.confirmed,
    }
}
pub(crate) fn encoded_command(c: &core::Command) -> ffi::ClansFlowCommand {
    ffi::ClansFlowCommand {
        action: c.action.clone(),
        value: c.value.clone(),
        role: c.role,
        confirmed: c.confirmed,
    }
}
fn party(c: &PartyCommand) -> ffi::PartyFlowCommand {
    ffi::PartyFlowCommand {
        action: c.action.clone(),
        value: c.value.clone(),
        playlist: c.playlist.clone(),
        revision: c.revision.clone(),
        version: c.version,
        privacy_present: c.privacy.is_some(),
        privacy: c.privacy.unwrap_or(0),
        privacy_revision: c.privacy_revision,
        privacy_context: c.privacy_context.clone(),
        join_context: c.join_context.clone(),
    }
}
struct Io<'a>(&'a ffi::NativeFrontendClansIo);
impl core::Io for Io<'_> {
    fn facts(&self) -> core::Result<core::Facts> {
        let f = self.0.clans_flow_facts().map_err(|e| e.to_string())?;
        let a = f.account;
        let v = f.voice;
        Ok(core::Facts {
            account_client: f.account_client,
            voice_client: f.voice_client,
            social_client: f.social_client,
            same_account: f.same_account,
            candidate_account: f.candidate_account,
            local_accounts: f.local_accounts,
            account: core::Account {
                message: a.message,
                clan: a.clan_present.then(|| {
                    let c = a.clan;
                    core::Clan {
                        name: c.name,
                        role: c.role,
                        members: c
                            .members
                            .into_iter()
                            .map(|p| core::Member {
                                id: p.id,
                                name: p.name,
                                role: p.role,
                                is_self: p.is_self,
                                presence: p.presence,
                            })
                            .collect(),
                        outgoing: c
                            .outgoing
                            .into_iter()
                            .map(|p| core::Invite {
                                id: p.id,
                                name: p.name,
                            })
                            .collect(),
                        messages: c
                            .messages
                            .into_iter()
                            .map(|p| core::Message {
                                name: p.name,
                                body: p.body,
                            })
                            .collect(),
                    }
                }),
                invitations: a
                    .invitations
                    .into_iter()
                    .map(|p| core::Invite {
                        id: p.id,
                        name: p.name,
                    })
                    .collect(),
                party: a.party_present.then_some(a.leader),
            },
            voice: core::Voice {
                joined: v.joined,
                connecting: v.connecting,
                microphone_muted: v.microphone_muted,
                transmitting: v.transmitting,
                muted: v.muted.into_iter().map(|p| p.value).collect(),
                talking: v.talking.into_iter().map(|p| p.value).collect(),
                message: v.message,
                revision: v.revision,
            },
            frame: f.frame_present.then(|| frame(f.frame)),
            frames: f.frames.into_iter().map(frame).collect(),
        })
    }
    fn command(&self, c: &core::Command) -> core::Result<bool> {
        self.0
            .clans_flow_command(&encoded_command(c))
            .map_err(|e| e.to_string())
    }
    fn party_command(&self, c: &PartyCommand) -> core::Result<()> {
        self.0
            .clans_flow_party_command(&party(c))
            .map_err(|e| e.to_string())
    }
    fn update_account(&self) -> core::Result<()> {
        self.0
            .clans_flow_update_account()
            .map_err(|e| e.to_string())
    }
    fn select(&self, index: u32) -> core::Result<()> {
        self.0.clans_flow_select(index).map_err(|e| e.to_string())
    }
    fn prepare_pane(&self, screen: u32, pane: u32) -> core::Result<()> {
        self.0
            .clans_flow_prepare_pane(screen, pane)
            .map_err(|e| e.to_string())
    }
    fn localized(&self, index: u32) -> core::Result<Vec<u8>> {
        self.0
            .clans_flow_localized(index)
            .map_err(|e| e.to_string())
    }
    fn localized_direct(&self, screen: u32, pane: u32, index: u32) -> core::Result<Vec<u8>> {
        self.0
            .clans_flow_localized_direct(screen, pane, index)
            .map_err(|e| e.to_string())
    }
    fn effect(&self, e: core::Effect) -> core::Result<()> {
        use core::{Effect::*, Field};
        let mut out = ffi::ClansFlowEffect {
            kind: 0,
            top: false,
            index: 0,
            a: 0,
            b: 0,
            flag: false,
            text: vec![],
            entry: ffi::AccountFlowEntry {
                label: vec![],
                destination: 0,
                value: 0,
            },
            entries: vec![],
        };
        out.kind = match e {
            Frame(t, v) => {
                out.top = matches!(t, core::Target::Top);
                match v {
                    Field::Title(v) => {
                        out.text = v;
                        0
                    }
                    Field::Description(v) => {
                        out.text = v;
                        1
                    }
                    Field::Append(v) => {
                        out.entry = encoded_entry(v);
                        2
                    }
                    Field::Replace(v) => {
                        out.entries = v.into_iter().map(encoded_entry).collect();
                        3
                    }
                    Field::Select(v) => {
                        out.a = v;
                        4
                    }
                }
            }
            Open(a, b) => {
                out.a = a;
                out.b = b;
                5
            }
            Pop => 6,
            Back => 7,
            ClearKeyboard => 8,
            Revision => 9,
            Unavailable(t) => {
                out.text = t;
                10
            }
            SignOutLocal(i) => {
                out.index = i as u64;
                11
            }
            ClearLocalAccounts => 12,
            VoiceLeave => 13,
            AccountSignOut => 14,
            InstallAccount => 15,
            ResetAccount => 16,
            ClearSounds => 17,
            BindSocial => 18,
            CreateVoice(candidate) => {
                out.flag = candidate;
                19
            }
            DropVoice => 20,
            ResetVoice(revision) => {
                out.index = revision;
                21
            }
            InstallVoice => 22,
            PushToTalk(held) => {
                out.flag = held;
                23
            }
            VoiceJoin => 24,
            Microphone(muted) => {
                out.flag = muted;
                25
            }
            Mute(id, muted) => {
                out.text = id;
                out.flag = muted;
                26
            }
        };
        self.0.clans_flow_effect(&out).map_err(|e| e.to_string())
    }
}
pub fn frontend_clans_screen(screen: u32, context: u32) -> bool {
    core::screen(screen, context)
}
pub fn frontend_clans_flow(
    controller: &FrontendPartyFlow,
    action: u32,
    arg: &ffi::ClansFlowArgument,
    native: &ffi::NativeFrontendClansIo,
) -> core::Result<ffi::ClansFlowResult> {
    let c = controller.controller();
    let io = Io(native);
    let mut out = ffi::ClansFlowResult {
        handled: true,
        text: vec![],
    };
    match action {
        0 => core::bind_account(c, &io)?,
        1 => core::bind_voice(c, &io)?,
        2 => core::push_to_talk(&io, arg.flag)?,
        3 => core::populate(c, &io)?,
        4 => out.text = core::column(c, &io, arg.a, arg.b)?,
        5 => out.text = core::text(c, &io, arg.a)?,
        6 => core::command(c, &command(&arg.command), &io)?,
        7 => core::confirm(c, command(&arg.command), arg.text.clone(), &io)?,
        8 => out.handled = core::accept(c, &io)?,
        9 => out.handled = core::advance_message(c, &io)?,
        _ => return Err("Invalid native clans flow action".into()),
    }
    Ok(out)
}
