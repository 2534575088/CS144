#include "wrapping_integers.hh"

// TCP 协议使用 32 位无符号整数来表示序列号，当序列号达到最大值（2^32 −1）后会发生回绕，重新从 0 开始

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;


// 将一个 64 位的绝对序列号（从 0 开始编号）转换为 32 位的包装整数
//! \param n The input absolute 64-bit sequence number
// 输入的 64 位绝对序列号
//! \param isn The initial sequence number
// 初始序列号
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    /*
    * 直接借助C++11的强制转换即可
    * 注：绝对->相对，需要加上isn,因为一般在TCP通信过程中，其开始的相对序列号一般是一个随机值，这是为了提高安全性。*/
    // 将 64 位的绝对序列号转换为 32 位无符号整数，并加上初始序列号的原始值
    // 由于 32 位无符号整数的范围是 0 到 2^32 - 1，当相加结果超过这个范围时会自动回绕
    return WrappingInt32(static_cast<uint32_t>(n) + isn.raw_value());}


// 将一个 32 位的包装整数转换为 64 位的绝对序列号（从 0 开始编号）
//! \param n 相对序列号（32 位包装整数）
//! \param isn 初始序列号
//! \param checkpoint 个最近的 64 位绝对序列号，用于帮助确定最接近的展开结果
//! \returns 返回一个 64 位的序列号，该序列号包装后等于 `n` 且最接近 `checkpoint`
// 注意：TCP 连接的两个数据流都有各自的初始序列号（ISN）。一个数据流从本地发送方到远程接收方，有一个 ISN；
// 另一个数据流从远程发送方到本地接收方，有不同的 ISN。
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    // 计算相对序列号与初始序列号的偏移量
    uint32_t offset = n.raw_value() - isn.raw_value();
    // 取 checkpoint 的高 32 位，并加上偏移量，得到一个初步的 64 位序列号
    uint64_t t = (checkpoint & 0xFFFFFFFF00000000) + offset;
    // 初始化返回值为初步计算的 64 位序列号
    uint64_t ret = t;

    // 检查 t + 2^32 是否更接近 checkpoint
    // 如果是，则将返回值更新为 t + 2^32
    // 1ul 表示一个无符号长整型（unsigned long）的常量 即为2^32
    if(abs(int64_t(t + (1ul << 32) - checkpoint)) < abs(int64_t(t - checkpoint)))
        ret = t + (1ul << 32);

    // 检查 t - 2^32 是否更接近 checkpoint
    // 前提是 t 大于等于 2^32
    // 如果是，则将返回值更新为 t - 2^32
    if(t >= (1ul << 32) && abs(int64_t(t - (1ul << 32) - checkpoint)) < abs(int64_t(ret - checkpoint)))
        ret = t - (1ul << 32);
    return ret;
}
