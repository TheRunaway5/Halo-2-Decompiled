//! Native catalog/profile and compatibility/eligibility policy authority.
//!
//! Dated rules and declared native gates retain their existing provenance.
//! This parses supplied catalogs; it does not recover missing Live downloads.
use crate::matchmaking_model::{self as m, Error, Result, array, flag, n, s};
use crate::matchmaking_skill as skill;
use serde_json::{Value, json};
use std::collections::HashSet;

pub const OPTIONS: [&str; 2] = ["halo2-native-queue-v15", "halo2-native-queue-v16"];
pub const EXCLUSIVE: [&str; 6] = [
    "halo2-native-queue-v11",
    "halo2-native-queue-v12",
    "halo2-native-queue-v13",
    "halo2-native-queue-v14",
    OPTIONS[0],
    OPTIONS[1],
];
pub const GATHERING: [&str; 9] = [
    "halo2-native-queue-v8",
    "halo2-native-queue-v9",
    "halo2-native-queue-v10",
    EXCLUSIVE[0],
    EXCLUSIVE[1],
    EXCLUSIVE[2],
    EXCLUSIVE[3],
    OPTIONS[0],
    OPTIONS[1],
];
pub fn supports(actual: &str, required: &str) -> bool {
    actual == required
        || (required == OPTIONS[0] && actual == OPTIONS[1])
        || (required == "halo2-native-queue-v10" && EXCLUSIVE.contains(&actual))
}
pub fn same_continuity(versions: &[&str]) -> bool {
    EXCLUSIVE
        .iter()
        .all(|e| !versions.contains(e) || versions.iter().all(|v| v == e))
}
fn optional_text<'a>(v: &'a Value, limit: usize) -> Result<Option<&'a str>> {
    if v.is_null() {
        Ok(None)
    } else {
        Ok(Some(m::text(v, limit)?))
    }
}
fn gcd(mut a: i64, mut b: i64) -> i64 {
    while b != 0 {
        (a, b) = (b, a % b);
    }
    a
}
fn named_rule(kind: &str, name: &Value) -> Result<Value> {
    if name.is_null() {
        return Ok(Value::Null);
    }
    let name = m::text(name, 64)?;
    m::defaults()[kind]
        .get(name)
        .cloned()
        .ok_or(Error(400, "Unknown named matchmaking policy"))
}
pub fn parse(bytes: &[u8]) -> Result<Vec<Value>> {
    let root = crate::matchmaking_json::parse(bytes, 1024 * 1024)?;
    m::fields(&root, &["version", "playlists"])?;
    m::check(
        root["version"].as_i64() == Some(1),
        "Unsupported matchmaking catalog",
    )?;
    let rows = root["playlists"]
        .as_array()
        .ok_or(Error(400, "Invalid playlist catalog"))?;
    m::check((1..=64).contains(&rows.len()), "Invalid playlist catalog")?;
    let mut entries = Vec::new();
    for row in rows {
        m::fields_optional(
            row,
            &[
                "id",
                "revision",
                "name",
                "description",
                "ranked",
                "quickmatch",
                "team_size",
                "minimum_players",
                "maximum_players",
                "maximum_party",
                "games",
                "stages",
                "skill",
                "provenance",
            ],
            &[
                "party_balance",
                "rank_configuration",
                "clan_match",
                "candidate_selection",
                "gathering",
                "required_compatibility",
                "team_count",
                "hopper",
            ],
        )?;
        let games = row["games"]
            .as_array()
            .ok_or(Error(400, "Invalid game pool"))?;
        m::check((1..=100).contains(&games.len()), "Invalid game pool")?;
        let mut converted = Vec::new();
        for game in games {
            m::fields_optional(game, &["id", "map", "variant", "weight"], &["settings"])?;
            let mut game = game.clone();
            game["settings"] = game["settings"].clone();
            converted.push(game);
        }
        let stages = row["stages"]
            .as_array()
            .ok_or(Error(400, "Invalid search stages"))?;
        m::check((1..=16).contains(&stages.len()), "Invalid search stages")?;
        let mut normalized = Vec::new();
        for stage in stages {
            m::fields_optional(
                stage,
                &[
                    "after_seconds",
                    "rank_numerator",
                    "rank_denominator",
                    "minimum_connection",
                ],
                &["unbounded_skill"],
            )?;
            let num = m::integer(&stage["rank_numerator"], 1, 10000)?;
            let den = m::integer(&stage["rank_denominator"], 1, 10000)?;
            let factor = gcd(num, den);
            normalized.push(json!({"after_seconds":stage["after_seconds"],"rank_fraction":[num/factor,den/factor],
                "minimum_connection":stage["minimum_connection"],"unbounded_skill":stage.get("unbounded_skill").cloned().unwrap_or(json!(false))}));
        }
        let skill = &row["skill"];
        m::fields_optional(
            skill,
            &["kind"],
            &["radius", "high_rank_radius", "mixed_party_floor"],
        )?;
        let historical = match m::text(&skill["kind"], 64)? {
            "bungie-2007" => true,
            "custom" => false,
            _ => return Err(Error(400, "Invalid skill policy")),
        };
        m::check(
            if historical {
                skill.get("radius").is_none()
            } else {
                skill.as_object().unwrap().len() == 2 && skill.get("radius").is_some()
            },
            "Incompatible skill fields",
        )?;
        let mut gathering = row["gathering"].clone();
        if !gathering.is_null() {
            if gathering
                .as_object()
                .is_some_and(|o| o.len() == 1 && o.contains_key("retail_hopper"))
            {
                let index = m::integer(&gathering["retail_hopper"], 0, 6)?;
                let tuple = crate::search_lifecycle::value(
                    &json!({"op":"retail_configuration","index":index}),
                )
                .map_err(|_| Error(400, "Invalid retail gathering profile"))?;
                m::check(
                    row["minimum_players"] == tuple[0] && row["maximum_players"] == tuple[1],
                    "Retail gathering dimensions differ from this playlist",
                )?;
                gathering = tuple[2].clone();
            } else {
                m::fields(&gathering, &["countdown_ms", "timeout_ms", "provenance"])?;
                m::text(&gathering["provenance"], 512)?;
                m::integer(&gathering["timeout_ms"], 10000, 3600000)?;
                let clocks = gathering["countdown_ms"]
                    .as_array()
                    .ok_or(Error(400, "Invalid gathering countdown"))?;
                m::check(
                    clocks.len() == 16,
                    "Gathering requires all sixteen countdown entries",
                )?;
                for clock in clocks {
                    m::integer(clock, 0, i32::MAX as i64)?;
                }
            }
        }
        let policy = json!({"id":row["id"],"revision":row["revision"],"ranked":row["ranked"],"team_size":row["team_size"],
            "minimum_players":row["minimum_players"],"maximum_players":row["maximum_players"],"maximum_party":row["maximum_party"],
            "games":converted,"stages":normalized,"provenance":row["provenance"],"party_balance":named_rule("party_rules_2007",&row["party_balance"])?,
            "use_2007_rank_limits":historical,"high_rank_radius":skill["high_rank_radius"],"mixed_party_floor":skill.get("mixed_party_floor").cloned().unwrap_or(json!(false)),
            "custom_rank_radius":skill["radius"],"clan_match":named_rule("clan_rules_retail",&row["clan_match"])?,"candidate_selection":row["candidate_selection"],
            "gathering":gathering,"team_count":row.get("team_count").cloned().unwrap_or(json!(2)),"hopper":row["hopper"]});
        entries.push(json!({"policy":policy,"name":row["name"],"description":row["description"],"quickmatch":row["quickmatch"],
            "rank_configuration":row["rank_configuration"],"required_compatibility":row["required_compatibility"]}));
    }
    Catalog::new(&json!(entries)).map(|c| c.entries)
}
#[derive(Clone)]
pub struct Catalog {
    pub(crate) entries: Vec<Value>,
}
impl Catalog {
    pub fn new(v: &Value) -> Result<Self> {
        let entries = v.as_array().ok_or(Error(400, "Invalid admitted catalog"))?;
        m::check(
            (1..=64).contains(&entries.len()),
            "Invalid admitted catalog",
        )?;
        let mut ids = HashSet::new();
        let mut quick = 0;
        for e in entries {
            m::fields(
                e,
                &[
                    "policy",
                    "name",
                    "description",
                    "quickmatch",
                    "rank_configuration",
                    "required_compatibility",
                ],
            )?;
            m::playlist(&e["policy"])?;
            m::text(&e["name"], 64)?;
            m::text(&e["description"], 256)?;
            quick += usize::from(m::boolean(&e["quickmatch"])?);
            m::check(ids.insert(s(&e["policy"], "id")), "Duplicate playlist")?;
            let required = optional_text(&e["required_compatibility"], 64)?;
            m::check(
                required.is_none_or(|r| GATHERING.contains(&r)),
                "Unknown required native compatibility",
            )?;
            let games = array(&e["policy"], "games");
            m::check(
                !games.iter().any(|g| !g["settings"].is_null())
                    || required.is_some_and(|r| OPTIONS.contains(&r)),
                "Complete hopper options require an updated native client",
            )?;
            m::check(
                !required.is_some_and(|r| OPTIONS.contains(&r))
                    || games.iter().all(|g| {
                        !g["settings"].is_null()
                            || matches!(s(g, "variant"), "Fiesta Slayer" | "Infection Slayer")
                    }),
                "A complete hopper cannot infer original options from a variant name",
            )?;
            let conf = optional_text(&e["rank_configuration"], 64)?;
            if let Some(conf) = conf {
                skill::policy(conf)?;
            }
            m::check(
                n(&e["policy"], "team_count") == 2 || conf.is_some(),
                "Original multi-team balancing requires an authenticated raw-rating configuration",
            )?;
            m::check(
                e["policy"]["clan_match"].is_null() || conf.is_some(),
                "Clan playlists require a supported clan rank configuration",
            )?;
        }
        m::check(quick <= 1, "Only one Quickmatch default is allowed")?;
        Ok(Self {
            entries: entries.clone(),
        })
    }
    pub fn entry(&self, id: &str) -> Result<&Value> {
        self.entries
            .iter()
            .find(|e| s(&e["policy"], "id") == id)
            .ok_or(Error(404, "Catalog entry not found"))
    }
    pub fn policies(&self) -> Vec<Value> {
        self.entries.iter().map(|e| e["policy"].clone()).collect()
    }
    pub fn with_policies(&self, policies: &[Value]) -> Result<Self> {
        let mut entries = Vec::new();
        for p in policies {
            let mut e = self.entry(s(p, "id"))?.clone();
            e["policy"] = p.clone();
            entries.push(e);
        }
        Self::new(&json!(entries))
    }
    pub fn display_requests(&self) -> Vec<Value> {
        self.entries
            .iter()
            .filter(|e| flag(&e["policy"], "ranked"))
            .map(|e| e["policy"]["id"].clone())
            .collect()
    }
    pub fn public_view(
        &self,
        compatibility: &str,
        connections: &Value,
        levels: &Value,
    ) -> Result<Value> {
        let levels = levels
            .as_object()
            .ok_or(Error(400, "Stored display levels required"))?;
        let expected = self.display_requests();
        m::check(
            levels.len() == expected.len()
                && expected
                    .iter()
                    .all(|key| levels.contains_key(key.as_str().unwrap())),
            "Public level owners differ from admitted catalog",
        )?;
        let mut rows = Vec::new();
        for e in &self.entries {
            let p = &e["policy"];
            let level = if flag(p, "ranked") {
                let value = &levels[s(p, "id")];
                if value.is_null() && !p["clan_match"].is_null() {
                    Value::Null
                } else {
                    json!(m::integer(value, 1, 50)?)
                }
            } else {
                Value::Null
            };
            let mut row = json!({"id":p["id"],"revision":p["revision"],"name":e["name"],"description":e["description"],
                "ranked":p["ranked"],"quickmatch":e["quickmatch"],"minimum_players":p["minimum_players"],"maximum_players":p["maximum_players"],
                "eligible":eligible(e,compatibility,connections)?,"level":level});
            if !p["clan_match"].is_null() {
                row["clan_match"] = json!(true);
            }
            rows.push(row);
        }
        Ok(json!({"playlists":rows}))
    }
    pub fn profile(&self, policy: &Value) -> Result<Value> {
        let entry = self.entry(s(policy, "id"))?;
        let mut metadata = entry.clone();
        metadata.as_object_mut().unwrap().remove("policy");
        let rank_identity = if let Some(name) = optional_text(&entry["rank_configuration"], 64)? {
            let (_, digest) = skill::policy(name)?;
            json!({"name":name,"table_sha256":digest})
        } else {
            Value::Null
        };
        Ok(json!({"policy":policy,"catalog":metadata,"rank_identity":rank_identity}))
    }
}
pub fn clan_group(rule: &Value, seats: &[Value]) -> Result<Value> {
    m::check(
        m::defaults()["clan_rules_retail"]
            .as_object()
            .unwrap()
            .values()
            .any(|known| known == rule),
        "Known admitted clan lineup policy required",
    )?;
    if !(n(rule, "minimum")..=n(rule, "maximum")).contains(&(seats.len() as i64)) {
        return Err(Error(
            409,
            "Clan party does not meet admitted lineup limits",
        ));
    }
    let mut clans = HashSet::new();
    for seat in seats {
        let clan = optional_text(&seat["clan"], 64)?.ok_or(Error(
            409,
            "Every party member must belong to the same accepted clan",
        ))?;
        clans.insert(clan);
    }
    if clans.len() != 1 {
        return Err(Error(
            409,
            "Every party member must belong to the same accepted clan",
        ));
    }
    Ok(json!(clans.into_iter().next().unwrap()))
}
/// Eligibility is a native UI/admission policy over authenticated IO facts.
/// It supplies neither verified identity proofs nor original Live procedure data.
pub fn eligible(entry: &Value, compatibility: &str, connections: &Value) -> Result<bool> {
    let p = &entry["policy"];
    let connections = connections
        .as_array()
        .ok_or(Error(400, "Invalid physical connections"))?;
    m::check(
        !connections.is_empty() && connections.len() <= 16,
        "Invalid physical connection count",
    )?;
    let mut versions = Vec::new();
    let mut active = Vec::new();
    let mut seats = Vec::new();
    let mut identities = HashSet::new();
    for connection in connections {
        if connection.is_null() {
            continue;
        }
        m::fields(connection, &["id", "compatibility", "maps", "seats"])?;
        m::text(&connection["id"], 64)?;
        versions.push(m::text(&connection["compatibility"], 64)?);
        let maps = connection["maps"]
            .as_array()
            .ok_or(Error(400, "Invalid connection maps"))?;
        m::check((1..=256).contains(&maps.len()), "Invalid connection maps")?;
        for map in maps {
            m::text(map, 64)?;
        }
        let rows = connection["seats"]
            .as_array()
            .ok_or(Error(400, "Invalid connection seats"))?;
        m::check((1..=4).contains(&rows.len()), "Invalid connection seats")?;
        for seat in rows {
            m::fields(seat, &["id", "guest", "clan"])?;
            m::text(&seat["id"], 64)?;
            m::boolean(&seat["guest"])?;
            optional_text(&seat["clan"], 64)?;
            m::check(identities.insert(s(seat, "id")), "Duplicate admitted seat")?;
            seats.push(seat.clone());
        }
        active.push(connection.clone());
    }
    let required = entry["required_compatibility"].as_str();
    if required.is_some_and(|r| !supports(compatibility, r))
        || (!p["candidate_selection"].is_null() && !GATHERING.contains(&compatibility))
        || !same_continuity(&versions)
    {
        return Ok(false);
    }
    if required.is_some_and(|r| {
        connections
            .iter()
            .any(|c| c.is_null() || !supports(s(c, "compatibility"), r))
    }) {
        return Ok(false);
    }
    if seats.len() > n(p, "maximum_party") as usize
        || ((flag(p, "ranked") || !p["clan_match"].is_null())
            && seats.iter().any(|s| flag(s, "guest")))
    {
        return Ok(false);
    }
    if !p["clan_match"].is_null() && clan_group(&p["clan_match"], &seats).is_err() {
        return Ok(false);
    }
    Ok(!m::available(array(p, "games"), &active).is_empty()
        && connections.len() <= n(p, "maximum_party") as usize)
}
pub fn value(r: &Value) -> Result<Value> {
    match m::text(&r["op"], 64)? {
        "catalog_parse" => Ok(json!(parse(
            r["raw"]
                .as_str()
                .ok_or(Error(400, "Catalog UTF8 text required"))?
                .as_bytes()
        )?)),
        "catalog_validate" => Ok(json!(Catalog::new(&r["entries"])?.entries)),
        "catalog_supports" => Ok(json!(supports(
            m::text(&r["actual"], 64)?,
            m::text(&r["required"], 64)?
        ))),
        "catalog_continuity" => {
            let values = r["versions"]
                .as_array()
                .ok_or(Error(400, "Invalid protocol versions"))?;
            m::check(values.len() <= 1000, "Too many protocol versions")?;
            let versions = values
                .iter()
                .map(|v| m::text(v, 64))
                .collect::<Result<Vec<_>>>()?;
            Ok(json!(same_continuity(&versions)))
        }
        _ => Err(Error(400, "Unknown catalog operation")),
    }
}
