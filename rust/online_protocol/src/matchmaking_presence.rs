//! Observed native population, not recovered Xbox Live presence flags.
//! Only admitted Rust account/session/room owners supply seats and activity.
use crate::matchmaking_model::{Result, check};
use serde_json::{Value, json};
use std::collections::BTreeMap;

pub(crate) const ACTIVITY_SECONDS: f64 = 30.;
pub(crate) fn recent(expires: f64, seen: f64, now: f64) -> bool {
    expires > now && seen <= now && now - seen < ACTIVITY_SECONDS
}

#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
pub(crate) enum Activity {
    Idle,
    Postgame,
    Queued,
    Joining,
    Playing,
}
impl Activity {
    fn name(self) -> &'static str {
        match self {
            Self::Idle => "idle",
            Self::Postgame => "postgame",
            Self::Queued => "queued",
            Self::Joining => "joining",
            Self::Playing => "playing",
        }
    }
}
fn counters() -> Value {
    let mut value = json!({});
    for key in ["online", "idle", "postgame", "queued", "joining", "playing"] {
        value[key] = json!({"users":0,"guests":0});
    }
    value
}
fn increment(row: &mut Value, state: Activity, guest: bool) {
    let kind = if guest { "guests" } else { "users" };
    for key in ["online", state.name()] {
        row[key][kind] = json!(row[key][kind].as_u64().unwrap() + 1);
    }
}
#[derive(Default)]
pub(crate) struct Population {
    present: BTreeMap<String, bool>,
    observed: BTreeMap<String, bool>,
    activity: BTreeMap<String, (String, Activity)>,
}
impl Population {
    pub(crate) fn seat(&mut self, player: &str, guest: bool, playing: bool) {
        self.present
            .entry(player.to_owned())
            .and_modify(|previous| *previous &= guest)
            .or_insert(guest);
        if !guest {
            self.observed
                .entry(player.to_owned())
                .and_modify(|previous| *previous |= playing)
                .or_insert(playing);
        }
    }
    pub(crate) fn assign(&mut self, player: &str, playlist: &str, state: Activity) {
        if self.present.contains_key(player)
            && self
                .activity
                .get(player)
                .is_none_or(|(_, previous)| state > *previous)
        {
            self.activity
                .insert(player.to_owned(), (playlist.to_owned(), state));
        }
    }
    pub(crate) fn finish(
        &self,
        entries: &[(String, String)],
        wall: i64,
        view: &str,
    ) -> Result<Value> {
        check(
            ["population", "observed"].contains(&view),
            "Unknown presence view",
        )?;
        if view == "observed" {
            return Ok(json!(
                self.observed
                    .iter()
                    .map(|(player, playing)| (player, if *playing { "playing" } else { "online" }))
                    .collect::<BTreeMap<_, _>>()
            ));
        }
        let mut totals = counters();
        let mut playlists = entries
            .iter()
            .map(|(id, name)| {
                let mut row = counters();
                row["id"] = json!(id);
                row["name"] = json!(name);
                row
            })
            .collect::<Vec<_>>();
        let indices = entries
            .iter()
            .enumerate()
            .map(|(i, (id, _))| (id.as_str(), i))
            .collect::<BTreeMap<_, _>>();
        for (player, guest) in &self.present {
            let (playlist, state) = self
                .activity
                .get(player)
                .map(|(id, state)| (Some(id.as_str()), *state))
                .unwrap_or((None, Activity::Idle));
            increment(&mut totals, state, *guest);
            if let Some(index) = playlist.and_then(|id| indices.get(id)) {
                increment(&mut playlists[*index], state, *guest);
            }
        }
        totals["updated_at"] = json!(wall);
        totals["activity_window_seconds"] = json!(ACTIVITY_SECONDS as u32);
        totals["playlists"] = json!(playlists);
        Ok(totals)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn unique_accounts_guests_and_private_observation_are_distinct() {
        let mut p = Population::default();
        p.seat("owner", false, false);
        p.seat("owner", false, false);
        p.seat("couch", true, false);
        p.seat("guest", true, false);
        p.seat("couch", false, false);
        for who in ["owner", "couch", "guest", "unobserved"] {
            p.assign(who, "ranked", Activity::Queued);
        }
        let counts = p
            .finish(
                &[
                    ("ranked".into(), "Ranked".into()),
                    ("casual".into(), "Casual".into()),
                ],
                117,
                "population",
            )
            .unwrap();
        assert_eq!(counts["online"], json!({"users":2,"guests":1}));
        assert_eq!(counts["queued"], counts["online"]);
        assert_eq!(counts["playlists"][0]["online"], counts["online"]);
        assert_eq!(counts["playlists"][1]["online"]["users"], 0);
        for secret in ["owner", "couch", "unobserved", "guest\""] {
            assert!(!counts.to_string().contains(secret));
        }
        assert_eq!(
            p.finish(&[], 117, "observed").unwrap(),
            json!({"owner":"online","couch":"online"})
        );
    }

    #[test]
    fn activity_precedence_counts_handoff_once_and_never_downgrades_playing() {
        let mut p = Population::default();
        p.seat("owner", false, true);
        p.seat("owner", false, false);
        for state in [
            Activity::Postgame,
            Activity::Queued,
            Activity::Joining,
            Activity::Playing,
            Activity::Queued,
        ] {
            p.assign("owner", "ranked", state);
        }
        let counts = p.finish(&[], 117, "population").unwrap();
        assert_eq!(counts["playing"]["users"], 1);
        assert_eq!(counts["queued"]["users"], 0);
        assert_eq!(counts["online"]["users"], 1);
        assert_eq!(
            p.finish(&[], 117, "observed").unwrap(),
            json!({"owner":"playing"})
        );
        let mut p = Population::default();
        p.seat("owner", false, false);
        p.assign("owner", "ranked", Activity::Postgame);
        p.assign("owner", "casual", Activity::Queued);
        let counts = p
            .finish(
                &[
                    ("ranked".into(), "Ranked".into()),
                    ("casual".into(), "Casual".into()),
                ],
                117,
                "population",
            )
            .unwrap();
        assert_eq!(counts["playlists"][1]["queued"]["users"], 1);
        assert_eq!(counts["postgame"]["users"], 0);
    }

    #[test]
    fn exact_activity_window_and_disabled_catalog() {
        assert!(recent(200., 99., 100.));
        assert!(!recent(100., 99., 100.));
        assert!(!recent(200., 70., 100.));
        assert!(!recent(200., 101., 100.));
        let mut p = Population::default();
        p.seat("menu-user", false, false);
        let counts = p.finish(&[], 117, "population").unwrap();
        assert_eq!(counts["online"], counts["idle"]);
        assert_eq!(counts["playlists"], json!([]));
        assert!(p.finish(&[], 117, "roster").is_err());
    }
}
