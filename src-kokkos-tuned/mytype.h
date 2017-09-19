/// \file
/// Frequently needed typedefs.

#ifndef __MYTYPE_H_
#define __MYTYPE_H_

/// \def COMD_SINGLE determines whether single or double precision is built
#ifdef COMD_SINGLE
typedef float real_t;  //!< define native type for CoMD as single precision
  #define FMT1 "%g"    //!< /def format argument for floats 
  #define EMT1 "%e"    //!< /def format argument for eng floats
#else
typedef double real_t; //!< define native type for CoMD as double precision
  #define FMT1 "%lg"   //!< \def format argument for doubles 
  #define EMT1 "%le"   //!< \def format argument for eng doubles 
#endif

typedef real_t real3[3]; //!< a convenience vector with three real_t 
__attribute__((vector))static void zeroReal3(real3 a)
{
   a[0] = 0.0;
   a[1] = 0.0;
   a[2] = 0.0;
}

#define screenOut stdout

#endif
