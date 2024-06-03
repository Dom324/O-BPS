#pragma once

#include <string>
#include <inttypes.h>
#include <limits>
#include "assert.h"

#include "counter_types.h"
#include "../../../src/utils.h"
#include "../../../build/parameters.h"
#include "../../../build/statistics.h"

#include <cereal/archives/json.hpp>
#include <cereal/types/array.hpp>

class PREDICTOR{

    public:
        PREDICTOR(void);
        bool GetPrediction(uint64_t PC);
        void UpdatePredictor(uint64_t PC, OpType opType, bool resolveDir, bool predDir, uint64_t branchTarget);
        void TrackOtherInst(uint64_t PC, OpType opType, bool branchDir, uint64_t branchTarget);

    private:
        typedef sat_ctr<CTR_WIDTH> counter_t;
        const constinit static uint32_t numPhtEntries = (1 << PHT_SIZE);
        ::pht<numPhtEntries, CTR_INIT, counter_t, 1, HYST> pht;

        uint64_t index;
        uint64_t ghr;
        counter_t counter;

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

