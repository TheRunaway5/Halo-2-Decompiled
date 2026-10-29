//! Immutable native result admission, report validation and consent authority.
//!
//! Unanimity/quarantine is declared native policy, not recovered Live
//! arbitration or independent simulation truth. Original raw/effective input
//! provenance and native executable/map attestation remain separate gates.
use super::{Coordinator, fingerprint, m, reject};
use crate::matchmaking_model::{Error, Result, array, flag, n, s};
use crate::matchmaking_skill as skill;
use serde_json::{Value, json};
use sha2::{Digest, Sha256};
use std::collections::{BTreeSet, HashMap};
use std::sync::OnceLock;

pub(super) fn presets() -> Result<&'static Value> {
    static PRESETS: OnceLock<Result<Value>> = OnceLock::new();
    PRESETS
        .get_or_init(|| {
            let data = include_bytes!("../data/tu15-result-presets.json");
            m::check(
                format!("{:x}", Sha256::digest(data))
                    == "70153f1eedc392124623d8f42ee775be2305b95a34d563dbcf9ab80c503f30db",
                "Unverified original preset registry",
            )?;
            serde_json::from_slice(data).map_err(|_| Error(500, "Invalid preset metadata"))
        })
        .as_ref()
        .map_err(|e| *e)
}
pub(super) fn known_variant(name: &str) -> Result<Option<(u32, bool)>> {
    Ok(presets()?["variants"]
        .as_array()
        .unwrap()
        .iter()
        .find(|p| p["name"] == name)
        .map(|p| {
            (
                p["engine"].as_u64().unwrap() as u32,
                p["team"].as_bool().unwrap(),
            )
        }))
}
pub(super) fn legacy_variant(name: &str) -> Result<Option<(u32, bool)>> {
    // Exact authored native rulesets in data/tags/variants and
    // custom_slayer.cpp, not original presets or an unknown-name fallback.
    // Legacy clients load these named assets locally; full extension settings
    // and executable attestation remain an explicit unclosed delivery boundary.
    if matches!(name, "Fiesta Slayer" | "Infection Slayer") {
        return Ok(Some((2, false)));
    }
    known_variant(name)
}
fn bytes_hex(bytes: &[u8]) -> String {
    bytes.iter().map(|b| format!("{b:02x}")).collect()
}
fn skill_snapshot(v: &Value, configuration: &str) -> Result<Value> {
    m::fields(v, &["raw", "xp", "configuration"])?;
    m::integer(&v["raw"], 0, 127)?;
    m::integer(&v["xp"], 0, skill::MAX_XP as i64)?;
    reject(
        v["configuration"] == configuration,
        409,
        "Skill configuration differs from admission",
    )?;
    Ok(v.clone())
}
fn configuration(v: &Value) -> Result<Option<&str>> {
    if v.is_null() {
        return Ok(None);
    }
    let name = m::text(v, 64)?;
    skill::policy(name)?;
    Ok(Some(name))
}

