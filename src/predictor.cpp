//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include <math.h>
#include "predictor.h"
#include <algorithm>
//
// TODO:Student Information
//
const char *studentName = "James Han";
const char *studentID = "A16781400";
const char *email = "jjhan@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = {"Static", "Gshare",
                         "Tournament", "Custom"};

// define number of bits required for indexing the BHT here.
int ghistoryBits = 15; // Number of bits used for Global History
int bpType;            // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
// TODO: Add your own Branch Predictor data structures here
//
// gshare
uint8_t *bht_gshare;
uint64_t ghistory;

// tournament
uint8_t ten_bits = 10; // for 1 << 10 = 1024 = 2^10
uint8_t twelve_bits = 12; // for 1 << 12 = 4096 = 2^12
uint16_t *bht_tournament;
uint8_t *bht_ctrs;
uint8_t *ght_tournament;
uint16_t global_tournament_history;
uint8_t *choice_tournament;

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//

// gshare functions
void init_gshare()
{
  int bht_entries = 1 << ghistoryBits;
  bht_gshare = (uint8_t *)malloc(bht_entries * sizeof(uint8_t));
  int i = 0;
  for (i = 0; i < bht_entries; i++)
  {
    bht_gshare[i] = WN;
  }
  ghistory = 0;
}

void init_tournament()
{
  int lht_entries = 1 << ten_bits; // 1024
  bht_tournament = (uint16_t *)malloc(lht_entries * sizeof(uint16_t)); // 16 * 1024 bits, holds 10 past outcomes, indexed by branch id
  bht_ctrs = (uint8_t *)malloc(lht_entries * sizeof(uint8_t)); // 8 * 1024 bits, holds 3 bit counters, indexed by bht pattern
  int ght_entries = 1 << twelve_bits; // 4096
  ght_tournament = (uint8_t *)malloc(ght_entries * sizeof(uint8_t)); // 8 * 4096 bits, holds 2 bit counters 
  choice_tournament = (uint8_t *)malloc(ght_entries * sizeof(uint8_t)); // 8 * 4096 bits, holds 2 bit counters

  int i = 0;
  for (i = 0; i < lht_entries; i++)
  {
    bht_tournament[i] = 0;
    bht_ctrs[i] = 3; // Weakly not taken
  }

  i = 0;
  for (i = 0; i < ght_entries; i++)
  {
    ght_tournament[i] = WN; // Weakly not taken
    choice_tournament[i] = WN; // Weakly not taken
  } 

  global_tournament_history = 0; // should be 12 bits of past global history, indexes into ght and choice

  // actual hardware budget usage
  /*
  
  bht_tournament = 10 * 1024 = 10 kb
  bht_ctr = 3 * 1024 = 3 kb
  ght_tournament = 2 * 4096 = 8 kb
  global_tournament_history = 12 bits
  choice_tournament = 2 * 4096 = 8kb

  total = 29kb + 12 bits

  budget = 64kb + 1024 bits
  */

}

uint8_t gshare_predict(uint32_t pc)
{
  // get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries - 1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries - 1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;
  switch (bht_gshare[index])
  {
  case WN:
    return NOTTAKEN;
  case SN:
    return NOTTAKEN;
  case WT:
    return TAKEN;
  case ST:
    return TAKEN;
  default:
    printf("Warning: Undefined state of entry in GSHARE BHT!\n");
    return NOTTAKEN;
  }
}

uint8_t tournament_predict(uint32_t pc){
  uint32_t bht_entries = 1 << ten_bits;
  uint32_t ght_entries = 1 << twelve_bits;
  uint32_t pc_lower_bits = pc & (bht_entries - 1);
  uint16_t global_tournament_history_lower_bits = global_tournament_history & (ght_entries - 1);

  uint8_t choice = choice_tournament[global_tournament_history_lower_bits];

  if (choice < 2){
    // use local
    uint16_t bht_pattern = bht_tournament[pc_lower_bits];
    uint16_t local_ctr = bht_ctrs[bht_pattern];

    if (local_ctr < 4){
      return NOTTAKEN;
    } else {
      return TAKEN;
    }
  } else {
    // use global
    uint8_t global_prediction = ght_tournament[global_tournament_history_lower_bits];
    if (global_prediction < 2){
      return NOTTAKEN;
    }  else {
      return TAKEN;
    }
  }

  return 0;
}


