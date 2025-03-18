#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <list>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

//! \brief 一个将字节流的一系列片段（可能是乱序的，也可能是重叠的）组装成有序字节流的类。
class StreamReassembler {
  private:
    struct block_node {
      size_t begin = 0;
      size_t length = 0;
      std::string data = "";
      // 使用 std::set 存储 block_node 对象，由于 block_node 重载了小于运算符，
      // 所以 _blocks 中的元素会按照起始位置 begin 自动排序
      bool operator<(const block_node t) const { return begin < t.begin;}
    };
    std::set<block_node> _blocks = {};
    std::vector<char> _buffer = {};
    size_t _unassembled_byte = 0;
    size_t _head_index = 0;
    bool _eof_flag = false;
    ByteStream _output; // 重组后的有序字节流
    size_t _capacity;   // 最大字节数

    //! merge elm2 to elm1, return merged bytes
    long merge_block(block_node &elm1, const block_node &elm2);


  public:
    //! \brief 构造一个最多可存储 `capacity` 字节的 `StreamReassembler` 对象。
    //! \note 这个容量既限制了已经重组的字节数，也限制了尚未重组的字节数。
    StreamReassembler(const size_t capacity);

    //! \brief 接收一个子字符串，并将任何新的连续字节写入流中。
    //! 
    //! 如果接受所有数据会超出此 `StreamReassembler` 的 `capacity`，那么只会接受适合的那部分数据。
    //! 如果子字符串只是部分被接受，那么 `eof` 标志将被忽略。
    //! 
    //! \param data 要添加的字符串
    //! \param index `data` 中第一个字节的索引
    //! \param eof 此段数据是否以流的结尾结束
    void push_substring(const std::string &data, const uint64_t index, const bool eof);

    //! \name 访问重组后的字节流
    //!@{
    // 常量引用方式返回重组后的字节流
    const ByteStream &stream_out() const { return _output; }
    // 非常量引用方式返回重组后的字节流
    ByteStream &stream_out() { return _output; }
    //!@}

    //! 已存储但尚未重组的子字符串中的字节数
    //! 
    //! \note 如果某个特定索引处的字节已被提交两次，在此函数的计算中该字节应只计算一次。
    size_t unassembled_bytes() const;

    //! \brief 内部状态是否为空（除了输出流之外）？
    //! \returns 如果没有子字符串等待组装，则返回 `true`
    bool empty() const;

    size_t head_index() const { return _head_index; }
    bool input_ended() const { return _output.input_ended(); }
};

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
