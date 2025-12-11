import csv
from typing import List, Dict, Tuple

# 初始化订单簿
def init_orderbook():
    return {
        'bids': {},  # {applseqnum: {price, qty, order_time}}
        'asks': {},  # {applseqnum: {price, qty, order_time}}
        'snapshots': [],
        'cumulative_volume': 0,
        'last_price': 0.0,
        'cumulative_trade_orders': 0,
        'number_of_trades': 0,
        'opening_price': 0.0,
        'has_opening_price': False
    }

# 获取最佳买价
def get_best_bid_price(book):
    if not book['bids']:
        return 0
    return max(order['price'] for order in book['bids'].values())

# 获取最佳卖价
def get_best_ask_price(book):
    if not book['asks']:
        return 0
    return min(order['price'] for order in book['asks'].values())

# 添加订单到订单簿
def add_order(book, order):
    if order['orderqty'] <= 0:
        return
    
    book_order = {
        'applseqnum': order['applseqnum'],
        'price': order['price'],
        'qty': order['orderqty'],
        'order_time': order['transacttime']
    }
    
    # 处理市价单和最优价单
    if order['ordertype'] == '1':  # 市价单
        if order['side'] == 1:  # 买入：使用最佳卖价
            best_ask = get_best_ask_price(book)
            if best_ask > 0:
                book_order['price'] = best_ask
            else:
                return
        else:  # 卖出：使用最佳买价
            best_bid = get_best_bid_price(book)
            if best_bid > 0:
                book_order['price'] = best_bid
            else:
                return
    elif order['ordertype'] == 'u':  # 最优价单
        if order['side'] == 1:  # 买入：使用当前最佳买价
            best_bid = get_best_bid_price(book)
            if best_bid > 0:
                book_order['price'] = best_bid
            else:
                return
        else:  # 卖出：使用当前最佳卖价
            best_ask = get_best_ask_price(book)
            if best_ask > 0:
                book_order['price'] = best_ask
            else:
                return
    
    # 添加到订单簿
    if order['side'] == 1:
        book['bids'][order['applseqnum']] = book_order
    else:
        book['asks'][order['applseqnum']] = book_order

# 执行交易
def execute_trade(book, trade):
    if trade['exectype'] == 'f':  # 成交
        # 更新市场统计
        book['cumulative_volume'] += trade['tradeqty']
        book['last_price'] = trade['tradeprice']
        book['number_of_trades'] += 1
        
        # 设置开盘价（如果这是第一笔交易）
        if not book['has_opening_price']:
            book['opening_price'] = trade['tradeprice']
            book['has_opening_price'] = True
        
        # 追踪参与交易的订单
        if trade['bidapplseqnum'] != 0:
            book['cumulative_trade_orders'] += 1
        if trade['offerapplseqnum'] != 0:
            book['cumulative_trade_orders'] += 1
        
        # 更新订单簿
        if trade['bidapplseqnum'] != 0 and trade['bidapplseqnum'] in book['bids']:
            book['bids'][trade['bidapplseqnum']]['qty'] -= trade['tradeqty']
            if book['bids'][trade['bidapplseqnum']]['qty'] <= 0:
                del book['bids'][trade['bidapplseqnum']]
        
        if trade['offerapplseqnum'] != 0 and trade['offerapplseqnum'] in book['asks']:
            book['asks'][trade['offerapplseqnum']]['qty'] -= trade['tradeqty']
            if book['asks'][trade['offerapplseqnum']]['qty'] <= 0:
                del book['asks'][trade['offerapplseqnum']]
    
    elif trade['exectype'] == '4':  # 撤单
        if trade['bidapplseqnum'] != 0 and trade['bidapplseqnum'] in book['bids']:
            del book['bids'][trade['bidapplseqnum']]
        if trade['offerapplseqnum'] != 0 and trade['offerapplseqnum'] in book['asks']:
            del book['asks'][trade['offerapplseqnum']]

# 获取前N档买单（从高到低）
def get_top_bids(book, n):
    price_levels = {}
    for order in book['bids'].values():
        price = order['price']
        if price not in price_levels:
            price_levels[price] = 0
        price_levels[price] += order['qty']
    
    result = [(price, qty) for price, qty in price_levels.items()]
    result.sort(key=lambda x: x[0], reverse=True)  # 从高到低
    
    return result[:n]

# 获取前N档卖单（从低到高）
def get_top_asks(book, n):
    price_levels = {}
    for order in book['asks'].values():
        price = order['price']
        if price not in price_levels:
            price_levels[price] = 0
        price_levels[price] += order['qty']
    
    result = [(price, qty) for price, qty in price_levels.items()]
    result.sort(key=lambda x: x[0])  # 从低到高
    
    return result[:n]

