#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanop>

// ========== 1. 定义数据结构 ==========

// 订单结构体
struct Order {
    long timestamp;      // 时间戳
    long order_id;       // 订单ID
    double price;        // 价格
    long volume;         // 数量
    char direction;      // 'B'买 / 'S'卖
    char type;           // 'A'新增 / 'C'撤单
};

// 成交结构体
struct Trade {
    long timestamp;      // 时间戳
    double price;        // 成交价格
    long volume;         // 成交数量
    long bid_order_id;   // 买方订单ID
    long ask_order_id;   // 卖方订单ID
    char direction;      // 'B'买方主动 / 'S'卖方主动
};

// 订单簿档位结构体
struct OrderBookLevel {
    double price;        // 价格
    long total_volume;   // 该价位总数量
    std::vector<long> order_ids;  // 该价位所有订单ID
};

// ========== 2. 全局数据存储 ==========

std::vector<Order> all_orders;    // 所有订单数据
std::vector<Trade> all_trades;    // 所有成交数据

// 当前活跃订单（用于快速查找）
std::vector<Order> active_orders; // 未完全成交的订单

// 订单簿数据
std::vector<OrderBookLevel> bid_levels;  // 买盘
std::vector<OrderBookLevel> ask_levels;  // 卖盘

// ========== 3. 文件读取函数 ==========

// 读取订单文件
void read_order_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "无法打开文件: " << filename << std::endl;
        return;
    }
    
    std::string line;
    std::getline(file, line);  // 跳过表头
    
    while (std::getline(file, line)) {
        // 简单的CSV解析
        std::vector<std::string> fields;
        std::string field = "";
        
        for (char c : line) {
            if (c == ',') {
                fields.push_back(field);
                field = "";
            } else {
                field += c;
            }
        }
        fields.push_back(field);  // 最后一个字段
        
        if (fields.size() >= 6) {
            Order order;
            order.timestamp = std::stol(fields[0]);
            order.order_id = std::stol(fields[1]);
            order.price = std::stod(fields[2]);
            order.volume = std::stol(fields[3]);
            order.direction = fields[4][0];
            order.type = fields[5][0];
            
            all_orders.push_back(order);
        }
    }
    
    file.close();
    std::cout << "读取了 " << all_orders.size() << " 条订单数据" << std::endl;
}

// 读取成交文件
void read_trade_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "无法打开文件: " << filename << std::endl;
        return;
    }
    
    std::string line;
    std::getline(file, line);  // 跳过表头
    
    while (std::getline(file, line)) {
        std::vector<std::string> fields;
        std::string field = "";
        
        for (char c : line) {
            if (c == ',') {
                fields.push_back(field);
                field = "";
            } else {
                field += c;
            }
        }
        fields.push_back(field);
        
        if (fields.size() >= 6) {
            Trade trade;
            trade.timestamp = std::stol(fields[0]);
            trade.price = std::stod(fields[1]);
            trade.volume = std::stol(fields[2]);
            trade.bid_order_id = std::stol(fields[3]);
            trade.ask_order_id = std::stol(fields[4]);
            trade.direction = fields[5][0];
            
            all_trades.push_back(trade);
        }
    }
    
    file.close();
    std::cout << "读取了 " << all_trades.size() << " 条成交数据" << std::endl;
}

// ========== 4. 订单处理函数 ==========

// 查找订单（按ID）
int find_active_order(long order_id) {
    for (int i = 0; i < active_orders.size(); i++) {
        if (active_orders[i].order_id == order_id) {
            return i;  // 返回索引
        }
    }
    return -1;  // 没找到
}

// 查找订单簿档位（买盘）
int find_bid_level(double price) {
    for (int i = 0; i < bid_levels.size(); i++) {
        // 价格相等（考虑浮点数误差）
        if (std::abs(bid_levels[i].price - price) < 0.0001) {
            return i;
        }
    }
    return -1;
}

// 查找订单簿档位（卖盘）
int find_ask_level(double price) {
    for (int i = 0; i < ask_levels.size(); i++) {
        if (std::abs(ask_levels[i].price - price) < 0.0001) {
            return i;
        }
    }
    return -1;
}

