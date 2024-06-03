///////////////////////////////////////////////////////////////////////
//  Copyright 2015 Samsung Austin Semiconductor, LLC.                //
///////////////////////////////////////////////////////////////////////

#include <map>

#include "utils.h"

#include "bt9_reader.h"

#include "../build/paths.h"
#include PREDICTOR_H_PATH

#include <cereal/archives/json.hpp>

#include <cereal/archives/json.hpp>
#include <cereal/types/array.hpp>

void CheckHeartBeat(uint64_t numIter, uint64_t numMispred)
{

  const uint64_t d1K   =1000;
  const uint64_t d10K  =10000;
  const uint64_t d100K =100000;
  const uint64_t d1M   =1000000;
  const uint64_t d10M  =10000000;
  const uint64_t d30M  =30000000;
  const uint64_t d60M  =60000000;
  const uint64_t d100M =100000000;
  const uint64_t d300M =300000000;
  const uint64_t d600M =600000000;
  const uint64_t d1B   =1000000000;
  const uint64_t d10B  =10000000000;

  if(numIter == d1K){
    stats.MPKBr_1K = 1000.0*(double)(numMispred)/(double)(numIter);
  }

  if(numIter == d10K){
    stats.MPKBr_10K = 1000.0*(double)(numMispred)/(double)(numIter);
  }

  if(numIter == d100K){
    stats.MPKBr_100K = 1000.0*(double)(numMispred)/(double)(numIter);
  }

  if(numIter == d1M){
    stats.MPKBr_1M = 1000.0*(double)(numMispred)/(double)(numIter);
  }

  if(numIter == d10M){
    stats.MPKBr_10M = 1000.0*(double)(numMispred)/(double)(numIter);
  }

  if(numIter == d30M){
    stats.MPKBr_30M = 1000.0*(double)(numMispred)/(double)(numIter);
  }

  if(numIter == d60M){
    stats.MPKBr_60M = 1000.0*(double)(numMispred)/(double)(numIter);
  }

  if(numIter == d100M){
    stats.MPKBr_100M = 1000.0*(double)(numMispred)/(double)(numIter);
  }

  if(numIter == d300M){
    stats.MPKBr_300M = 1000.0*(double)(numMispred)/(double)(numIter);
  }

  if(numIter == d600M){
    stats.MPKBr_600M = 1000.0*(double)(numMispred)/(double)(numIter);
  }

  if(numIter == d1B){
    stats.MPKBr_1B = 1000.0*(double)(numMispred)/(double)(numIter);
  }

  if(numIter == d10B){
    stats.MPKBr_10B = 1000.0*(double)(numMispred)/(double)(numIter);
  }

}

// usage: predictor <trace>

