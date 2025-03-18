#include "stream_reassembler.hh"

#include <iostream>
// 流重组器的占位实现。
// 对于实验 1，请用一个能通过 `make check_lab1` 自动检查的真正实现来替换此代码。
// 你需要在 `stream_reassembler.hh` 中的类声明里添加私有成员。

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

// 构造一个最大容量为 `capacity` 的流重组器。
// `_output` 是内部的字节流，其容量也被设置为 `capacity`，用于存储重组后的有序字节流。
// `_capacity` 存储了该流重组器允许处理的最大字节数。
StreamReassembler::StreamReassembler(const size_t capacity) 
    : _output(capacity), _capacity(capacity) // 成员初始化列表
{
    _buffer.resize(capacity);
}



/* 尝试合并两个字节块（由 block_node 结构体表示）。
 * 如果这两个字节块存在重叠部分，就将它们合并成一个新的字节块；若不存在重叠部分，则不进行合并操作。
 * 函数最终返回合并过程中被合并掉的字节数量，若无法合并则返回 -1*/
// 定义 merge_block 函数，用于合并两个字节块
long StreamReassembler::merge_block(block_node &elm1, const block_node &elm2){
    block_node x, y;
    // 比较 elm1 和 elm2 的起始位置，将起始位置较小的字节块赋值给 x，较大的赋值给 y
    if (elm1.begin > elm2.begin) {
        x = elm2;
        y = elm1;
    } else {
        x = elm1;
        y = elm2;
    }
    
    // 情况 1：两个字节块没有重叠部分
    // 判断 x 字节块的结束位置（起始位置 + 长度）是否小于 y 字节块的起始位置
    if (x.begin + x.length < y.begin) {
        return -1;
    } 
    // 情况 2：x 字节块完全包含 y 字节块
    // 判断 x 字节块的结束位置是否大于等于 y 字节块的结束位置
    else if (x.begin + x.length >= y.begin + y.length) {
        // 将合并结果（即 x 字节块）赋值给 elm1
        elm1 = x;
        // 返回被合并掉的 y 字节块的长度
        return y.length;
    } 
    // 情况 3：两个字节块部分重叠
    else {
        // 合并后的字节块起始位置为 x 字节块的起始位置
        elm1.begin = x.begin;
        // 合并后的字节块数据由 x 字节块的数据和 y 字节块中未重叠部分的数据拼接而成
        elm1.data = x.data + y.data.substr(x.begin + x.length - y.begin);
        // 更新合并后字节块的长度
        elm1.length = elm1.data.length();
        // 返回重叠部分的字节数量
        return x.begin + x.length - y.begin;
    }
}

//! \details 此函数接收来自逻辑流的一个子字符串（也称为一个段），该子字符串可能是乱序的。
//! 它会将任何新的连续子字符串进行组装，并按顺序写入输出流中。
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // _head_index 表示当前等待组装的第一个字节的索引，_capacity 是缓冲区的容量
    // 如果起始索引大于等于 _head_index + _capacity，说明该子字符串超出了缓冲区范围，直接返回
    if(index >= _head_index + _capacity){
        return;
    }

    block_node elm;

    // 情况 1：子字符串完全在已处理范围之前
    // 如果子字符串的结束索引（index + data.length()）小于等于 _head_index，说明该子字符串已经处理过了
    if(index + data.length() <= _head_index){
        // 跳转到 JUDGE_EOF 标签处，检查 EOF 标志
        goto JUDGE_EOF;
    }
    // 情况 2：子字符串部分在已处理范围之前
    else if(index < _head_index){
        // 计算子字符串中需要跳过的字节数
        size_t offset = _head_index - index;
        // 截取子字符串中从 _head_index 开始的部分
        elm.data.assign(data.begin() + offset, data.end());
        // 更新子字符串的起始索引
        elm.begin = index + offset;
        // 更新子字符串的长度
        elm.length = elm.data.length();
    }
    // 情况 3：子字符串完全在未处理范围
    else{
        // 直接使用传入的起始索引
        elm.begin = index;
        // 直接使用传入的子字符串长度
        elm.length = data.length();
        // 直接使用传入的子字符串数据
        elm.data = data;
    }

    // 增加未组装的字节数
    _unassembled_byte += elm.length;

    // 合并重叠的子字符串
    do{
        // 初始化合并的字节数为 0
        long merged_bytes = 0;
        // 找到第一个起始位置大于等于 elm.begin 的块
        auto iter = _blocks.lower_bound(elm);
        // 从当前找到的块开始，向后遍历 _blocks 集合，尝试合并重叠的块
        while(iter != _blocks.end() && (merged_bytes = merge_block(elm, *iter)) >= 0){
            // 减少未组装的字节数
            _unassembled_byte -= merged_bytes;
            // 从 _blocks 集合中移除已合并的块
            _blocks.erase(iter);
            // 重新找到第一个起始位置大于等于 elm.begin 的块
            iter = _blocks.lower_bound(elm);
        }
        // 如果已经到了 _blocks 集合的开头，说明前面没有可合并的块了，跳出循环
        if(iter == _blocks.begin()){
            break;
        }
        // 向前移动迭代器
        iter--;
        // 从当前位置开始，向前遍历 _blocks 集合，尝试合并重叠的块
        while((merged_bytes = merge_block(elm, *iter)) >= 0){
            // 减少未组装的字节数
            _unassembled_byte -= merged_bytes;
            // 从 _blocks 集合中移除已合并的块
            _blocks.erase(iter);
            // 重新找到第一个起始位置大于等于 elm.begin 的块
            iter = _blocks.lower_bound(elm);
            // 如果已经到了 _blocks 集合的开头，说明前面没有可合并的块了，跳出循环
            if(iter == _blocks.begin()){
                break;
            }
            // 向前移动迭代器
            iter--;
        }
    // 由于条件为 false，循环只会执行一次
    }while(false);

    // 将处理后的块插入到 _blocks 集合中
    _blocks.insert(elm);

    // 检查是否有可以写入输出流的连续块
    if(!_blocks.empty() && _blocks.begin()->begin == _head_index){
        // 获取第一个块
        const block_node head_block = *_blocks.begin();
        // 将该块的数据写入输出流，返回实际写入的字节数
        size_t write_bytes = _output.write(head_block.data);
        // 更新 _head_index 为下一个等待组装的字节的索引
        _head_index += write_bytes;
        // 减少未组装的字节数
        _unassembled_byte -= write_bytes;
        // 从 _blocks 集合中移除已写入输出流的块
        _blocks.erase(_blocks.begin());
    }

JUDGE_EOF:
    // 如果该子字符串是流的最后一部分，设置 eof 标志
    if (eof) {
        _eof_flag = true;
    }
    // 如果已经收到 eof 标志且没有未组装的数据块，结束输入
    if (_eof_flag && empty()) {
        _output.end_input();
    }
}

// 返回已存储但尚未重组的子字符串中的字节数。
size_t StreamReassembler::unassembled_bytes() const { return _unassembled_byte; }

// 检查内部状态是否为空（除了输出流之外）。
// 如果没有子字符串等待组装，则返回 `true`，否则返回 `false`。
bool StreamReassembler::empty() const { return _unassembled_byte == 0; }