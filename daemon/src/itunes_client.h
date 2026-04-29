#pragma once

#include "queue.h"
#include <string>
#include <vector>

class ItunesClient {
public:
    ItunesClient();

    // Search iTunes for tracks matching query
    // Returns empty vector on failure (triggers fallback to local catalog)
    std::vector<Track> search(const std::string& query, int limit = 5);

    // Fetch top tracks for an artist - used for queue auto-refill
    std::vector<Track> get_tracks_by_artist(const std::string& artist, int limit = 5);

    // Fault injection: simulate API being unreachable
    void simulate_failure(bool enabled);
    bool is_simulating_failure() const { return simulate_failure_; }

private:
    bool simulate_failure_;

    std::vector<Track> fetch(const std::string& url);
    Track parse_track_json(const std::string& item_json);
};