void train_gshare(uint32_t pc, uint8_t outcome)
{
  // get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries - 1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries - 1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;

  // Update state of entry in bht based on outcome
  switch (bht_gshare[index])
  {
  case WN:
    bht_gshare[index] = (outcome == TAKEN) ? WT : SN;
    break;
  case SN:
    bht_gshare[index] = (outcome == TAKEN) ? WN : SN;
    break;
  case WT:
    bht_gshare[index] = (outcome == TAKEN) ? ST : WN;
    break;
  case ST:
    bht_gshare[index] = (outcome == TAKEN) ? ST : WT;
    break;
  default:
    printf("Warning: Undefined state of entry in GSHARE BHT!\n");
    break;
  }

  // Update history register
  ghistory = ((ghistory << 1) | outcome);
}

void train_tournament(uint32_t pc, uint8_t outcome)
{
  uint32_t bht_entries = 1 << ten_bits;
  uint32_t pc_lower_bits = pc & (bht_entries - 1);
  uint32_t ght_entries = 1 << twelve_bits;
  uint32_t global_tournament_history_lower_bits = global_tournament_history & (ght_entries - 1);
  uint8_t choice = choice_tournament[global_tournament_history_lower_bits] < 2 ? LOCAL : GLOBAL;

  uint8_t bht_prediction = bht_ctrs[bht_tournament[pc_lower_bits]] > 3 ? TAKEN : NOTTAKEN;
  uint8_t ght_prediction = ght_tournament[global_tournament_history_lower_bits] > 1 ? TAKEN: NOTTAKEN;

  if (choice == LOCAL){
    // last used local
    if (outcome == TAKEN){
      if (bht_prediction == TAKEN){
        bht_ctrs[bht_tournament[pc_lower_bits]] = MIN(7, bht_ctrs[bht_tournament[pc_lower_bits]] + 1);
        if (choice_tournament[global_tournament_history_lower_bits] > 0) choice_tournament[global_tournament_history_lower_bits]--;
      } else {
        if (bht_ctrs[bht_tournament[pc_lower_bits]] > 0) bht_ctrs[bht_tournament[pc_lower_bits]]--;
        if (ght_prediction == TAKEN){
          choice_tournament[global_tournament_history_lower_bits] = MIN(3, choice_tournament[global_tournament_history_lower_bits] + 1);
        }
      }
      // update global prediction
      if (ght_prediction == TAKEN){
        ght_tournament[global_tournament_history_lower_bits] = MIN(3, ght_tournament[global_tournament_history_lower_bits] + 1);
      } else {
        if (ght_tournament[global_tournament_history_lower_bits] > 0) ght_tournament[global_tournament_history_lower_bits]--;
      }
    } else {
      // outcome was not taken
      if (bht_prediction == TAKEN){
        if (bht_ctrs[bht_tournament[pc_lower_bits]] > 0) bht_ctrs[bht_tournament[pc_lower_bits]]--;
        if (ght_prediction == NOTTAKEN){
          choice_tournament[global_tournament_history_lower_bits] = MIN(3, choice_tournament[global_tournament_history_lower_bits] + 1);
        }
      } else {
        bht_ctrs[bht_tournament[pc_lower_bits]] = MIN(7, bht_ctrs[bht_tournament[pc_lower_bits]] + 1);
        if (choice_tournament[global_tournament_history_lower_bits] > 0) choice_tournament[global_tournament_history_lower_bits]--;
      }
      // update global prediction
      if (ght_prediction == NOTTAKEN){
        ght_tournament[global_tournament_history_lower_bits] = MIN(3, ght_tournament[global_tournament_history_lower_bits] + 1);
      } else {
        if (ght_tournament[global_tournament_history_lower_bits] > 0) ght_tournament[global_tournament_history_lower_bits]--;
      }
    }
  } else {
    // last used global
    if (outcome == TAKEN){
      if (ght_prediction == TAKEN){
        ght_tournament[global_tournament_history_lower_bits] = MIN(3, ght_tournament[global_tournament_history_lower_bits] + 1);
        choice_tournament[global_tournament_history_lower_bits] = MIN(3, choice_tournament[global_tournament_history_lower_bits] + 1);
      } else {
        if (ght_tournament[global_tournament_history_lower_bits] > 0) ght_tournament[global_tournament_history_lower_bits]--;
        if (bht_prediction == TAKEN){
          if (choice_tournament[global_tournament_history_lower_bits] > 0) choice_tournament[global_tournament_history_lower_bits]--;
        }
      }
      // update local prediction
      if (bht_prediction == TAKEN){
        bht_ctrs[bht_tournament[pc_lower_bits]] = MIN(7, bht_ctrs[bht_tournament[pc_lower_bits]] + 1);
      } else {
        if (bht_ctrs[bht_tournament[pc_lower_bits]] > 0) bht_ctrs[bht_tournament[pc_lower_bits]]--;
      }
    } else {
      // outcome was not taken
     if (ght_prediction == TAKEN){
        if (ght_tournament[global_tournament_history_lower_bits] > 0) ght_tournament[global_tournament_history_lower_bits]--;
        if (bht_prediction == NOTTAKEN){
          if (choice_tournament[global_tournament_history_lower_bits] > 0) choice_tournament[global_tournament_history_lower_bits]--;
        }
      } else {
        ght_tournament[global_tournament_history_lower_bits] = MIN(3, ght_tournament[global_tournament_history_lower_bits] + 1);
        choice_tournament[global_tournament_history_lower_bits] = MIN(3, choice_tournament[global_tournament_history_lower_bits] + 1);
      }
      // update local prediction
      if (bht_prediction == NOTTAKEN){
        bht_ctrs[bht_tournament[pc_lower_bits]] = MIN(7, bht_ctrs[bht_tournament[pc_lower_bits]] + 1);
      } else {
        if (bht_ctrs[bht_tournament[pc_lower_bits]] > 0) bht_ctrs[bht_tournament[pc_lower_bits]]--;
      }
    }
  }

  bht_tournament[pc_lower_bits] = (bht_tournament[pc_lower_bits] << 1) | outcome;
  // update global_tournament_history
  global_tournament_history = ((global_tournament_history << 1) | outcome);
}


