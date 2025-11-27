/** \file
 * \brief IupMatrix Expansion Library.
 *
 * Original implementation from Bruno Kassar.
 * See Copyright Notice in "iup.h"
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <math.h>

#include "iup.h"
#include "iupcbs.h"

#include "iup_array.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_matrixex.h"


#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

/* Source for conversion names, symbols and factors:
   http://en.wikipedia.org/wiki/Conversion_of_units

   This file is in UTF-8 encoding with BOM, so the comments are correctly displayed. 
   Units are written in hex to avoid conversion problems.
*/


#define IMATEX_UNIT_MAXCOUNT 25

typedef struct _ImatExUnit {
  const char* u_name;
  const char* symbol;
  double factor;
  const char* symbol_utf8;
} ImatExUnit;

#define GRAVITY 9.80665

#define IMATEX_LENGTH_COUNT 12
static ImatExUnit IMATEX_LENGTH_UNITS [IMATEX_UNIT_MAXCOUNT] = {
  {"metre"        ,"m" , 1,       NULL},
  {"centimetre"   ,"cm", 0.01,    NULL},
  {"millimetre"   ,"mm", 0.001,   NULL},
  {"kilometre"    ,"km", 1000,    NULL},
  {"nanometre"    ,"nm", 1.0e-9,  NULL},
  {"Angstrom"     ,"\xC5" , 1.0e-10, "\xC3\x85" },  /* Å */ 
  {"micron"       ,"\xB5" , 1.0e-6,  "\xC2\xB5"  }, /* µ */ /* International yard and pound agreement in 1959 */                
  {"inch"         ,"in", 0.0254,   NULL},   /* 0.0254                 (in = 25.4 mm)   */
  {"foot"         ,"ft", 0.3048,   NULL},    /* 0.0254 * 12            (ft = 12 in)     */
  {"yard"         ,"yd", 0.9144,   NULL},     /* 0.0254 * 12 * 3        (yd = 3 ft)      */
  {"mile"         ,"mi", 1609.344, NULL},   /* 0.0254 * 12 * 3 * 1760 (mi = 1760 yd)   */
  {"nautical mile","NM", 1853.184, NULL}};  /* 6080 ft */

#define IMATEX_TIME_COUNT 7                
static ImatExUnit IMATEX_TIME_UNITS [IMATEX_UNIT_MAXCOUNT] = {
  {"second","s"  , 1, NULL},
  {"minute","min", 60, NULL},
  {"hour"  ,"h"  , 3600, NULL},     /* 60 * 60 */
  {"day"   ,"d"  , 86400, NULL},    /* 60 * 60 * 24 */
  {"week"  ,"wk" , 604800, NULL},  /* 60 * 60 * 24 * 7 */
  {"millisecond","ms", 0.001, NULL},                                       
  {"microsecond","\xB5""s", 1.0e-6, "\xC2\xB5s"}};  /* µs */    

#define IMATEX_MASS_COUNT 5               
static ImatExUnit IMATEX_MASS_UNITS [IMATEX_UNIT_MAXCOUNT] = {
  {"kilogram"   ,"kg" , 1, NULL},
  {"gram"       ,"g"  , 0.001          , NULL},
  {"tonne"      ,"t"  , 1000           , NULL},  /*  metric ton */
  {"pound"      ,"lb" , 0.45359237     , NULL},  /* International yard and pound agreement in 1959 */
  {"ounce"      ,"oz" , 0.45359237/16.0, NULL}}; /* (international avoirdupois) */

#define IMATEX_CELSIUS 1
#define IMATEX_FAHRENHEIT 2
#define IMATEX_TEMPERATURE_COUNT 4                       
static ImatExUnit IMATEX_TEMPERATURE_UNITS [IMATEX_UNIT_MAXCOUNT] = {
  {"Kelvin"           ,"K"  , 1, NULL},
  {"degree Celsius"   ,"\xB0""C" , 1,     "\xC2\xBA""C"},   /* °C" */
  {"degree Fahrenheit","\xB0""F" , 5./9., "\xC2\xBA""F"},   /* °F" */
  {"degree Rankine"   ,"\xB0""Ra", 5./9., "\xC2\xBA""Ra"}}; /* °Ra */

#define IMATEX_AREA_COUNT 13
static ImatExUnit IMATEX_AREA_UNITS [IMATEX_UNIT_MAXCOUNT] = {
  {"square metre"     ,"m\xB2"   , 1,       "m\xC2\xB2"},                  /* m²  */
  {"square centimetre","cm\xB2"  , 1.0e-4, "cm\xC2\xB2"},   /* 0.01^2 */   /* cm² */
  {"square millimetre","mm\xB2"  , 1.0e-6, "mm\xC2\xB2"},   /* 0.001^2 */  /* mm² */
  {"square kilometre" ,"km\xB2"  , 1.0e6,  "km\xC2\xB2"},   /* 1000^2 */   /* km² */
  {"square nanometre", "nm\xB2",   1.0e-18, "nm\xC2\xB2" },                /* nm² */
  {"square Angstrom",  "\xC5""\xB2",    1.0e-20, "\xC3\x85\xC2\xB2" },     /* Å²  */
  {"square micron",    "\xB5""\xB2",    1.0e-12, "\xC2\xB5\xC2\xB2" },     /* µ²  */
  {"square inch"      ,"sq in", 6.4516e-4, NULL},       /* 0.0254^2 */
  {"square foot"      ,"sq ft", 9.290304e-2, NULL},      /* 0.3048^2 */   
  {"square yard"      ,"sq yd", 0.83612736, NULL},       /* 0.9144^2 */
  {"square mile"      ,"sq mi", 2.589988110336e6, NULL}, /* 1609.344^2 */
  {"acre"             ,"ac"   , 4046.8564224, NULL},     /* 0.9144^2 * 4840 (4840 sq yd) */
  {"hectare"          ,"ha"   , 1.0e4,      NULL}};

