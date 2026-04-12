#pragma once

#include <queue>
#include <mutex>
#include <string>
#include <memory>
#include <vector>

#include <ixwebsocket/IXWebSocket.h>
#include "raylib.h"

class BouncingBall;

class WebSocketClient {
public:
    void Init(const std::string& url);
    void Update(std::vector<std::unique_ptr<BouncingBall>> &balls);
    void Shutdown();

private:
    ix::WebSocket ws;

    std::queue<std::string> messageQueue;
    std::mutex queueMutex;

    //std::unordered_map<std::string, std::shared_ptr<Texture2D>> textureCache;

    //void ApplyPfp(BouncingBall &ball, const std::string &pfp);
};
