///////////////////////////////////////////////////////////////////////
////  Copyright 2015 Samsung Austin Semiconductor, LLC.                //
/////////////////////////////////////////////////////////////////////////
//
#ifndef _PREDICTOR_H_
#define _PREDICTOR_H_

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <math.h>

#include "../../../src/utils.h"
#include "../../../build/parameters.h"
#include "../../../build/statistics.h"

#include <cereal/archives/json.hpp>
#include <cereal/types/array.hpp>

// Struct with statistics which will be serialized to json
struct stats_t {
    //NOTE: competitors are judged solely on MISPRED_PER_1K_INST. The additional stats are just for tuning your predictors.
    double MPKBr_1K = std::numeric_limits<double>::quiet_NaN();
    double MPKBr_10K = std::numeric_limits<double>::quiet_NaN();
    double MPKBr_100K = std::numeric_limits<double>::quiet_NaN();
    double MPKBr_1M = std::numeric_limits<double>::quiet_NaN();
    double MPKBr_10M = std::numeric_limits<double>::quiet_NaN();
    double MPKBr_30M = std::numeric_limits<double>::quiet_NaN();
    double MPKBr_60M = std::numeric_limits<double>::quiet_NaN();
    double MPKBr_100M = std::numeric_limits<double>::quiet_NaN();
    double MPKBr_300M = std::numeric_limits<double>::quiet_NaN();
    double MPKBr_600M = std::numeric_limits<double>::quiet_NaN();
    double MPKBr_1B = std::numeric_limits<double>::quiet_NaN();
    double MPKBr_10B = std::numeric_limits<double>::quiet_NaN();
    uint64_t NUM_INSTRUCTIONS;
    uint64_t NUM_BR;
    uint64_t NUM_UNCOND_BR;
    uint64_t NUM_CONDITIONAL_BR;
    uint64_t NUM_MISPREDICTIONS;
    double MISPRED_PER_1K_INST = std::numeric_limits<double>::quiet_NaN();
    std::string TRACE;

#ifdef USER_STATS
    // Users can define their own stats here
#endif

    template<class Archive>
    void serialize(Archive & ar)
    {
        ar(
            CEREAL_NVP(MPKBr_1K),
            CEREAL_NVP(MPKBr_10K),
            CEREAL_NVP(MPKBr_100K),
            CEREAL_NVP(MPKBr_1M),
            CEREAL_NVP(MPKBr_10M),
            CEREAL_NVP(MPKBr_30M),
            CEREAL_NVP(MPKBr_60M),
            CEREAL_NVP(MPKBr_100M),
            CEREAL_NVP(MPKBr_300M),
            CEREAL_NVP(MPKBr_600M),
            CEREAL_NVP(MPKBr_1B),
            CEREAL_NVP(MPKBr_10B),
            CEREAL_NVP(TRACE),

#ifdef USER_STATS
    // Users can define their own stats here
#endif

            CEREAL_NVP(NUM_INSTRUCTIONS),
            CEREAL_NVP(NUM_BR),
            CEREAL_NVP(NUM_UNCOND_BR),
            CEREAL_NVP(NUM_CONDITIONAL_BR),
            CEREAL_NVP(NUM_MISPREDICTIONS),
            CEREAL_NVP(MISPRED_PER_1K_INST)
        );
    }
};

stats_t stats;

#define UINT64 uint64_t
#define ASSERT(cond) if (!(cond)) {fprintf(stderr,"file %s assert line %d\n",__FILE__,__LINE__); abort();}


/***************************************************
              2bc gskew predictor: History Lengthes tuned for the CBP challenge

The indexing functions have not been optimized for each predictor
size,  but are derived from old PARLE'93 skewed associative paper.
Characteristics for a 2**N bits predictor:
-------------------------------------------
-  a single array of 2**(N-2) hysteresis bits (four-way banked)
   is shared among  the 4 logical tables
-  G0 and G1 logical prediction tables  shared a single array
   of 2**(N-1) bits prediction bits)
-  BIM  and Meta logical prediction tables  shared a single array
   of 2**(N-2) bits prediction bits)

history lengths optimized for the set of benchmarks at CBP
 ***************************************************/


#include <math.h>

//#define LOGPRED 18
#ifndef NR
#define NR 31
#endif


#include <cstddef>
#include <inttypes.h>
#include <vector>


static char GOG1[1 << (LOGPRED - G0G1_SIZE)];
/* GOG1: shared prediction tables for G0 and G1*/
static char BIMMETA[1 << (LOGPRED - BIM_META_SIZE)];
/*BIMMETA: shared prediction tables for  BIM and META*/
static char HYST[1 << (LOGPRED - HYST_SIZE)];
/* HYST: shared hysteresis tables */
long long ghist;


//static const int L_BIM = 10;
//static const int L_G0 = 24;
//static const int L_G1 = 64;
//static const int L_META = 14;
static int LOGSIZE;
/*********************************************/
/* for the index functions*/
int
H (long long a)

{
 long long res  = 0;
  res = a ^ (a << (LOGSIZE - 1));
  res = res & (1 << (LOGSIZE - 1));
  a = a & ((1 << LOGSIZE) - 1);
  a = a >> 1;
  res = res + a;
  return (int (res));
}

