
# WinDrawLib

Home: http://github.com/mity/windrawlib

WinDrawLib is C library for Windows for easy-to-use painting with Direct2D
or, on older Windows versions, GDI+.

The WinDrawLib was created by isolating and cleaning a helper painting code
from the [mCtrl](http://mctrl.org) project as it seems to be very useful on its
own.


## Building WinDrawLib

To build WinDrawLib you need to use CMake to generate MS Visual Studio solution
or to generate Makefile. Then build that normally using your tool chain.

For example, to build with MSYS + [mingw-w64](http://mingw-w64.org) + Make:
```sh
$ cd path/to/windrawlib
$ mkdir build
$ cd build
$ cmake -G "MSYS Makefiles" ..
$ make
```

Static lib of WinDrawLib is built as well as few examples using the library.


## Using WinDrawLib

Use WinDrawLib as a normal static library. There is single public header file,
`wdl.h` in the include directory which you need to #include in your sources.

API is documented directly in the public header.

Then link with the static library `WINDRAWLIB.LIB` (Visual Studio) or
`LIBWINDRAWLIB.A` (gcc).


## Release Cycle and API Stability

Note the API is not considered rock-stable and it may be in a constant flux,
although hopefully it will converge to something more or less stable in near
future.

Also there is no typical release process and no official binary packages.
The library is quite small and simple and the intended use is to directly
embed it into a client project.


## License

WinDrawLib is covered with MIT license:

```
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
