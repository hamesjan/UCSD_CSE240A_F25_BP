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

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "tage_state.h" 
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


/*
custom
*/

// Storage budget 64KB -> 2^16 bits

uint64_t tage_ght[4];
uint16_t tage_pht; // 16 bit, will record 1 address bit for each address. add LSB of PC

uint16_t t0_entries = 1 << 12; // 2 ^ 16 - 4
uint16_t tx_entries = 1 << 10; // 2 ^ 16 - 6 
uint8_t num_components = 4;

uint8_t *tage_t0; // 4096 entries, bimodal 
uint16_t *tage_component_tags;
uint16_t *tage_components[4];  // 1024 entries each, // tag width - 9 bits
uint8_t *tage_pred_ctrs; // 3 bit, MSB provides prediction
uint8_t *tage_pred_ctrs_component[4];
uint8_t *tage_useful_ctrs; // 2bit, unsigned. 
uint8_t *tage_useful_component[4];

uint8_t L1 = 2;
uint8_t alpha = 2;
uint8_t L2 = 0;
uint8_t L3 = 0;
uint8_t L4 = 0;

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
  choice_tournament = 2 * 4096 = 8kb
  global_tournament_history = 12 bits

  total = 29kb + 12 bits

  budget = 64kb + 1024 bits
  */

}