/*********************************************
 inverse fonction Hi
 *********************************************/

 int
Hi (long long a)

{
 long long res  = 0;
  res = a >> (LOGSIZE - 1);
  res = (res ^ (a >> (LOGSIZE - 2))) & 1;
  a = a & ((1 << (LOGSIZE - 1)) - 1);
  a = a << 1;
  res = res + a;
  return (int (res));
}

/********************************************
skewing functions from PARLE 93 paper
 ********************************************/

 int
F1 (long long a)
{
 long long res ;
  res = (H (a) ^ Hi (a >> LOGSIZE) ^ (a >> LOGSIZE)) & ((1 << LOGSIZE) - 1);
  return (int (res));
}

 int
F2 (long long a)

{
 long long res ;


  res = (H (a) ^ Hi (a >> LOGSIZE) ^ (a)) & ((1 << LOGSIZE) - 1);
  return (int (res));
}

 int
F3 (long long a)

{
 long long res ;

  res = (Hi (a) ^ H (a >> LOGSIZE) ^ (a >> LOGSIZE)) & ((1 << LOGSIZE) - 1);
  return (int (res));
}
 int
F4 (long long a)

{
 long long res ;

  res = (Hi (a) ^ H (a >> LOGSIZE) ^ (a)) & ((1 << LOGSIZE) - 1);
  return (int (res));
}

 int
INDEX (long long Add,  long long histo, int m, int funct)
{
  long long hm, inter;
  int i;
  int RES;

  if (m < 32)
    hm = (histo & ((1 << m) - 1)) + (Add << m);
  else
    {
      if (m != 32)
	{
	  hm = (histo << (64 - m)) ^ (Add);
	}
      else
	hm = ((histo & (0xFFFFFFFF)) << 18) ^ (Add);
    }
  hm = hm ^ (Add << funct) ^ (Add << (10 + funct));
/* incorporate address bits*/
  inter = hm;
  for (i = 0; i < 64; i += (2 * (LOGSIZE - funct) + 1))
    {
      inter = inter >> LOGSIZE;
      inter = inter >> (LOGSIZE - (funct + 1));
      hm = (hm ^ inter);
    }
  switch (funct)
    {
    case 4:
RES= (F4 (hm));
      break;
    case 1:
   RES = (F1 (hm));
      break;

    case 2:
      RES = (F2 (hm));
      break;
    case 3:
      RES = (F3 (hm));
      break;
    default:
      printf (" Problem, inumplemented index function F%d\n", funct);
      exit (1);
    }

  return ( RES);
}


class TWOBCGSKEW
{
public:

  bool get_prediction (UINT64 PC)
  {
    int add,indexbim, indexg1, indexg0, indexmeta,  NUMHYST;
    char pg0, pg1, pbim, pmeta;
    bool prediction = false;
    long long Add;

    long long GHIST;
    add = PC;
    add = (add >> 4) ^ add;
    NUMHYST = ((add ^ (int (ghist))) & 3);
    Add = (long long) add;
    GHIST =  ghist  ^ ((ghist & 3) << 5);
    Add = Add ^ (Add >> 5);

    LOGSIZE = LOGPRED - 3;
    indexg0 = (INDEX (Add, GHIST, L_G0, 1) << 2) + (NUMHYST);
    indexg1 = (INDEX (Add, GHIST, L_G1, 2) << 2) + (NUMHYST ^ 1);

    LOGSIZE = LOGPRED - 4;
    indexbim = (INDEX (Add, GHIST, L_BIM, 3) << 2) + (NUMHYST ^ 2);
    indexmeta = (INDEX (Add, GHIST, L_META, 4) << 2) + (NUMHYST ^ 3);

    ASSERT((indexg0>>(LOGPRED-1))==0);
    ASSERT((indexg1>>(LOGPRED-1))==0);
    ASSERT((indexbim>>(LOGPRED-2))==0);
    ASSERT((indexmeta>>(LOGPRED-2))==0);

    pg0 = GOG1[indexg0];
    pg1 = GOG1[indexg1];
    pbim = BIMMETA[indexbim];
    pmeta = BIMMETA[indexmeta];
    if (pmeta)
      prediction = ((pbim + pg0 + pg1) > 1);
    else
      prediction = (pbim > 0);

    return prediction;		// true for taken, false for not taken
  }


