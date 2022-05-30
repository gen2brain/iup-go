/** \file
 * \brief imask parser
 *
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <setjmp.h>

#include "iup_maskparse.h"
#include "iup_maskmatch.h"

/*
 * Table of characters (customizaveis atraves de iupMaskSetChar)
 */

static char* imask_parse_chars = "|*+()[]-^/.?^${}~";
/*                                01234567890123456     */

#define OR_CH        imask_parse_chars[0]  /* OR CHaracter      */
#define CL_CH        imask_parse_chars[1]  /* CLosure CHaracter    */
#define OOM_CH       imask_parse_chars[2]  /* One Or More CHaracter  */
#define OPGR_CH      imask_parse_chars[3]  /* OPen  GRoup CHaracter  */
#define CLGR_CH      imask_parse_chars[4]  /* CLose GRoup CHaracter  */
#define OPCL_CH      imask_parse_chars[5]  /* OPen CLass CHaracter    */
#define CLCL_CH      imask_parse_chars[6]  /* CLose CLass CHaracter  */
#define SEPCL_CH     imask_parse_chars[7]  /* SEParate CLass CHaracter  */
#define NEGCL_CH     imask_parse_chars[8]  /* NEGation CLass CHaracter  */
#define SPC_CH       imask_parse_chars[9]  /* SPeCial function CHaracter  */
#define ANY_CH       imask_parse_chars[10]  /* ANY CHaracter    */
#define ONE_CH       imask_parse_chars[11]  /* ONE or no CHaracter    */
#define BEGIN_CH     imask_parse_chars[12]  /* BEGINning of a line CHaracter*/
#define END_CH       imask_parse_chars[13]  /* END of a line CHaracter  */
#define CAP_OPEN_CH  imask_parse_chars[14]  /* CAPture OPEN CHaracter  */
#define CAP_CLOSE_CH imask_parse_chars[15]  /* CAPture CLOSE CHaracter  */
#define NEG_CH       imask_parse_chars[16]  /* NEGation CHaracter    */

#define SPC2_CH  '\\'    /* SPeCial 2 CHaracter */

#define isvalid(c) (c != 0 && c != OR_CH && c != OPGR_CH && c != CLGR_CH &&\
                         c != CL_CH && c != OPCL_CH && c != CLCL_CH &&\
                         c != CAP_OPEN_CH && c != CAP_CLOSE_CH && c != OOM_CH)

#define STATE_BLOCK        30

typedef struct _ImaskParseVars
{
  const char *string;
  int state, j, num_states;
  ImaskParsed *fsm;
  short capture[30];
  short size;
  char nextcap;
  jmp_buf env;
} ImaskParseVars;

static int iMaskParseExpression (ImaskParseVars * vars);
static int iMaskParseTerm (ImaskParseVars * vars);
static int iMaskParseFactor (ImaskParseVars * vars);
static void iMaskParseError (ImaskParseVars *vars);
static void iMaskParseNewState (ImaskParseVars * vars);
static void iMaskParseSetState (ImaskParseVars * vars, int state, char ch, char command, int next1, int next2);

int iupMaskSetChar (int char_number, char new_char)
{
  if ((char_number < 0) || (char_number > (int)strlen(imask_parse_chars)))
    return 0;

  imask_parse_chars[char_number] = new_char;

  return 1;
}

    /*
     * Funcao de interface, recebe padrao e retorna array contendo as finite
     * state machines (fsm) construidas a partir do padrao
     */

int iupMaskParse(const char *text, ImaskParsed ** fsm)
{
  int t;
  ImaskParseVars vars;

  /* inicializacao das variaveis */

  vars.state = 1;
  vars.j = 0;
  vars.num_states = 0;
  vars.size = 0;
  vars.nextcap = 0;
  vars.string = text;

  if ((vars.fsm = (ImaskParsed *) malloc (STATE_BLOCK * sizeof (ImaskParsed))) == NULL)
    return IMASK_MEM_ERROR;

  memset(vars.fsm, 0, STATE_BLOCK*sizeof (ImaskParsed));
  vars.num_states = STATE_BLOCK;

  /* a principio, nao ha captura. Se ocorrer uma, ele e setado
     para IMASK_CAPTURE */

  vars.fsm[0].ch = IMASK_NOCAPTURE;

  if (setjmp (vars.env) == 0)
    t = iMaskParseExpression (&vars);

  else
  {
    free (vars.fsm);
    return IMASK_PARSE_ERROR;
  }

  /* seta os estados inicial e final, guardando no inicial
     (fsm[0].next1) o tamanho da maquina */

  iMaskParseSetState (&vars, 0, vars.fsm[0].ch, IMASK_NULL_CMD, t, vars.state + 1);
  iMaskParseSetState (&vars, vars.state, 0, IMASK_NULL_CMD, 0, 0);

  *fsm = vars.fsm;

  return IMASK_PARSE_OK;
}

