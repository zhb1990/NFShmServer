﻿// -------------------------------------------------------------------------
//    @FileName         :    NFCommon.h
//    @Author           :    xxxxx
//    @Date             :   xxxx-xx-xx
//    @Email			:    xxxxxxxxx@xxx.xxx
//    @Module           :    NFCore
//
// -------------------------------------------------------------------------

#pragma once

#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdarg.h>
#include <cassert>
#include <cstdio>
#include <string>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <map>
#include <stack>
#include <vector>
#include <regex>
#include "NFPlatform.h"

/////////////////////////////////////////////////
/**
* @file NFCommon.h
* @brief  帮助类,都是静态函数,提供一些常用的函数 .
*
*/
/////////////////////////////////////////////////

/**
* @brief  基础工具类，提供了一些非常基本的函数使用.
*
* 这些函数都是以静态函数提供。 包括以下几种函数:
*
* Trim类函数,大小写转换函数,分隔字符串函数（直接分隔字符串，
*
* 数字等）,时间相关函数,字符串转换函数,二进制字符串互转函数,
*
* 替换字符串函数,Ip匹配函数,判断一个数是否是素数等
*/
class _NFExport NFCommon
{
public:

	//操作bit
	static bool GetBit(uint32_t src, uint32_t pos);
	static void SetBit(uint32_t &src, uint32_t pos, bool flag);

	/**
	* @brief  去掉头部以及尾部的字符或字符串.
	*
	* @param sStr    输入字符串
	* @param s       需要去掉的字符
	* @param bChar   如果为true, 则去掉s中每个字符; 如果为false, 则去掉s字符串
	* @return string 返回去掉后的字符串
	*/
	static std::string trim(const std::string &sStr, const std::string &s = " \r\n\t", bool bChar = true);

	/**
	* @brief  去掉左边的字符或字符串.
	*
	* @param sStr    输入字符串
	* @param s       需要去掉的字符
	* @param bChar   如果为true, 则去掉s中每个字符; 如果为false, 则去掉s字符串
	* @return string 返回去掉后的字符串
	*/
	static std::string trimleft(const std::string &sStr, const std::string &s = " \r\n\t", bool bChar = true);

	/**
	* @brief  去掉右边的字符或字符串.
	*
	* @param sStr    输入字符串
	* @param s       需要去掉的字符
	* @param bChar   如果为true, 则去掉s中每个字符; 如果为false, 则去掉s字符串
	* @return string 返回去掉后的字符串
	*/
	static std::string trimright(const std::string &sStr, const std::string &s = " \r\n\t", bool bChar = true);

	/**
	* @brief  字符串转换成小写.
	*
	* @param sString  字符串
	* @return string  转换后的字符串
	*/
	static std::string lower(const std::string &sString);

	/**
	* @brief  字符串转换成大写.
	*
	* @param sString  字符串
	* @return string  转换后的大写的字符串
	*/
	static std::string upper(const std::string &sString);

	/**
	* @brief  字符串是否都是数字的.
	*
	* @param sString  字符串
	* @return bool    是否是数字
	*/
	static bool isdigit(const std::string &sInput);

	/**
	* @brief  字符串转换成时间结构.
	*
	* @param sString  字符串时间格式
	* @param sFormat  格式
	* @param stTm     时间结构
	* @return         0: 成功, -1:失败
	*/
	static int str2tm(const std::string &sString, const std::string &sFormat, struct tm &stTm);

	/**
	* @brief GMT格式的时间转化为时间结构
	*
	* eg.Sat, 06 Feb 2010 09:29:29 GMT, %a, %d %b %Y %H:%M:%S GMT
	*
	* 可以用mktime换成time_t, 但是注意时区 可以用mktime(&stTm)
	*
	* - timezone换成本地的秒(time(NULL)值相同) timezone是系统的 ,
	*
	* 需要extern long timezone;
	*
	* @param sString  GMT格式的时间
	* @param stTm     转换后的时间结构
	* @return         0: 成功, -1:失败
	*/
	static int strgmt2tm(const std::string &sString, struct tm &stTm);

