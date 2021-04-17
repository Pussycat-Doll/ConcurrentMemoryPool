#pragma once
#include"Common.h"
class Pagecache
{
public:
	Span* NewSpan(size_t num);//numҳ��span
	Span* _NewSpan(size_t num);//numҳ��span
	Span* GetIdToSpan(PAGE_ID id);
	void ReleaseSpanToPageCache(Span* span);
	
	static Pagecache& GetInstance()//����ģʽ��������ģʽ
	{
		static Pagecache pagecaCheInst;
		return pagecaCheInst;
	}
private:
	Pagecache()
	{}
	Pagecache(const Pagecache&) = delete;
	//���캯�����������캯��˽�л�����֤ȫ��ֻ��һ��
	SpanList _spanlists[MAX_PAGES];
	std::unordered_map<PAGE_ID, Span*> _idSpanMap;//����span��������ҳ�ŵ�ӳ��
	std::mutex _mtx;
};
