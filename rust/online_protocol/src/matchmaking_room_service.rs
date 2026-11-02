//! Production room admission/projections over the existing Rust owners.
//! Only typed SQL snapshots cross the private service seam, never HTTP facts.
use super::{Coordinator, fingerprint, m, reject};
use crate::matchmaking_model::{Error, Result, array, flag, s};
use serde_json::{Value, json};
use std::collections::{BTreeMap, BTreeSet};

fn call(core: &mut Coordinator, mut r: Value) -> Result<Value> {
    r["now"] = json!(core.clock);
    core.execute(&r)
}
fn prepare(core: &mut Coordinator, generation: &Value, wall: &Value) -> Result<Value> {
    let actor = core.sessions.checked(generation, core.clock)?;
    let who = s(&actor, "player");
    let ticket = core
        .tickets
        .iter()
        .find(|t| array(t, "players").iter().any(|p| p["id"] == who))
        .cloned()
        .ok_or(Error(409, "No active match search"))?;
    let status = call(core, json!({"op":"status","id":ticket["id"]}))?;
    let id = status["proposal_id"]
        .as_str()
        .ok_or(Error(409, "Players are still being found"))?;
    if core.setup.service_contains(id) {
        return Ok(json!({"id":id,"existing":true}));
    }
    let proposal = core.proposal(id)?;
    let policy = core.policy(s(proposal, "playlist"))?;
    let catalog = core
        .catalog
        .as_ref()
        .ok_or(Error(503, "Admitted service catalog required"))?;
    let entry = catalog.entry(s(proposal, "playlist"))?;
    let configuration = entry["rank_configuration"].clone();
    let group = core.group(&super::strings(&proposal["tickets"])?)?;
    let players = Coordinator::players(&group);
    let mut clans = BTreeMap::new();
    let mut clan_players = BTreeMap::new();
    let mut operations = Vec::new();
    let mut subjects = Vec::new();
    let is_clan = !policy["clan_match"].is_null();
    if is_clan {
        for (team, ids) in array(proposal, "teams").iter().enumerate() {
            let mut names = BTreeSet::new();
            for member in ids.as_array().unwrap() {
                let t = group
                    .iter()
                    .find(|t| array(t, "players").iter().any(|p| p["id"] == *member))
                    .unwrap();
                let clan = m::text(&t["clan"], 64)?.to_owned();
                names.insert(clan.clone());
                clan_players.insert(m::text(member, 64)?.to_owned(), clan);
            }
            reject(names.len() == 1, 409, "Clan result owners changed")?;
            clans.insert(team.to_string(), names.into_iter().next().unwrap());
        }
        reject(
            clans.len() == 2 && clans.values().collect::<BTreeSet<_>>().len() == 2,
            409,
            "Clan result owners changed",
        )?;
        for clan in clans.values() {
            subjects.push(clan.clone());
            operations.push(json!({"mode":"clan_snapshot","owner":clan,"playlist":proposal["playlist"],"configuration":configuration}));
        }
    } else if !configuration.is_null() {
        for player in players.iter().filter(|p| !flag(p, "guest")) {
            subjects.push(s(player, "id").to_owned());
            operations.push(json!({"mode":"player_snapshot","owner":player["id"],"playlist":proposal["playlist"],"configuration":configuration}));
        }
    }
    let binding = core
        .proposal_bindings
        .get(id)
        .ok_or(Error(409, "Proposal binding retired"))?;
    Ok(
        json!({"id":id,"existing":false,"binding":fingerprint(binding),
        "operations":operations,"subjects":subjects,"clan_players":clan_players,
        "context":{"proposal":id,"skill_configuration":if is_clan{Value::Null}else{configuration.clone()},
            "clan_configuration":if is_clan{configuration}else{Value::Null},"skills":{},"clans":clans,"clan_skills":{},
            "created_wall":m::integer(wall,0,9007199254740991)?}}),
    )
}
pub(super) fn execute(core: &mut Coordinator, r: &Value) -> Result<Value> {
    match s(r, "op") {
        "room_service_exists" => Ok(json!(core.setup.service_contains(m::text(&r["id"], 64)?))),
        "room_service_prepare" => prepare(core, &r["generation"], &r["wall"]),
        "room_service_admit" => {
            // A rejected snapshot/proof cannot leave result or setup state,
            // even when this private operation is called outside the SQL owner.
            let mut next = core.clone();
            let plan = prepare(
                &mut next,
                &r["generation"],
                &r["prepared"]["context"]["created_wall"],
            )?;
            reject(
                plan == r["prepared"] && plan["existing"] == false,
                409,
                "Room admission changed during IO",
            )?;
            let facts = r["clans"]
                .as_object()
                .ok_or(Error(503, "Current clan facts required"))?;
            reject(
                plan["clan_players"].as_object().unwrap().len() == facts.len()
                    && plan["clan_players"]
                        .as_object()
                        .unwrap()
                        .iter()
                        .all(|(p, c)| facts.get(p) == Some(c)),
                409,
                "Clan match owners changed",
            )?;
            let snapshots = r["snapshots"]
                .as_array()
                .ok_or(Error(503, "Room skill snapshots required"))?;
            let subjects = array(&plan, "subjects");
            reject(
                snapshots.len() == subjects.len(),
                503,
                "Room snapshot bank differs",
            )?;
            let mut context = plan["context"].clone();
            let field = if context["clan_configuration"].is_null() {
                "skills"
            } else {
                "clan_skills"
            };
            for (subject, snapshot) in subjects.iter().zip(snapshots) {
                context[field][subject.as_str().unwrap()] = snapshot.clone();
            }
            call(
                &mut next,
                json!({"op":"result_open","id":plan["id"],"context":context}),
            )?;
            call(&mut next, json!({"op":"api_room_admit","id":plan["id"]}))?;
            call(
                &mut next,
                json!({"op":"room_state_get","id":plan["id"],"io_now":r["io_now"]}),
            )?;
            *core = next;
            Ok(plan["id"].clone())
        }
        "room_service_observations" => {
            let rows = core
                .setup
                .service_rows(m::real(&r["io_now"])?, &core.results)?;
            let select = |fields: &[&str]| {
                json!(
                    rows.as_array()
                        .unwrap()
                        .iter()
                        .map(|r| fields
                            .iter()
                            .map(|k| (k.to_string(), r[*k].clone()))
                            .collect::<serde_json::Map<_, _>>())
                        .collect::<Vec<_>>()
                )
            };
            Ok(
                json!({"queue":select(&["id","closed","phase","owners","active"]),
                "matching":select(&["closed","phase","owners","active","playlist","clans"]),
                "sessions":select(&["closed","phase","owners","active"])}),
            )
        }
        _ => Err(Error(400, "Unknown room service operation")),
    }
}
