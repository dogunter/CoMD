/// \file
/// Computes forces for the 12-6 Lennard Jones (LJ) potential.
///
/// The Lennard-Jones model is not a good representation for the
/// bonding in copper, its use has been limited to constant volume
/// simulations where the embedding energy contribution to the cohesive
/// energy is not included in the two-body potential
///
/// The parameters here are taken from Wolf and Phillpot and fit to the
/// room temperature lattice constant and the bulk melt temperature
/// Ref: D. Wolf and S.Yip eds. Materials Interfaces (Chapman & Hall
///      1992) Page 230.
///
/// Notes on LJ:
///
/// http://en.wikipedia.org/wiki/Lennard_Jones_potential
///
/// The total inter-atomic potential energy in the LJ model is:
///
/// \f[
///   E_{tot} = \sum_{ij} U_{LJ}(r_{ij})
/// \f]
/// \f[
///   U_{LJ}(r_{ij}) = 4 \epsilon
///           \left\{ \left(\frac{\sigma}{r_{ij}}\right)^{12}
///           - \left(\frac{\sigma}{r_{ij}}\right)^6 \right\}
/// \f]
///
/// where \f$\epsilon\f$ and \f$\sigma\f$ are the material parameters in the potential.
///    - \f$\epsilon\f$ = well depth
///    - \f$\sigma\f$   = hard sphere diameter
///
///  To limit the interation range, the LJ potential is typically
///  truncated to zero at some cutoff distance. A common choice for the
///  cutoff distance is 2.5 * \f$\sigma\f$.
///  This implementation can optionally shift the potential slightly
///  upward so the value of the potential is zero at the cuotff
///  distance.  This shift has no effect on the particle dynamics.
///
///
/// The force on atom i is given by
///
/// \f[
///   F_i = -\nabla_i \sum_{jk} U_{LJ}(r_{jk})
/// \f]
///
/// where the subsrcipt i on the gradient operator indicates that the
/// derivatives are taken with respect to the coordinates of atom i.
/// Liberal use of the chain rule leads to the expression
///
/// \f{eqnarray*}{
///   F_i &=& - \sum_j U'_{LJ}(r_{ij})\hat{r}_{ij}\\
///       &=& \sum_j 24 \frac{\epsilon}{r_{ij}} \left\{ 2 \left(\frac{\sigma}{r_{ij}}\right)^{12}
///               - \left(\frac{\sigma}{r_{ij}}\right)^6 \right\} \hat{r}_{ij}
/// \f}
///
/// where \f$\hat{r}_{ij}\f$ is a unit vector in the direction from atom
/// i to atom j.
/// 
///

#include "ljForce.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <omp.h>

#include "constants.h"
#include "mytype.h"
#include "parallel.h"
#include "linkCells.h"
#include "memUtils.h"
#include "CoMDTypes.h"

#define POT_SHIFT 1.0

#ifdef __INTEL_COMPILER
        #define ASSUME_ALIGN
        #define _ALIGN_INT 64
#endif

/// Derived struct for a Lennard Jones potential.
/// Polymorphic with BasePotential.
/// \see BasePotential
typedef struct LjPotentialSt
{
   real_t cutoff;          //!< potential cutoff distance in Angstroms
   real_t mass;            //!< mass of atoms in intenal units
   real_t lat;             //!< lattice spacing (angs) of unit cell
   char latticeType[8];    //!< lattice type, e.g. FCC, BCC, etc.
   char  name[3];	   //!< element name
   int	 atomicNo;	   //!< atomic number  
   int  (*force)(SimFlat* s); //!< function pointer to force routine
   void (*print)(FILE* file, BasePotential* pot);
   void (*destroy)(BasePotential** pot); //!< destruction of the potential
   real_t sigma;
   real_t epsilon;
} LjPotential;

static int ljForce(SimFlat* s);
static void ljPrint(FILE* file, BasePotential* pot);

