/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#include "PHY/types.h"
#include "PHY/defs_eNB.h"

#include "common/utils/LOG/vcd_signal_dumper.h"

#define DEBUG_PHY

int lte_est_timing_advance(LTE_DL_FRAME_PARMS *frame_parms,
                           LTE_eNB_SRS *lte_eNB_srs,
                           unsigned int  *eNB_id,
                           unsigned char clear,
                           unsigned char number_of_cards,
                           short coef)

{

  static int max_pos_fil2 = 0;
  int temp, i, aa, max_pos = 0,ind;
  int max_val=0;
  short Re,Im,ncoef;
#ifdef DEBUG_PHY
  char fname[100],vname[100];
#endif

  ncoef = 32768 - coef;

  for (ind=0; ind<number_of_cards; ind++) {

    if (ind==0)
      max_val=0;


    for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
      // do ifft of channel estimate
      int len=0;
      switch(frame_parms->N_RB_DL) {
      case 6:
        len = 128;
        break;
      case 25:
        len = 512;
        break;
      case 50:
        len = 1024;
        break;
      case 100:
        len = 2048;
        break;
      }
       if (len)
      dft(get_dft(len), (int16_t *)lte_eNB_srs->srs_ch_estimates[aa], (int16_t *)lte_eNB_srs->srs_ch_estimates_time[aa], 1);
#ifdef DEBUG_PHY
      sprintf(fname,"srs_ch_estimates_time_%d%d.m",ind,aa);
      sprintf(vname,"srs_time_%d%d",ind,aa);
      LOG_M(fname,vname,lte_eNB_srs->srs_ch_estimates_time[aa],frame_parms->ofdm_symbol_size*2,2,1);
#endif
    }

    // we only use channel estimates from tx antenna 0 here
    // remember we fixed the SRS to use only every second subcarriers
    for (i = 0; i < frame_parms->ofdm_symbol_size/2; i++) {
      temp = 0;

      for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
        Re = ((int16_t*)lte_eNB_srs->srs_ch_estimates_time[aa])[(i<<1)];
        Im = ((int16_t*)lte_eNB_srs->srs_ch_estimates_time[aa])[1+(i<<1)];
        temp += (Re*Re/2) + (Im*Im/2);
      }

      if (temp > max_val) {
        max_pos = i;
        max_val = temp;
        *eNB_id = ind;
      }
    }
  }

  // filter position to reduce jitter
  if (clear == 1)
    max_pos_fil2 = max_pos;
  else
    max_pos_fil2 = ((max_pos_fil2 * coef) + (max_pos * ncoef)) >> 15;

  return(max_pos_fil2);
}


int lte_est_timing_advance_pusch(LTE_DL_FRAME_PARMS *frame_parms,
                                 int32_t **ul_ch_estimates_time)
{
  int temp, i, aa, max_pos=0, max_val=0;
  short Re,Im;

 // uint8_t cyclic_shift = 0;
  int sync_pos = 0;//(frame_parms->ofdm_symbol_size-cyclic_shift*frame_parms->ofdm_symbol_size/12)%(frame_parms->ofdm_symbol_size);
  
  AssertFatal(frame_parms->ofdm_symbol_size > 127,"frame_parms->ofdm_symbol_size %d<128\n",frame_parms->ofdm_symbol_size);
  AssertFatal(frame_parms->nb_antennas_rx >0 && frame_parms->nb_antennas_rx<=4,"frame_parms->nb_antennas_rx %d not in [1...%d]\n",
	      frame_parms->nb_antennas_rx,4);
  for (i = 0; i < frame_parms->ofdm_symbol_size; i++) {
    temp = 0;

    for (aa=0; aa<frame_parms->nb_antennas_rx; aa++) {
      Re = ((int16_t*)ul_ch_estimates_time[aa])[(i<<1)];
      Im = ((int16_t*)ul_ch_estimates_time[aa])[1+(i<<1)];
      temp += (Re*Re/2) + (Im*Im/2);
    }

    if (temp > max_val) {
      max_pos = i;
      max_val = temp;
    }
  }

  if (max_pos>frame_parms->ofdm_symbol_size/2)
    max_pos = max_pos-frame_parms->ofdm_symbol_size;

  return max_pos - sync_pos;
}