	/**
	* @brief  时间转换成字符串.
	*
	* @param stTm     时间结构
	* @param sFormat  需要转换的目标格式，默认为紧凑格式
	* @return string  转换后的时间字符串
	*/
	static std::string tm2str(const struct tm &stTm, const std::string &sFormat = "%Y%m%d%H%M%S");

	/**
	* @brief  时间转换成字符串.
	*
	* @param t        时间结构
	* @param sFormat  需要转换的目标格式，默认为紧凑格式
	* @return string  转换后的时间字符串
	*/
	static std::string tm2str(const time_t &t, const std::string &sFormat = "%Y%m%d%H%M%S");

	/**
	* @brief  当前时间转换成紧凑格式字符串
	* @param sFormat 格式，默认为紧凑格式
	* @return string 转换后的时间字符串
	*/
	static std::string now2str(const std::string &sFormat = "%Y%m%d%H%M%S");

	/**
	* @brief  时间转换成GMT字符串，GMT格式:Fri, 12 Jan 2001 18:18:18 GMT
	* @param stTm    时间结构
	* @return string GMT格式的时间字符串
	*/
	static std::string tm2GMTstr(const struct tm &stTm);

	/**
	* @brief  时间转换成GMT字符串，GMT格式:Fri, 12 Jan 2001 18:18:18 GMT
	* @param stTm    时间结构
	* @return string GMT格式的时间字符串
	*/
	static std::string tm2GMTstr(const time_t &t);

	/**
	* @brief  当前时间转换成GMT字符串，GMT格式:Fri, 12 Jan 2001 18:18:18 GMT
	* @return string GMT格式的时间字符串
	*/
	static std::string now2GMTstr();

	/**
	* @brief  当前的日期(年月日)转换成字符串(%Y%m%d).
	*
	* @return string (%Y%m%d)格式的时间字符串
	*/
	static std::string nowdate2str();

	/**
	* @brief  当前的时间(时分秒)转换成字符串(%H%M%S).
	*
	* @return string (%H%M%S)格式的时间字符串
	*/
	static std::string nowtime2str();

	/**
	* @brief  获取当前时间的的毫秒数.
	*
	* @return int64_t 当前时间的的毫秒数
	*/
	static int64_t now2ms();

	/**
	* @brief  取出当前时间的微秒.
	*
	* @return int64_t 取出当前时间的微秒
	*/
	static int64_t now2us();

	/**
	* @brief  字符串转化成T型，如果T是数值类型, 如果str为空,则T为0.
	*
	* @param sStr  要转换的字符串
	* @return T    T型类型
	*/
	template<typename T>
	static T strto(const std::string &sStr);

	/**
	* @brief  字符串转化成T型.
	*
	* @param sStr      要转换的字符串
	* @param sDefault  缺省值
	* @return T        转换后的T类型
	*/
	template<typename T>
	static T strto(const std::string &sStr, const std::string &sDefault);

	typedef bool(*depthJudge)(const std::string& str1, const std::string& str2);

	/**
	* @brief  解析字符串,用分隔符号分隔,保存在vector里
	*
	* 例子: |a|b||c|
	*
	* 如果withEmpty=true时, 采用|分隔为:"","a", "b", "", "c", ""
	*
	* 如果withEmpty=false时, 采用|分隔为:"a", "b", "c"
	*
	* 如果T类型为int等数值类型, 则分隔的字符串为"", 则强制转化为0
	*
	* @param sStr      输入字符串
	* @param sSep      分隔字符串(每个字符都算为分隔符)
	* @param withEmpty true代表空的也算一个元素, false时空的过滤
	* @param depthJudge 对分割后的字符再次进行判断
	* @return          解析后的字符vector
	*/
	template<typename T>
	static std::vector<T> sepstr(const std::string &sStr, const std::string &sSep, bool withEmpty = false, depthJudge judge = nullptr);

	/**
	* @brief T型转换成字符串，只要T能够使用ostream对象用<<重载,即可以被该函数支持
	* @param t 要转换的数据
	* @return  转换后的字符串
	*/
	template<typename T>
	static std::string tostr(const T &t);

