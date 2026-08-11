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
static constexpr size_t MAX_LEVELS = 32;

class OrderBook {
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
        void add_bid_volume(uint32_t price, uint32_t shares);
        void remove_bid_volume(uint32_t price, uint32_t shares);
        void add_ask_volume(uint32_t price, uint32_t shares);
        void remove_ask_volume(uint32_t price, uint32_t shares);

        Order orders_[MAX_ORDERS];
        PriceLevel bids_[MAX_LEVELS];
        PriceLevel asks_[MAX_LEVELS];

        size_t num_bids_ = 0;
        size_t num_asks_ = 0;
};