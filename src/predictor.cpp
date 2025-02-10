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

//
// TODO:Student Information
//
const char *studentName = "Ozgur";
const char *studentID = "A69033985";
const char *email = "oozerdem@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = {"Static", "Gshare",
                         "Tournament", "Custom"};

// define number of bits required for indexing the BHT here.
int ghistoryBits = 17; // Number of bits used for Global History
int bpType;            // Branch Prediction Type
int verbose;

int tournement_global_bits=16;
int tournement_local_bits=13;




//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
// TODO: Add your own Branch Predictor data structures here
//
// gshare
uint8_t *bht_gshare;


// tournament


uint16_t *pht_tournament_local;
uint8_t *bht_tournament_local;
uint8_t *bht_tournament_corele;
uint8_t *bht_tournament_choice;


int custom_base_bits = 16;  // # of bits for the base predictor index
int tag_bits = 29;          // # of bits in the partial tag
int decrement = 5;          // for shrinking the table size, 1*decrement

// Global arrays
uint8_t *bht_custom_base;    // base predictor 2-bit counters
uint8_t *bht_custom_stage1;  // tagged predictor 2-bit counters (2 ways)
__int128_t *tag_custom_stage1; // partial tags for the 2-way table

__int128_t ghistory; // global history register



void init_custom()
{
    int entries_base   = 1 << custom_base_bits;           // 2^16
    int entries_stage1 = 1 << (custom_base_bits - decrement); // e.g. 2^(16-4) = 2^12 = 4096

    // Allocate
    bht_custom_base = (uint8_t*) malloc(entries_base * sizeof(uint8_t));
    bht_custom_stage1 = (uint8_t*) malloc(entries_stage1 * 2 * sizeof(uint8_t));
    tag_custom_stage1 = (__int128_t*) malloc(entries_stage1 * 2 * sizeof(__int128_t));

    // Initialize
    for (int i = 0; i < entries_base; i++) {
        bht_custom_base[i] = WN;  // weakly not taken
    }
    for (int i = 0; i < (entries_stage1 * 2); i++) {
        bht_custom_stage1[i] = WN;   // weakly not taken
        tag_custom_stage1[i] = 0;    // partial tag = 0
    }
    ghistory = 0;
}

uint8_t custom_predict(uint32_t pc)
{
    int entries_base   = 1 << custom_base_bits;
    int entries_stage1 = 1 << (custom_base_bits - decrement);

    // Compute set index from lower bits of (pc ^ ghistory)
    uint32_t index_base   = (pc ^ ghistory) & (entries_base - 1);
    uint32_t set_index    = (pc ^ ghistory) & (entries_stage1 - 1);

    // Two ways: idx0, idx1
    uint32_t idx0 = 2 * set_index;
    uint32_t idx1 = 2 * set_index + 1;

    // Compute partial tag from higher bits (some TAGE-like scheme)
    // For a 14-bit partial tag:
    //   (pc ^ ghistory) >> (custom_base_bits - decrement)
    //   custom_base_bits = 16, decrement = 4 => shift = 16 - 4 = 12
    __int128_t partial_tag = ((pc ^ ghistory) >> (custom_base_bits - decrement)) & ((1 << tag_bits) - 1); 
    // i.e., shift right by 12, then mask 14 bits

    // Check if either way matches the partial tag
    uint8_t prediction;
    if (tag_custom_stage1[idx0] == partial_tag) {
        prediction = bht_custom_stage1[idx0];
    }
    else if (tag_custom_stage1[idx1] == partial_tag) {
        prediction = bht_custom_stage1[idx1];
    }
    else {
        prediction = bht_custom_base[index_base];
    }

    // Convert 2-bit state to TAKEN / NOTTAKEN
    if (prediction == ST || prediction == WT) {
        return TAKEN;
    } else {
        return NOTTAKEN;
    }
}

