#pragma once
#include"Common.h"
class CentralCache
{
public:
	// �����Ļ����ȡ����Ϊnum,��СΪsize�Ķ����threadcache
	//��Щ�ڴ������һ���Σ���start��ʼ����end����
	size_t FetchRangeObj(void*& start, void*& end, size_t num, size_t size);

	//���ڴ�����ͷŹ�һ������ʱ���ͽ���黹��span
	void ReleaseListToSpans(void* start,size_t size);

	//��sapnlist ���� pagecache�л�ȡһ��span
	Span* GetOneSpan(size_t size);
	static CentralCache& GetInsatnce()
	{
		static CentralCache centralcacheInst;//����̹߳���һ��CentralCache
		return centralcacheInst;
	}

private:
	CentralCache()
	{}
	CentralCache(const CentralCache&) = delete;
	SpanList _spanlists[NFREELIST];
};
