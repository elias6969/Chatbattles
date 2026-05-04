#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

struct PlayerStats {
  std::string userId;
  std::string username;

  int kills = 0;
  int deaths = 0;

  float damageDealt = 0.0f;
  float damageTaken = 0.0f;

  int streak = 0;
  int bestStreak = 0;

  double lastKillTime = -1.0;
};

class StatsManager {
public:
  PlayerStats& EnsurePlayer(const std::string& userId, const std::string& username);
  void SetUsername(const std::string& userId, const std::string& username);

  void RecordDamage(const std::string& attackerUserId,
                    const std::string& victimUserId,
                    float amount);

  void RecordDeath(const std::string& victimUserId);
  void RecordKill(const std::string& killerUserId,
                  const std::string& victimUserId,
                  double nowSeconds);

  const std::unordered_map<std::string, PlayerStats>& All() const;

  std::vector<const PlayerStats*> TopKills(std::size_t n) const;
  std::vector<const PlayerStats*> TopDamage(std::size_t n) const;

private:
  std::unordered_map<std::string, PlayerStats> statsByUserId;
};