static int iMaskParseExpression (ImaskParseVars * vars)
{
  int r, t1;
  int last_state = vars->state - 1;

  t1 = iMaskParseTerm (vars);
  r = t1;

  if (vars->string[vars->j] == OR_CH)
  {
    int t2 = vars->state;
    int t3;

    r = t2;

    vars->j++;
    iMaskParseNewState (vars);

    t3 = iMaskParseExpression (vars);  /* pega o 2o ramo do OR */

    /* faz o primeiro state antes do OR apontar para o state de entrada
     * do OR */

    if (vars->fsm[last_state].next1 == t1)
      vars->fsm[last_state].next1 = t2;

    if (vars->fsm[last_state].next2 == t1)
      vars->fsm[last_state].next2 = t2;

    /* faz o ultimo state do primeiro ramo do OR apontar para o
     * state de saida do OR */

    if (vars->fsm[t2 - 1].next1 == t2)
      vars->fsm[t2 - 1].next1 = vars->state;

    if (vars->fsm[t2 - 1].next2 == t2)
      vars->fsm[t2 - 1].next2 = vars->state;

    iMaskParseSetState (vars, t2, 0, IMASK_NULL_CMD, t1, t3);
    iMaskParseSetState (vars, vars->state, 0, IMASK_NULL_CMD, vars->state + 1,
        vars->state + 1);

    iMaskParseNewState (vars);
  }
  return r;
}

static int iMaskParseTerm (ImaskParseVars * vars)
{
  int r;

  r = iMaskParseFactor (vars);

  if ((vars->string[vars->j] == OPGR_CH) ||
      (isvalid (vars->string[vars->j])) ||
      (vars->string[vars->j] == OPCL_CH) ||
      (vars->string[vars->j] == CAP_OPEN_CH) ||
      (vars->string[vars->j] == NEG_CH))
    iMaskParseTerm (vars);

  if (!((vars->string[vars->j] == OR_CH) ||
        (vars->string[vars->j] == CLGR_CH) ||
        (vars->string[vars->j] == '\0') ||
        (vars->string[vars->j] == CAP_CLOSE_CH)))
    iMaskParseError (vars);

  return r;
}