  void update_predictor (UINT64 PC, bool taken)
  {
    int add, indexbim, indexg1, indexg0, indexmeta,  NUMHYST;
    char pg0, pg1, pbim, pmeta, PESKEW,outcome;
    bool peskew, prediction, psmall;
    long long Add;
    long long GHIST;


    /* first recompute the prediction */
    //long long GHIST;
    add = PC;
    add = (add >> 4) ^ add;
    NUMHYST = ((add ^ (int (ghist))) & 3);
    Add = (long long) add;
    GHIST =  ghist  ^ ((ghist & 3) << 5);
    Add = Add ^ (Add >> 5);

    LOGSIZE = LOGPRED - 3;
    indexg0 = (INDEX (Add, GHIST, L_G0, 1) << 2) + (NUMHYST);
    indexg1 = (INDEX (Add, GHIST, L_G1, 2) << 2) + (NUMHYST ^ 1);
    /*indexg0, indexg1 are smaller than 2**(LOGPRED-1)*/

    LOGSIZE = LOGPRED - 4;
    indexbim = (INDEX (Add, GHIST, L_BIM, 3) << 2) + (NUMHYST ^ 2);
    indexmeta = (INDEX (Add, GHIST, L_META, 4) << 2) + (NUMHYST ^ 3);
    /*indexg0, indexg1 are smaller than 2**(LOGPRED-2)*/
    pg0 = GOG1[indexg0];
    pg1 = GOG1[indexg1];
    pbim = BIMMETA[indexbim];
    pmeta = BIMMETA[indexmeta];
    PESKEW = pbim + pg0 + pg1;
    peskew = ((pbim + pg0 + pg1) > 1);
    psmall = (pbim > 0);

    if (pmeta)
      prediction = peskew;
    else
      prediction = psmall;

    /*recompute the complete counter values*/
    pg0 = (pg0 << 1) + HYST[indexg0 & ((1 << (LOGPRED - 2)) - 1)];
    pg1 = (pg1 << 1) + HYST[indexg1 & ((1 << (LOGPRED - 2)) - 1)];
    pbim = (pbim << 1) + HYST[indexbim];
    pmeta = (pmeta << 1) + HYST[indexmeta];
    /*      pg0 ^= 1;
	    pg1 ^= 1;
	    pbim ^= 1;
	    pmeta ^=1;*/

    if (taken)
      outcome = 1;
    else
      outcome = 0;
    /*always easier to manipulate integers than booleans */
    if ((prediction != taken) & ((random () & NR) == 0))
      {
	/* to break ping-pong phenomena*/
	if (peskew == psmall)
	  {
	    if (taken)
	      {
		pbim = 2;
		pg0 = 2;
		pg1 = 2;
	      }
	    else
	      {
		pbim = 1;
		pg0 = 1;
		pg1 = 1;
	      }
	  }

	else
	  {
	    pmeta = (pmeta & 2) ^ 2;
	  }
      }
    else if (PESKEW != 3 * outcome)
      {
	if ((pbim & 2) == 2 * outcome)
	  {
	    pbim = 3 * outcome;
	  }
	else if (prediction != taken)
	  pbim = (pbim & 1) + 1;

	if (peskew != psmall)
	  {
	    if (peskew == outcome)
	      {
		pmeta++;
		if (pmeta > 3)
		  pmeta = 3;
	      }
	    else
	      {
		pmeta--;
		if (pmeta < 0)
		  pmeta = 0;
	      }
	  }

	if ((pmeta > 1) | (prediction != taken))
	  {
	    if ((pg1 & 2) == 2 * outcome)
	      {
		pg1 = 3 * outcome;
	      }
	    else if (prediction != taken)
	      pg1 = (pg1 & 1) + 1;
	  }

	if ((pmeta > 1) | (prediction != taken))
	  {
	    if ((pg0 & 2) == 2 * outcome)
	      {

		pg0 = 3 * outcome;
	      }
	    else if (prediction != taken)
	      pg0 = (pg0 & 1) + 1;
	  }
      }

    /*	pg0 ^= 1;
        pg1 ^= 1;
        pbim ^= 1;
        pmeta ^=1;*/
    HYST[indexg0 & ((1 << (LOGPRED - 2)) - 1)] = (pg0 & 1);
    HYST[indexg1 & ((1 << (LOGPRED - 2)) - 1)] = (pg1 & 1);
    HYST[indexbim] = (pbim & 1);
    HYST[indexmeta] = (pmeta & 1);
    GOG1[indexg0] = (pg0 >> 1) & 1;
    GOG1[indexg1] = (pg1 >> 1) & 1;
    BIMMETA[indexbim] = (pbim >> 1) & 1;
    BIMMETA[indexmeta] = (pmeta >> 1) & 1;

    ghist = ghist << 1;
    if (taken)
      ghist++;


  }

};


class PREDICTOR {


 private:

  TWOBCGSKEW pred;

 public:

  PREDICTOR(void);

  bool    GetPrediction(UINT64 PC);
  void    UpdatePredictor(UINT64 PC, OpType opType, bool resolveDir, bool predDir, UINT64 branchTarget);
  void    TrackOtherInst(UINT64 PC, OpType opType, bool branchDir, UINT64 branchTarget);


};



PREDICTOR::PREDICTOR(void)
{


}


bool
PREDICTOR::GetPrediction(UINT64 PC)
{
  PC ^= PC >> 2;
  return pred.get_prediction (PC);
}



void
PREDICTOR::UpdatePredictor(UINT64 PC, OpType opType, bool resolveDir, bool predDir, UINT64 branchTarget)
{
  PC ^= PC >> 2;
  pred.update_predictor (PC,resolveDir);
}



void
PREDICTOR::TrackOtherInst(UINT64 PC, OpType opType, bool branchDir, UINT64 branchTarget)
{
  PC ^= PC >> 2;
}



#endif

