#include "gamecore/Stats.h"

#include <algorithm>

PlayerStats& StatsManager::EnsurePlayer(const std::string& userId,
                                        const std::string& username) {
  auto it = statsByUserId.find(userId);
  if (it == statsByUserId.end()) {
    PlayerStats ps;
    ps.userId = userId;
    ps.username = username;
    it = statsByUserId.emplace(userId, std::move(ps)).first;
  } else if (!username.empty()) {
    it->second.username = username;
  }
  return it->second;
}

void StatsManager::SetUsername(const std::string& userId,
                               const std::string& username) {
  if (userId.empty()) return;
  EnsurePlayer(userId, username).username = username;
}

void StatsManager::RecordDamage(const std::string& attackerUserId,
                                const std::string& victimUserId,
                                float amount) {
  if (amount <= 0.0f) return;
  if (!attackerUserId.empty() && attackerUserId != victimUserId) {
    EnsurePlayer(attackerUserId, "").damageDealt += amount;
  }
  if (!victimUserId.empty()) {
    EnsurePlayer(victimUserId, "").damageTaken += amount;
  }
}

void StatsManager::RecordDeath(const std::string& victimUserId) {
  if (victimUserId.empty()) return;
  auto& v = EnsurePlayer(victimUserId, "");
  v.deaths += 1;
  v.streak = 0;
}

void StatsManager::RecordKill(const std::string& killerUserId,
                              const std::string& victimUserId,
                              double nowSeconds) {
  if (killerUserId.empty() || killerUserId == victimUserId) return;
  auto& k = EnsurePlayer(killerUserId, "");
  k.kills += 1;
  k.streak += 1;
  if (k.streak > k.bestStreak) k.bestStreak = k.streak;
  k.lastKillTime = nowSeconds;
}

const std::unordered_map<std::string, PlayerStats>& StatsManager::All() const {
  return statsByUserId;
}

std::vector<const PlayerStats*> StatsManager::TopKills(std::size_t n) const {
  std::vector<const PlayerStats*> out;
  out.reserve(statsByUserId.size());
  for (const auto& kv : statsByUserId) out.push_back(&kv.second);

  const auto cmp = [](const PlayerStats* a, const PlayerStats* b) {
    if (a->kills != b->kills) return a->kills > b->kills;
    if (a->damageDealt != b->damageDealt) return a->damageDealt > b->damageDealt;
    return a->username < b->username;
  };

  if (out.size() > n) {
    std::partial_sort(out.begin(), out.begin() + (long)n, out.end(), cmp);
    out.resize(n);
  } else {
    std::sort(out.begin(), out.end(), cmp);
  }
  return out;
}

std::vector<const PlayerStats*> StatsManager::TopDamage(std::size_t n) const {
  std::vector<const PlayerStats*> out;
  out.reserve(statsByUserId.size());
  for (const auto& kv : statsByUserId) out.push_back(&kv.second);

  const auto cmp = [](const PlayerStats* a, const PlayerStats* b) {
    if (a->damageDealt != b->damageDealt) return a->damageDealt > b->damageDealt;
    if (a->kills != b->kills) return a->kills > b->kills;
    return a->username < b->username;
  };

  if (out.size() > n) {
    std::partial_sort(out.begin(), out.begin() + (long)n, out.end(), cmp);
    out.resize(n);
  } else {
    std::sort(out.begin(), out.end(), cmp);
  }
  return out;
}

