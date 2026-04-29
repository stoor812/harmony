#include "itunes_client.h"
#include "logger.h"

#include <nlohmann/json.hpp>
#include <cpr/cpr.h>

using json = nlohmann::json;

ItunesClient::ItunesClient() : simulate_failure_(false) {
    Logger::instance().info("ITUNES", "iTunes client initialized");
}

void ItunesClient::simulate_failure(bool enabled) {
    simulate_failure_ = enabled;
    Logger::instance().warn("ITUNES",
        std::string("FAULT INJECTION: API failure simulation ") +
        (enabled ? "ENABLED" : "DISABLED"));
}

std::vector<Track> ItunesClient::search(const std::string& query, int limit) {
    if (simulate_failure_) {
        Logger::instance().warn("ITUNES", "Simulated failure - returning empty results");
        return {};
    }

    // URL encode the query (replace spaces with +)
    std::string encoded_query = query;
    for (char& c : encoded_query) {
        if (c == ' ') c = '+';
    }

    std::string url = "https://itunes.apple.com/search?term=" +
                      encoded_query +
                      "&entity=song&limit=" +
                      std::to_string(limit);

    Logger::instance().info("ITUNES", "Searching for: " + query);
    return fetch(url);
}

std::vector<Track> ItunesClient::get_tracks_by_artist(const std::string& artist, int limit) {
    if (simulate_failure_) {
        Logger::instance().warn("ITUNES", "Simulated failure - cannot refill queue");
        return {};
    }

    Logger::instance().info("ITUNES", "Fetching tracks for artist: " + artist);
    return search(artist, limit);
}

std::vector<Track> ItunesClient::fetch(const std::string& url) {
    std::vector<Track> results;

    try {
        // cpr makes HTTP requests look clean - internally uses libcurl
        auto response = cpr::Get(
            cpr::Url{url},
            cpr::Timeout{5000}  // 5 second timeout - don't hang forever
        );

        if (response.status_code != 200) {
            Logger::instance().error("ITUNES",
                "API returned status " + std::to_string(response.status_code));
            return {};
        }

        json data = json::parse(response.text);
        auto& items = data["results"];

        int track_id = 1000;  // Start IDs high so they don't clash with local tracks
        for (auto& item : items) {
            // iTunes returns many types - only process songs
            if (item.value("kind", "") != "song") continue;

            Track track;
            track.id          = std::to_string(track_id++);
            track.title       = item.value("trackName", "Unknown");
            track.artist      = item.value("artistName", "Unknown");
            track.album       = item.value("collectionName", "Unknown");
            track.album_art   = item.value("artworkUrl100", "");
            track.preview_url = item.value("previewUrl", "");

            if (!track.preview_url.empty()) {
                results.push_back(track);
            }
        }

        Logger::instance().info("ITUNES",
            "Found " + std::to_string(results.size()) + " tracks for query");

    } catch (const std::exception& e) {
        Logger::instance().error("ITUNES",
            "Request failed: " + std::string(e.what()));
    }

    return results;
}