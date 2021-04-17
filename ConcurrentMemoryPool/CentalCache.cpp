#define _CRT_SECURE_NO_WARNINGS 1
#include"CentralCache.h"
#include"Pagecache.h"
 

//从spanlist/pagecache中获取一个span
Span* CentralCache::GetOneSpan(size_t size)
{
	//获取一个有对象的span
	size_t index = Sizeclass::ListIndex(size);
	//遍历spanlist
	SpanList& spanlist = _spanlists[index];
	Span* it = spanlist.Begin();
	while (it != spanlist.End())
	{
		if (!it->_freelist.Empty())//span中存在等待使用的内存，就取出来用
		{
			return it;
		}
		else
		{
			it = it->_next;
		}
	}
	//此时，centralcache中没有我们需要的内存的对象，需要从pagecache中去取
	size_t numpage = Sizeclass::NumMovePage(size);
	Span* span = Pagecache::GetInstance().NewSpan(numpage);

	//把span对象按照size大小切割并挂到span的freelist当中。
	char* start = (char*)(span->_pageid << 12);//左移12相当于乘以4K
	char* end = start + (span->_pagesize << 12);
	while (start < end)
	{
		char* obj = start;
		start += size;
		span->_freelist.Push(obj);
	}
	span->_objSize = size;
	spanlist.PushFront(span);
	return span;
}
// 从中心缓存获取一定数量为num,大小为size的对象给threadcache
//这些内存对象是一整段，以start开始，以end结束
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t num, size_t size)
{
	//获取一个有对象的span
	size_t index = Sizeclass::ListIndex(size);
	SpanList& spanlist = _spanlists[index];
	spanlist.Lock();//加互斥锁

	//获取一个有对象的span
	Span* span = GetOneSpan(size); 
	FreeList& freelist = span->_freelist;
	size_t actuallNum = freelist.PopRange(start, end, num);
	span->_usecount += actuallNum;

	spanlist.UnLock();
	return actuallNum;
}

//当内存对象释放够一定数量时，就将其归还到span
//单个大小为size,start指向归还的内存对象的开始
void CentralCache::ReleaseListToSpans(void* start,size_t size)
{

	size_t index = Sizeclass::ListIndex(size);
	SpanList& spanlist = _spanlists[index];
	spanlist.Lock();
	while (start)
	{
		void* next = NextObj(start);
		PAGE_ID id = (PAGE_ID)start >> PAGE_SHIFT;
		Span* span = Pagecache::GetInstance().GetIdToSpan(id);
		span->_freelist.Push(start);
		span->_usecount--;
		//表示当前span切出去的内存对象全部返回，可以将span还给page cache，进行合并，减少内存碎片。
		if (span->_usecount == 0)
		{
			size_t index = Sizeclass::ListIndex(span->_objSize);
			_spanlists[index].Erase(span);
			span->_freelist.Clear();

			Pagecache::GetInstance().ReleaseSpanToPageCache(span);
		}
		start = next;
	}
	spanlist.UnLock();
}


