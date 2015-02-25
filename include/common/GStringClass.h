/*
 * A mis-guided attempt to make a string class look and feel like a python string.
 *
 * Author: Matthew Allen
 * Email: fret@memecode.com
 * Created: 16 Sept 2014
 */
#ifndef _GSTRING_H_
#define _GSTRING_H_

#include <xmath.h>
#include "GUtf8.h"

class GString
{
	struct RefStr
	{
		int32 Refs;
		uint32 Len;
		char Str[1];
	}	*Str;
	
	inline void _strip(GString &ret, const char *set, bool left, bool right)
	{
		if (!Str) return;
		
		char *s = Str->Str;
		char *e = s + Str->Len;

		if (!set) set = " \t\r\n";
		
		while (left && *s && strchr(set, *s))
			s++;
		
		while (right && e > s && strchr(set, e[-1]))
			e--;
		
		ret.Set(s, e - s);
	}

public:
	#ifdef LGI_UNIT_TESTS
	static int32 RefStrCount;
	#endif

	/// A copyable array of strings
	class Array : public GArray<GString>
	{
	public:
		Array() {}
		Array(const Array &a)
		{
			*this = (Array&)a;
		}
		
		Array &operator =(Array &a)
		{
			SetFixedLength(false);
			Length(a.Length());
			for (uint32 i=0; i<a.Length(); i++)
			{
				(*this)[i] = a[i];
			}
			SetFixedLength(true);
			return *this;
		}
	};

	/// Empty constructor
	GString()
	{
		Str = NULL;
	}
	
	/// String constructor
	GString(const char *str, int len = -1)
	{
		Str = NULL;
		Set(str, len);
	}

	/// GString constructor
	GString(GString &s)
	{
		Str = s.Str;
		if (Str)
			Str->Refs++;
	}
	
	~GString()
	{
		Empty();
	}
	
	/// Removes a reference to the string and deletes if needed
	void Empty()
	{
		if (!Str) return;
		Str->Refs--;
		LgiAssert(Str->Refs >= 0);
		if (Str->Refs == 0)
		{
			free(Str);
			#ifdef LGI_UNIT_TESTS
			RefStrCount--;
			#endif
		}
		Str = NULL;
	}
	
	/// Returns the pointer to the string data
	char *Get()
	{
		return Str ? Str->Str : NULL;
	}
	
	/// Sets the string to a new value
	bool Set
	(
		/// Can be a pointer to string data or NULL to create an empty buffer (requires valid length)
		const char *str,
		/// Byte length of input string or -1 to copy till the NULL terminator.
		int len = -1
	)
	{
		Empty();

		if (len < 0)
		{
			if (str)
				len = strlen(str);
			else
				return false;
		}

		Str = (RefStr*)malloc(sizeof(RefStr) + len);
		if (!Str)
			return false;
		
		Str->Refs = 1;
		Str->Len = len;
		#ifdef LGI_UNIT_TESTS
		RefStrCount++;
		#endif
		
		if (str)
			memcpy(Str->Str, str, len);
		
		Str->Str[len] = 0;
		return true;
	}
	
	/// Assignment operator to copy one string to another
	GString &operator =(const GString &s)
	{
		if (this != &s)
		{
			Empty();
			Str = s.Str;
			if (Str)
				Str->Refs++;
		}
		return *this;
	}

	/// Assignment operator for a C string
	GString &operator =(const char *s)
	{
		Empty();
		Set(s);
		return *this;
	}
	
	/// Cast to C string operator
	operator char *()
	{
		return Str ? Str->Str : NULL;
	}
	
	/// Gets the length in bytes
	uint32 Length()
	{
		return Str ? Str->Len : 0;
	}

	/// Splits the string into parts
	Array Split(const char *Sep, int Count = -1, bool CaseSen = false)
	{
		Array a;

		if (Str && Sep)
		{
			const char *s = Get(), *Prev = s;
			int SepLen = strlen(Sep);
			
			while (s = CaseSen ? strstr(s, Sep) : stristr(s, Sep))
			{
				a.New().Set(Prev, s - Prev);
				s += SepLen;
				Prev = s;
				if (Count > 0 && a.Length() >= (uint32)Count)
					break;
			}
			
			if (*Prev)
				a.New().Set(Prev);

		}

		a.SetFixedLength();
		return a;
	}
	