# 获取最低N档买单（价格最低的）
def get_bottom_bids(book, n):
    price_levels = {}
    for order in book['bids'].values():
        price = order['price']
        if price not in price_levels:
            price_levels[price] = 0
        price_levels[price] += order['qty']
    
    result = [(price, qty) for price, qty in price_levels.items()]
    result.sort(key=lambda x: x[0])  # 从低到高
    
    return result[:n]

# 获取最高N档卖单（价格最高的）
def get_bottom_asks(book, n):
    price_levels = {}
    for order in book['asks'].values():
        price = order['price']
        if price not in price_levels:
            price_levels[price] = 0
        price_levels[price] += order['qty']
    
    result = [(price, qty) for price, qty in price_levels.items()]
    result.sort(key=lambda x: x[0], reverse=True)  # 从高到低
    
    return result[:n]

# 拍摄快照
def take_snapshot(book, clockatarrival, transacttime):
    snapshot = {
        'clockatarrival': clockatarrival,
        'transacttime': transacttime,
        'best_bids': get_top_bids(book, 5),
        'best_asks': get_top_asks(book, 5),
        'worst_bids': get_bottom_bids(book, 5),
        'worst_asks': get_bottom_asks(book, 5),
        'cvl': book['cumulative_volume'],
        'lpr': book['last_price'],
        'cto': book['cumulative_trade_orders'],
        'nts': book['number_of_trades'],
        'opx': book['opening_price']
    }
    book['snapshots'].append(snapshot)

# 读取订单文件
def read_order_file(filename):
    orders = []
    try:
        with open(filename, 'r', encoding='utf-8') as f:
            reader = csv.reader(f)
            header = next(reader)
            print(f"Header: {','.join(header)}")
            
            for line_num, row in enumerate(reader, start=2):
                if len(row) >= 8:
                    order = {
                        'clockatarrival': int(row[0]),
                        'sequenceno': int(row[1]),
                        'transacttime': int(row[2]),
                        'applseqnum': int(row[3]),
                        'side': int(row[4]),
                        'ordertype': row[5][0] if row[5] else '2',
                        'price': float(row[6]) if row[6] else 0.0,
                        'orderqty': int(row[7]) if row[7] else 0
                    }
                    orders.append(order)
                else:
                    print(f"Warning: Line {line_num} has only {len(row)} fields")
    except Exception as e:
        print(f"Error reading file {filename}: {e}")
    
    print(f"Read {len(orders)} orders")
    return orders

# 读取交易文件
def read_trade_file(filename):
    trades = []
    try:
        with open(filename, 'r', encoding='utf-8') as f:
            reader = csv.reader(f)
            header = next(reader)
            print(f"Header: {','.join(header)}")
            
            for line_num, row in enumerate(reader, start=2):
                if len(row) >= 10:
                    trade = {
                        'clockatarrival': int(row[0]),
                        'sequenceno': int(row[1]),
                        'transacttime': int(row[2]),
                        'applseqnum': int(row[3]),
                        'exectype': row[4][0] if row[4] else 'f',
                        'tradeprice': float(row[5]) if row[5] else 0.0,
                        'tradeqty': int(row[6]) if row[6] else 0,
                        'trademoney': float(row[7]) if row[7] else 0.0,
                        'bidapplseqnum': int(row[8]) if row[8] else 0,
                        'offerapplseqnum': int(row[9]) if row[9] else 0
                    }
                    trades.append(trade)
                else:
                    print(f"Warning: Line {line_num} has only {len(row)} fields")
    except Exception as e:
        print(f"Error reading file {filename}: {e}")
    
    print(f"Read {len(trades)} trades")
    return trades

