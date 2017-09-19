/// \file
/// Initialize the atom configuration.

#ifndef __INIT_ATOMS_H
#define __INIT_ATOMS_H

#include "mytype.h"
#include <Kokkos_Core.hpp>

struct SimFlatSt;
struct LinkCellSt;

/// Atom data
typedef struct AtomsSt
{
   // atom-specific data
   int nLocal;    //!< total number of atoms on this processor
   int nGlobal;   //!< total number of atoms in simulation

   int* gid;      //!< A globally unique id for each atom
   int* iSpecies; //!< the species index of the atom

   real3*  r;     //!< positions
   real3*  p;     //!< momenta of atoms
   real3*  f;     //!< forces 
   real_t* U;     //!< potential energy per atom
} Atoms;

// A struct needed to implement a multiple variable reduction in Kokkos
   struct vstruct {
      real_t v0;
      real_t v1;
      real_t v2;
      real_t v3;
      
      // default constructor
      KOKKOS_INLINE_FUNCTION
      vstruct() {
         v0 = 0.0;
         v1 = 0.0;
         v2 = 0.0;
         v3 = 0.0;
      }
     
      // operator += (volatile)
      KOKKOS_INLINE_FUNCTION
      vstruct & operator+=( const volatile vstruct & src) volatile {
         v0 += src.v0;
         v1 += src.v1;
         v2 += src.v2;
         v3 += src.v3;
      }

   };


/// Allocates memory to store atom data.
Atoms* initAtoms(struct LinkCellSt* boxes);
void destroyAtoms(struct AtomsSt* atoms);

void createFccLattice(int nx, int ny, int nz, real_t lat, struct SimFlatSt* s);

void setVcm(struct SimFlatSt* s, real_t vcm[3]);
void setTemperature(struct SimFlatSt* s, real_t temperature);
void randomDisplacements(struct SimFlatSt* s, real_t delta);
#endif
