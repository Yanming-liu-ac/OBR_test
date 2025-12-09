#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>
#include <cmath>

// ========== 1. Data Structures ==========

// Order structure
struct Order {
    long long timestamp;      // Timestamp
    long long order_id;       // Order ID
    double price;        // Price
    long long volume;         // Volume
    char direction;      // 'B' = Buy / 'S' = Sell
    char type;           // 'A' = Add / 'C' = Cancel
};

// Trade structure
struct Trade {
    long long timestamp;      // Timestamp
    double price;        // Trade price
    long long volume;         // Trade volume
    long long bid_order_id;   // Buyer order ID
    long long ask_order_id;   // Seller order ID
    char direction;      // 'B' = Buyer initiated / 'S' = Seller initiated
};

// Order book level structure
struct OrderBookLevel {
    double price;        // Price
    long long total_volume;   // Total volume at this price level
    std::vector<long long> order_ids;  // All order IDs at this price level
};

// ========== 2. Global Data Storage ==========

std::vector<Order> all_orders;    // All order data
std::vector<Trade> all_trades;    // All trade data

// Currently active orders (for quick lookup)
std::vector<Order> active_orders; // Unfilled orders

// Order book data
std::vector<OrderBookLevel> bid_levels;  // Buy side
std::vector<OrderBookLevel> ask_levels;  // Sell side

// ========== 3. File Reading Functions ==========

// Read order file
void read_order_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Cannot open file: " << filename << std::endl;
        return;
    }
    
    std::string line;
    std::getline(file, line);  // Skip header
    std::cout << "Header: " << line << std::endl;
    
    int line_num = 1;
    while (std::getline(file, line)) {
        line_num++;
        if (line.empty()) continue;  // Skip empty lines
        
        try {
            // Simple CSV parsing
            std::vector<std::string> fields;
            std::string field = "";
            
            for (char c : line) {
                if (c == ',' || c == '\r') {  // Handle both comma and carriage return
                    if (c == ',') {
                        fields.push_back(field);
                        field = "";
                    }
                } else {
                    field += c;
                }
            }
            fields.push_back(field);  // Last field
            
            if (fields.size() >= 6) {
                Order order;
                order.timestamp = std::stoll(fields[0]);
                order.order_id = std::stoll(fields[1]);
                order.price = std::stod(fields[2]);
                order.volume = std::stoll(fields[3]);
                order.direction = fields[4][0];
                order.type = fields[5][0];
                
                all_orders.push_back(order);
            } else {
                std::cout << "Warning: Line " << line_num << " has only " << fields.size() << " fields" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "Error parsing line " << line_num << ": " << e.what() << std::endl;
            std::cout << "Line content: " << line << std::endl;
        }
    }
    
    file.close();
    std::cout << "Read " << all_orders.size() << " orders" << std::endl;
}

// Read trade file
void read_trade_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Cannot open file: " << filename << std::endl;
        return;
    }
    
    std::string line;
    std::getline(file, line);  // Skip header
    std::cout << "Header: " << line << std::endl;
    
    int line_num = 1;
    while (std::getline(file, line)) {
        line_num++;
        if (line.empty()) continue;  // Skip empty lines
        
        try {
            std::vector<std::string> fields;
            std::string field = "";
            
            for (char c : line) {
                if (c == ',' || c == '\r') {  // Handle both comma and carriage return
                    if (c == ',') {
                        fields.push_back(field);
                        field = "";
                    }
                } else {
                    field += c;
                }
            }
            fields.push_back(field);
            
            if (fields.size() >= 6) {
                Trade trade;
                trade.timestamp = std::stoll(fields[0]);
                trade.price = std::stod(fields[1]);
                trade.volume = std::stoll(fields[2]);
                trade.bid_order_id = std::stoll(fields[3]);
                trade.ask_order_id = std::stoll(fields[4]);
                trade.direction = fields[5][0];
                
                all_trades.push_back(trade);
            } else {
                std::cout << "Warning: Line " << line_num << " has only " << fields.size() << " fields" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "Error parsing line " << line_num << ": " << e.what() << std::endl;
            std::cout << "Line content: " << line << std::endl;
        }
    }
    
    file.close();
    std::cout << "Read " << all_trades.size() << " trades" << std::endl;
}