# 处理事件
def process_events(orders, trades, output_file):
    book = init_orderbook()
    
    # 定义开盘时间（9:30:00）
    OPENING_TIME = 93000000
    market_opened = False
    
    # 构建订单到即时交易的映射
    order_has_immediate_trade = {}
    for trade in trades:
        if trade['exectype'] == 'f':
            for order in orders:
                if ((order['applseqnum'] == trade['bidapplseqnum'] or 
                     order['applseqnum'] == trade['offerapplseqnum']) and
                    abs(order['transacttime'] - trade['transacttime']) <= 1000):
                    order_has_immediate_trade[order['applseqnum']] = True
    
    # 构建事件列表
    events = []
    for i, order in enumerate(orders):
        events.append({
            'type': 'order',
            'time': order['transacttime'],
            'index': i
        })
    
    for i, trade in enumerate(trades):
        events.append({
            'type': 'trade',
            'time': trade['transacttime'],
            'index': i
        })
    
    # 排序事件（先按时间，再按类型：订单优先于交易）
    events.sort(key=lambda e: (e['time'], 0 if e['type'] == 'order' else 1))
    
    # 处理事件
    for event in events:
        if event['type'] == 'order':
            order = orders[event['index']]
            
            is_immediate_market_order = (
                (order['ordertype'] == '1' or order['ordertype'] == 'u') and
                order['applseqnum'] in order_has_immediate_trade
            )
            
            if order['transacttime'] < OPENING_TIME or not is_immediate_market_order:
                add_order(book, order)
            
            if order['transacttime'] >= OPENING_TIME and not is_immediate_market_order:
                if not market_opened:
                    print("Market opened! Taking first snapshot...")
                    market_opened = True
                take_snapshot(book, order['clockatarrival'], order['transacttime'])
        else:
            trade = trades[event['index']]
            execute_trade(book, trade)
            take_snapshot(book, trade['clockatarrival'], trade['transacttime'])
    
    # 写入输出文件
    with open(output_file, 'w', newline='', encoding='utf-8') as f:
        writer = csv.writer(f)
        
        # 写入表头
        header = ['clockatarrival', 'transacttime']
        for i in range(1, 6):
            header.extend([f'best_bid_{i}_price', f'best_bid_{i}_qty'])
        for i in range(1, 6):
            header.extend([f'best_ask_{i}_price', f'best_ask_{i}_qty'])
        for i in range(1, 6):
            header.extend([f'worst_bid_{i}_price', f'worst_bid_{i}_qty'])
        for i in range(1, 6):
            header.extend([f'worst_ask_{i}_price', f'worst_ask_{i}_qty'])
        header.extend(['cvl', 'lpr', 'cto', 'nts', 'opx'])
        writer.writerow(header)
        
        # 写入快照数据
        for snapshot in book['snapshots']:
            row = [snapshot['clockatarrival'], snapshot['transacttime']]
            
            # 最佳买单（5档）
            for i in range(5):
                if i < len(snapshot['best_bids']):
                    row.extend([f"{snapshot['best_bids'][i][0]:.2f}", snapshot['best_bids'][i][1]])
                else:
                    row.extend(['', ''])
            
            # 最佳卖单（5档）
            for i in range(5):
                if i < len(snapshot['best_asks']):
                    row.extend([f"{snapshot['best_asks'][i][0]:.2f}", snapshot['best_asks'][i][1]])
                else:
                    row.extend(['', ''])
            
            # 最差买单（5档）
            for i in range(5):
                if i < len(snapshot['worst_bids']):
                    row.extend([f"{snapshot['worst_bids'][i][0]:.2f}", snapshot['worst_bids'][i][1]])
                else:
                    row.extend(['', ''])
            
            # 最差卖单（5档）
            for i in range(5):
                if i < len(snapshot['worst_asks']):
                    row.extend([f"{snapshot['worst_asks'][i][0]:.2f}", snapshot['worst_asks'][i][1]])
                else:
                    row.extend(['', ''])
            
            # 市场统计
            row.extend([
                snapshot['cvl'],
                f"{snapshot['lpr']:.2f}",
                snapshot['cto'],
                snapshot['nts'],
                f"{snapshot['opx']:.2f}"
            ])
            
            writer.writerow(row)
    
    print(f"Order book snapshots saved to {output_file}")
    print(f"Total snapshots: {len(book['snapshots'])}")

# 主函数
def main():
    print("========== Order Book Reconstruction ==========")
    
    # 尝试不同的路径
    paths_to_try = [
        'order_new.csv',
        '../order_new.csv',
        '../../order_new.csv',
        '../../../order_new.csv',
        '../../../../order_new.csv'
    ]
    
    order_path = None
    trade_path = None
    output_path = None
    
    for path in paths_to_try:
        try:
            with open(path, 'r') as f:
                order_path = path
                trade_path = path.replace('order_new.csv', 'trade_new.csv')
                output_path = path.replace('order_new.csv', 'book_new.csv')
                print(f"Found files at: {path}")
                break
        except FileNotFoundError:
            continue
    
    if not order_path:
        print("Error: Could not find order_new.csv in any expected location!")
        return
    
    # 读取数据
    orders = read_order_file(order_path)
    trades = read_trade_file(trade_path)
    
    if not orders:
        print("Error: No orders loaded!")
        return
    
    # 处理事件
    process_events(orders, trades, output_path)
    
    print("Processing complete!")
    print(f"Output saved to: {output_path}")

if __name__ == '__main__':
    main()

