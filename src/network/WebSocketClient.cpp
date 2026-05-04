#include "network/WebSocketClient.h"
#include "gamecore/BouncingBall.h"
#include "raylib.h"
#include "tools/EngineConfig.h"

#include <cstddef>
#include <iostream>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <unordered_set>

#include <curl/curl.h>

using json = nlohmann::json;

const size_t MAX_PLAYERS = 100;

// single owner of textures
static std::unordered_map<std::string, Texture2D> textureCache;
static std::unordered_map<std::string, uint64_t> textureLastUsedTick;
static uint64_t textureUseTick = 0;
static const size_t MAX_TEXTURES = 200;

// expose cache (read-only)
const std::unordered_map<std::string, Texture2D> &GetTextureCache() {
  return textureCache;
}

void WebSocketClient::Init(const std::string &url) {
  ws.setUrl(url);

  ws.setOnMessageCallback([this](const ix::WebSocketMessagePtr &msg) {
    if (msg->type == ix::WebSocketMessageType::Open) {
      connected = true;
    } else if (msg->type == ix::WebSocketMessageType::Close ||
               msg->type == ix::WebSocketMessageType::Error) {
      connected = false;
    }
    if (msg->type == ix::WebSocketMessageType::Message) {
      std::lock_guard<std::mutex> lock(queueMutex);
      messageQueue.push(msg->str);
    }
  });

  ws.start();
}

bool WebSocketClient::IsConnected() const { return connected; }

// updated: only sets path, cache owns textures
static bool StartsWith(const std::string& s, const char* prefix)
{
  const size_t n = std::char_traits<char>::length(prefix);
  return s.size() >= n && s.compare(0, n, prefix) == 0;
}

static std::string GuessImageExtension(const std::string& urlOrPath)
{
  auto q = urlOrPath.find('?');
  std::string s = (q == std::string::npos) ? urlOrPath : urlOrPath.substr(0, q);

  auto dot = s.find_last_of('.');
  if (dot == std::string::npos) return ".png";
  std::string ext = s.substr(dot);
  if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga" || ext == ".gif")
    return ext;
  return ".png";
}

static size_t CurlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
  const size_t total = size * nmemb;
  auto* out = static_cast<std::vector<unsigned char>*>(userp);
  const unsigned char* begin = static_cast<unsigned char*>(contents);
  out->insert(out->end(), begin, begin + total);
  return total;
}

static bool DownloadUrlToMemory(const std::string& url, std::vector<unsigned char>& outBytes)
{
  outBytes.clear();
  CURL* curl = curl_easy_init();
  if (!curl) return false;

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outBytes);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "Chatbattles/1.0");
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

  CURLcode res = curl_easy_perform(curl);
  long httpCode = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
  curl_easy_cleanup(curl);

  return (res == CURLE_OK) && (httpCode >= 200 && httpCode < 300) && !outBytes.empty();
}

static void EvictOneTextureIfNeeded(const std::unordered_set<std::string>& inUseKeys)
{
  if (textureCache.size() < MAX_TEXTURES) return;

  // Prefer evicting textures that are not referenced by any live ball.
  for (auto it = textureCache.begin(); it != textureCache.end(); ++it)
  {
    if (inUseKeys.find(it->first) == inUseKeys.end())
    {
      UnloadTexture(it->second);
      textureLastUsedTick.erase(it->first);
      textureCache.erase(it);
      return;
    }
  }

  // All cached textures appear to be in use. Evict least-recently-used.
  uint64_t bestTick = UINT64_MAX;
  std::string bestKey;
  for (const auto& kv : textureLastUsedTick)
  {
    if (kv.second < bestTick)
    {
      bestTick = kv.second;
      bestKey = kv.first;
    }
  }

  if (!bestKey.empty())
  {
    auto it = textureCache.find(bestKey);
    if (it != textureCache.end())
    {
      UnloadTexture(it->second);
      textureCache.erase(it);
    }
    textureLastUsedTick.erase(bestKey);
  }
}

