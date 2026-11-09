use crate::ffi;
use halo2_online_protocol::matchmaking_frontend_postgame as core;
fn report(r: ffi::PostgameFlowReport) -> core::Report {
    core::Report {
        online: r.online,
        player_count: r.results_present.then_some(r.player_count),
        select: r.select,
    }
}
struct Io<'a>(&'a ffi::NativeFrontendPostgameIo);
impl core::Io for Io<'_> {
    fn facts(&self) -> core::Result<core::Facts> {
        let f = self.0.postgame_flow_facts().map_err(|e| e.to_string())?;
        Ok(core::Facts {
            authenticated: f.authenticated,
            party: f.party.then(|| core::Party {
                leader: f.leader,
                members: f
                    .members
                    .into_iter()
                    .map(|m| core::Member {
                        is_self: m.is_self,
                        leader: m.leader,
                        postgame: m.postgame_present.then_some(m.postgame),
                    })
                    .collect(),
            }),
            mode: f.mode,
            frames: f
                .frames
                .into_iter()
                .map(|v| core::Frame {
                    screen: v.screen,
                    report: v.report_present.then(|| report(v.report)),
                })
                .collect(),
            prior: f.prior_present.then(|| report(f.prior)),
            view: f.view_present.then(|| report(f.view)),
        })
    }
    fn effect(&self, e: core::Effect) -> core::Result<()> {
        use core::Effect::*;
        self.0
            .postgame_flow_effect(match e {
                SelectPrior => 0,
                OpenPrior => 1,
                Pop => 2,
                Revision => 3,
                CapturePrior => 4,
                ClearNavigation => 5,
                Back => 6,
                RefreshAccount => 7,
                ReturnOnline => 8,
            })
            .map_err(|e| e.to_string())
    }
    fn hold(&self, i: u32) -> core::Result<bool> {
        self.0.postgame_flow_hold(i).map_err(|e| e.to_string())
    }
    fn continue_held(&self) -> core::Result<()> {
        self.0.postgame_flow_continue().map_err(|e| e.to_string())
    }
}
impl core::LeaveIo for Io<'_> {
    fn request_party_leave(&self) -> core::Result<bool> {
        self.0
            .postgame_flow_request_leave()
            .map_err(|e| e.to_string())
    }
    fn leave_held_session(&self) -> core::Result<()> {
        self.0.postgame_flow_leave_held().map_err(|e| e.to_string())
    }
    fn unavailable(&self, message: &str) -> core::Result<()> {
        self.0
            .postgame_flow_unavailable(message)
            .map_err(|e| e.to_string())
    }
}
pub fn frontend_postgame_flow(
    action: u32,
    id: u32,
    io: &ffi::NativeFrontendPostgameIo,
) -> core::Result<bool> {
    let io = Io(io);
    match action {
        0 => return Ok(core::host(&core::Io::facts(&io)?)),
        1 => core::update(&io)?,
        2 => return Ok(core::has_prior(&core::Io::facts(&io)?)),
        3 => return core::review(&io),
        4 => core::close(&io)?,
        5 => core::capture(&io, id)?,
        6 => core::navigation(&io, id)?,
        7 => return core::confirm_leave(&io),
        _ => return Err("Invalid native postgame flow action".into()),
    }
    Ok(false)
}