void ljDestroy(BasePotential** inppot)
{
   if ( ! inppot ) return;
   LjPotential* pot = (LjPotential*)(*inppot);
   if ( ! pot ) return;
   comdFree(pot);
   *inppot = NULL;

   return;
}

/// Initialize an Lennard Jones potential for Copper.
BasePotential* initLjPot(void)
{
   LjPotential *pot = (LjPotential*)comdMalloc(sizeof(LjPotential));
   pot->force = ljForce;
   pot->print = ljPrint;
   pot->destroy = ljDestroy;
   pot->sigma = 2.315;	                  // Angstrom
   pot->epsilon = 0.167;                  // eV
   pot->mass = 63.55 * amuToInternalMass; // Atomic Mass Units (amu)

   pot->lat = 3.615;                      // Equilibrium lattice const in Angs
   strcpy(pot->latticeType, "FCC");       // lattice type, i.e. FCC, BCC, etc.
   pot->cutoff = 2.5*pot->sigma;          // Potential cutoff in Angs

   strcpy(pot->name, "Cu");
   pot->atomicNo = 29;

   return (BasePotential*) pot;
}

void ljPrint(FILE* file, BasePotential* pot)
{
   LjPotential* ljPot = (LjPotential*) pot;
   fprintf(file, "  Potential type   : Lennard-Jones\n");
   fprintf(file, "  Species name     : %s\n", ljPot->name);
   fprintf(file, "  Atomic number    : %d\n", ljPot->atomicNo);
   fprintf(file, "  Mass             : "FMT1" amu\n", ljPot->mass / amuToInternalMass); // print in amu
   fprintf(file, "  Lattice Type     : %s\n", ljPot->latticeType);
   fprintf(file, "  Lattice spacing  : "FMT1" Angstroms\n", ljPot->lat);
   fprintf(file, "  Cutoff           : "FMT1" Angstroms\n", ljPot->cutoff);
   fprintf(file, "  Epsilon          : "FMT1" eV\n", ljPot->epsilon);
   fprintf(file, "  Sigma            : "FMT1" Angstroms\n", ljPot->sigma);
}


//reduction(+:sum_U,ePot) reduction(-:sum_F_0,sum_F_1,sum_F_2)
#pragma omp declare simd notinbranch aligned(s_atoms_R_0,s_atoms_R_1,s_atoms_R_2:_ALIGN_INT) uniform(jOff,iOff,rCut2,s6,eShift,epsilon)
static void compute_Sum(
		int jOff,
		int iOff,
		real_t rCut2,
		real_t s6,
		real_t eShift,
		real_t epsilon,
		real_t *s_atoms_R_0,
		real_t *s_atoms_R_1,
		real_t *s_atoms_R_2,
		real_t *sum_U,
		real_t *sum_F_0,
		real_t *sum_F_1,
		real_t *sum_F_2,
		real_t *ePot
		)
{
  	real_t dr_0 = 0.;
  	real_t dr_1 = 0.;
  	real_t dr_2 = 0.;
	real_t r2 = 0.0;

	dr_0 = s_atoms_R_0[iOff] - s_atoms_R_0[jOff];
	r2+=dr_0*dr_0;

	dr_1 = s_atoms_R_1[iOff] - s_atoms_R_1[jOff];
	r2+=dr_1*dr_1;

	dr_2 = s_atoms_R_2[iOff] - s_atoms_R_2[jOff];
	r2+=dr_2*dr_2;

	if ( r2 <= rCut2 && r2 > 0.0)
	{

		// Important note:
		// from this point on r actually refers to 1.0/r
		real_t r3 = 1.0/r2;
		real_t r6 = s6 * (r3*r3*r3);
		real_t eLocal = r6 * (r6 - 1.0) - eShift;
		*sum_U += 0.5*eLocal;
		*ePot  += 0.5*eLocal;

		// different formulation to avoid sqrt computation
		real_t fr = - 4.0*epsilon*r6*r3*(12.0*r6 - 6.0);

		*sum_F_0 -= dr_0*fr;
		*sum_F_1 -= dr_1*fr;
		*sum_F_2 -= dr_2*fr;
	}
}


