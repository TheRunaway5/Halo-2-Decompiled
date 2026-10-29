//! Native accepted-party search consent and private queue permits.
//!
//! Accepted membership and room/clan observations arrive from trusted directory
//! IO. Invitation/social policy remains a separate migration boundary. This is
//! declared native service authorization, not recovered Xbox Live arbitration.
use super::{Coordinator, fingerprint, id, m, reject};
use crate::matchmaking_catalog;
use crate::matchmaking_model::{Error, Result, array, flag, n, s};
use serde_json::{Value, json};
use std::collections::{HashMap, HashSet};

#[path = "matchmaking_party_countdowns.rs"]
mod countdowns;

fn stmt(sql: &str, params: Value, changes: Option<i64>) -> Value {
    json!({"sql":sql,"params":params,"changes":changes})
}
fn party(facts: &Value) -> Result<Option<&Value>> {
    m::fields(facts, &["party", "clans"])?;
    let clans = facts["clans"]
        .as_array()
        .ok_or(Error(400, "Clan membership facts required"))?;
    m::check(clans.len() <= 64, "Too many clan membership facts")?;
    let mut seen = HashSet::new();
    for clan in clans {
        m::fields(clan, &["player", "clan"])?;
        m::check(
            seen.insert(m::text(&clan["player"], 64)?),
            "Duplicate clan membership fact",
        )?;
        if !clan["clan"].is_null() {
            m::text(&clan["clan"], 64)?;
        }
    }
    let p = &facts["party"];
    if p.is_null() {
        return Ok(None);
    }
    m::fields(p, &["id", "leader", "version", "members"])?;
    m::text(&p["id"], 64)?;
    let leader = m::text(&p["leader"], 64)?;
    m::integer(&p["version"], 1, i64::MAX)?;
    let members = p["members"]
        .as_array()
        .ok_or(Error(400, "Accepted party roster required"))?;
    m::check(
        (1..=16).contains(&members.len()),
        "Invalid accepted party capacity",
    )?;
    let mut players = HashSet::new();
    let mut handles = HashSet::new();
    for row in members {
        m::fields(
            row,
            &[
                "player",
                "handle",
                "ready_playlist",
                "ready_revision",
                "ready_version",
                "consent",
            ],
        )?;
        m::check(
            players.insert(m::text(&row["player"], 64)?),
            "Duplicate accepted party member",
        )?;
        m::check(
            handles.insert(m::text(&row["handle"], 64)?),
            "Duplicate party member handle",
        )?;
        for field in ["ready_playlist", "ready_revision"] {
            if row[field] != "" {
                m::text(&row[field], 64)?;
            }
        }
        m::integer(&row["ready_version"], 0, i64::MAX)?;
        if !row["consent"].is_null() {
            let c = &row["consent"];
            m::fields(
                c,
                &[
                    "party",
                    "version",
                    "handle",
                    "playlist",
                    "revision",
                    "profile",
                    "session_key",
                    "generation",
                    "roster",
                ],
            )?;
            for field in ["party", "handle", "playlist", "revision"] {
                m::text(&c[field], 64)?;
            }
            m::hex(&c["profile"], 32)?;
            m::hex(&c["session_key"], 32)?;
            m::hex(&c["roster"], 32)?;
            m::integer(&c["version"], 1, i64::MAX)?;
            m::integer(&c["generation"], 1, i64::MAX)?;
        }
    }
    m::check(
        players.contains(leader),
        "Accepted party leader is not a member",
    )?;
    Ok(Some(p))
}
fn rooms(value: &Value) -> Result<&[Value]> {
    let rows = value
        .as_array()
        .ok_or(Error(400, "Room observations required"))?;
    m::check(rows.len() <= 64, "Too many room observations")?;
    for room in rows {
        m::fields(
            room,
            &["closed", "phase", "owners", "active", "playlist", "clans"],
        )?;
        m::boolean(&room["closed"])?;
        m::text(&room["phase"], 32)?;
        m::text(&room["playlist"], 64)?;
        let owners = room["owners"]
            .as_object()
            .ok_or(Error(400, "Room owners required"))?;
        m::check(owners.len() <= 16, "Too many room seats")?;
        for (player, owner) in owners {
            m::text(&json!(player), 64)?;
            m::text(owner, 64)?;
        }
        let active = room["active"]
            .as_array()
            .ok_or(Error(400, "Room capabilities required"))?;
        m::check(active.len() <= 16, "Too many room capabilities")?;
        for owner in active {
            m::text(owner, 64)?;
        }
        let clans = room["clans"]
            .as_array()
            .ok_or(Error(400, "Room clans required"))?;
        m::check(clans.len() <= 16, "Too many room clans")?;
        for clan in clans {
            m::text(clan, 64)?;
        }
    }
    Ok(rows)
}
fn active(room: &Value, who: &str) -> bool {
    !flag(room, "closed")
        && room["owners"]
            .get(who)
            .is_some_and(|owner| array(room, "active").contains(owner))
}
fn playing(rows: &[Value], who: &str) -> bool {
    rows.iter()
        .any(|r| active(r, who) && r["phase"] != "postgame")
}
fn roster_pin(core: &Coordinator, who: &str, pid: &str, facts: &Value) -> Result<String> {
    let connected = connections(core, who, facts)?;
    let p = party(facts)?;
    let policy = core.policy(pid)?;
    Ok(fingerprint(
        &json!({"party":p.map(|p|p["id"].clone()),"version":p.map(|p|p["version"].clone()),"leader":p.map(|p|p["leader"].clone()),
        "members":connected.iter().enumerate().map(|(index,connection)|json!({"handle":p.map(|p|p["members"][index]["handle"].clone()),
            "connection":connection.as_ref().map(|c|json!({"owner":c["player"],"generation":c["generation"],"session_key":c["key"],"compatibility":c["compatibility"],"maps":c["maps"],"language":c["language"],
                "seats":array(c,"local_players").iter().map(|seat|json!({"id":seat["player"],"name":seat["screen_name"],"guest":seat["guest"],"clan":if policy["clan_match"].is_null(){Value::Null}else{clan(facts,&seat["player"])}})).collect::<Vec<_>>()}))})).collect::<Vec<_>>()}),
    ))
}
fn consent_matches(
    core: &Coordinator,
    p: &Value,
    member: &Value,
    connection: Option<&Value>,
    pid: &str,
    facts: &Value,
) -> bool {
    let Some(policy) = core.playlists.iter().find(|policy| policy["id"] == pid) else {
        return false;
    };
    let consent = &member["consent"];
    member["ready_playlist"] == pid
        && member["ready_revision"] == policy["revision"]
        && member["ready_version"] == p["version"]
        && connection.is_some_and(|c| {
            !consent.is_null()
                && consent["party"] == p["id"]
                && consent["version"] == p["version"]
                && consent["handle"] == member["handle"]
                && consent["playlist"] == pid
                && consent["revision"] == policy["revision"]
                && consent["profile"] == core.policy_pins[pid]
                // Starting the search selects the leader's current roster.
                // Remote members must have consented to that whole roster.
                && (member["player"]==p["leader"] || roster_pin(core, s(member, "player"), pid, facts)
                    .is_ok_and(|pin| consent["roster"] == pin))
                && consent["session_key"] == c["key"]
                && consent["generation"] == c["generation"]
        })
}
fn initial_room_gate(rows: &[Value], who: &str) -> Result<()> {
    reject(
        !rows
            .iter()
            .any(|r| active(r, who) && r["phase"] != "connecting" && r["phase"] != "postgame"),
        409,
        "Leave the current ONLINE match before searching again",
    )
}
fn clan(facts: &Value, who: &Value) -> Value {
    array(facts, "clans")
        .iter()
        .find(|c| c["player"] == *who)
        .map(|c| c["clan"].clone())
        .unwrap_or(Value::Null)
}
fn connections(core: &Coordinator, who: &str, facts: &Value) -> Result<Vec<Option<Value>>> {
    let p = party(facts)?;
    if let Some(p) = p {
        reject(
            array(p, "members").iter().any(|m| m["player"] == who),
            403,
            "Account is not in the accepted party",
        )?;
        Ok(array(p, "members")
            .iter()
            .map(|m| core.sessions.primary(s(m, "player"), core.clock))
            .collect())
    } else {
        Ok(vec![core.sessions.primary(who, core.clock)])
    }
}
fn eligible(core: &Coordinator, who: &str, playlist: &str, facts: &Value) -> Result<bool> {
    let connections = connections(core, who, facts)?;
    let Some(actor) = core.sessions.primary(who, core.clock) else {
        return Ok(false);
    };
    let rows=connections.iter().map(|connection|connection.as_ref().map(|c|json!({"id":c["player"],"compatibility":c["compatibility"],"maps":c["maps"],
        "seats":array(c,"local_players").iter().map(|p|json!({"id":p["player"],"guest":p["guest"],"clan":clan(facts,&p["player"])})).collect::<Vec<_>>()})).unwrap_or(Value::Null)).collect::<Vec<_>>();
    let catalog = core
        .catalog
        .as_ref()
        .ok_or(Error(503, "Party matching requires the admitted catalog"))?;
    matchmaking_catalog::eligible(
        catalog.entry(playlist)?,
        s(&actor, "compatibility"),
        &json!(rows),
    )
}
fn select(
    core: &Coordinator,
    who: &str,
    pid: &str,
    facts: &Value,
    observations: &Value,
) -> Result<Value> {
    let p = party(facts)?;
    let policy = core.policy(pid)?;
    let connected = connections(core, who, facts)?;
    let rows = rooms(observations)?;
    if let Some(p) = p {
        reject(
            p["leader"] == who,
            403,
            "Only the party leader can start a search",
        )?;
        reject(
            array(p, "members").len() <= n(policy, "maximum_party") as usize,
            409,
            "Party is too large for this playlist",
        )?;
        for (member, connection) in array(p, "members").iter().zip(&connected) {
            if member["player"] == who {
                continue;
            }
            reject(
                consent_matches(core, p, member, connection.as_ref(), pid, facts),
                409,
                "Every party member must agree to this playlist and current session/profile",
            )?;
        }
        reject(
            connected.iter().all(Option::is_some),
            409,
            "Every party member must open matchmaking",
        )?;
        let versions = connected
            .iter()
            .map(|c| s(c.as_ref().unwrap(), "compatibility"))
            .collect::<HashSet<_>>();
        reject(
            versions.len() == 1,
            409,
            "Party members need compatible game builds",
        )?;
        reject(
            !array(p, "members")
                .iter()
                .any(|m| playing(rows, s(m, "player"))),
            409,
            "A party member is still in a match",
        )?;
    } else {
        reject(
            connected[0].is_some(),
            401,
            "Matchmaking session expired; reconnect",
        )?;
    }
    let members = connected
        .into_iter()
        .map(Option::unwrap)
        .collect::<Vec<_>>();
    let actor = members
        .iter()
        .find(|c| c["player"] == who)
        .ok_or(Error(403, "Search owner is not connected"))?;
    let generations = members
        .iter()
        .map(|c| c["generation"].clone())
        .collect::<Vec<_>>();
    let members = core.sessions.group_snapshots(
        core,
        actor["generation"].as_u64().unwrap(),
        pid,
        &json!(generations),
    )?;
    let selected_clan = if !policy["clan_match"].is_null() {
        let seats = members
            .iter()
            .flat_map(|c| array(c, "local_players").iter())
            .map(|p| json!({"clan":clan(facts,&p["player"])}))
            .collect::<Vec<_>>();
        let c = matchmaking_catalog::clan_group(&policy["clan_match"], &seats)?;
        reject(
            !rows.iter().any(|r| {
                !flag(r, "closed")
                    && r["phase"] != "postgame"
                    && r["playlist"] == pid
                    && array(r, "clans").contains(&c)
            }),
            409,
            "This clan is already playing this playlist",
        )?;
        c
    } else {
        Value::Null
    };
    let party_id = p.map(|p| p["id"].clone()).unwrap_or(json!(who));
    let binding = json!({"party":party_id,"version":p.map(|p|p["version"].clone()),"leader":who,"playlist":pid,"profile":core.policy_pins[pid],
        "members":members.iter().map(|c|json!({"owner":c["player"],"session_key":c["key"],"generation":c["generation"],
            "handle":p.and_then(|p|array(p,"members").iter().find(|m|m["player"]==c["player"])).map(|m|m["handle"].clone()),
            "seats":array(c,"local_players").iter().map(|seat|json!({"player":seat["player"],"guest":seat["guest"]})).collect::<Vec<_>>()})).collect::<Vec<_>>()});
    Ok(json!({"party_id":party_id,"members":members,"clan":selected_clan,"binding":binding}))
}
fn change_plan(pid: &str, facts: &Value, admitted: bool) -> Result<Value> {
    let p = party(facts)?;
    if let Some(p) = p {
        m::check(p["id"] == pid, "Changed party differs from accepted facts")?;
        reject(n(p, "version") < i64::MAX, 503, "Party revision exhausted")?;
    }
    let mut statements = vec![
        stmt(
            "UPDATE parties SET version=version+1 WHERE id=? AND version=?",
            json!([pid, p.map(|p| p["version"].clone()).unwrap_or(json!(0))]),
            Some(i64::from(p.is_some())),
        ),
        stmt(
            "UPDATE party_members SET ready_playlist='',ready_revision='',ready_version=0 WHERE party=?",
            json!([pid]),
            None,
        ),
        stmt(
            "DELETE FROM matchmaking_party_consents WHERE party=?",
            json!([pid]),
            None,
        ),
    ];
    if admitted && p.is_some_and(|p| array(p, "members").len() < 16) {
        statements.push(stmt("UPDATE party_activity_links SET version=version+1 WHERE party=? AND version=(SELECT version-1 FROM parties WHERE id=?)",json!([pid,pid]),None));
        statements.push(stmt("DELETE FROM party_activity_links WHERE party=? AND version!=(SELECT version FROM parties WHERE id=?)",json!([pid,pid]),None));
    } else {
        statements.push(stmt(
            "DELETE FROM party_activity_links WHERE party=?",
            json!([pid]),
            None,
        ));
    }
    Ok(json!({"statements":statements,"effects":{"parties":[pid]}}))
}
fn ready(core: &Coordinator, r: &Value) -> Result<Value> {
    let who = m::text(&r["player"], 64)?;
    let pid = m::text(&r["playlist"], 64)?;
    let revision = m::text(&r["revision"], 64)?;
    let p = party(&r["facts"])?.ok_or(Error(409, "Create or join a party first"))?;
    reject(
        p["version"] == r["version"],
        409,
        "Party changed; refresh it",
    )?;
    let policy = core
        .policy(pid)
        .map_err(|_| Error(409, "Playlist changed; refresh it"))?;
    reject(
        policy["revision"] == revision,
        409,
        "Playlist changed; refresh it",
    )?;
    let observed_rooms = rooms(&r["rooms"])?;
    reject(
        !array(p, "members")
            .iter()
            .any(|m| playing(observed_rooms, s(m, "player"))),
        409,
        "Leave the current match first",
    )?;
    reject(
        eligible(core, who, pid, &r["facts"])?,
        409,
        "Open matchmaking with the required maps first",
    )?;
    reject(
        array(p, "members").len() <= n(policy, "maximum_party") as usize,
        409,
        "Party is too large for this playlist",
    )?;
    let actor = core
        .sessions
        .primary(who, core.clock)
        .ok_or(Error(409, "Open matchmaking with the required maps first"))?;
    let member = array(p, "members")
        .iter()
        .find(|m| m["player"] == who)
        .ok_or(Error(403, "Account is not a party member"))?;
    let changed = core.tickets.iter().any(|t| {
        array(t, "players").iter().any(|p| p["id"] == who)
            && (t["playlist"] != pid || !core.current_profile(policy, t))
    });
    let mut plan = if changed {
        change_plan(s(p, "id"), &r["facts"], false)?
    } else {
        json!({"statements":[stmt("UPDATE parties SET version=version WHERE id=? AND leader=? AND version=?",json!([p["id"],p["leader"],p["version"]]),Some(1))],"effects":{"parties":[]}})
    };
    let version = n(p, "version") + i64::from(changed);
    let handle = &member["handle"];
    let mut consent_facts = r["facts"].clone();
    consent_facts["party"]["version"] = json!(version);
    let roster = roster_pin(core, who, pid, &consent_facts)?;
    plan["consent"] = json!({"party":p["id"],"version":version,"handle":handle,"playlist":pid,"revision":revision,"profile":core.policy_pins[pid],"session_key":actor["key"],"generation":actor["generation"],"roster":roster});
    plan["statements"].as_array_mut().unwrap().extend([
        stmt("UPDATE party_members SET ready_playlist=?,ready_revision=?,ready_version=? WHERE player=? AND party=? AND handle=?",json!([pid,revision,version,who,p["id"],handle]),Some(1)),
        stmt("INSERT INTO matchmaking_party_consents(player,party,version,handle,playlist,revision,profile,session_key,generation,roster) VALUES(?,?,?,?,?,?,?,?,?,?) ON CONFLICT(player) DO UPDATE SET party=excluded.party,version=excluded.version,handle=excluded.handle,playlist=excluded.playlist,revision=excluded.revision,profile=excluded.profile,session_key=excluded.session_key,generation=excluded.generation,roster=excluded.roster",
            json!([who,p["id"],version,handle,pid,revision,core.policy_pins[pid],actor["key"],m::integer(&actor["generation"],1,i64::MAX)?,roster]),Some(1))]);
    Ok(plan)
}
#[derive(Default)]
pub(super) struct Registry {
    permits: HashMap<String, Value>,
    countdowns: countdowns::Registry,
}
impl Registry {
    pub(super) fn abort_permit(&mut self, token: &str) {
        self.permits.remove(token);
    }
    pub(super) fn retire_generation(&mut self, generation: u64) {
        self.countdowns.retire_generation(generation);
        self.permits
            .retain(|_, p| !array(p, "generations").contains(&json!(generation)));
    }
    pub(super) fn cleanup(&mut self, now: f64, pins: &HashMap<String, String>) {
        self.countdowns.cleanup(pins);
        self.permits.retain(|_, p| {
            p["expires"].as_f64().unwrap() > now
                && pins
                    .get(s(&p["body"], "playlist_id"))
                    .is_some_and(|pin| p["profile"] == *pin)
        });
    }
    pub(super) fn permit(&self, core: &Coordinator, r: &Value) -> Result<Value> {
        let token = m::text(&r["permit"], 64)?;
        let p = self
            .permits
            .get(token)
            .ok_or(Error(409, "Party queue permit retired"))?;
        reject(
            p["expires"].as_f64().unwrap() > core.clock
                && p["generation"] == r["generation"]
                && p["body"] == r["body"]
                && p["facts"] == fingerprint(&r["facts"])
                && p["profile"] == core.policy_pins[s(&r["body"], "playlist_id")]
                && p["generations"] == r["members"]
                && p["selected"]["party_id"] == r["queue"]["party_id"]
                && p["selected"]["clan"] == r["queue"]["clan"],
            409,
            "Party consent or queue permit changed",
        )?;
        Ok(p["selected"]["binding"].clone())
    }
    pub(super) fn consume(&mut self, r: &Value) {
        self.permits.remove(r["permit"].as_str().unwrap());
    }
    fn effects(&mut self, core: &mut Coordinator, r: &Value) -> Result<Value> {
        m::fields(&r["effects"], &["parties"])?;
        let ids = r["effects"]["parties"]
            .as_array()
            .ok_or(Error(400, "Changed party list required"))?;
        m::check(ids.len() <= 16, "Too many changed parties")?;
        for pid in ids {
            m::text(pid, 64)?;
        }
        self.countdowns.invalidate(ids);
        self.permits
            .retain(|_, p| !ids.contains(&p["selected"]["party_id"]));
        let tickets = core
            .tickets
            .iter()
            .filter(|t| ids.contains(&t["party_id"]))
            .map(|t| s(t, "id").to_owned())
            .collect::<Vec<_>>();
        for id in tickets {
            if core.ticket(&id).is_ok() {
                core.cancel_ticket(&id)?;
            }
        }
        let mut gatherings = std::mem::take(&mut core.gatherings);
        gatherings.cleanup(core);
        core.gatherings = gatherings;
        Ok(json!(ids))
    }
    pub(super) fn execute(&mut self, core: &mut Coordinator, r: &Value) -> Result<Value> {
        if r["op"]
            .as_str()
            .is_some_and(|op| op.starts_with("party_countdown_"))
        {
            return self.countdowns.execute(core, r);
        }
        self.permits
            .retain(|_, p| p["expires"].as_f64().unwrap() > core.clock);
        match m::text(&r["op"], 64)? {
            "party_directory_context" => {
                let players = r["players"]
                    .as_array()
                    .ok_or(Error(400, "Party context subjects required"))?;
                m::check(players.len() <= 16, "Too many party context subjects")?;
                let observed = rooms(&r["rooms"])?;
                let mut records = Vec::new();
                for player in players {
                    let who = m::text(player, 64)?;
                    let searching = core.tickets.iter().any(|t| {
                        array(t, "players").iter().any(|p| p["id"] == who)
                            && (t["expires_at"].as_f64().is_some_and(|v| v > core.clock)
                                || core.proposals.iter().any(|p| {
                                    p["expires_at"].as_f64().is_some_and(|v| v > core.clock)
                                        && array(p, "tickets").contains(&t["id"])
                                }))
                    });
                    records.push(json!({"player":who,"session":core.sessions.primary(who,core.clock),"playing":playing(observed,who),"searching":searching}));
                }
                Ok(json!(records))
            }
            "party_directory_cancel_queue" => {
                let who = m::text(&r["player"], 64)?;
                let ids = core
                    .tickets
                    .iter()
                    .filter(|t| array(t, "players").iter().any(|p| p["id"] == who))
                    .map(|t| s(t, "id").to_owned())
                    .collect::<Vec<_>>();
                for id in ids {
                    core.cancel_ticket(&id)?;
                }
                let mut gatherings = std::mem::take(&mut core.gatherings);
                gatherings.cleanup(core);
                core.gatherings = gatherings;
                Ok(Value::Null)
            }
            "party_ready" => ready(core, r),
            "party_ready_view" => {
                let p = party(&r["facts"])?;
                Ok(json!(
                    p.map(|p| array(p, "members")
                        .iter()
                        .map(|member| {
                            let pid = s(member, "ready_playlist");
                            let connection = core.sessions.primary(s(member, "player"), core.clock);
                            if consent_matches(
                                core,
                                p,
                                member,
                                connection.as_ref(),
                                pid,
                                &r["facts"],
                            ) {
                                json!(pid)
                            } else {
                                json!("")
                            }
                        })
                        .collect::<Vec<_>>())
                        .unwrap_or_default()
                ))
            }
            "party_effects" => self.effects(core, r),
            "party_select" => {
                let selected = select(
                    core,
                    m::text(&r["player"], 64)?,
                    m::text(&r["playlist"], 64)?,
                    &r["facts"],
                    &r["rooms"],
                )?;
                self.countdowns.consume(s(&selected, "party_id"))?;
                Ok(selected)
            }
            "party_queue_check" => {
                let actor = core.sessions.checked(&r["generation"], core.clock)?;
                initial_room_gate(rooms(&r["rooms"])?, s(&actor, "player"))?;
                core.sessions.queue_check(core, r)
            }
            "party_queue_prepare" => {
                let check = core.sessions.queue_check(core, r)?;
                let actor = core.sessions.checked(&r["generation"], core.clock)?;
                initial_room_gate(rooms(&r["rooms"])?, s(&actor, "player"))?;
                reject(
                    check["ticket"].is_null(),
                    409,
                    "Queue request already has a ticket",
                )?;
                let selected = select(
                    core,
                    s(&actor, "player"),
                    s(&check, "playlist"),
                    &r["facts"],
                    &r["rooms"],
                )?;
                reject(
                    self.permits.len() < 1000,
                    503,
                    "Party queue permit capacity reached",
                )?;
                let token = id()?;
                self.countdowns.consume(s(&selected, "party_id"))?;
                let generations = array(&selected, "members")
                    .iter()
                    .map(|m| m["generation"].clone())
                    .collect::<Vec<_>>();
                self.permits.insert(token.clone(),json!({"expires":core.clock+core.proposal_lease as f64,"generation":actor["generation"],"body":r["body"],"facts":fingerprint(&r["facts"]),
                    "profile":core.policy_pins[s(&check,"playlist")],"generations":generations,"selected":selected}));
                let mut result = selected;
                result["permit"] = json!(token);
                Ok(result)
            }
            _ => Err(Error(400, "Unknown native party operation")),
        }
    }
}
fn schema(r: &Value) -> Result<Value> {
    let required = [
        "player",
        "party",
        "version",
        "handle",
        "playlist",
        "revision",
        "profile",
        "session_key",
        "generation",
    ];
    let columns = if let Some(columns) = r.get("columns") {
        columns
            .as_array()
            .ok_or(Error(400, "Consent schema column observations required"))?
            .clone()
    } else {
        Vec::new()
    };
    m::check(columns.len() <= 64, "Too many consent schema columns")?;
    let names = columns
        .iter()
        .map(|v| m::text(v, 64))
        .collect::<Result<HashSet<_>>>()?;
    m::check(
        names.len() == columns.len(),
        "Duplicate consent schema column",
    )?;
    reject(
        names.is_empty() || required.iter().all(|c| names.contains(c)),
        409,
        "Unknown party consent schema",
    )?;
    let mut statements = vec![stmt(
        "CREATE TABLE IF NOT EXISTS matchmaking_party_consents(player TEXT PRIMARY KEY REFERENCES players(id) ON DELETE CASCADE,party TEXT NOT NULL REFERENCES parties(id) ON DELETE CASCADE,version INTEGER NOT NULL CHECK(version>0),handle TEXT NOT NULL,playlist TEXT NOT NULL,revision TEXT NOT NULL,profile TEXT NOT NULL,session_key TEXT NOT NULL,generation INTEGER NOT NULL CHECK(generation>0),roster TEXT NOT NULL)",
        json!([]),
        None,
    )];
    if !names.is_empty() && !names.contains("roster") {
        statements.push(stmt(
            "ALTER TABLE matchmaking_party_consents ADD COLUMN roster TEXT NOT NULL DEFAULT ''",
            json!([]),
            None,
        ));
    }
    statements.push(stmt(
        "DELETE FROM matchmaking_party_consents",
        json!([]),
        None,
    ));
    statements.push(stmt(
        "UPDATE party_members SET ready_playlist='',ready_revision='',ready_version=0",
        json!([]),
        None,
    ));
    Ok(json!({"statements":statements,"effects":{"parties":[]}}))
}
pub(super) fn value(r: &Value) -> Result<Value> {
    match m::text(&r["op"], 64)? {
        "party_countdown_sample" | "party_countdown_sample_tick" => countdowns::sample(r),
        "party_consent_schema" => schema(r),
        "party_change_plan" => change_plan(
            m::text(&r["party"], 64)?,
            &r["facts"],
            m::boolean(&r["admitted"])?,
        ),
        _ => Err(Error(400, "Unknown native party value operation")),
    }
}
