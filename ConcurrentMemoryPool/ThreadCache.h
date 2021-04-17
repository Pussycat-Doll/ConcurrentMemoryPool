#pragma once
#include"Common.h"

class ThreadCache
{
public:
	//申请内存和释放内存
	void* Allocte(size_t size);
	void Deallocte(void* ptr, size_t size);
	//从中心缓存获取内存
	void* FetchFromCentralCache(size_t index);
	//如果自由链表中对象超过一定长度就要释放给中心缓存
	void ListTooLong(FreeList& freeList, size_t num,size_t size);
private:
	FreeList _freelist[NFREELIST];//存放指针的链表(映射表) 
};

_declspec (thread) static ThreadCache*  pThreadCache = nullptr;