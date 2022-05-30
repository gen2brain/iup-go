/** \file
 * \brief HSI Color Manipulation
 * Copied and adapted from IM
 *
 * See Copyright Notice in "iup.h"
 */


#include <math.h>
#include <stdio.h>
#include "iup_colorhsi.h"

static const double rad60 =  1.0471975;
static const double rad120 = 2.0943951;
static const double rad180 = 3.1415926;
static const double rad240 = 4.1887902;
static const double rad300 = 5.2359877;
static const double rad360 = 6.2831853;
static const double sqrt3 = 1.7320508;
static const double rad2deg = 57.2957795131;


static double iColorNormHue(double H)
{
  while (H < 0.0) 
    H += rad360;

  if (H > rad360)  
    H = fmod(H, rad360);

  return H;
}

static unsigned char iColorQuantize(double value)
{
  if (value >= 1.0) return 255;
  if (value <= 0.0) return 0;
  return (unsigned char)(value*256);  
}                               

static double iColorReconstruct(unsigned char value)
{
  if (value == 0) return 0.0;
  if (value >= 255) return 1.0;
  return (((double)value + 0.5)/256.0);
}

static void iColorSmax01(double h, double hr, double hb, double hg, double *h0, double *h1)
{
  if (h < rad120)
    *h0 = hb;
  else if (h < rad240)
    *h0 = hr;
  else
    *h0 = hg;

  if (h < rad60 || h > rad300)
    *h1 = hr;
  else if (h < rad180)
    *h1 = hg;
  else
    *h1 = hb;
}

static double iColorHSI_Smax(double h, double cosH, double sinH, double i)
{
  double hr, hb, hg, imax, h0, h1;

  /* i here is normalized between 0-1 */

  if (i == 0.0 || i == 1.0)
    return 0.0;

  /* Making r=0, g=0, b=0, r=1, g=1 or b=1 in the parametric equations and
  writing s in function of H and I. */

  hr = (2.0 * cosH) / 3.0;
  hg = (-cosH + sinH*sqrt3) / 3.0;
  hb = (-cosH - sinH*sqrt3) / 3.0;

  /* at bottom */
  if (i <= 1.0 / 3.0)
  {
    /* face B=0 */
    if (h < rad120)
      return fabs(-i / hb);

    /* face R=0 */
    if (h < rad240)
      return fabs(-i / hr);

    /* face G=0 */
    return fabs(-i / hg);
  }

  /* at top */
  if (i >= 2.0 / 3.0)
  {
    /* face R=1 */
    if (h < rad60 || h > rad300)
      return fabs((1.0 - i) / hr);

    /* face G=1 */
    if (h < rad180)
      return fabs((1.0 - i) / hg);

    /* face B=1 */
    return fabs((1.0 - i) / hb);
  }

  /* in the middle */

  iColorSmax01(h, hr, hb, hg, &h0, &h1);

  if (h == 0.0 || h == rad120 || h == rad240)
    imax = 1.0 / 3.0;
  else if (h == rad60 || h == rad180 || h == rad300)
    imax = 2.0 / 3.0;
  else
    imax = fabs(h0 / (h0 - h1));

  if (i < imax)
    return fabs(-i / h0);
  else
    return fabs((1.0 - i) / h1);
}

/* Given H, returns I where S is max,
The maximum S is where top meets bottom.
- i / h0 = (1 - i) / h1
*/
static double iColorHSI_ImaxS(double h, double cosH, double sinH)
{
  double i, h0, h1;
  double hr, hb, hg;

  if (h == 0.0 || h == rad120 || h == rad240)
    return 1.0 / 3.0;

  if (h == rad60 || h == rad180 || h == rad300)
    return 2.0 / 3.0;

  hr = (2.0 * cosH) / 3.0;
  hg = (-cosH + sinH*sqrt3) / 3.0;
  hb = (-cosH - sinH*sqrt3) / 3.0;

  iColorSmax01(h, hr, hb, hg, &h0, &h1);

  i = h0 / (h0 - h1);

  return fabs(i);
}

