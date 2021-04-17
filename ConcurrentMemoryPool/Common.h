#pragma once

#include<iostream>
#include<assert.h>
#include<map>
#include<unordered_map>
#include<thread>
#include<mutex>
#ifdef _WIN32
#include<windows.h>
#endif // _WIN32

using std::cout;
using std::endl;

const size_t MAX_SIZE = 64 * 1024;//64k 
const size_t NFREELIST = MAX_SIZE / 8;//freelist的大小
const size_t MAX_PAGES = 129;//可申请页数的最大值(给129，是方便下标对齐，1对准下标1,128对准下标128)
const size_t PAGE_SHIFT = 12; //4k也就是（1>>12）
inline void*& NextObj(void* obj)//获取下一个对象，下一个对象存在头四个字节(32位)/头八个字节(64位)之中
{
	return *((void**)obj);//对void**解引用，返回void*的大小，32位平台4个字节，64位平台8个字节
}
class FreeList
{
public:
	void Push(void* obj)//头插，归还对象
	{
		NextObj(obj) = _freelist;
		_freelist = obj;
		++_num;
	}
	void PushRange(void* head, void* tail,size_t num)//插入一连串的对象
	{
		NextObj(tail) = _freelist;
		_freelist = head;
		_num += num;
	}
	size_t PopRange(void*& start, void*& end, size_t num)//返回实际拿到的内存对象个数
	{
		size_t actuallNum = 0;
		void* prev = nullptr;
		void* cur = _freelist;
		for (; actuallNum < num && cur != nullptr; ++actuallNum)//包含了实际个数<=想拿到个数的情况
		{
			prev = cur;
			cur = NextObj(cur);
		}
		start = _freelist;
		end = prev;
		_freelist = cur;
		_num -= actuallNum;
		return actuallNum;
	}
	void* Pop()//头删，拿一个对象
	{
		void* obj = _freelist;
		_freelist = NextObj(obj); 
		--_num;
		return obj;
	}
	bool Empty()//判空
	{
		return _freelist == nullptr;
	}
	size_t Num()
	{
		return _num;
	}
	void Clear()
	{
		_freelist = nullptr;
		_num = 0;
	}
private:
	void* _freelist = nullptr;
	size_t _num = 0;//内存对象的个数
};

class Sizeclass
{
public:

	
	/*static size_t RoungUP(size_t size)//根据大小向上对齐(size=5,return 8;size=8,return 8;size=10,return 16)
	{
		if (size % 8 != 0)
			return (size / 8 + 1) * 8;
		else
			return size;
	}
	static size_t ListIndex(size_t size)//计算其在自由链表中的下标
	{
		if (size % 8 == 0)
			return size / 8 - 1;
		else
			return size / 8;
	}内存池是比较注重效率的，但是除法(用减法来做的)和取模运算效率是比较低的，所以要对其进行优化，如下*/
	//按照8byte对齐
	//[9,16]+7=[16,23]-->二进制表示：[1 0000,1 0111] 16 8 4 2 1 & ~7(1 1000) --> (1 0000)即为16
	//按照16byte对齐
	//[1,16]+15=[16,31]-->二进制表示：[01 0000,01 1111] 32 16 8 4 2 1 & ~15(11 0000) -->(01 0000)即为16
	//[17,32]+15=[32,47]-->二进制表示：[010 0001,010 0111] 64 32 16 8 4 2 1 & ~15(111 0000) -->(010 0000)即为32
	static size_t _RoundUP(size_t size, size_t alignment)//alignment为对齐数，size要申请哒大小
	{
		return (size + alignment - 1)&(~(alignment - 1));
	}
	/*
	* 为什么要对齐呢？申请5字节，不会就只给5字节，threadcache会给你8字节，根据你要申请哒
	* 的大小，我会对齐一个一定内 存碎片浪费的数目上
	控制在最多[1%,10%]左右的内碎片浪费   分子--最大的浪费数，分母--符合该对齐数的最大/最小字节
	[1,128] 8byte对齐 freelist[0,16) 浪费率--7/128 = 0.05      
	[129,1024] 16byte对齐 freelist[16,72)浪费率--15/(128+16) = 0.10   15/1024 = 0.01   (128+16)--128是已经全部对齐到8字节的+符合该对齐数的最小字节数
	[1025,8*1024] 128byte对齐 freelist[72,128)浪费率--127/(1024+128) = 0.11  127/8192 = 0.01
	[8*1024 + 1,64*1024] 1024byte对齐 freelist[128,184)浪费率--1023/(8*1024+1024) = 0.11   1023/64*2014 = 0.01 
	*/
	static inline size_t RoundUp(size_t size)
	{
		assert(size <= MAX_SIZE);
		if (size <= 128)
			return _RoundUP(size, 8);
		else if (size <= 1024)
			return _RoundUP(size, 16);
		else if (size <= 1024 * 8)
			return _RoundUP(size, 128);
		else if (size <= 1024 * 64)
			return _RoundUP(size, 1024);
		else
			return -1;
	}
	//[0,8]字节对应自由链表中的下标为0
	//[9,16]字节对应自由链表中的下标为1
	//alignment_shift是对齐数对应2的幂，如：8对应2的幂为3
	//(1<<alignment_shift)为对齐数
	//>> alignment_shift即为除以对齐数，得到的结果为正确下标的后一位
	//[9,16] + 7 == [16,23] -->  [16,23]-[1 0000, 1 0111]>>3 == 0 0010 == 2 
	//即正确下标的下一位
	//最后再减一减到正确下标
	static size_t _ListIndex(size_t size, int alignment_shift)
	{
		return ((size + (1 << alignment_shift) - 1) >> alignment_shift) - 1;
	}
	/*
	[1,128]字节以 8byte 对齐 freelist下标范围[0,16) <--- 128/8 = 16
	[129,1024]字节以 16byte 对齐 freelist下标范围[16,72) <--- (1024-128)/16 = 56  56 + 16= 72
	[1025,8*1024]字节以 128byte 对齐 freelist下标范围[72,128) <--- (8*1024-1024)/128  = 56  56 + 72 = 128
	[8*1024 + 1,64*1024]字节以 1024byte 对齐 freelist下标范围[128,184)<---(64*1024 - 8*1024)/1024 = 56  56 + 128 = 184
	*/
	static size_t ListIndex(size_t size)
	{
		assert(size <= MAX_SIZE);
		//每个区间有多少个链表
		static int group_array[4] = { 16, 56, 56, 56 }; //计算下标
		if (size <= 128)//以8byte对齐   
			return _ListIndex(size, 3);
		else if (size <= 1024)//以16byte对齐 
			return _ListIndex(size - 128, 4) + group_array[0];
		else if (size <= 8 * 1024)//以128byte对齐
			return _ListIndex(size - 1024, 7) + group_array[0] + group_array[1];
		else if (size <= 64 * 1024)//以1024byte对齐
			return _ListIndex(size - 8192, 10) + group_array[0] + group_array[1] + group_array[2];
		else
			return -1;
	}
	//[2,512]字节,申请的空间比较小，就会给得多一点；申请得空间比较大，就会给得少点
	static size_t NumMoveSize(size_t size)//size---单个对象的大小
	{
		if (size == 0)//防止分子为零的情况出现
			return 0;
		int num = MAX_SIZE / size;
		if (num < 2)//最少一次取两个内存对象
			num = 2;

		if (num > 512)
			num = 512;//最多给512个内存对象
		return num;
	}
	//计算一次向系统获取几个页
	static size_t NumMovePage(size_t size)
	{
		size_t num = NumMoveSize(size);//申请的内存对象的个数
		size_t npage = num*size;//总的字节数

		npage >>= 12;//2^12=4k一页(右移12页，就是除以4k)
		if (npage == 0)
			npage = 1;//最少返回一页
		return npage;
		

	}

};
//////////////////////////////////////////////////////////////////////
//span 跨度--管理以页为单位的内存对象，本质是方便做合并，解决内存碎片
//当是在32位平台时，int还足够去显示页码 2^32/2^12(4k) == 2^20 页
//但是在64位平台中    2^64/2^12(4k) == 2^52，用int不能容纳全部的页码
//所以对于_pageid的类型，需要用一个条件编译来选择合适的一种
//long long         ----> 2^64
//unsigned long long----> 2^63 (去掉了负数也就是一半)
//////////////////////////////////////////////////////////////////////////////
//针对windows平台
#ifdef _WIN32//32位平台
typedef unsigned int PAGE_ID;
#else//64位平台，如果要在Linux平台跑该程序，还需要再单独处理再写另一种适合Linux平台的
typedef unsigned long long PAGE_ID; 
#endif