static void ApplyPfp(BouncingBall &ball, const std::string &pfp, const std::unordered_set<std::string>& inUseKeys) {
  if (pfp.empty())
    return;

  auto it = textureCache.find(pfp);
  if (it != textureCache.end() && it->second.id != 0) {
    ball.ball.pfpPath = pfp;
    textureLastUsedTick[pfp] = textureUseTick;
    return;
  }

  std::cout << "Loading PFP: " << pfp << std::endl;

  Texture2D tex{};
  if (StartsWith(pfp, "http://") || StartsWith(pfp, "https://"))
  {
    std::vector<unsigned char> bytes;
    if (!DownloadUrlToMemory(pfp, bytes))
    {
      std::cout << "PFP download failed: " << pfp << std::endl;
      return;
    }

    const std::string ext = GuessImageExtension(pfp);
    Image img = LoadImageFromMemory(ext.c_str(), bytes.data(), (int)bytes.size());
    if (!img.data)
    {
      std::cout << "Image decode failed: " << pfp << std::endl;
      return;
    }

    tex = LoadTextureFromImage(img);
    UnloadImage(img);
  }
  else
  {
    Image img = LoadImage(pfp.c_str());
    if (!img.data) {
      std::cout << "Image load failed: " << pfp << std::endl;
      return;
    }

    tex = LoadTextureFromImage(img);
    UnloadImage(img);
  }

  if (tex.id == 0) {
    std::cout << "Texture creation failed: " << pfp << std::endl;
    return;
  }

  EvictOneTextureIfNeeded(inUseKeys);

  textureCache[pfp] = tex;
  textureLastUsedTick[pfp] = textureUseTick;

  // only store path (NO texture copy!)
  ball.ball.pfpPath = pfp;
}

