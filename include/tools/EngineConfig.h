#ifndef ENGINE_CONFIG_H
#define ENGINE_CONFIG_H

#include <string>


class EngineConfig {
public:
  // Asset directories
  static std::string TextureDirectory;
  static std::string ShaderDirectory;
  static std::string CubemapDirectory;
  static std::string FontDirectory;
  static std::string ModelDirectory;

  // Window / framebuffer state
  static int WindowWidth;
  static int WindowHeight;
  static void UpdateWindowSize();
  static void PollWindowResize();
  static float dt;
};

#endif // ENGINE_CONFIG_H
