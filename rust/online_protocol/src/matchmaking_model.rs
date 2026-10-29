//! Validated owned models for the native matchmaking coordinator.
//! Historical data and native replacement policies retain explicit provenance.
use crate::{
    matchmaking_ratings::RankMap, search_lifecycle::GatheringConfiguration, team_assignment::Hopper,
};
use serde_json::{Value, json};
use std::collections::HashSet;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Error(pub u16, pub &'static str);
pub type Result<T> = std::result::Result<T, Error>;
pub fn check(condition: bool, message: &'static str) -> Result<()> {
    if condition {
        Ok(())
    } else {
        Err(Error(400, message))
    }
}
pub fn fields(v: &Value, names: &[&str]) -> Result<()> {
    let object = v.as_object().ok_or(Error(400, "Object required"))?;
    check(
        object.len() == names.len() && names.iter().all(|n| object.contains_key(*n)),
        "Invalid matchmaking fields",
    )
}
pub fn fields_optional(v: &Value, required: &[&str], optional: &[&str]) -> Result<()> {
    let object = v.as_object().ok_or(Error(400, "Object required"))?;
    check(
        required.iter().all(|key| object.contains_key(*key))
            && object
                .keys()
                .all(|key| required.contains(&key.as_str()) || optional.contains(&key.as_str())),
        "Invalid matchmaking fields",
    )
}
pub fn text(v: &Value, limit: usize) -> Result<&str> {
    let value = v.as_str().ok_or(Error(400, "Invalid text"))?;
    check(
        !value.is_empty() && value.len() <= limit,
        "Invalid text length",
    )?;
    check(
        value.chars().all(|c| c >= ' ' && c != '\x7f'),
        "Control character in text",
    )?;
    Ok(value)
}
pub fn integer(v: &Value, low: i64, high: i64) -> Result<i64> {
    let n = v.as_i64().ok_or(Error(400, "Invalid integer"))?;
    check((low..=high).contains(&n), "Integer out of range")?;
    Ok(n)
}
pub fn boolean(v: &Value) -> Result<bool> {
    v.as_bool().ok_or(Error(400, "Invalid boolean"))
}
pub fn real(v: &Value) -> Result<f64> {
    let n = v.as_f64().ok_or(Error(400, "Invalid coordinator clock"))?;
    check(n.is_finite(), "Invalid coordinator clock")?;
    Ok(n)
}
pub fn hex(v: &Value, bytes: usize) -> Result<Vec<u8>> {
    let s = v
        .as_str()
        .ok_or(Error(400, "Invalid byte carrier"))?
        .as_bytes();
    check(s.len() == bytes * 2, "Invalid byte length")?;
    fn nibble(c: u8) -> Option<u8> {
        match c {
            b'0'..=b'9' => Some(c - b'0'),
            b'a'..=b'f' => Some(c - b'a' + 10),
            _ => None,
        }
    }
    s.chunks_exact(2)
        .map(|p| {
            Ok(nibble(p[0]).ok_or(Error(400, "Invalid hex"))? * 16
                + nibble(p[1]).ok_or(Error(400, "Invalid hex"))?)
        })
        .collect()
}
pub fn n(v: &Value, k: &str) -> i64 {
    v[k].as_i64().expect("validated integer")
}
pub fn s<'a>(v: &'a Value, k: &str) -> &'a str {
    v[k].as_str().expect("validated string")
}
pub fn flag(v: &Value, k: &str) -> bool {
    v[k].as_bool().expect("validated boolean")
}
pub fn array<'a>(v: &'a Value, k: &str) -> &'a [Value] {
    v[k].as_array().expect("validated array")
}

pub fn rank_radius(level: i32, high: Option<i32>) -> Result<i32> {
    check((1..=50).contains(&level), "Invalid public level")?;
    Ok(match level {
        1..=6 => 11 - level,
        7..=16 => 6,
        17..=26 => 7,
        27..=29 => 8,
        30..=35 => level - 21,
        _ => high.filter(|n| (0..=49).contains(n)).ok_or(Error(
            400,
            "Original high-rank table is ambiguous; explicit tuning required",
        ))?,
    })
}

