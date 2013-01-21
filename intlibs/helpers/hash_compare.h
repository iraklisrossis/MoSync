/* Copyright (C) 2011 MoSync AB

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

#ifndef HASH_COMPARE_H
#define HASH_COMPARE_H

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
#else
namespace __gnu_cxx {
        template<> struct hash<std::string>
        {
                size_t operator()(const std::string& __s) const
                { return __stl_hash_string(__s.c_str()); }
        };
#ifdef DARWIN
	template<class T> struct hash<T*> {
		size_t operator()(T* ptr) const {
			return hash<size_t>()((size_t)ptr);
		}
	};
#endif	//DARWIN
}
template<class T> class hash_compare : public hash<T> {};
#endif

#endif	//HASH_COMPARE_H
