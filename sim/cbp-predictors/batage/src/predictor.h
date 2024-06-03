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

#include "batage.h"

#define UINT64 uint64_t

class PREDICTOR {

 private:

  batage pred;
  histories hist;

 public:

  PREDICTOR(void);
  bool GetPrediction(UINT64 PC);
  void UpdatePredictor(UINT64 PC, OpType opType, bool resolveDir, bool predDir, UINT64 branchTarget);
  void TrackOtherInst(UINT64 PC, OpType opType, bool branchDir, UINT64 branchTarget);

};

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

extern stats_t stats;

#endif