	/**
	* @brief  vector转换成string.
	*
	* @param t 要转换的vector型的数据
	* @return  转换后的字符串
	*/
	template<typename T>
	static std::string tostr(const std::vector<T> &t);

	/**
	* @brief  把map输出为字符串.
	*
	* @param map<K, V, D, A>  要转换的map对象
	* @return                    string 输出的字符串
	*/
	template<typename K, typename V, typename D, typename A>
	static std::string tostr(const std::map<K, V, D, A> &t);

	/**
	* @brief  map输出为字符串.
	*
	* @param multimap<K, V, D, A>  map对象
	* @return                      输出的字符串
	*/
	template<typename K, typename V, typename D, typename A>
	static std::string tostr(const std::multimap<K, V, D, A> &t);

	/**
	* @brief  pair 转化为字符串，保证map等关系容器可以直接用tostr来输出
	* @param pair<F, S> pair对象
	* @return           输出的字符串
	*/
	template<typename F, typename S>
	static std::string tostr(const std::pair<F, S> &itPair);

	/**
	* @brief  container 转换成字符串.
	*
	* @param iFirst  迭代器
	* @param iLast   迭代器
	* @param sSep    两个元素之间的分隔符
	* @return        转换后的字符串
	*/
	template <typename InputIter>
	static std::string tostr(InputIter iFirst, InputIter iLast, const std::string &sSep = "|");


	/**
	* @brief  二进制数据转换成字符串.
	*
	* @param buf     二进制buffer
	* @param len     buffer长度
	* @param sSep    分隔符
	* @param lines   多少个字节换一行, 默认0表示不换行
	* @return        转换后的字符串
	*/
	static std::string bin2str(const void *buf, size_t len, const std::string &sSep = "", size_t lines = 0);

	/**
	* @brief  二进制数据转换成字符串.
	*
	* @param sBinData  二进制数据
	* @param sSep     分隔符
	* @param lines    多少个字节换一行, 默认0表示不换行
	* @return         转换后的字符串
	*/
	static std::string bin2str(const std::string &sBinData, const std::string &sSep = "", size_t lines = 0);

	/**
	* @brief   字符串转换成二进制.
	*
	* @param psAsciiData 字符串
	* @param sBinData    二进制数据
	* @param iBinSize    需要转换的字符串长度
	* @return            转换长度，小于等于0则表示失败
	*/
	static int str2bin(const char *psAsciiData, unsigned char *sBinData, int iBinSize);

	/**
	* @brief  字符串转换成二进制.
	*
	* @param sBinData  要转换的字符串
	* @param sSep      分隔符
	* @param lines     多少个字节换一行, 默认0表示不换行
	* @return          转换后的二进制数据
	*/
	static std::string str2bin(const std::string &sBinData, const std::string &sSep = "", size_t lines = 0);

	/**
	* @brief  替换字符串.
	*
	* @param sString  输入字符串
	* @param sSrc     原字符串
	* @param sDest    目的字符串
	* @return string  替换后的字符串
	*/
	static std::string replace(const std::string &sString, const std::string &sSrc, const std::string &sDest);

	/**
	* @brief  批量替换字符串.
	*
	* @param sString  输入字符串
	* @param mSrcDest  map<原字符串,目的字符串>
	* @return string  替换后的字符串
	*/
	static std::string replace(const std::string &sString, const std::map<std::string, std::string>& mSrcDest);

	/**
	* @brief 匹配以.分隔的字符串，pat中*则代表通配符，代表非空的任何字符串
	* s为空, 返回false ，pat为空, 返回true
	* @param s    普通字符串
	* @param pat  带*则被匹配的字符串，用来匹配ip地址
	* @return     是否匹配成功
	*/
	static bool matchPeriod(const std::string& s, const std::string& pat);

	/**
	* @brief  匹配以.分隔的字符串.
	*
	* @param s   普通字符串
	* @param pat vector中的每个元素都是带*则被匹配的字符串，用来匹配ip地址
	* @return    是否匹配成功
	*/
	static bool matchPeriod(const std::string& s, const std::vector<std::string>& pat);

