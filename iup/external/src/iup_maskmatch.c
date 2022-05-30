/** \file
 * \brief iupMask functions
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "iup_maskparse.h"
#include "iup_maskmatch.h"


#define IMASK_MIN_STACK_ELEMENTS 1000
enum {IMASK_CAPT_OPEN, IMASK_CAPT_CLOSE};

typedef struct _ImaskCapt
{
  struct _ImaskCapt* next_one;
  int type;
  short which_one;
  long pos;
} ImaskCapt;

typedef struct _ImaskMatchVars
{
  const char *text;
  ImaskParsed *fsm;
  iMaskMatchFunc function;
  short *tested;
  void *user;
} ImaskMatchVars;


typedef struct _ImaskStack
{
  short *stack;
  short size;
} ImaskStack;


#define isalphanum(_x) (isalnum((int)(unsigned char)(_x)) || ((_x) == '_'))


/* match functions corresponding to regular expressions */

static int match_blanks (const char *text, long j)
{
  return (((text[j] == '\t') || (text[j] == '\xff') || (text[j] == ' ') ||
     (text[j] == '\n'))) ? IMASK_NORMAL_MATCH : IMASK_NO_MATCH;
}

static int match_non_blanks (const char *text, long j)
{
  return (!((text[j] == '\t') || (text[j] == '\xff') ||
      (text[j] == ' ') || (text[j] == '\n')))
    ? IMASK_NORMAL_MATCH : IMASK_NO_MATCH;
}

static int match_alpha (const char *text, long j)
{
  return (isalpha((int)(unsigned char)text[j])) ? IMASK_NORMAL_MATCH : IMASK_NO_MATCH;
}

static int match_non_alpha (const char *text, long j)
{
  return (!isalpha((int)(unsigned char)text[j]) && (text[j] != '\0'))
    ? IMASK_NORMAL_MATCH : IMASK_NO_MATCH;
}

static int match_digit (const char *text, long j)
{
  return (isdigit((int)(unsigned char)text[j])) ? IMASK_NORMAL_MATCH : IMASK_NO_MATCH;
}

static int match_non_digit (const char *text, long j)
{
  return (!isdigit((int)(unsigned char)text[j]) && (text[j] != '\0'))
    ? IMASK_NORMAL_MATCH : IMASK_NO_MATCH;
}

static int match_alphanum (const char *text, long j)
{
  return isalphanum (text[j]) ? IMASK_NORMAL_MATCH : IMASK_NO_MATCH;
}

static int match_non_alphanum (const char *text, long j)
{
  return (isalphanum (text[j]) || (text[j] == '\0'))
    ? IMASK_NO_MATCH : IMASK_NORMAL_MATCH;
}

static int match_word_boundary (const char *text, long j)
{
  if ((j == 0) && isalphanum (text[j]))
    return IMASK_NO_CHAR_MATCH;

  else if (isalphanum (text[j - 1]) && !isalphanum (text[j]))
    return IMASK_NO_CHAR_MATCH;

  else if (isalphanum (text[j]) && !isalphanum (text[j - 1]))
    return IMASK_NO_CHAR_MATCH;

  return IMASK_NO_MATCH;

}

static ImaskMatchFunc imask_match_functions[] =
{
  {'w', &match_alphanum},
  {'W', &match_non_alphanum},
  {'d', &match_digit},
  {'D', &match_non_digit},
  {'S', &match_non_blanks},
  {'s', &match_blanks},
  {'b', &match_word_boundary},
  {'l', &match_alpha},
  {'L', &match_non_alpha},
  {'\0', NULL}
};

ImaskMatchFunc* iupMaskMatchGetFuncs(void)
{
  return imask_match_functions;
}

