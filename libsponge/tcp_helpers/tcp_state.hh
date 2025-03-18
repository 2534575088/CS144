#ifndef SPONGE_LIBSPONGE_TCP_STATE
#define SPONGE_LIBSPONGE_TCP_STATE

#include "tcp_receiver.hh"
#include "tcp_sender.hh"

#include <string>

//! \brief TCP 连接内部状态的摘要信息
//!
//! 大多数 TCP 实现都有一个全局的、针对每个连接的状态机，
//! 如 [TCP](\ref rfc::rfc793) 规范中所描述的那样。
//! Sponge 稍有不同：我们将连接拆分为两个独立的部分（发送器和接收器）。
//! TCPSender 和 TCPReceiver 会独立维护它们内部的状态变量 
//! （例如，下一个序列号、正在传输中的字节数，或者每个流是否已经结束）。
//! 除了发送器和接收器之外，这里没有离散状态机的概念，也没有太多的总体状态。
//! 为了测试 Sponge 是否遵循 TCP 规范，我们使用这个类来比较 
//! “官方” 状态与 Sponge 的发送器/接收器状态，以及属于总体 TCPConnection 对象的两个变量。
class TCPState {
  private:
    std::string _sender{};  // 发送器状态的字符串表示
    std::string _receiver{};  // 接收器状态的字符串表示
    bool _active{true};  // 连接是否处于活动状态
    bool _linger_after_streams_finish{true};  // 流结束后是否保持连接一段时间

  public:
    bool operator==(const TCPState &other) const;  // 重载相等运算符，用于比较两个 TCPState 对象是否相等
    bool operator!=(const TCPState &other) const;  // 重载不等运算符，用于比较两个 TCPState 对象是否不相等

    //! \brief [TCP](\ref rfc::rfc793) 规范中定义的官方状态名称
    enum class State {
        LISTEN = 0,   //!< 正在监听，等待对等方建立连接
        SYN_RCVD,     //!< 已收到对等方的 SYN 包
        SYN_SENT,     //!< 已发送 SYN 包以发起连接
        ESTABLISHED,  //!< 三次握手已完成
        CLOSE_WAIT,   //!< 远程端已发送 FIN 包，连接处于半关闭状态
        LAST_ACK,     //!< 本地端在 CLOSE_WAIT 状态下发送了 FIN 包，正在等待 ACK
        FIN_WAIT_1,   //!< 已向远程端发送 FIN 包，但尚未收到 ACK
        FIN_WAIT_2,   //!< 已收到之前发送的 FIN 包的 ACK
        CLOSING,      //!< 在我们发送 FIN 包之后，紧接着收到了对方的 FIN 包
        TIME_WAIT,    //!< 双方都已发送 FIN 包并收到 ACK，正在等待 2 倍最大段生命周期
        CLOSED,       //!< 连接已正常终止
        RESET,        //!< 连接已异常终止
    };

    //! \brief 将 TCPState 状态信息总结为一个字符串
    std::string name() const;

    //! \brief 根据发送器、接收器以及 TCP 连接的活动状态和延迟状态构造一个 TCPState 对象
    TCPState(const TCPSender &sender, const TCPReceiver &receiver, const bool active, const bool linger);

    //! \brief 构造一个对应于 “官方” TCP 状态名称之一的 TCPState 对象
    TCPState(const TCPState::State state);

    //! \brief 将 TCPReceiver 的状态总结为一个字符串
    static std::string state_summary(const TCPReceiver &receiver);

    //! \brief 将 TCPSender 的状态总结为一个字符串
    static std::string state_summary(const TCPSender &receiver);
};

namespace TCPReceiverStateSummary {
const std::string ERROR = "error (connection was reset)";  // 错误状态（连接已重置）
const std::string LISTEN = "waiting for stream to begin (listening for SYN)";  // 等待流开始（监听 SYN 包）
const std::string SYN_RECV = "stream started";  // 流已开始
const std::string FIN_RECV = "stream finished";  // 流已结束
}  // namespace TCPReceiverStateSummary

namespace TCPSenderStateSummary {
const std::string ERROR = "error (connection was reset)";  // 错误状态（连接已重置）
const std::string CLOSED = "waiting for stream to begin (no SYN sent)";  // 等待流开始（未发送 SYN 包）
const std::string SYN_SENT = "stream started but nothing acknowledged";  // 流已开始，但尚未收到任何确认
const std::string SYN_ACKED = "stream ongoing";  // 流正在进行中
const std::string FIN_SENT = "stream finished (FIN sent) but not fully acknowledged";  // 流已结束（已发送 FIN 包），但尚未完全确认
const std::string FIN_ACKED = "stream finished and fully acknowledged";  // 流已结束且已完全确认
}  // namespace TCPSenderStateSummary

#endif  // SPONGE_LIBSPONGE_TCP_STATE
