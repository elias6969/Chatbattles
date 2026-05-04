#include "gamecore/KillFeed.h"

#include <algorithm>

void KillFeed::Add(double nowSeconds, std::string text) {
  if (text.empty()) return;
  entries.push_back({nowSeconds, std::move(text)});
  while (entries.size() > maxEntries) entries.pop_front();
}

void KillFeed::Clear() { entries.clear(); }

std::vector<KillFeedEntry> KillFeed::Get(double nowSeconds) const {
  std::vector<KillFeedEntry> out;
  out.reserve(entries.size());

  for (const auto& e : entries) {
    if ((nowSeconds - e.timeSeconds) <= maxAgeSeconds) out.push_back(e);
  }

  // newest first
  std::reverse(out.begin(), out.end());
  return out;
}

void KillFeed::SetMaxEntries(std::size_t n) {
  maxEntries = (n == 0) ? 1 : n;
  while (entries.size() > maxEntries) entries.pop_front();
}

void KillFeed::SetMaxAgeSeconds(double seconds) {
  maxAgeSeconds = (seconds <= 0.0) ? 1.0 : seconds;
}