void WebSocketClient::Update(
    std::vector<std::unique_ptr<BouncingBall>> &balls) {

  std::lock_guard<std::mutex> lock(queueMutex);

  ++textureUseTick;

  std::unordered_set<std::string> inUseKeys;
  inUseKeys.reserve(balls.size());
  for (const auto& b : balls)
  {
    if (!b) continue;
    if (!b->ball.pfpPath.empty())
    {
      inUseKeys.insert(b->ball.pfpPath);
      textureLastUsedTick[b->ball.pfpPath] = textureUseTick;
    }
  }

  while (!messageQueue.empty()) {
    std::string msg = messageQueue.front();
    messageQueue.pop();

    try {
      auto data = json::parse(msg);

      if (!data.contains("type") || !data["type"].is_string())
        continue;
      if (!data.contains("user") || !data["user"].is_string())
        continue;
      if (!data.contains("userId") || !data["userId"].is_string())
        continue;

      const std::string type = data["type"];
      const std::string user = data["user"];
      const std::string userId = data["userId"];

      std::string pfp;
      if (data.contains("pfp") && data["pfp"].is_string()) {
        pfp = data["pfp"];
      }

      // 🔍 find existing
      BouncingBall *target = nullptr;

      for (auto &b : balls) {
        if (b->ball.userId == userId) {
          target = b.get();
          break;
        }
      }

      // create if not found
      if (!target) {

        // 🔍 try to reuse a dead ball
        for (auto &b : balls) {
          if (!b->ball.alive) {

            const int w = (EngineConfig::WindowWidth > 0) ? EngineConfig::WindowWidth : 1;
            const int h = (EngineConfig::WindowHeight > 0) ? EngineConfig::WindowHeight : 1;
            Vector2 spawn = {(float)(rand() % w), (float)(rand() % h)};

            b->Init(spawn);
            b->ball.username = user;
            b->ball.userId = userId;
            //b->ball.pfpPath = pfp;
            ApplyPfp(*b, pfp, inUseKeys);
            if (!pfp.empty()) inUseKeys.insert(pfp);

            target = b.get();
            break;
          }
        }

        // ➕ if no dead slot, only add if under limit
        if (!target && balls.size() < MAX_PLAYERS) {
          auto newBall = std::make_unique<BouncingBall>();

          const int w = (EngineConfig::WindowWidth > 0) ? EngineConfig::WindowWidth : 1;
          const int h = (EngineConfig::WindowHeight > 0) ? EngineConfig::WindowHeight : 1;
          Vector2 spawn = {(float)(rand() % w), (float)(rand() % h)};

          newBall->Init(spawn);
          newBall->ball.username = user;
          newBall->ball.userId = userId;
          //newBall->ball.pfpPath = pfp;
          ApplyPfp(*newBall, pfp, inUseKeys);
          if (!pfp.empty()) inUseKeys.insert(pfp);

          balls.push_back(std::move(newBall));
          target = balls.back().get();
        }

        // ❌ if full and no dead players → ignore new user
        if (!target) {
          return;
        }
      }

      target->ball.username = user;

      if (!pfp.empty() && target->ball.pfpPath != pfp) {
        ApplyPfp(*target, pfp, inUseKeys);
        inUseKeys.insert(pfp);
      }

      // ---------------- CHAT ----------------
      // Viewer abilities are gift-driven (no chat commands here).
      if (type == "chat") {
        continue;
      }

      // ---------------- GIFT ----------------
      else if (type == "gift") {
        // The Node service is the source of truth for gift name resolution
        // (it owns the comprehensive id map + live availableGifts catalog).
        // We accept either a resolved gift name OR a raw giftId, plus an
        // optional diamondCount that drives tier-based fallback.
        std::string gift;
        if (data.contains("gift") && data["gift"].is_string()) {
          gift = data["gift"];
        } else if (data.contains("giftId") && data["giftId"].is_number_integer()) {
          const int giftId = data["giftId"];
          // Minimal local fallback for the most common ids in case the
          // server message is missing a name string. Names here MUST match
          // the cases in AbilityFromGift in src/gamecore/Abilities.cpp.
          static const std::unordered_map<int, std::string> giftMap = {
              {5269, "TikTok"},        {5487, "Finger Heart"},  {5650, "Mic"},
              {5655, "Rose"},          {5657, "Lollipop"},      {5658, "Perfume"},
              {5659, "Paper Crane"},   {5660, "Hand Hearts"},   {5707, "Love you"},
              {5663, "Heels"},         {5879, "Doughnut"},      {5586, "Hearts"},
              {5915, "Music Note"},    {5651, "Garland"},       {5652, "Ferris Wheel"},
              {5489, "Carousel"},      {5765, "Motorcycle"},    {5767, "Private Jet"},
              {5938, "Pool Party"},    {5732, "Submarine"},     {5763, "Speedboat"},
              {5764, "Ice Machine"},   {5488, "LOVE Balloon"},  {5897, "Swan"},
              {5730, "Treehouse"},     {5731, "Coral"},         {5734, "Goggles"},
              {5880, "Lock and Key"},  {5882, "Rock 'n' Roll"}, {5661, "Air Dancer"},
              {5899, "Swing"},         {5662, "Necklace"},      {5760, "Weights"},
              {5827, "Ice Cream Cone"},{5874, "Lion 222"},      {5919, "Love you"},
              {6149, "Interstellar"},  {5954, "Planet"},        {5930, "Rocket"},
              {6223, "Lion"},
          };
          auto it = giftMap.find(giftId);
          if (it != giftMap.end()) gift = it->second;
          else gift = "gift_" + std::to_string(giftId);
        }

        int amount = 1;
        if (data.contains("amount") && data["amount"].is_number_integer()) {
          amount = data["amount"];
        }

        int diamondCount = 0;
        if (data.contains("diamondCount") && data["diamondCount"].is_number_integer()) {
          diamondCount = data["diamondCount"];
        } else if (data.contains("diamond_count") && data["diamond_count"].is_number_integer()) {
          diamondCount = data["diamond_count"];
        }

        if (gift.empty() && diamondCount <= 0) continue;
        target->ApplyGift(gift, amount, diamondCount);
      }

    } catch (const std::exception &e) {
      std::cerr << "JSON parse error: " << e.what() << std::endl;
    }
  }
}

void WebSocketClient::Shutdown() {
  ws.stop();
  ws.close();

  for (auto &pair : textureCache) {
    UnloadTexture(pair.second);
  }
  textureCache.clear();
  textureLastUsedTick.clear();
}
