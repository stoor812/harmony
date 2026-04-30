#pragma once

#include <string>
#include <array>
#include <mutex>

// Track data structure - mirrors the tracks.json schema exactly
struct Track {
    std::string id;
    std::string title;
    std::string artist;
    std::string album;
    std::string album_art;
    std::string preview_url;

    bool is_valid() const { return !id.empty(); }
};

// Fixed-size queue - max 20 tracks (10 local + headroom for search results)
class TrackQueue {
public:
    static constexpr int MAX_SIZE = 20;

    TrackQueue();

    // Load tracks from tracks.json on startup
    bool load_from_file(const std::string& filepath);

    // Playback controls
    Track current_track() const;
    bool advance();           // Move to next track
    bool is_empty() const;
    int size() const;
    int remaining() const;    // Tracks left after current

    // Add a track (from iTunes search results)
    bool enqueue(const Track& track);
    bool enqueue_front(const Track& track);  // insert at position 1 (plays next)

    // Get next N tracks for "Up Next" display in frontend
    std::array<Track, 3> peek_next(int count) const;

    // Check if queue needs refilling (drops to 2 or fewer)
    bool needs_refill() const;

    void clear();

private:
    std::array<Track, MAX_SIZE> tracks_;
    int head_;    // Index of current track
    int count_;   // Total tracks loaded
    mutable std::mutex mutex_;
};