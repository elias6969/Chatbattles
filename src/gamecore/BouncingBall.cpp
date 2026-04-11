#include "gamecore/BouncingBall.h"
#include "tools/EngineConfig.h"

#include <cmath>
#include <cstdlib>
#include <ctime>

void BouncingBall::Init() {
  static bool seeded = false;
  if (!seeded) {
    std::srand(std::time(nullptr));
    seeded = true;
  }

  ball.position = {
    EngineConfig::WindowWidth * 0.5f,
    EngineConfig::WindowHeight * 0.5f
  };

  float speed = 500.0f;

  float dirX = (std::rand() % 200 - 100) / 100.0f;
  float dirY = (std::rand() % 200 - 100) / 100.0f;

  float length = std::sqrt(dirX * dirX + dirY * dirY);

  if (length == 0.0f) {
    dirX = 1.0f;
    dirY = 0.0f;
  } else {
    dirX /= length;
    dirY /= length;
  }

  ball.velocity = { dirX * speed, dirY * speed };

  ball.radius = 50.0f;
  ball.color = BLUE;

  font = std::make_unique<Font>(
    LoadFont("../assets/fonts/Roboto/Roboto.ttf")
  );
}

void BouncingBall::Update() {
  ball.position.x += ball.velocity.x * EngineConfig::dt;
  ball.position.y += ball.velocity.y * EngineConfig::dt;

  if (ball.position.x - ball.radius <= 0) {
    ball.position.x = ball.radius;
    ball.velocity.x *= -1;
  }
  else if (ball.position.x + ball.radius >= EngineConfig::WindowWidth) {
    ball.position.x = EngineConfig::WindowWidth - ball.radius;
    ball.velocity.x *= -1;
  }

  if (ball.position.y - ball.radius <= 0) {
    ball.position.y = ball.radius;
    ball.velocity.y *= -1;
  }
  else if (ball.position.y + ball.radius >= EngineConfig::WindowHeight) {
    ball.position.y = EngineConfig::WindowHeight - ball.radius;
    ball.velocity.y *= -1;
  }
}

void BouncingBall::Draw() {
  if (ball.hasPfp) {
    DrawTexturePro(
      ball.pfpTexture,
      { 0, 0, (float)ball.pfpTexture.width, (float)ball.pfpTexture.height },
      { ball.position.x, ball.position.y, ball.radius * 2, ball.radius * 2 },
      { ball.radius, ball.radius },
      0.0f,
      WHITE
    );
  } else {
    DrawCircleV(ball.position, ball.radius, ball.color);
  }

  DrawTextEx(
    *font,
    ball.username.c_str(),
    { ball.position.x - 40.0f, ball.position.y - 80.0f },
    50.0f,
    2.0f,
    BLACK
  );
}

void BouncingBall::SetTexture(Texture2D tex) {
  ball.pfpTexture = tex;
  ball.hasPfp = true;
}
