#pragma once

#include <cstdint>
#include <map>
#include <unordered_map>

struct Order {
    uint64_t order_ref_num;
    char side;           // 'B' for Buy, 'S' for Sell
    uint32_t shares;     // Current remaining open shares
    uint32_t price;      // Raw fixed-point integer price (divide by 10000.0 for dollars)
    uint16_t stock_locate;
};

class OrderBook {
    public:
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
        std::unordered_map<uint64_t, Order> orders_;

        // Bids: Sorted Descending (Highest buy price at top)
        std::map<uint32_t, uint32_t, std::greater<uint32_t>> bids_;

        // Asks: Sorted Ascending (Lowest sell price at top)
        std::map<uint32_t, uint32_t> asks_;
};