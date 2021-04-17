#define _CRT_SECURE_NO_WARNINGS 1
#include"CentralCache.h"
#include"Pagecache.h"
 

//��spanlist/pagecache�л�ȡһ��span
Span* CentralCache::GetOneSpan(size_t size)
{
	//��ȡһ���ж����span
	size_t index = Sizeclass::ListIndex(size);
	//����spanlist
	SpanList& spanlist = _spanlists[index];
	Span* it = spanlist.Begin();
	while (it != spanlist.End())
	{
		if (!it->_freelist.Empty())//span�д��ڵȴ�ʹ�õ��ڴ棬��ȡ������
		{
			return it;
		}
		else
		{
			it = it->_next;
		}
	}
	//��ʱ��centralcache��û��������Ҫ���ڴ�Ķ�����Ҫ��pagecache��ȥȡ
	size_t numpage = Sizeclass::NumMovePage(size);
	Span* span = Pagecache::GetInstance().NewSpan(numpage);

	//��span������size��С�и�ҵ�span��freelist���С�
	char* start = (char*)(span->_pageid << 12);//����12�൱�ڳ���4K
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
// �����Ļ����ȡһ������Ϊnum,��СΪsize�Ķ����threadcache
//��Щ�ڴ������һ���Σ���start��ʼ����end����
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t num, size_t size)
{
	//��ȡһ���ж����span
	size_t index = Sizeclass::ListIndex(size);
	SpanList& spanlist = _spanlists[index];
	spanlist.Lock();//�ӻ�����

	//��ȡһ���ж����span
	Span* span = GetOneSpan(size); 
	FreeList& freelist = span->_freelist;
	size_t actuallNum = freelist.PopRange(start, end, num);
	span->_usecount += actuallNum;

	spanlist.UnLock();
	return actuallNum;
}

//���ڴ�����ͷŹ�һ������ʱ���ͽ���黹��span
//������СΪsize,startָ��黹���ڴ����Ŀ�ʼ
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
		//��ʾ��ǰspan�г�ȥ���ڴ����ȫ�����أ����Խ�span����page cache�����кϲ��������ڴ���Ƭ��
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


