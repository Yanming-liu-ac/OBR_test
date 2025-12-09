import csv
from collections import defaultdict
from typing import Dict, List, Tuple
import heapq

class OrderBook:
    def __init__(self):
        # 买单：价格从高到低（使用负价格实现最大堆）
        self.bids = {}  # {applseqnum: {'price': float, 'qty': int, 'time': int}}
        # 卖单：价格从低到高
        self.asks = {}  # {applseqnum: {'price': float, 'qty': int, 'time': int}}
        
        # 用于记录每次快照
        self.snapshots = []
        
    def add_order(self, applseqnum, side, ordertype, price, qty, clockatarrival, transacttime):
        """添加订单到订单簿"""
        if qty <= 0:
            return
            
        order = {
            'price': price,
            'qty': qty,
            'time': transacttime,
            'clockatarrival': clockatarrival,
            'applseqnum': applseqnum
        }
        
        if side == 1:  # 买单
            # 市价单：使用当前最优卖价
            if ordertype == 1:  # 市价单
                if self.asks:
                    best_ask = min(self.asks.values(), key=lambda x: (x['price'], x['time']))
                    order['price'] = best_ask['price']
                else:
                    return  # 没有对手价，市价单无法成交
            # 最优单：使用当前最优买价
            elif ordertype == 'u':
                if self.bids:
                    best_bid = max(self.bids.values(), key=lambda x: (x['price'], -x['time']))
                    order['price'] = best_bid['price']
                else:
                    return  # 没有最优买价
                    
            self.bids[applseqnum] = order
        else:  # 卖单 side == 2
            # 市价单：使用当前最优买价
            if ordertype == 1:
                if self.bids:
                    best_bid = max(self.bids.values(), key=lambda x: (x['price'], -x['time']))
                    order['price'] = best_bid['price']
                else:
                    return  # 没有对手价
            # 最优单：使用当前最优卖价
            elif ordertype == 'u':
                if self.asks:
                    best_ask = min(self.asks.values(), key=lambda x: (x['price'], x['time']))
                    order['price'] = best_ask['price']
                else:
                    return  # 没有最优卖价
                    
            self.asks[applseqnum] = order
    
    def execute_trade(self, exectype, tradeprice, tradeqty, bidapplseqnum, offerapplseqnum):
        """执行交易或撤单"""
        if exectype == 'f':  # 交易成功
            # 减少买单数量
            if bidapplseqnum != 0 and bidapplseqnum in self.bids:
                self.bids[bidapplseqnum]['qty'] -= tradeqty
                if self.bids[bidapplseqnum]['qty'] <= 0:
                    del self.bids[bidapplseqnum]
            
            # 减少卖单数量
            if offerapplseqnum != 0 and offerapplseqnum in self.asks:
                self.asks[offerapplseqnum]['qty'] -= tradeqty
                if self.asks[offerapplseqnum]['qty'] <= 0:
                    del self.asks[offerapplseqnum]
                    
        elif exectype == '4':  # 撤单
            # 撤销买单
            if bidapplseqnum != 0 and bidapplseqnum in self.bids:
                del self.bids[bidapplseqnum]
            # 撤销卖单
            if offerapplseqnum != 0 and offerapplseqnum in self.asks:
                del self.asks[offerapplseqnum]
    
    def get_top_levels(self, side, n=5):
        """获取最优n档价位"""
        if side == 'bid':
            if not self.bids:
                return []
            # 按价格降序，时间升序排列
            sorted_bids = sorted(self.bids.values(), 
                               key=lambda x: (-x['price'], x['time']))
            # 合并相同价格的订单
            price_levels = defaultdict(int)
            for order in sorted_bids:
                price_levels[order['price']] += order['qty']
            # 取前n档
            top_n = sorted(price_levels.items(), key=lambda x: -x[0])[:n]
            return top_n
        else:  # ask
            if not self.asks:
                return []
            # 按价格升序，时间升序排列
            sorted_asks = sorted(self.asks.values(), 
                               key=lambda x: (x['price'], x['time']))
            # 合并相同价格的订单
            price_levels = defaultdict(int)
            for order in sorted_asks:
                price_levels[order['price']] += order['qty']
            # 取前n档
            top_n = sorted(price_levels.items(), key=lambda x: x[0])[:n]
            return top_n
    
    def get_bottom_levels(self, side, n=5):
        """获取最差n档价位"""
        if side == 'bid':
            if not self.bids:
                return []
            # 按价格升序，时间升序排列（最差的买价）
            sorted_bids = sorted(self.bids.values(), 
                               key=lambda x: (x['price'], x['time']))
            # 合并相同价格的订单
            price_levels = defaultdict(int)
            for order in sorted_bids:
                price_levels[order['price']] += order['qty']
            # 取前n档（最低价格）
            bottom_n = sorted(price_levels.items(), key=lambda x: x[0])[:n]
            return bottom_n
        else:  # ask
            if not self.asks:
                return []
            # 按价格降序，时间升序排列（最差的卖价）
            sorted_asks = sorted(self.asks.values(), 
                               key=lambda x: (-x['price'], x['time']))
            # 合并相同价格的订单
            price_levels = defaultdict(int)
            for order in sorted_asks:
                price_levels[order['price']] += order['qty']
            # 取前n档（最高价格）
            bottom_n = sorted(price_levels.items(), key=lambda x: -x[0])[:n]
            return bottom_n
    
    def take_snapshot(self, clockatarrival, transacttime):
        """记录当前订单簿状态"""
        snapshot = {
            'clockatarrival': clockatarrival,
            'transacttime': transacttime,
            'top_5_bids': self.get_top_levels('bid', 5),
            'top_5_asks': self.get_top_levels('ask', 5),
            'bottom_5_bids': self.get_bottom_levels('bid', 5),
            'bottom_5_asks': self.get_bottom_levels('ask', 5)
        }
        self.snapshots.append(snapshot)
        return snapshot


