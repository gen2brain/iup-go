## IupStringCompare

Utility function to compare strings lexicographically.
Used internally in **IupMatrixEx** when sorting, but available in the main library.

This means that numbers and text in the string are sorted separately (for example: A1 A2 A11 A30 B1).
Also, natural alphabetic order is used: 123...aAáÁ...bBcC...
The comparison will work only for Latin-1 characters, even if UTF8MODE is YES.

### Parameters/Return

    int IupStringCompare(const char* str1, const char* str2, int casesensitive, int lexicographic);

**str1**, **str2**: strings to be compared.\
**casesensitive**: flag to enable case-sensitive compare. Can be 0 (disable) or 1 (enable).\
**lexicographic**: flag to enable lexicographic compare. Can be 0 (disable) or 1 (enable).
When disabled the compare will only return if the strings are equal (0) or different (1).

**Returns:** 0 if str1 == str2, -1 if str1 < str2, 1 if str1 > str2 (same return values of the **strcmp** function).

### Notes

The Alphanum Algorithm is discussed at <http://www.DaveKoelle.com/alphanum.html>.

Based on the "alphanum.hpp" implementation by Dirk Jagdmann <<doj@cubic.org>>, Copyright (c) 2008, libpng/zlib license, converted to C.

### See Also

[IupMatrixEx](../ctrl/iup_matrixex.md)
