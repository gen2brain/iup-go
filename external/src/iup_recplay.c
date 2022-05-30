/** \file
 * \brief global attributes environment
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdlib.h>      
#include <stdio.h>      
#include <string.h>      
#include <time.h>

#include "iup.h" 
#include "iup_key.h"

#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_drvinfo.h"
#include "iup_str.h"


static FILE* irec_file = NULL;
static int irec_lastclock = 0;
static int irec_mode = 0;

static int iRecClock(void)
{
  return (int)((clock()*1000)/CLOCKS_PER_SEC); /* time in miliseconds */
}

static void iRecWriteInt(FILE* file, int value, int mode)
{
  if (mode == IUP_RECTEXT)
    fprintf(file, "%d ", value);
  else
    fwrite(&value, sizeof(int), 1, file);
}

static void iRecWriteFloat(FILE* file, float value, int mode)
{
  if (mode == IUP_RECTEXT)
    fprintf(file, IUP_FLOAT2STR" ", value);
  else
    fwrite(&value, sizeof(float), 1, file);
}

static void iRecWriteChar(FILE* file, char value, int mode)
{
  if (mode == IUP_RECTEXT)
    fprintf(file, "%c ", value);
  else
    fwrite(&value, 1, 1, file);
}

static void iRecWriteByte(FILE* file, char value, int mode)
{
  if (mode == IUP_RECTEXT)
    fprintf(file, "%d ", (int)value);
  else
    fwrite(&value, 1, 1, file);
}

static void iRecWriteStr(FILE* file, char* value, int mode)
{
  fwrite(value, 1, strlen(value), file);

  if (mode == IUP_RECTEXT)
    fwrite(" ", 1, 1, file);
}

static void iRecInputWheelCB(float delta, int x, int y, char* status)
{
  (void)status;
  if (irec_file)
  {
    int time = iRecClock() - irec_lastclock;
    iRecWriteStr(irec_file, "WHE", irec_mode);
    iRecWriteInt(irec_file, time, irec_mode);
    iRecWriteFloat(irec_file, delta, irec_mode);
    iRecWriteInt(irec_file, x, irec_mode);
    iRecWriteInt(irec_file, y, irec_mode);
    iRecWriteByte(irec_file, '\n', IUP_RECBINARY);  /* no space after */
    irec_lastclock = iRecClock();
  }
}

static void iRecInputButtonCB(int button, int pressed, int x, int y, char* status)
{
  if (irec_file)
  {
    int time = iRecClock() - irec_lastclock;
    if (pressed && iup_isdouble(status)) pressed = 2;
    iRecWriteStr(irec_file, "BUT", irec_mode);
    iRecWriteInt(irec_file, time, irec_mode);
    iRecWriteChar(irec_file, (char)button, irec_mode);
    iRecWriteByte(irec_file, (char)pressed, irec_mode);
    iRecWriteInt(irec_file, x, irec_mode);
    iRecWriteInt(irec_file, y, irec_mode);
    iRecWriteByte(irec_file, '\n', IUP_RECBINARY);  /* no space after */
    irec_lastclock = iRecClock();
  }
}

static void iRecInputMotionCB(int x, int y, char* status)
{
  if (irec_file)
  {
    char button = '0';
    int time = iRecClock() - irec_lastclock;
    iRecWriteStr(irec_file, "MOV", irec_mode);
    iRecWriteInt(irec_file, time, irec_mode);
    iRecWriteInt(irec_file, x, irec_mode);
    iRecWriteInt(irec_file, y, irec_mode);
    if (iup_isbutton1(status)) button = '1';
    if (iup_isbutton2(status)) button = '2';
    if (iup_isbutton3(status)) button = '3';
    if (iup_isbutton4(status)) button = '4';
    if (iup_isbutton5(status)) button = '5';
    iRecWriteChar(irec_file, button, irec_mode);
    iRecWriteByte(irec_file, '\n', IUP_RECBINARY);  /* no space after */
    irec_lastclock = iRecClock();
  }
}

static void iRecInputKeyPressCB(int key, int pressed)
{
  if (irec_file)
  {
    int time = iRecClock() - irec_lastclock;
    iRecWriteStr(irec_file, "KEY", irec_mode);
    iRecWriteInt(irec_file, time, irec_mode);
    iRecWriteInt(irec_file, key, irec_mode);
    iRecWriteByte(irec_file, (char)pressed, irec_mode);
    iRecWriteByte(irec_file, '\n', IUP_RECBINARY);  /* no space after */
    irec_lastclock = iRecClock();
  }
}

