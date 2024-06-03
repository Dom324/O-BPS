#include "predictor.h"

stats_t stats;

PREDICTOR::PREDICTOR(void) {

    return;

}

bool PREDICTOR::GetPrediction(uint64_t PC) {

    index = PC & (numPhtEntries - 1);
    counter = pht.get_counter(index);

    return counter.Dir;

}

void PREDICTOR::UpdatePredictor(uint64_t PC, OpType opType, bool resolveDir, bool predDir, uint64_t branchTarget) {

    counter_t new_counter = counter.updateCounter(resolveDir);
    pht.save_counter(index, new_counter);

    return;

}

void PREDICTOR::TrackOtherInst(uint64_t PC, OpType opType, bool branchDir, uint64_t branchTarget) {

    return;

}