static void iColorRGB2HSI(double r, double g, double b, double *h, double *s, double *i)
{            
  double v, u, ImaxS; 

  /* Parametric equations */
  v = r - (g + b)/2.0;
  u = (g - b) * (sqrt3/2.0);

  *i = (r + g + b) / 3.0;  /* already normalized to 0-1 */

  *s = sqrt(v*v + u*u);  /* s is between 0-1, BUT it is linear in the cube and it is in u,v space. */

  if (*s == 0.0)
  {
    /*  *h = <any>  (left unchanged) */
    ImaxS = 1.0/3.0;
  }
  else
  {
    double H, cosH, sinH;

    H = atan2(u, v);
    H = iColorNormHue(H);

    cosH = cos(H);
    sinH = sin(H);

    /* must scale S from 0-Smax to 0-1 */
    if (*i == 0.0 || *i == 1.0)
      *s = 0.0;  /* by definition */
    else
    {
      /* must scale S from 0-Smax to 0-1 */
      double Smax = iColorHSI_Smax(H, cosH, sinH, *i);
      *s /= Smax;
      if (*s > 1.0) /* because of round problems when calculating s and Smax */
        *s = 1.0;
    }

    *h = H * rad2deg;

    ImaxS = iColorHSI_ImaxS((double)H, cosH, sinH);
  }

  /* must convert I from linear scale to non-linear scale. USED ONLY FOR THE COLORBROWSER */
  if (*i < 0.5) /* half I is I=ImaxS, not I=0.5 */
    *i = ((*i)*0.5)/ImaxS;
  else
    *i = (((*i) - ImaxS)*0.5)/(1.0 - ImaxS) + 0.5;
}

static void iColorHSI2RGB(double h, double s, double i, double *r, double *g, double *b)
{
  double cosH, sinH, H, v, u;
  double Smax, ImaxS; 

  if (i < 0.0) i = 0.0;
  else if (i > 1.0) i = 1.0;

  if (s < 0.0) s = 0.0;
  else if (s > 1.0) s = 1.0;

  if (s == 0.0 || i == 1.0 || i == 0.0 || h == 360.0)
  {
    *r = i;
    *g = i;
    *b = i;
    return;
  }

  H = h/rad2deg;
  H = iColorNormHue(H);

  cosH = cos(H);
  sinH = sin(H);

  /* must convert I from non-linear scale to linear scale. USED ONLY FOR THE COLORBROWSER */
  ImaxS = iColorHSI_ImaxS(H, cosH, sinH);
  if (i < 0.5) /* half I is I=ImaxS, not I=0.5 */
    i  = ImaxS * (i / 0.5);
  else
    i = (1.0 - ImaxS) * (i - 0.5)/0.5 + ImaxS;

  /* must scale S from 0-1 to 0-Smax */
  Smax = iColorHSI_Smax(H, cosH, sinH, i);
  s *= Smax;
  if (s > 1.0) /* because of round problems when calculating s and Smax */
    s = 1.0;

  v = s * cosH;
  u = s * sinH;

  /* Inverse of the Parametric equations, using i normalized to 0-1 */
  *r = i + (2.0 * v)/3.0;
  *g = i - (v - u*sqrt3)/3.0;
  *b = i - (v + u*sqrt3)/3.0;

  /* fix round errors */
  if (*r < 0.0) *r = 0.0;
  if (*g < 0.0) *g = 0.0;
  if (*b < 0.0) *b = 0.0;
    
  if (*r > 1.0) *r = 1.0;
  if (*g > 1.0) *g = 1.0;
  if (*b > 1.0) *b = 1.0;
}

/*********************************************************************************************/

void iupColorRGB2HSI(unsigned char r, unsigned char g, unsigned char b, double *h, double *s, double *i)
{
  double fr = iColorReconstruct(r);
  double fg = iColorReconstruct(g);
  double fb = iColorReconstruct(b);
  
  iColorRGB2HSI(fr, fg, fb, h, s, i);
}

void iupColorHSI2RGB(double h, double s, double i, unsigned char *r, unsigned char *g, unsigned char *b)
{
  double fr, fg, fb;
  
  iColorHSI2RGB(h, s, i, &fr, &fg, &fb);
  
  *r = iColorQuantize(fr);
  *g = iColorQuantize(fg);
  *b = iColorQuantize(fb);
}

int iupStrToHSI(const char *str, double *h, double *s, double *i)
{
  double fh, fs, fi;
  if (!str) return 0;
  if (sscanf(str, "%lf %lf %lf", &fh, &fs, &fi) != 3) return 0;
  if (fh > 359.0 || fs > 1.0 || fi > 1.0) return 0;
  if (fh < 0.0 || fs < 0.0 || fi < 0.0) return 0;
  *h = fh;
  *s = fs;
  *i = fi;
  return 1;
}
