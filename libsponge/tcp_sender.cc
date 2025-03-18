#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;


// 函数设计思路：
	// 1. 首先判断是否需要发送SYN段，这部分由两个变量控制，一个是成员变量syn_flag，一个是形参send_syn
	// 2. 如果发送SYN段，则设置TCP头中的syn标志，然后调用send_segment标志。
	// 3. 获取窗口大小
	// 4. 通过while循环不断发送，直到需要出站流结束，需要发送FIN段为止 
//! \param[in] capacity 输出字节流的容量
//! \param[in] retx_timeout 重传最旧的未确认分段之前等待的初始时间
//! \param[in] fixed_isn 初始序列号（ISN），如果设置了则使用该值，否则使用随机生成的 ISN
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    // 如果 fixed_isn 有值则使用该值，否则使用随机生成的 ISN
    // value_or 用于在 std::optional 对象有值时返回其存储的值，在对象为空时返回一个默认值
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()})) 
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _retransmission_timeout(retx_timeout) {}

// 获取当前正在传输中的字节数
uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

// 填充发送窗口，尝试发送数据
void TCPSender::fill_window(bool send_syn) {
    // 在发送其他分段之前先发送 SYN 分段
    if (!_syn_flag) {
        if (send_syn) {
            TCPSegment seg;
            // 设置 SYN 标志
            seg.header().syn = true;
            // 发送该分段
            send_segment(seg);
            // 标记 SYN 已发送
            _syn_flag = true;
        }
        return;
    }

    // take window_size as 1 when it equal 0
    size_t win = _window_size > 0 ? _window_size : 1;
    // window's free space
    size_t remain;  
    // when window isn't full and never sent FIN
    while ((remain = win - (_next_seqno - _recv_ackno)) != 0 && !_fin_flag) {
        // 取最大有效载荷大小和窗口剩余空间的最小值作为本次要发送的数据大小
        size_t size = min(TCPConfig::MAX_PAYLOAD_SIZE, remain);
        TCPSegment seg;
        // 从字节流中读取数据
        string str = _stream.read(size);
         // 将读取的数据放入分段的有效载荷中
        seg.payload() = Buffer(std::move(str));
        // 如果分段长度小于窗口大小且字节流已结束，添加 FIN 标志 FIN 标志也会消耗一个序列
        if (seg.length_in_sequence_space() < win && _stream.eof()) {
            seg.header().fin = true;
            _fin_flag = true;
        }
        // 如果分段长度为 0，说明没有数据可发送，退出循环
        if (seg.length_in_sequence_space() == 0) {
            return;
        }
        send_segment(seg);
    }
}

// 函数设计思路：
	// 1. 将到来的确认号转换为绝对序列号
	// 2. 判断，与下一个要发送的绝对序列号、已经接收的最大绝对序列号进行比较
	// 3. 根据window_size更新成员变量_window_size(记录窗口大小)
	// 4. 更新成员变量recv_ackno（即已经接收的最大绝对序列号）
	// 5. 更新segment_outstanding(已发送但是未确认的TCP段)队列
	// 6. 窗口右移，即调用fill_window()
	// 7. 重启计时器，因为发送了新的TCP段
/*
 * @brief 是否收到了新的确认
 * @param[in] ackno 远端接收方的确认号,即期待发送方的下一个序号
 * @param[in] window_size 远程接收器的窗口大小
 * @details 可以导致TCPSender发送一个段的方法
 * @attention 采用累积确认
 * @return 如果确认无效（确认TCPSender尚未发送的内容），返回‘ false ’
 */
bool TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    // 将相对确认号转换为绝对确认号
    size_t abs_ackno = unwrap(ackno, _isn, _recv_ackno);
    // 大于下一个要发送的绝对序列号，返回 false
    if (abs_ackno > _next_seqno) {
        return false;
    }

    // 如果确认号合法，更新窗口大小
    _window_size = window_size;

    // 如果确认号已经被接收过，直接返回 true
    if (abs_ackno <= _recv_ackno) {
        return true;
    }

    // 更新已接收的确认号
    _recv_ackno = abs_ackno;

    // 从待确认分段队列中移除所有序列号小于等于确认号的分段
    while (!_segments_outstanding.empty()) {
        TCPSegment seg = _segments_outstanding.front();
        // 判断TCP段是否被确认-----累积确认
        // 小笔记：
        // 已知ack n是确认n-1都已经到达，为什么这里可以等于
        // 因为：例如ack=100,seqno=0,length=100,很明显tcp段的序号是0-99，但是0+100=100,因此可以=
        if (unwrap(seg.header().seqno, _isn, _next_seqno) + seg.length_in_sequence_space() <= abs_ackno) {
            _bytes_in_flight -= seg.length_in_sequence_space();
            _segments_outstanding.pop();
        } else {
            break;
        }
    }

    // // 现在接收到了确认之后，就需要窗口往右边移动，因此，对窗口进行填充，并进行发送
    fill_window();

    // 重置重传超时时间和连续重传次数
    _retransmission_timeout = _initial_retransmission_timeout;
    _consecutive_retransmission = 0;

    // 如果还有待确认的分段，重启定时器
    if (!_segments_outstanding.empty()) {
        _timer_running = true;
        _timer = 0;
    }
    return true;
}

// 函数设计思路：
	// 1. 主要注重一点，重传计时器是全局的，而不是每个TCP段都有一个重传计时器，
    // 即一旦有一个段超时了，RTO就得*2，有一个新的发送的，RTO设计为初始值

// 处理定时器滴答事件，检查是否需要重传
void TCPSender::tick(const size_t ms_since_last_tick) {
    // 更新定时器时间
    _timer += ms_since_last_tick;
    // 如果定时器超时且有待确认的分段
    if (_timer >= _retransmission_timeout && !_segments_outstanding.empty()) {
        // 重传最旧的未确认分段
        _segments_out.push(_segments_outstanding.front());
        // 连续重传次数加 1
        _consecutive_retransmission++;
        // 重传超时时间翻倍
        _retransmission_timeout *= 2;
        // 重启定时器
        _timer_running = true;
        _timer = 0;
    }
    // 如果没有待确认的分段，停止定时器
    if (_segments_outstanding.empty()) {
        _timer_running = false;
    }
}

// 获取连续重传的次数
unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmission; }

// 发送一个空分段
void TCPSender::send_empty_segment() {
    // empty segment doesn't need store to outstanding queue
    TCPSegment seg;
    // 设置分段的序列号
    seg.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(seg);
}

// 发送一个指定序列号的空分段
void TCPSender::send_empty_segment(WrappingInt32 seqno) {
    // empty segment doesn't need store to outstanding queue
    TCPSegment seg;
    // 设置分段的序列号
    seg.header().seqno = seqno;
    _segments_out.push(seg);
}

// 发送一个分段
void TCPSender::send_segment(TCPSegment &seg) {
    // 设置分段的序列号
    seg.header().seqno = wrap(_next_seqno, _isn);
    // 更新下一个要发送的序列号
    _next_seqno += seg.length_in_sequence_space();
    // 增加正在传输中的字节数
    _bytes_in_flight += seg.length_in_sequence_space();
    // 将分段放入待确认队列
    _segments_outstanding.push(seg);
    // 将分段放入发送队列
    _segments_out.push(seg);

    // start timers
    if (!_timer_running) {  
        _timer_running = true;
        _timer = 0;
    }
}