// ========== 4. Order Processing Functions ==========

// Find order by ID
int find_active_order(long long order_id) {
    for (int i = 0; i < active_orders.size(); i++) {
        if (active_orders[i].order_id == order_id) {
            return i;  // Return index
        }
    }
    return -1;  // Not found
}

// Find bid level
int find_bid_level(double price) {
    for (int i = 0; i < bid_levels.size(); i++) {
        // Price equality (considering floating point error)
        if (std::abs(bid_levels[i].price - price) < 0.0001) {
            return i;
        }
    }
    return -1;
}

// Find ask level
int find_ask_level(double price) {
    for (int i = 0; i < ask_levels.size(); i++) {
        if (std::abs(ask_levels[i].price - price) < 0.0001) {
            return i;
        }
    }
    return -1;
}

// Process new order
void process_new_order(const Order& order) {
    // 1. Add to active orders list
    active_orders.push_back(order);
    
    // 2. Update order book
    if (order.direction == 'B') {  // Buy order
        int level_idx = find_bid_level(order.price);
        
        if (level_idx >= 0) {
            // Price level exists, add volume
            bid_levels[level_idx].total_volume += order.volume;
            bid_levels[level_idx].order_ids.push_back(order.order_id);
        } else {
            // New price level, create and insert
            OrderBookLevel new_level;
            new_level.price = order.price;
            new_level.total_volume = order.volume;
            new_level.order_ids.push_back(order.order_id);
            
            // Insert at correct position (bid prices high to low)
            int insert_pos = 0;
            while (insert_pos < bid_levels.size() && 
                   bid_levels[insert_pos].price > order.price) {
                insert_pos++;
            }
            bid_levels.insert(bid_levels.begin() + insert_pos, new_level);
        }
    } else {  // Sell order
        int level_idx = find_ask_level(order.price);
        
        if (level_idx >= 0) {
            ask_levels[level_idx].total_volume += order.volume;
            ask_levels[level_idx].order_ids.push_back(order.order_id);
        } else {
            OrderBookLevel new_level;
            new_level.price = order.price;
            new_level.total_volume = order.volume;
            new_level.order_ids.push_back(order.order_id);
            
            // Insert at correct position (ask prices low to high)
            int insert_pos = 0;
            while (insert_pos < ask_levels.size() && 
                   ask_levels[insert_pos].price < order.price) {
                insert_pos++;
            }
            ask_levels.insert(ask_levels.begin() + insert_pos, new_level);
        }
    }
}

// Process cancel order
void process_cancel_order(const Order& order) {
    int order_idx = find_active_order(order.order_id);
    if (order_idx < 0) return;  // Order not found
    
    Order& active_order = active_orders[order_idx];
    
    if (active_order.direction == 'B') {  // Cancel buy order
        int level_idx = find_bid_level(active_order.price);
        if (level_idx >= 0) {
            // Reduce volume
            bid_levels[level_idx].total_volume -= active_order.volume;
            
            // Remove from order ID list
            std::vector<long long>& ids = bid_levels[level_idx].order_ids;
            for (int i = 0; i < ids.size(); i++) {
                if (ids[i] == order.order_id) {
                    ids.erase(ids.begin() + i);
                    break;
                }
            }
            
            // Remove level if no orders left
            if (bid_levels[level_idx].total_volume <= 0) {
                bid_levels.erase(bid_levels.begin() + level_idx);
            }
        }
    } else {  // Cancel sell order
        int level_idx = find_ask_level(active_order.price);
        if (level_idx >= 0) {
            ask_levels[level_idx].total_volume -= active_order.volume;
            
            std::vector<long long>& ids = ask_levels[level_idx].order_ids;
            for (int i = 0; i < ids.size(); i++) {
                if (ids[i] == order.order_id) {
                    ids.erase(ids.begin() + i);
                    break;
                }
            }
            
            if (ask_levels[level_idx].total_volume <= 0) {
                ask_levels.erase(ask_levels.begin() + level_idx);
            }
        }
    }
    
    // Remove from active orders
    active_orders.erase(active_orders.begin() + order_idx);
}