int main(int argc, char* argv[]){

  if (argc != 2) {
    printf("usage: %s <trace>\n", argv[0]);
    exit(-1);
  }

  ///////////////////////////////////////////////
  // Init variables
  ///////////////////////////////////////////////

    PREDICTOR  brpred = PREDICTOR();  // this instantiates the predictor code
  ///////////////////////////////////////////////
  // read each trace recrod, simulate until done
  ///////////////////////////////////////////////

    std::string trace_path;
    trace_path = argv[1];
    bt9::BT9Reader bt9_reader(trace_path);

    std::string key = "total_instruction_count:";
    std::string value;
    bt9_reader.header.getFieldValueStr(key, value);
    uint64_t     total_instruction_counter = std::stoull(value, nullptr, 0);
    uint64_t current_instruction_counter = 0;
    key = "branch_instruction_count:";
    bt9_reader.header.getFieldValueStr(key, value);
    uint64_t     branch_instruction_counter = std::stoull(value, nullptr, 0);
    uint64_t     numMispred =0;
//ver2    uint64_t     numMispred_btbMISS =0;
//ver2    uint64_t     numMispred_btbANSF =0;
//ver2    uint64_t     numMispred_btbATSF =0;
//ver2    uint64_t     numMispred_btbDYN =0;

    uint64_t cond_branch_instruction_counter=0;
//ver2     uint64_t btb_ansf_cond_branch_instruction_counter=0;
//ver2     uint64_t btb_atsf_cond_branch_instruction_counter=0;
//ver2     uint64_t btb_dyn_cond_branch_instruction_counter=0;
//ver2     uint64_t btb_miss_cond_branch_instruction_counter=0;
           uint64_t uncond_branch_instruction_counter=0;

//ver2    ///////////////////////////////////////////////
//ver2    // model simple branch marking structure
//ver2    ///////////////////////////////////////////////
//ver2    std::map<uint64_t, uint32_t> myBtb;
//ver2    std::map<uint64_t, uint32_t>::iterator myBtbIterator;
//ver2
//ver2    myBtb.clear();

  ///////////////////////////////////////////////
  // read each trace record, simulate until done
  ///////////////////////////////////////////////

      OpType opType;
      uint64_t PC;
      bool branchTaken;
      uint64_t branchTarget;

      uint64_t numIter = 0;
      uint64_t CheckHeartBeat_numIter = 0;
      const uint64_t CheckHeartBeat_interval = 1000;


      for (auto it = bt9_reader.begin(); it != bt9_reader.end(); ++it) {
        numIter++;
        CheckHeartBeat_numIter++;
        if(CheckHeartBeat_numIter == CheckHeartBeat_interval){
          CheckHeartBeat(numIter, numMispred); //Here numIter will be equal to number of branches read
          CheckHeartBeat_numIter = 0;
        }

        try {
          bt9::BrClass_BrBehavior BrClass_BrBehavior = it->getSrcNode()->brClass_brBehavior();

//          bool dirDynamic = (it->getSrcNode()->brObservedTakenCnt() > 0) && (it->getSrcNode()->brObservedNotTakenCnt() > 0); //JD2_2_2016
//          bool dirNeverTkn = (it->getSrcNode()->brObservedTakenCnt() == 0) && (it->getSrcNode()->brObservedNotTakenCnt() > 0); //JD2_2_2016

//JD2_2_2016 break down branch instructions into all possible types
          opType = OPTYPE_ERROR;

          if ((BrClass_BrBehavior.type == bt9::BrClass::Type::UNKNOWN) && (it->getSrcNode()->brNodeIndex())) { //only fault if it isn't the first node in the graph (fake branch)
            opType = OPTYPE_ERROR; //sanity check
          }
//NOTE unconditional could be part of an IT block that is resolved not-taken
//          else if (dirNeverTkn && (br_class.conditionality == bt9::BrClass::Conditionality::UNCONDITIONAL)) {
//            opType = OPTYPE_ERROR; //sanity check
//          }
//JD_2_22 There is a bug in the instruction decoder used to generate the traces
//          else if (dirDynamic && (br_class.conditionality == bt9::BrClass::Conditionality::UNCONDITIONAL)) {
//            opType = OPTYPE_ERROR; //sanity check
//          }
          else if (BrClass_BrBehavior.type == bt9::BrClass::Type::RET) {
            if (BrClass_BrBehavior.conditionality == bt9::BrClass::Conditionality::CONDITIONAL)
              opType = OPTYPE_RET_COND;
            else if (BrClass_BrBehavior.conditionality == bt9::BrClass::Conditionality::UNCONDITIONAL)
              opType = OPTYPE_RET_UNCOND;
            else {
              opType = OPTYPE_ERROR;
            }
          }
          else if (BrClass_BrBehavior.directness == bt9::BrClass::Directness::INDIRECT) {
            if (BrClass_BrBehavior.type == bt9::BrClass::Type::CALL) {
              if (BrClass_BrBehavior.conditionality == bt9::BrClass::Conditionality::CONDITIONAL)
                opType = OPTYPE_CALL_INDIRECT_COND;
              else if (BrClass_BrBehavior.conditionality == bt9::BrClass::Conditionality::UNCONDITIONAL)
                opType = OPTYPE_CALL_INDIRECT_UNCOND;
              else {
                opType = OPTYPE_ERROR;
              }
            }
            else if (BrClass_BrBehavior.type == bt9::BrClass::Type::JMP) {
              if (BrClass_BrBehavior.conditionality == bt9::BrClass::Conditionality::CONDITIONAL)
                opType = OPTYPE_JMP_INDIRECT_COND;
              else if (BrClass_BrBehavior.conditionality == bt9::BrClass::Conditionality::UNCONDITIONAL)
                opType = OPTYPE_JMP_INDIRECT_UNCOND;
              else {
                opType = OPTYPE_ERROR;
              }
            }
            else {
              opType = OPTYPE_ERROR;
            }
          }
          else if (BrClass_BrBehavior.directness == bt9::BrClass::Directness::DIRECT) {
            if (BrClass_BrBehavior.type == bt9::BrClass::Type::CALL) {
              if (BrClass_BrBehavior.conditionality == bt9::BrClass::Conditionality::CONDITIONAL) {
                opType = OPTYPE_CALL_DIRECT_COND;
              }
              else if (BrClass_BrBehavior.conditionality == bt9::BrClass::Conditionality::UNCONDITIONAL) {
                opType = OPTYPE_CALL_DIRECT_UNCOND;
              }
              else {
                opType = OPTYPE_ERROR;
              }
            }
            else if (BrClass_BrBehavior.type == bt9::BrClass::Type::JMP) {
              if (BrClass_BrBehavior.conditionality == bt9::BrClass::Conditionality::CONDITIONAL) {
                opType = OPTYPE_JMP_DIRECT_COND;
              }
              else if (BrClass_BrBehavior.conditionality == bt9::BrClass::Conditionality::UNCONDITIONAL) {
                opType = OPTYPE_JMP_DIRECT_UNCOND;
              }
              else {
                opType = OPTYPE_ERROR;
              }
            }
            else {
              opType = OPTYPE_ERROR;
            }
          }
          else {
            opType = OPTYPE_ERROR;
          }


          PC = it->getSrcNode()->brVirtualAddr();

          branchTaken = it->getEdge()->isTakenPath();
          branchTarget = it->getEdge()->brVirtualTarget();

          //printf("PC: %llx type: %x T %d N %d outcome: %d", PC, (uint32_t)opType, it->getSrcNode()->brObservedTakenCnt(), it->getSrcNode()->brObservedNotTakenCnt(), branchTaken);

/************************************************************************************************************/

          if (opType == OPTYPE_ERROR) {
            if (it->getSrcNode()->brNodeIndex()) { //only fault if it isn't the first node in the graph (fake branch)
              fprintf(stderr, "OPTYPE_ERROR\n");
              printf("OPTYPE_ERROR\n");
              exit(-1); //this should never happen, if it does please email CBP org chair.
            }
          }
          else if (BrClass_BrBehavior.conditionality == bt9::BrClass::Conditionality::CONDITIONAL) { //JD2_17_2016 call UpdatePredictor() for all branches that decode as conditional
            //printf("COND ");

//NOTE: contestants are NOT allowed to use the btb* information from ver2 of the infrastructure below:
//ver2             myBtbIterator = myBtb.find(PC); //check BTB for a hit
//ver2            bool btbATSF = false;
//ver2            bool btbANSF = false;
//ver2            bool btbDYN = false;
//ver2
//ver2            if (myBtbIterator == myBtb.end()) { //miss -> we have no history for the branch in the marking structure
//ver2              //printf("BTB miss ");
//ver2              myBtb.insert(pair<uint64_t, uint32_t>(PC, (uint32_t)branchTaken)); //on a miss insert with outcome (N->btbANSF, T->btbATSF)
//ver2              predDir = brpred->GetPrediction(PC, btbANSF, btbATSF, btbDYN);
//ver2              brpred->UpdatePredictor(PC, opType, branchTaken, predDir, branchTarget, btbANSF, btbATSF, btbDYN);
//ver2            }
//ver2            else {
//ver2              btbANSF = (myBtbIterator->second == 0);
//ver2              btbATSF = (myBtbIterator->second == 1);
//ver2              btbDYN = (myBtbIterator->second == 2);
//ver2              //printf("BTB hit ANSF: %d ATSF: %d DYN: %d ", btbANSF, btbATSF, btbDYN);
//ver2
//ver2              predDir = brpred->GetPrediction(PC, btbANSF, btbATSF, btbDYN);
//ver2              brpred->UpdatePredictor(PC, opType, branchTaken, predDir, branchTarget, btbANSF, btbATSF, btbDYN);
//ver2
//ver2              if (  (btbANSF && branchTaken)   // only exhibited N until now and we just got a T -> upgrade to dynamic conditional
//ver2                 || (btbATSF && !branchTaken)  // only exhibited T until now and we just got a N -> upgrade to dynamic conditional
//ver2                 ) {
//ver2                myBtbIterator->second = 2; //2-> dynamic conditional (has exhibited both taken and not-taken in the past)
//ver2              }
//ver2            }
//ver2            //puts("");

            bool predDir = false;

            predDir = brpred.GetPrediction(PC);
            brpred.UpdatePredictor(PC, opType, branchTaken, predDir, branchTarget);

            if(predDir != branchTaken){
              numMispred++; // update mispred stats
//ver2              if(btbATSF)
//ver2                numMispred_btbATSF++; // update mispred stats
//ver2              else if(btbANSF)
//ver2                numMispred_btbANSF++; // update mispred stats
//ver2              else if(btbDYN)
//ver2                numMispred_btbDYN++; // update mispred stats
//ver2              else
//ver2                numMispred_btbMISS++; // update mispred stats
            }
            cond_branch_instruction_counter++;

//ver2            if (btbDYN)
//ver2              btb_dyn_cond_branch_instruction_counter++; //number of branches that have been N at least once after being T at least once
//ver2            else if (btbATSF)
//ver2              btb_atsf_cond_branch_instruction_counter++; //number of branches that have been T at least once, but have not yet seen a N after the first T
//ver2            else if (btbANSF)
//ver2              btb_ansf_cond_branch_instruction_counter++; //number of cond branches that have not yet been observed T
//ver2            else
//ver2              btb_miss_cond_branch_instruction_counter++; //number of cond branches that have not yet been observed T
          }
          else if (BrClass_BrBehavior.conditionality == bt9::BrClass::Conditionality::UNCONDITIONAL) { // for predictors that want to track unconditional branches
            uncond_branch_instruction_counter++;
            brpred.TrackOtherInst(PC, opType, branchTaken, branchTarget);
          }
          else {
            fprintf(stderr, "CONDITIONALITY ERROR\n");
            printf("CONDITIONALITY ERROR\n");
            exit(-1); //this should never happen, if it does please email CBP org chair.
          }

/************************************************************************************************************/
        }
        catch (const std::out_of_range & ex) {
          std::cout << ex.what() << '\n';
          break;
        }

      } //for (auto it = bt9_reader.begin(); it != bt9_reader.end(); ++it)


    ///////////////////////////////////////////
    //print_stats
    ///////////////////////////////////////////

    std::string filename = trace_path.substr(trace_path.find_last_of("/\\") + 1);

    std::string::size_type const first_dot = filename.find_first_of('.');
    std::string filename_without_extension = filename.substr(0, first_dot);

    stats.NUM_INSTRUCTIONS = total_instruction_counter;
    stats.NUM_BR = branch_instruction_counter-1; //JD2_2_2016 NOTE there is a dummy branch at the beginning of the trace...
    stats.NUM_UNCOND_BR = uncond_branch_instruction_counter;
    stats.NUM_CONDITIONAL_BR = cond_branch_instruction_counter;
    stats.NUM_MISPREDICTIONS = numMispred;
    stats.MISPRED_PER_1K_INST = 1000.0*(double)(numMispred)/(double)(total_instruction_counter);
    stats.TRACE = filename_without_extension;

    cereal::JSONOutputArchive archive(std::cout);
    archive(cereal::make_nvp(filename_without_extension.c_str(), stats));

 /*
        uint64_t bimodalUsed, bimodalCorrect;
        std::array<uint64_t, 1 << BIM_META_WIDTH> bimodal_used_arr;
        std::array<uint64_t, 1 << BIM_META_WIDTH> bimodal_correct_arr;

        uint64_t bigskewUsed, bigskewCorrect;
        uint64_t bigskew_used_arr[1<<BIM_META_WIDTH][1<<PHT_WIDTH][1<<PHT_WIDTH][1<<PHT_WIDTH][1<<PHT_WIDTH];
        uint64_t bigskew_pred1_arr[1<<BIM_META_WIDTH][1<<PHT_WIDTH][1<<PHT_WIDTH][1<<PHT_WIDTH][1<<PHT_WIDTH];
        uint64_t bigskew_was_correct_arr[1<<BIM_META_WIDTH][1<<PHT_WIDTH][1<<PHT_WIDTH][1<<PHT_WIDTH][1<<PHT_WIDTH];

        uint64_t correct_pred_arr[1<<BIM_META_WIDTH][1<<PHT_WIDTH][1<<PHT_WIDTH][1<<PHT_WIDTH][1<<PHT_WIDTH];
*/
//ver2      printf("  NUM_CONDITIONAL_BR_BTB_MISS \t : %10llu",   btb_miss_cond_branch_instruction_counter);
//ver2      printf("  NUM_CONDITIONAL_BR_BTB_ANSF \t : %10llu",   btb_ansf_cond_branch_instruction_counter);
//ver2      printf("  NUM_CONDITIONAL_BR_BTB_ATSF \t : %10llu",   btb_atsf_cond_branch_instruction_counter);
//ver2      printf("  NUM_CONDITIONAL_BR_BTB_DYN  \t : %10llu",   btb_dyn_cond_branch_instruction_counter);
//ver2      printf("  NUM_MISPREDICTIONS_BTB_MISS \t : %10llu",   numMispred_btbMISS);
//ver2      printf("  NUM_MISPREDICTIONS_BTB_ANSF \t : %10llu",   numMispred_btbANSF);
//ver2      printf("  NUM_MISPREDICTIONS_BTB_ATSF \t : %10llu",   numMispred_btbATSF);
//ver2      printf("  NUM_MISPREDICTIONS_BTB_DYN  \t : %10llu",   numMispred_btbDYN);
//ver2      printf("  MISPRED_PER_1K_INST_BTB_MISS\t : %10.4f",   1000.0*(double)(numMispred_btbMISS)/(double)(total_instruction_counter));
//ver2      printf("  MISPRED_PER_1K_INST_BTB_ANSF\t : %10.4f",   1000.0*(double)(numMispred_btbANSF)/(double)(total_instruction_counter));
//ver2      printf("  MISPRED_PER_1K_INST_BTB_ATSF\t : %10.4f",   1000.0*(double)(numMispred_btbATSF)/(double)(total_instruction_counter));
//ver2      printf("  MISPRED_PER_1K_INST_BTB_DYN \t : %10.4f",   1000.0*(double)(numMispred_btbDYN)/(double)(total_instruction_counter));
}

