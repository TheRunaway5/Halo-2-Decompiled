//! Original retail/TU1.5 XP arithmetic and separately named skill domains.
//!
//! These initializer tables are authenticated reference inputs, not recovered
//! downloaded service profiles. Production comparison-skill provenance remains
//! a separate requirement from the pair/collection/rank arithmetic here.
use crate::matchmaking_model::{self as m, Error, Result};
use serde_json::{Value, json};
use sha2::{Digest, Sha256};
use std::sync::OnceLock;

pub const MAX_XP: i32 = 0x3fffffff;
pub const RETAIL: &str = "retail-launch-2004";
pub const TU15: &str = "title-update-1.5-defaults";
pub const RETAIL_DIGEST: &str = "5b5760a487d6af59da885edb89f2d7d7e76df5fa0d2379cb7a43ff9dfa3e55bd";
pub const TU15_DIGEST: &str = "9ad56e3c3ef7249e8ada858a29016b4ecaca0148b9deb4efe87efa7ded15c58f";

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct ComparisonRank(pub i32);
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct PersistentRawRank(pub u8);
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct PublicLevel(pub u8);
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Xp(pub i32);

#[derive(Clone)]
pub struct Tables {
    value: Value,
    win: [i32; 255],
    loss: [i32; 255],
    win_percent: [i32; 128],
    loss_percent: [i32; 128],
    win_offsets: [i32; 128],
    loss_offsets: [i32; 128],
    thresholds: [i32; 128],
    bounds: [i32; 2],
}
fn values<const N: usize>(v: &Value, key: &str, low: i64, high: i64) -> Result<[i32; N]> {
    let a = v[key]
        .as_array()
        .ok_or(Error(400, "Invalid rank table array"))?;
    m::check(
        a.len() == N,
        "Rank table length differs from original layout",
    )?;
    let mut out = [0; N];
    for (o, v) in out.iter_mut().zip(a) {
        *o = m::integer(v, low, high)? as i32;
    }
    Ok(out)
}
impl Tables {
    pub fn parse(v: &Value) -> Result<Self> {
        m::fields(
            v,
            &[
                "win_exchange",
                "loss_exchange",
                "win_percent",
                "loss_percent",
                "win_offsets",
                "loss_offsets",
                "xp_thresholds",
                "delta_bounds",
            ],
        )?;
        Ok(Self {
            value: v.clone(),
            win: values(v, "win_exchange", -32768, 32767)?,
            loss: values(v, "loss_exchange", -32768, 32767)?,
            win_percent: values(v, "win_percent", 0, 255)?,
            loss_percent: values(v, "loss_percent", 0, 255)?,
            win_offsets: values(v, "win_offsets", -32768, 32767)?,
            loss_offsets: values(v, "loss_offsets", -32768, 32767)?,
            thresholds: values(v, "xp_thresholds", i32::MIN as i64, i32::MAX as i64)?,
            bounds: values(v, "delta_bounds", i32::MIN as i64, i32::MAX as i64)?,
        })
    }
    pub fn value(&self) -> Value {
        self.value.clone()
    }
    pub fn pair(
        &self,
        own: ComparisonRank,
        other: ComparisonRank,
        place: i32,
        other_place: i32,
    ) -> Result<i32> {
        m::check(
            (0..=127).contains(&own.0) && (0..=127).contains(&other.0),
            "Invalid comparison rank",
        )?;
        let win = place != -1 && (other_place == -1 || place < other_place);
        let loss = other_place != -1 && (place == -1 || other_place < place);
        if !win && !loss {
            return Ok(0);
        }
        let at = (own.0 - other.0 + 127) as usize;
        let own = own.0 as usize;
        Ok(if win {
            (self.win[at] * self.win_percent[own]) / 100 + self.win_offsets[own]
        } else {
            (self.loss[at] * self.loss_percent[own]) / 100 + self.loss_offsets[own]
        })
    }
    pub fn player(
        &self,
        players: &[RatingPlayer],
        selected: usize,
        key: i32,
        team_places: Option<&Value>,
    ) -> Result<i32> {
        m::check(
            !players.is_empty() && players.len() <= 16 && selected < players.len(),
            "Invalid player collection",
        )?;
        let own = &players[selected];
        let mut sum = 0i32;
        let mut count = 0;
        let place = |p: &RatingPlayer| -> Result<i32> {
            let Some(places) = team_places else {
                return Ok(p.place);
            };
            let value = if let Some(values) = places.as_array() {
                values.get(p.team as usize)
            } else {
                places.get(p.team.to_string())
            };
            Ok(m::integer(value.ok_or(Error(400, "Missing team standing"))?, -128, 127)? as i32)
        };
        let own_place = place(own)?;
        for (index, other) in players.iter().enumerate() {
            if index == selected
                || other.key != key
                || !other.active
                || other.team == -1
                || other.flags & 3 != 0
                || other.skill.0 == -1
                || other.team == own.team
            {
                continue;
            }
            sum += self.pair(own.skill, other.skill, own_place, place(other)?)?;
            count += 1;
        }
        let mut delta = if count == 0 { 0 } else { sum / count };
        if i64::from(own.xp.0) + i64::from(delta) < 0 {
            delta = -own.xp.0;
        }
        if i64::from(own.xp.0) + i64::from(delta) > i64::from(MAX_XP) {
            delta = MAX_XP - own.xp.0;
        }
        Ok(if delta < self.bounds[1] {
            self.bounds[1]
        } else if delta > self.bounds[0] {
            self.bounds[0]
        } else {
            delta
        })
    }
    pub fn clan(&self, clans: &[RatingClan], selected: usize) -> Result<i32> {
        m::check(
            !clans.is_empty() && clans.len() <= 16 && selected < clans.len(),
            "Invalid clan collection",
        )?;
        let own = &clans[selected];
        let mut sum = 0;
        let mut count = 0;
        for (index, other) in clans.iter().enumerate() {
            if index != selected && other.active && other.skill.0 != -1 {
                sum += self.pair(own.skill, other.skill, own.place, other.place)?;
                count += 1;
            }
        }
        let delta = if count == 0 { 0 } else { sum / count };
        Ok(delta
            .max(-own.xp.0)
            .min(MAX_XP - own.xp.0)
            .min(self.bounds[0])
            .max(self.bounds[1]))
    }
    pub fn rank(&self, xp: Xp, previous: PersistentRawRank) -> PersistentRawRank {
        for i in 1..128 {
            if xp.0 < self.thresholds[i] {
                let candidate = i as u8 - 1;
                // Original signed addition/division, including the strict midpoint.
                let midpoint = self.thresholds[i - 1].wrapping_add(self.thresholds[i]) / 2;
                return if i32::from(candidate) == i32::from(previous.0) - 1 && xp.0 > midpoint {
                    previous
                } else {
                    PersistentRawRank(candidate)
                };
            }
        }
        PersistentRawRank(127)
    }
}
pub fn display(raw: PersistentRawRank) -> PublicLevel {
    PublicLevel(raw.0.min(49) + 1)
}