// 处理新增订单
void process_new_order(const Order& order) {
    // 1. 添加到活跃订单列表
    active_orders.push_back(order);
    
    // 2. 更新订单簿
    if (order.direction == 'B') {  // 买单
        int level_idx = find_bid_level(order.price);
        
        if (level_idx >= 0) {
            // 该价格已存在，增加总量
            bid_levels[level_idx].total_volume += order.volume;
            bid_levels[level_idx].order_ids.push_back(order.order_id);
        } else {
            // 新价格档位，创建并插入
            OrderBookLevel new_level;
            new_level.price = order.price;
            new_level.total_volume = order.volume;
            new_level.order_ids.push_back(order.order_id);
            
            // 插入到正确位置（买盘价格从高到低）
            int insert_pos = 0;
            while (insert_pos < bid_levels.size() && 
                   bid_levels[insert_pos].price > order.price) {
                insert_pos++;
            }
            bid_levels.insert(bid_levels.begin() + insert_pos, new_level);
        }
    } else {  // 卖单
        int level_idx = find_ask_level(order.price);
        
        if (level_idx >= 0) {
            ask_levels[level_idx].total_volume += order.volume;
            ask_levels[level_idx].order_ids.push_back(order.order_id);
        } else {
            OrderBookLevel new_level;
            new_level.price = order.price;
            new_level.total_volume = order.volume;
            new_level.order_ids.push_back(order.order_id);
            
            // 插入到正确位置（卖盘价格从低到高）
            int insert_pos = 0;
            while (insert_pos < ask_levels.size() && 
                   ask_levels[insert_pos].price < order.price) {
                insert_pos++;
            }
            ask_levels.insert(ask_levels.begin() + insert_pos, new_level);
        }
    }
}