static void iMaskMatchCaptureResult (ImaskMatchVars * vars, ImaskCapt * capture)
{
  ImaskCapt *next = NULL;

  while (capture != NULL)
  {
    ImaskCapt *cap = capture->next_one;

    capture->next_one = next;
    next = capture;
    capture = cap;
  }

  capture = next;
  next = NULL;

  while (capture != NULL)
  {
    if (capture->type == IMASK_CAPT_OPEN)
    {
      ImaskCapt *cap = capture->next_one;

      capture->next_one = next;
      next = capture;
      capture = cap;
    }
    else
    {
      if (next) /* should always be valid - safety check */
      {
        if (capture->pos >= next->pos)
          (*vars->function)((char)capture->which_one, next->pos, capture->pos, vars->text, vars->user);

        next = next->next_one;
      }

      capture = capture->next_one;
    }
  }
}

static long iMaskMatchRecursive (ImaskMatchVars * vars, long j, int state, ImaskCapt * capture, int size)
{
  switch (vars->fsm[state].command)
  {
  case IMASK_NULL_CMD:
    if (vars->fsm[state].next1 == 0)    /*se chegou ao fim da maquina de estados */
    {
      if (vars->function != NULL)
        iMaskMatchCaptureResult (vars, capture);         /* guarda capturas */

      return j;
    }

    /* verifica o estado atual ja foi avaliado antes */
    {
      int count;
      for (count = 0; count < size; count++)
        if (vars->tested[count] == state)
          return IMASK_NOMATCH;
    }

    vars->tested[size++] = (short)state;       /* indicada que o estado foi testado */

    /* se houverem dois ramos, chama a funcao recursivamente,
    retornando com o primeiro que completar a maquina */

    if (vars->fsm[state].next1 != vars->fsm[state].next2)
    {
      long a;

      a = iMaskMatchRecursive (vars, j, vars->fsm[state].next2, capture, size);

      if (a != IMASK_NOMATCH)         /* se deu match */
        return a;

      a = iMaskMatchRecursive (vars, j, vars->fsm[state].next1, capture, size);

      return a;
    }
    break;

  case IMASK_CAP_OPEN_CMD:
    {
      long a;
      ImaskCapt new_cap;

      new_cap.next_one = capture;
      new_cap.type = IMASK_CAPT_OPEN;
      new_cap.pos = j;
      new_cap.which_one = vars->fsm[state].ch;

      a = iMaskMatchRecursive (vars, j, vars->fsm[state].next1, &new_cap, size);

      return a;
    }
    break;

  case IMASK_CAP_CLOSE_CMD:
    {
      long a;
      ImaskCapt new_cap;

      new_cap.next_one = capture;
      new_cap.type = IMASK_CAPT_CLOSE;
      new_cap.pos = j - 1;
      new_cap.which_one = vars->fsm[state].ch;

      a = iMaskMatchRecursive (vars, j, vars->fsm[state].next1, &new_cap, size);

      return a;
    }

  case IMASK_CLASS_CMD:
    {
      int temp, found = 0, negate;

      temp = vars->fsm[state].next1;
      negate = vars->fsm[state].next2;
      state++;

      while (vars->fsm[state].command != IMASK_NULL_CMD)
      {
        if (vars->fsm[state].command == IMASK_CLASS_CMD_RANGE)
        {
          if ((vars->text[j] >= vars->fsm[state].ch) &&
            (vars->text[j] <= vars->fsm[state].next1))
          {
            found = 1;
            break;
          }
        }

        else if ((vars->fsm[state].command == IMASK_CLASS_CMD_CHAR) &&
          (vars->text[j] == vars->fsm[state].ch))
        {
          found = 1;
          break;
        };
        state++;
      }

      if (found ^ negate)
      {
        if (vars->text[j] == '\0')
          return IMASK_NOMATCH;
        j++;
        vars->tested = &vars->tested[size + 1];
        size = 0;
        return iMaskMatchRecursive (vars, j, temp, capture, size);
      }
      else
        return IMASK_NOMATCH;
    }

  case IMASK_CHAR_CMD:
    if (vars->text[j] != vars->fsm[state].ch)
      return IMASK_NOMATCH;
    j++;
    vars->tested = &vars->tested[size + 1];
    size = 0;
    break;

  case IMASK_ANY_CMD:
    if ((vars->text[j] == '\0') || (vars->text[j] == '\n'))
      return IMASK_NOMATCH;
    j++;
    vars->tested = &vars->tested[size + 1];
    size = 0;
    break;

  case IMASK_SPC_CMD:
    {
      long a;

      a = (*imask_match_functions[(int) vars->fsm[state].ch].function) (vars->text, j);

      switch (a)
      {
      case IMASK_NO_MATCH:
        return IMASK_NOMATCH;

      case IMASK_NORMAL_MATCH:
        j++;
        vars->tested = &vars->tested[size + 1];
        size = 0;
        break;

      case IMASK_NO_CHAR_MATCH:;  /* does nothing */
      }
    }
    break;

  case IMASK_BEGIN_CMD:
    if (!((vars->text[j - 1] == '\n') || (j == 0)))
      return IMASK_NOMATCH;

    break;

  case IMASK_END_CMD:
    if (!((vars->text[j] == '\n') || (vars->text[j] == '\0')))
      return IMASK_NOMATCH;

    break;
  }

  return iMaskMatchRecursive (vars, j, vars->fsm[state].next1, capture, size);
}