#define IMATEX_VOLUME_COUNT 11
static ImatExUnit IMATEX_VOLUME_UNITS [IMATEX_UNIT_MAXCOUNT] = {
  {"cubic metre"     ,"m\xB3"   , 1,       "m\xC2\xB3"},  /* m³  */
  {"cubic centimetre","cm\xB3"  , 1.0e-6, "cm\xC2\xB3"},  /* cm³ */  /* 0.01^3 */
  {"cubic millimetre","mm\xB3"  , 1.0e-9, "mm\xC2\xB3"},  /* mm³ */  /* 0.001^3 */
  {"cubic kilometre" ,"km\xB3"  , 1.0e9,  "km\xC2\xB3"},  /* km³ */  /* 1000^3 */
  {"cubic inch"      ,"cu in", 16.387064e-6, NULL},          /* 0.0254^3 */
  {"cubic foot"      ,"cu ft", 0.028316846592, NULL},        /* 0.3048^3 */   
  {"cubic yard"      ,"cu yd", 0.764554857984, NULL},        /* 0.9144^3 */
  {"cubic mile"      ,"cu mi", 4168181825.440579584, NULL},  /* 1609.344^3 */
  {"litre"           ,"L"    , 0.001           , NULL},
  {"gallon"          ,"gal"  , 3.785411784e-3, NULL},     /* (US fluid; Wine = 231 cu in) */
  {"barrel"          ,"bl"   , 0.158987294928, NULL}};    /* (petroleum = 42 gal) */

#define IMATEX_SPEED_COUNT 7
static ImatExUnit IMATEX_SPEED_UNITS [IMATEX_UNIT_MAXCOUNT] = {
  {"metre per second"     ,"m/s" , 1, NULL},
  {"inch per second"      ,"in/s" , 0.0254    , NULL},
  {"foot per second"      ,"ft/s" , 0.3048    , NULL},
  {"kilometre per hour"   ,"km/h", 1.0/3.6   , NULL},
  {"centimetre per second","cm/s", 0.01      , NULL},
  {"mile per hour"        ,"mph" , 0.44704   , NULL},    /* 1609.344 / 3600 */
  {"knot"                 ,"kn"  , 1.852/3.6 , NULL}};   /* kn = 1 NM/h = 1.852 km/h */

#define IMATEX_ANGULAR_SPEED_COUNT 6
static ImatExUnit IMATEX_ANGULAR_SPEED_UNITS [IMATEX_UNIT_MAXCOUNT] = {
  {"radian per second"    ,"rad/s"  , 1, NULL},              
  {"radian per minute"    ,"rad/min", 60        , NULL},
  {"degree per second"    ,"deg/s"  , M_PI/180.0, NULL},
  {"degree per minute"    ,"deg/min", M_PI/3.0  , NULL},     /* M_PI/180.0 * 60 */
  {"Hertz"                ,"Hz"     , 2.0*M_PI  , NULL},     /* angular speed = 2*PI * angular frequency */
  {"revolution per minute","rpm"    , 120.0*M_PI, NULL}};

#define IMATEX_ACCELERATION_COUNT 5
static ImatExUnit IMATEX_ACCELERATION_UNITS [IMATEX_UNIT_MAXCOUNT] = {
  {"metre per second squared","m/s\xB2", 1,         "m/s\xC2\xB2"},    /* m/s² */
  {"inch per second squared" ,"in/s\xB2", 0.0254,  "in/s\xC2\xB2"},    /* in/s² */
  {"knot per second"         ,"kn/s", 1.852/3.6 , NULL},               
  {"mile per second squared" ,"mi/s\xB2", 1609.344, "mi/s\xC2\xB2"},   /* mi/s² */
  {"standard gravity"        ,"g"   , GRAVITY  , NULL}};

#define IMATEX_FORCE_COUNT 7
static ImatExUnit IMATEX_FORCE_UNITS [IMATEX_UNIT_MAXCOUNT] = {
  {"Newton"        ,"N"  , 1, NULL},              /* N = kg·m/s^2 */
  {"Kilonewton"    ,"kN" , 1000     , NULL},
  {"dyne"          ,"dyn", 1.0e-5   , NULL},      /* g·cm/s^2 */
  {"kilogram-force","kgf", GRAVITY  , NULL},
  {"pound-force"   ,"lbf", GRAVITY * 0.45359237, NULL},          /* lbf = g × lb */
  {"kip-force"     ,"kip", GRAVITY * 0.45359237 * 1000, NULL},   /* kip = g × 1000 lb */
  {"ton-force"     ,"tnf", GRAVITY * 0.45359237 * 2000, NULL}};  /* tnf = g × 2000 lb */

#define IMATEX_PRESSURE_COUNT 8
static ImatExUnit IMATEX_PRESSURE_UNITS [IMATEX_UNIT_MAXCOUNT] = {
  {"Pascal"               ,"Pa"  , 1, NULL},      /* Pa = N/(m^2) = kg/(m·s^2) */
  {"kilopascal"           ,"kPa" , 1000    , NULL},
  {"atmosphere"           ,"atm" , 101325  , NULL},   /* (standard) */
  {"millimetre of mercury","mmHg", 133.322387415, NULL},    /* 13595.1 kg/m^3 × mm × g = 13595.1 * 0.001 * GRAVITY */
  {"bar"                  ,"bar" , 1.0e5   , NULL},                                                       
  {"torr"                 ,"torr", 101325.0/760.0, NULL},   /* aprox 133.3224 */
  {"pound per square inch","psi" , 4.4482216152605/6.4516e-4, NULL},  /* psi = lbf/(in^2) = GRAVITY*0.45359237/0,0254^2 */
  {"kip per square inch"  ,"ksi" , 4.4482216152605/6.4516e-1, NULL}}; /* ksi = kip/in^2 */          