pub fn defaults() -> Value {
    json!({
        "party_rules_2007":{
            "team_skirmish":{"minimum_large":3,"maximum":4,"imbalance":0},
            "big_team_battle":{"minimum_large":3,"maximum":5,"imbalance":2},
            "minor_clan_match":{"minimum_large":3,"maximum":4,"imbalance":0},
            "major_clan_match":{"minimum_large":6,"maximum":8,"imbalance":2},
            "double_team":{"minimum_large":2,"maximum":2,"imbalance":0},
            "team_preview":{"minimum_large":3,"maximum":4,"imbalance":0}},
        "clan_rules_retail":{
            "minor-retail-launch-2004":{"minimum":3,"maximum":4,"imbalance":0},
            "major-retail-launch-2004":{"minimum":6,"maximum":8,"imbalance":2}}
    })
}

pub fn player(v: &Value) -> Result<()> {
    fields(
        v,
        &[
            "id",
            "screen_name",
            "rank",
            "maps",
            "raw_rank",
            "guest",
            "connection_owner",
        ],
    )?;
    text(&v["id"], 64)?;
    text(&v["screen_name"], 64)?;
    boolean(&v["guest"])?;
    if !v["connection_owner"].is_null() {
        text(&v["connection_owner"], 64)?;
    }
    let rank = integer(&v["rank"], 1, 50)?;
    if !v["raw_rank"].is_null() {
        let raw = integer(&v["raw_rank"], 0, 127)?;
        check(
            rank == raw.min(49) + 1,
            "Display level differs from private skill",
        )?;
    }
    let maps = v["maps"]
        .as_array()
        .ok_or(Error(400, "Invalid installed-map set"))?;
    check((1..=256).contains(&maps.len()), "Invalid installed-map set")?;
    let mut seen = HashSet::new();
    for map in maps {
        check(seen.insert(text(map, 64)?), "Duplicate installed map")?;
    }
    Ok(())
}

pub fn game(v: &Value, teams: bool) -> Result<()> {
    fields(v, &["id", "map", "variant", "weight", "settings"])?;
    for k in ["id", "map", "variant"] {
        text(&v[k], 64)?;
    }
    integer(&v["weight"], i32::MIN as i64, i32::MAX as i64)?;
    if !v["settings"].is_null() {
        let raw = hex(&v["settings"], 304)?;
        let engine = i32::from_le_bytes(raw[0x44..0x48].try_into().unwrap());
        check(
            [1, 2, 3, 4, 7, 8, 9].contains(&engine) && (raw[0x48] & 1 != 0) == teams,
            "Hopper variant engine/team rule differs from playlist",
        )?;
    } else {
        check(
            crate::matchmaking_coordinator::known_variant(s(v, "variant"))?.is_some(),
            "Unfamiliar variants require complete admitted settings",
        )?;
    }
    Ok(())
}

pub fn stage(v: &Value) -> Result<()> {
    fields(
        v,
        &[
            "after_seconds",
            "rank_fraction",
            "minimum_connection",
            "unbounded_skill",
        ],
    )?;
    integer(&v["after_seconds"], 0, 3600)?;
    integer(&v["minimum_connection"], 0, 100)?;
    boolean(&v["unbounded_skill"])?;
    let f = v["rank_fraction"]
        .as_array()
        .ok_or(Error(400, "Invalid rank expansion fraction"))?;
    check(f.len() == 2, "Invalid rank expansion fraction")?;
    let numerator = integer(&f[0], 1, i64::MAX)?;
    let denominator = integer(&f[1], 1, i64::MAX)?;
    check(numerator <= denominator, "Invalid rank expansion fraction")
}

