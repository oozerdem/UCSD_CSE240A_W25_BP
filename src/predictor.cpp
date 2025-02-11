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
int tournement_local_bits=12;
int tournement_pattern_bits=12;
int tournement_choice_bits=15;


int custom_global_bits=15;
int custom_local_bits=12;
int custom_pattern_bits=14;
int custom_choice_bits=15;








int ghistory;

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

uint8_t *bht_bimode_choice;
uint8_t *bht_bimode_nt;
uint8_t *bht_bimode_tk;








void init_tournament()
{
  int global_entries = 1 << tournement_global_bits;
  int local_entries = 1 << tournement_local_bits;
  int choice_entries=1<<tournement_choice_bits;
  int pattern_entries=1<<tournement_pattern_bits;

  pht_tournament_local=(uint16_t *)malloc(local_entries * sizeof(uint16_t));

  bht_tournament_local= (uint8_t *)malloc(pattern_entries * sizeof(uint8_t));
  bht_tournament_corele = (uint8_t *)malloc(global_entries* sizeof(uint8_t));
  bht_tournament_choice = (uint8_t *)malloc(choice_entries * sizeof(uint8_t));

  int i = 0;
  for (i = 0; i < local_entries; i++)
  {
    pht_tournament_local[i] = WN;
    
  }
  for (i = 0; i < pattern_entries; i++)
  {
    bht_tournament_local[i] = WN;
    
  }
    for (i = 0; i < global_entries; i++)
  {
    bht_tournament_corele[i] = WN;
    
  }
    for (i = 0; i < choice_entries; i++)
  {
    bht_tournament_choice[i] = WN;
    
  }

  ghistory = 0;
}



uint8_t tournament_predict(uint32_t pc)
{
  // get lower ghistoryBits of pc
  int global_entries = 1 << tournement_global_bits;
  int local_entries = 1 << tournement_local_bits;
  int choice_entries=1<<tournement_choice_bits;
  int pattern_entries=1<<tournement_pattern_bits;

  uint32_t pc_lower_bits = pc & (local_entries- 1);
  uint32_t ghistory_lower_bits_choice = (pc^ghistory) & (choice_entries - 1);
  uint32_t ghistory_lower_bits_corele = (pc^ghistory) & (global_entries - 1);
  uint16_t pattern = pht_tournament_local[pc_lower_bits]& (pattern_entries - 1);

  uint8_t local_predict = bht_tournament_local[pattern];
  uint8_t global_predict = bht_tournament_corele[ghistory_lower_bits_corele];

  uint8_t selected=0;






  switch (bht_tournament_choice[ghistory_lower_bits_choice])
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

    
    return NOTTAKEN;
  }
}




void train_tournament(uint32_t pc, uint8_t outcome)
{
  // get lower ghistoryBits of pc
  int global_entries = 1 << tournement_global_bits;
  int local_entries = 1 << tournement_local_bits;
  int choice_entries=1<<tournement_choice_bits;
  int pattern_entries=1<<tournement_pattern_bits;

  uint32_t pc_lower_bits = pc & (local_entries- 1);
  uint32_t ghistory_lower_bits_choice = (pc^ghistory)& (choice_entries - 1);
  uint32_t ghistory_lower_bits_corele = (pc^ghistory) & (global_entries - 1);
  uint16_t pattern = pht_tournament_local[pc_lower_bits]& (pattern_entries - 1);

  uint8_t local_predict = bht_tournament_local[pattern];
  uint8_t global_predict = bht_tournament_corele[ghistory_lower_bits_corele];

  uint8_t selected=0;



  switch (bht_tournament_choice[ghistory_lower_bits_choice])
  {
  case WN:
    bht_tournament_choice[ghistory_lower_bits_choice]= (outcome == ((local_predict>1) ? TAKEN:NOTTAKEN)) ? SN : WT;
    break;
  case SN:
    bht_tournament_choice[ghistory_lower_bits_choice]= (outcome == ((local_predict>1) ? TAKEN:NOTTAKEN)) ? SN : WN;
    break;
  case WT:
    bht_tournament_choice[ghistory_lower_bits_choice]= (outcome == ((global_predict>1) ? TAKEN:NOTTAKEN)) ? ST : WN;
    break;
  case ST:
    bht_tournament_choice[ghistory_lower_bits_choice]= (outcome == ((global_predict>1) ? TAKEN:NOTTAKEN)) ? ST : WT;
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
    bht_tournament_corele[ghistory_lower_bits_corele] = (outcome == TAKEN) ? WT : SN;
    break;
  case SN:
    bht_tournament_corele[ghistory_lower_bits_corele] = (outcome == TAKEN) ? WN : SN;
    break;
  case WT:
    bht_tournament_corele[ghistory_lower_bits_corele]= (outcome == TAKEN) ? ST : WN;
    break;
  case ST:
    bht_tournament_corele[ghistory_lower_bits_corele]= (outcome == TAKEN) ? ST : WT;
    break;
  default:
    printf("global predict!\n");
    break;
  }



  // Update history register
  ghistory = ((ghistory << 1) | outcome);
  pht_tournament_local[pc_lower_bits]= ((pattern << 1) | outcome);

}