// 处理撤单
void process_cancel_order(const Order& order) {
    int order_idx = find_active_order(order.order_id);
    if (order_idx < 0) return;  // 订单不存在
    
    Order& active_order = active_orders[order_idx];
    
    if (active_order.direction == 'B') {  // 买单撤单
        int level_idx = find_bid_level(active_order.price);
        if (level_idx >= 0) {
            // 减少总量
            bid_levels[level_idx].total_volume -= active_order.volume;
            
            // 从订单ID列表中移除
            std::vector<long>& ids = bid_levels[level_idx].order_ids;
            for (int i = 0; i < ids.size(); i++) {
                if (ids[i] == order.order_id) {
                    ids.erase(ids.begin() + i);
                    break;
                }
            }
            
            // 如果该档位没有订单了，删除档位
            if (bid_levels[level_idx].total_volume <= 0) {
                bid_levels.erase(bid_levels.begin() + level_idx);
            }
        }
    } else {  // 卖单撤单（类似）
        int level_idx = find_ask_level(active_order.price);
        if (level_idx >= 0) {
            ask_levels[level_idx].total_volume -= active_order.volume;
            
            std::vector<long>& ids = ask_levels[level_idx].order_ids;
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
    
    // 从活跃订单中移除
    active_orders.erase(active_orders.begin() + order_idx);
}

// 处理成交
void process_trade(const Trade& trade) {
    // 1. 处理买方订单
    int bid_idx = find_active_order(trade.bid_order_id);
    if (bid_idx >= 0) {
        Order& bid_order = active_orders[bid_idx];
        
        // 减少订单数量
        bid_order.volume -= trade.volume;
        
        if (bid_order.volume <= 0) {
            // 订单完全成交，从活跃订单中移除
            process_cancel_order(bid_order);  // 复用撤单逻辑
        } else {
            // 部分成交，更新订单簿
            int level_idx = find_bid_level(bid_order.price);
            if (level_idx >= 0) {
                bid_levels[level_idx].total_volume -= trade.volume;
                
                // 检查是否需要删除档位
                if (bid_levels[level_idx].total_volume <= 0) {
                    bid_levels.erase(bid_levels.begin() + level_idx);
                }
            }
        }
    }
    
    // 2. 处理卖方订单
    int ask_idx = find_active_order(trade.ask_order_id);
    if (ask_idx >= 0) {
        Order& ask_order = active_orders[ask_idx];
        
        ask_order.volume -= trade.volume;
        
        if (ask_order.volume <= 0) {
            process_cancel_order(ask_order);
        } else {
            int level_idx = find_ask_level(ask_order.price);
            if (level_idx >= 0) {
                ask_levels[level_idx].total_volume -= trade.volume;
                
                if (ask_levels[level_idx].total_volume <= 0) {
                    ask_levels.erase(ask_levels.begin() + level_idx);
                }
            }
        }
    }
}

// ========== 5. 订单簿重建主函数 ==========

void rebuild_order_book() {
    std::cout << "开始重建订单簿..." << std::endl;
    
    // 先处理所有订单
    for (const Order& order : all_orders) {
        if (order.type == 'A') {
            process_new_order(order);
        } else if (order.type == 'C') {
            process_cancel_order(order);
        }
    }
    
    // 再处理所有成交
    for (const Trade& trade : all_trades) {
        process_trade(trade);
    }
    
    std::cout << "订单簿重建完成！" << std::endl;
}

// ========== 6. 显示订单簿函数 ==========

void print_order_book(int max_levels = 5) {
    std::cout << "\n======= 订单簿快照 =======" << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    
    // 卖盘（价格从低到高）
    std::cout << "\n卖盘 (ASK):" << std::endl;
    std::cout << "价格\t\t数量\t\t订单数" << std::endl;
    std::cout << "--------------------------------" << std::endl;
    
    int ask_count = std::min((int)ask_levels.size(), max_levels);
    for (int i = 0; i < ask_count; i++) {
        std::cout << ask_levels[i].price << "\t\t" 
                  << ask_levels[i].total_volume << "\t\t"
                  << ask_levels[i].order_ids.size() << std::endl;
    }
    
    if (ask_levels.size() > max_levels) {
        std::cout << "... 还有 " << (ask_levels.size() - max_levels) << " 个卖盘档位" << std::endl;
    }
    
    // 买盘（价格从高到低）
    std::cout << "\n买盘 (BID):" << std::endl;
    std::cout << "价格\t\t数量\t\t订单数" << std::endl;
    std::cout << "--------------------------------" << std::endl;
    
    int bid_count = std::min((int)bid_levels.size(), max_levels);
    for (int i = 0; i < bid_count; i++) {
        std::cout << bid_levels[i].price << "\t\t" 
                  << bid_levels[i].total_volume << "\t\t"
                  << bid_levels[i].order_ids.size() << std::endl;
    }
    
    if (bid_levels.size() > max_levels) {
        std::cout << "... 还有 " << (bid_levels.size() - max_levels) << " 个买盘档位" << std::endl;
    }
    
    // 统计信息
    std::cout << "\n统计信息:" << std::endl;
    std::cout << "活跃订单数: " << active_orders.size() << std::endl;
    std::cout << "买盘档位数: " << bid_levels.size() << std::endl;
    std::cout << "卖盘档位数: " << ask_levels.size() << std::endl;
    
    // 显示最佳买卖价
    if (!bid_levels.empty() && !ask_levels.empty()) {
        double bid_price = bid_levels[0].price;
        double ask_price = ask_levels[0].price;
        double spread = ask_price - bid_price;
        
        std::cout << "\n最佳报价:" << std::endl;
        std::cout << "买一价: " << bid_price << " (" << bid_levels[0].total_volume << "股)" << std::endl;
        std::cout << "卖一价: " << ask_price << " (" << ask_levels[0].total_volume << "股)" << std::endl;
        std::cout << "买卖价差: " << spread << std::endl;
    }
}

// ========== 7. 主函数 ==========

int main() {
    // 1. 读取数据
    read_order_file("orders.csv");
    read_trade_file("trades.csv");
    
    // 2. 重建订单簿
    rebuild_order_book();
    
    // 3. 显示结果
    print_order_book();
    
    // 4. 额外功能：显示前5个活跃订单
    std::cout << "\n前5个活跃订单:" << std::endl;
    std::cout << "ID\t\t方向\t价格\t剩余数量" << std::endl;
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