#define IMATEX_FORCE_PER_LENGTH_COUNT 4            /* same as (Linear Weight) */
static ImatExUnit IMATEX_FORCE_PER_LENGTH_UNITS [IMATEX_UNIT_MAXCOUNT] = {
  {"Newton per metre"         ,"N/m"  , 1,            NULL},      
  {"Kilonewton per metre"     ,"kN/m" , 1000    ,     NULL},
  {"kilogram-force per metre" ,"kgf/m", GRAVITY  ,    NULL},
  {"ton-force per metre"      ,"tnf/m", GRAVITY*1000, NULL}};

#define IMATEX_MOMENT_COUNT 8           /* Torque */
static ImatExUnit IMATEX_MOMENT_UNITS [IMATEX_UNIT_MAXCOUNT] = {
  {"Newton metre"             , "N\xB7""m"   , 1,                "N\xC2\xB7m"},       /* N·m    */
  {"kilogram-force metre"     , "kgf\xB7""m" , GRAVITY,          "kgf\xC2\xB7m"},     /* kgf·m  */
  {"ton-force metre"          , "tnf\xB7""m" , GRAVITY*1000,     "tnf\xC2\xB7m"},     /* tnf·m  */
  {"Newton centimetre"        , "N\xB7""cm"  , 100,              "N\xC2\xB7m"},       /* N·cm   */ 
  {"kilogram-force centimetre", "kgf\xB7""cm", GRAVITY*100,      "kgf\xC2\xB7""cm"},  /* kgf·cm */
  {"ton-force centimetre"     , "tnf\xB7""cm", GRAVITY*1000*100, "tnf\xC2\xB7""cm"},  /* tnf·cm */
  {"Kilonewton-metre"         , "kN\xB7""m"  , 1000,             "kN\xC2\xB7m"},      /* kN·m   */
  {"metre kilogram"           , "m\xB7""kg"  , 1.0/GRAVITY,      "m\xC2\xB7kg"}};     /* m·kg   */ 

#define IMATEX_ANGLE_COUNT 3
static ImatExUnit IMATEX_ANGLE_UNITS [IMATEX_UNIT_MAXCOUNT] = {
  {"radian", "rad", 1,            NULL},
  {"degree", "\xB0", M_PI/180.0,     "\xC2\xBA"}, /* ° */
  {"gradian", "grad", M_PI/200.0, NULL}};

#define IMATEX_SPECIFIC_MASS_COUNT 7
static ImatExUnit IMATEX_SPECIFIC_MASS_UNITS [IMATEX_UNIT_MAXCOUNT] = {
  {"kilogram per cubic metre" ,"kg/m\xB3"  , 1,                         "kg/m\xC2\xB3"},  /* kg/m³ */
  {"gram per cubic centimetre","g/cm\xB3"  , 1000,                      "g/cm\xC2\xB3"},  /* g/cm³ */
  {"gram per millilitre"      ,"g/mL"   , 1000,                      NULL},               
  {"kilogram per litre"       ,"kg/L"   , 1000,                      NULL},               
  {"pound per cubic foot"     ,"lb/ft\xB3" , 0.45359237/16.387064e-6,   "lb/ft\xC2\xB3"}, /* lb/ft³ */
  {"pound per cubic inch"     ,"lb/in\xB3" , 0.45359237/0.028316846592, "lb/in\xC2\xB3"}, /* lb/in³ */
  {"pound per gallon"         ,"lb/gal" , 0.45359237/3.785411784e-3, NULL}};
                                                     
#define IMATEX_SPECIFIC_WEIGHT_COUNT 6
static ImatExUnit IMATEX_SPECIFIC_WEIGHT_UNITS [IMATEX_UNIT_MAXCOUNT] = {
  {"Newton per cubic metre"        ,"N/m\xB3"  , 1,                  "N/m\xC2\xB3"},                          /* N/m³    */  
  {"Kilonewton per cubic metre"    ,"kN/m\xB3" , 1000,               "kN/m\xC2\xB3"},                         /* kN/m³   */ 
  {"kilogram-force per cubic metre","kgf/m\xB3", GRAVITY,            "kgf/m\xC2\xB3"},                        /* kgf/m³  */
  {"ton-force per cubic metre"     ,"tnf/m\xB3", GRAVITY * 1000,     "tnf/m\xC2\xB3"},                        /* tnf/m³  */
  {"pound-force per cubic foot"    ,"lbf/ft\xB3", (GRAVITY * 0.45359237) / 0.028316846592, "lbf/m\xC2\xB3"},  /* lbf/ft³ */
  {"kilogram-force per litre"      ,"kgf/L" , GRAVITY * 0.001,                          NULL}};
                                                              
#define IMATEX_ENERGY_COUNT   7
static ImatExUnit IMATEX_ENERGY_UNITS[IMATEX_UNIT_MAXCOUNT] = {
  {"Joule"          ,"J"   , 1, NULL},                  /* J = m * N */
  {"Kilojoule"      ,"kJ"  , 1000                  , NULL},
  {"calorie"        ,"cal" , 4.1868                , NULL},  /* (International Table) */
  {"kilocalorie"    ,"kcal", 4.1868e3              , NULL},
  {"BTU"            ,"BTU" , 1.05505585262e3       , NULL},  /* (International Table) */
  {"Kilowatt-hour"  ,"kW\xB7""h", 3.6e6                 , "kW\xC2\xB7h"},  /* kW·h */
  {"horsepower-hour","hp\xB7""h", 2.684519537696172792e6, "hp\xC2\xB7h"}}; /* hp·h */  /* hp * 3600 */
                                                               
