#include "predictor.h"

stats_t stats;

PREDICTOR::PREDICTOR(void) {

    ghr = 0;
    return;

}

bool PREDICTOR::GetPrediction(uint64_t PC) {

    index = (ghr ^ PC) & (numPhtEntries - 1);
    counter = pht.get_counter(index);

    return counter.Dir;

}

void PREDICTOR::UpdatePredictor(uint64_t PC, OpType opType, bool resolveDir, bool predDir, uint64_t branchTarget) {

    counter_t new_counter = counter.updateCounter(resolveDir);
    pht.save_counter(index, new_counter);

    ghr = ghr << 1;
    ghr |= resolveDir;

    return;

}

void PREDICTOR::TrackOtherInst(uint64_t PC, OpType opType, bool branchDir, uint64_t branchTarget) {

    return;

}

