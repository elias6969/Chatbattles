#pragma once

#include <queue>
#include <mutex>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

#include <ixwebsocket/IXWebSocket.h>
#include "raylib.h"

class BouncingBall;

const std::unordered_map<std::string, Texture2D>& GetTextureCache();

class WebSocketClient {
public:
    void Init(const std::string& url);
    void Update(std::vector<std::unique_ptr<BouncingBall>> &balls);
    void Shutdown();
    bool IsConnected() const;

private:
    ix::WebSocket ws;
    bool connected = false;

    std::queue<std::string> messageQueue;
    std::mutex queueMutex;
};
