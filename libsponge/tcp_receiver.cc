#include "tcp_receiver.hh"

#include <optional>

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;
// 函数设计思路：
	// 1. if(SYN段)
	// 		a. 如果是SYN段，且没接收过SYN段，则对相关变量进行设置，如SYN_flag=true，_base=1等
	//		b. 如果是SYN段，但是已经接收过SYN段，则直接丢弃 return 
	// 2. else if(!SYN_flag)，且之前没有接受过，则直接丢弃 return 
	// 3. else ：即进来的不是SYN段，且已经接收过SYN段，则代表进来的是正常数据段，因此计算绝对序列号
	// 4. 计算有效载荷
	// 5. if(fin) 
	//		a. if(FIN_flag) return
	//		b. else 设置FIN_flag
	// 6. else if(判断是否是空的ACK段/数据段) return 
	// 7. else if(判断是否在窗口)
	// 8. 重组数据，由于重组器中，SYN/FIN不会消耗序列号，因此绝对序列号会比stream index多1个，因此在重组时，stream index = abs_seqno -1
	// 9. 移动窗口，由于8，因此，_base = 重组器的head_index + 1;
	// 10. 如果流结束了，由于FIN会消耗一个序列号，因此_base++;
// 处理接收到的 TCP 分段
bool TCPReceiver::segment_received(const TCPSegment &seg) {
    bool ret = false;  // 用于标记分段是否被成功处理
    static size_t abs_seqno = 0; // 存储绝对序列号，初始化为 0
    size_t length;  // 存储当前分段在序列号空间中的长度

    // 处理 SYN 标志
    if(seg.header().syn){
        // 如果 SYN 标志已经被设置过，说明之前已经收到过 SYN 分段，直接返回 false
        if(_syn_flag){
            return false;
        }

        _syn_flag = true;  // 设置 SYN 标志，表示已经收到 SYN 分段
        ret = true;        // 标记分段处理成功
        _isn = seg.header().seqno.raw_value();  // 记录初始序列号（ISN）
        abs_seqno = 1;     // 收到 SYN 后，绝对序列号从 1 开始
        _base = 1;         // 接收窗口的起始位置设置为 1
        length = seg.length_in_sequence_space() - 1;  // 减去 SYN 标志占用的一个序列号
        // 如果除去 SYN 后没有有效数据，直接返回 true
        if(length == 0){
            return true;
        }
    }
    // 如果还没有收到 SYN 分段，忽略该分段
    else if (!_syn_flag){
        return false;
    } 
    // 已经收到 SYN 分段，将相对序列号转换为绝对序列号
    else{
        abs_seqno = unwrap(WrappingInt32(seg.header().seqno.raw_value()), WrappingInt32(_isn), abs_seqno);
        length = seg.length_in_sequence_space();  // 获取当前分段在序列号空间中的长度
    }

    // 处理 FIN 标志
    if(seg.header().fin){
        // 如果 FIN 标志已经被设置过，说明之前已经收到过 FIN 分段，直接返回 false
        if(_fin_flag){
            return false;
        }
        _fin_flag = true;  // 设置 FIN 标志，表示已经收到 FIN 分段 
        ret = true;        // 标记分段处理成功
    }
    // 处理长度为 0 且序列号等于接收窗口起始位置的分段
    else if (seg.length_in_sequence_space() == 0 && abs_seqno == _base){
        return true;
    } 
    // 检查分段是否在接收窗口之外
    else if (abs_seqno >= _base + window_size() || abs_seqno + length <= _base){
        if(!ret)
            return false;
    }

    // 将分段的有效载荷数据推送给重组器进行处理
    // 开始重组数据，注意abs_seqno是TCP绝对序列号，会计算SYN，而此时我们需要的索引是针对流的，而流忽略了SYN，因此需要-1.
    _reassembler.push_substring(seg.payload().copy(), abs_seqno - 1, seg.header().fin);

    // 更新接收窗口的起始位置 但是窗口的绝对序列不会忽略SYN，而流重组器会忽略SYN，因此为head_index+1，
    _base = _reassembler.head_index() + 1;  
    // 如果重组器的输入已经结束（收到 FIN 且所有数据都已重组），接收窗口起始位置加 1
    if(_reassembler.input_ended())
        // FIN会消耗一个序列
        _base++;
    return true;
}


optional<WrappingInt32> TCPReceiver::ackno() const { 
    /*在TCPReceiver类当中，我们会使用_base来作为窗口的左边界，即下一个要处理的字节的绝对序号。
	  由于在TCP三次握手中，SYN会消耗一个序列号，因此_base一定会>0，也因此，当_base==0时，连接一定还没建立。*/

    // 如果接收窗口起始位置大于 0，将其转换为相对序列号并返回
    if(_base > 0)
        return WrappingInt32(wrap(_base, WrappingInt32(_isn)));
    // 否则返回 std::nullopt，表示没有有效的确认号
    else
        return std::nullopt;
}

// 获取接收窗口的大小
size_t TCPReceiver::window_size() const { 
    // 接收窗口大小等于总容量减去重组器输出流缓冲区中已有的数据量
    return _capacity - _reassembler.stream_out().buffer_size(); 
}