void train_custom(uint32_t pc, uint8_t outcome)
{
    int entries_base   = 1 << custom_base_bits;
    int entries_stage1 = 1 << (custom_base_bits - decrement);

    // Indices
    uint32_t index_base = (pc ^ ghistory) & (entries_base - 1);
    uint32_t set_index  = (pc ^ ghistory) & (entries_stage1 - 1);

    uint32_t idx0 = 2 * set_index;
    uint32_t idx1 = 2 * set_index + 1;

    // Compute partial tag using the OLD ghistory (before shifting)
    __int128_t partial_tag = ((pc ^ ghistory) >> (16 - 4)) & ((1 << tag_bits) - 1); 

    // First figure out whose prediction was used
    uint8_t pred;
    int which_way = -1; // 0 or 1 if matched way 0 or way 1, otherwise -1 (used base)

    if (tag_custom_stage1[idx0] == partial_tag) {
        pred       = bht_custom_stage1[idx0];
        which_way  = 0;
    }
    else if (tag_custom_stage1[idx1] == partial_tag) {
        pred       = bht_custom_stage1[idx1];
        which_way  = 1;
    }
    else {
        pred       = bht_custom_base[index_base];
    }

    // Update the 2-bit counter for whichever way matched
    if (which_way == 0) {
        switch (bht_custom_stage1[idx0]) {
        case WN: bht_custom_stage1[idx0] = (outcome == TAKEN) ? WT : SN; break;
        case SN: bht_custom_stage1[idx0] = (outcome == TAKEN) ? WN : SN; break;
        case WT: bht_custom_stage1[idx0] = (outcome == TAKEN) ? ST : WN; break;
        case ST: bht_custom_stage1[idx0] = (outcome == TAKEN) ? ST : WT; break;
        }
    }
    else if (which_way == 1) {
        switch (bht_custom_stage1[idx1]) {
        case WN: bht_custom_stage1[idx1] = (outcome == TAKEN) ? WT : SN; break;
        case SN: bht_custom_stage1[idx1] = (outcome == TAKEN) ? WN : SN; break;
        case WT: bht_custom_stage1[idx1] = (outcome == TAKEN) ? ST : WN; break;
        case ST: bht_custom_stage1[idx1] = (outcome == TAKEN) ? ST : WT; break;
        }
    }

    // Always update base predictor
    switch (bht_custom_base[index_base]) {
    case WN: bht_custom_base[index_base] = (outcome == TAKEN) ? WT : SN; break;
    case SN: bht_custom_base[index_base] = (outcome == TAKEN) ? WN : SN; break;
    case WT: bht_custom_base[index_base] = (outcome == TAKEN) ? ST : WN; break;
    case ST: bht_custom_base[index_base] = (outcome == TAKEN) ? ST : WT; break;
    }

    // Determine the actual prediction (before update) as TAKEN/NOTTAKEN
    // so we can detect misprediction
    uint8_t pred_taken = (pred == WT || pred == ST) ? TAKEN : NOTTAKEN;

    // If it was a misprediction, consider allocating a new tag if no match
    if (pred_taken != outcome) {
        if (which_way < 0) {
            // means neither way's tag matched partial_tag
            // => check if we should allocate partial_tag in a way
            // using your special rule:
            //   if bht_custom_stage1[idx0] in {ST, SN} => allocate in idx1
            //   else allocate in idx0

            if (bht_custom_stage1[idx0] == ST || bht_custom_stage1[idx0] == SN) {
                tag_custom_stage1[idx1] = partial_tag;
                // (Optionally re-init that 2-bit counter, e.g. WN or WT)
                // bht_custom_stage1[idx1] = (outcome == TAKEN) ? WT : WN;
            }
            else {
                tag_custom_stage1[idx0] = partial_tag;
                // bht_custom_stage1[idx0] = (outcome == TAKEN) ? WT : WN;
            }
        }
    }

    // Finally, shift the ghistory AFTER we use the old value for indexing/tags
    ghistory = ((ghistory << 1) | (outcome & 1));
}







//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//

// gshare functions


//inits
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
  int global_entries = 1 << tournement_global_bits;
  int local_entries = 1 << tournement_local_bits;

  pht_tournament_local=(uint16_t *)malloc(local_entries * sizeof(uint16_t));

  bht_tournament_local= (uint8_t *)malloc(local_entries * sizeof(uint8_t));
  bht_tournament_corele = (uint8_t *)malloc(global_entries* sizeof(uint8_t));
  bht_tournament_choice = (uint8_t *)malloc(global_entries * sizeof(uint8_t));

  int i = 0;
  for (i = 0; i < local_entries; i++)
  {
    pht_tournament_local[i] = 0;
    bht_tournament_local[i] = WN;
  }
  for (i = 0; i < global_entries; i++)
  {
    bht_tournament_corele[i] = WN;
    bht_tournament_choice[i] = WN;
  }

  ghistory = 0;
}




//predicts



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

