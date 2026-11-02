use crate::ffi;
use halo2_online_protocol::matchmaking_frontend_presentation as core;
struct Io<'a>(&'a ffi::NativeFrontendTextIo);
impl core::Io for Io<'_> {
    fn original_text(&self, screen: u32, pane: u32, index: u32) -> core::Result<Vec<u8>> {
        self.0
            .text_original_text(screen, pane, index)
            .map_err(|e| e.to_string())
    }
    fn original_string(&self, screen: u32, id: u32) -> core::Result<Vec<u8>> {
        self.0
            .text_original_string(screen, id)
            .map_err(|e| e.to_string())
    }
    fn pc_string(&self, id: u32) -> core::Result<Vec<u8>> {
        self.0.text_pc_string(id).map_err(|e| e.to_string())
    }
    fn pc_authored(&self, pane: u32) -> core::Result<Vec<u8>> {
        self.0.text_pc_authored(pane).map_err(|e| e.to_string())
    }
    fn pc_match_line(&self) -> core::Result<Vec<u8>> {
        self.0.text_pc_match_line().map_err(|e| e.to_string())
    }
    fn map_name(&self) -> core::Result<Vec<u8>> {
        self.0.text_map_name().map_err(|e| e.to_string())
    }
    fn quick_option(&self, index: u32) -> core::Result<Vec<u8>> {
        self.0.text_quick_option(index).map_err(|e| e.to_string())
    }
    fn party_count(&self) -> core::Result<u64> {
        self.0.text_party_count().map_err(|e| e.to_string())
    }
    fn expand(&self, value: &[u8]) -> core::Result<Vec<u8>> {
        self.0.text_expand(value).map_err(|e| e.to_string())
    }
}
pub fn frontend_presentation_text(
    f: &ffi::PresentationFacts,
    original: bool,
    pane: u32,
    index: u32,
    io: &ffi::NativeFrontendTextIo,
) -> core::Result<Vec<u8>> {
    let facts = core::Facts {
        screen: f.screen,
        selected_row: f.selected_row,
        selected_id: f.selected_id.clone(),
        playlists: f
            .playlists
            .iter()
            .map(|p| core::Playlist {
                id: p.id.clone(),
                name: p.name.clone(),
                description: p.description.clone(),
                eligible: p.eligible,
                ranked: p.ranked,
                clan: p.clan,
                minimum: p.minimum,
                maximum: p.maximum,
                level: p.level_present.then_some(p.level),
            })
            .collect(),
        status: f.status,
        ui: f.ui,
        pane: f.pane,
        title: f.title,
        countdown: f.countdown,
        pc_progress: f.pc_progress,
        matched: f.matched_present.then_some(f.matched),
        gathering: f.gathering_present.then_some(f.gathering),
        minimum: f.minimum,
        queued: f.queued,
        variant: f.variant.clone(),
        map: f.map.clone(),
        message: f.message.clone(),
        party: f.party,
        leader: f.leader,
        party_message: f.party_message.clone(),
    };
    if original {
        core::original_text(&facts, pane, index, &Io(io))
    } else {
        core::text(&facts, index, &Io(io))
    }
}
