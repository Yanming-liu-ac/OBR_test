#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <iomanip>
#include <cmath>

// Order structure
struct Order {
    long long clockatarrival;
    int sequenceno;
    long long transacttime;
    int applseqnum;
    int side;                // 1=buy, 2=sell
    char ordertype;          // '1'=market, '2'=limit, 'u'=best
    double price;
    int orderqty;
};

// Trade structure
struct Trade {
    long long clockatarrival;
    int sequenceno;
    long long transacttime;
    int applseqnum;
    char exectype;           // 'f'=filled, '4'=cancelled
    double tradeprice;
    int tradeqty;
    double trademoney;
    int bidapplseqnum;
    int offerapplseqnum;
};

// Order book snapshot structure
struct BookSnapshot {
    long long clockatarrival;
    long long transacttime;
    std::vector<std::pair<double, int> > best_bids;
    std::vector<std::pair<double, int> > best_asks;
    std::vector<std::pair<double, int> > worst_bids;
    std::vector<std::pair<double, int> > worst_asks;
    
    // Additional market statistics
    long long cvl;          // Cumulative Volume: total traded volume
    double lpr;             // Last Price: most recent trade price
    int cto;                // Cumulative Trade Orders: total number of orders that traded
    int nts;                // Number of Trades: total number of trades executed
    double opx;             // Opening Price: first trade price of the session
};

// Order in book
struct BookOrder {
    int applseqnum;
    double price;
    int qty;
    long long order_time;
};

// Order book structure
struct OrderBook {
    std::map<int, BookOrder> bids;
    std::map<int, BookOrder> asks;
    std::vector<BookSnapshot> snapshots;
    
    // Market statistics
    long long cumulative_volume;
    double last_price;
    int cumulative_trade_orders;
    int number_of_trades;
    double opening_price;
    bool has_opening_price;
};

// Event structure
struct Event {
    std::string type;
    long long time;
    int index;
};

// Initialize order book
void init_orderbook(OrderBook& book) {
    book.cumulative_volume = 0;
    book.last_price = 0.0;
    book.cumulative_trade_orders = 0;
    book.number_of_trades = 0;
    book.opening_price = 0.0;
    book.has_opening_price = false;
}

// Get best bid price
double get_best_bid_price(const OrderBook& book) {
    if (book.bids.empty()) return 0;
    double best = 0;
    
    std::map<int, BookOrder>::const_iterator it;
    for (it = book.bids.begin(); it != book.bids.end(); ++it) {
        if (it->second.price > best) {
            best = it->second.price;
        }
    }
    return best;
}

// Get best ask price
double get_best_ask_price(const OrderBook& book) {
    if (book.asks.empty()) return 0;
    double best = 1e9;
    
    std::map<int, BookOrder>::const_iterator it;
    for (it = book.asks.begin(); it != book.asks.end(); ++it) {
        if (it->second.price < best) {
            best = it->second.price;
        }
    }
    return best == 1e9 ? 0 : best;
}

// Add order to book
void add_order(OrderBook& book, const Order& order) {
    if (order.orderqty <= 0) return;
    
    BookOrder book_order;
    book_order.applseqnum = order.applseqnum;
    book_order.price = order.price;
    book_order.qty = order.orderqty;
    book_order.order_time = order.transacttime;
    
    // Handle market and best orders
    if (order.ordertype == '1') {  // Market order
        if (order.side == 1) {  // Buy: use best ask
            double best_ask = get_best_ask_price(book);
            if (best_ask > 0) {
                book_order.price = best_ask;
            } else {
                return;
            }
        } else {  // Sell: use best bid
            double best_bid = get_best_bid_price(book);
            if (best_bid > 0) {
                book_order.price = best_bid;
            } else {
                return;
            }
        }
    } else if (order.ordertype == 'u') {  // Best order
        if (order.side == 1) {  // Buy: use current best bid
            double best_bid = get_best_bid_price(book);
            if (best_bid > 0) {
                book_order.price = best_bid;
            } else {
                return;
            }
        } else {  // Sell: use current best ask
            double best_ask = get_best_ask_price(book);
            if (best_ask > 0) {
                book_order.price = best_ask;
            } else {
                return;
            }
        }
    }
    
    // Add to order book
    if (order.side == 1) {
        book.bids[order.applseqnum] = book_order;
    } else {
        book.asks[order.applseqnum] = book_order;
    }
}

