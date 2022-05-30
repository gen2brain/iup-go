
This directory contains files which are a moral replacements for some standard
Win32API headers which Microsoft decided to create incompatible with plain C
for no apparent reason.

Note that these dummy headers are very incomplete: It contains only the stuff
we need.

Also, to avoid clash with various standard headers (which may contain for
example forward declarations of functions) the dummy headers use the prefix
"dummy_" for all globally visible symbols.
