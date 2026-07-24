#include "order_book.h"
#include <iomanip>
#include <iostream>

void OrderBook::add_order(uint64_t ref_num, char side, uint32_t shares, uint32_t price, uint16_t locate) {
    
    orders_[ref_num] = Order{ref_num, side, shares, price, locate};   

    if (side == 'B') {
        bids_[price] += shares;
    } else if (side == 'S') {
        asks_[price] += shares;
    }
}

void OrderBook::execute_order(uint64_t ref_num, uint32_t exec_shares) {
    auto it = orders_.find(ref_num);
    if(it == orders_.end()) return; // Order not found

    Order& order = it->second;
    if(order.side == 'B'){
        bids_[order.price] -= exec_shares;
        if(bids_[order.price] == 0) bids_.erase(order.price);
    } else if(order.side == 'S'){
        asks_[order.price] -= exec_shares;
        if(asks_[order.price] == 0) asks_.erase(order.price);
    }

    if (order.shares <= exec_shares) {
        orders_.erase(it); // Order fully filled, remove from lookup map
    } else {
        order.shares -= exec_shares; // Order partially filled
    }
}

void OrderBook::cancel_order(uint64_t ref_num, uint32_t cancel_shares) {
    // Identical volume-reduction logic to execution
    execute_order(ref_num, cancel_shares);
}

void OrderBook::delete_order(uint64_t ref_num) {
    auto it = orders_.find(ref_num);
    if(it == orders_.end()) return; 

    Order& order = it->second;
    if(order.side == 'B'){
        bids_[order.price] -= order.shares;
        if(bids_[order.price] == 0) bids_.erase(order.price);
    } else if(order.side == 'S'){
        asks_[order.price] -= order.shares;
        if(asks_[order.price] == 0) asks_.erase(order.price);
    }

    orders_.erase(it);
}

void OrderBook::replace_order(uint64_t orig_ref_num, uint64_t new_ref_num, uint32_t new_shares, uint32_t new_price) {
    auto it = orders_.find(orig_ref_num);
    if(it == orders_.end()) return; // Original order not found

    char side = it->second.side;
    uint16_t locate = it->second.stock_locate;

    delete_order(orig_ref_num); // Remove original order
    add_order(new_ref_num, side, new_shares, new_price, locate); // Add updated order
}

uint32_t OrderBook::get_best_bid() const {
    if(bids_.empty()) return 0;
    return bids_.begin()->first; // Highest price in bids
}

uint32_t OrderBook::get_best_ask() const {
    if(asks_.empty()) return 0;
    return asks_.begin()->first; // Lowest price in asks
}

void OrderBook::print_bbo() const {
        if (bids_.empty() && asks_.empty()) {
            std::cout << "[BBO] Empty Book\n";
            return;
        }

        std::cout << "[BBO] ";
        if (!bids_.empty()) {
            double bid_px = bids_.begin()->first / 10000.0;
            uint32_t bid_vol = bids_.begin()->second;
            std::cout << "Bid: $" << std::fixed << std::setprecision(2) << bid_px << " (" << bid_vol << " sh)";
        } else {
            std::cout << "Bid: N/A";
        }

        std::cout << " | ";

        if (!asks_.empty()) {
            double ask_px = asks_.begin()->first / 10000.0;
            uint32_t ask_vol = asks_.begin()->second;
            std::cout << "Ask: $" << std::fixed << std::setprecision(2) << ask_px << " (" << ask_vol << " sh)";
        } else {
            std::cout << "Ask: N/A";
        }

        if (!bids_.empty() && !asks_.empty()) {
            double spread = (static_cast<double>(asks_.begin()->first) - static_cast<double>(bids_.begin()->first)) / 10000.0;
            std::cout << " | Spread: $" << std::fixed << std::setprecision(2) << spread;
        }

        std::cout << "\n";
    }