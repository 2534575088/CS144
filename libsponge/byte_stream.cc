#include "byte_stream.hh"

#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <sstream>

/*
代码功能概述
这段代码实现了一个简单的字节流类 ByteStream，用于管理字节数据的读写操作。该类提供了写入数据、查看数据、移除数据、标记输入结束等功能，
同时还提供了一些用于查询字节流状态的方法，如缓冲区大小、是否输入结束、是否到达末尾等。

主要方法解释
write 方法：向字节流中写入数据，会根据字节流的剩余容量调整实际写入的字节数。
peek_output 方法：从字节流的输出端查看指定长度的字节数据，但不会将其从缓冲区移除。 配合pop_output()即可实现读
pop_output 方法：从字节流的输出端移除指定长度的字节数据。
end_input 方法：标记字节流的输入结束。
其他查询方法：用于获取字节流的各种状态信息，如缓冲区大小、是否输入结束、是否到达末尾、已写入和已读取的字节数、剩余容量等。
*/

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

// 构造函数，初始化字节流对象，指定字节流的容量
ByteStream::ByteStream(const size_t capacity) : _capacity(capacity) {}

// 向字节流写入数据
size_t ByteStream::write(const string &data) {
    // 获取写入数据的长度
    size_t len = data.length();
    // 如果要写入数据长度超过了字节流剩余的容量
    if (len > _capacity - _buffer.size()){
        // 将写入长度调节为字节流剩余容量
        len = _capacity - _buffer.size();
    }
    // 累加写入的字节数
    _write_count += len;
    // 将数据的前len个字符添加到字符的缓冲区中
    for(size_t i = 0; i < len; i++){
        _buffer.push_back(data[i]);
    }
    // 返回实际写入字节
    return len;
}

// 从字节流的输出端查看指定长度的字节数据 但不将其从缓冲区移除
string ByteStream::peek_output(const size_t len) const {
    // // 要查看的字节长度
    // size_t length = len;
    // // 如果要查看的长度超过了缓冲区的大小
    // if (length > _buffer.size()){
    //     // 则将查看长度调整为缓冲区的大小
    //     length = _buffer.size();
    // }

    const size_t length = std::min(len, _buffer.size());

    // 从缓冲区的开始位置复制 length 个字符到一个新的字符串中并返回
    // assign 是 std::string 类的成员函数，用于将指定范围的字符赋值给字符串对象 
    // return string().assign(_buffer.begin(), _buffer.begin() + length);

    // 可以直接使用 std::string 的构造函数进行复制
    // 优点1：assign 方法可能会有一些额外的内部管理开销（如清空原字符串等操作），直接用string复制避免了这些开销。
    // 优点2：直接调用string的构造函数生成了一个临时对象，然后在返回这个字符串并进行赋值的时候，不会调用拷贝构造，而是移动构造函数
    // 拷贝构造函数会重新复制一份，而移动构造函数只会转移资源的所有权
    
    // move(temp)是将资源的所有权移送给别人，要求temp是一个临时对象或者右值引用。 右值引用是&&，左值引用则是&
    // 同时可以结合移动构造函数和移动赋值函数来，与拷贝构造拷贝赋值没有很大的区别，移动构造、赋值是&&，而拷贝是&
    return string(_buffer.begin(), _buffer.begin() + length);
}

// 从字节流的输出端移除指定长度的字节数据
void ByteStream::pop_output(const size_t len) { 
    // 要移除的字节长度
    size_t length = len;
    // 如果要移除的长度超过了缓冲区的大小
    if (length > _buffer.size()){
        length = _buffer.size();
    }
    // 累加读取的字节数
    _read_count += length;
    // 循环移除缓冲区的前 length 个字符
    while(length--){
        _buffer.pop_front();
    }
    return ;
}

// 标记输入结束
void ByteStream::end_input() {
    _input_ended_flag = true;
}

// 判断输入是否结束
bool ByteStream::input_ended() const {
    return _input_ended_flag;
}

// 获取字节流缓冲区的当前大小
size_t ByteStream::buffer_size() const {
    return _buffer.size();
}

// 判断字节流缓冲区是否为空
bool ByteStream::buffer_empty() const {
    return _buffer.size() == 0;
}

// 判断是否到达字节流的末尾（缓冲区为空且输入结束）
bool ByteStream::eof() const {
    return buffer_empty() && input_ended();
}

// 获取已经写入字节流的总字节数
size_t ByteStream::bytes_written() const {
    return _write_count;
}

// 获取已经从字节流中读取的总字节数
size_t ByteStream::bytes_read() const {
    return _read_count;
}

// 获取字节流的剩余容量
size_t ByteStream::remaining_capacity() const {
    return _capacity - _buffer.size();
}