void init_custom()
{

  /*
// Storage budget 64KB -> 2^16 bits
uint64_t tage_ght[4];
uint16_t tage_pht; // 16 bit, will record 1 address bit for each address. add LSB of PC
uint16_t t0_entries = 1 << 12; // 2 ^ 16 - 4
uint16_t tx_entries = 1 << 10; // 2 ^ 16 - 6 
uint8_t *tage_t0; // 4096 entries, bimodal 
uint16_t *tage_component_tags;
uint16_t *tage_components[4];  // 1024 entries each, // tag width - 9 bits
uint8_t *tage_pred_ctrs; // 3 bit, MSB provides prediction
uint8_t *tage_ored_ctrs_component[4];
uint8_t *tage_useful_ctrs // 2bit, unsigned. 
uint8_t *tage_useful_component[4];

  */

  tage_t0 = (uint8_t *)malloc(t0_entries * sizeof(uint8_t));  
  tage_component_tags = (uint16_t *)malloc(tx_entries * num_components * sizeof(uint16_t)); // 5 component tage predictor, 4 predictor tables + t0
  tage_pred_ctrs = (uint8_t *)malloc(tx_entries * num_components * sizeof(uint8_t));
  tage_useful_ctrs = (uint8_t *)malloc(tx_entries * num_components * sizeof(uint8_t));

  for (int i = 0; i < num_components; i++) {
    tage_components[i] = tage_component_tags + (i * tx_entries);
    tage_pred_ctrs_component[i] = tage_pred_ctrs + (i * tx_entries);
    tage_useful_component[i] = tage_useful_ctrs + (i * tx_entries);
    tage_ght[i] = 0;
  }

  // bimodal base
  for (int i = 0; i < t0_entries; i++)
  {
    tage_t0[i] = WN;
  }

  // setting L values for all predictor components
  L2 = (uint8_t)round(L1 * pow(alpha, 1)); 
  L3 = (uint8_t)round(L1 * pow(alpha, 2));
  L4 = (uint8_t)round(L1 * pow(alpha, 3));
  
  tage_pht = 0;
  // on init, load previous tage state stored in tage_state.h
  #if defined(TAGE_STATE_VALID)
    // If tage_state.h exists and defines this macro, load previous state
    memcpy(tage_t0, tage_t0_saved, t0_entries);
    memcpy(tage_component_tags, tage_tags_saved, num_components * tx_entries * sizeof(uint16_t));
    memcpy(tage_pred_ctrs, tage_pred_saved, num_components * tx_entries);
    memcpy(tage_useful_ctrs, tage_useful_saved, num_components * tx_entries);
    memcpy(tage_ght, tage_ght_saved, sizeof(uint64_t) * num_components);
    tage_pht = tage_pht_saved;
  #else
      // Else initialize fresh
      memset(tage_t0, 0, t0_entries);
      memset(tage_component_tags, 0, num_components * tx_entries * sizeof(uint16_t));
      memset(tage_pred_ctrs, 0, num_components * tx_entries);
      memset(tage_useful_ctrs, 0, num_components * tx_entries);
      memset(tage_ght, 0, sizeof(uint64_t) * num_components);
      tage_pht = 0;
  #endif

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



uint16_t pred_hash_idx(uint32_t pc, int L) {
    const int idx_bits = 10; // 1024 entries, so need 10 bits for idx
    uint16_t folded = 0;
    int pos = 0;

    // XOR-fold the last L bits of the global history
    for (int i = 0; i < L; i++) {
        int word = i / 64;
        int offset = i % 64;
        uint8_t bit = (tage_ght[word] >> offset) & 1ULL;
        folded ^= (bit << (pos % idx_bits));
        pos++;
    }
    // Hash with PC
    uint16_t pc_lower_ten = pc & ((1 << idx_bits) - 1);
    uint16_t index = (folded ^ pc_lower_ten) & ((1 << idx_bits) - 1); // mask
    return index;
}

uint16_t pred_hash_tag(uint32_t pc, int L) {
    const int tag_bits = 9; // 1024 entries, so need 10 bits for idx
    uint16_t folded = 0;
    int pos = 0;
    // XOR-fold the last L bits of the global history
    for (int i = 0; i < L; i++) {
        int word = i / 64;
        int offset = i % 64;
        uint8_t bit = (tage_ght[word] >> offset) & 0x01;
        folded ^= (bit << (pos % tag_bits));
        pos++;
    }
    // Hash with PC
    uint16_t pc_lower_nine = pc & ((1 << tag_bits) - 1);
    uint16_t index = (folded ^ pc_lower_nine) & ((1 << tag_bits) - 1); // mask
    return index;
}

uint8_t custom_predict(uint32_t pc){
  uint16_t t0_idx = pc & (t0_entries - 1);

  uint16_t t1_idx = pred_hash_idx(pc, L1);
  uint16_t t2_idx = pred_hash_idx(pc, L2);
  uint16_t t3_idx = pred_hash_idx(pc, L3);
  uint16_t t4_idx = pred_hash_idx(pc, L4);

  uint16_t t1_tag = pred_hash_tag(pc, L1);
  uint16_t t2_tag = pred_hash_tag(pc, L2);
  uint16_t t3_tag = pred_hash_tag(pc, L3);
  uint16_t t4_tag = pred_hash_tag(pc, L4);


  uint16_t idx_list[4] = {t1_idx, t2_idx, t3_idx, t4_idx};
  uint16_t tag_list[4] = {t1_tag, t2_tag, t3_tag, t4_tag};

  // walk down
  int provider_component = -1;
  for (int i = num_components - 1; i >= 0; i--){
    if (tage_components[i][idx_list[i]] == tag_list[i]) {
        provider_component = i;
        break; 
    }
  }

  // use base predictor
  if (provider_component < 0){
    if (tage_t0[t0_idx] < 2){
      return NOTTAKEN;
    } else {
      return TAKEN;
    }
  } else {
    // use chosen predictor
    if (tage_pred_ctrs_component[provider_component][idx_list[provider_component]] < 4){
      return NOTTAKEN;
    } else {
      return TAKEN;
    }

  }


  return NOTTAKEN;
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

  // update predictors
  if (outcome == TAKEN){
    if (bht_ctrs[bht_tournament[pc_lower_bits]] < 7){
        bht_ctrs[bht_tournament[pc_lower_bits]]++;
    }
    if (ght_tournament[global_tournament_history_lower_bits] < 3){
         ght_tournament[global_tournament_history_lower_bits]++;
      }
  } else {
    if (bht_ctrs[bht_tournament[pc_lower_bits]] > 0){
        bht_ctrs[bht_tournament[pc_lower_bits]]--;
      }
    if (ght_tournament[global_tournament_history_lower_bits] > 0){
        ght_tournament[global_tournament_history_lower_bits]--;
      }
  }

  // update choice
  if (bht_prediction != ght_prediction){
    if (outcome == bht_prediction){
      if (choice_tournament[global_tournament_history_lower_bits] > 0){
        choice_tournament[global_tournament_history_lower_bits]--;
      }

    } else {
      if (choice_tournament[global_tournament_history_lower_bits] < 3){
        choice_tournament[global_tournament_history_lower_bits]++;
      }
    }
  }

  bht_tournament[pc_lower_bits] = (bht_tournament[pc_lower_bits] << 1) | outcome;
  // update global_tournament_history
  global_tournament_history = ((global_tournament_history << 1) | outcome);
}

void train_custom(uint32_t pc, uint8_t outcome)
{
  return;
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

void cleanup_custom() {
    FILE *fp = fopen("tage_state.h", "w");
    if (!fp) return;

    fprintf(fp, "#ifndef __TAGE_STATE_H__\n#define __TAGE_STATE_H__\n");
    fprintf(fp, "#define TAGE_STATE_VALID\n\n");

    fprintf(fp, "static uint8_t tage_t0_saved[%d] = {", 1 << 12);
    for (int i = 0; i < (1 << 12); i++)
        fprintf(fp, "%u,", tage_t0[i]);
    fprintf(fp, "};\n\n");

    fprintf(fp, "static uint16_t tage_tags_saved[%d] = {", num_components * (1 << 10));
    for (int i = 0; i < num_components * (1 << 10); i++)
        fprintf(fp, "%u,", tage_component_tags[i]);
    fprintf(fp, "};\n\n");

    fprintf(fp, "static uint8_t tage_pred_saved[%d] = {", num_components * (1 << 10));
    for (int i = 0; i < num_components * (1 << 10); i++)
        fprintf(fp, "%u,", tage_pred_ctrs[i]);
    fprintf(fp, "};\n\n");

    fprintf(fp, "static uint8_t tage_useful_saved[%d] = {", num_components * (1 << 10));
    for (int i = 0; i < num_components * (1 << 10); i++)
        fprintf(fp, "%u,", tage_useful_ctrs[i]);
    fprintf(fp, "};\n\n");

    fprintf(fp, "static uint64_t tage_ght_saved[%d] = {", 4);
    for (int i = 0; i < 4; i++)
        fprintf(fp, "%llu,", (unsigned long long)tage_ght[i]);
    fprintf(fp, "};\n\n");

    fprintf(fp, "static uint16_t tage_pht_saved = %u;\n", tage_pht);

    fprintf(fp, "\n#endif\n");
    fclose(fp);

    free(tage_t0);
    free(tage_component_tags);
    free(tage_pred_ctrs);
    free(tage_useful_ctrs);
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
    init_custom();
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
    return custom_predict(pc);
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
      return train_custom(pc, outcome);
    default:
      break;
    }
  }
}
