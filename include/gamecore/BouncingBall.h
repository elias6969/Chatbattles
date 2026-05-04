#pragma once

#include "raylib.h"
#include <string>
#include <vector>
#include "gamecore/Abilities.h"

struct Particle {
  Vector2 position;
  Vector2 velocity;
  float life;
  float maxLife;
  float size;
  Color color{255, 200, 50, 255};
};

struct Ball {
  Vector2 position;
  Vector2 velocity;
  float radius;
  Color color;

  std::string username;
  std::string userId;

  //Texture2D pfpTexture{};
  //bool hasPfp = false;
  std::string pfpPath;

  float health = 100.0f;
  float maxHealth = 100.0f;

  float hitTimer = 0.0f;
  bool alive = true;

  // combat attribution (session-only)
  std::string lastHitByUserId;
  double lastHitAtSeconds = -1.0;

  // respawn / protection
  float respawnTimer = 0.0f;
  float invulnTimer = 0.0f;

  // ability state
  AbilityState ability;

  // GAME STATS
  float speedMultiplier = 1.0f;
  float damageMultiplier = 1.0f;
  float knockbackMultiplier = 1.0f;
  float shield = 0.0f;

  // timers
  float speedTimer = 0.0f;
  float damageTimer = 0.0f;
  float shieldTimer = 0.0f;
};

class BouncingBall {
public:
  void Init(Vector2 startPos);
  void Update();
  void Draw();

  // Applies collision damage. Returns damage applied (after shield), \u2265 0.
  // Caller can check `ball.alive` to see if it resulted in death.
  float OnCollision(const std::string& attackerUserId,
                    double nowSeconds,
                    Vector2 point,
                    float strength);

  void ApplyGift(const std::string& gift, int amount, int diamondCount = 0);

  void SetTexture(Texture2D tex);

  Ball ball;

private:
  std::vector<Particle> particles;
  void SpawnParticles(Vector2 pos, float strength);
  void UpdateParticles();
  void DrawParticles();
};