int ljForce(SimFlat* s)
{
   LjPotential* pot = (LjPotential *) s->pot;
   real_t sigma = pot->sigma;
   real_t epsilon = pot->epsilon;
   real_t rCut = pot->cutoff;
   real_t rCut2 = rCut*rCut;

   // zero forces and energy
   real_t ePot = 0.0;
   s->ePotential = 0.0;
   int fSize = s->boxes->nTotalBoxes*MAXATOMS;

#ifdef ASSUME_ALIGN
   __assume_aligned(s->atoms->f_0,_ALIGN_INT);
   __assume_aligned(s->atoms->f_1,_ALIGN_INT);
   __assume_aligned(s->atoms->f_2,_ALIGN_INT);
   __assume_aligned(s->atoms->U,_ALIGN_INT);
#endif

   #pragma omp parallel for simd
   for (int ii=0; ii<fSize; ++ii)
   {
      s->atoms->f_0[ii] = 0.;
      s->atoms->f_1[ii] = 0.;
      s->atoms->f_2[ii] = 0.;
      s->atoms->U[ii] = 0.;
   }
   
   real_t s6 = sigma*sigma*sigma*sigma*sigma*sigma;

   real_t rCut6 = s6 / (rCut2*rCut2*rCut2);
   real_t eShift = POT_SHIFT * rCut6 * (rCut6 - 1.0);
   int nNbrBoxes = 27;

#ifdef ASSUME_ALIGN
   __assume_aligned(s->atoms->r_0,_ALIGN_INT);
   __assume_aligned(s->atoms->r_1,_ALIGN_INT);
   __assume_aligned(s->atoms->r_2,_ALIGN_INT);
#endif

   // loop over local boxes
   #pragma omp parallel for default(shared) reduction(+:ePot) //private(ePot)           
   for (int iBox=0; iBox<s->boxes->nLocalBoxes; iBox++)
   {
      int nIBox = s->boxes->nAtoms[iBox];
   
      // loop over neighbors of iBox
      for (int jTmp=0; jTmp<nNbrBoxes; jTmp++)
      {
         int jBox = s->boxes->nbrBoxes[iBox][jTmp];
         
         assert(jBox>=0);
         
         int nJBox = s->boxes->nAtoms[jBox];
         real_t sum_U;
         real_t sum_F_0;
         real_t sum_F_1;
         real_t sum_F_2;
         
         // loop over atoms in iBox
         for (int iOff=MAXATOMS*iBox; iOff<(iBox*MAXATOMS+nIBox); iOff++)
         {

             // loop over atoms in jBox
        	   #pragma simd reduction(+:sum_U,ePot) reduction(-:sum_F_0,sum_F_1,sum_F_2)
            for (int jOff=jBox*MAXATOMS; jOff<(jBox*MAXATOMS+nJBox); jOff++)
            {
					compute_Sum(
							jOff,
							iOff,
							rCut2,
							s6,
							eShift,
							epsilon,
							s->atoms->r_0,
							s->atoms->r_1,
							s->atoms->r_2,
							&sum_U,
							&sum_F_0,
							&sum_F_1,
							&sum_F_2,
							&ePot);
            } // loop over atoms in jBox

            s->atoms->U[iOff]   += sum_U;
            s->atoms->f_0[iOff] += sum_F_0;
            s->atoms->f_1[iOff] += sum_F_1;
            s->atoms->f_2[iOff] += sum_F_2;
            sum_F_0 = 0.;
            sum_F_1 = 0.;
            sum_F_2 = 0.;
            sum_U = 0.;

         } // loop over atoms in iBox
      } // loop over neighbor boxes
   } // loop over local boxes in system

   ePot = ePot*4.0*epsilon;
   s->ePotential = ePot;

   return 0;
}