// Execute trade
void execute_trade(OrderBook& book, const Trade& trade) {
    if (trade.exectype == 'f') {  // Filled
        // Update market statistics
        book.cumulative_volume += trade.tradeqty;
        book.last_price = trade.tradeprice;
        book.number_of_trades++;
        
        // Set opening price if this is the first trade
        if (!book.has_opening_price) {
            book.opening_price = trade.tradeprice;
            book.has_opening_price = true;
        }
        
        // Track orders that participated in trade
        if (trade.bidapplseqnum != 0) {
            book.cumulative_trade_orders++;
        }
        if (trade.offerapplseqnum != 0) {
            book.cumulative_trade_orders++;
        }
        
        // Update order book
        if (trade.bidapplseqnum != 0) {
            std::map<int, BookOrder>::iterator it = book.bids.find(trade.bidapplseqnum);
            if (it != book.bids.end()) {
                it->second.qty -= trade.tradeqty;
                if (it->second.qty <= 0) {
                    book.bids.erase(it);
                }
            }
        }
        
        if (trade.offerapplseqnum != 0) {
            std::map<int, BookOrder>::iterator it = book.asks.find(trade.offerapplseqnum);
            if (it != book.asks.end()) {
                it->second.qty -= trade.tradeqty;
                if (it->second.qty <= 0) {
                    book.asks.erase(it);
                }
            }
        }
    } else if (trade.exectype == '4') {  // Cancelled
        if (trade.bidapplseqnum != 0) {
            book.bids.erase(trade.bidapplseqnum);
        }
        if (trade.offerapplseqnum != 0) {
            book.asks.erase(trade.offerapplseqnum);
        }
    }
}

// Comparison function for sorting bids (high to low)
bool compare_bids_desc(const std::pair<double, int>& a, const std::pair<double, int>& b) {
    return a.first > b.first;
}

// Comparison function for sorting asks (low to high)
bool compare_asks_asc(const std::pair<double, int>& a, const std::pair<double, int>& b) {
    return a.first < b.first;
}

// Get top bids
std::vector<std::pair<double, int> > get_top_bids(const OrderBook& book, int n) {
    std::map<double, int> price_levels;
    std::map<int, BookOrder>::const_iterator it;
    
    for (it = book.bids.begin(); it != book.bids.end(); ++it) {
        price_levels[it->second.price] += it->second.qty;
    }
    
    std::vector<std::pair<double, int> > result;
    std::map<double, int>::iterator level_it;
    for (level_it = price_levels.begin(); level_it != price_levels.end(); ++level_it) {
        result.push_back(*level_it);
    }
    
    std::sort(result.begin(), result.end(), compare_bids_desc);
    
    if (result.size() > (size_t)n) {
        result.resize(n);
    }
    return result;
}

// Get top asks
std::vector<std::pair<double, int> > get_top_asks(const OrderBook& book, int n) {
    std::map<double, int> price_levels;
    std::map<int, BookOrder>::const_iterator it;
    
    for (it = book.asks.begin(); it != book.asks.end(); ++it) {
        price_levels[it->second.price] += it->second.qty;
    }
    
    std::vector<std::pair<double, int> > result;
    std::map<double, int>::iterator level_it;
    for (level_it = price_levels.begin(); level_it != price_levels.end(); ++level_it) {
        result.push_back(*level_it);
    }
    
    std::sort(result.begin(), result.end(), compare_asks_asc);
    
    if (result.size() > (size_t)n) {
        result.resize(n);
    }
    return result;
}

// Get bottom bids (lowest prices)
std::vector<std::pair<double, int> > get_bottom_bids(const OrderBook& book, int n) {
    std::map<double, int> price_levels;
    std::map<int, BookOrder>::const_iterator it;
    
    for (it = book.bids.begin(); it != book.bids.end(); ++it) {
        price_levels[it->second.price] += it->second.qty;
    }
    
    std::vector<std::pair<double, int> > result;
    std::map<double, int>::iterator level_it;
    for (level_it = price_levels.begin(); level_it != price_levels.end(); ++level_it) {
        result.push_back(*level_it);
    }
    
    std::sort(result.begin(), result.end(), compare_asks_asc);  // Low to high
    
    if (result.size() > (size_t)n) {
        result.resize(n);
    }
    return result;
}

