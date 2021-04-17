#define _CRT_SECURE_NO_WARNINGS 1
#include"Pagecache.h"


Span* Pagecache::_NewSpan(size_t numpage)
{
	//_spanlists[numpage].Lock();���ܼ��ں����ڲ�����Ϊ�����ݹ�
	if (!_spanlists[numpage].Empty())//����ͬ��С��span,����ֱ��ȡ��
	{
		Span* span = _spanlists[numpage].Begin();
		_spanlists[numpage].PopFront();
		return span;
	}
	//û����ͬ��С��Ҫ�ұ����Ľ����и�
	for (size_t i = numpage + 1; i < MAX_PAGES; ++i)
	{
		if (!_spanlists[i].Empty())
		{
			//���ѵĹ���
			Span* span = _spanlists[i].Begin();
			_spanlists[i].PopFront();

			Span* splitspan = new Span;//����span
			splitspan->_pageid = span->_pageid + span->_pagesize - numpage;//��ʼҳ�ţ�ҳ��ƫ��
			splitspan->_pagesize = numpage;
			for (PAGE_ID i = 0; i < numpage; ++i)
			{
				_idSpanMap[splitspan->_pageid + i] = splitspan;
			}
			span->_pagesize -= numpage;
			_spanlists[span->_pagesize].PushFront(span);
			return splitspan;
		}
	}
	//���涼û�ҵ�����ϵͳ�������ҳ��Ȼ��ҽ�span
	void* ptr = SystemAlloc(MAX_PAGES - 1);
	Span* bigspan = new Span;

	bigspan->_pageid = (PAGE_ID)ptr >> PAGE_SHIFT;
	bigspan->_pagesize = MAX_PAGES - 1;

	for (PAGE_ID i = 0; i < bigspan->_pagesize; ++i)
	{
		_idSpanMap[bigspan->_pageid + i] = bigspan;
		//_idSpanMap.insert(std::make_pair(bigspan->_pageid + i, bigspan));
	}

	_spanlists[bigspan->_pagesize].PushFront(bigspan);
	//�ٽ��е����Լ������зָ�
	return _NewSpan(numpage);
}
Span* Pagecache::NewSpan(size_t numpage)
{
	_mtx.lock();
	Span* span = _NewSpan(numpage);
	_mtx.unlock();
	return span;
}

Span* Pagecache::GetIdToSpan(PAGE_ID id)
{
	//std::map<PAGE_ID, Span*>::iterator it = _idSpanMap.find(id);
	auto it = _idSpanMap.find(id);
	if (it != _idSpanMap.end())
		return it->second;
	else
		return nullptr;
}
void Pagecache::ReleaseSpanToPageCache(Span* span)
{
	//ҳ�ĺϲ�
	//1.��ǰ�ϲ�
	while (1)
	{
		PAGE_ID prevPageId = span->_pageid - 1;	//ǰһҳ��ҳ��
		auto pit = _idSpanMap.find(prevPageId);
		if (pit == _idSpanMap.end())//������ǰ���ҳ
		{
			break;
		}

		Span* prevSpan = pit->second;
		if (prevSpan->_usecount != 0)//˵��ǰһ��ҳ����ʹ�ã����ܺϲ�
		{
			break;
		}
		if (span->_pagesize + prevSpan->_pagesize >= MAX_PAGES)//�ϲ�֮������ҳ�����ܳ������ҳ���������ҳ�Ͳ�Ҫ�ϲ�����
		{
			break;
		}
		//ǰһ��ҳ���Ժϲ�
		span->_pageid = prevSpan->_pageid;
		span->_pagesize += prevSpan->_pagesize;
		//����map��ӳ��
		for (PAGE_ID i = 0; i < prevSpan->_pagesize; ++i)
		{
			_idSpanMap[prevSpan->_pageid + i] = span;
		}
		_spanlists[prevSpan->_pagesize].Erase(prevSpan);
		delete prevSpan;
	}
	//2.���ϲ�
	while (1)
	{
		PAGE_ID nextPageId = span->_pageid + span->_pagesize;
		auto nextIt = _idSpanMap.find(nextPageId);
		if (nextIt == _idSpanMap.end())//�����ҳ�Ų�����
			break;
		Span* nextSpan = nextIt->second;
		if (nextSpan->_usecount != 0)//����ҳ�Ż���ʹ��
			break;
		if (span->_pagesize + nextSpan->_pagesize >= MAX_PAGES)//
			break;
		span->_pagesize += nextSpan->_pagesize;

		for (PAGE_ID i = 0; i < nextSpan->_pagesize; ++i)
		{
			_idSpanMap[nextSpan->_pageid + i] = span;
		}
		_spanlists[nextSpan->_pagesize].Erase(nextSpan);

		delete nextSpan;
	}
	_spanlists[span->_pagesize].PushFront(span);
}