void cleanup_gshare()
{
  free(bht_gshare);
  bht_gshare = NULL;
}

void cleanup_tournament()
{
  free(bht_tournament);
  free(bht_ctrs);
  free(ght_tournament);
  free(choice_tournament);

  bht_tournament = NULL;
  bht_ctrs = NULL;
  ght_tournament = NULL;
  choice_tournament = NULL;

}

void init_predictor()
{
  switch (bpType)
  {
  case STATIC:
    break;
  case GSHARE:
    init_gshare();
    break;
  case TOURNAMENT:
    init_tournament();
    break;
  case CUSTOM:
    break;
  default:
    break;
  }
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint32_t make_prediction(uint32_t pc, uint32_t target, uint32_t direct)
{

  // Make a prediction based on the bpType
  switch (bpType)
  {
  case STATIC:
    return TAKEN;
  case GSHARE:
    return gshare_predict(pc);
  case TOURNAMENT:
    return tournament_predict(pc);
  case CUSTOM:
    return NOTTAKEN;
  default:
    break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//

void train_predictor(uint32_t pc, uint32_t target, uint32_t outcome, uint32_t condition, uint32_t call, uint32_t ret, uint32_t direct)
{
  if (condition)
  {
    switch (bpType)
    {
    case STATIC:
      return;
    case GSHARE:
      return train_gshare(pc, outcome);
    case TOURNAMENT:
      return train_tournament(pc, outcome);
    case CUSTOM:
      return;
    default:
      break;
    }
  }
}