#define IMATEX_POWER_COUNT    4
static ImatExUnit IMATEX_POWER_UNITS[IMATEX_UNIT_MAXCOUNT] = {
  {"Watt"              ,"W"    , 1, NULL},               /* W = J/s */
  {"Kilowatt"          ,"kW"   , 1000, NULL},
  {"calorie per second","cal/s", 4.1868, NULL},  /* (International Table) */
  {"horsepower"        ,"hp"   , 745.69987158227022 , NULL}};  /* hp = 550 ft lbf/s (imperial mechanical)   */
                                                         /*      550 * 0.3048 *  9.80665 * 0.45359237 */   
#define IMATEX_FRACTION_COUNT 4
static ImatExUnit IMATEX_FRACTION_UNITS[IMATEX_UNIT_MAXCOUNT] = {
  {"percentage"  , "%"    , 1,    NULL},
  {"per one"     , "/1"   , 100 , NULL},
  {"per ten"     , "/10"  , 10  , NULL},
  {"per thousand", "/1000", 0.1 , NULL}};

#define IMATEX_KINEMATIC_VISCOSITY_COUNT 3
static ImatExUnit IMATEX_KINEMATIC_VISCOSITY_UNITS [IMATEX_UNIT_MAXCOUNT] = {
  {"square metre per second", "m\xB2""/s"  , 1,           "m\xC2\xB2/s"},   /* m²/s  */
  {"square foot per second" , "ft\xB2""/s" , 9.290304e-2, "ft\xC2\xB2/s"},  /* ft²/s */   /* 0.3048^2 */
  {"stokes"                 , "St"    , 1.0e-4,      NULL}};

#define IMATEX_DYNAMIC_VISCOSITY_COUNT 4
static ImatExUnit IMATEX_DYNAMIC_VISCOSITY_UNITS [IMATEX_UNIT_MAXCOUNT] = {
  {"Pascal second"        , "Pa\xB7""s"      , 1,                        "Pa\xC2\xB7s"},        /* Pa·s */
  {"poise"                , "P"         , 0.1,                      NULL},                     
  {"pound per foot hour"  , "lb/(ft\xB7""h)" , 0.45359237/(0.3048*3600), "lb/(ft\xC2\xB7h)"},   /* lb/(ft·h) */
  {"pound per foot second", "lb/(ft\xB7""s)" , 0.45359237/0.3048,        "lb/(ft\xC2\xB7s)"}};  /* lb/(ft·s) */
                                                            
#define IMATEX_FLOW_COUNT 3
static ImatExUnit IMATEX_FLOW_UNITS [IMATEX_UNIT_MAXCOUNT] = {
  {"cubic metre per second", "m\xB3""/s",  0.1,            "m\xC2\xB3/s"},   /* m³/s  */
  {"cubic inch per second",  "in\xB3""/s", 16.387064e-6,   "in\xC2\xB3/s"},  /* in³/s */  /* 0.0254^3  */
  {"cubic foot per second",  "ft\xB3""/s", 0.028316846592, "ft\xC2\xB3/s"}}; /* ft³/s */  /* 0.3048^3  */

#define IMATEX_ILLUMINANCE_COUNT 4
static ImatExUnit IMATEX_ILLUMINANCE_UNITS [IMATEX_UNIT_MAXCOUNT] = {
  {"lux",                   "lx",     1,             NULL},
  {"footcandle",            "fc",     10.763910417,  NULL},   /* lumen per square foot  */
  {"lumen per square inch", "lm/in\xB2", 1550.00310001, "lm/in\xC2\xB2"},  /* lm/in² */
  {"phot",                  "ph",     1.0e4,         NULL}};

#define IMATEX_ELECTRIC_CHARGE_COUNT 3
static ImatExUnit IMATEX_ELECTRIC_CHARGE_UNITS [IMATEX_UNIT_MAXCOUNT] = {
  {"Coulomb",               "C",     1,          NULL},   /* A·s */
  {"Faraday",               "F",     96485.3383, NULL},
  {"milliampere hour",      "mA\xB7""h",  3.6,        "mA\xC2\xB7h"}}; /* mA·h */


typedef struct _ImatExQuantity {
  const char* q_name;
  const ImatExUnit* units;
  int units_count;
} ImatExQuantity;