void init_custom()
{
  int global_entries = 1 << custom_global_bits;
  int local_entries = 1 << custom_local_bits;
  int choice_entries=1<< custom_choice_bits;
  int pattern_entries=1<<custom_pattern_bits;

  pht_tournament_local=(uint16_t *)malloc(local_entries * sizeof(uint16_t));

  bht_tournament_local= (uint8_t *)malloc(pattern_entries * sizeof(uint8_t));
  bht_bimode_choice = (uint8_t *)malloc((global_entries)* sizeof(uint8_t));
  bht_bimode_tk = (uint8_t *)malloc((global_entries)* sizeof(uint8_t));
  bht_bimode_nt = (uint8_t *)malloc((global_entries)* sizeof(uint8_t));
  bht_tournament_choice = (uint8_t *)malloc(choice_entries * sizeof(uint8_t));

  int i = 0;
  for (i = 0; i < local_entries; i++)
  {
    pht_tournament_local[i] = WN;
    
  }
  for (i = 0; i < pattern_entries; i++)
  {
    bht_tournament_local[i] = WN;
    
  }
    for (i = 0; i < (global_entries); i++)
  {
    bht_bimode_choice[i] = WN;
    
  }
      for (i = 0; i < (global_entries); i++)
  {
    bht_bimode_nt[i] = WN;
    bht_bimode_tk[i] = WT;
    
  }
    for (i = 0; i < choice_entries; i++)
  {
    bht_tournament_choice[i] = WN;
    
  }

  ghistory = 0;
}




//predicts


uint8_t custom_predict(uint32_t pc)
{
  // get lower ghistoryBits of pc
  int global_entries = 1 << custom_global_bits;
  int local_entries = 1 << custom_local_bits;
  int choice_entries=1<<custom_choice_bits;
  int pattern_entries=1<<custom_pattern_bits;

  uint32_t pc_lower_bits = pc & (local_entries- 1);
  uint32_t ghistory_lower_bits_choice = (pc^ghistory) & (choice_entries - 1);
  uint32_t ghistory_lower_bits_corele = (pc^ghistory) & (global_entries - 1);
  uint16_t pattern = pht_tournament_local[pc_lower_bits]& (pattern_entries - 1);

  uint8_t local_predict = bht_tournament_local[pattern];


  uint8_t bimode_choice = bht_bimode_choice[(ghistory_lower_bits_corele)];
  uint8_t bimode_tk = bht_bimode_tk[(ghistory_lower_bits_corele)];
  uint8_t bimode_nt = bht_bimode_nt[(ghistory_lower_bits_corele)];

  uint8_t global_predict=((bimode_choice>1) ? bimode_tk:bimode_nt);

  uint8_t selected=0;




  

  switch (bht_tournament_choice[ghistory_lower_bits_choice])
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

    
    return NOTTAKEN;
  }
}








// trains


