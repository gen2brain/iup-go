/** \file
 * \brief Class Callback Utilities.
 */

#ifndef __IUPCBS_HPP
#define __IUPCBS_HPP

#define IUP_CLASS_GET_OBJECT(__ih, __class) dynamic_cast<__class*>((__class*)IupGetAttribute(__ih, #__class "->this"))

#define IUP_CLASS_INITCALLBACK(__ih, __class) \
  IupSetAttribute(__ih, #__class "->this", (char*)this)

#define IUP_CLASS_SETCALLBACK(__ih, __name, __cb) \
  IupSetCallback(__ih, __name, (Icallback)CB_##__cb)

#ifdef __IUP_PLUS_H

#define IUP_PLUS_GET_OBJECT(__elem, __class) dynamic_cast<__class*>((__class*)IupGetAttribute(__elem.GetHandle(), #__class "->this"))

#define IUP_PLUS_INITCALLBACK(__elem, __class) \
  IupSetAttribute(__elem.GetHandle(), #__class "->this", (char*)this)

#define IUP_PLUS_SETCALLBACK(__elem, __name, __cb) \
  IupSetCallback(__elem.GetHandle(), __name, (Icallback)CB_##__cb)

#endif

#define IUP_CLASS_DECLARECALLBACK_IFn(__class, __cb) \
  int __cb(Ihandle* ih); \
  static int CB_##__cb(Ihandle* ih) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFni(__class, __cb) \
  int __cb(Ihandle* ih, int i1); \
  static int CB_##__cb(Ihandle* ih, int i1) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, i1); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFnii(__class, __cb) \
  int __cb(Ihandle* ih, int i1, int i2); \
  static int CB_##__cb(Ihandle* ih, int i1, int i2) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, i1, i2); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFniii(__class, __cb) \
  int __cb(Ihandle* ih, int i1, int i2, int i3); \
  static int CB_##__cb(Ihandle* ih, int i1, int i2, int i3) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, i1, i2, i3); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFniiii(__class, __cb) \
  int __cb(Ihandle* ih, int i1, int i2, int i3, int i4); \
  static int CB_##__cb(Ihandle* ih, int i1, int i2, int i3, int i4) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, i1, i2, i3, i4); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFniiiii(__class, __cb) \
  int __cb(Ihandle* ih, int i1, int i2, int i3, int i4, int i5); \
  static int CB_##__cb(Ihandle* ih, int i1, int i2, int i3, int i4, int i5) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, i1, i2, i3, i4, i5); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFniiiiii(__class, __cb) \
  int __cb(Ihandle* ih, int i1, int i2, int i3, int i4, int i5, int i6); \
  static int CB_##__cb(Ihandle* ih, int i1, int i2, int i3, int i4, int i5, int i6) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, i1, i2, i3, i4, i5, i6); \
  }

#define IUP_CLASS_DECLARECALLBACK_dIFnii(__class, __cb) \
  double __cb(Ihandle* ih, int i1, int i2); \
  static double CB_##__cb(Ihandle* ih, int i1, int i2) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, i1, i2); \
  }

#define IUP_CLASS_DECLARECALLBACK_sIFni(__class, __cb) \
  char* __cb(Ihandle* ih, int i1); \
  static char* CB_##__cb(Ihandle* ih, int i1) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, i1); \
  }

#define IUP_CLASS_DECLARECALLBACK_sIFnii(__class, __cb) \
  char* __cb(Ihandle* ih, int i1, int i2); \
  static char* CB_##__cb(Ihandle* ih, int i1, int i2) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, i1, i2); \
  }