static int iMaskParseFactor (ImaskParseVars * vars)
{
  int r, t1, t2 = 0;

  t1 = vars->state;

  if (vars->string[vars->j] == OPGR_CH)
  {
    vars->j++;
    t2 = iMaskParseExpression (vars);

    if (vars->string[vars->j] == CLGR_CH)
      vars->j++;
    else
      iMaskParseError (vars);
  }

  else if (vars->string[vars->j] == CAP_OPEN_CH)
  {
    vars->fsm[0].ch = IMASK_CAPTURE;
    iMaskParseSetState (vars, vars->state, vars->nextcap, 
                 IMASK_CAP_OPEN_CMD, vars->state + 1, vars->state + 1);
    t2 = vars->state;
    iMaskParseNewState (vars);
    vars->capture[++vars->size] = vars->nextcap++;
    vars->j++;

    iMaskParseExpression (vars);

    if (vars->string[vars->j] == CAP_CLOSE_CH)
    {
      iMaskParseSetState (vars, vars->state, (char)vars->capture[vars->size--],
                   IMASK_CAP_CLOSE_CMD, vars->state + 1, vars->state + 1);

      iMaskParseNewState (vars);
      vars->j++;
    }
    else
      iMaskParseError (vars);

  }

  else if (vars->string[vars->j] == ANY_CH)
  {
    iMaskParseSetState (vars, vars->state, 1, IMASK_ANY_CMD, vars->state + 1, vars->state + 1);
    t2 = vars->state;
    vars->j++;
    iMaskParseNewState (vars);
  }

  else if (vars->string[vars->j] == NEG_CH)
  {
    int t6;
    t2 = vars->state;
    vars->j++;
    iMaskParseNewState (vars);
    t6 = iMaskParseFactor (vars);
    iMaskParseSetState (vars, t2, 1, IMASK_NEG_OPEN_CMD, t6, vars->state);
    iMaskParseSetState (vars, vars->state, 1, IMASK_NEG_CLOSE_CMD, vars->state + 1, vars->state + 1);
    iMaskParseNewState (vars);
  }

  else if (vars->string[vars->j] == BEGIN_CH)
  {
    iMaskParseSetState (vars, vars->state, 1, IMASK_BEGIN_CMD, vars->state + 1,
      vars->state + 1);
    t2 = vars->state;
    vars->j++;
    iMaskParseNewState (vars);
  }
  else if (vars->string[vars->j] == END_CH)
  {
    iMaskParseSetState (vars, vars->state, 1, IMASK_END_CMD, vars->state + 1,
      vars->state + 1);
    t2 = vars->state;
    vars->j++;
    iMaskParseNewState (vars);
  }

  else if (isvalid (vars->string[vars->j]) && (vars->string[vars->j]
  != SPC_CH) && (vars->string[vars->j] != ANY_CH))
  {
    iMaskParseSetState (vars, vars->state, vars->string[vars->j], 
                 IMASK_CHAR_CMD, vars->state + 1, vars->state + 1);
    t2 = vars->state;
    vars->j++;
    iMaskParseNewState (vars);
  }

  else if (vars->string[vars->j] == OPCL_CH)
  {
    vars->j++;
    iMaskParseSetState (vars, vars->state, 0, IMASK_CLASS_CMD, 0, 0);

    if (vars->string[vars->j] == NEGCL_CH)
    {
      vars->fsm[vars->state].next2 = 1;
      vars->j++;
    }

    t2 = vars->state;
    iMaskParseNewState (vars);

    if (vars->string[vars->j] == SEPCL_CH)
      iMaskParseError (vars);

    while ((vars->string[vars->j] != CLCL_CH) && (vars->string[vars->j] != '\n')
      && (vars->string[vars->j] != '\0'))
    {
      if (vars->string[vars->j] == SPC_CH)
      {
        char temp;

        vars->j++;
        switch (vars->string[vars->j])
        {
        case 'n':
          temp = '\n';
          break;

        case 't':
          temp = '\t';
          break;

        case 'e':
          temp = 27;
          break;

        default:
          temp = vars->string[vars->j];
        }
        iMaskParseSetState (vars, vars->state, temp, IMASK_CLASS_CMD_CHAR, 0, 0);
        vars->j++;
        iMaskParseNewState (vars);
      }
      else if (vars->string[vars->j] == SEPCL_CH)
      {
        char temp = 0;

        vars->j++;

        if (vars->string[vars->j] == SPC_CH)
        {
          vars->j++;
          switch (vars->string[vars->j])
          {
          case 'n':
            temp = '\n';
            break;

          case 't':
            temp = '\t';
            break;

          case 'e':
            temp = 27;
            break;

          default:
            temp = vars->string[vars->j];
          }
        }
        else if (vars->string[vars->j] != CLCL_CH)
          temp = vars->string[vars->j];

        else
          iMaskParseError (vars);

        iMaskParseSetState (vars, vars->state - 1, vars->fsm[vars->state - 1].ch,
                     IMASK_CLASS_CMD_RANGE, temp, 0);
        vars->j++;
      }
      else if (vars->string[vars->j] == BEGIN_CH)
      {
        iMaskParseSetState (vars, vars->state, 1, IMASK_BEGIN_CMD, vars->state + 1, vars->state + 1);
        t2 = vars->state;
        vars->j++;
        iMaskParseNewState (vars);
      }
      else if (vars->string[vars->j] == END_CH)
      {
        iMaskParseSetState (vars, vars->state, 1, IMASK_END_CMD, vars->state + 1, vars->state + 1);
        t2 = vars->state;
        vars->j++;
        iMaskParseNewState (vars);
      }

      else
      {
        iMaskParseSetState (vars, vars->state, vars->string[vars->j], IMASK_CLASS_CMD_CHAR, 0, 0);
        vars->j++;
        iMaskParseNewState (vars);
      }

    }
    if (vars->string[vars->j] != CLCL_CH)
      iMaskParseError (vars);
    iMaskParseSetState (vars, vars->state, 0, IMASK_NULL_CMD, vars->state + 1, vars->state + 1);
    vars->fsm[t2].next1 = vars->state;
    vars->j++;
    iMaskParseNewState (vars);

  }

  else if (vars->string[vars->j] == SPC_CH)
  {
    int loop1 = 0;
    ImaskMatchFunc* match_functions = iupMaskMatchGetFuncs();

    vars->j++;

    while (match_functions[loop1].ch != '\0' && 
           match_functions[loop1].ch != vars->string[vars->j])
      loop1++;

    if (match_functions[loop1].ch == '\0')
    {
      int temp;

      switch (vars->string[vars->j])
      {
      case 'n':
        temp = '\n';
        break;

      case 't':
        temp = '\t';
        break;

      case 'e':
        temp = 27;
        break;

      case 'x':
        vars->j++;
        sscanf (&vars->string[vars->j], "%2x", &temp);
        vars->j++;
        break;

      case 'o':
        vars->j++;
        sscanf (&vars->string[vars->j], "%3o", &temp);
        vars->j += 2;
        break;

      default:
        if (isdigit((int)vars->string[vars->j]))
        {
          sscanf (&vars->string[vars->j], "%3d", &temp);
          if (temp > 255)
          {
            iMaskParseError (vars);
          }
          vars->j += 2;
        }
        else
          temp = vars->string[vars->j];
      }
      iMaskParseSetState (vars, vars->state, (char)temp, IMASK_CHAR_CMD, vars->state + 1, vars->state + 1);
    }

    else
    {
      iMaskParseSetState (vars, vars->state, (char)loop1, IMASK_SPC_CMD, vars->state + 1, vars->state + 1);
    }

    t2 = vars->state;
    vars->j++;
    iMaskParseNewState (vars);
  }
  else
    iMaskParseError (vars);

  if (vars->string[vars->j] == CL_CH)
  {
    iMaskParseSetState (vars, vars->state, 0, IMASK_NULL_CMD, vars->state + 1, t2);
    r = vars->state;

    if (vars->fsm[t1 - 1].next1 == t1)
      vars->fsm[t1 - 1].next1 = vars->state;

    if (vars->fsm[t1 - 1].next2 == t1)
      vars->fsm[t1 - 1].next2 = vars->state;

    vars->j++;
    iMaskParseNewState (vars);
  }

  else if (vars->string[vars->j] == ONE_CH)
  {
    iMaskParseSetState (vars, vars->state, 0, IMASK_NULL_CMD, vars->state + 1, t2);
    r = vars->state;

    if (vars->fsm[t1 - 1].next1 == t1)
      vars->fsm[t1 - 1].next1 = vars->state;

    if (vars->fsm[t1 - 1].next2 == t1)
      vars->fsm[t1 - 1].next2 = vars->state;

    if (vars->fsm[vars->state - 1].next1 == vars->state)
      vars->fsm[vars->state - 1].next1 = vars->state + 1;

    if (vars->fsm[vars->state - 1].next2 == vars->state)
      vars->fsm[vars->state - 1].next2 = vars->state + 1;

    vars->j++;
    iMaskParseNewState (vars);

    iMaskParseSetState (vars, vars->state, 0, IMASK_NULL_CMD, vars->state + 1, vars->state + 1);

    iMaskParseNewState (vars);
  }
  else if (vars->string[vars->j] == OOM_CH)
  {
    iMaskParseSetState (vars, vars->state, 0, IMASK_NULL_CMD, vars->state + 1, t2);
    r = t2;

    vars->j++;

    iMaskParseNewState (vars);

  }
  else
    r = t2;

  return r;
}

static void iMaskParseError (ImaskParseVars * vars)
{
  longjmp (vars->env, 1);
}

static void iMaskParseNewState (ImaskParseVars * vars)
{

  if (vars->state >= vars->num_states - 1)
  {
    ImaskParsed *new_fsm = (ImaskParsed*) realloc (vars->fsm, (vars->num_states + STATE_BLOCK) * sizeof (ImaskParsed));
    memset(new_fsm + vars->num_states, 0, STATE_BLOCK*sizeof(ImaskParsed));
    vars->fsm = new_fsm;
    vars->num_states += STATE_BLOCK;
  }

  vars->state++;
}

static void iMaskParseSetState (ImaskParseVars * vars, int state, char ch, char command, int next1, int next2)
{
  vars->fsm[state].ch = ch;
  vars->fsm[state].command = command;
  vars->fsm[state].next1 = next1;
  vars->fsm[state].next2 = next2;
}