#define IMATEX_TEMPERATURE 1
#define IMATEX_QUANTITY_COUNT 31
#define IMATEX_QUANTITY_CUSTOM 25
static int imatex_quantity_count = IMATEX_QUANTITY_COUNT;
static ImatExQuantity imatex_quantities [IMATEX_QUANTITY_COUNT+IMATEX_QUANTITY_CUSTOM] = {
  { "None"             , NULL                       , 0                             },
  { "Temperature"      , IMATEX_TEMPERATURE_UNITS     , IMATEX_TEMPERATURE_COUNT      },  /* must not be change from here */
  { "Length"           , IMATEX_LENGTH_UNITS          , IMATEX_LENGTH_COUNT           },
  { "Time"             , IMATEX_TIME_UNITS            , IMATEX_TIME_COUNT             },
  { "Mass"             , IMATEX_MASS_UNITS            , IMATEX_MASS_COUNT             },
  { "Area"             , IMATEX_AREA_UNITS            , IMATEX_AREA_COUNT             },
  { "Volume"           , IMATEX_VOLUME_UNITS          , IMATEX_VOLUME_COUNT           },
  { "Speed"            , IMATEX_SPEED_UNITS           , IMATEX_SPEED_COUNT            },
  { "Velocity"         , IMATEX_SPEED_UNITS           , IMATEX_SPEED_COUNT            },
  { "Angular Speed"    , IMATEX_ANGULAR_SPEED_UNITS   , IMATEX_ANGULAR_SPEED_COUNT    },
  { "Acceleration"     , IMATEX_ACCELERATION_UNITS    , IMATEX_ACCELERATION_COUNT     },
  { "Pressure"         , IMATEX_PRESSURE_UNITS        , IMATEX_PRESSURE_COUNT         },
  { "Mechanical Stress", IMATEX_PRESSURE_UNITS        , IMATEX_PRESSURE_COUNT         },
  { "Force"            , IMATEX_FORCE_UNITS           , IMATEX_FORCE_COUNT            },
  { "Force per length" , IMATEX_FORCE_PER_LENGTH_UNITS, IMATEX_FORCE_PER_LENGTH_COUNT },
  { "Linear Weight"    , IMATEX_FORCE_PER_LENGTH_UNITS, IMATEX_FORCE_PER_LENGTH_COUNT },
  { "Torque"           , IMATEX_MOMENT_UNITS          , IMATEX_MOMENT_COUNT           },
  { "Moment of Force"  , IMATEX_MOMENT_UNITS          , IMATEX_MOMENT_COUNT           },
  { "Angle"            , IMATEX_ANGLE_UNITS           , IMATEX_ANGLE_COUNT            },
  { "Specific Mass"    , IMATEX_SPECIFIC_MASS_UNITS   , IMATEX_SPECIFIC_MASS_COUNT    },
  { "Density"          , IMATEX_SPECIFIC_MASS_UNITS   , IMATEX_SPECIFIC_MASS_COUNT    },
  { "Specific Weight"  , IMATEX_SPECIFIC_WEIGHT_UNITS , IMATEX_SPECIFIC_WEIGHT_COUNT  },
  { "Energy"           , IMATEX_ENERGY_UNITS          , IMATEX_ENERGY_COUNT           },
  { "Power"            , IMATEX_POWER_UNITS           , IMATEX_POWER_COUNT            },
  { "Heat Flow Rate"   , IMATEX_POWER_UNITS           , IMATEX_POWER_COUNT            },
  { "Fraction"         , IMATEX_FRACTION_UNITS        , IMATEX_FRACTION_COUNT         },
  { "Flow"             , IMATEX_FLOW_UNITS            , IMATEX_FLOW_COUNT             },
  { "Illuminance"      , IMATEX_ILLUMINANCE_UNITS     , IMATEX_ILLUMINANCE_COUNT      },
  { "Kinematic Viscosity", IMATEX_KINEMATIC_VISCOSITY_UNITS, IMATEX_KINEMATIC_VISCOSITY_COUNT},
  { "Dynamic Viscosity"  , IMATEX_DYNAMIC_VISCOSITY_UNITS  , IMATEX_DYNAMIC_VISCOSITY_COUNT  },
  { "Electric Charge"    , IMATEX_ELECTRIC_CHARGE_UNITS    , IMATEX_ELECTRIC_CHARGE_COUNT    },
};

static ImatExUnit IMATEX_CUSTOM_UNITS[IMATEX_QUANTITY_CUSTOM][IMATEX_UNIT_MAXCOUNT];

static int imatex_unity_spell = 0;
static int imatex_last_addquantity = -1;
static int imatex_last_addunit = -1;


/* Same definition as in IupMatrix */
static double iMatrixConvertFunc(double number, int quantity, int unit_from, int unit_to)
{
  /* this function is called only when unit_from!=unit_to */
  const ImatExUnit* units = imatex_quantities[quantity].units;

  if (quantity == IMATEX_TEMPERATURE)  /* the only quantity that we know that there is an offset for conversion */
  {
    if (unit_from!=0)
    {
      if (unit_from==IMATEX_CELSIUS)
        number = number + 273.15;
      else if (unit_from==IMATEX_FAHRENHEIT)
        number = number + 459.67;

      number = number * units[unit_from].factor;
    }

    if (unit_to!=0)
    {
      number = number / units[unit_to].factor;

      if (unit_from==IMATEX_CELSIUS)
        number = number - 273.15;
      else if (unit_from==IMATEX_FAHRENHEIT)
        number = number - 459.67;
    }
  }
  else
  {
    if (unit_from!=0)
      number = number * units[unit_from].factor;

    if (unit_to!=0)
      number = number / units[unit_to].factor;
  }

  return number;
}

static char* iMatrixExReturnSymbol(const ImatExUnit* units)
{
  if (units->symbol_utf8)
  {
    if (IupGetInt(NULL, "UTF8MODE"))
      return (char*)units->symbol_utf8;
  }
                                                       
  return (char*)units->symbol;
}

static int iMatrixGetUnity(const char* name, char* am_name)
{
  /* Convert to American spelling */
  const char* s = strstr(name, "metre");
  if (s)
  {
    int off = (int)(s - name);
    strcpy(am_name, name);
    strncpy(am_name+off, "meter", 5);
    return 1;
  }
  else
  {
    s = strstr(name, "litre");
    if (s)
    {
      int off = (int)(s - name);
      strcpy(am_name, name);
      strncpy(am_name+off, "liter", 5);   /* don't confuse with litter */
      return 1;
    }
  }

  return 0;
}