struct Span
{
	PAGE_ID _pageid = 0;//页号
	PAGE_ID _pagesize = 0;//页的数量

	FreeList _freelist;//内存对象的自由链表
	size_t _objSize = 0;//自由链表对象大小
	int _usecount = 0;//内存块对象使用的数量
	//size_t objectsize;//对象大小

	//双向循环的，方便取走或插入其中的一个
	Span* _next = nullptr;
	Span* _prev = nullptr;
};
class SpanList
{
public:
	SpanList()
	{
		_head = new Span;
		_head->_next = _head;
		_head->_prev = _head;
	}
	Span* Begin()
	{
		return _head->_next;
	}
	Span* End()//带头双向循环链表的end就是_head
	{
		return _head;
	}
	void PushFront(Span* newspan)//在一个带头双向循环中头插，就是在头结点的后一位进行插入
	{
		Insert(_head->_next, newspan);
	}
	void PopFront()//头删
	{
		Erase(_head->_next);
	}
	void PushBack(Span* newspan)//尾插
	{
		Insert(_head, newspan);
	}
	void PopBack()//尾删
	{
		Erase(_head->_prev);
	}
	void Insert(Span* pos, Span* newspan)
	{
		//prev newspan pos
		Span* prev = pos->_prev;
		prev->_next = newspan;
		newspan->_prev = prev; 
		newspan->_next = pos;
		pos->_prev = newspan;
	}
	void Erase(Span* pos)
	{
		//prev pos next
		assert(pos != _head);//不能把头结点删除
		Span* prev = pos->_prev;
		Span* next = pos->_next;
		prev->_next = next;
		next->_prev = prev;
		//delete pos;在这里要删除的span其实不是真的将其delete掉，而是将它还给pagecache,让pagecache对其进行合并
	}
	bool Empty()
	{
		return Begin() == End();
	}
	void Lock()
	{
		_mtx.lock();
	}
	void UnLock()
	{
		_mtx.unlock();
	}
private:
	Span* _head;
	std::mutex _mtx;
};

//向系统申请numpage页内存挂到自 由链表
inline static void* SystemAlloc(size_t numpage)
{
#ifdef _WIN32
	void* ptr = VirtualAlloc(0, numpage * (1 << PAGE_SHIFT), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	//brk(),mmap等
#endif // _WIN32
	if (ptr == nullptr)
		throw std::bad_alloc();
	return ptr;
}
inline static void SystemFree(void* ptr)
{
#ifdef _WIN32
	VirtualFree(ptr,0,MEM_RELEASE);
	//brk(),mmap等
#else
#endif // _WIN32

}