	/**
	* @brief  判断一个数是否为素数.
	*
	* @param n  需要被判断的数据
	* @return   true代表是素数，false表示非素数
	*/
	static bool isPrimeNumber(size_t n);

	/**
	* @brief  daemon
	*/
	static void daemon();

	/**
	* @brief  忽略管道异常
	*/
	static void ignorePipe();

	/**
	* @brief  将一个string类型转成一个字节 .
	*
	* @param sWhat 要转换的字符串
	* @return char    转换后的字节
	*/
	static char x2c(const std::string &sWhat);

	/**
	* @brief  大小字符串换成字节数，支持K, M, G两种 例如: 1K, 3M, 4G, 4.5M, 2.3G
	* @param s            要转换的字符串
	* @param iDefaultSize 格式错误时, 缺省的大小
	* @return             字节数
	*/
	static size_t toSize(const std::string &s, size_t iDefaultSize);

	static bool IsValidPhone(const std::string& phone);
};

namespace p
{
	template<typename D>
	struct strto1
	{
		D operator()(const std::string &sStr)
		{
			std::string s = "0";

			if (!sStr.empty())
			{
				s = sStr;
			}

			istringstream sBuffer(s);

			D t;
			sBuffer >> t;

			return t;
		}
	};

	template<>
	struct strto1<char>
	{
		char operator()(const std::string &sStr)
		{
			if (!sStr.empty())
			{
				return sStr[0];
			}
			return 0;
		}
	};

	template<>
	struct strto1<short>
	{
		short operator()(const std::string &sStr)
		{
			if (!sStr.empty())
			{
				return atoi(sStr.c_str());
			}
			return 0;
		}
	};

	template<>
	struct strto1<unsigned short>
	{
		unsigned short operator()(const std::string &sStr)
		{
			if (!sStr.empty())
			{
				return (unsigned short)strtoul(sStr.c_str(), NULL, 10);
			}
			return 0;
		}
	};

	template<>
	struct strto1<int>
	{
		int operator()(const std::string &sStr)
		{
			if (!sStr.empty())
			{
				return atoi(sStr.c_str());
			}
			return 0;
		}
	};

	template<>
	struct strto1<unsigned int>
	{
		unsigned int operator()(const std::string &sStr)
		{
			if (!sStr.empty())
			{
				return strtoul(sStr.c_str(), NULL, 10);
			}
			return 0;
		}
	};

	template<>
	struct strto1<long>
	{
		long operator()(const std::string &sStr)
		{
			if (!sStr.empty())
			{
				return atol(sStr.c_str());
			}
			return 0;
		}
	};

	template<>
	struct strto1<long long>
	{
		long long operator()(const std::string &sStr)
		{
			if (!sStr.empty())
			{
				return atoll(sStr.c_str());
			}
			return 0;
		}
	};

	template<>
	struct strto1<unsigned long>
	{
		unsigned long operator()(const std::string &sStr)
		{
			if (!sStr.empty())
			{
				return strtoul(sStr.c_str(), NULL, 10);
			}
			return 0;
		}
	};

	template<>
	struct strto1<float>
	{
		float operator()(const std::string &sStr)
		{
			if (!sStr.empty())
			{
				return (float)atof(sStr.c_str());
			}
			return 0;
		}
	};

	template<>
	struct strto1<double>
	{
		double operator()(const std::string &sStr)
		{
			if (!sStr.empty())
			{
				return atof(sStr.c_str());
			}
			return 0;
		}
	};

	template<typename D>
	struct strto2
	{
		D operator()(const std::string &sStr)
		{
			istringstream sBuffer(sStr);

			D t;
			sBuffer >> t;

			return t;
		}
	};

	template<>
	struct strto2<std::string>
	{
		std::string operator()(const std::string &sStr)
		{
			return sStr;
		}
	};

}

template<typename T>
T NFCommon::strto(const std::string &sStr)
{
	using strto_type = typename std::conditional<std::is_arithmetic<T>::value, p::strto1<T>, p::strto2<T>>::type;

	return strto_type()(sStr);
}

