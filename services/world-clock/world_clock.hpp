#pragma once

#include "common/types.hpp"

#include <functional>
#include <vector>

struct Tick {
    int day = 1;
    int year = 1;
    Season season = Season::Spring;
    int day_of_season = 1;
};

class WorldClock {
public:
    static constexpr int DAYS_PER_SEASON = 90;
    static constexpr int SEASONS_PER_YEAR = 4;
    static constexpr int DAYS_PER_YEAR = DAYS_PER_SEASON * SEASONS_PER_YEAR;

    WorldClock() = default;

    Tick tick() {
        ++day_;
        if (day_ > DAYS_PER_YEAR) {
            day_ = 1;
            ++year_;
        }

        auto t = current();

        for (auto& cb : on_tick_) cb(t);

        Season prev = static_cast<Season>(((day_ - 2 + DAYS_PER_YEAR) % DAYS_PER_YEAR) / DAYS_PER_SEASON);
        if (prev != t.season) {
            for (auto& cb : on_season_change_) cb(t);
        }
        if (day_ == 1) {
            for (auto& cb : on_year_change_) cb(t);
        }

        return t;
    }

    [[nodiscard]] Tick current() const {
        Season s = static_cast<Season>((day_ - 1) / DAYS_PER_SEASON);
        int day_of_season = ((day_ - 1) % DAYS_PER_SEASON) + 1;
        return { day_, year_, s, day_of_season };
    }

    void on_tick(std::function<void(const Tick&)> cb) {
        on_tick_.push_back(std::move(cb));
    }
    void on_season_change(std::function<void(const Tick&)> cb) {
        on_season_change_.push_back(std::move(cb));
    }
    void on_year_change(std::function<void(const Tick&)> cb) {
        on_year_change_.push_back(std::move(cb));
    }

    void reset() { day_ = 1; year_ = 1; }
    [[nodiscard]] int day() const { return day_; }
    [[nodiscard]] int year() const { return year_; }

private:
    int day_ = 1;
    int year_ = 1;
    bool running_ = false;
    std::vector<std::function<void(const Tick&)>> on_tick_;
    std::vector<std::function<void(const Tick&)>> on_season_change_;
    std::vector<std::function<void(const Tick&)>> on_year_change_;
};
