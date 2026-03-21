## IupStringCompare

Utility function to compare strings lexicographically.
Used internally in **IupMatrixEx** when sorting, but available in the main library.

This means that numbers and text in the string are sorted separately (for ex: A1 A2 A11 A30 B1).
Also, natural alphabetic order is used: 123...aAáÁ...bBcC...
The comparison will work only for Latin-1 characters, even if UTF8MODE is Yes.

### Parameters/Return

    int IupStringCompare(const char* str1, const char* str2, int casesensitive, int lexicographic);

**str1 and str2**: strings to be compared.\
**casesensitive**: flag to enable case-sensitive compare. Can be 0 (disable) or 1 (enable).\
**lexicographic**: flag to enable lexicographic compare. Can be 0 (disable) or 1 (enable).
When disabled the compare will only return if the strings are equal (0) or different (1).

**Returns:** 0 if str1 == str2, -1 if str1<str2, 1 if str1 > str2 (same return values of the **strcmp** function).

### Notes

The Alphanum Algorithm is discussed at <http://www.DaveKoelle.com/alphanum.html>.

This implementation is Copyright (c) 2008 Dirk Jagdmann <<doj@cubic.org>>.
It is a cleanroom implementation of the algorithm and not derived by other's works.
In contrast to the versions written by Dave Koelle, this source code is distributed with the libpng/zlib license.

The IUP implementation is based on the "alphanum.hpp" code downloaded from the Dave Koelle page and implemented by Dirk Jagdmann.
It was modified to the C language and simplified to IUP needs.

### See Also

[IupMatrixEx](../ctrl/iup_matrixex.md)  