// Get bottom asks (highest prices)
std::vector<std::pair<double, int> > get_bottom_asks(const OrderBook& book, int n) {
    std::map<double, int> price_levels;
    std::map<int, BookOrder>::const_iterator it;
    
    for (it = book.asks.begin(); it != book.asks.end(); ++it) {
        price_levels[it->second.price] += it->second.qty;
    }
    
    std::vector<std::pair<double, int> > result;
    std::map<double, int>::iterator level_it;
    for (level_it = price_levels.begin(); level_it != price_levels.end(); ++level_it) {
        result.push_back(*level_it);
    }
    
    std::sort(result.begin(), result.end(), compare_bids_desc);  // High to low
    
    if (result.size() > (size_t)n) {
        result.resize(n);
    }
    return result;
}

// Take snapshot
void take_snapshot(OrderBook& book, long long clockatarrival, long long transacttime) {
    BookSnapshot snapshot;
    snapshot.clockatarrival = clockatarrival;
    snapshot.transacttime = transacttime;
    snapshot.best_bids = get_top_bids(book, 5);
    snapshot.best_asks = get_top_asks(book, 5);
    snapshot.worst_bids = get_bottom_bids(book, 5);
    snapshot.worst_asks = get_bottom_asks(book, 5);
    
    // Add market statistics
    snapshot.cvl = book.cumulative_volume;
    snapshot.lpr = book.last_price;
    snapshot.cto = book.cumulative_trade_orders;
    snapshot.nts = book.number_of_trades;
    snapshot.opx = book.opening_price;
    
    book.snapshots.push_back(snapshot);
}

// Read orders
void read_order_file(const std::string& filename, std::vector<Order>& orders) {
    std::ifstream file(filename.c_str());
    
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
        if (line.empty()) continue;
        
        try {
            std::vector<std::string> fields;
            std::string field = "";
            
            for (size_t i = 0; i < line.length(); i++) {
                char c = line[i];
                if (c == ',' || c == '\r') {
                    if (c == ',') {
                        fields.push_back(field);
                        field = "";
                    }
                } else {
                    field += c;
                }
            }
            fields.push_back(field);  // Last field
            
            if (fields.size() >= 8) {
                Order order;
                order.clockatarrival = std::atoll(fields[0].c_str());
                order.sequenceno = std::atoi(fields[1].c_str());
                order.transacttime = std::atoll(fields[2].c_str());
                order.applseqnum = std::atoi(fields[3].c_str());
                order.side = std::atoi(fields[4].c_str());
                order.ordertype = fields[5][0];
                order.price = std::atof(fields[6].c_str());
                order.orderqty = std::atoi(fields[7].c_str());
                
                orders.push_back(order);
            } else {
                std::cout << "Warning: Line " << line_num << " has only " << fields.size() << " fields" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "Error parsing line " << line_num << ": " << e.what() << std::endl;
            std::cout << "Line content: " << line << std::endl;
        }
    }
    
    file.close();
    std::cout << "Read " << orders.size() << " orders" << std::endl;
}

// Read trades
void read_trade_file(const std::string& filename, std::vector<Trade>& trades) {
    std::ifstream file(filename.c_str());
    
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
        if (line.empty()) continue;
        
        try {
            std::vector<std::string> fields;
            std::string field = "";
            
            for (size_t i = 0; i < line.length(); i++) {
                char c = line[i];
                if (c == ',' || c == '\r') {
                    if (c == ',') {
                        fields.push_back(field);
                        field = "";
                    }
                } else {
                    field += c;
                }
            }
            fields.push_back(field);  // Last field
            
            if (fields.size() >= 10) {
                Trade trade;
                trade.clockatarrival = std::atoll(fields[0].c_str());
                trade.sequenceno = std::atoi(fields[1].c_str());
                trade.transacttime = std::atoll(fields[2].c_str());
                trade.applseqnum = std::atoi(fields[3].c_str());
                trade.exectype = fields[4][0];
                trade.tradeprice = std::atof(fields[5].c_str());
                trade.tradeqty = std::atoi(fields[6].c_str());
                trade.trademoney = std::atof(fields[7].c_str());
                trade.bidapplseqnum = std::atoi(fields[8].c_str());
                trade.offerapplseqnum = std::atoi(fields[9].c_str());
                
                trades.push_back(trade);
            } else {
                std::cout << "Warning: Line " << line_num << " has only " << fields.size() << " fields" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "Error parsing line " << line_num << ": " << e.what() << std::endl;
            std::cout << "Line content: " << line << std::endl;
        }
    }
    
    file.close();
    std::cout << "Read " << trades.size() << " trades" << std::endl;
}