	/// Joins an array of strings using a separator
	GString Join(Array &a)
	{
		GString ret;
		
		char *Sep = Get();
		int SepLen = Sep ? strlen(Sep) : 0;
		int Bytes = SepLen * (a.Length() - 1);
		GArray<unsigned> ALen;
		for (unsigned i=0; i<a.Length(); i++)
		{
			ALen[i] = a[i].Length();
			Bytes += ALen[i];
		}
		
		// Create an empty buffer of the right size
		if (ret.Set(NULL, Bytes))
		{
			char *ptr = ret.Get();
			for (unsigned i=0; i<a.Length(); i++)
			{
				if (i && Sep)
				{
					memcpy(ptr, Sep, SepLen);
					ptr += SepLen;
				}
				memcpy(ptr, a[i].Get(), ALen[i]);
				ptr += ALen[i];
			}
			LgiAssert(ptr - ret.Get() == Bytes);
			*ptr++ = 0; // NULL terminate
		}

		return ret;
	}

	/// Convert string to double
	double Float()
	{
		return Str ? atof(Str->Str) : NAN;
	}

	/// Convert to integer
	int Int()
	{
		return Str ? atoi(Str->Str) : -1;
	}

	/// Find a sub-string	
	int Find(const char *needle, int start = 0, int end = -1)
	{
		if (!needle) return -1;
		char *c = Get();
		if (!c) return -1;

		char *pos = c + start;
		while (c < pos)
		{
			if (!*c)
				return -1;
			c++;
		}
		
		char *found = (end > 0) ? strnstr(c, needle, end - start) : strstr(c, needle);
		return (found) ? found - Get() : -1;
	}

	/// Reverse find a string (starting from the end)
	int RFind(const char *needle, int start = 0, int end = -1)
	{
		if (!needle) return -1;
		char *c = Get();
		if (!c) return -1;

		char *pos = c + start;
		while (c < pos)
		{
			if (!*c)
				return -1;
			c++;
		}
		
		char *found, *prev = NULL;
		int str_len = strlen(needle);
		while
		(
			found =
			(
				(end > 0) ?
				strnstr(c, needle, end - start) :
				strstr(c, needle)
			)
		)
		{
			prev = found;
			c = found + str_len;
		}
		return (prev) ? prev - Get() : -1;
	}

	/// Returns a copy of the string with all the characters converted to lower case
	GString Lower()
	{
		GString s;
		s.Set(Get());
		StrLwr(s.Get());
		return s;
	}
	
	/// Returns a copy of the string with all the characters converted to upper case
	GString Upper()
	{
		GString s;
		s.Set(Get());
		StrUpr(s.Get());
		return s;
	}

	/// Gets the character at 'index'
	GString operator() (int index)
	{
		GString s;
		if (Str)
		{
			char *c = Str->Str;
			if (index < 0)
			{
				int idx = Str->Len + index;
				if (idx >= 0)
				{
					s.Set(c + idx, 1);
				}
			}
			else if (index < Str->Len)
			{
				s.Set(c + index, 1);
			}
		}
		return s;
	}
	
	/// Gets the string between at 'start' and 'end' (not including the end'th character)
	GString operator() (int start, int end)
	{
		GString s;
		if (Str)
		{
			char *c = Str->Str;
			int start_idx = start < 0 ? Str->Len + start + 1 : start;
			int end_idx = end < 0 ? Str->Len + end + 1 : end;
			if (start_idx >= 0 && end_idx > start_idx)
				s.Set(c + start_idx, end_idx - start_idx);
		}
		return s;
	}

	/// Strip off any leading and trailing whitespace	
	GString Strip(const char *set = NULL)
	{
		GString ret;
		_strip(ret, set, true, true);
		return ret;
	}

	/// Strip off any leading whitespace	
	GString LStrip(const char *set = NULL)
	{
		GString ret;
		_strip(ret, set, true, false);
		return ret;
	}	

	/// Strip off any trailing whitespace	
	GString RStrip(const char *set = NULL)
	{
		GString ret;
		_strip(ret, set, false, true);
		return ret;
	}
};

#endif