static int iMaskInStack (ImaskStack * stack, int state)
{
  int a;
  for (a = 0; a < stack->size; a++)
    if (stack->stack[a] == state)
      return 1;

  return 0;
}

static void iMaskNewStack (ImaskStack * new_stack, short *stack)
{
  new_stack->size = 0;
  new_stack->stack = stack;
}

static void iMaskPushStack (ImaskStack * stack, int value)
{
  stack->stack[stack->size++] = (short)value;
}

static void iMaskMoveStack (ImaskStack * dest, ImaskStack * source)
{
  short *temp = dest->stack;
  dest->stack = source->stack;
  source->stack = temp;

  dest->size = source->size;
  source->size = 0;
}

/* non recursive */
static long iMaskMatchLocal (const char *text, ImaskParsed * fsm, long start, char *addchar, int casei)
{
  int finished = IMASK_NOMATCH;
  ImaskStack now, next;
  short a1[IMASK_MIN_STACK_ELEMENTS];
  short a2[IMASK_MIN_STACK_ELEMENTS];
  int state;
  int j = 0;
  int pos;

  if (addchar) addchar[0] = 0;   

  j = start;

  iMaskNewStack(&now, a1);
  iMaskNewStack(&next, a2);

  iMaskPushStack (&now, fsm[0].next1);

  for (;;)
  {
    for (pos = 0; pos < now.size; pos++)
    {
      state = now.stack[pos];

      if (state == 0)
      {
        finished = j - start;
        continue;
      }

      if (fsm[state].command == IMASK_NULL_CMD)
      {
        if(!iMaskInStack (&now, fsm[state].next2))
          iMaskPushStack (&now, fsm[state].next2);

        if(fsm[state].next1 != fsm[state].next2)
        {
          if(!iMaskInStack (&now, fsm[state].next1))
            iMaskPushStack (&now, fsm[state].next1);
        }
      }
      else if (text[j] == '\0');  /* ignore \0 */
      else if (((fsm[state].command == IMASK_CHAR_CMD) &&
        ((!casei && fsm[state].ch == text[j]) ||
        (casei && tolower(fsm[state].ch) == tolower(text[j]))
        )
        ) ||
        ((fsm[state].command == IMASK_ANY_CMD) &&
        (text[j] != '\n')
        )
        )
        iMaskPushStack (&next, fsm[state].next1);
      else if (fsm[state].command == IMASK_SPC_CMD)
      {
        int ret;

        ret = (*(imask_match_functions[(int) fsm[state].ch].function))(text, j);
        switch (ret)
        {
        case IMASK_NO_MATCH:
          break;

        case IMASK_NORMAL_MATCH:
          iMaskPushStack (&next, fsm[state].next1);
          break;

        case IMASK_NO_CHAR_MATCH:
          iMaskPushStack (&now, fsm[state].next1);
          break;
        }
      }
      else if (fsm[state].command == IMASK_CLASS_CMD)
      {
        int temp, found = 0, negate;

        temp = fsm[state].next1;
        negate = fsm[state].next2;
        state++;

        while (fsm[state].command != IMASK_NULL_CMD)
        {
          if (fsm[state].command == IMASK_CLASS_CMD_RANGE)
          {
            if((!casei && (text[j]>=fsm[state].ch) &&
              (text[j]<=fsm[state].next1)
              ) ||
              (casei && (tolower(text[j])>=tolower(fsm[state].ch)) &&
              (tolower(text[j])<=tolower(fsm[state].next1))
              )
              )
            {
              found = 1;
              break;
            }
          }
          else if ((fsm[state].command == IMASK_CLASS_CMD_CHAR) &&
            ((!casei && text[j] == fsm[state].ch) ||
            (casei && tolower(text[j]) == tolower(fsm[state].ch))
            )
            )
          {
            found = 1;
            break;
          }
          state++;
        }

        if(found ^ negate)
        {
          iMaskPushStack (&next, temp);
          /* state = temp; -- unused */
        }
      }
      else if (fsm[state].command == IMASK_BEGIN_CMD)
      {
        if (text[j - 1] == '\n' || j == 0)
          iMaskPushStack (&now, fsm[state].next1);
      }
      else if (fsm[state].command == IMASK_END_CMD)
      {
        if (text[j] == '\n' || text[j] == '\0')
          iMaskPushStack (&now, fsm[state].next1);
      }
    }

    if (text[j] == '\0')
    {
      if(next.size == 0 && finished == j)
      {
        return finished;
      }
      else if(addchar)
      {
        for (pos = 0; pos < now.size; pos++)
        {
          state = now.stack[pos];
          if (fsm[state].command == IMASK_CHAR_CMD)
          {
            iMaskPushStack (&next, state);
          }
          else if (fsm[state].command != IMASK_NULL_CMD)
          {
            next.size = 0;
            break;
          }
          else
          {
            if (!iMaskInStack (&now, fsm[state].next2))
              iMaskPushStack (&now, fsm[state].next2);

            if (fsm[state].next1 != fsm[state].next2)
            {
              if (!iMaskInStack (&now, fsm[state].next1))
                iMaskPushStack (&now, fsm[state].next1);
            }
          }           
        }

        iMaskMoveStack (&now, &next);

        if (now.size == 1)
        {
          int inx=0;
          state = now.stack[0];
          while(fsm[state].next1 == fsm[state].next2)
          {
            if(fsm[state].command == IMASK_CHAR_CMD)
              addchar[inx++] = fsm[state].ch;
            else if(fsm[state].command != IMASK_NULL_CMD)
              break;

            state = fsm[state].next1;
          }
          addchar[inx]=0;
        }
      }
      return IMASK_PARTIALMATCH;
    }

    j++;

    if (next.size == 0)
    {
      if (finished > IMASK_NOMATCH)
        return finished;

      return IMASK_NOMATCH;
    }

    iMaskMoveStack (&now, &next);
  }
}

int iupMaskMatch (const char *text, ImaskParsed * fsm, long start, iMaskMatchFunc function, void *user, char *addchar, int icase)
{
  long ret;
  short tested[10000];  /* to be eliminated */
  ImaskMatchVars vars;

  /* use recursive only for standard capture */

  if (fsm[0].ch == IMASK_NOCAPTURE)
    return iMaskMatchLocal (text, fsm, start, addchar, icase);

  vars.text = text;
  vars.fsm = fsm;
  vars.tested = tested;
  vars.function = function;
  vars.user = user;

  ret = iMaskMatchRecursive (&vars, start, fsm[0].next1, NULL, 0);

  return (int)((ret >= start) ? ret - start : ret);
}
