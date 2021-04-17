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
const size_t NFREELIST = MAX_SIZE / 8;//freelist�Ĵ�С
const size_t MAX_PAGES = 129;//������ҳ�������ֵ(��129���Ƿ����±���룬1��׼�±�1,128��׼�±�128)
const size_t PAGE_SHIFT = 12; //4kҲ���ǣ�1>>12��
inline void*& NextObj(void* obj)//��ȡ��һ��������һ���������ͷ�ĸ��ֽ�(32λ)/ͷ�˸��ֽ�(64λ)֮��
{
	return *((void**)obj);//��void**�����ã�����void*�Ĵ�С��32λƽ̨4���ֽڣ�64λƽ̨8���ֽ�
}
class FreeList
{
public:
	void Push(void* obj)//ͷ�壬�黹����
	{
		NextObj(obj) = _freelist;
		_freelist = obj;
		++_num;
	}
	void PushRange(void* head, void* tail,size_t num)//����һ�����Ķ���
	{
		NextObj(tail) = _freelist;
		_freelist = head;
		_num += num;
	}
	size_t PopRange(void*& start, void*& end, size_t num)//����ʵ���õ����ڴ�������
	{
		size_t actuallNum = 0;
		void* prev = nullptr;
		void* cur = _freelist;
		for (; actuallNum < num && cur != nullptr; ++actuallNum)//������ʵ�ʸ���<=���õ����������
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
	void* Pop()//ͷɾ����һ������
	{
		void* obj = _freelist;
		_freelist = NextObj(obj); 
		--_num;
		return obj;
	}
	bool Empty()//�п�
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
	size_t _num = 0;//�ڴ����ĸ���
};

class Sizeclass
{
public:

	
	/*static size_t RoungUP(size_t size)//���ݴ�С���϶���(size=5,return 8;size=8,return 8;size=10,return 16)
	{
		if (size % 8 != 0)
			return (size / 8 + 1) * 8;
		else
			return size;
	}
	static size_t ListIndex(size_t size)//�����������������е��±�
	{
		if (size % 8 == 0)
			return size / 8 - 1;
		else
			return size / 8;
	}�ڴ���ǱȽ�ע��Ч�ʵģ����ǳ���(�ü���������)��ȡģ����Ч���ǱȽϵ͵ģ�����Ҫ��������Ż�������*/
	//����8byte����
	//[9,16]+7=[16,23]-->�����Ʊ�ʾ��[1 0000,1 0111] 16 8 4 2 1 & ~7(1 1000) --> (1 0000)��Ϊ16
	//����16byte����
	//[1,16]+15=[16,31]-->�����Ʊ�ʾ��[01 0000,01 1111] 32 16 8 4 2 1 & ~15(11 0000) -->(01 0000)��Ϊ16
	//[17,32]+15=[32,47]-->�����Ʊ�ʾ��[010 0001,010 0111] 64 32 16 8 4 2 1 & ~15(111 0000) -->(010 0000)��Ϊ32
	static size_t _RoundUP(size_t size, size_t alignment)//alignmentΪ��������sizeҪ�����մ�С
	{
		return (size + alignment - 1)&(~(alignment - 1));
	}
	/*
	* ΪʲôҪ�����أ�����5�ֽڣ������ֻ��5�ֽڣ�threadcache�����8�ֽڣ�������Ҫ������
	* �Ĵ�С���һ����һ��һ���� ����Ƭ�˷ѵ���Ŀ��
	���������[1%,10%]���ҵ�����Ƭ�˷�   ����--�����˷�������ĸ--���ϸö����������/��С�ֽ�
	[1,128] 8byte���� freelist[0,16) �˷���--7/128 = 0.05      
	[129,1024] 16byte���� freelist[16,72)�˷���--15/(128+16) = 0.10   15/1024 = 0.01   (128+16)--128���Ѿ�ȫ�����뵽8�ֽڵ�+���ϸö���������С�ֽ���
	[1025,8*1024] 128byte���� freelist[72,128)�˷���--127/(1024+128) = 0.11  127/8192 = 0.01
	[8*1024 + 1,64*1024] 1024byte���� freelist[128,184)�˷���--1023/(8*1024+1024) = 0.11   1023/64*2014 = 0.01 
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
	//[0,8]�ֽڶ�Ӧ���������е��±�Ϊ0
	//[9,16]�ֽڶ�Ӧ���������е��±�Ϊ1
	//alignment_shift�Ƕ�������Ӧ2���ݣ��磺8��Ӧ2����Ϊ3
	//(1<<alignment_shift)Ϊ������
	//>> alignment_shift��Ϊ���Զ��������õ��Ľ��Ϊ��ȷ�±�ĺ�һλ
	//[9,16] + 7 == [16,23] -->  [16,23]-[1 0000, 1 0111]>>3 == 0 0010 == 2 
	//����ȷ�±����һλ
	//����ټ�һ������ȷ�±�
	static size_t _ListIndex(size_t size, int alignment_shift)
	{
		return ((size + (1 << alignment_shift) - 1) >> alignment_shift) - 1;
	}
	/*
	[1,128]�ֽ��� 8byte ���� freelist�±귶Χ[0,16) <--- 128/8 = 16
	[129,1024]�ֽ��� 16byte ���� freelist�±귶Χ[16,72) <--- (1024-128)/16 = 56  56 + 16= 72
	[1025,8*1024]�ֽ��� 128byte ���� freelist�±귶Χ[72,128) <--- (8*1024-1024)/128  = 56  56 + 72 = 128
	[8*1024 + 1,64*1024]�ֽ��� 1024byte ���� freelist�±귶Χ[128,184)<---(64*1024 - 8*1024)/1024 = 56  56 + 128 = 184
	*/
	static size_t ListIndex(size_t size)
	{
		assert(size <= MAX_SIZE);
		//ÿ�������ж��ٸ�����
		static int group_array[4] = { 16, 56, 56, 56 }; //�����±�
		if (size <= 128)//��8byte����   
			return _ListIndex(size, 3);
		else if (size <= 1024)//��16byte���� 
			return _ListIndex(size - 128, 4) + group_array[0];
		else if (size <= 8 * 1024)//��128byte����
			return _ListIndex(size - 1024, 7) + group_array[0] + group_array[1];
		else if (size <= 64 * 1024)//��1024byte����
			return _ListIndex(size - 8192, 10) + group_array[0] + group_array[1] + group_array[2];
		else
			return -1;
	}
	//[2,512]�ֽ�,����Ŀռ�Ƚ�С���ͻ���ö�һ�㣻����ÿռ�Ƚϴ󣬾ͻ�����ٵ�
	static size_t NumMoveSize(size_t size)//size---��������Ĵ�С
	{
		if (size == 0)//��ֹ����Ϊ����������
			return 0;
		int num = MAX_SIZE / size;
		if (num < 2)//����һ��ȡ�����ڴ����
			num = 2;

		if (num > 512)
			num = 512;//����512���ڴ����
		return num;
	}
	//����һ����ϵͳ��ȡ����ҳ
	static size_t NumMovePage(size_t size)
	{
		size_t num = NumMoveSize(size);//������ڴ����ĸ���
		size_t npage = num*size;//�ܵ��ֽ���

		npage >>= 12;//2^12=4kһҳ(����12ҳ�����ǳ���4k)
		if (npage == 0)
			npage = 1;//���ٷ���һҳ
		return npage;
		

	}

};
//////////////////////////////////////////////////////////////////////
//span ���--������ҳΪ��λ���ڴ���󣬱����Ƿ������ϲ�������ڴ���Ƭ
//������32λƽ̨ʱ��int���㹻ȥ��ʾҳ�� 2^32/2^12(4k) == 2^20 ҳ
//������64λƽ̨��    2^64/2^12(4k) == 2^52����int��������ȫ����ҳ��
//���Զ���_pageid�����ͣ���Ҫ��һ������������ѡ����ʵ�һ��
//long long         ----> 2^64
//unsigned long long----> 2^63 (ȥ���˸���Ҳ����һ��)
//////////////////////////////////////////////////////////////////////////////
//���windowsƽ̨
#ifdef _WIN32//32λƽ̨
typedef unsigned int PAGE_ID;
#else//64λƽ̨�����Ҫ��Linuxƽ̨�ܸó��򣬻���Ҫ�ٵ���������д��һ���ʺ�Linuxƽ̨��
typedef unsigned long long PAGE_ID; 
#endif

struct Span
{
	PAGE_ID _pageid = 0;//ҳ��
	PAGE_ID _pagesize = 0;//ҳ������

