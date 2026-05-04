#pragma once

#include <deque>
#include <string>
#include <vector>

struct KillFeedEntry {
  double timeSeconds = 0.0;
  std::string text;
};

class KillFeed {
public:
  void Add(double nowSeconds, std::string text);
  void Clear();

  // Returns newest-first entries, already trimmed by age and max size.
  std::vector<KillFeedEntry> Get(double nowSeconds) const;

  void SetMaxEntries(std::size_t n);
  void SetMaxAgeSeconds(double seconds);

private:
  std::deque<KillFeedEntry> entries; // oldest -> newest
  std::size_t maxEntries = 12;
  double maxAgeSeconds = 20.0;
};

