#ifndef SPONGE_LIBSPONGE_TCP_FACTORED_HH
#define SPONGE_LIBSPONGE_TCP_FACTORED_HH

#include "tcp_config.hh"
#include "tcp_receiver.hh"
#include "tcp_sender.hh"
#include "tcp_state.hh"

//! \brief 一个完整的 TCP 连接端点
class TCPConnection {
  private:
    TCPConfig _cfg;  // TCP 连接的配置信息
    // 接收端，使用配置中的接收缓冲区容量进行初始化
    TCPReceiver _receiver{_cfg.recv_capacity};
    // 发送端，使用配置中的发送缓冲区容量、重传超时时间和固定初始序列号进行初始化
    TCPSender _sender{_cfg.send_capacity, _cfg.rt_timeout, _cfg.fixed_isn};

    //! 待发送的 TCP 段队列，TCPConnection 想要发送的段会存放在这里
    std::queue<TCPSegment> _segments_out{};

    //! 在两个流都结束后，TCPConnection 是否应该保持活跃状态（并继续发送 ACK）
    //! 持续 10 * _cfg.rt_timeout 毫秒，以防远程 TCPConnection 不知道我们已经接收到了其整个流
    bool _linger_after_streams_finish{true};

    size_t _time_since_last_segment_received = 0;
    bool _active = true;
    bool _need_send_rst = false;
    bool _ack_for_fin_sent = false;

    bool push_segments_out(bool send_syn = false);
    void unclean_shutdown(bool send_rst);
    bool clean_shutdown();
    bool in_listen();
    bool in_syn_recv();
    bool in_syn_sent();

  public:
    //! \name 面向写入方的 “输入” 接口
    //!@{

    //! \brief 通过发送 SYN 段来发起一个连接
    void connect();

    //! \brief 将数据写入出站字节流，并在可能的情况下通过 TCP 发送
    //! \returns 实际从 `data` 中写入的字节数
    size_t write(const std::string &data);

    //! \returns 当前可以立即写入的字节数
    size_t remaining_outbound_capacity() const;

    //! \brief 关闭出站字节流（仍然允许读取传入的数据）
    void end_input_stream();
    //!@}

    //! \name 面向读取方的 “输出” 接口
    //!@{

    //! \brief 从对端接收到的入站字节流
    ByteStream &inbound_stream() { return _receiver.stream_out(); }
    //!@}

    //! \name 用于测试的访问器

    //!@{
    //! \brief 已发送但尚未确认的字节数，将 SYN/FIN 各计为一个字节
    size_t bytes_in_flight() const;
    //! \brief 尚未重新组装的字节数
    size_t unassembled_bytes() const;
    //! \brief 自上次接收到段以来经过的毫秒数
    size_t time_since_last_segment_received() const;
    //!< \brief 总结发送端、接收端和连接的状态
    TCPState state() const { return {_sender, _receiver, active(), _linger_after_streams_finish}; };
    //!@}

    //! \name 供所有者或操作系统调用的方法
    //!@{

    //! 当从网络接收到一个新的段时调用
    void segment_received(const TCPSegment &seg);

    //! 当时间流逝时定期调用
    void tick(const size_t ms_since_last_tick);

    //! \brief TCPConnection 已排入队列等待传输的 TCP 段
    //! \note 所有者或操作系统将从队列中取出这些段，并将每个段放入下层数据报（通常是互联网数据报 (IP)，
    //! 但也可以是用户数据报 (UDP) 或任何其他类型）的有效负载中。
    std::queue<TCPSegment> &segments_out() { return _segments_out; }

    //! \brief 连接是否仍以任何方式处于活动状态？
    //! \returns 如果任一个流仍在运行，或者在两个流都结束后 TCPConnection 仍处于逗留状态（例如，为了对来自对端的重传进行 ACK），则返回 `true`
    bool active() const;
    //!@}

    //! 根据配置构造一个新的连接
    explicit TCPConnection(const TCPConfig &cfg) : _cfg{cfg} {}

    //! \name 构造和析构
    //! 允许移动操作；禁止复制操作；不允许默认构造

    //!@{
    ~TCPConnection();  //!< 如果连接仍处于打开状态，析构函数会发送一个 RST 段
    TCPConnection() = delete;
    TCPConnection(TCPConnection &&other) = default;
    TCPConnection &operator=(TCPConnection &&other) = default;
    TCPConnection(const TCPConnection &other) = delete;
    TCPConnection &operator=(const TCPConnection &other) = delete;
    //!@}
};

#endif  // SPONGE_LIBSPONGE_TCP_FACTORED_HH