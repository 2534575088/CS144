#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

// 返回发送方字节流中剩余的可写入容量
size_t TCPConnection::remaining_outbound_capacity() const { return {_sender.stream_in().remaining_capacity()}; }

// 返回已发送但尚未被确认的字节数
size_t TCPConnection::bytes_in_flight() const { return {_sender.bytes_in_flight()}; }

// 返回接收方尚未重新组装的字节数
size_t TCPConnection::unassembled_bytes() const { return {_receiver.unassembled_bytes()}; }

// 返回自上次接收到TCP段以来经过的时间
size_t TCPConnection::time_since_last_segment_received() const { return {_time_since_last_segment_received}; }

// 当接收到一个新的TCP段时调用此方法
void TCPConnection::segment_received(const TCPSegment &seg) { 
    // 如果连接不活跃 直接返回
    if(!_active)
        return;
    // 重置自上次接收到段以来的事件为0
    _time_since_last_segment_received = 0;

    // 如果处于SYN_SENT状态（即完成了第一个握手），则忽略掉带有数据的ACK段，因为需要的是不带数据的ACK段（即需要第二次握手）
    if(in_syn_sent() && seg.header().ack && seg.payload().size() > 0){
        return;
    }
    bool send_empty = false;

    // 如果发送方已经发送了数据且接受到的段带有ACK
    if(_sender.next_seqno_absolute() > 0 && seg.header().ack){
        // 如果接收到的ACK不是一个新的有效确认，可能是一个重复的 ACK 标记需要发送一个空段
        if(!_sender.ack_received(seg.header().ackno, seg.header().win)){
            // 指示需要发送一个空段来再次确认相关信息  应对重复确认的情况
            send_empty = true;
        }
    }

    // 调用接收方处理接受到的段 并记录处理结果
    bool recv_flag = _receiver.segment_received(seg);
    if(!recv_flag){
        send_empty = true;
    }

    // 如果接收的是SYN段且下一个要发送的是0号序列（代表没有发送过数据，因此要发起连接）
    if(seg.header().syn && _sender.next_seqno_absolute() == 0){
        connect();
        return;
    }
    
    // 如果接收到的段带有RST标志
    if(seg.header().rst){
        // 在SYN_SENT状态下 忽略没有ACK的RST段
        if(in_syn_sent() && !seg.header().ack){
            return;
        }
        // 异常关闭连接 已经接收到对端的 RST 段，说明对端已经发起了异常关闭，本端不需要再发送 RST 段
        unclean_shutdown(false);
        return;
    }

    // 如果接收到的段在序列号空间中有长度
    if(seg.length_in_sequence_space() > 0){
        send_empty = true;
    }

    // 如果需要发送空段
    if(send_empty){
        // 如果接收方有确认号且发送方的待发送队列为空（保证没有重复的ACK）
        if(_receiver.ackno().has_value() && _sender.segments_out().empty()){
            // 发送一个空段
            _sender.send_empty_segment();
        }
    }
    // 将发送方的段推送到待发送队列
    push_segments_out();
}

// 判断 TCP 连接是否处于活跃状态
bool TCPConnection::active() const { return { _active }; }

// 将数据写入发送方的字节流，并尝试发送
size_t TCPConnection::write(const string &data) {
    // 写入数据并记录实际写入的字节数
    size_t ret = _sender.stream_in().write(data);
    push_segments_out();
    return ret;
}

//! \param[in] ms_since_last_tick 自上次调用该函数以来运行了多久
// 当时间流逝时调用此方法，处理超时等情况
void TCPConnection::tick(const size_t ms_since_last_tick) { 
    if(!_active) return;
    // 更新自上次接收到段以来的时间
    _time_since_last_segment_received += ms_since_last_tick;

    // 通知发送方时间流逝
    _sender.tick(ms_since_last_tick);

    // 如果连续重传次数超过最大允许次数
    if(_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS){
        // 异常关闭连接，发送 RST 段
        unclean_shutdown(true);
    }
    // 保证每次定时器被调用的时候，都能推送数据，因为_sender的tick()函数会将超时的重新加入_sender的输出队列
    // 因此需要调用此函数来保证重传的数据也能正确发送
    push_segments_out();
}

// 关闭发送方的字节流
void TCPConnection::end_input_stream() {
    // 标记发送方字节流输入结束
    _sender.stream_in().end_input();
    push_segments_out();
}