IUP_API int IupRecordInput(const char* filename, int mode)
{
  if (irec_file)
    fclose(irec_file);

  if (filename)
  {
    irec_file = fopen(filename, "wb");
    if (!irec_file)
      return IUP_ERROR;
    irec_mode = mode;
  }
  else
    irec_file = NULL;

  if (irec_file)
  {
    char* mode_str[3] = {"BIN", "TXT", "SYS"};
    iRecWriteStr(irec_file, "IUPINPUT", IUP_RECTEXT);  /* add space after, even for non text mode */
    iRecWriteStr(irec_file, mode_str[irec_mode], IUP_RECBINARY); /* no space after */
    iRecWriteByte(irec_file, '\n', IUP_RECBINARY);  /* no space after */
    irec_lastclock = iRecClock();

    IupSetGlobal("INPUTCALLBACKS", "Yes");
    IupSetFunction("GLOBALWHEEL_CB", (Icallback)iRecInputWheelCB);
    IupSetFunction("GLOBALBUTTON_CB", (Icallback)iRecInputButtonCB);
    IupSetFunction("GLOBALMOTION_CB", (Icallback)iRecInputMotionCB);
    IupSetFunction("GLOBALKEYPRESS_CB", (Icallback)iRecInputKeyPressCB);
  }
  else
  {
    IupSetGlobal("INPUTCALLBACKS", "No");
    IupSetFunction("GLOBALWHEEL_CB", NULL);
    IupSetFunction("GLOBALBUTTON_CB", NULL);
    IupSetFunction("GLOBALMOTION_CB", NULL);
    IupSetFunction("GLOBALKEYPRESS_CB", NULL);
  }

  return IUP_NOERROR;
}


/*************************************************************************************/


static void iPlayReadInt(FILE* file, int *value, int mode)
{
  if (mode == IUP_RECTEXT)
    fscanf(file, "%d ", value);
  else
    fread(value, sizeof(int), 1, file);
}

static void iPlayReadFloat(FILE* file, float *value, int mode)
{
  if (mode == IUP_RECTEXT)
    fscanf(file, "%f ", value);
  else
    fread(value, sizeof(float), 1, file);
}

static void iPlayReadByte(FILE* file, char *value, int mode)
{
  if (mode == IUP_RECTEXT)
  {
    int ivalue;
    fscanf(file, "%d ", &ivalue);
    *value = (char)ivalue;
  }
  else
    fread(value, 1, 1, file);
}

static void iPlayReadChar(FILE* file, char *value, int mode)
{
  if (mode == IUP_RECTEXT)
    fscanf(file, "%c ", value);
  else
    fread(value, 1, 1, file);
}

static void iPlayReadStr(FILE* file, char* value, int len, int mode)
{
  fread(value, 1, len, file);
  value[len] = 0;

  if (mode == IUP_RECTEXT)
  {
    char spc;
    fread(&spc, 1, 1, file); /* skip space */
  }
}

static int iPlayAction(FILE* file, int mode)
{
  char action[4];
  char eol;
  int time;
  static int last_pressed = 0;

  iPlayReadStr(file, action, 3, mode);
  iPlayReadInt(file, &time, mode);
  if (ferror(file)) return -1;

  time -= iRecClock() - irec_lastclock;
  if (time < 0) time = 0;
  if (time)
    iupdrvSleep(time);

  switch (action[0])
  {
  case 'B':
    {
      char button, status;
      int x, y;
      iPlayReadChar(file, &button, mode);
      iPlayReadByte(file, &status, mode);
      iPlayReadInt(file, &x, mode);
      iPlayReadInt(file, &y, mode);
      if (mode == IUP_RECBINARY) iPlayReadByte(file, &eol, mode);
      if (ferror(file)) return -1;

      /* IupSetfAttribute(NULL, "MOUSEBUTTON", "%dx%d %c %d", x, y, button, (int)status);*/
      iupdrvSendMouse(x, y, (int)button, (int)status);

      /* Process all messages between button press and release without interruption.
         This will not work if two butons are pressed together. */
/*      if (status == 1 || status == 2)
        last_pressed = 1;
      else if (status == 0)
        last_pressed = 0; */
      break;
    }
  case 'M':
    {
      char button;
      int x, y;
      iPlayReadInt(file, &x, mode);
      iPlayReadInt(file, &y, mode);
      iPlayReadChar(file, &button, mode);
      if (mode == IUP_RECBINARY) iPlayReadByte(file, &eol, mode);
      if (ferror(file)) return -1;

      /* IupSetfAttribute(NULL, "CURSORPOS", "%dx%d", x, y); */
      iupdrvSendMouse(x, y, (int)button, -1);
      break;
    }
  case 'K':
    {
      int key;
      char pressed;
      iPlayReadInt(file, &key, mode);
      iPlayReadByte(file, &pressed, mode);
      if (mode == IUP_RECBINARY) iPlayReadByte(file, &eol, mode);
      if (ferror(file)) return -1;

      if (pressed)
        /* IupSetInt(NULL, "KEYPRESS", key); */
        iupdrvSendKey(key, 0x01);
      else
        /* IupSetInt(NULL, "KEYRELEASE", key); */
        iupdrvSendKey(key, 0x02);
      break;
    }
  case 'W':
    {
      float delta;
      int x, y;
      iPlayReadFloat(file, &delta, mode);
      iPlayReadInt(file, &x, mode);
      iPlayReadInt(file, &y, mode);
      if (mode == IUP_RECBINARY) iPlayReadByte(file, &eol, mode);
      if (ferror(file)) return -1;

      /* IupSetfAttribute(NULL, "MOUSEBUTTON", "%dx%d %c %d", x, y, 'W', (int)delta);*/
      iupdrvSendMouse(x, y, 'W', (int)delta);
      break;
    }
  default:
      return -1;
  }

  irec_lastclock = iRecClock();
  return last_pressed;
}

