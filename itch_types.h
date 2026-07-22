#pragma once
#include <cstdint>

// ============================================================================
// PACKED ITCH 5.0 STRUCTURES
// ============================================================================
#pragma pack(push, 1)


// Message Type: 'S' - System Event Message
struct SystemEventMessage {
    char message_type;        // 'S'
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint8_t timestamp[6];
    char event_code;
};

// Message Type: 'R' - Stock Directory Message
struct StockDirectoryMessage {
    char message_type;        // 'R'
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint8_t timestamp[6];
    char stock[8];
    char market_category;
    char financial_status_indicator;
    uint32_t round_lot_size;
    char round_lots_only;
    char issue_classification;
    char issue_subtype[2];
    char authenticity;
    char short_sale_threshold_indicator;
    char ipo_flag;
    char luld_reference_price_tier;
    char etp_flag;
    uint32_t etp_leverage_factor;
    char inverse_indicator;
};

// Message Type: 'H' - Stock Trading Action Message
struct StockTradingActionMessage {
    char message_type;        // 'H'
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint8_t timestamp[6];
    char stock[8];
    char trading_state;
    char reserved;
    char reason[4];
};

// Message Type: 'Y' - Reg SHO Indicator Message
struct RegSHOIndicatorMessage {
    char message_type;        // 'Y'
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint8_t timestamp[6];
    char stock[8];
    char reg_sho_action;
};

// Message Type: 'L' - Market Participant Position Message
struct MarketParticipantPositionMessage {
    char message_type;        // 'L'
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint8_t timestamp[6];
    char mpid[4];
    char stock[8];
    char primary_market_maker;
    char market_maker_mode;
    char market_participant_state;
};

// Message Type: 'V' - MWCB Decline Level Message
struct MWCBDeclineLevelMessage {
    char message_type;        // 'V'
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint8_t timestamp[6];
    uint64_t level1;
    uint64_t level2;
    uint64_t level3;
};

// Message Type: 'W' - MWCB Status Message
struct MWCBStatusMessage {
    char message_type;        // 'W'
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint8_t timestamp[6];
    char breached_level;
};

// Message Type: 'K' - IPO Quoting Period Update Message
struct IPOQuotingPeriodUpdateMessage {
    char message_type;        // 'K'
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint8_t timestamp[6];
    char stock[8];
    uint32_t ipo_quotation_release_time;
    char ipo_quotation_release_qualifier;
    uint32_t ipo_price;
};

// Message Type: 'J' - LULD Auction Collar Message
struct LULDAuctionCollarMessage {
    char message_type;        // 'J'
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint8_t timestamp[6];
    char stock[8];
    uint32_t auction_collar_reference_price;
    uint32_t upper_auction_collar_price;
    uint32_t lower_auction_collar_price;
    uint32_t auction_collar_extension;
};

// Message Type: 'A' - Add Order Message (No MPID)
struct AddOrderMessage {
    char message_type;        // 'A'
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint8_t timestamp[6];
    uint64_t order_ref_num;
    char buy_sell_indicator;
    uint32_t shares;
    char stock[8];
    uint32_t price;
};

// Message Type: 'F' - Add Order Message (With MPID / Attribution)
struct AddOrderMPIDMessage {
    char message_type;        // 'F'
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint8_t timestamp[6];
    uint64_t order_ref_num;
    char buy_sell_indicator;
    uint32_t shares;
    char stock[8];
    uint32_t price;
    char attribution[4];
};

// Message Type: 'E' - Order Executed Message
struct OrderExecutedMessage {
    char message_type;        // 'E'
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint8_t timestamp[6];
    uint64_t order_ref_num;
    uint32_t executed_shares;
    uint64_t match_number;
};

// Message Type: 'C' - Order Executed Price Message
struct OrderExecutedPriceMessage {
    char message_type;        // 'C'
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint8_t timestamp[6];
    uint64_t order_ref_num;
    uint32_t executed_shares;
    uint64_t match_number;
    char printable;
    uint32_t execution_price;
};

// Message Type: 'X' - Order Cancel Message
struct OrderCancelMessage {
    char message_type;        // 'X'
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint8_t timestamp[6];
    uint64_t order_ref_num;
    uint32_t cancelled_shares;
};

// Message Type: 'D' - Order Delete Message
struct OrderDeleteMessage {
    char message_type;        // 'D'
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint8_t timestamp[6];
    uint64_t order_ref_num;
};

// Message Type: 'U' - Order Replace Message
struct OrderReplaceMessage {
    char message_type;        // 'U'
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint8_t timestamp[6];
    uint64_t original_order_ref_num;
    uint64_t new_order_ref_num;
    uint32_t shares;
    uint32_t price;
};

// Message Type: 'P' - Trade Message (Non-Cross)
struct TradeMessage {
    char message_type;        // 'P'
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint8_t timestamp[6];
    uint64_t order_ref_num;   // Note: Always 0 or dummy for non-cross trades
    char buy_sell_indicator;
    uint32_t shares;
    char stock[8];
    uint32_t price;
    uint64_t match_number;
};

// Message Type: 'Q' - Cross Trade Message
struct CrossTradeMessage {
    char message_type;        // 'Q'
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint8_t timestamp[6];
    uint64_t shares;
    char stock[8];
    uint32_t cross_price;
    uint64_t match_number;
    char cross_type;
};

// Message Type: 'B' - Broken Trade Message
struct BrokenTradeMessage {
    char message_type;        // 'B'
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint8_t timestamp[6];
    uint64_t match_number;
};

// Message Type: 'I' - Net Order Imbalance Indicator Message (NOII)
struct NOIIMessage {
    char message_type;        // 'I'
    uint16_t stock_locate;
    uint16_t tracking_number;
    uint8_t timestamp[6];
    uint64_t paired_shares;
    uint64_t imbalance_shares;
    char imbalance_direction;
    char stock[8];
    uint32_t far_price;
    uint32_t near_price;
    uint32_t current_reference_price;
    char cross_type;
    char price_variation_indicator;
};

#pragma pack(pop)