void train_custom(uint32_t pc, uint8_t outcome)
{
  // get lower ghistoryBits of pc
  int global_entries = 1 << custom_global_bits;
  int local_entries = 1 << custom_local_bits;
  int choice_entries=1<<custom_choice_bits;
  int pattern_entries=1<<custom_pattern_bits;

  uint32_t pc_lower_bits = pc & (local_entries- 1);
  uint32_t ghistory_lower_bits_choice = (pc^ghistory) & (choice_entries - 1);
  uint32_t ghistory_lower_bits_corele = (pc^ghistory) & (global_entries - 1);
  uint16_t pattern = pht_tournament_local[pc_lower_bits]& (pattern_entries - 1);

  uint8_t local_predict = bht_tournament_local[pattern];
  uint8_t bimode_choice = bht_bimode_choice[(ghistory_lower_bits_corele)];
  uint8_t bimode_tk = bht_bimode_tk[(ghistory_lower_bits_corele)];
  uint8_t bimode_nt = bht_bimode_nt[(ghistory_lower_bits_corele)];
  uint8_t global_predict=((bimode_choice>1) ? bimode_tk:bimode_nt);

  uint8_t selected=0;



  switch (bht_tournament_choice[ghistory_lower_bits_choice])
  {
  case WN:
    bht_tournament_choice[ghistory_lower_bits_choice]= (outcome == ((local_predict>1) ? TAKEN:NOTTAKEN)) ? SN : WT;
    break;
  case SN:
    bht_tournament_choice[ghistory_lower_bits_choice]= (outcome == ((local_predict>1) ? TAKEN:NOTTAKEN)) ? SN : WN;
    break;
  case WT:
    bht_tournament_choice[ghistory_lower_bits_choice]= (outcome == ((global_predict>1) ? TAKEN:NOTTAKEN)) ? ST : WN;
    break;
  case ST:
    bht_tournament_choice[ghistory_lower_bits_choice]= (outcome == ((global_predict>1) ? TAKEN:NOTTAKEN)) ? ST : WT;
    break;
  default:
  break;

  }






 if(bht_tournament_choice[ghistory_lower_bits_choice]<2){


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
   }
   else{

    if(!(((global_predict>1 ? TAKEN:NOTTAKEN)==outcome)&&(((global_predict>1 ? TAKEN:NOTTAKEN))!=((bimode_choice>1 ? TAKEN:NOTTAKEN))))){
      switch (bimode_choice)
  {
  case WN:
   bht_bimode_choice[(ghistory_lower_bits_corele)]= (outcome == TAKEN) ? WT : SN;
    break;
  case SN:
   bht_bimode_choice[(ghistory_lower_bits_corele)] = (outcome == TAKEN) ? WN : SN;
    break;
  case WT:
    bht_bimode_choice[(ghistory_lower_bits_corele)]= (outcome == TAKEN) ? ST : WN;
    break;
  case ST:
    bht_bimode_choice[(ghistory_lower_bits_corele)]= (outcome == TAKEN) ? ST : WT;
    break;
  default:
    printf("global predict!\n");
    break;
  }
    }

    if(bimode_choice>1)
  {
              switch (bimode_tk)
  {
  case WN:
    bht_bimode_tk[(ghistory_lower_bits_corele)]= (outcome == TAKEN) ? WT : SN;
    break;
  case SN:
    bht_bimode_tk[(ghistory_lower_bits_corele)] = (outcome == TAKEN) ? WN : SN;
    break;
  case WT:
    bht_bimode_tk[(ghistory_lower_bits_corele)]= (outcome == TAKEN) ? ST : WN;
    break;
  case ST:
    bht_bimode_tk[(ghistory_lower_bits_corele)]= (outcome == TAKEN) ? ST : WT;
    break;
  default:
    printf("global predict!\n");
    break;
  }

  }
  else{
              switch (bimode_nt)
  {
  case WN:
    bht_bimode_nt[(ghistory_lower_bits_corele)]= (outcome == TAKEN) ? WT : SN;
    break;
  case SN:
    bht_bimode_nt[(ghistory_lower_bits_corele)] = (outcome == TAKEN) ? WN : SN;
    break;
  case WT:
    bht_bimode_nt[(ghistory_lower_bits_corele)]= (outcome == TAKEN) ? ST : WN;
    break;
  case ST:
    bht_bimode_nt[(ghistory_lower_bits_corele)]= (outcome == TAKEN) ? ST : WT;
    break;
  default:
    printf("global predict!\n");
    break;
  }

  }





  


 }
   



  // Update history register
  ghistory = ((ghistory << 1) | outcome);
  pht_tournament_local[pc_lower_bits]= ((pattern << 1) | outcome);


  

}

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

uint8_t gshare_predict(uint32_t pc)
{
 
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



void train_gshare(uint32_t pc, uint8_t outcome)
{
 
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries - 1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries - 1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;

 
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
{cleanup_tournament();





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