// Comparison function for sorting events
bool compare_events(const Event& a, const Event& b) {
    if (a.time != b.time) return a.time < b.time;
    return a.type == "order" && b.type == "trade";
}

// Process events
void process_events(const std::vector<Order>& orders, 
                   const std::vector<Trade>& trades,
                   const std::string& output_file) {
    OrderBook book;
    init_orderbook(book);
    
    // Define opening time (9:30:00)
    const long long OPENING_TIME = 93000000;
    bool market_opened = false;
    
    // Build a map of order applseqnum to immediate trades
    std::map<int, bool> order_has_immediate_trade;
    for (size_t i = 0; i < trades.size(); i++) {
        if (trades[i].exectype == 'f') {
            for (size_t j = 0; j < orders.size(); j++) {
                if ((orders[j].applseqnum == trades[i].bidapplseqnum || 
                     orders[j].applseqnum == trades[i].offerapplseqnum) &&
                    abs(orders[j].transacttime - trades[i].transacttime) <= 1000) {
                    order_has_immediate_trade[orders[j].applseqnum] = true;
                }
            }
        }
    }
    
    std::vector<Event> events;
    
    for (size_t i = 0; i < orders.size(); i++) {
        Event e;
        e.type = "order";
        e.time = orders[i].transacttime;
        e.index = i;
        events.push_back(e);
    }
    
    for (size_t i = 0; i < trades.size(); i++) {
        Event e;
        e.type = "trade";
        e.time = trades[i].transacttime;
        e.index = i;
        events.push_back(e);
    }
    
    std::sort(events.begin(), events.end(), compare_events);
    
    for (size_t i = 0; i < events.size(); i++) {
        if (events[i].type == "order") {
            const Order& order = orders[events[i].index];
            
            bool is_immediate_market_order = 
                (order.ordertype == '1' || order.ordertype == 'u') && 
                order_has_immediate_trade.count(order.applseqnum) > 0;
            
            if (order.transacttime < OPENING_TIME || !is_immediate_market_order) {
                add_order(book, order);
            }
            
            if (order.transacttime >= OPENING_TIME && !is_immediate_market_order) {
                if (!market_opened) {
                    std::cout << "Market opened! Taking first snapshot..." << std::endl;
                    market_opened = true;
                }
                take_snapshot(book, order.clockatarrival, order.transacttime);
            }
        } else {
            const Trade& trade = trades[events[i].index];
            execute_trade(book, trade);
            take_snapshot(book, trade.clockatarrival, trade.transacttime);
        }
    }
    
    std::ofstream out(output_file.c_str());
    if (!out.is_open()) {
        std::cerr << "Cannot create output file: " << output_file << std::endl;
        return;
    }
    
    out << "clockatarrival,transacttime,"
        << "best_bid_1_price,best_bid_1_qty,best_bid_2_price,best_bid_2_qty,"
        << "best_bid_3_price,best_bid_3_qty,best_bid_4_price,best_bid_4_qty,"
        << "best_bid_5_price,best_bid_5_qty,"
        << "best_ask_1_price,best_ask_1_qty,best_ask_2_price,best_ask_2_qty,"
        << "best_ask_3_price,best_ask_3_qty,best_ask_4_price,best_ask_4_qty,"
        << "best_ask_5_price,best_ask_5_qty,"
        << "worst_bid_1_price,worst_bid_1_qty,worst_bid_2_price,worst_bid_2_qty,"
        << "worst_bid_3_price,worst_bid_3_qty,worst_bid_4_price,worst_bid_4_qty,"
        << "worst_bid_5_price,worst_bid_5_qty,"
        << "worst_ask_1_price,worst_ask_1_qty,worst_ask_2_price,worst_ask_2_qty,"
        << "worst_ask_3_price,worst_ask_3_qty,worst_ask_4_price,worst_ask_4_qty,"
        << "worst_ask_5_price,worst_ask_5_qty,"
        << "cvl,lpr,cto,nts,opx\n";
    
    for (size_t snap_idx = 0; snap_idx < book.snapshots.size(); snap_idx++) {
        const BookSnapshot& snapshot = book.snapshots[snap_idx];
        out << snapshot.clockatarrival << "," << snapshot.transacttime;
        
        for (int i = 0; i < 5; i++) {
            if (i < (int)snapshot.best_bids.size()) {
                out << "," << std::fixed << std::setprecision(2) 
                    << snapshot.best_bids[i].first << "," << snapshot.best_bids[i].second;
            } else {
                out << ",,";
            }
        }
        
        for (int i = 0; i < 5; i++) {
            if (i < (int)snapshot.best_asks.size()) {
                out << "," << std::fixed << std::setprecision(2)
                    << snapshot.best_asks[i].first << "," << snapshot.best_asks[i].second;
            } else {
                out << ",,";
            }
        }
        
        for (int i = 0; i < 5; i++) {
            if (i < (int)snapshot.worst_bids.size()) {
                out << "," << std::fixed << std::setprecision(2)
                    << snapshot.worst_bids[i].first << "," << snapshot.worst_bids[i].second;
            } else {
                out << ",,";
            }
        }
        
        for (int i = 0; i < 5; i++) {
            if (i < (int)snapshot.worst_asks.size()) {
                out << "," << std::fixed << std::setprecision(2)
                    << snapshot.worst_asks[i].first << "," << snapshot.worst_asks[i].second;
            } else {
                out << ",,";
            }
        }
        
        out << "," << snapshot.cvl
            << "," << std::fixed << std::setprecision(2) << snapshot.lpr
            << "," << snapshot.cto
            << "," << snapshot.nts
            << "," << std::fixed << std::setprecision(2) << snapshot.opx;
        
        out << "\n";
    }
    
    out.close();
    std::cout << "Order book snapshots saved to " << output_file << std::endl;
    std::cout << "Total snapshots: " << book.snapshots.size() << std::endl;
}