static char* iMatrixExReturnUnit(const char* name)
{
  if (imatex_unity_spell)
  {
    char am_name[30];
    if (iMatrixGetUnity(name, am_name))
      return iupStrReturnStr(am_name);
  }

  return (char*)name;
}

static int iMatrixCompareUnity(const char* name, const char* value)
{
  if (imatex_unity_spell)
  {
    char am_name[30];
    if (iMatrixGetUnity(name, am_name))
      return iupStrEqualNoCaseNoSpace(am_name, value);
  }

  return iupStrEqualNoCaseNoSpace(name, value);
}
        
static int iMatrixFindQuantity(const char* value)
{
  int i;
  for (i=0; i<imatex_quantity_count; i++)
  {
    if (iupStrEqualNoCaseNoSpace(imatex_quantities[i].q_name, value))
      return i;
  }

  return -1;
}

static int iMatrixFindUnit(const ImatExUnit* units, int units_count, const char* value)
{
  int i;
  for (i=0; i<units_count; i++)
  {
    if (iMatrixCompareUnity(units[i].u_name, value))
      return i;
  }

  return -1;
}

static int iMatrixFindUnitSymbol(const ImatExUnit* units, int units_count, const char* value, int utf8)
{
  int i;
  for (i=0; i<units_count; i++)
  {
    const char* symbol;
    if (utf8 && units[i].symbol_utf8)
      symbol = units[i].symbol_utf8;
    else
      symbol = units[i].symbol;

    if (iupStrEqual(symbol, value))
      return i;
  }

  return -1;
}


/*****************************************************************************/


static int iMatrixExSetNumericUnitSpellAttrib(Ihandle* ih, const char* value)
{
  if (iupStrEqualNoCase(value, "AMERICAN"))
    imatex_unity_spell = 1;
  else
    imatex_unity_spell = 0;

  (void)ih;
  return 0;
}

static int iMatrixExSetNumericAddQuantityAttrib(Ihandle* ih, const char* value)
{
  int quantity = iMatrixFindQuantity(value);
  if (quantity < 0) /* not found */
  {
    /* add new quantity if has space */
    if (imatex_quantity_count < IMATEX_QUANTITY_COUNT+IMATEX_QUANTITY_CUSTOM)
    {
      imatex_quantities[imatex_quantity_count].q_name = value;
      imatex_quantities[imatex_quantity_count].units_count = 0;
      imatex_quantities[imatex_quantity_count].units = IMATEX_CUSTOM_UNITS[imatex_quantity_count - IMATEX_QUANTITY_COUNT];

      imatex_last_addquantity = imatex_quantity_count;
      imatex_quantity_count++;
    }
    else
      imatex_last_addquantity = -1;  /* no space left, return invalid quantity */
  }
  else
  {
    /* prepare to add units to existing quantity */
    imatex_last_addquantity = quantity;
  }

  imatex_last_addunit = -1;

  (void)ih;
  return 0;
}

static int iMatrixExSetNumericAddUnitAttrib(Ihandle* ih, const char* value)
{
  if (imatex_last_addquantity != -1)
  {
    int quantity = imatex_last_addquantity;

    /* add new unit if has space */
    if (imatex_quantities[quantity].units_count < IMATEX_UNIT_MAXCOUNT)
    {
      ImatExUnit* units = (ImatExUnit*)(imatex_quantities[quantity].units);
      int unit = imatex_quantities[quantity].units_count;

      units[unit].u_name = value;
      units[unit].symbol = NULL;
      units[unit].factor = 0;
      units[unit].symbol_utf8 = NULL;

      imatex_last_addunit = unit;
      imatex_quantities[quantity].units_count++;
      return 0;
    }
  }

  imatex_last_addunit = -1;

  (void)ih;
  return 0;
}

static int iMatrixExSetNumericAddUnitSymbolAttrib(Ihandle* ih, const char* value)
{
  if (imatex_last_addquantity != -1 && imatex_last_addunit != -1)
  {
    int quantity = imatex_last_addquantity;
    int unit = imatex_last_addunit;
    ImatExUnit* units = (ImatExUnit*)(imatex_quantities[quantity].units);
    units[unit].symbol = value;
  }

  (void)ih;
  return 0;
}

static int iMatrixExSetNumericAddUnitFactorAttrib(Ihandle* ih, const char* value)
{
  if (imatex_last_addquantity != -1 && imatex_last_addunit != -1)
  {
    double factor = 0;
    if (iupStrToDouble(value, &factor))
    {
      int quantity = imatex_last_addquantity;
      int unit = imatex_last_addunit;
      ImatExUnit* units = (ImatExUnit*)(imatex_quantities[quantity].units);
      units[unit].factor = factor;
    }
  }

  (void)ih;
  return 0;
}


/*****************************************************************************/


static int iMatrixExSetNumericUnitSearchAttrib(Ihandle* ih, const char* value)
{
  int q, u;

  iupAttribSet(ih, "NUMERICFOUNDQUANTITY", NULL);
  iupAttribSet(ih, "NUMERICFOUNDUNIT", NULL);
  iupAttribSet(ih, "NUMERICFOUNDUNITSYMBOL", NULL);

  for (q=0; q<imatex_quantity_count; q++)
  {
    ImatExUnit* units = (ImatExUnit*)(imatex_quantities[q].units);
    int units_count = imatex_quantities[q].units_count;
    for (u=0; u<units_count; u++)
    {
      if (iMatrixCompareUnity(units[u].u_name, value))
      {
        iupAttribSet(ih, "NUMERICFOUNDQUANTITY", imatex_quantities[q].q_name);
        iupAttribSet(ih, "NUMERICFOUNDUNIT", units[u].u_name);
        iupAttribSetStr(ih, "NUMERICFOUNDUNITSYMBOL", iMatrixExReturnSymbol(units + u));
        return 0;
      }
    }
  }

  (void)ih;
  return 0;
}

