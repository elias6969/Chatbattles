#pragma once

#include "raylib.h"
#include <string>
#include <vector>

struct Particle {
  Vector2 position;
  Vector2 velocity;
  float life;
  float maxLife;
  float size;
};

struct Ball {
  Vector2 position;
  Vector2 velocity;
  float radius;
  Color color;

  std::string username;
  std::string userId;

  Texture2D pfpTexture{};
  bool hasPfp = false;

  float health = 100.0f;
  float maxHealth = 100.0f;

  float hitTimer = 0.0f;
  bool alive = true;

  // 🎮 GAME STATS
  float speedMultiplier = 1.0f;
  float damageMultiplier = 1.0f;
  float knockbackMultiplier = 1.0f;
  float shield = 0.0f;

  // ⏱ timers
  float speedTimer = 0.0f;
  float damageTimer = 0.0f;
  float shieldTimer = 0.0f;
};

class BouncingBall {
public:
  void Init(Vector2 startPos);
  void Update();
  void Draw();

  void OnCollision(Vector2 point, float strength);

  void ApplyGift(const std::string& gift, int amount);

  void SetTexture(Texture2D tex);

  Ball ball;

private:
  std::vector<Particle> particles;

  Font font;
  void SpawnParticles(Vector2 pos, float strength);
  void UpdateParticles();
  void DrawParticles();
};
