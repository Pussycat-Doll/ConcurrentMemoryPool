#pragma once
#include"Common.h"
class CentralCache
{
public:
	// 从中心缓存获取数量为num,大小为size的对象给threadcache
	//这些内存对象是一整段，以start开始，以end结束
	size_t FetchRangeObj(void*& start, void*& end, size_t num, size_t size);

	//当内存对象释放够一定数量时，就将其归还到span
	void ReleaseListToSpans(void* start,size_t size);

	//从sapnlist 或者 pagecache中获取一个span
	Span* GetOneSpan(size_t size);
	static CentralCache& GetInsatnce()
	{
		static CentralCache centralcacheInst;//多个线程共享一个CentralCache
		return centralcacheInst;
	}

private:
	CentralCache()
	{}
	CentralCache(const CentralCache&) = delete;
	SpanList _spanlists[NFREELIST];
};