#define IUP_CLASS_DECLARECALLBACK_sIFniis(__class, __cb) \
  char* __cb(Ihandle* ih, int i1, int i2, char* s); \
  static char* CB_##__cb(Ihandle* ih, int i1, int i2, char* s) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, i1, i2, s); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFnff(__class, __cb) \
  int __cb(Ihandle* ih, float f1, float f2); \
  static int CB_##__cb(Ihandle* ih, float f1, float f2) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, f1, f2); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFniff(__class, __cb) \
  int __cb(Ihandle* ih, int i1, float f1, float f2); \
  static int CB_##__cb(Ihandle* ih, int i1, float f1, float f2) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, i1, f1, f2); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFnfiis(__class, __cb) \
  int __cb(Ihandle* ih, float f1, int i1, int i2, char* s); \
  static int CB_##__cb(Ihandle* ih, float f1, int i1, int i2, char* s) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, f1, i1, i2, s); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFnd(__class, __cb) \
  int __cb(Ihandle* ih, double d1); \
  static int CB_##__cb(Ihandle* ih, double d1) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, d1); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFndds(__class, __cb) \
  int __cb(Ihandle* ih, double d1, double d2, char* s); \
  static int CB_##__cb(Ihandle* ih, double d1, double d2, char* s) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, d1, d2, s); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFniid(__class, __cb) \
  int __cb(Ihandle* ih, int i1, int i2, double d1); \
  static int CB_##__cb(Ihandle* ih, int i1, int i2, double d1) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, i1, i2, d1); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFniidd(__class, __cb) \
  int __cb(Ihandle* ih, int i1, int i2, double d1, double d2); \
  static int CB_##__cb(Ihandle* ih, int i1, int i2, double d1, double d2) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, i1, i2, d1, d2); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFniiddi(__class, __cb) \
  int __cb(Ihandle* ih, int i1, int i2, double d1, double d2, int i3); \
  static int CB_##__cb(Ihandle* ih, int i1, int i2, double d1, double d2, int i3) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, i1, i2, d1, d2, i3); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFniidds(__class, __cb) \
  int __cb(Ihandle* ih, int i1, int i2, double d1, double d2, char* s); \
  static int CB_##__cb(Ihandle* ih, int i1, int i2, double d1, double d2, char* s) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, i1, i2, d1, d2, s); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFniiIII(__class, __cb) \
  int __cb(Ihandle* ih, int i1, int i2, int* I1, int* I2, int* I3); \
  static int CB_##__cb(Ihandle* ih, int i1, int i2, int* I1, int* I2, int* I3) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, i1, i2, I1, I2, I3); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFniIIII(__class, __cb) \
  int __cb(Ihandle* ih, int i1, int* I1, int* I2, int* I3, int* I4); \
  static int CB_##__cb(Ihandle* ih, int i1, int* I1, int* I2, int* I3, int* I4) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, i1, I1, I2, I3, I4); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFnIi(__class, __cb) \
  int __cb(Ihandle* ih, int* I1, int i1); \
  static int CB_##__cb(Ihandle* ih, int* I1, int i1) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, I1, i1); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFnccc(__class, __cb) \
  int __cb(Ihandle* ih, char c1, char c2, char c3); \
  static int CB_##__cb(Ihandle* ih, char c1, char c2, char c3) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, c1, c2, c3); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFnis(__class, __cb) \
  int __cb(Ihandle* ih, int i1, char* s); \
  static int CB_##__cb(Ihandle* ih, int i1, char* s) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, i1, s); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFniis(__class, __cb) \
  int __cb(Ihandle* ih, int i1, int i2, char* s); \
  static int CB_##__cb(Ihandle* ih, int i1, int i2, char* s) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, i1, i2, s); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFniiis(__class, __cb) \
  int __cb(Ihandle* ih, int i1, int i2, int i3, char* s); \
  static int CB_##__cb(Ihandle* ih, int i1, int i2, int i3, char* s) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, i1, i2, i3, s); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFniiiis(__class, __cb) \
  int __cb(Ihandle* ih, int i1, int i2, int i3, int i4, char* s); \
  static int CB_##__cb(Ihandle* ih, int i1, int i2, int i3, int i4, char* s) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, i1, i2, i3, i4, s); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFniiiiis(__class, __cb) \
  int __cb(Ihandle* ih, int i1, int i2, int i3, int i4, int i5, char* s); \
  static int CB_##__cb(Ihandle* ih, int i1, int i2, int i3, int i4, int i5, char* s) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, i1, i2, i3, i4, i5, s); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFniiiiiis(__class, __cb) \
  int __cb(Ihandle* ih, int i1, int i2, int i3, int i4, int i5, int i6, char* s); \
  static int CB_##__cb(Ihandle* ih, int i1, int i2, int i3, int i4, int i5, int i6, char* s) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, i1, i2, i3, i4, i5, i6, s); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFnss(__class, __cb) \
  int __cb(Ihandle* ih, char* s1, char* s2); \
  static int CB_##__cb(Ihandle* ih, char* s1, char* s2) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, s1, s2); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFns(__class, __cb) \
  int __cb(Ihandle* ih, char* s1); \
  static int CB_##__cb(Ihandle* ih, char* s1) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, s1); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFnsi(__class, __cb) \
  int __cb(Ihandle* ih, char* s1, int i1); \
  static int CB_##__cb(Ihandle* ih, char* s1, int i1) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, s1, i1); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFnsii(__class, __cb) \
  int __cb(Ihandle* ih, char* s1, int i1, int i2); \
  static int CB_##__cb(Ihandle* ih, char* s1, int i1, int i2) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, s1, i1, i2); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFnsiii(__class, __cb) \
  int __cb(Ihandle* ih, char* s1, int i1, int i2, int i3); \
  static int CB_##__cb(Ihandle* ih, char* s1, int i1, int i2, int i3) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, s1, i1, i2, i3); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFnnii(__class, __cb) \
  int __cb(Ihandle* ih, Ihandle* ih1, int i1, int i2); \
  static int CB_##__cb(Ihandle* ih, Ihandle* ih1, int i1, int i2) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, ih1, i1, i2); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFnnn(__class, __cb) \
  int __cb(Ihandle* ih, Ihandle* ih1, Ihandle* ih2); \
  static int CB_##__cb(Ihandle* ih, Ihandle* ih1, Ihandle* ih2) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, ih1, ih2); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFniinsii(__class, __cb) \
  int __cb(Ihandle* ih, int i1, int i2, Ihandle* ih1, char* s, int i3, int i4); \
  static int CB_##__cb(Ihandle* ih, int i1, int i2, Ihandle* ih1, char* s, int i3, int i4) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, i1, i2, ih1, s, i3, i4); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFnsVi(__class, __cb) \
  int __cb(Ihandle* ih, char* s1, void* V1, int i1); \
  static int CB_##__cb(Ihandle* ih, char* s1, void* V1, int i1) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, s1, V1, i1); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFnsViii(__class, __cb) \
  int __cb(Ihandle* ih, char* s1, void* V1, int i1, int i2, int i3); \
  static int CB_##__cb(Ihandle* ih, char* s1, void* V1, int i1, int i2, int i3) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, s1, V1, i1, i2, i3); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFnn(__class, __cb) \
  int __cb(Ihandle* ih, Ihandle* ih1); \
  static int CB_##__cb(Ihandle* ih, Ihandle* ih1) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, ih1); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFnni(__class, __cb) \
  int __cb(Ihandle* ih, Ihandle* ih1, int i1); \
  static int CB_##__cb(Ihandle* ih, Ihandle* ih1, int i1) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, ih1, i1); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFniisi(__class, __cb) \
  int __cb(Ihandle* ih, int i1, int i2, char* s, int i3); \
  static int CB_##__cb(Ihandle* ih, int i1, int i2, char* s, int i3) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, i1, i2, s, i3); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFnsidv(__class, __cb) \
  int __cb(Ihandle* ih, char* s, int i1, double d1, void* V1); \
  static int CB_##__cb(Ihandle* ih, char* s, int i1, double d1, void* V1) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, s, i1, d1, V1); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFnssi(__class, __cb) \
  int __cb(Ihandle* ih, char* s1, char* s2, int i1); \
  static int CB_##__cb(Ihandle* ih, char* s1, char* s2, int i1) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, s1, s2, i1); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFniiddiddi(__class, __cb) \
  int __cb(Ihandle* ih, int i1, int i2, double d1, double d2, int i3, double d3, double d4, int i4); \
  static int CB_##__cb(Ihandle* ih, int i1, int i2, double d1, double d2, int i3, double d3, double d4, int i4) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, i1, i2, d1, d2, i3, d3, d4, i4); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFnssds(__class, __cb) \
  int __cb(Ihandle* ih, char* s1, char* s2, double d1, char* s3); \
  static int CB_##__cb(Ihandle* ih, char* s1, char* s2, double d1, char* s3) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    return obj->__cb(ih, s1, s2, d1, s3); \
  }

#define IUP_CLASS_DECLARECALLBACK_IFniiv(__class, __cb) \
  void __cb(Ihandle* ih, int i1, int i2, void* V1); \
  static void CB_##__cb(Ihandle* ih, int i1, int i2, void* V1) \
  { \
    __class* obj = IUP_CLASS_GET_OBJECT(ih, __class); \
    obj->__cb(ih, i1, i2, V1); \
  }

#endif