template<typename T>
T NFCommon::strto(const std::string &sStr, const std::string &sDefault)
{
	string s;

	if (!sStr.empty())
	{
		s = sStr;
	}
	else
	{
		s = sDefault;
	}

	return strto<T>(s);
}


template<typename T>
std::vector<T> NFCommon::sepstr(const std::string &sStr, const std::string &sSep, bool withEmpty, NFCommon::depthJudge judge)
{
	std::vector<T> vt;

	std::string::size_type pos = 0;
	std::string::size_type pos1 = 0;
	int pos_tmp = -1;

	while (true)
	{
		std::string s;
		std::string s1;
		pos1 = sStr.find_first_of(sSep, pos);
		if (pos1 == std::string::npos)
		{
			if (pos + 1 <= sStr.length())
			{
				s = sStr.substr(-1 != pos_tmp ? pos_tmp : pos);
				s1 = "";
			}
		}
		else if (pos1 == pos && (pos1 + 1 == sStr.length()))
		{
			s = "";
			s1 = "";
		}
		else
		{
			s = sStr.substr(-1 != pos_tmp ? pos_tmp : pos, pos1 - (-1 != pos_tmp ? pos_tmp : pos));
			s1 = sStr.substr(pos1 + 1);
			if (-1 == pos_tmp)
				pos_tmp = pos;
			pos = pos1;
		}

		if (nullptr == judge || judge(s, s1))
		{
			if (withEmpty)
			{
				vt.push_back(strto<T>(s));
			}
			else
			{
				if (!s.empty())
				{
					T tmp = strto<T>(s);
					vt.push_back(tmp);
				}
			}
			pos_tmp = -1;
		}

		if (pos1 == string::npos)
		{
			break;
		}

		pos++;
	}

	return vt;
}
template<typename T>
std::string NFCommon::tostr(const T &t)
{
	ostringstream sBuffer;
	sBuffer << t;
	return sBuffer.str();
}

template<typename T>
std::string NFCommon::tostr(const std::vector<T> &t)
{
	string s;
	for (size_t i = 0; i < t.size(); i++)
	{
		s += tostr(t[i]);
		s += " ";
	}
	return s;
}

template<typename K, typename V, typename D, typename A>
std::string NFCommon::tostr(const std::map<K, V, D, A> &t)
{
	string sBuffer;
	typename map<K, V, D, A>::const_iterator it = t.begin();
	while (it != t.end())
	{
		sBuffer += " [";
		sBuffer += tostr(it->first);
		sBuffer += "]=[";
		sBuffer += tostr(it->second);
		sBuffer += "] ";
		++it;
	}
	return sBuffer;
}

template<typename K, typename V, typename D, typename A>
std::string NFCommon::tostr(const std::multimap<K, V, D, A> &t)
{
	string sBuffer;
	typename multimap<K, V, D, A>::const_iterator it = t.begin();
	while (it != t.end())
	{
		sBuffer += " [";
		sBuffer += tostr(it->first);
		sBuffer += "]=[";
		sBuffer += tostr(it->second);
		sBuffer += "] ";
		++it;
	}
	return sBuffer;
}

template<typename F, typename S>
std::string NFCommon::tostr(const std::pair<F, S> &itPair)
{
	string sBuffer;
	sBuffer += "[";
	sBuffer += tostr(itPair.first);
	sBuffer += "]=[";
	sBuffer += tostr(itPair.second);
	sBuffer += "]";
	return sBuffer;
}

template <typename InputIter>
std::string NFCommon::tostr(InputIter iFirst, InputIter iLast, const std::string &sSep)
{
	std::string sBuffer;
	InputIter it = iFirst;

	while (it != iLast)
	{
		sBuffer += tostr(*it);
		++it;

		if (it != iLast)
		{
			sBuffer += sSep;
		}
		else
		{
			break;
		}
	}

	return sBuffer;
}

class NFIdGenerator
{
private:
	uint64_t m_nServerID;
	uint64_t m_nLastTime;
	uint64_t m_nLastID;

public:
	NFIdGenerator(uint64_t server_id);
	uint64_t GenId();
};





