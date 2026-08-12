#pragma once

#include <cstdint>
#include <map>
#include <unordered_map>

struct PriceLevel {
    uint32_t price;
    uint32_t volume;
};

struct Order {
    uint64_t order_ref_num;
    char side;           // 'B' for Buy, 'S' for Sell
    uint32_t shares;     // Current remaining open shares
    uint32_t price;      // Raw fixed-point integer price (divide by 10000.0 for dollars)
    uint16_t stock_locate;
    bool active;         // If order slot is currently in use
};

static constexpr size_t MAX_ORDERS = 4096;
static constexpr size_t MAX_LEVELS = 8;

class alignas(64) OrderBook {
    public:
        OrderBook();
        // Methods to handle ITCH events
        void add_order(uint64_t ref_num, char side, uint32_t shares, uint32_t price, uint16_t locate);
        void execute_order(uint64_t ref_num, uint32_t exec_shares);
        void cancel_order(uint64_t ref_num, uint32_t cancel_shares);
        void delete_order(uint64_t ref_num);
        void replace_order(uint64_t orig_ref_num, uint64_t new_ref_num, uint32_t new_shares, uint32_t new_price);

        // Queries
        uint32_t get_best_bid() const;
        uint32_t get_best_ask() const;
        void print_bbo() const;

    private:
        inline void add_bid_volume(uint32_t price, uint32_t shares) {
            if (num_asks_ > 0 && price >= asks_[0].price) [[unlikely]] {
                while (num_asks_ > 0 && price >= asks_[0].price) {
                    for (size_t j = 0; j < num_asks_ - 1; ++j) asks_[j] = asks_[j + 1];
                    num_asks_--;
                }
            }
            size_t i = 0;
            for (; i < num_bids_; ++i) {
                if (bids_[i].price == price) {
                    bids_[i].volume += shares;
                    return;
                }
                if (bids_[i].price < price) break;
            }
            if (num_bids_ >= MAX_LEVELS) return;
            for (size_t j = num_bids_; j > i; --j) bids_[j] = bids_[j - 1];
            bids_[i] = {price, shares};
            num_bids_++;
        }

        inline void remove_bid_volume(uint32_t price, uint32_t shares) {
            for (size_t i = 0; i < num_bids_; ++i) {
                if (bids_[i].price == price) {
                    if (bids_[i].volume <= shares) {
                        for (size_t j = i; j < num_bids_ - 1; ++j) bids_[j] = bids_[j + 1];
                        num_bids_--;
                    } else {
                        bids_[i].volume -= shares;
                    }
                    return;
                }
                if (bids_[i].price < price) break; // Prices are sorted descending
            }
        }

        inline void add_ask_volume(uint32_t price, uint32_t shares) {
            if (num_bids_ > 0 && price <= bids_[0].price) [[unlikely]] {
                while (num_bids_ > 0 && price <= bids_[0].price) {
                    for (size_t j = 0; j < num_bids_ - 1; ++j) bids_[j] = bids_[j + 1];
                    num_bids_--;
                }
            }
            size_t i = 0;
            for (; i < num_asks_; ++i) {
                if (asks_[i].price == price) {
                    asks_[i].volume += shares;
                    return;
                }
                if (asks_[i].price > price) break;
            }
            if (num_asks_ >= MAX_LEVELS) return;
            for (size_t j = num_asks_; j > i; --j) asks_[j] = asks_[j - 1];
            asks_[i] = {price, shares};
            num_asks_++;
        }

        inline void remove_ask_volume(uint32_t price, uint32_t shares) {
            for (size_t i = 0; i < num_asks_; ++i) {
                if (asks_[i].price == price) {
                    if (asks_[i].volume <= shares) {
                        for (size_t j = i; j < num_asks_ - 1; ++j) asks_[j] = asks_[j + 1];
                        num_asks_--;
                    } else {
                        asks_[i].volume -= shares;
                    }
                    return;
                }
                if (asks_[i].price > price) break; // Prices are sorted ascending
            }
        }

        Order orders_[MAX_ORDERS];
        PriceLevel bids_[MAX_LEVELS];
        PriceLevel asks_[MAX_LEVELS];

        size_t num_bids_ = 0;
        size_t num_asks_ = 0;
};