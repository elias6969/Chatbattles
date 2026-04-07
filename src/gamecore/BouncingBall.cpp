#include "gamecore/BouncingBall.h"
#include "raylib.h"
#include "tools/EngineConfig.h"

void BouncingBall::Init() {
  ball.position = {EngineConfig::WindowWidth * 0.5f,
                   EngineConfig::WindowHeight * 0.5f};

  ball.velocity = {1000.0f, 1000.0f};
  ball.radius = 50.0f;
  ball.color = BLUE;
}

void BouncingBall::Update() {
  // Move
  ball.position.x += ball.velocity.x * EngineConfig::dt;
  ball.position.y += ball.velocity.y * EngineConfig::dt;

  // Horizontal bounce
  if (ball.position.x - ball.radius <= 0) {
    ball.position.x = ball.radius;
    ball.velocity.x *= -1;
    ball.color = RED;
  } else if (ball.position.x + ball.radius >= EngineConfig::WindowWidth) {
    ball.position.x = EngineConfig::WindowWidth - ball.radius;
    ball.velocity.x *= -1;
    ball.color = RED;
  }

  // Vertical bounce
  if (ball.position.y - ball.radius <= 0) {
    ball.position.y = ball.radius;
    ball.velocity.y *= -1;
    ball.color = WHITE;
  } else if (ball.position.y + ball.radius >= EngineConfig::WindowHeight) {
    ball.position.y = EngineConfig::WindowHeight - ball.radius;
    ball.velocity.y *= -1;
    ball.color = WHITE;
  }
}

void BouncingBall::Draw() {
  DrawCircleV(ball.position, ball.radius, ball.color);
}
