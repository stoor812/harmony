#include "queue.h"
#include "logger.h"

#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

TrackQueue::TrackQueue() : head_(0), count_(0) {
    Logger::instance().info("QUEUE", "Track queue initialized (max size: " +
        std::to_string(MAX_SIZE) + ")");
}

bool TrackQueue::load_from_file(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        Logger::instance().error("QUEUE", "Could not open tracks file: " + filepath);
        return false;
    }

    try {
        json data = json::parse(file);
        auto& tracks_json = data["tracks"];

        int loaded = 0;
        for (auto& t : tracks_json) {
            if (loaded >= MAX_SIZE) break;

            Track track;
            track.id          = t.value("id", "");
            track.title       = t.value("title", "Unknown Title");
            track.artist      = t.value("artist", "Unknown Artist");
            track.album       = t.value("album", "Unknown Album");
            track.album_art   = t.value("album_art", "");
            track.preview_url = t.value("preview_url", "");

            tracks_[loaded++] = track;
        }

        count_ = loaded;
        head_  = 0;

        Logger::instance().info("QUEUE", "Loaded " + std::to_string(count_) +
            " tracks from " + filepath);
        Logger::instance().info("QUEUE", "Now playing: " +
            tracks_[0].title + " by " + tracks_[0].artist);

        return count_ > 0;

    } catch (const json::exception& e) {
        Logger::instance().error("QUEUE", "JSON parse error: " + std::string(e.what()));
        return false;
    }
}

Track TrackQueue::current_track() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (count_ == 0) return Track{};
    return tracks_[head_];
}

bool TrackQueue::advance() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (count_ == 0) return false;

    // Shift all tracks forward - drop the current one
    for (int i = 0; i < count_ - 1; i++) {
        tracks_[i] = tracks_[i + 1];
    }
    count_--;

    Logger::instance().info("QUEUE", "Advanced queue. Remaining: " +
        std::to_string(count_));

    if (count_ > 0) {
        Logger::instance().info("QUEUE", "Next up: " +
            tracks_[0].title + " by " + tracks_[0].artist);
    }

    return count_ > 0;
}

bool TrackQueue::is_empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return count_ == 0;
}

int TrackQueue::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return count_;
}

int TrackQueue::remaining() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::max(0, count_ - 1);
}

bool TrackQueue::enqueue(const Track& track) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (count_ >= MAX_SIZE) {
        Logger::instance().warn("QUEUE", "Queue full, cannot enqueue: " + track.title);
        return false;
    }
    tracks_[count_++] = track;
    Logger::instance().info("QUEUE", "Enqueued: " + track.title +
        " by " + track.artist);
    return true;
}

std::array<Track, 3> TrackQueue::peek_next(int count) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::array<Track, 3> result{};
    int available = std::min(count, std::min(count_ - 1, 3));
    for (int i = 0; i < available; i++) {
        result[i] = tracks_[i + 1];
    }
    return result;
}

bool TrackQueue::needs_refill() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return count_ <= 2;
}

void TrackQueue::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    count_ = 0;
    head_  = 0;
    Logger::instance().info("QUEUE", "Queue cleared");
}