/** \file
 * \brief Plot component for Iup.
 *
 * See Copyright Notice in "iup.h"
 */
 
#ifndef __IUP_PLOT_H 
#define __IUP_PLOT_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef IUPPLOT_API
#ifdef IUPPLOT_BUILD_LIBRARY
  #ifdef __EMSCRIPTEN__
    #include <emscripten.h>
    #define IUPPLOT_API EMSCRIPTEN_KEEPALIVE
  #elif defined(_WIN32)
    #define IUPPLOT_API __declspec(dllexport)
  #elif defined(__GNUC__) && __GNUC__ >= 4
    #define IUPPLOT_API __attribute__ ((visibility("default")))
  #else
    #define IUPPLOT_API
  #endif
#else
  #define IUPPLOT_API
#endif /* IUPPLOT_BUILD_LIBRARY */
#endif /* IUPPLOT_API */

/* Initialize IupPlot widget class */
IUPPLOT_API void IupPlotOpen(void);

/* Create an IupPlot widget instance */
IUPPLOT_API Ihandle* IupPlot(void);

/***********************************************/
/*           Additional API                    */

IUPPLOT_API void IupPlotBegin(Ihandle *ih, int strXdata);
IUPPLOT_API void IupPlotAdd(Ihandle *ih, double x, double y);
IUPPLOT_API void IupPlotAddStr(Ihandle *ih, const char* x, double y);
IUPPLOT_API void IupPlotAddSegment(Ihandle *ih, double x, double y);
IUPPLOT_API int  IupPlotEnd(Ihandle *ih);

IUPPLOT_API int  IupPlotLoadData(Ihandle* ih, const char* filename, int strXdata);

IUPPLOT_API void IupPlotInsert(Ihandle *ih, int ds_index, int sample_index, double x, double y);
IUPPLOT_API void IupPlotInsertStr(Ihandle *ih, int ds_index, int sample_index, const char* x, double y);
IUPPLOT_API void IupPlotInsertSegment(Ihandle *ih, int ds_index, int sample_index, double x, double y);

IUPPLOT_API void IupPlotInsertStrSamples(Ihandle* ih, int ds_index, int sample_index, const char** x, double* y, int count);
IUPPLOT_API void IupPlotInsertSamples(Ihandle* ih, int ds_index, int sample_index, double *x, double *y, int count);

IUPPLOT_API void IupPlotAddSamples(Ihandle* ih, int ds_index, double *x, double *y, int count);
IUPPLOT_API void IupPlotAddStrSamples(Ihandle* ih, int ds_index, const char** x, double* y, int count);

IUPPLOT_API void IupPlotGetSample(Ihandle* ih, int ds_index, int sample_index, double *x, double *y);
IUPPLOT_API void IupPlotGetSampleStr(Ihandle* ih, int ds_index, int sample_index, const char* *x, double *y);
IUPPLOT_API int  IupPlotGetSampleSelection(Ihandle* ih, int ds_index, int sample_index);
IUPPLOT_API double IupPlotGetSampleExtra(Ihandle* ih, int ds_index, int sample_index);
IUPPLOT_API void IupPlotSetSample(Ihandle* ih, int ds_index, int sample_index, double x, double y);
IUPPLOT_API void IupPlotSetSampleStr(Ihandle* ih, int ds_index, int sample_index, const char* x, double y);
IUPPLOT_API void IupPlotSetSampleSelection(Ihandle* ih, int ds_index, int sample_index, int selected);
IUPPLOT_API void IupPlotSetSampleExtra(Ihandle* ih, int ds_index, int sample_index, double extra);

IUPPLOT_API void IupPlotTransform(Ihandle* ih, double x, double y, double *cnv_x, double *cnv_y);
IUPPLOT_API void IupPlotTransformTo(Ihandle* ih, double cnv_x, double cnv_y, double *x, double *y);

IUPPLOT_API int  IupPlotFindSample(Ihandle* ih, double cnv_x, double cnv_y, int *ds_index, int *sample_index);
IUPPLOT_API int  IupPlotFindSegment(Ihandle* ih, double cnv_x, double cnv_y, int *ds_index, int *sample_index1, int *sample_index2);

/***********************************************/


#ifdef __cplusplus
}
#endif

#endif
