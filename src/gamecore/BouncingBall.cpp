#include "gamecore/BouncingBall.h"
#include "raylib.h"
#include "tools/EngineConfig.h"
#include <memory>

void BouncingBall::Init() {
  ball.position = {EngineConfig::WindowWidth * 0.5f,
                   EngineConfig::WindowHeight * 0.5f};

  ball.velocity = {500.0f, 500.0f};
  ball.radius = 50.0f;
  ball.color = BLUE;
  font = std::make_unique<Font>(LoadFont("../assets/fonts/Roboto/Roboto.ttf"));
}

void BouncingBall::Update() {
  // Move
  ball.position.x += ball.velocity.x * EngineConfig::dt;
  ball.position.y += ball.velocity.y * EngineConfig::dt;

  // Horizontal bounce
  if (ball.position.x - ball.radius <= 0) {
    ball.position.x = ball.radius;
    ball.velocity.x *= -1;
  } else if (ball.position.x + ball.radius >= EngineConfig::WindowWidth) {
    ball.position.x = EngineConfig::WindowWidth - ball.radius;
    ball.velocity.x *= -1;
  }

  // Vertical bounce
  if (ball.position.y - ball.radius <= 0) {
    ball.position.y = ball.radius;
    ball.velocity.y *= -1;
  } else if (ball.position.y + ball.radius >= EngineConfig::WindowHeight) {
    ball.position.y = EngineConfig::WindowHeight - ball.radius;
    ball.velocity.y *= -1;
  }
}

void BouncingBall::Draw() {
  DrawCircleV(ball.position, ball.radius, ball.color);

  DrawTextEx(*font, ball.username.c_str(),
             {ball.position.x - 40.0f, ball.position.y - 80.0f}, 50.0f, 2.0f,
             BLACK);
}