uint8_t tournament_predict(uint32_t pc)
{
  // get lower ghistoryBits of pc
  uint32_t global_entries = 1 << tournement_global_bits;
  uint32_t local_entries = 1 << tournement_local_bits;
  uint32_t pc_lower_bits = pc & (local_entries- 1);
  uint32_t ghistory_lower_bits = ghistory & (global_entries - 1);

  uint16_t pattern = pht_tournament_local[pc_lower_bits]& (local_entries - 1);

  uint8_t local_predict = bht_tournament_local[pattern];
  uint8_t global_predict = bht_tournament_corele[ghistory_lower_bits];

  uint8_t selected=0;






  switch (bht_tournament_choice[ghistory_lower_bits])
  {
  case WN:
    selected=local_predict;
    break;
  case SN:
    selected=local_predict;
    break;
  case WT:
    selected=global_predict;
    break;
  case ST:
    selected=global_predict;
    break;
  default:
  printf("choice predict\n");
    selected=local_predict;
  }


  switch (selected)
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
    printf("tournamnet predict\n");
    printf("%u\n", selected);
    printf("%u\n", bht_tournament_choice[ghistory_lower_bits]);
    
    return NOTTAKEN;
  }
}








// trains


void train_tournament(uint32_t pc, uint8_t outcome)
{
  // get lower ghistoryBits of pc
  uint32_t global_entries = 1 << tournement_global_bits;
  uint32_t local_entries = 1 << tournement_local_bits;
  uint32_t pc_lower_bits = pc & (local_entries- 1);
  uint32_t ghistory_lower_bits = ghistory & (global_entries - 1);

  uint16_t pattern = pht_tournament_local[pc_lower_bits]& (local_entries- 1);

  uint8_t local_predict = bht_tournament_local[pattern];
  uint8_t global_predict = bht_tournament_corele[ghistory_lower_bits];

  uint8_t selected=0;



  switch (bht_tournament_choice[ghistory_lower_bits])
  {
  case WN:
    bht_tournament_choice[ghistory_lower_bits]= (outcome == ((local_predict>1) ? TAKEN:NOTTAKEN)) ? SN : WT;
    break;
  case SN:
    bht_tournament_choice[ghistory_lower_bits]= (outcome == ((local_predict>1) ? TAKEN:NOTTAKEN)) ? SN : WN;
    break;
  case WT:
    bht_tournament_choice[ghistory_lower_bits]= (outcome == ((global_predict>1) ? TAKEN:NOTTAKEN)) ? ST : WN;
    break;
  case ST:
    bht_tournament_choice[ghistory_lower_bits]= (outcome == ((global_predict>1) ? TAKEN:NOTTAKEN)) ? ST : WT;
    break;
  default:
  break;

  }






  // Update state of entry in bht based on outcome
  switch (local_predict)
  {
  case WN:
    bht_tournament_local[pattern] = (outcome == TAKEN) ? WT : SN;
    break;
  case SN:
    bht_tournament_local[pattern] = (outcome == TAKEN) ? WN : SN;
    break;
  case WT:
    bht_tournament_local[pattern] = (outcome == TAKEN) ? ST : WN;
    break;
  case ST:
    bht_tournament_local[pattern] = (outcome == TAKEN) ? ST : WT;
    break;
  default:
    printf("local predict!\n");
    break;
  }

    switch (global_predict)
  {
  case WN:
    bht_tournament_corele[ghistory_lower_bits] = (outcome == TAKEN) ? WT : SN;
    break;
  case SN:
    bht_tournament_corele[ghistory_lower_bits] = (outcome == TAKEN) ? WN : SN;
    break;
  case WT:
    bht_tournament_corele[ghistory_lower_bits]= (outcome == TAKEN) ? ST : WN;
    break;
  case ST:
    bht_tournament_corele[ghistory_lower_bits]= (outcome == TAKEN) ? ST : WT;
    break;
  default:
    printf("global predict!\n");
    break;
  }



  // Update history register
  ghistory = ((ghistory << 1) | outcome);
  pht_tournament_local[pc_lower_bits]= ((pattern << 1) | outcome);

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


//cleans
void cleanup_gshare()
{
  free(bht_gshare);
}
void cleanup_tournament()
{
  free(bht_tournament_choice);
  free(bht_tournament_corele);
  free(bht_tournament_local);
  free(pht_tournament_local);

}

void cleanup_custom()
{




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
    return  tournament_predict(pc);
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
      return train_custom(pc, outcome);;
    default:
      break;
    }
  }
}
