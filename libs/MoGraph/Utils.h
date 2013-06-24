/*
Copyright (C) 2011 MoSync AB

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License,
version 2, as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
MA 02110-1301, USA.
*/

#ifndef UTILS_H_
#define UTILS_H_

#include <vector>
#include <string>

#if defined(MOSYNC_NATIVE) && defined(__IOS__)
#include <ext/hash_map>

namespace std {
	using namespace __gnu_cxx;
}

namespace __gnu_cxx
{
	template<> struct hash< std::string >
	{
		size_t operator()( const std::string& x ) const
		{
			return hash< const char* >()( x.c_str() );
		}
	};
}

#else
#include <hash_map>
#ifdef __BB10__
namespace std {
template<> struct hash<string>
{
	size_t operator()(const string& __s) const
	{ return hash_value(__s); }
};

template<>
class hash_compare<string, less<string> >
{	// traits class for hash containers
public:
	enum
	{	// parameters for hash table
		bucket_size = 4,	// 0 < bucket_size
		min_buckets = 8};	// min_buckets = 2 ^^ N, 0 < N

	hash_compare()
	: comp()
	{	// construct with default comparator
	}

	size_t operator()(const string& _Keyval) const
	{	// hash _Keyval to size_t value by pseudorandomizing transform
		return hash_value(_Keyval);
	}

	bool operator()(const string& _Keyval1, const string& _Keyval2) const
	{	// test if _Keyval1 ordered before _Keyval2
		return (comp(_Keyval1, _Keyval2));
	}

protected:
	less<string> comp;	// the comparator object
};
}
#endif
#endif

/**
 * \brief namespace Utils	, small global functions encapsulated under the namespace Utils
 */
namespace Utils
{
	/**
	 * \brief stringCopy, copy a subset of a string, with optinal trim parameters.
	 * @param in	,input string
	 * @param start	,start from index
	 * @param end	,end at index
	 * @param trim	,trim spaces if flag == true (removes all spaces in string, by brute force)
	 * @return 		,output string result
	 */

	std::vector<std::string> split(std::string &line,char delimiter,bool trim);

	/**
	 * \brief split		,tokenizer that splits string up into vectored string sing delimiter and/or trimming text
	 * @param line		,one row of data untill EOL
	 * @param delimiter ,using delimiter to split substrings left right side
	 * @param trim		,optional parameter for removing spaces from string
	 * @return vector of strings that has been splitted up with the delimiter.
	 */

	std::string stringCopy(std::string &in, size_t start, size_t end, bool trim);

	/**
	 * \brief unQuote	,removes quotes from a string e.g. "test123" => test123
	 * @param str		,input string
	 * @param quote		,quote char e.g. "
	 * @return output unquoted string
	 */

	std::string unQuote(const std::string &str,const char quote= '\"');
}

#endif /* UTILS_H_ */
