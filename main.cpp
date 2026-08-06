 #include "mapped_file.h"
#include "itch_types.h"
#include "itch_utils.h"
#include "order_book.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <cstring>

// Set to true to print individual message logs, or false for high-speed benchmark mode
constexpr bool VERBOSE = false;

// Fixed 2D array of 65,536 entries storing raw 8-byte symbol blocks (Zero Heap Allocations)
char symbol_map[65536][9]; 

// Array of OrderBook engines (one for every possible stock locate ID)
OrderBook books[65536];

int main() {
    // For faster I/O
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    // Open ITCH file in binary mode (ifstream)
    // std::ifstream file("real_sample.itch", std::ios::binary);
    // if (!file) {
    //     std::cerr << "Failed to open binary file. Make sure it exists in the working directory.\n";
    //     return 1;
    // }

    // Open ITCH file using Memory-Mapped I/O
    MappedFile file("real_sample.itch");

    const uint8_t* ptr = file.data();
    const uint8_t* end = file.data() + file.size();


    std::cout << "Starting ITCH 5.0 Feed Parser (REAL NASDAQ SESSION: DEC 30, 2019)\n";
    
    std::uint16_t raw_length = 0;
    size_t processed_count = 0;

    // Accumulator to ensure compiler never optimizes away field decoding math
    uint64_t total_volume_traded = 0;

    // Fixed stack buffer allocated ONCE outside the loop (alignas(8) for L1 data cache speed)
    alignas(8) char buffer[512];

    auto start_time = std::chrono::high_resolution_clock::now();

    // Loop and read the 2-byte length prefix until EOF
    while (ptr + 2 <= end) {
        uint16_t raw_length = 0;
        std::memcpy(&raw_length, ptr, sizeof(raw_length));

        uint16_t msg_length = bswap16(raw_length);
        ptr += 2;

        if(msg_length ==0 || ptr + msg_length > end) break;

        // Zero copy pointer directly into mapped memory
        const char* buffer = reinterpret_cast<const char*>(ptr);
        char message_type = buffer[0];
        processed_count++;


        switch (message_type) {

            // Stock Directory Message
            case 'R': {
                const auto* msg = reinterpret_cast<const StockDirectoryMessage*>(buffer);
                uint16_t locate = bswap16(msg->stock_locate);
                std::memcpy(symbol_map[locate], msg->stock, 8);
                symbol_map[locate][8] = '\0'; // Null-terminate for string safety

                if (VERBOSE) {
                    uint32_t lot_size = bswap32(msg->round_lot_size);
                    std::cout << "[Stock Directory] Locate: " << std::setw(5) << locate
                              << " | Symbol: " << std::setw(8) << symbol_map[locate]
                              << " | Lot Size: " << lot_size << "\n";
                }
                break;
            }
            
            // System Event Message
            case 'S': {
                const auto* msg = reinterpret_cast<const SystemEventMessage*>(buffer);
                uint16_t stock_locate = bswap16(msg->stock_locate);
                uint16_t tracking_num = bswap16(msg->tracking_number);
                uint64_t ns = parse_timestamp48(msg->timestamp);

                if (VERBOSE) {
                    std::cout << "[System Event] "
                              << "Locate: " << std::setw(5) << stock_locate
                              << " | Seq: " << std::setw(5) << tracking_num
                              << " | Time: " << format_timestamp(ns)
                              << " | Code: " << msg->event_code << "\n";
                }
                break;
            }

            // Add Order Message (No MPID)
            case 'A': {
                const auto* msg = reinterpret_cast<const AddOrderMessage*>(buffer);
                uint16_t stock_locate = bswap16(msg->stock_locate);
                uint16_t tracking_num = bswap16(msg->tracking_number);
                uint64_t ns = parse_timestamp48(msg->timestamp);
                uint64_t order_ref = bswap64(msg->order_ref_num);
                uint32_t shares = bswap32(msg->shares);
                uint32_t raw_price = bswap32(msg->price);
                double price = raw_price / 10000.0;
                total_volume_traded += shares;

                books[stock_locate].add_order(order_ref, msg->buy_sell_indicator, shares, raw_price, stock_locate);

                if (VERBOSE) {
                    std::string symbol = clean_symbol(msg->stock, 8);
                    std::cout << "[Add Order]    "
                              << "Locate: " << std::setw(5) << stock_locate
                              << " | Seq: " << std::setw(5) << tracking_num
                              << " | Time: " << format_timestamp(ns)
                              << " | Ref#: " << std::setw(10) << order_ref
                              << " | Side: " << msg->buy_sell_indicator
                              << " | Shares: " << std::setw(5) << shares
                              << " | Stock: " << std::setw(8) << symbol
                              << " | Price: $" << std::fixed << std::setprecision(4) << price << "\n";
                }
                break;
            }

            // Order Executed Message
            case 'E': {
                const auto* msg = reinterpret_cast<const OrderExecutedMessage*>(buffer);
                uint16_t stock_locate = bswap16(msg->stock_locate);
                uint16_t tracking_num = bswap16(msg->tracking_number);
                uint64_t ns = parse_timestamp48(msg->timestamp);
                uint64_t order_ref = bswap64(msg->order_ref_num);
                uint32_t exec_shares = bswap32(msg->executed_shares);
                uint64_t match_num = bswap64(msg->match_number);
                total_volume_traded += exec_shares;

                books[stock_locate].execute_order(order_ref, exec_shares);

                if (VERBOSE) {
                    std::cout << "[Executed]     "
                              << "Locate: " << std::setw(5) << stock_locate
                              << " | Seq: " << std::setw(5) << tracking_num
                              << " | Time: " << format_timestamp(ns)
                              << " | Ref#: " << std::setw(10) << order_ref
                              << " | ExecShares: " << std::setw(5) << exec_shares
                              << " | Stock: " << std::setw(8) << symbol_map[stock_locate]
                              << " | Match#: " << match_num << "\n";
                }
                break;
            }

            // Order Cancel Message
            case 'X': {
                const auto* msg = reinterpret_cast<const OrderCancelMessage*>(buffer);
                uint16_t stock_locate = bswap16(msg->stock_locate);
                uint16_t tracking_num = bswap16(msg->tracking_number);
                uint64_t ns = parse_timestamp48(msg->timestamp);
                uint64_t order_ref = bswap64(msg->order_ref_num);
                uint32_t cancel_shares = bswap32(msg->cancelled_shares);

                books[stock_locate].cancel_order(order_ref, cancel_shares);

                if (VERBOSE) {
                    std::cout << "[Cancel]       "
                              << "Locate: " << std::setw(5) << stock_locate
                              << " | Seq: " << std::setw(5) << tracking_num
                              << " | Time: " << format_timestamp(ns)
                              << " | Ref#: " << std::setw(10) << order_ref
                              << " | CancelShares: " << std::setw(5) << cancel_shares
                              << " | Stock: " << std::setw(8) << symbol_map[stock_locate] << "\n";
                }
                break;
            }

            // Order Delete Message
            case 'D': {
                const auto* msg = reinterpret_cast<const OrderDeleteMessage*>(buffer);
                uint16_t stock_locate = bswap16(msg->stock_locate);
                uint16_t tracking_num = bswap16(msg->tracking_number);
                uint64_t ns = parse_timestamp48(msg->timestamp);
                uint64_t order_ref = bswap64(msg->order_ref_num);

                books[stock_locate].delete_order(order_ref);

                if (VERBOSE) {
                    std::cout << "[Delete]       "
                              << "Locate: " << std::setw(5) << stock_locate
                              << " | Seq: " << std::setw(5) << tracking_num
                              << " | Time: " << format_timestamp(ns)
                              << " | Ref#: " << std::setw(10) << order_ref
                              << " | Stock: " << std::setw(8) << symbol_map[stock_locate] << "\n";
                }
                break;
            }

            // Order Replace Message
            case 'U': {
                const auto* msg = reinterpret_cast<const OrderReplaceMessage*>(buffer);
                uint16_t stock_locate = bswap16(msg->stock_locate);
                uint16_t tracking_num = bswap16(msg->tracking_number);
                uint64_t ns = parse_timestamp48(msg->timestamp);
                uint64_t orig_ref = bswap64(msg->original_order_ref_num);
                uint64_t new_ref = bswap64(msg->new_order_ref_num);
                uint32_t shares = bswap32(msg->shares);
                uint32_t raw_price = bswap32(msg->price);
                double price = raw_price / 10000.0;

                books[stock_locate].replace_order(orig_ref, new_ref, shares, raw_price);

                if (VERBOSE) {
                    std::cout << "[Replace]      "
                              << "Locate: " << std::setw(5) << stock_locate
                              << " | Seq: " << std::setw(5) << tracking_num
                              << " | Time: " << format_timestamp(ns)
                              << " | OrigRef#: " << std::setw(10) << orig_ref
                              << " | NewRef#: " << std::setw(10) << new_ref
                              << " | Shares: " << std::setw(5) << shares
                              << " | Stock: " << std::setw(8) << symbol_map[stock_locate]
                              << " | NewPrice: $" << std::fixed << std::setprecision(4) << price << "\n";
                }
                break;
            }

            default:
                break;
        }
        ptr += msg_length;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    double elapsed_sec = std::chrono::duration<double>(end_time - start_time).count();

    std::cout << "=========================================================================\n";
    std::cout << "Parsing ended. Processed " << processed_count << " messages.\n";
    std::cout << "Total Volume Processed: " << total_volume_traded << " shares\n";
    std::cout << "Processed " << processed_count << " messages in " << elapsed_sec << " seconds.\n";
    std::cout << "True Field-Decoding Throughput: " << static_cast<uint64_t>(processed_count / elapsed_sec) << " msg/sec\n";
    std::cout << "=========================================================================\n";

    std::cout << "=========================================================================\n";
    std::cout << "REAL MARKET TOP-OF-BOOK (BBO) STATE (DEC 30, 2019 SAMPLES)\n";
    std::cout << "=========================================================================\n";

    size_t printed_stocks = 0;
    for (uint16_t loc = 1; loc < 65536; loc++) {
        if (books[loc].get_best_bid() > 0 || books[loc].get_best_ask() > 0) {
            std::cout << "Stock: " << std::setw(8) << symbol_map[loc] << " (Locate " << std::setw(4) << loc << ") | ";
            books[loc].print_bbo();
            printed_stocks++;
            if (printed_stocks >= 15) {
                break; // Top 15 active real stocks
            }
        }
    }
    std::cout << "=========================================================================\n";

    return 0;
}