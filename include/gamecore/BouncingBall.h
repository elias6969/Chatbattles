#pragma once

#include "raylib.h"

struct Ball {
  Vector2 position;
  Vector2 velocity;
  float radius;
  Color color;
};

class BouncingBall {
public:
  void Init();
  void Update();
  void Draw();

  Ball ball;
private:
};