static int iMatrixExSetNumericUnitSymbolSearchAttrib(Ihandle* ih, const char* value)
{
  int q, u;

  int utf8 = IupGetInt(NULL, "UTF8MODE");

  iupAttribSet(ih, "NUMERICFOUNDQUANTITY", NULL);
  iupAttribSet(ih, "NUMERICFOUNDUNIT", NULL);
  iupAttribSet(ih, "NUMERICFOUNDUNITSYMBOL", NULL);

  for (q=0; q<imatex_quantity_count; q++)
  {
    ImatExUnit* units = (ImatExUnit*)(imatex_quantities[q].units);
    int units_count = imatex_quantities[q].units_count;
    for (u=0; u<units_count; u++)
    {
      const char* symbol;
      if (utf8 && units[u].symbol_utf8)
        symbol = units[u].symbol_utf8;
      else
        symbol = units[u].symbol;

      if (iupStrEqual(symbol, value))
      {
        iupAttribSet(ih, "NUMERICFOUNDQUANTITY", imatex_quantities[q].q_name);
        iupAttribSet(ih, "NUMERICFOUNDUNIT", units[u].u_name);
        iupAttribSetStr(ih, "NUMERICFOUNDUNITSYMBOL", symbol);
        return 0;
      }
    }
  }

  (void)ih;
  return 0;
}


/*****************************************************************************/


static int iMatrixExSetNumericQuantityAttrib(Ihandle* ih, int col, const char* value)
{
  if (!value)
  {
    IupSetCallback(ih, "NUMERICCONVERT_FUNC", NULL);
    IupSetAttributeId(ih, "NUMERICQUANTITYINDEX", col, NULL);
  }
  else
  {
    int quantity = iMatrixFindQuantity(value);
    if (quantity < 0)
      quantity = 0;  /* Set to None, this will enable the numeric column, but no unit conversion */

    /* set the callback before setting the attribute */
    IupSetCallback(ih, "NUMERICCONVERT_FUNC", (Icallback)iMatrixConvertFunc);
    IupSetIntId(ih, "NUMERICQUANTITYINDEX", col, quantity);
  }

  return 0;
}

static int iMatrixExCheckQuantity(int quantity)
{
  if (quantity < 0 || quantity >= imatex_quantity_count)
    return 0;
  else
    return 1;
}

static int iMatrixExCheckUnit(int quantity, int unit)
{
  if (unit < 0 || unit >= imatex_quantities[quantity].units_count)
    return 0;
  else
    return 1;
}

static char* iMatrixExGetNumericQuantityAttrib(Ihandle* ih, int col)
{
  if (!IupGetAttributeId(ih, "NUMERICQUANTITYINDEX", col))
    return NULL;
  else
  {
    int quantity = IupGetIntId(ih, "NUMERICQUANTITYINDEX", col);
    if (!iMatrixExCheckQuantity(quantity))
      return NULL;

    return (char*)imatex_quantities[quantity].q_name;
  }
}

static int iMatrixExSetNumericUnitAttrib(Ihandle* ih, int col, const char* value)
{
  int unit;
  int quantity = IupGetIntId(ih, "NUMERICQUANTITYINDEX", col);
  if (quantity == 0) /* NULL or None */
    return 0;
  if (!iMatrixExCheckQuantity(quantity))
    return 0;

  unit = iMatrixFindUnit(imatex_quantities[quantity].units, imatex_quantities[quantity].units_count, value);
  if (unit < 0)
    return 0;

  IupSetIntId(ih, "NUMERICUNITINDEX", col, unit);
  return 0;
}

static char* iMatrixExGetNumericUnitAttrib(Ihandle* ih, int col)
{
  int unit;
  int quantity = IupGetIntId(ih, "NUMERICQUANTITYINDEX", col);
  if (quantity == 0) /* NULL or None */
    return NULL;
  if (!iMatrixExCheckQuantity(quantity))
    return NULL;

  unit = IupGetIntId(ih, "NUMERICUNITINDEX", col);
  if (!iMatrixExCheckUnit(quantity, unit))
    return NULL;

  return iMatrixExReturnUnit(imatex_quantities[quantity].units[unit].u_name);
}

static int iMatrixExSetNumericUnitShownAttrib(Ihandle* ih, int col, const char* value)
{
  int unit;
  int quantity = IupGetIntId(ih, "NUMERICQUANTITYINDEX", col);
  if (quantity == 0) /* NULL or None */
    return 0;
  if (!iMatrixExCheckQuantity(quantity))
    return 0;

  unit = iMatrixFindUnit(imatex_quantities[quantity].units, imatex_quantities[quantity].units_count, value);
  if (unit < 0)
    return 0;

  IupSetIntId(ih, "NUMERICUNITSHOWNINDEX", col, unit);
  return 0;
}

static char* iMatrixExGetNumericUnitShownAttrib(Ihandle* ih, int col)
{
  int unit;
  int quantity = IupGetIntId(ih, "NUMERICQUANTITYINDEX", col);
  if (quantity == 0) /* NULL or None */
    return NULL;
  if (!iMatrixExCheckQuantity(quantity))
    return NULL;

  unit = IupGetIntId(ih, "NUMERICUNITSHOWNINDEX", col);
  if (!iMatrixExCheckUnit(quantity, unit))
    return NULL;

  return iMatrixExReturnUnit(imatex_quantities[quantity].units[unit].u_name);
}

