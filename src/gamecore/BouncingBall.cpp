#include "gamecore/BouncingBall.h"
#include "raylib.h"
#include "tools/EngineConfig.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

void BouncingBall::Init(Vector2 startPos) {
  ball.position = startPos;

  float speed = 400.0f;
  float angle = ((float)rand() / RAND_MAX) * 2.0f * PI;

  ball.velocity = {
    std::cos(angle) * speed,
    std::sin(angle) * speed
  };

  ball.radius = 25.0f;
  ball.color = BLUE;

  ball.health = 100.0f;
  ball.maxHealth = 100.0f;
  ball.alive = true;
  font = LoadFont("../assets/fonts/Roboto/Roboto.ttf"); 
}

void BouncingBall::Update() {
  if (!ball.alive) return;

  ball.position.x += ball.velocity.x * EngineConfig::dt * ball.speedMultiplier;
  ball.position.y += ball.velocity.y * EngineConfig::dt * ball.speedMultiplier;

  // damping
  ball.velocity.x *= 0.998f;
  ball.velocity.y *= 0.998f;

  // keep movement alive
  float minSpeed = 80.0f;
  float speed = sqrt(ball.velocity.x * ball.velocity.x +
                     ball.velocity.y * ball.velocity.y);

  if (speed < minSpeed && speed > 0.0f) {
    ball.velocity.x *= (minSpeed / speed);
    ball.velocity.y *= (minSpeed / speed);
  }

  // walls
  if (ball.position.x - ball.radius < 0) {
    ball.position.x = ball.radius;
    ball.velocity.x *= -1;
  }
  if (ball.position.x + ball.radius > EngineConfig::WindowWidth) {
    ball.position.x = EngineConfig::WindowWidth - ball.radius;
    ball.velocity.x *= -1;
  }
  if (ball.position.y - ball.radius < 0) {
    ball.position.y = ball.radius;
    ball.velocity.y *= -1;
  }
  if (ball.position.y + ball.radius > EngineConfig::WindowHeight) {
    ball.position.y = EngineConfig::WindowHeight - ball.radius;
    ball.velocity.y *= -1;
  }

  // timers
  ball.speedTimer -= EngineConfig::dt;
  ball.damageTimer -= EngineConfig::dt;
  ball.shieldTimer -= EngineConfig::dt;

  if (ball.speedTimer <= 0) ball.speedMultiplier = 1.0f;
  if (ball.damageTimer <= 0) ball.damageMultiplier = 1.0f;
  if (ball.shieldTimer <= 0) ball.shield = 0.0f;

  ball.hitTimer -= EngineConfig::dt;
  if (ball.hitTimer < 0) ball.hitTimer = 0;

  UpdateParticles();
}

void BouncingBall::Draw() {
  if (!ball.alive) return;

  DrawParticles();

  Color drawColor = (ball.hitTimer > 0) ? RED : ball.color;

  if (ball.hasPfp && ball.pfpTexture.id != 0) {
    DrawTexturePro(
      ball.pfpTexture,
      {0, 0, (float)ball.pfpTexture.width, (float)ball.pfpTexture.height},
      {ball.position.x, ball.position.y, ball.radius * 2, ball.radius * 2},
      {ball.radius, ball.radius},
      0.0f,
      WHITE
    );
  } else {
    DrawCircleV(ball.position, ball.radius, drawColor);
  }

  // username
  int fontSize = 20;
  int textWidth = MeasureText(ball.username.c_str(), fontSize);


DrawTextEx(
  font,
  ball.username.c_str(),
  { ball.position.x - textWidth / 2,
    ball.position.y - ball.radius - 25 },
  (float)fontSize,
  1.0f,   // spacing
  BLACK
);

  // health bar
  float barWidth = ball.radius * 2;
  float healthRatio = ball.health / ball.maxHealth;

  DrawRectangle(ball.position.x - ball.radius,
                ball.position.y - ball.radius - 10,
                barWidth, 5, DARKGRAY);

  DrawRectangle(ball.position.x - ball.radius,
                ball.position.y - ball.radius - 10,
                barWidth * healthRatio, 5, GREEN);
}

void BouncingBall::SetTexture(Texture2D tex) {
  ball.pfpTexture = tex;
  ball.hasPfp = true;
}

void BouncingBall::OnCollision(Vector2 point, float strength) {
  ball.hitTimer = 0.1f;

  float damage = strength * 25.0f * ball.damageMultiplier;

  // shield absorbs first
  if (ball.shield > 0) {
    ball.shield -= damage;
    if (ball.shield < 0) {
      damage = -ball.shield;
      ball.shield = 0;
    } else {
      damage = 0;
    }
  }

  ball.health -= damage;

  if (ball.health <= 0) {
    ball.alive = false;
  }

  SpawnParticles(point, strength);
}

void BouncingBall::ApplyGift(const std::string& gift, int amount) {

  if (gift == "Rose") {
    ball.speedMultiplier = 1.2f;
    ball.speedTimer = 5.0f;
    ball.color = PINK;
  }

  else if (gift == "TikTok") {
    ball.velocity.x += 300.0f * amount;
    ball.velocity.y += 300.0f * amount;
    ball.color = GREEN;
  }

  else if (gift == "Finger Heart") {
    ball.health += 20.0f * amount;
    if (ball.health > ball.maxHealth) ball.health = ball.maxHealth;
  }

  else if (gift == "Perfume") {
    ball.shield += 50.0f * amount;
    ball.shieldTimer = 8.0f;
    ball.color = SKYBLUE;
  }

  else if (gift == "Galaxy") {
    ball.damageMultiplier = 2.0f;
    ball.damageTimer = 6.0f;
    ball.color = RED;
  }

  else if (gift == "Shamrock") {
    ball.speedMultiplier = 1.5f;
    ball.speedTimer = 6.0f;
  }

  else if (gift == "Rosa") {
    ball.health += 40.0f;
  }

  else if (gift == "Donut") {
    ball.radius += 5.0f;
  }

  else if (gift == "GG") {
    ball.knockbackMultiplier = 2.0f;
  }
}

void BouncingBall::SpawnParticles(Vector2 pos, float strength) {
  int count = 5 + (int)(strength * 10);

  for (int i = 0; i < count; i++) {
    float angle = ((float)rand() / RAND_MAX) * 2.0f * PI;
    float speed = ((float)rand() / RAND_MAX) * 200.0f;

    Particle p;
    p.position = pos;
    p.velocity = {
      std::cos(angle) * speed,
      std::sin(angle) * speed
    };
    p.life = 0;
    p.maxLife = 0.5f;
    p.size = 3.0f;

    particles.push_back(p);
  }
}

void BouncingBall::UpdateParticles() {
  for (auto &p : particles) {
    p.life += EngineConfig::dt;
    p.position.x += p.velocity.x * EngineConfig::dt;
    p.position.y += p.velocity.y * EngineConfig::dt;
  }

  particles.erase(
    std::remove_if(particles.begin(), particles.end(),
      [](Particle &p) { return p.life > p.maxLife; }),
    particles.end()
  );
}

void BouncingBall::DrawParticles() {
  for (auto &p : particles) {
    float alpha = 1.0f - (p.life / p.maxLife);

    DrawCircleV(p.position, p.size,
      {255, 200, 50, (unsigned char)(alpha * 255)});
  }
}