	FreeList _freelist;//�ڴ�������������
	size_t _objSize = 0;//������������С
	int _usecount = 0;//�ڴ�����ʹ�õ�����
	//size_t objectsize;//�����С

	//˫��ѭ���ģ�����ȡ�߻�������е�һ��
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
	Span* End()//��ͷ˫��ѭ�������end����_head
	{
		return _head;
	}
	void PushFront(Span* newspan)//��һ����ͷ˫��ѭ����ͷ�壬������ͷ���ĺ�һλ���в���
	{
		Insert(_head->_next, newspan);
	}
	void PopFront()//ͷɾ
	{
		Erase(_head->_next);
	}
	void PushBack(Span* newspan)//β��
	{
		Insert(_head, newspan);
	}
	void PopBack()//βɾ
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
		assert(pos != _head);//���ܰ�ͷ���ɾ��
		Span* prev = pos->_prev;
		Span* next = pos->_next;
		prev->_next = next;
		next->_prev = prev;
		//delete pos;������Ҫɾ����span��ʵ������Ľ���delete�������ǽ�������pagecache,��pagecache������кϲ�
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

//��ϵͳ����numpageҳ�ڴ�ҵ��� ������
inline static void* SystemAlloc(size_t numpage)
{
#ifdef _WIN32
	void* ptr = VirtualAlloc(0, numpage * (1 << PAGE_SHIFT), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
	//brk(),mmap��
#endif // _WIN32
	if (ptr == nullptr)
		throw std::bad_alloc();
	return ptr;
}
inline static void SystemFree(void* ptr)
{
#ifdef _WIN32
	VirtualFree(ptr,0,MEM_RELEASE);
	//brk(),mmap��
#else
#endif // _WIN32

}