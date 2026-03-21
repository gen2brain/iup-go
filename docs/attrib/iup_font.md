## FONT

Character font of the text shown in the element.
Although it is an inheritable attribute, it is defined only on elements that have a native representation, but other elements are affected because it affects the [SIZE](iup_size.md) attribute.

### Value

Font description containing typeface, style and size. Default: the global attribute DEFAULTFONT.

The common format definition is similar to the [Pango](http://www.pango.org/) library Font Description, used by GTK+.
It is defined as having 3 parts: "<font face>**,** <font styles> <font size>".

Font face is the font face name and can be any name.
Although only names recognized by the system will be actually used.
The names Helvetica, Courier and Times are always accepted in all systems.

The supported **font style** is a combination of: **Bold**, **Italic**, **Underline** and **Strikeout**.
The Pango format includes many other definitions not supported by the common format; they are supported only by the GTK driver.
Unsupported values are simply ignored. The names must be in the same case described here.

**Font size** is in points (1/72 inch) or in pixels (using negative values).

Returned values will be the same value when changing the attribute, except for the old font names that will be converted to the new common format definition.

#### Windows

The DEFAULTFONT is retrieved from the System Settings (see below), if this failed then "Tahoma, 10" for Windows XP, or "Segoe UI, 9" since Windows Vista, is assumed.

The native handle can be obtained using the "**HFONT**" attribute.

In "Control Panel", open the "Display Properties" then click on "Advanced" and select "Message Box" and change its Font to affect the default font for applications.
In Vista go to "Window Color and Appearance", then "Open Classic Appearance", then Advanced.

#### Motif

The DEFAULTFONT is retrieved from the user resource file (see below), if failed, then "Fixed, 11" is assumed.

The X-Windows Logical Font Description format (XLFD) is also supported.

The native handle can be obtained using the "**XMFONTLIST**" and "**XFONTSTRUCT**" attributes.
The selected X Logical Font Description string can be obtained from the attribute "**XLFD**".

You can use the **xfontsel** program to obtain a string in the X-Windows Logical Font Description format (XLFD).
Noticed that the first size entry of the X-Windows font string format (**pxlsz**) is in **pixels** and the next one (**ptSz**) is in deci-points (multiply the value in points by 10).

Be aware that the resource files ".Xdefaults" and "Iup" in the user home folder can affect the default font and many other visual appearance features in Motif.

#### GTK

The DEFAULTFONT is retrieved from the style defined by GNOME (see below), if failed "Sans, 10" is assumed.

The X-Windows Logical Font Description format (XLFD) is also supported.

The native handle can be obtained using the "**PANGOFONTDESC**" attribute.

Font face can be a list of font face names in GTK. For example, "Arial,Helvetica, 12".
Not accepted in the other drivers.

Style can also be a combination of: Small-Caps, [Ultra-Light\|Light\|Medium\|Semi-Bold\|Bold\|Ultra-Bold\|Heavy], [Ultra-Condensed\|Extra-Condensed\|Condensed\|Semi-Condensed\|Semi-Expanded\|Expanded\|Extra-Expanded\|Ultra-Expanded].
Those values can be used only when the string is a full Pango compliant font, i.e., no underline, no strikeout and size>0.

In GNOME, go to the "Appearance Preferences" tool, then in the Fonts tab change the Applications Font to affect the default font.

#### Examples:

    "Times, Bold 18"
    "Arial, 24" (no style)
    "Courier New, Italic Underline -30" (size in pixels)

### Affects

All elements, since the SIZE attribute depends on the FONT attribute, except for menus.

### Notes

When the FONT is changed and [SIZE](iup_size.md) is set, then [RASTERSIZE](iup_rastersize.md) is also updated.

Since font face names are not a standard between Windows, Motif and GTK, a few names are specially handled to improve application portability.
If you want to use names that work for all systems, we recommend using: Courier, Times and Helvetica (same as Motif).
Those names always have a native system name equivalent.
If you use those names, IUP will automatically map to the native system equivalent.
See the table below:

| Recommended/Motif | Windows         | GTK       | Description                     |
|-------------------|-----------------|-----------|---------------------------------|
| **Helvetica**     | Arial           | Sans      | without serif, variable spacing |
| **Courier**       | Courier New     | Monospace | with serif, fixed spacing       |
| **Times**         | Times New Roman | Serif     | with serif, variable spacing    |

### Auxiliary Attributes

They will change the FONT attribute, and depend on it.
They are used only to set partial FONT parameters of style and size.
To do that, the FONT attribute is parsed, changed and updated to the new value in the common format definition.
This means that if the attribute was set in X-Windows format or in the old Windows and IUP formats, the previous value will be replaced by a new value in the common format definition.
Pango additional styles will also be removed.

#### FONTSTYLE (non-inheritable)

Replaces or returns the style of the current FONT attribute.
Since font styles are case-sensitive, this attribute is also case-sensitive.

#### FONTSIZE (non-inheritable)

Replaces or returns the size of the current FONT attribute.

#### FONTFACE (non-inheritable)

Replaces or returns the face name of the current FONT attribute.

#### CHARSIZE (read-only, non-inheritable)

Returns the average character size of the current FONT attribute.
This is the factor used by the SIZE attribute to convert its units to pixels.

#### FOUNDRY [Motif Only] (non-inheritable)

Allows selecting a foundry for the FONT being selected.
Must be set before setting the FONT attribute.

### Encoding

The number that represents each character is dependent on the encoding used.
For example, in ASCII encoding, the letter A has code 65, but codes above 128 can be very different, according to the encoding.
Encoding is defined by a charset.

IUP uses the default locale in ANSI-C. This means that it does not adopt a specific charset, and so the results can be different if the developer charset is different from the run time charset where the user is running the application.
For example, if the developer is using a charset, and its user is also using the same encoding, then everything will work fine without the need of text encoding conversions.
The advantage is that any charset can be used, and localization is usually done in that way.

IUP supports also the UTF-8 (ISO10646-1) encoding in the GTK and Windows drivers.
To specify a string in UTF-8 encoding set the global attribute "[UTF8MODE](iup_globals.md)" to "Yes".

#### ISO8859-1 and Windows-1252 Displayable Characters

The Latin-1 charset (ISO8859-1) defines an encoding for all the characters used in Western languages.
It is the most common encoding, besides UTF-8.

The first half of Latin-1 is standard ASCII (128 characters), while the second half (with the highest bit set) contains accented characters needed for Western languages other than English.
In Windows, the ISO8859-1 charset was changed, and some control characters were replaced to include more display characters, this new charset is named Windows-1252 (these characters are marked in the table below with thick borders).

|             |             |             |             |             |             |             |             |             |             |             |             |              |             |             |             |
|-------------|-------------|-------------|-------------|-------------|-------------|-------------|-------------|-------------|-------------|-------------|-------------|--------------|-------------|-------------|-------------|
| ***32***    | ! ***33***  | " ***34***  | \# ***35*** | \$ ***36*** | % ***37***  | & ***38***  | ' ***39***  | ( ***40***  | ) ***41***  | \* ***42*** | \+ ***43*** | , ***44***   | - ***45***  | . ***46***  | / ***47***  |
| 0 ***48***  | 1 ***49***  | 2 ***50***  | 3 ***51***  | 4 ***52***  | 5 ***53***  | 6 ***54***  | 7 ***55***  | 8 ***56***  | 9 ***57***  | : ***58***  | ; ***59***  | < ***60***   | = ***61***  | > ***62***  | ? ***63***  |
| @ ***64***  | A ***65***  | B ***66***  | C ***67***  | D ***68***  | E ***69***  | F ***70***  | G ***71***  | H ***72***  | I ***73***  | J ***74***  | K ***75***  | L ***76***   | M ***77***  | N ***78***  | O ***79***  |
| P ***80***  | Q ***81***  | R ***82***  | S ***83***  | T ***84***  | U ***85***  | V ***86***  | W ***87***  | X ***88***  | Y ***89***  | Z ***90***  | [ ***91***  | \\ ***92***  | ] ***93***  | ^ ***94***  | _ ***95***  |
| \` ***96*** | a ***97***  | b ***98***  | c ***99***  | d ***100*** | e ***101*** | f ***102*** | g ***103*** | h ***104*** | i ***105*** | j ***106*** | k ***107*** | l ***108***  | m ***109*** | n ***110*** | o ***111*** |
| p ***112*** | q ***113*** | r ***114*** | s ***115*** | t ***116*** | u ***117*** | v ***118*** | w ***119*** | x ***120*** | y ***121*** | z ***122*** | { ***123*** | \| ***124*** | } ***125*** | ~ ***126*** | ***127***   |
| € ***128*** | ***129***   | ‚ ***130*** | ƒ ***131*** | „ ***132*** | … ***133*** | † ***134*** | ‡ ***135*** | ˆ ***136*** | ‰ ***137*** | Š ***138*** | ‹ ***139*** | Œ ***140***  | ***141***   | Ž ***142*** | ***143***   |
| ***144***   | ‘ ***145*** | ’ ***146*** | “ ***147*** | ” ***148*** | • ***149*** | – ***150*** | — ***151*** | ˜ ***152*** | ™ ***153*** | š ***154*** | › ***155*** | œ ***156***  | ***157***   | ž ***158*** | Ÿ ***159*** |
| ***160***   | ¡ ***161*** | ¢ ***162*** | £ ***163*** | ¤ ***164*** | ¥ ***165*** | ¦ ***166*** | § ***167*** | ¨ ***168*** | © ***169*** | ª ***170*** | « ***171*** | ¬ ***172***  | ***173***   | ® ***174*** | ¯ ***175*** |
| ° ***176*** | ± ***177*** | ² ***178*** | ³ ***179*** | ´ ***180*** | µ ***181*** | ¶ ***182*** | · ***183*** | ¸ ***184*** | ¹ ***185*** | º ***186*** | » ***187*** | ¼ ***188***  | ½ ***189*** | ¾ ***190*** | ¿ ***191*** |
| À ***192*** | Á ***193*** | Â ***194*** | Ã ***195*** | Ä ***196*** | Å ***197*** | Æ ***198*** | Ç ***199*** | È ***200*** | É ***201*** | Ê ***202*** | Ë ***203*** | Ì ***204***  | Í ***205*** | Î ***206*** | Ï ***207*** |
| Ð ***208*** | Ñ ***209*** | Ò ***210*** | Ó ***211*** | Ô ***212*** | Õ ***213*** | Ö ***214*** | × ***215*** | Ø ***216*** | Ù ***217*** | Ú ***218*** | Û ***219*** | Ü ***220***  | Ý ***221*** | Þ ***222*** | ß ***223*** |
| à ***224*** | á ***225*** | â ***226*** | ã ***227*** | ä ***228*** | å ***229*** | æ ***230*** | ç ***231*** | è ***232*** | é ***233*** | ê ***234*** | ë ***235*** | ì ***236***  | í ***237*** | î ***238*** | ï ***239*** |
| ð ***240*** | ñ ***241*** | ò ***242*** | ó ***243*** | ô ***244*** | õ ***245*** | ö ***246*** | ÷ ***247*** | ø ***248*** | ù ***249*** | ú ***250*** | û ***251*** | ü ***252***  | ý ***253*** | þ ***254*** | ÿ ***255*** |

\

|     |                           |
|-----|---------------------------|
|     |   Punctuation and Symbols |
|     |  Numbers                  |
|     |  Letters                  |
|     |  Accented                 |

Adapted from [http://en.wikipedia.org/wiki/Windows-1252](http://en.wikipedia.org/wiki/Windows-1252).

#### UTF-8

"Universal character set Transformation Format - 8 bits" is part of the Unicode standard that is used in most modern Web applications, and it is becoming widely used in desktop applications too.

It allows the application to use a regular "char*" for strings, but it is a variable width encoding, meaning that a single character may have up to four bytes in sequence.
And the code "\0" is still used as a string terminator (NULL).
So all the regular **strstr**, **strcmp**, **strlen**, **strcpy** and **strcat** functions will work normally, except **strchr** because it will search only for 1 byte characters.
Notice that **strlen** will return the number of bytes, not the number of multibyte characters.
And **strcmp** will compare byte encodings.

The first 128 characters of Unicode, which correspond one-to-one with [ASCII](http://en.wikipedia.org/wiki/ASCII), are encoded using a single octet with the same binary value as ASCII, making valid ASCII text valid UTF-8-encoded Unicode as well.
If the highest bit is 1, then one to three more bytes will follow to define the actual character encoding.
The number of bytes following is determined by the number of bits set to 1 after the highest bit.

The next 1920 characters need two bytes to encode.
This covers the remainder of almost all [Latin-derived alphabets](http://en.wikipedia.org/wiki/Latin-derived_alphabet), and also [Greek](http://en.wikipedia.org/wiki/Greek_alphabet), [Cyrillic](http://en.wikipedia.org/wiki/Cyrillic_script), [Coptic](http://en.wikipedia.org/wiki/Coptic_alphabet), [Armenian](http://en.wikipedia.org/wiki/Armenian_alphabet), [Hebrew](http://en.wikipedia.org/wiki/Hebrew_alphabet), [Arabic](http://en.wikipedia.org/wiki/Arabic_alphabet), [Syriac](http://en.wikipedia.org/wiki/Syriac_alphabet) and [Tāna](http://en.wikipedia.org/wiki/Tāna) alphabets, as well as [Combining Diacritical Marks](http://en.wikipedia.org/wiki/Combining_Diacritical_Marks).
Three bytes are needed for characters in the rest of the [Basic Multilingual Plane](http://en.wikipedia.org/wiki/Mapping_of_Unicode_character_planes) (which contains virtually all characters in common use[[11]](http://en.wikipedia.org/wiki/UTF-8#cite_note-unicode-ch02-bmp-11)).
Four bytes are needed for characters in the [other planes of Unicode](http://en.wikipedia.org/wiki/Mapping_of_Unicode_characters), which include less common [CJK characters](http://en.wikipedia.org/wiki/CJK_characters) and various historic scripts and mathematical symbols.

The bytes 0xFE and 0xFF do not appear, so a valid UTF-8 string cannot be confused with a UTF-16 sequence.

The second half (128-255) of the Latin-1 charset characters found in the previous table, are called "Latin-1 Supplement" in the Unicode standard.
They all have two bytes, except some of the additional Windows 1252 characters.
And they have the following encoding in UTF-8 (codes in hexadecimal):

|            |            |            |            |            |            |            |            |         |            |         |            |         |         |         |         |
|------------|------------|------------|------------|------------|------------|------------|------------|---------|------------|---------|------------|---------|---------|---------|---------|
| € E2 82 AC |            | ‚ E2 80 9A | ƒ C6 92    | „ E2 80 9E | … E2 80 A6 | † E2 80 A0 | ‡ E2 80 A1 | ˆ CB 86 | ‰ E2 80 B0 | Š C5 A0 | ‹ E2 80 B9 | Œ C5 92 |         | Ž C5 BD |         |
|            | ‘ E2 80 98 | ’ E2 80 99 | “ E2 80 9C | ” E2 80 9D | • E2 80 A2 | – E2 80 93 | — E2 80 94 | ˜ CB 9C | ™ E2 84 A2 | š C5 A1 | › E2 80 BA | œ C5 93 |         | ž C5 BE | Ÿ C5 B8 |
| C2 A0      | ¡ C2 A1    | ¢ C2 A2    | £ C2 A3    | ¤ C2 A4    | ¥ C2 A5    | ¦ C2 A6    | § C2 A7    | ¨ C2 A8 | © C2 A9    | ª C2 AA | « C2 AB    | ¬ C2 AC | C2 AD   | ® C2 AE | ¯ C2 AF |
| ° C2 B0    | ± C2 B1    | ² C2 B2    | ³ C2 B3    | ´ C2 B4    | µ C2 B5    | ¶ C2 B6    | · C2 B7    | ¸ C2 B8 | ¹ C2 B9    | º C2 BA | » C2 BB    | ¼ C2 BC | ½ C2 BD | ¾ C2 BE | ¿ C2 BF |
| À C3 80    | Á C3 81    | Â C3 82    | Ã C3 83    | Ä C3 84    | Å C3 85    | Æ C3 86    | Ç C3 87    | È C3 88 | É C3 89    | Ê C3 8A | Ë C3 8B    | Ì C3 8C | Í C3 8D | Î C3 8E | Ï C3 8F |
| Ð C3 90    | Ñ C3 91    | Ò C3 92    | Ó C3 93    | Ô C3 94    | Õ C3 95    | Ö C3 96    | × C3 97    | Ø C3 98 | Ù C3 99    | Ú C3 9A | Û C3 9B    | Ü C3 9C | Ý C3 9D | Þ C3 9E | ß C3 9F |
| à C3 A0    | á C3 A1    | â C3 A2    | ã C3 A3    | ä C3 A4    | å C3 A5    | æ C3 A6    | ç C3 A7    | è C3 A8 | é C3 A9    | ê C3 AA | ë C3 AB    | ì C3 AC | í C3 AD | î C3 AE | ï C3 AF |
| ð C3 B0    | ñ C3 B1    | ò C3 B2    | ó C3 B3    | ô C3 B4    | õ C3 B5    | ö C3 B6    | ÷ C3 B7    | ø C3 B8 | ù C3 B9    | ú C3 BA | û C3 BB    | ü C3 BC | ý C3 BD | þ C3 BE | ÿ C3 BF |

Adapted from <http://en.wikipedia.org/wiki/UTF-8>