int main() {
    std::cout << "========== Order Book Reconstruction ==========" << std::endl;
    
    std::vector<std::string> paths_to_try;
    paths_to_try.push_back("order_new.csv");
    paths_to_try.push_back("../order_new.csv");
    paths_to_try.push_back("../../order_new.csv");
    paths_to_try.push_back("../../../order_new.csv");
    paths_to_try.push_back("../../../../order_new.csv");
    
    std::string order_path;
    std::string trade_path;
    std::string output_path;
    
    bool found = false;
    for (size_t i = 0; i < paths_to_try.size(); i++) {
        std::ifstream test(paths_to_try[i].c_str());
        if (test.good()) {
            test.close();
            order_path = paths_to_try[i];
            
            trade_path = paths_to_try[i];
            size_t pos = trade_path.find("order_new.csv");
            if (pos != std::string::npos) {
                trade_path.replace(pos, 13, "trade_new.csv");
            }
            
            output_path = paths_to_try[i];
            pos = output_path.find("order_new.csv");
            if (pos != std::string::npos) {
                output_path.replace(pos, 13, "book_new.csv");
            }
            
            std::cout << "Found files at: " << paths_to_try[i] << std::endl;
            found = true;
            break;
        }
    }
    
    if (!found) {
        std::cerr << "Error: Could not find order_new.csv in any expected location!" << std::endl;
        return 1;
    }
    
    std::vector<Order> orders;
    std::vector<Trade> trades;
    
    read_order_file(order_path, orders);
    read_trade_file(trade_path, trades);
    
    if (orders.empty()) {
        std::cerr << "Error: No orders loaded!" << std::endl;
        return 1;
    }
    
    process_events(orders, trades, output_path);
    
    std::cout << "Processing complete!" << std::endl;
    std::cout << "Output saved to: " << output_path << std::endl;
    
    return 0;
}