// Process trade
void process_trade(const Trade& trade) {
    // 1. Process buyer order
    int bid_idx = find_active_order(trade.bid_order_id);
    if (bid_idx >= 0) {
        Order& bid_order = active_orders[bid_idx];
        double order_price = bid_order.price;
        
        // First reduce volume in order book
        int level_idx = find_bid_level(order_price);
        if (level_idx >= 0) {
            bid_levels[level_idx].total_volume -= trade.volume;
            
            // If order fully filled, remove from order_ids
            if (bid_order.volume <= trade.volume) {
                std::vector<long long>& ids = bid_levels[level_idx].order_ids;
                for (int i = 0; i < ids.size(); i++) {
                    if (ids[i] == trade.bid_order_id) {
                        ids.erase(ids.begin() + i);
                        break;
                    }
                }
            }
            
            // Check if level should be removed
            if (bid_levels[level_idx].total_volume <= 0) {
                bid_levels.erase(bid_levels.begin() + level_idx);
            }
        }
        
        // Then update order volume
        bid_order.volume -= trade.volume;
        
        // If fully filled, remove from active orders
        if (bid_order.volume <= 0) {
            active_orders.erase(active_orders.begin() + bid_idx);
        }
    }
    
    // 2. Process seller order
    int ask_idx = find_active_order(trade.ask_order_id);
    if (ask_idx >= 0) {
        Order& ask_order = active_orders[ask_idx];
        double order_price = ask_order.price;
        
        // First reduce volume in order book
        int level_idx = find_ask_level(order_price);
        if (level_idx >= 0) {
            ask_levels[level_idx].total_volume -= trade.volume;
            
            // If order fully filled, remove from order_ids
            if (ask_order.volume <= trade.volume) {
                std::vector<long long>& ids = ask_levels[level_idx].order_ids;
                for (int i = 0; i < ids.size(); i++) {
                    if (ids[i] == trade.ask_order_id) {
                        ids.erase(ids.begin() + i);
                        break;
                    }
                }
            }
            
            // Check if level should be removed
            if (ask_levels[level_idx].total_volume <= 0) {
                ask_levels.erase(ask_levels.begin() + level_idx);
            }
        }
        
        // Then update order volume
        ask_order.volume -= trade.volume;
        
        // If fully filled, remove from active orders
        if (ask_order.volume <= 0) {
            active_orders.erase(active_orders.begin() + ask_idx);
        }
    }
}

// ========== 5. Order Book Rebuild Main Function ==========

void rebuild_order_book() {
    std::cout << "Starting order book rebuild..." << std::endl;
    
    // First process all orders
    int add_count = 0, cancel_count = 0;
    for (const Order& order : all_orders) {
        try {
            if (order.type == 'A') {
                process_new_order(order);
                add_count++;
            } else if (order.type == 'C') {
                process_cancel_order(order);
                cancel_count++;
            }
        } catch (const std::exception& e) {
            std::cout << "Error processing order " << order.order_id << ": " << e.what() << std::endl;
        }
    }
    std::cout << "Processed " << add_count << " add orders, " << cancel_count << " cancel orders" << std::endl;
    
    // Then process all trades
    int trade_count = 0;
    for (const Trade& trade : all_trades) {
        try {
            process_trade(trade);
            trade_count++;
        } catch (const std::exception& e) {
            std::cout << "Error processing trade: " << e.what() << std::endl;
        }
    }
    std::cout << "Processed " << trade_count << " trades" << std::endl;
    
    std::cout << "Order book rebuild complete!" << std::endl;
}

// ========== 6. Display Order Book Function ==========