def process_orderbook(order_file, trade_file, output_file):
    """处理订单和交易数据，生成订单簿快照"""
    orderbook = OrderBook()
    
    # 读取订单数据
    orders = []
    with open(order_file, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        for row in reader:
            orders.append({
                'clockatarrival': int(row['clockatarrival']),
                'sequenceno': int(row['sequenceno']),
                'transacttime': int(row['transacttime']),
                'applseqnum': int(row['applseqnum']),
                'side': int(row['side']),
                'ordertype': row['ordertype'] if row['ordertype'] == 'u' else int(row['ordertype']),
                'price': float(row['price']),
                'orderqty': int(row['orderqty'])
            })
    
    # 读取交易数据
    trades = []
    with open(trade_file, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f)
        for row in reader:
            trades.append({
                'clockatarrival': int(row['clockatarrival']),
                'sequenceno': int(row['sequenceno']),
                'transacttime': int(row['transacttime']),
                'applseqnum': int(row['applseqnum']),
                'exectype': row['exectype'],
                'tradeprice': float(row['tradeprice']),
                'tradeqty': int(row['tradeqty']),
                'trademoney': float(row['trademoney']),
                'bidapplseqnum': int(row['bidapplseqnum']),
                'offerapplseqnum': int(row['offerapplseqnum'])
            })
    
    # 合并所有事件并按时间排序
    events = []
    for order in orders:
        events.append(('order', order['transacttime'], order))
    for trade in trades:
        events.append(('trade', trade['transacttime'], trade))
    
    events.sort(key=lambda x: (x[1], x[0] == 'trade'))  # 先按时间，同一时间订单先于交易
    
    # 处理事件
    for event_type, _, data in events:
        if event_type == 'order':
            orderbook.add_order(
                data['applseqnum'],
                data['side'],
                data['ordertype'],
                data['price'],
                data['orderqty'],
                data['clockatarrival'],
                data['transacttime']
            )
            # 每个订单后拍快照
            orderbook.take_snapshot(data['clockatarrival'], data['transacttime'])
        else:  # trade
            orderbook.execute_trade(
                data['exectype'],
                data['tradeprice'],
                data['tradeqty'],
                data['bidapplseqnum'],
                data['offerapplseqnum']
            )
            # 每笔交易后拍快照
            orderbook.take_snapshot(data['clockatarrival'], data['transacttime'])
    
    # 写入输出文件
    with open(output_file, 'w', encoding='utf-8', newline='') as f:
        fieldnames = [
            'clockatarrival', 'transacttime',
            'best_bid_1_price', 'best_bid_1_qty',
            'best_bid_2_price', 'best_bid_2_qty',
            'best_bid_3_price', 'best_bid_3_qty',
            'best_bid_4_price', 'best_bid_4_qty',
            'best_bid_5_price', 'best_bid_5_qty',
            'best_ask_1_price', 'best_ask_1_qty',
            'best_ask_2_price', 'best_ask_2_qty',
            'best_ask_3_price', 'best_ask_3_qty',
            'best_ask_4_price', 'best_ask_4_qty',
            'best_ask_5_price', 'best_ask_5_qty',
            'worst_bid_1_price', 'worst_bid_1_qty',
            'worst_bid_2_price', 'worst_bid_2_qty',
            'worst_bid_3_price', 'worst_bid_3_qty',
            'worst_bid_4_price', 'worst_bid_4_qty',
            'worst_bid_5_price', 'worst_bid_5_qty',
            'worst_ask_1_price', 'worst_ask_1_qty',
            'worst_ask_2_price', 'worst_ask_2_qty',
            'worst_ask_3_price', 'worst_ask_3_qty',
            'worst_ask_4_price', 'worst_ask_4_qty',
            'worst_ask_5_price', 'worst_ask_5_qty'
        ]
        
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        
        for snapshot in orderbook.snapshots:
            row = {
                'clockatarrival': snapshot['clockatarrival'],
                'transacttime': snapshot['transacttime']
            }
            
            # 最优买价（从高到低）
            for i in range(5):
                if i < len(snapshot['top_5_bids']):
                    row[f'best_bid_{i+1}_price'] = snapshot['top_5_bids'][i][0]
                    row[f'best_bid_{i+1}_qty'] = snapshot['top_5_bids'][i][1]
                else:
                    row[f'best_bid_{i+1}_price'] = ''
                    row[f'best_bid_{i+1}_qty'] = ''
            
            # 最优卖价（从低到高）
            for i in range(5):
                if i < len(snapshot['top_5_asks']):
                    row[f'best_ask_{i+1}_price'] = snapshot['top_5_asks'][i][0]
                    row[f'best_ask_{i+1}_qty'] = snapshot['top_5_asks'][i][1]
                else:
                    row[f'best_ask_{i+1}_price'] = ''
                    row[f'best_ask_{i+1}_qty'] = ''
            
            # 最差买价（从低到高）
            for i in range(5):
                if i < len(snapshot['bottom_5_bids']):
                    row[f'worst_bid_{i+1}_price'] = snapshot['bottom_5_bids'][i][0]
                    row[f'worst_bid_{i+1}_qty'] = snapshot['bottom_5_bids'][i][1]
                else:
                    row[f'worst_bid_{i+1}_price'] = ''
                    row[f'worst_bid_{i+1}_qty'] = ''
            
            # 最差卖价（从高到低）
            for i in range(5):
                if i < len(snapshot['bottom_5_asks']):
                    row[f'worst_ask_{i+1}_price'] = snapshot['bottom_5_asks'][i][0]
                    row[f'worst_ask_{i+1}_qty'] = snapshot['bottom_5_asks'][i][1]
                else:
                    row[f'worst_ask_{i+1}_price'] = ''
                    row[f'worst_ask_{i+1}_qty'] = ''
            
            writer.writerow(row)
    
    print(f"订单簿快照已保存到 {output_file}")
    print(f"总共生成了 {len(orderbook.snapshots)} 个快照")


if __name__ == '__main__':
    process_orderbook('order_new.csv', 'trade_new.csv', 'book_new.csv')