pub(super) struct Record {
    pub context: Value,
    pub digest: String,
    pub report: Vec<u8>,
    pub confirmations: BTreeSet<String>,
    pub active: BTreeSet<String>,
    pub owners: BTreeSet<String>,
    pub quarantined: Option<&'static str>,
    pub committed: bool,
}
impl Record {
    fn new(core: &Coordinator, r: &Value) -> Result<Self> {
        m::fields(
            r,
            &[
                "proposal",
                "skill_configuration",
                "clan_configuration",
                "skills",
                "clans",
                "clan_skills",
                "created_wall",
            ],
        )?;
        let proposal = core.proposal(m::text(&r["proposal"], 64)?)?;
        let policy = core.policy(s(proposal, "playlist"))?;
        let binding = core
            .proposal_bindings
            .get(s(proposal, "id"))
            .ok_or(Error(404, "Proposal binding not found"))?;
        let group = core.group(&super::strings(&proposal["tickets"])?)?;
        reject(
            group.iter().all(|t| {
                core.current_profile(policy, t) && t["expires_at"].as_f64().unwrap() > core.clock
            }),
            409,
            "Result admission profile or ticket changed",
        )?;
        let team_mode = n(policy, "team_size") != 0;
        let admitted = Coordinator::players(&group);
        let mut players = Vec::new();
        let mut owners = BTreeSet::new();
        let mut identities = serde_json::Map::new();
        for (team, rows) in array(proposal, "teams").iter().enumerate() {
            for member in rows.as_array().unwrap() {
                let player = admitted
                    .iter()
                    .find(|p| p["id"] == *member)
                    .ok_or(Error(409, "Result roster changed"))?;
                let owner = player["connection_owner"]
                    .as_str()
                    .unwrap_or_else(|| s(player, "id"));
                owners.insert(owner.to_owned());
                m::check(
                    admitted
                        .iter()
                        .any(|p| p["id"] == owner && !flag(p, "guest")),
                    "Physical result owner is not admitted",
                )?;
                m::check(
                    s(player, "screen_name").encode_utf16().count() <= 31,
                    "Result name does not fit original storage",
                )?;
                let mut identity = [0u8; 8];
                getrandom::getrandom(&mut identity)
                    .map_err(|_| Error(503, "Result identity entropy unavailable"))?;
                identities.insert(s(player, "id").to_owned(), json!(bytes_hex(&identity)));
                players.push(json!({"id":player["id"],"name":player["screen_name"],"team":team,"guest":player["guest"],"owner":owner,
                "search_raw_rank":player["raw_rank"]}));
            }
        }
        m::check(
            (2..=16).contains(&players.len()) && !owners.is_empty(),
            "Invalid admitted result roster",
        )?;
        let conf = configuration(&r["skill_configuration"])?;
        let clan_conf = configuration(&r["clan_configuration"])?;
        if let Some(catalog) = &core.catalog {
            let entry = catalog.entry(s(proposal, "playlist"))?;
            let admitted = entry["rank_configuration"].as_str();
            reject(
                if policy["clan_match"].is_null() {
                    conf == admitted && clan_conf.is_none()
                } else {
                    conf.is_none() && clan_conf == admitted
                },
                409,
                "Result skill policy differs from admitted catalog",
            )?;
        }

        m::check(
            conf.is_none() || clan_conf.is_none(),
            "Player and clan XP namespaces must remain separate",
        )?;
        let clans = r["clans"]
            .as_object()
            .ok_or(Error(400, "Invalid result clan namespace"))?;
        let mut frozen_clans = serde_json::Map::new();
        if !policy["clan_match"].is_null() {
            m::check(
                clan_conf.is_some() && clans.len() == 2,
                "Missing admitted clan skill policy",
            )?;
            for (team, members) in array(proposal, "teams").iter().enumerate() {
                let mut ids = BTreeSet::new();
                for member in members.as_array().unwrap() {
                    for t in &group {
                        if array(t, "players").iter().any(|p| p["id"] == *member) {
                            ids.insert(m::text(&t["clan"], 64)?);
                        }
                    }
                }
                m::check(ids.len() == 1, "Clan team differs from admission")?;
                let clan = *ids.first().unwrap();
                reject(
                    clans.get(&team.to_string()).is_some_and(|v| v == clan),
                    409,
                    "Clan result owner changed",
                )?;
                frozen_clans.insert(team.to_string(), json!(clan));
            }
            m::check(
                frozen_clans
                    .values()
                    .map(|v| v.as_str().unwrap())
                    .collect::<BTreeSet<_>>()
                    .len()
                    == 2,
                "Clan result owners are not distinct",
            )?;
        } else {
            m::check(
                clans.is_empty() && clan_conf.is_none(),
                "Unexpected clan result namespace",
            )?;
        }
        let skills = r["skills"]
            .as_object()
            .ok_or(Error(400, "Invalid admitted player skills"))?;
        let mut frozen_skills = serde_json::Map::new();
        if let Some(configuration) = conf {
            for p in &players {
                if !flag(p, "guest") {
                    frozen_skills.insert(
                        s(p, "id").to_owned(),
                        skill_snapshot(
                            skills
                                .get(s(p, "id"))
                                .ok_or(Error(409, "Player skill snapshot missing"))?,
                            configuration,
                        )?,
                    );
                }
            }
        }
        m::check(
            skills.len() == frozen_skills.len(),
            "Unexpected player skill snapshots",
        )?;
        let clan_skills = r["clan_skills"]
            .as_object()
            .ok_or(Error(400, "Invalid admitted clan skills"))?;
        let mut frozen_clan_skills = serde_json::Map::new();
        if let Some(configuration) = clan_conf {
            for name in frozen_clans.values() {
                let name = name.as_str().unwrap();
                frozen_clan_skills.insert(
                    name.to_owned(),
                    skill_snapshot(
                        clan_skills
                            .get(name)
                            .ok_or(Error(409, "Clan skill snapshot missing"))?,
                        configuration,
                    )?,
                );
            }
        }
        m::check(
            clan_skills.len() == frozen_clan_skills.len(),
            "Unexpected clan skill snapshots",
        )?;
        let compatibility = s(&group[0], "compatibility");
        let context = json!({"namespace":"native-matchmaking-v1","source_binding":binding,"proposal":proposal,"ranked":flag(policy,"ranked"),"team_mode":team_mode,
            "players":players,"owners":owners,"identities":identities,"compatibility":compatibility,"accuracy":compatibility=="halo2-native-queue-v16",
            "skill_configuration":conf,"clan_configuration":clan_conf,"skills":frozen_skills,"clans":frozen_clans,"clan_skills":frozen_clan_skills,
            "created_wall":m::integer(&r["created_wall"],0,9007199254740991)?,
            "comparison_skill_policy":"native-persistent-raw-v1; original effective-to-result provenance unclosed"});
        validate_game(&context)?;
        Ok(Self {
            context,
            digest: String::new(),
            report: Vec::new(),
            confirmations: BTreeSet::new(),
            active: owners.clone(),
            owners,
            quarantined: None,
            committed: false,
        })
    }
    pub fn validate(&self, report: &str) -> Result<(Vec<u8>, String)> {
        let raw = m::hex(&json!(report), crate::security_policy::PCR_SIZE)?;
        validate_report(&self.context, &raw)?;
        let digest = format!("{:x}", Sha256::digest(&raw));
        Ok((raw, digest))
    }
    fn snapshot(&self) -> Value {
        json!({"digest":self.digest,"confirmations":self.confirmations,"admitted_owners":self.owners,"active":self.active,
        "identities":self.context["identities"],"accuracy":self.context["accuracy"],"admission_sha256":fingerprint(&self.context),
        "quarantined":self.quarantined,"committed":self.committed})
    }
    pub fn commit_context(&self) -> Result<Value> {
        reject(
            !self.report.is_empty() && self.confirmations == self.owners,
            400,
            "All admitted physical connections must confirm the result",
        )?;
        reject(
            self.quarantined.is_none(),
            409,
            "Conflicting result is quarantined",
        )?;
        Ok(
            json!({"admission":self.context,"admission_sha256":fingerprint(&self.context),"report":bytes_hex(&self.report),"digest":self.digest,"confirmations":self.confirmations}),
        )
    }
}
fn validate_game(context: &Value) -> Result<(u32, Option<bool>)> {
    let game = &context["proposal"]["game"];
    if !game["settings"].is_null() {
        let raw = m::hex(&game["settings"], 304)?;
        let engine = u32::from_le_bytes(raw[0x44..0x48].try_into().unwrap());
        m::check(
            [1, 2, 3, 4, 7, 8, 9].contains(&engine),
            "Unknown selected game engine",
        )?;
        return Ok((engine, Some(raw[0x48] & 1 != 0)));
    }
    let Some((engine, team)) = legacy_variant(m::text(&game["variant"], 64)?)? else {
        return Err(Error(
            409,
            "Unfamiliar variants require complete admitted settings",
        ));
    };
    // Explicit legacy built-in compatibility. Unknown names never gain Slayer.
    // Authored legacy Slayer team mode comes from admission; buffered profiles
    // instead bind the complete selected option array below.
    Ok((engine, if engine == 2 { None } else { Some(team) }))
}
pub(super) fn validate_report(context: &Value, raw: &[u8]) -> Result<()> {
    m::check(
        raw.len() == crate::security_policy::PCR_SIZE,
        "Invalid postgame report size",
    )?;
    let (engine, expected_team) = validate_game(context)?;
    let team_mode = flag(context, "team_mode");
    m::check(
        raw[..2] == [1, 1]
            && raw[0x10..0x12] == [1, 1]
            && raw[0x13] == u8::from(team_mode)
            && u32::from_le_bytes(raw[0x64..0x68].try_into().unwrap()) == engine
            && expected_team.is_none_or(|team| team == team_mode)
            && raw[0x68] & 1 == u8::from(team_mode),
        "Unfinished admitted multiplayer postgame report",
    )?;
    let mut team_changing=false;
    if !context["proposal"]["game"]["settings"].is_null() {
        let selected = m::hex(&context["proposal"]["game"]["settings"], 304)?;
        team_changing=selected[0x48]&0x40!=0;
        m::check(
            raw[0x20..0x150] == selected,
            "Postgame options differ from the complete admitted variant",
        )?;
    }
    let players = array(context, "players");
    m::check(
        (2..=16).contains(&players.len()),
        "Invalid result roster count",
    )?;
    let mut teams = if team_mode {
        players
            .iter()
            .map(|p| n(p, "team") as u8)
            .collect::<BTreeSet<_>>()
    } else {
        BTreeSet::new()
    };
    m::check(teams.iter().all(|t| *t < 8), "Invalid admitted teams")?;
    let mut required_teams=teams.clone();
    for (slot, p) in players.iter().enumerate() {
        let at = 0x370 + slot * 0x94;
        let identity = m::hex(&context["identities"][s(p, "id")], 8)?;
        m::check(
            raw[at] == 1 && raw[at + 2..at + 10] == identity,
            "Postgame roster differs from admission",
        )?;
        let mut name = Vec::new();
        for word in s(p, "name").encode_utf16() {
            name.extend(word.to_le_bytes());
        }
        m::check(name.len() <= 62, "Postgame name too long")?;
        name.resize(64, 0);
        m::check(
            raw[at + 12..at + 76] == name,
            "Postgame name differs from admission",
        )?;
        m::check(
            (raw[at + 0x90] as usize) < players.len(),
            "Postgame placement outside roster",
        )?;
        let current=raw[at+0x88];
        m::check(if team_mode {current<8 && (team_changing || current==n(p,"team") as u8)} else {current==255},
            "Postgame team differs from the admitted rule")?;
        if team_mode {required_teams.insert(current);}

    }
    m::check(
        (players.len()..16).all(|i| raw[0x370 + i * 0x94] == 0),
        "Postgame contains unadmitted players",
    )?;
    if team_mode && team_changing {
        teams=(0u8..8).filter(|team| raw[0xcb0+usize::from(*team)*0x18]==1).collect();
        m::check(required_teams.is_subset(&teams),"Postgame omitted an admitted or final team")?;
    }
    for team in 0..16 {
        let at = 0xcb0 + team * 0x18;
        m::check(
            raw[at] == u8::from(teams.contains(&(team as u8))),
            "Postgame team rows differ from admission",
        )?;
        if teams.contains(&(team as u8)) {
            m::check(
                (raw[at + 1] as usize) < teams.len(),
                "Team placement outside admission",
            )?;
        }
    }
    Ok(())
}