pub fn playlist(v: &Value) -> Result<()> {
    fields(
        v,
        &[
            "id",
            "revision",
            "ranked",
            "team_size",
            "minimum_players",
            "maximum_players",
            "maximum_party",
            "games",
            "stages",
            "provenance",
            "party_balance",
            "use_2007_rank_limits",
            "high_rank_radius",
            "mixed_party_floor",
            "custom_rank_radius",
            "clan_match",
            "candidate_selection",
            "gathering",
            "team_count",
            "hopper",
        ],
    )?;
    for k in ["id", "revision", "provenance"] {
        text(&v[k], 512)?;
    }
    let ranked = boolean(&v["ranked"])?;
    let minimum = integer(&v["minimum_players"], 2, 16)?;
    let maximum = integer(&v["maximum_players"], minimum, 16)?;
    let team = integer(&v["team_size"], 0, 8)?;
    let count = integer(&v["team_count"], 2, 4)?;
    let party = integer(
        &v["maximum_party"],
        1,
        if !ranked || team == 0 { maximum } else { team },
    )?;
    check(
        team == 0 || maximum == count * team,
        "Team playlist capacity differs from its configured team count",
    )?;
    let clan = !v["clan_match"].is_null();
    let significant = !v["party_balance"].is_null();
    check(
        count == 2 || (team > 0 && !ranked && !clan && !significant),
        "Original multi-team assignment requires an explicit casual team policy",
    )?;
    let candidate = !v["candidate_selection"].is_null();
    check(
        !candidate || v["candidate_selection"] == "retail-candidates-native-relay-v1",
        "Unknown candidate selection policy",
    )?;
    check(
        candidate == !v["gathering"].is_null(),
        "Gathering selection requires explicit countdown configuration",
    )?;
    if candidate {
        GatheringConfiguration::from_json(&v["gathering"])
            .map_err(|_| Error(400, "Invalid gathering configuration"))?;
    }
    let header = !v["hopper"].is_null();
    check(
        team == 0 || clan || header || minimum == maximum,
        "Reduced team counts require a separately specified policy",
    )?;
    if header {
        let raw = hex(&v["hopper"], 0x614)?;
        let h = Hopper::from_header(&raw).ok_or(Error(400, "Invalid complete hopper header"))?;
        let kind = if clan {
            5
        } else if team > 0 {
            if ranked { 4 } else { 3 }
        } else if ranked {
            2
        } else {
            1
        };
        check(h.kind as i64 == kind, "Hopper type differs from playlist")?;
        check(
            if team > 0 {
                (
                    i64::from(h.width) * i64::from(h.height),
                    i64::from(h.width) * i64::from(h.depth),
                ) == (minimum, maximum)
            } else {
                (i64::from(h.width), i64::from(h.height)) == (minimum, maximum)
            },
            "Hopper population bounds differ from playlist",
        )?;
        check(
            team == 0
                || (i64::from(h.width) == count
                    && i64::from(h.depth) == team
                    && (1..=h.depth).contains(&h.height)),
            "Hopper team dimensions differ from playlist",
        )?;
        check(
            (0..=15).contains(&h.imbalance) && raw[0x604] <= 1,
            "Invalid hopper assignment flags",
        )?;
        RankMap::from_header(&raw).ok_or(Error(400, "Invalid hopper rank table"))?;
    }
    let historical = boolean(&v["use_2007_rank_limits"])?;
    let mixed = boolean(&v["mixed_party_floor"])?;
    if clan {
        let c = &v["clan_match"];
        fields(c, &["minimum", "maximum", "imbalance"])?;
        check(
            defaults()["clan_rules_retail"]
                .as_object()
                .unwrap()
                .values()
                .any(|r| r == c),
            "Unknown clan lineup policy",
        )?;
        check(
            ranked
                && team == n(c, "maximum")
                && party == team
                && minimum == 2 * n(c, "minimum")
                && maximum == 2 * team
                && !significant
                && !mixed
                && !historical,
            "Clan playlist dimensions must match its recovered lineup policy",
        )?;
    }
    let games = v["games"]
        .as_array()
        .ok_or(Error(400, "Invalid game pool"))?;
    check((1..=100).contains(&games.len()), "Invalid game pool")?;
    let mut ids = HashSet::new();
    let mut total = 0_i64;
    for row in games {
        game(row, team > 0)?;
        check(ids.insert(s(row, "id")), "Duplicate game entry")?;
        total += n(row, "weight").clamp(1, 1000);
    }
    check(
        total <= 32766,
        "Hopper weight total exceeds the original signed-short draw domain",
    )?;
    let stages = v["stages"]
        .as_array()
        .ok_or(Error(400, "Invalid search stages"))?;
    check((1..=16).contains(&stages.len()), "Invalid search stages")?;
    let (mut time, mut fraction, mut connection) = (-1, (0_i128, 1_i128), 101);
    for (index, row) in stages.iter().enumerate() {
        stage(row)?;
        let f = &row["rank_fraction"];
        let pair = (
            i128::from(f[0].as_i64().unwrap()),
            i128::from(f[1].as_i64().unwrap()),
        );
        check(
            !flag(row, "unbounded_skill")
                || (!ranked && index + 1 == stages.len() && n(row, "after_seconds") > 0),
            "Only a final casual retry may remove skill bounds",
        )?;
        check(
            n(row, "after_seconds") > time
                && pair.0 * fraction.1 >= fraction.0 * pair.1
                && n(row, "minimum_connection") <= connection,
            "Search stages must progressively relax",
        )?;
        time = n(row, "after_seconds");
        fraction = pair;
        connection = n(row, "minimum_connection");
    }
    check(
        n(&stages[0], "after_seconds") == 0,
        "Search must start at zero",
    )?;
    check(
        historical != !v["custom_rank_radius"].is_null(),
        "Choose historical ranked limits or an explicitly custom skill radius",
    )?;
    if !v["custom_rank_radius"].is_null() {
        integer(&v["custom_rank_radius"], 0, 49)?;
    }
    if !v["high_rank_radius"].is_null() {
        integer(&v["high_rank_radius"], 0, 49)?;
        check(historical, "High-rank tuning requires the historical table")?;
    }
    check(
        !mixed || historical,
        "Mixed-party inference requires the corresponding historical table",
    )?;
    if significant {
        let b = &v["party_balance"];
        fields(b, &["minimum_large", "maximum", "imbalance"])?;
        let min = integer(&b["minimum_large"], 2, 8)?;
        let max = integer(&b["maximum"], min, 8)?;
        integer(&b["imbalance"], 0, 7)?;
        check(
            team > 0 && party == max,
            "Large-party policy must agree with playlist limits",
        )?;
    }
    Ok(())
}

pub fn search_rank(player: &Value) -> i32 {
    player["raw_rank"]
        .as_i64()
        .unwrap_or_else(|| n(player, "rank")) as i32
}
pub fn party_rank(players: &[Value]) -> Result<i32> {
    let raw = players.iter().map(search_rank).collect::<Vec<_>>();
    let flags = players
        .iter()
        .map(|p| u32::from(flag(p, "guest")))
        .collect::<Vec<_>>();
    let stats = crate::matchmaking_ratings::raw_statistics(&raw, &flags)
        .ok_or(Error(400, "Invalid rating roster"))?;
    check(
        stats.rated_count > 0,
        "A couch party needs a signed-in player",
    )?;
    Ok(stats.average)
}
pub fn available(games: &[Value], players: &[Value]) -> Vec<usize> {
    games
        .iter()
        .enumerate()
        .filter_map(|(i, g)| {
            (!players.is_empty() && players.iter().all(|p| array(p, "maps").contains(&g["map"])))
                .then_some(i)
        })
        .collect()
}