pub fn policy(name: &str) -> Result<(&'static Tables, &'static str)> {
    static A: OnceLock<Result<Tables>> = OnceLock::new();
    static B: OnceLock<Result<Tables>> = OnceLock::new();
    let (slot, bytes, pin) = match name {
        RETAIL => (
            &A,
            include_bytes!("../data/retail-launch-2004-rank.json").as_slice(),
            RETAIL_DIGEST,
        ),
        TU15 => (
            &B,
            include_bytes!("../data/title-update-1.5-rank.json").as_slice(),
            TU15_DIGEST,
        ),
        _ => return Err(Error(409, "Unknown rank configuration")),
    };
    let tables = slot
        .get_or_init(|| {
            m::check(
                format!("{:x}", Sha256::digest(bytes)) == pin,
                "Unverified original rank configuration",
            )?;
            let value: Value = serde_json::from_slice(bytes)
                .map_err(|_| Error(500, "Invalid embedded rank metadata"))?;
            Tables::parse(&value["tables"])
        })
        .as_ref()
        .map_err(|e| *e)?;
    Ok((tables, pin))
}

#[derive(Clone)]
pub struct RatingPlayer {
    pub skill: ComparisonRank,
    pub place: i32,
    pub team: i32,
    pub xp: Xp,
    pub key: i32,
    pub flags: u32,
    pub active: bool,
}
impl RatingPlayer {
    pub fn parse(v: &Value) -> Result<Self> {
        m::fields(
            v,
            &["skill", "place", "team", "xp", "key", "flags", "active"],
        )?;
        Ok(Self {
            skill: ComparisonRank(m::integer(&v["skill"], -1, 127)? as i32),
            place: m::integer(&v["place"], -128, 127)? as i32,
            team: m::integer(&v["team"], -128, 127)? as i32,
            xp: Xp(m::integer(&v["xp"], 0, MAX_XP as i64)? as i32),
            key: m::integer(&v["key"], i32::MIN as i64, i32::MAX as i64)? as i32,
            flags: m::integer(&v["flags"], 0, 255)? as u32,
            active: m::boolean(&v["active"])?,
        })
    }
}
#[derive(Clone)]
pub struct RatingClan {
    pub skill: ComparisonRank,
    pub place: i32,
    pub xp: Xp,
    pub active: bool,
}
impl RatingClan {
    pub fn parse(v: &Value) -> Result<Self> {
        m::fields(v, &["skill", "place", "xp", "active"])?;
        Ok(Self {
            skill: ComparisonRank(m::integer(&v["skill"], -1, 127)? as i32),
            place: m::integer(&v["place"], -128, 127)? as i32,
            xp: Xp(m::integer(&v["xp"], 0, MAX_XP as i64)? as i32),
            active: m::boolean(&v["active"])?,
        })
    }
}
pub fn value(r: &Value) -> Result<Value> {
    let op = m::text(&r["op"], 64)?;
    if op == "rank_metadata" {
        return Ok(
            json!({"max_xp":MAX_XP,"policy":RETAIL,"table_digest":RETAIL_DIGEST,"later_policy":TU15,"later_digest":TU15_DIGEST}),
        );
    }
    if op == "rank_policy" {
        let (tables, digest) = policy(m::text(&r["name"], 64)?)?;
        return Ok(json!({"tables":tables.value(),"digest":digest}));
    }
    if op == "rank_policies" {
        return Ok(json!([RETAIL, TU15]));
    }
    if op == "rank_display" {
        return Ok(json!(
            display(PersistentRawRank(m::integer(&r["rank"], 0, 127)? as u8)).0
        ));
    }
    if op == "rank_divide" {
        return Ok(json!(
            m::integer(&r["value"], i32::MIN as i64, i32::MAX as i64)?
                / m::integer(&r["denominator"], 1, i32::MAX as i64)?
        ));
    }
    let tables = Tables::parse(&r["tables"])?;
    match op {
        "rank_pair" => Ok(json!(tables.pair(
            ComparisonRank(m::integer(&r["own"], 0, 127)? as i32),
            ComparisonRank(m::integer(&r["other"], 0, 127)? as i32),
            m::integer(&r["place"], -128, 127)? as i32,
            m::integer(&r["other_place"], -128, 127)? as i32
        )?)),
        "rank_player" => {
            let values = r["players"]
                .as_array()
                .ok_or(Error(400, "Invalid rated players"))?;
            m::check(values.len() <= 16, "Player rating capacity exceeded")?;
            let players = values
                .iter()
                .map(RatingPlayer::parse)
                .collect::<Result<Vec<_>>>()?;
            Ok(json!(tables.player(
                &players,
                m::integer(&r["selected"], 0, 15)? as usize,
                m::integer(&r["key"], i32::MIN as i64, i32::MAX as i64)? as i32,
                if r["team_places"].is_null() {
                    None
                } else {
                    Some(&r["team_places"])
                }
            )?))
        }
        "rank_clan" => {
            let values = r["clans"]
                .as_array()
                .ok_or(Error(400, "Invalid rated clans"))?;
            m::check(values.len() <= 16, "Clan rating capacity exceeded")?;
            let clans = values
                .iter()
                .map(RatingClan::parse)
                .collect::<Result<Vec<_>>>()?;
            Ok(json!(tables.clan(
                &clans,
                m::integer(&r["selected"], 0, 15)? as usize
            )?))
        }
        "rank_raw" => Ok(json!(
            tables
                .rank(
                    Xp(m::integer(&r["xp"], 0, MAX_XP as i64)? as i32),
                    PersistentRawRank(m::integer(&r["previous"], 0, 127)? as u8)
                )
                .0
        )),
        _ => Err(Error(400, "Unknown rank operation")),
    }
}