#[derive(Default)]
pub(super) struct Registry {
    values: HashMap<String, Record>,
}
impl Registry {
    pub(super) fn identities(&self, key: &str) -> Result<&Value> {
        Ok(&self
            .values
            .get(key)
            .ok_or(Error(404, "Result admission retired"))?
            .context["identities"])
    }
    pub(super) fn report_hex(&self, key: &str) -> Result<String> {
        Ok(bytes_hex(
            &self
                .values
                .get(key)
                .ok_or(Error(404, "Result admission retired"))?
                .report,
        ))
    }
    pub(super) fn session_identity(&self, key: &str) -> Result<Value> {
        let p = &self
            .values
            .get(key)
            .ok_or(Error(404, "Result admission retired"))?
            .context["proposal"];
        Ok(
            json!({"playlist":p["playlist"],"revision":p["revision"],"map":p["game"]["map"],"variant":p["game"]["variant"]}),
        )
    }
    pub(super) fn is_active(&self, key: &str, actor: &str) -> bool {
        self.values
            .get(key)
            .is_some_and(|record| record.active.contains(actor))
    }
    pub(super) fn is_committed(&self, key: &str) -> bool {
        self.values.get(key).is_some_and(|record| record.committed)
    }
    pub(super) fn has_report(&self, key: &str) -> bool {
        self.values
            .get(key)
            .is_some_and(|record| !record.report.is_empty())
    }
    pub(super) fn commit_context(&self, key: &str, epoch: u32) -> Result<Value> {
        let record = self
            .values
            .get(key)
            .ok_or(Error(404, "Result admission retired"))?;
        let mut value = record.commit_context()?;
        let departed =
            array(&record.context, "players")
                .iter()
                .enumerate()
                .fold(0u32, |mask, (slot, p)| {
                    mask | if !record.active.contains(s(p, "owner")) {
                        1 << slot
                    } else {
                        0
                    }
                });
        let compatibility = s(&record.context, "compatibility");
        value["departed_slots"] = json!(departed);
        value["authority_epoch"] = json!(epoch);
        value["consensus_required"] = json!(
            compatibility == "halo2-native-queue-v11"
                || ([
                    "halo2-native-queue-v12",
                    "halo2-native-queue-v13",
                    "halo2-native-queue-v14",
                    "halo2-native-queue-v15",
                    "halo2-native-queue-v16"
                ]
                .contains(&compatibility)
                    && departed != 0)
        );
        Ok(value)
    }
    pub(super) fn execute(&mut self, core: &Coordinator, r: &Value) -> Result<Value> {
        let op = m::text(&r["op"], 64)?;
        let key = m::text(&r["id"], 64)?;
        if op == "result_open" {
            m::check(
                self.values.len() < 1000,
                "Result admission capacity reached",
            )?;
            reject(
                !self.values.contains_key(key),
                409,
                "Result admission already exists",
            )?;
            let value = Record::new(core, &r["context"])?;
            m::check(
                value.context["proposal"]["id"] == key,
                "Result admission identity differs",
            )?;
            let snapshot = value.snapshot();
            self.values.insert(key.to_owned(), value);
            return Ok(snapshot);
        }
        if op == "result_drop" {
            self.values.remove(key);
            return Ok(Value::Null);
        }
        let value = self
            .values
            .get_mut(key)
            .ok_or(Error(404, "Result admission retired"))?;
        match op {
            "result_snapshot" => Ok(value.snapshot()),
            "result_report" => Ok(json!(bytes_hex(&value.report))),
            "result_validate" => {
                let (_, digest) =
                    value.validate(m::text(&r["report"], 2 * crate::security_policy::PCR_SIZE)?)?;
                Ok(json!(digest))
            }
            "result_submit" => {
                let actor = m::text(&r["actor"], 64)?;
                reject(
                    value.active.contains(actor),
                    403,
                    "Result publisher is not admitted",
                )?;
                let report = m::text(&r["report"], 2 * crate::security_policy::PCR_SIZE)?;
                if !value.report.is_empty() {
                    if report != bytes_hex(&value.report) {
                        if !value.committed {
                            value.quarantined = Some("conflicting-report");
                        }
                        return Err(Error(
                            409,
                            "Postgame retry differs from the original report",
                        ));
                    }
                    return Ok(json!(value.digest));
                }
                let (raw, digest) = value.validate(report)?;
                value.report = raw;
                value.digest = digest;
                Ok(json!(value.digest))
            }
            "result_confirm" => {
                let actor = m::text(&r["actor"], 64)?;
                reject(
                    value.active.contains(actor),
                    403,
                    "Result voter is not admitted",
                )?;
                let digest = m::text(&r["digest"], 64)?;
                if value.report.is_empty() || digest != value.digest {
                    if !value.committed && !value.report.is_empty() {
                        value.quarantined = Some("conflicting-confirmation");
                    }
                    return Err(Error(
                        409,
                        "Match results disagree; no competitive result was recorded",
                    ));
                }
                reject(
                    value.quarantined.is_none(),
                    409,
                    "Conflicting result is quarantined",
                )?;
                value.confirmations.insert(actor.to_owned());
                Ok(value.snapshot())
            }
            "result_revoke" => {
                value.active.remove(m::text(&r["actor"], 64)?);
                Ok(Value::Null)
            }
            "result_review" => Ok(
                json!({"present":!value.report.is_empty()&&!value.active.is_empty()&&value.active.is_subset(&value.confirmations),
                "recordable":!value.report.is_empty()&&value.confirmations==value.owners&&value.quarantined.is_none()}),
            ),
            "result_committed" => {
                reject(
                    m::text(&r["digest"], 64)? == value.digest,
                    409,
                    "Committed result digest differs",
                )?;
                value.committed = true;
                Ok(Value::Null)
            }
            "result_commit_context" => value.commit_context(),
            #[cfg(feature = "matchmaking-test-inputs")]
            "result_fixture_accuracy" => {
                m::check(
                    value.report.is_empty(),
                    "Cannot change admitted fixture after reporting",
                )?;
                value.context["accuracy"] = json!(true);
                Ok(value.snapshot())
            }
            _ => Err(Error(400, "Unknown result operation")),
        }
    }
}
pub(super) fn value(r: &Value) -> Result<Value> {
    match m::text(&r["op"], 64)? {
        "result_presets" => Ok(Value::Object(
            presets()?["variants"]
                .as_array()
                .unwrap()
                .iter()
                .map(|p| (s(p, "name").to_owned(), json!([p["engine"], p["team"]])))
                .collect(),
        )),
        #[cfg(feature = "matchmaking-test-inputs")]
        "result_validate_fixture" => {
            let c = &r["context"];
            m::fields(c, &["proposal", "players", "team_mode", "identities"])?;
            m::boolean(&c["team_mode"])?;
            m::game(&c["proposal"]["game"], flag(c, "team_mode"))?;
            let players = c["players"]
                .as_array()
                .ok_or(Error(400, "Invalid fixture roster"))?;
            m::check(
                (2..=16).contains(&players.len()),
                "Invalid fixture roster size",
            )?;
            for p in players {
                m::fields(p, &["id", "name", "team"])?;
                m::text(&p["id"], 64)?;
                m::text(&p["name"], 64)?;
                m::integer(&p["team"], 0, 15)?;
                m::hex(&c["identities"][s(p, "id")], 8)?;
            }
            let raw = m::hex(&r["report"], crate::security_policy::PCR_SIZE)?;
            validate_report(c, &raw)?;
            Ok(json!(format!("{:x}", Sha256::digest(raw))))
        }
        _ => Err(Error(400, "Unknown result value operation")),
    }
}