static int iMatrixExSetNumericUnitSymbolShownAttrib(Ihandle* ih, int col, const char* value)
{
  int unit, utf8;
  int quantity = IupGetIntId(ih, "NUMERICQUANTITYINDEX", col);
  if (quantity == 0) /* NULL or None */
    return 0;
  if (!iMatrixExCheckQuantity(quantity))
    return 0;

  utf8 = IupGetInt(NULL, "UTF8MODE");

  unit = iMatrixFindUnitSymbol(imatex_quantities[quantity].units, imatex_quantities[quantity].units_count, value, utf8);
  if (unit < 0)
    return 0;

  IupSetIntId(ih, "NUMERICUNITSHOWNINDEX", col, unit);
  return 0;
}

static char* iMatrixExGetNumericUnitSymbolAttrib(Ihandle* ih, int col)
{
  int unit;
  int quantity = IupGetIntId(ih, "NUMERICQUANTITYINDEX", col);
  if (quantity == 0) /* NULL or None */
    return NULL;
  if (!iMatrixExCheckQuantity(quantity))
    return NULL;

  unit = IupGetIntId(ih, "NUMERICUNITINDEX", col);
  if (!iMatrixExCheckUnit(quantity, unit))
    return NULL;

  return iMatrixExReturnSymbol(&(imatex_quantities[quantity].units[unit]));
}

static int iMatrixExSetNumericUnitSymbolAttrib(Ihandle* ih, int col, const char* value)
{
  int unit, utf8;
  int quantity = IupGetIntId(ih, "NUMERICQUANTITYINDEX", col);
  if (quantity == 0) /* NULL or None */
    return 0;
  if (!iMatrixExCheckQuantity(quantity))
    return 0;

  utf8 = IupGetInt(NULL, "UTF8MODE");

  unit = iMatrixFindUnitSymbol(imatex_quantities[quantity].units, imatex_quantities[quantity].units_count, value, utf8);
  if (unit < 0)
    return 0;

  IupSetIntId(ih, "NUMERICUNITINDEX", col, unit);
  return 0;
}

static char* iMatrixExGetNumericUnitSymbolShownAttrib(Ihandle* ih, int col)
{
  int unit;
  int quantity = IupGetIntId(ih, "NUMERICQUANTITYINDEX", col);
  if (quantity == 0) /* NULL or None */
    return NULL;
  if (!iMatrixExCheckQuantity(quantity))
    return NULL;

  unit = IupGetIntId(ih, "NUMERICUNITSHOWNINDEX", col);
  if (!iMatrixExCheckUnit(quantity, unit))
    return NULL;

  return iMatrixExReturnSymbol(&(imatex_quantities[quantity].units[unit]));
}

static char* iMatrixExGetNumericUnitCountAttrib(Ihandle* ih, int col)
{
  int quantity = IupGetIntId(ih, "NUMERICQUANTITYINDEX", col);
  if (quantity == 0) /* NULL or None */
    return NULL;
  if (!iMatrixExCheckQuantity(quantity))
    return NULL;

  return iupStrReturnInt(imatex_quantities[quantity].units_count);
}

void iupMatrixExRegisterUnits(Iclass* ic)
{
  /* Defined in IupMatrix - Internal
    NUMERICQUANTITYINDEX
    NUMERICUNITINDEX
    NUMERICUNITSHOWNINDEX */

  /* Defined in IupMatrix - Exported
    NUMERICFORMAT
    NUMERICFORMATPRECISION
    NUMERICFORMATTITLE
    NUMERICFORMATDEF   */

  iupClassRegisterAttributeId(ic, "NUMERICQUANTITY", iMatrixExGetNumericQuantityAttrib, iMatrixExSetNumericQuantityAttrib, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "NUMERICUNIT", iMatrixExGetNumericUnitAttrib, iMatrixExSetNumericUnitAttrib, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "NUMERICUNITSHOWN", iMatrixExGetNumericUnitShownAttrib, iMatrixExSetNumericUnitShownAttrib, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "NUMERICUNITSYMBOL", iMatrixExGetNumericUnitSymbolAttrib, iMatrixExSetNumericUnitSymbolAttrib, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "NUMERICUNITSYMBOLSHOWN", iMatrixExGetNumericUnitSymbolShownAttrib, iMatrixExSetNumericUnitSymbolShownAttrib, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "NUMERICUNITCOUNT", iMatrixExGetNumericUnitCountAttrib, NULL, IUPAF_NOT_MAPPED|IUPAF_READONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "NUMERICUNITSPELL", NULL, iMatrixExSetNumericUnitSpellAttrib, IUPAF_SAMEASSYSTEM, "INTERNATIONAL", IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NUMERICADDQUANTITY", NULL, iMatrixExSetNumericAddQuantityAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NUMERICADDUNIT", NULL, iMatrixExSetNumericAddUnitAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NUMERICADDUNITSYMBOL", NULL, iMatrixExSetNumericAddUnitSymbolAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NUMERICADDUNITFACTOR", NULL, iMatrixExSetNumericAddUnitFactorAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "NUMERICUNITSEARCH", NULL, iMatrixExSetNumericUnitSearchAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NUMERICUNITSYMBOLSEARCH", NULL, iMatrixExSetNumericUnitSymbolSearchAttrib, NULL, NULL, IUPAF_NOT_MAPPED|IUPAF_WRITEONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NUMERICFOUNDQUANTITY", NULL, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NUMERICFOUNDUNIT", NULL, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "NUMERICFOUNDUNITSYMBOL", NULL, NULL, NULL, NULL, IUPAF_READONLY|IUPAF_NO_INHERIT);
}