void print_order_book(int max_levels = 5) {
    std::cout << "\n======= Order Book Snapshot =======" << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    
    // Ask side (prices low to high)
    std::cout << "\nASK (Sell Side):" << std::endl;
    std::cout << "Price\t\tVolume\t\tOrders" << std::endl;
    std::cout << "--------------------------------" << std::endl;
    
    int ask_count = std::min((int)ask_levels.size(), max_levels);
    for (int i = 0; i < ask_count; i++) {
        std::cout << ask_levels[i].price << "\t\t" 
                  << ask_levels[i].total_volume << "\t\t"
                  << ask_levels[i].order_ids.size() << std::endl;
    }
    
    if (ask_levels.size() > max_levels) {
        std::cout << "... " << (ask_levels.size() - max_levels) << " more ask levels" << std::endl;
    }
    
    // Bid side (prices high to low)
    std::cout << "\nBID (Buy Side):" << std::endl;
    std::cout << "Price\t\tVolume\t\tOrders" << std::endl;
    std::cout << "--------------------------------" << std::endl;
    
    int bid_count = std::min((int)bid_levels.size(), max_levels);
    for (int i = 0; i < bid_count; i++) {
        std::cout << bid_levels[i].price << "\t\t" 
                  << bid_levels[i].total_volume << "\t\t"
                  << bid_levels[i].order_ids.size() << std::endl;
    }
    
    if (bid_levels.size() > max_levels) {
        std::cout << "... " << (bid_levels.size() - max_levels) << " more bid levels" << std::endl;
    }
    
    // Statistics
    std::cout << "\nStatistics:" << std::endl;
    std::cout << "Active orders: " << active_orders.size() << std::endl;
    std::cout << "Bid levels: " << bid_levels.size() << std::endl;
    std::cout << "Ask levels: " << ask_levels.size() << std::endl;
    
    // Best bid/ask
    if (!bid_levels.empty() && !ask_levels.empty()) {
        double bid_price = bid_levels[0].price;
        double ask_price = ask_levels[0].price;
        double spread = ask_price - bid_price;
        
        std::cout << "\nBest Quotes:" << std::endl;
        std::cout << "Best Bid: " << bid_price << " (" << bid_levels[0].total_volume << " shares)" << std::endl;
        std::cout << "Best Ask: " << ask_price << " (" << ask_levels[0].total_volume << " shares)" << std::endl;
        std::cout << "Spread: " << spread << std::endl;
    }
}

// ========== 7. Main Function ==========

int main() {
    // 1. Read data
    // Try different possible paths
    std::vector<std::string> paths_to_try = {
        "orders.csv",           // Same directory as executable
        "../orders.csv",        // One level up
        "../../orders.csv",     // Two levels up
        "../../../orders.csv",  // Three levels up
        "../../../../orders.csv" // Four levels up
    };
    
    bool found_orders = false;
    for (const auto& path : paths_to_try) {
        std::ifstream test(path);
        if (test.good()) {
            test.close();
            std::cout << "Found orders.csv at: " << path << std::endl;
            read_order_file(path);
            
            // Try trades.csv at same location
            std::string trades_path = path;
            size_t pos = trades_path.find("orders.csv");
            if (pos != std::string::npos) {
                trades_path.replace(pos, 10, "trades.csv");
                read_trade_file(trades_path);
            }
            found_orders = true;
            break;
        }
    }
    
    if (!found_orders) {
        std::cout << "ERROR: Could not find orders.csv in any of the expected locations!" << std::endl;
        std::cout << "Please copy orders.csv and trades.csv to the same directory as the executable." << std::endl;
        return 1;
    }
    
    // Check if we have data
    if (all_orders.empty()) {
        std::cout << "ERROR: No orders were loaded!" << std::endl;
        return 1;
    }
    
    if (all_trades.empty()) {
        std::cout << "WARNING: No trades were loaded!" << std::endl;
    }
    
    // 2. Rebuild order book
    rebuild_order_book();
    
    // 3. Display results
    if (bid_levels.empty() && ask_levels.empty()) {
        std::cout << "ERROR: Order book is empty after rebuild!" << std::endl;
        return 1;
    }
    
    print_order_book();
    
    // 4. Extra: Display first 5 active orders
    std::cout << "\nFirst 5 Active Orders:" << std::endl;
    std::cout << "ID\t\tDir\tPrice\tRemaining" << std::endl;
    std::cout << "--------------------------------" << std::endl;
    
    int show_count = std::min(5, (int)active_orders.size());
    for (int i = 0; i < show_count; i++) {
        std::cout << active_orders[i].order_id << "\t" 
                  << active_orders[i].direction << "\t"
                  << active_orders[i].price << "\t"
                  << active_orders[i].volume << std::endl;
    }
    
    return 0;
}
