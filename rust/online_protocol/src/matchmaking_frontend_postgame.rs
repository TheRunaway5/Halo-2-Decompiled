//! Modern native online PCR caller. The native adapter owns the actual report,
//! result bank and callbacks. This module owns the postgame decisions.
use crate::matchmaking_account_client::{postgame_follower, postgame_leader_left};
pub type Result<T> = std::result::Result<T, String>;
pub const REVIEW_SCREEN: u32 = 0x10000 | 185;
#[derive(Clone, Debug, Default)]
pub struct Member {
    pub is_self: bool,
    pub leader: bool,
    pub postgame: Option<bool>,
}
#[derive(Clone, Debug, Default)]
pub struct Party {
    pub leader: bool,
    pub members: Vec<Member>,
}
#[derive(Clone, Debug, Default)]
pub struct Report {
    pub online: bool,
    pub player_count: Option<u32>,
    pub select: bool,
}
#[derive(Clone, Debug, Default)]
pub struct Frame {
    pub screen: u32,
    pub report: Option<Report>,
}
#[derive(Clone, Debug, Default)]
pub struct Facts {
    pub authenticated: bool,
    pub party: Option<Party>,
    pub mode: i32,
    pub frames: Vec<Frame>,
    pub prior: Option<Report>,
    pub view: Option<Report>,
}
impl Facts {
    fn top(&self) -> Result<&Frame> {
        self.frames
            .last()
            .ok_or_else(|| "Native postgame frame owner absent".into())
    }
}
#[derive(Clone, Copy, Debug)]
pub enum Effect {
    SelectPrior,
    OpenPrior,
    Pop,
    Revision,
    CapturePrior,
    ClearNavigation,
    Back,
    RefreshAccount,
    ReturnOnline,
}
pub trait Io {
    fn facts(&self) -> Result<Facts>;
    fn effect(&self, effect: Effect) -> Result<()>;
    /// Copy the actual retained shared owner, as the original native local does.
    /// The return value is that owner's current continuation callback presence.
    fn hold(&self, index: u32) -> Result<bool>;
    fn continue_held(&self) -> Result<()>;
}
pub trait LeaveIo: Io {
    fn request_party_leave(&self) -> Result<bool>;
    fn leave_held_session(&self) -> Result<()>;
    fn unavailable(&self, message: &str) -> Result<()>;
}
fn first_online_report(f: &Facts) -> Option<u32> {
    f.frames
        .iter()
        .position(|frame| frame.report.as_ref().is_some_and(|report| report.online))
        .map(|index| index as u32)
}
pub fn host(f: &Facts) -> bool {
    !f.authenticated
        || f.party
            .as_ref()
            .is_none_or(|p| !postgame_follower(p.leader, p.members.len()))
}
pub fn has_prior(f: &Facts) -> bool {
    f.prior
        .as_ref()
        .and_then(|p| p.player_count)
        .is_some_and(|n| n > 0)
}
pub fn update(io: &impl Io) -> Result<()> {
    let f = io.facts()?;
    if host(&f) {
        return Ok(());
    }
    let p = f
        .party
        .as_ref()
        .ok_or("Native postgame party owner absent")?;
    if !postgame_leader_left(
        p.leader,
        p.members.len(),
        p.members.iter().map(|m| (m.is_self, m.leader, m.postgame)),
    ) {
        return Ok(());
    }
    if let Some(index) = first_online_report(&f) {
        if io.hold(index)? {
            io.continue_held()?;
        }
    }
    Ok(())
}
/// Returns true when the caller can finish the accepted menu action.
pub fn confirm_leave(io: &impl LeaveIo) -> Result<bool> {
    let facts = io.facts()?;
    let report = first_online_report(&facts);
    // The party command may reenter the frontend and replace the report.
    if let Some(index) = report {
        io.hold(index)?;
    }
    if facts.party.is_some() && !io.request_party_leave()? {
        io.effect(Effect::Back)?;
        io.unavailable("Wait for the current ONLINE operation to finish.")?;
        return Ok(false);
    }
    if report.is_some() {
        io.leave_held_session()?;
    }
    io.effect(Effect::RefreshAccount)?;
    io.effect(Effect::ReturnOnline)?;
    Ok(true)
}
pub fn review(io: &impl Io) -> Result<bool> {
    let f = io.facts()?;
    if f.mode != 3 {
        return Ok(false);
    }
    if f.top()?.screen != 14 || !has_prior(&f) {
        return Ok(false);
    }
    if f.prior.as_ref().is_some_and(|p| p.select) {
        io.effect(Effect::SelectPrior)?;
    }
    // The selection callback can replace the prior owner. Open its actual
    // current native pointer, rather than retaining the pre-callback report.
    io.effect(Effect::OpenPrior)?;
    Ok(true)
}
pub fn close(io: &impl Io) -> Result<()> {
    let f = io.facts()?;
    let top = f.top()?;
    if top.screen != REVIEW_SCREEN
        || !top.report.as_ref().is_some_and(|p| p.online)
        || f.frames.len() < 2
    {
        return Ok(());
    }
    io.effect(Effect::Pop)?;
    io.effect(Effect::Revision)
}
pub fn capture(io: &impl Io, id: u32) -> Result<()> {
    // The complete native factory has already admitted a nonnull report.
    if id == 16
        && io
            .facts()?
            .view
            .as_ref()
            .ok_or("Native postgame capture owner absent")?
            .online
    {
        io.effect(Effect::CapturePrior)?;
    }
    Ok(())
}
pub fn navigation(io: &impl Io, id: u32) -> Result<()> {
    if id != 185
        || !io
            .facts()?
            .view
            .as_ref()
            .ok_or("Native postgame navigation owner absent")?
            .online
    {
        io.effect(Effect::ClearNavigation)?;
    }
    Ok(())
}
