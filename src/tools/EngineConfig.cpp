#include "tools/EngineConfig.h"
#include "raylib.h"
#include "tools/filemanager.h"

// Virtual File System (lazy init)
static VirtualFileSystem &GetVirtualFileSystem() {
  static VirtualFileSystem vfs("../assets");
  return vfs;
}

// Asset paths
std::string EngineConfig::TextureDirectory =
    GetVirtualFileSystem().getFullPath("textures/");

std::string EngineConfig::ShaderDirectory =
    GetVirtualFileSystem().getFullPath("shaders/");

std::string EngineConfig::CubemapDirectory =
    GetVirtualFileSystem().getFullPath("cubemap/");

std::string EngineConfig::FontDirectory =
    GetVirtualFileSystem().getFullPath("fonts/");

std::string EngineConfig::ModelDirectory =
    GetVirtualFileSystem().getFullPath("models/");

int EngineConfig::WindowWidth = 800;
int EngineConfig::WindowHeight = 600;
float EngineConfig::dt = 0.0f;

void EngineConfig::UpdateWindowSize() {
  WindowWidth = GetRenderWidth();
  WindowHeight = GetRenderHeight();
}

void EngineConfig::PollWindowResize() {
  if (IsWindowResized()) {
    UpdateWindowSize();
  }
}
