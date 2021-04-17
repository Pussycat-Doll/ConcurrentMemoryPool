#define _CRT_SECURE_NO_WARNINGS 1
#include"Pagecache.h"


Span* Pagecache::_NewSpan(size_t numpage)
{
	//_spanlists[numpage].Lock();不能加在函数内部，因为这里会递归
	if (!_spanlists[numpage].Empty())//有相同大小的span,可以直接取走
	{
		Span* span = _spanlists[numpage].Begin();
		_spanlists[numpage].PopFront();
		return span;
	}
	//没有相同大小的要找比其大的进行切割
	for (size_t i = numpage + 1; i < MAX_PAGES; ++i)
	{
		if (!_spanlists[i].Empty())
		{
			//分裂的过程
			Span* span = _spanlists[i].Begin();
			_spanlists[i].PopFront();

			Span* splitspan = new Span;//分裂span
			splitspan->_pageid = span->_pageid + span->_pagesize - numpage;//起始页号＋页面偏移
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
	//上面都没找到，向系统申请最大页，然后挂进span
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
	//再进行调用自己，进行分割
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
	//页的合并
	//1.向前合并
	while (1)
	{
		PAGE_ID prevPageId = span->_pageid - 1;	//前一页的页号
		auto pit = _idSpanMap.find(prevPageId);
		if (pit == _idSpanMap.end())//不存在前面的页
		{
			break;
		}

		Span* prevSpan = pit->second;
		if (prevSpan->_usecount != 0)//说明前一个页还在使用，不能合并
		{
			break;
		}
		if (span->_pagesize + prevSpan->_pagesize >= MAX_PAGES)//合并之后的最大页数不能超过最大页，超出最大页就不要合并。、
		{
			break;
		}
		//前一个页可以合并
		span->_pageid = prevSpan->_pageid;
		span->_pagesize += prevSpan->_pagesize;
		//更新map的映射
		for (PAGE_ID i = 0; i < prevSpan->_pagesize; ++i)
		{
			_idSpanMap[prevSpan->_pageid + i] = span;
		}
		_spanlists[prevSpan->_pagesize].Erase(prevSpan);
		delete prevSpan;
	}
	//2.向后合并
	while (1)
	{
		PAGE_ID nextPageId = span->_pageid + span->_pagesize;
		auto nextIt = _idSpanMap.find(nextPageId);
		if (nextIt == _idSpanMap.end())//后面的页号不存在
			break;
		Span* nextSpan = nextIt->second;
		if (nextSpan->_usecount != 0)//后面页号还在使用
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