#include "order_book.h"
#include <iomanip>
#include <iostream>

OrderBook:: OrderBook(): num_bids_(0), num_asks_(0){
    for(size_t i=0; i<MAX_ORDERS; i++){
        orders_[i].active = false;
    }
}

void OrderBook::add_bid_volume(uint32_t price, uint32_t shares) {
    for(size_t i=0; i<num_bids_; ++i){
        if(bids_[i].price == price){
            bids_[i].volume += shares;
            return;
        }
    }
    if(num_bids_ >= MAX_LEVELS) return;

    size_t pos = 0;

    while(pos<num_bids_ && bids_[pos].price > price) pos++;

    for(size_t i=num_bids_; i>pos; i--) bids_[i] = bids_[i-1];

    bids_[pos] = {price, shares};
    num_bids_++;
}

void OrderBook::remove_bid_volume(uint32_t price, uint32_t shares) {
    for(size_t i=0; i<num_bids_;++i){
        if(bids_[i].price == price){
            if(bids_[i].volume <= shares){
                for(size_t j = i; j<num_bids_-1; ++j){
                    bids_[j] = bids_[j+1];
                }
                num_bids_--;
            } else {
                bids_[i].volume -= shares;
            }
            return;
        }
    }
}

void OrderBook::add_ask_volume(uint32_t price, uint32_t shares) {
    for(size_t i=0; i<num_asks_; ++i){
        if(asks_[i].price == price){
            asks_[i].volume += shares;
            return;
        }
    }

    if(num_asks_ >= MAX_LEVELS) return;

    size_t pos = 0;

    while(pos < num_asks_ && asks_[pos].price < price) pos++;

    for(size_t i = num_asks_; i>pos; --i) asks_[i] = asks_[i-1];

    asks_[pos] = {price, shares};
    num_asks_++;
}

void OrderBook::remove_ask_volume(uint32_t price, uint32_t shares) {
    for(size_t i = 0; i<num_asks_; ++i){
        if(asks_[i].price == price){
            if(asks_[i].volume <=shares){
                for(size_t j = i; j<num_asks_-1; ++j){
                    asks_[j] = asks_[j+1];
                }
                num_asks_--;
            } else {
                asks_[i].volume -= shares;
            }
            return;
        }
    }
}

void OrderBook::add_order(uint64_t ref_num, char side, uint32_t shares, uint32_t price, uint16_t locate) {

    size_t idx = ref_num & (MAX_ORDERS - 1);
    size_t probes = 0;
    while (orders_[idx].active && orders_[idx].order_ref_num != ref_num && probes < 8) {
        idx = (idx + 1) & (MAX_ORDERS - 1);
        probes++;
    }

    orders_[idx] = Order{ref_num, side, shares, price, locate, true};  

    if (side == 'B') {
        add_bid_volume(price, shares);
    } else if (side == 'S') {
        add_ask_volume(price, shares);
    }
}

void OrderBook::execute_order(uint64_t ref_num, uint32_t exec_shares) {
    
    size_t idx = ref_num & (MAX_ORDERS - 1);
    size_t probes = 0;
    while (orders_[idx].active && orders_[idx].order_ref_num != ref_num && probes < 8) {
        idx = (idx + 1) & (MAX_ORDERS - 1);
        probes++;
    }
    
    Order& order = orders_[idx];
    if(!order.active || order.order_ref_num != ref_num) return;

    if(order.side == 'B'){
        remove_bid_volume(order.price, exec_shares);
    } else if(order.side == 'S'){
        remove_ask_volume(order.price, exec_shares);
    }

    if (order.shares <= exec_shares) {
        order.active = false;
    } else {
        order.shares -= exec_shares; // Order partially filled
    }
}

void OrderBook::cancel_order(uint64_t ref_num, uint32_t cancel_shares) {
    // Identical volume-reduction logic to execution
    execute_order(ref_num, cancel_shares);
}

void OrderBook::delete_order(uint64_t ref_num) {
    size_t idx = ref_num & (MAX_ORDERS - 1);
    size_t probes = 0;
    while (orders_[idx].active && orders_[idx].order_ref_num != ref_num && probes < 8) {
        idx = (idx + 1) & (MAX_ORDERS - 1);
        probes++;
    }
    Order& order = orders_[idx];
    if(!order.active || order.order_ref_num != ref_num) return;

    if(order.side == 'B'){
        remove_bid_volume(order.price, order.shares);
    } else if(order.side == 'S'){
        remove_ask_volume(order.price, order.shares);
    }
    order.active = false;
}

void OrderBook::replace_order(uint64_t orig_ref_num, uint64_t new_ref_num, uint32_t new_shares, uint32_t new_price) {

    size_t idx = orig_ref_num & (MAX_ORDERS - 1);
    size_t probes = 0;
    while (orders_[idx].active && orders_[idx].order_ref_num != orig_ref_num && probes < 8) {
        idx = (idx + 1) & (MAX_ORDERS - 1);
        probes++;
    }

    if(!orders_[idx].active || orders_[idx].order_ref_num != orig_ref_num) return;

    char side = orders_[idx].side;
    uint16_t locate = orders_[idx].stock_locate;

    delete_order(orig_ref_num); // Remove original order
    add_order(new_ref_num, side, new_shares, new_price, locate); // Add updated order
}

uint32_t OrderBook::get_best_bid() const {
    return (num_bids_ > 0) ? bids_[0].price : 0; // Highest price in bids
}

uint32_t OrderBook::get_best_ask() const {
    return (num_asks_ > 0) ? asks_[0].price : 0; // Lowest price in asks
}

void OrderBook::print_bbo() const {
        if (num_bids_ == 0 && num_asks_ == 0) {
            std::cout << "[BBO] Empty Book\n";
            return;
        }

        std::cout << "[BBO] ";
        if (num_bids_ > 0) {
            double bid_px = bids_[0].price / 10000.0;
            uint32_t bid_vol = bids_[0].volume;
            std::cout << "Bid: $" << std::fixed << std::setprecision(2) << bid_px << " (" << bid_vol << " sh)";
        } else {
            std::cout << "Bid: N/A";
        }

        std::cout << " | ";

        if (num_asks_ > 0) {
            double ask_px = asks_[0].price / 10000.0;
            uint32_t ask_vol = asks_[0].volume;
            std::cout << "Ask: $" << std::fixed << std::setprecision(2) << ask_px << " (" << ask_vol << " sh)";
        } else {
            std::cout << "Ask: N/A";
        }

        if (num_bids_ > 0 && num_asks_ > 0) {
            double spread = (static_cast<double>(asks_[0].price) - static_cast<double>(bids_[0].price)) / 10000.0;
            std::cout << " | Spread: $" << std::fixed << std::setprecision(2) << spread;
        }

        std::cout << "\n";
    }