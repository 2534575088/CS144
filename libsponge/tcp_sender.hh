// 头文件保护，防止头文件被重复包含，避免重复定义的问题
#ifndef SPONGE_LIBSPONGE_TCP_SENDER_HH
#define SPONGE_LIBSPONGE_TCP_SENDER_HH

// 包含字节流处理的头文件，用于处理待发送的字节流
#include "byte_stream.hh"
// 包含 TCP 配置相关的头文件，提供 TCP 的默认配置参数
#include "tcp_config.hh"
// 包含 TCP 段相关的头文件，用于处理 TCP 段的封装和解析
#include "tcp_segment.hh"
// 包含包装整数相关的头文件，用于处理 TCP 序列号的包装和解包
#include "wrapping_integers.hh"

// 包含标准库中的 functional 头文件，用于处理函数对象
#include <functional>
// 包含标准库中的 queue 头文件，用于使用队列数据结构
#include <queue>

// 定义 TCPSender 类，实现 TCP 发送方的功能
class TCPSender {
  private:
    
    // 初始序列号，用于 TCP 连接建立时的 SYN 段
    WrappingInt32 _isn;

    // 待发送的 TCP 段的队列，存储需要发送到网络中的 TCP 段
    std::queue<TCPSegment> _segments_out{};

    // 连接的初始重传超时时间，用于设置重传定时器的初始值
    unsigned int _initial_retransmission_timeout;

    // 待发送的字节流，存储还未被分割成 TCP 段发送出去的数据
    ByteStream _stream;

    // 下一个待发送字节的绝对序列号，用于跟踪发送进度
    uint64_t _next_seqno{0};

    // 已发送但未确认的 TCP 段的队列，用于跟踪哪些段还在传输中
    std::queue<TCPSegment> _segments_outstanding{};
    // 已发送但未确认的字节数，记录当前处于传输中的字节数量
    size_t _bytes_in_flight = 0;
    // 接收到的确认号，记录接收方已经成功接收的数据的序列号
    size_t _recv_ackno = 0;
    // 表示是否已发送 SYN 标志，用于跟踪 TCP 连接建立的状态
    bool _syn_flag = false;
    // 表示是否已发送 FIN 标志，用于跟踪 TCP 连接关闭的状态
    bool _fin_flag = false;
    // 接收方的窗口大小，指示接收方当前能够接收的数据量
    size_t _window_size = 0;

    // 重传定时器的计数器，记录从定时器启动到现在经过的时间
    size_t _timer = 0;
    // 表示重传定时器是否正在运行，用于控制定时器的启动和停止
    bool _timer_running = false;
    // 当前的重传超时时间，可能会根据网络情况进行调整
    size_t _retransmission_timeout = 0;
    // 连续重传的次数，用于实现 TCP 的重传策略
    size_t _consecutive_retransmission = 0;

    // 私有成员函数，用于发送一个 TCP 段
    void send_segment(TCPSegment &seg);

  public:

    // 构造函数，用于初始化 TCPSender 对象
    // capacity：字节流的容量，默认为 TCP 配置中的默认容量
    // retx_timeout：重传超时时间，默认为 TCP 配置中的默认超时时间
    // fixed_isn：可选的初始序列号，如果未提供则使用默认值
    TCPSender(const size_t capacity = TCPConfig::DEFAULT_CAPACITY,
              const uint16_t retx_timeout = TCPConfig::TIMEOUT_DFLT,
              const std::optional<WrappingInt32> fixed_isn = {});

    // 非 const 版本的输入字节流访问函数，返回字节流的引用
    ByteStream &stream_in() { return _stream; }
    // const 版本的输入字节流访问函数，返回字节流的常量引用
    const ByteStream &stream_in() const { return _stream; }
    // 处理接收到的确认号和窗口大小，返回是否成功处理的布尔值
    bool ack_received(const WrappingInt32 ackno, const uint16_t window_size);

    // 生成一个空负载的 TCP 段，用于创建空的 ACK 段
    void send_empty_segment();
    // 重载的生成空负载 TCP 段的函数，指定序列号
    void send_empty_segment(WrappingInt32 seqno);

    // 创建并发送尽可能多的段以填满窗口，可选择是否发送 SYN 段
    void fill_window(bool send_syn = true);

    // 通知 TCPSender 时间的流逝，用于更新重传定时器等
    void tick(const size_t ms_since_last_tick);
    // 返回已发送但未确认的字节数，考虑 SYN 和 FIN 各占一个字节
    size_t bytes_in_flight() const;

    // 返回连续重传的次数
    unsigned int consecutive_retransmissions() const;

    // 返回待发送的 TCP 段的队列，这些段需要由 TCPConnection 出队并发送
    std::queue<TCPSegment> &segments_out() { return _segments_out; }
    // 返回下一个待发送字节的绝对序列号
    uint64_t next_seqno_absolute() const { return _next_seqno; }

    // 返回下一个待发送字节的相对序列号
    WrappingInt32 next_seqno() const { return wrap(_next_seqno, _isn); }

};

// 结束头文件保护
#endif  // SPONGE_LIBSPONGE_TCP_SENDER_HH