// 发起一个 TCP 连接
void TCPConnection::connect() {
    // 连接时，必须主动发送一个 SYN 段
    push_segments_out(true);
}

// TCP 连接的析构函数
TCPConnection::~TCPConnection() {
    try {
        // 如果连接仍处于活跃状态
        if (active()) {
            // 你的代码：需要向对端发送一个 RST 段
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            unclean_shutdown(true);
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

// 将发送方的段推送到待发送队列
bool TCPConnection::push_segments_out(bool send_syn){
    // 处于syn_recv状态时，需要发送SYN_ACK段
    _sender.fill_window(send_syn || in_syn_recv());

    TCPSegment seg;
    // 循环处理发送方待发送队列中的段
    while(!_sender.segments_out().empty()){
        // 取出发送方待发送队列的第一个段
        seg = _sender.segments_out().front();
        // 从发送方待发送队列中移除该段
        _sender.segments_out().pop();
        // 如果接收方有确认号
        if(_receiver.ackno().has_value()){
            // 在段的头部设置 ACK 标志
            seg.header().ack = true;
            // 设置段的确认号
            seg.header().ackno = _receiver.ackno().value();
            // 设置段的窗口大小
            seg.header().win = _receiver.window_size();
        }
        // 如果需要发送 RST 段
        if(_need_send_rst){
            _need_send_rst = false;
            seg.header().rst = true;
        }
        // 将处理后的段添加到待发送队列
        _segments_out.push(seg);
    }
    // 尝试正常关闭连接
    clean_shutdown();
    return true;
}

// @brief 由于RST引起的连接中断
// @param send_rst 是否需要发送RST段
void TCPConnection::unclean_shutdown(bool send_rst){
    // 标记接收方字节流出错
    _receiver.stream_out().set_error();
    // 标记发送方字节流出错
    _sender.stream_in().set_error();
    // 标记连接不活跃
    _active = false;
    // 如果需要发送 RST 段
    if(send_rst){
        _need_send_rst = true;
        if(_sender.segments_out().empty()){
            _sender.send_empty_segment();
        }
        push_segments_out();
    }
}

// 尝试正常关闭 TCP 连接
bool TCPConnection::clean_shutdown(){
    // 如果接收方字节流输入结束，但发送方字节流还未结束
    if(_receiver.stream_out().input_ended() && !(_sender.stream_in().eof())){
        // 不需要在流结束后逗留
        _linger_after_streams_finish = false;
    }

    // 如果发送方字节流结束，已发送但未确认的字节数为 0，且接收方字节流输入结束
    if(_sender.stream_in().eof() && _sender.bytes_in_flight() == 0 && _receiver.stream_out().input_ended()){
        // 如果不需要逗留或者自上次接收到段以来的时间超过 10 倍的重传超时时间
        if(!_linger_after_streams_finish || time_since_last_segment_received() >= 10*_cfg.rt_timeout){
            // 标记连接不活跃
            _active = false;
        }
    }
    return !_active;
}

// 判断 TCP 连接是否处于 LISTEN 状态
bool TCPConnection::in_listen(){
    return !_receiver.ackno().has_value() && _sender.next_seqno_absolute() == 0;
}

// syn_recv是TCP连接被动方接收了SYN段并发送了SYN_ACK之后的状态
bool TCPConnection::in_syn_recv(){
    // syn_recv是指远程对等方已经发送了对本地方SYN段的确认，从而等待接收本地方ACK的状态
    // 如何判断处于syn_recv状态
    //  首先是远方的ackno()函数肯定会有值，因为接收方的_base是期待的下一个到来的相对序列号
    //  而ackno()是返回该值的绝对值，又因为接收方接收到了相对序号为0的SYN段，因此_base将会是1
    //  因此，有了第一个判断条件 _receiver.ackno().has_value();
    //  其次，接收方的入站流也不会结束，因为还要接收数据

    return _receiver.ackno().has_value() && !_receiver.stream_out().input_ended();
}

// syn_sent是TCP连接发起方完成第一次握手之后的状态
// syn_sent是已经发送了SYN段但是还没有接收的状态
bool TCPConnection::in_syn_sent(){
    // SYN会消耗一个序列号，因此要发送的下一个绝对序列号应该大于0，此时要发送的下一个绝对序列号就是1,并且发送出去但是还未确认字节数就是1
    return _sender.next_seqno_absolute() > 0 && _sender.bytes_in_flight() == _sender.next_seqno_absolute();
}