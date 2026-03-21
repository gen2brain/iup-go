## MASK (non-inheritable)

Defines a mask that will filter interactive text input.

### Value

string

Set to NULL to remove the mask.

### Notes

Since the validation process is performed key by key when the user is typing, an intermediate value cannot be typed if it does not follow the mask rules.

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
| IUP_MASK_FLOATCOMMA  | "[+/-]?(/d+/,?/d*\|/,/d+)"                 | floating point number                           |
| IUP_MASK_UFLOATCOMMA | "(/d+/,?/d*\|/,/d+)"                       | unsigned floating point number                  |

### Auxiliary Attributes

#### MASKCASEI (non-inheritable) 

If YES, will turn the filter case-insensitive. Default: NO.

#### MASKNOEMPTY (non-inheritable)

If YES, the value cannot be NULL or empty. Default: NO (can be empty or NULL).

#### MASKDECIMALSYMBOL (non-inheritable)

The decimal symbol for string/float conversion. Can be "." or ",". Must be set before MASKFLOAT.

#### MASKINT (non-inheritable) (write-only)

Defines an integer mask with limits. Format: "%d:%d" ("min:max").
It will replace MASK using one of the pre-defined masks.

#### MASKFLOAT (non-inheritable) (write-only)

Defines a floating point mask with limits. Format: "%g:%g" ("min:max").
It will replace MASK using one of the pre-defined masks.

### Auxiliary Callbacks

**MASKFAIL_CB**: Action generated when the new text fails at the mask check.

    int function(Ihandle *ih, char *new_value);

**ih**: identifier of the element that activated the event.\
**new_value**: Represents the new text value.

### Pattern Specification

The pattern to be searched in the text can be defined by the rules given below.

- "Function" codes (such as /l, /D, /w) cannot be used inside a class ([...]).
- If the character following a / does not mean a special case (such as /w or /n), it is matched with no / - that means that /x will match only x, and not /x. If you want to match /x, use //x.
- The caret (^) character has different meanings when used inside or outside a class - inside a class it means negative, and outside a class it is an anchor to the beginning of a line.
- The boundary function (/b) anchors the pattern to a word boundary - it does not match anything. A word boundary is a point between a /w and a /W character.
- Capture operators (f and g) group patterns and are also used to keep matched sections of texts.
- A word on precedence: concatenation has precedence over the alternation (j) operator - that is, faj fej fi will match fa OR fe OR fi.
- The @ character is used to determine that, instead of searching the text until the first match is made, the function should try to match the pattern only with the first character. If present, it must be the first character of the pattern.
- The % character is used to determine that the text should be searched to its end, independently of the number of matches found. If present, it must be the first character of the pattern. This is only useful when combined with the capture feature.

### Allowed pattern characters

|           |                                                                              |
|-----------|------------------------------------------------------------------------------|
| c         | Matches a "c" (non-special) character                                        |
| .         | Matches any single character                                                 |
| [abc]     | Matches an "a", "b" or "c" characters                                        |
| [a-d]     | Matches any character between "a" and "d", including them (just like [abcd]) |
| [^a-dg]   | Matches any character which is neither between "a" and "d" nor "a" "g"       |
| /d        | Matches any digit (just like [0-9])                                          |
| /D        | Matches any non-digit (just like [^0-9])                                     |
| /l        | Matches any letter (just like [a-zA-Z])                                      |
| /L        | Matches any non-letter (just like [^a-zA-Z])                                 |
| /w        | Matches any alphanumeric character (just like [0-9a-zA-Z ])                  |
| /W        | Matches any non-alphanumeric character (just like [^0-9a-zA-Z ])             |
| /s        | Matches any "blank" character (TAB, SPACE, CR)                               |
| /S        | Matches any non-blank character                                              |
| /n        | Matches a newline character                                                  |
| /t        | Matches a tabulation character                                               |
| /nnn      | Matches an ASCII character with a nnn value (decimal)                        |
| /xnn      | Matches an ASCII character with a nn value (hexadecimal)                     |
| /special  | Matches the special character literally (/[, //, /.)                         |
| abc       | Matches a sequence of a, b and c patterns in order                           |
| aj bj c   | Matches a pattern a, b or c                                                  |
| a*        | Matches 0 or more characters a                                               |
| a+        | Matches 1 or more characters a                                               |
| a?        | Matches 1 or no characters a                                                 |
| (pattern) | Considers pattern as one character for the above                             |
| fpatterng | Captures pattern for later reference                                         |
| /b        | Anchors to a word boundary                                                   |
| /B        | Anchors to a non-boundary                                                    |
| ^pattern  | Anchors pattern to the beginning of a line                                   |
| pattern$  | Anchors pattern to the end of a line                                         |
| @pattern  | Returns the match found only in the beginning of the text                    |
| %pattern  | Returns the first match found, but searches all the text                     |

### Examples

|                   |                                                                                                                  |
|-------------------|------------------------------------------------------------------------------------------------------------------|
| (my\|his)         | Matches both my pattern and his pattern.                                                                         |
| /d/d:/d/d(:/d/d)? | Matches time with seconds (01:25:32) or without seconds (02:30).                                                 |
| [A-D]/l+          | Matches names such as Australia, Bolivia, Canada or Denmark, but not England, Spain or single letters such as A. |
| /l/w*             | my variable = 23 * width;                                                                                        |
| ^Subject:[^/n]*/n | Subject: How to match a subject line.1                                                                           |
| /b[ABab]/w*       | Matches any word that begins with A or B                                                                         |
| from:/s*/w+       | Captures "sender" in a message from sender                                                                       |

### Affects

[IupText](../elem/iup_text.md), [IupMultiline](../elem/iup_multiline.md), [IupList](../elem/iup_list.md) and [IupMatrix](../ctrl/iup_matrix.md)
