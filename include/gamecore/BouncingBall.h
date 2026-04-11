#pragma once

#include "raylib.h"
#include <memory>
#include <string>

struct Ball {
  Vector2 position;
  Vector2 velocity;
  float radius;
  Color color;
  std::string username;

  Texture2D pfpTexture;
  bool hasPfp = false;
  std::string userId;
};

class BouncingBall {
public:
  void Init();
  void Update();
  void Draw();

  void SetTexture(Texture2D tex);

  Ball ball;

private:
  std::unique_ptr<Font> font;
};
