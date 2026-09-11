## MASK (non-inheritable)

Defines a mask that will filter interactive text input.

### Value

string

Set to NULL to remove the mask.

### Notes

The value is checked at every change while the user types, and when VALUEMASKED is set.
The whole value must match the mask; a value that is the beginning of a matching value is also accepted, any other value is refused.
MASKFAIL_CB is called when a change is refused.

If you set the VALUE attribute, any text can be used.
To set a value that is validated by the current MASK, use VALUEMASKED.

### Pre-Defined Masks

| Definition           | Value                                      | Description                                     |
|----------------------|--------------------------------------------|-------------------------------------------------|
| IUP_MASK_INT         | "[+/-]?/d+"                                | integer number                                  |
| IUP_MASK_UINT        | "/d+"                                      | unsigned integer number                         |
| IUP_MASK_FLOAT       | "[+/-]?(/d+/.?/d*\|/./d+)"                 | floating point number                           |
| IUP_MASK_UFLOAT      | "(/d+/.?/d*\|/./d+)"                       | unsigned floating point number                  |
| IUP_MASK_EFLOAT      | "[+/-]?(/d+/.?/d*\|/./d+)([eE][+/-]?/d+)?" | floating point number with exponential notation |
| IUP_MASK_UEFLOAT     | "(/d+/.?/d*\|/./d+)([eE][+/-]?/d+)?"       | unsigned floating point number with exponential notation |
| IUP_MASK_FLOATCOMMA  | "[+/-]?(/d+/,?/d*\|/,/d+)"                 | floating point number with a decimal comma      |
| IUP_MASK_UFLOATCOMMA | "(/d+/,?/d*\|/,/d+)"                       | unsigned floating point number with a decimal comma |

### Auxiliary Attributes

#### MASKCASEI (non-inheritable)

If YES, will turn the filter case-insensitive. Default: NO.

#### MASKNOEMPTY (non-inheritable)

If YES, the value cannot be NULL or empty. Default: NO (can be empty or NULL).

#### MASKDECIMALSYMBOL (non-inheritable)

The decimal symbol used by MASKFLOAT and MASKREAL. Can be "." or ",". Must be set before MASKFLOAT or MASKREAL.
When not set, MASKFLOAT uses the global attribute DEFAULTDECIMALSYMBOL and MASKREAL uses ".".

#### MASKINT (non-inheritable) (write-only)

Defines an integer mask with limits. Format: "%d:%d" ("min:max").
It replaces the current mask, and MASK then returns the pattern of the pre-defined mask that was installed.

#### MASKFLOAT (non-inheritable) (write-only)

Defines a floating point mask with limits. Format: "%g:%g" ("min:max").
It replaces the current mask, and MASK then returns the pattern of the pre-defined mask that was installed.

#### MASKREAL (non-inheritable) (write-only)

Defines a floating point mask without limits. Can be "SIGNED" or "UNSIGNED".
It replaces the current mask, and MASK then returns the pattern of the pre-defined mask that was installed.

### Auxiliary Callbacks

**MASKFAIL_CB**: Action generated when the new text fails at the mask check.
Not called by IupMatrix.

    int function(Ihandle *ih, char *new_value);

**ih**: identifier of the element that activated the event.\
**new_value**: Represents the new text value.

### Pattern Specification

- The whole value is matched, from its first character to its last.
- "Function" codes (such as /l, /D, /w) cannot be used inside a class ([...]).
- If the character following a / does not mean a special case (such as /w or /n), it is matched without the / - that means that /q will match only q, and not /q. If you want to match /q, use //q.
- The caret (^) inside a class means negation.
- The boundary function (/b) anchors the pattern to a word boundary - it does not match anything. A word boundary is a point between a /w and a /W character.
- Concatenation has precedence over the alternation (|) operator - that is, fa|fe|fi will match fa OR fe OR fi.

### Allowed pattern characters

|           |                                                                              |
|-----------|------------------------------------------------------------------------------|
| c         | Matches a "c" (non-special) character                                        |
| .         | Matches any single character                                                 |
| [abc]     | Matches an "a", "b" or "c" characters                                        |
| [a-d]     | Matches any character between "a" and "d", including them (just like [abcd]) |
| [^a-dg]   | Matches any character which is neither between "a" and "d" nor "g"           |
| /d        | Matches any digit (just like [0-9])                                          |
| /D        | Matches any non-digit (just like [^0-9])                                     |
| /l        | Matches any letter (just like [a-zA-Z])                                      |
| /L        | Matches any non-letter (just like [^a-zA-Z])                                 |
| /w        | Matches any alphanumeric character or underscore (just like [0-9a-zA-Z_])    |
| /W        | Matches any non-alphanumeric character (just like [^0-9a-zA-Z_])             |
| /s        | Matches any "blank" character (TAB, SPACE, LF)                               |
| /S        | Matches any non-blank character                                              |
| /n        | Matches a newline character                                                  |
| /t        | Matches a tabulation character                                               |
| /e        | Matches an escape character (ASCII 27)                                       |
| /nnn      | Matches an ASCII character with a nnn value (decimal, up to 255)             |
| /xnn      | Matches an ASCII character with a nn value (hexadecimal)                     |
| /onnn     | Matches an ASCII character with a nnn value (octal)                          |
| /special  | Matches the special character literally (/[, //, /.)                         |
| abc       | Matches a sequence of a, b and c patterns in order                           |
| a\|b\|c   | Matches a pattern a, b or c                                                  |
| a*        | Matches 0 or more characters a                                               |
| a+        | Matches 1 or more characters a                                               |
| a?        | Matches 1 or no characters a                                                 |
| (pattern) | Considers pattern as one character for the above                             |
| /b        | Anchors to a word boundary                                                   |

### Examples

|                   |                                                                                                                  |
|-------------------|------------------------------------------------------------------------------------------------------------------|
| (my\|his)         | Matches "my" or "his".                                                                                           |
| /d/d:/d/d(:/d/d)? | Matches time with seconds (01:25:32) or without seconds (02:30).                                                 |
| [A-D]/l+          | Matches names such as Australia, Bolivia, Canada or Denmark, but not England, Spain or single letters such as A. |
| /l/w*             | Matches an identifier: a letter followed by letters, digits or underscores.                                      |
| [^/n]*            | Matches any text without a line break.                                                                           |

### Affects

[IupText](../elem/iup_text.md), [IupMultiline](../elem/iup_multiline.md), [IupList](../elem/iup_list.md) and [IupMatrix](../ctrl/iup_matrix.md)