static int iPlayTimer_CB(Ihandle* timer)
{
  FILE* file = (FILE*)IupGetAttribute(timer, "_IUP_PLAYFILE");
  if(feof(file) || ferror(file))
  {
    fclose(file);
    IupSetAttribute(timer, "RUN", "NO");
    IupDestroy(timer);
    IupSetGlobal("_IUP_PLAYTIMER", NULL);
    return IUP_IGNORE;
  }
  else
  {
    int cont = 1;
    int mode = IupGetInt(timer, "_IUP_PLAYMODE");

/*    while (cont)    //did not work, menus do not receive the click, why? */
    {
      cont = iPlayAction(file, mode);

      if (cont == -1)  /* error */
      {
        fclose(file);
        IupSetAttribute(timer, "RUN", "NO");
        IupDestroy(timer);
        IupSetGlobal("_IUP_PLAYTIMER", NULL);
        return IUP_IGNORE;
      }
    }

    IupFlush();
  }

  return IUP_DEFAULT;
}

IUP_API int IupPlayInput(const char* filename)
{
  Ihandle* timer = (Ihandle*)IupGetGlobal("_IUP_PLAYTIMER");
  FILE* file;
  char sig[9], mode_str[4];
  int mode;

  if (timer)
  {
    if (filename && filename[0]==0)
    {
      if (IupGetInt(timer, "RUN"))
        IupSetAttribute(timer, "RUN", "NO");
      else
        IupSetAttribute(timer, "RUN", "Yes");
      return IUP_NOERROR;
    }

    file = (FILE*)IupGetAttribute(timer, "_IUP_PLAYFILE");

    fclose(file);
    IupSetAttribute(timer, "RUN", "NO");
    IupDestroy(timer);
    IupSetGlobal("_IUP_PLAYTIMER", NULL);
  }
  else
  {
    if (!filename || filename[0]==0)
      return IUP_ERROR;
  }

  if (!filename)
    return IUP_NOERROR;

  file = fopen(filename, "rb");
  if (!file)
    return IUP_ERROR;

  iPlayReadStr(file, sig, 8, IUP_RECTEXT);
  iPlayReadStr(file, mode_str, 3, IUP_RECTEXT); /* read also the eol */
  if (ferror(file))
  {
    fclose(file);
    return IUP_ERROR;
  }

  if (!iupStrEqual(sig, "IUPINPUT"))
  {
    fclose(file);
    return IUP_ERROR;
  }

  mode = IUP_RECBINARY;
  if (iupStrEqual(mode_str, "TXT"))
    mode = IUP_RECTEXT;

  irec_lastclock = iRecClock();

  timer = IupTimer();
  IupSetCallback(timer, "ACTION_CB", (Icallback)iPlayTimer_CB);
  IupSetAttribute(timer, "TIME", "20");
  IupSetAttribute(timer, "_IUP_PLAYFILE", (char*)file);
  IupSetInt(timer, "_IUP_PLAYMODE", mode);
  IupSetAttribute(timer, "RUN", "YES");

  IupSetGlobal("_IUP_PLAYTIMER", (char*)timer);
  return IUP_NOERROR;
}
