/*
 * Copyright 2017 Cisco Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <signal.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sched.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>

#include <nfapi_interface.h>
#include <nfapi.h>
#include "nfapi_nr_interface.h"
#include "nfapi_nr_interface_scf.h"
#include "nfapi/oai_integration/vendor_ext.h"
#include <debug.h>
#include "nr_fapi_p5.h"
#include "nr_fapi.h"

// Pack routines
//TODO: Add pacl/unpack fns for uint32 and uint64
static uint8_t pack_nr_pnf_param_request(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  nfapi_nr_pnf_param_request_t *request = (nfapi_nr_pnf_param_request_t *)msg;
  return pack_vendor_extension_tlv(request->vendor_extension, ppWritePackedMsg, end, config);
}

static uint8_t pack_pnf_param_request(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  nfapi_pnf_param_request_t *request = (nfapi_pnf_param_request_t *)msg;
  return pack_vendor_extension_tlv(request->vendor_extension, ppWritePackedMsg, end, config);
}

static uint8_t pack_pnf_rf_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_pnf_rf_t *value = (nfapi_pnf_rf_t *)tlv;
  uint16_t rf_index = 0;

  if(push16(value->number_of_rfs, ppWritePackedMsg, end) == 0)
    return 0;

  for(; rf_index < value->number_of_rfs; ++rf_index) {
    if( !(push16(value->rf[rf_index].rf_config_index, ppWritePackedMsg, end) &&
          push16(value->rf[rf_index].band, ppWritePackedMsg, end) &&
          pushs16(value->rf[rf_index].maximum_transmit_power, ppWritePackedMsg, end) &&
          pushs16(value->rf[rf_index].minimum_transmit_power, ppWritePackedMsg, end) &&
          push8(value->rf[rf_index].number_of_antennas_suppported, ppWritePackedMsg, end) &&
          push32(value->rf[rf_index].minimum_downlink_frequency, ppWritePackedMsg, end) &&
          push32(value->rf[rf_index].maximum_downlink_frequency, ppWritePackedMsg, end) &&
          push32(value->rf[rf_index].minimum_uplink_frequency, ppWritePackedMsg, end) &&
          push32(value->rf[rf_index].maximum_uplink_frequency, ppWritePackedMsg, end)))
      return 0;
  }

  return 1;
}
static uint8_t pack_pnf_phy_rel10_info(void *elem, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_pnf_phy_rel10_info_t *phy = (nfapi_pnf_phy_rel10_info_t *)elem;
  return(push16(phy->phy_config_index, ppWritePackedMsg, end) &&
         push16(phy->transmission_mode_7_supported, ppWritePackedMsg, end) &&
         push16(phy->transmission_mode_8_supported, ppWritePackedMsg, end) &&
         push16(phy->two_antenna_ports_for_pucch, ppWritePackedMsg, end) &&
         push16(phy->transmission_mode_9_supported, ppWritePackedMsg, end) &&
         push16(phy->simultaneous_pucch_pusch, ppWritePackedMsg, end) &&
         push16(phy->four_layer_tx_with_tm3_and_tm4, ppWritePackedMsg, end));
}

static uint8_t pack_pnf_phy_rel10_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_pnf_phy_rel10_t *value = (nfapi_pnf_phy_rel10_t *)tlv;
  return (push16(value->number_of_phys, ppWritePackedMsg, end) &&
          packarray(value->phy, sizeof(nfapi_pnf_phy_rel10_info_t), NFAPI_MAX_PNF_PHY, value->number_of_phys, ppWritePackedMsg, end, &pack_pnf_phy_rel10_info));
}

static uint8_t pack_pnf_phy_rel11_info(void *elem, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_pnf_phy_rel11_info_t *phy = (nfapi_pnf_phy_rel11_info_t *)elem;
  return (push16(phy->phy_config_index, ppWritePackedMsg, end) &&
          push16(phy->edpcch_supported, ppWritePackedMsg, end) &&
          push16(phy->multi_ack_csi_reporting, ppWritePackedMsg, end) &&
          push16(phy->pucch_tx_diversity, ppWritePackedMsg, end) &&
          push16(phy->ul_comp_supported, ppWritePackedMsg, end) &&
          push16(phy->transmission_mode_5_supported, ppWritePackedMsg, end ));
}
static uint8_t pack_pnf_phy_rel11_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_pnf_phy_rel11_t *value = (nfapi_pnf_phy_rel11_t *)tlv;
  return (push16(value->number_of_phys, ppWritePackedMsg, end) &&
          packarray(value->phy, sizeof(nfapi_pnf_phy_rel11_info_t), NFAPI_MAX_PNF_PHY, value->number_of_phys, ppWritePackedMsg, end, &pack_pnf_phy_rel11_info));
}
static uint8_t pack_pnf_phy_rel12_info(void *elem, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_pnf_phy_rel12_info_t *phy = (nfapi_pnf_phy_rel12_info_t *)elem;
  return( push16(phy->phy_config_index, ppWritePackedMsg, end) &&
          push16(phy->csi_subframe_set, ppWritePackedMsg, end) &&
          push16(phy->enhanced_4tx_codebook, ppWritePackedMsg, end) &&
          push16(phy->drs_supported, ppWritePackedMsg, end) &&
          push16(phy->ul_64qam_supported, ppWritePackedMsg, end) &&
          push16(phy->transmission_mode_10_supported, ppWritePackedMsg, end) &&
          push16(phy->alternative_bts_indices, ppWritePackedMsg, end));
}
static uint8_t pack_pnf_phy_rel12_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_pnf_phy_rel12_t *value = (nfapi_pnf_phy_rel12_t *)tlv;
  return (push16(value->number_of_phys, ppWritePackedMsg, end) &&
          packarray(value->phy, sizeof(nfapi_pnf_phy_rel12_info_t), NFAPI_MAX_PNF_PHY, value->number_of_phys, ppWritePackedMsg, end, &pack_pnf_phy_rel12_info));
}

static uint8_t pack_pnf_phy_rel13_info(void *elem, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_pnf_phy_rel13_info_t *phy = (nfapi_pnf_phy_rel13_info_t *)elem;
  return( push16(phy->phy_config_index, ppWritePackedMsg, end) &&
          push16(phy->pucch_format4_supported, ppWritePackedMsg, end) &&
          push16(phy->pucch_format5_supported, ppWritePackedMsg, end) &&
          push16(phy->more_than_5_ca_support, ppWritePackedMsg, end) &&
          push16(phy->laa_supported, ppWritePackedMsg, end) &&
          push16(phy->laa_ending_in_dwpts_supported, ppWritePackedMsg, end) &&
          push16(phy->laa_starting_in_second_slot_supported, ppWritePackedMsg, end) &&
          push16(phy->beamforming_supported, ppWritePackedMsg, end) &&
          push16(phy->csi_rs_enhancement_supported, ppWritePackedMsg, end) &&
          push16(phy->drms_enhancement_supported, ppWritePackedMsg, end) &&
          push16(phy->srs_enhancement_supported, ppWritePackedMsg, end) );
}

static uint8_t pack_pnf_phy_rel13_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_pnf_phy_rel13_t *value = (nfapi_pnf_phy_rel13_t *)tlv;
  return (push16(value->number_of_phys, ppWritePackedMsg, end) &&
          packarray(value->phy, sizeof(nfapi_pnf_phy_rel13_info_t), NFAPI_MAX_PNF_PHY, value->number_of_phys, ppWritePackedMsg, end, &pack_pnf_phy_rel13_info));
}

static uint8_t pack_pnf_phy_rel13_nb_iot_info(void *elem, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_pnf_phy_rel13_nb_iot_info_t *phy = (nfapi_pnf_phy_rel13_nb_iot_info_t *)elem;
  return( push16(phy->phy_config_index, ppWritePackedMsg, end) &&
          push16(phy->number_of_rfs, ppWritePackedMsg, end) &&
          packarray(phy->rf_config, sizeof(nfapi_rf_config_info_t), NFAPI_MAX_PNF_PHY_RF_CONFIG, phy->number_of_rfs, ppWritePackedMsg, end, &pack_rf_config_info) &&
          push16(phy->number_of_rf_exclusions, ppWritePackedMsg, end) &&
          packarray(phy->excluded_rf_config, sizeof(nfapi_rf_config_info_t), NFAPI_MAX_PNF_PHY_RF_CONFIG, phy->number_of_rf_exclusions, ppWritePackedMsg, end, &pack_rf_config_info) &&
          push8(phy->number_of_dl_layers_supported, ppWritePackedMsg, end) &&
          push8(phy->number_of_ul_layers_supported, ppWritePackedMsg, end) &&
          push16(phy->maximum_3gpp_release_supported, ppWritePackedMsg, end) &&
          push8(phy->nmm_modes_supported, ppWritePackedMsg, end));
}

static uint8_t pack_pnf_phy_rel13_nb_iot_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_pnf_phy_rel13_nb_iot_t *value = (nfapi_pnf_phy_rel13_nb_iot_t *)tlv;
  return (push16(value->number_of_phys, ppWritePackedMsg, end) &&
          packarray(value->phy, sizeof(nfapi_pnf_phy_rel13_nb_iot_info_t), NFAPI_MAX_PNF_PHY, value->number_of_phys, ppWritePackedMsg, end, &pack_pnf_phy_rel13_nb_iot_info));
}
/*
static uint8_t pack_nr_pnf_param_response(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t* config)
{
  nfapi_nr_pnf_param_response_t *pNfapiMsg = (nfapi_nr_pnf_param_response_t*)msg;

  return (push32(pNfapiMsg->error_code, ppWritePackedMsg, end) &&
      pack_tlv(NFAPI_PNF_PARAM_GENERAL_TAG, &pNfapiMsg->pnf_param_general, ppWritePackedMsg, end, &pack_pnf_param_general_value) &&
      pack_tlv(NFAPI_PNF_PHY_TAG, &pNfapiMsg->pnf_phy, ppWritePackedMsg, end, &pack_pnf_phy_value) &&
      pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}
*/
static uint8_t pack_nr_pnf_param_response(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config)
{
  nfapi_nr_pnf_param_response_t *pNfapiMsg = (nfapi_nr_pnf_param_response_t *)msg;
  return (
      push8(pNfapiMsg->error_code, ppWritePackedMsg, end) && push8(pNfapiMsg->num_tlvs, ppWritePackedMsg, end) && // numTLVs
      pack_nr_tlv(NFAPI_PNF_PARAM_GENERAL_TAG, &pNfapiMsg->pnf_param_general, ppWritePackedMsg, end, &pack_pnf_param_general_value)
      && pack_nr_tlv(NFAPI_PNF_PHY_TAG, &pNfapiMsg->pnf_phy, ppWritePackedMsg, end, &pack_pnf_phy_value)
      && pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_pnf_param_response(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  nfapi_pnf_param_response_t *pNfapiMsg = (nfapi_pnf_param_response_t *)msg;
  return (push32(pNfapiMsg->error_code, ppWritePackedMsg, end) &&
          pack_tlv(NFAPI_PNF_PARAM_GENERAL_TAG, &pNfapiMsg->pnf_param_general, ppWritePackedMsg, end, &pack_pnf_param_general_value) &&
          pack_tlv(NFAPI_PNF_PHY_TAG, &pNfapiMsg->pnf_phy, ppWritePackedMsg, end, &pack_pnf_phy_value) &&
          pack_tlv(NFAPI_PNF_RF_TAG, &pNfapiMsg->pnf_rf, ppWritePackedMsg, end, &pack_pnf_rf_value) &&
          pack_tlv(NFAPI_PNF_PHY_REL10_TAG, &pNfapiMsg->pnf_phy_rel10, ppWritePackedMsg, end, &pack_pnf_phy_rel10_value) &&
          pack_tlv(NFAPI_PNF_PHY_REL11_TAG, &pNfapiMsg->pnf_phy_rel11, ppWritePackedMsg, end, &pack_pnf_phy_rel11_value) &&
          pack_tlv(NFAPI_PNF_PHY_REL12_TAG, &pNfapiMsg->pnf_phy_rel12, ppWritePackedMsg, end, &pack_pnf_phy_rel12_value) &&
          pack_tlv(NFAPI_PNF_PHY_REL13_TAG, &pNfapiMsg->pnf_phy_rel13, ppWritePackedMsg, end, &pack_pnf_phy_rel13_value) &&
          pack_tlv(NFAPI_PNF_PHY_REL13_NB_IOT_TAG, &pNfapiMsg->pnf_phy_rel13_nb_iot, ppWritePackedMsg, end, &pack_pnf_phy_rel13_nb_iot_value) &&
          pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_nr_pnf_config_request(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config)
{
  nfapi_nr_pnf_config_request_t *pNfapiMsg = (nfapi_nr_pnf_config_request_t *)msg;
  return (push8(pNfapiMsg->num_tlvs, ppWritePackedMsg, end) && // numTLVs
          pack_nr_tlv(NFAPI_PNF_PHY_RF_TAG, &pNfapiMsg->pnf_phy_rf_config, ppWritePackedMsg, end, &pack_pnf_phy_rf_config_value)
          && pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_pnf_config_request(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  nfapi_pnf_config_request_t *pNfapiMsg = (nfapi_pnf_config_request_t *)msg;
  return (push8(pNfapiMsg->num_tlvs,ppWritePackedMsg,end) &&
          pack_tlv(NFAPI_PNF_PHY_RF_TAG, &pNfapiMsg->pnf_phy_rf_config, ppWritePackedMsg, end, &pack_pnf_phy_rf_config_value) &&
          pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_nr_pnf_config_response(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config)
{
  nfapi_nr_pnf_config_response_t *pNfapiMsg = (nfapi_nr_pnf_config_response_t *)msg;
  return (push8(pNfapiMsg->error_code, ppWritePackedMsg, end)
          && pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_pnf_config_response(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  nfapi_pnf_config_response_t *pNfapiMsg = (nfapi_pnf_config_response_t *)msg;
  return ( push32(pNfapiMsg->error_code, ppWritePackedMsg, end) &&
           pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_nr_pnf_start_request(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  nfapi_nr_pnf_start_request_t *pNfapiMsg = (nfapi_nr_pnf_start_request_t *)msg;
  return ( pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_pnf_start_request(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  nfapi_pnf_start_request_t *pNfapiMsg = (nfapi_pnf_start_request_t *)msg;
  return ( pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}


static uint8_t pack_nr_pnf_start_response(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  nfapi_nr_pnf_start_response_t *pNfapiMsg = (nfapi_nr_pnf_start_response_t *)msg;
  return( push32(pNfapiMsg->error_code, ppWritePackedMsg, end) &&
          pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}


static uint8_t pack_pnf_start_response(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  nfapi_pnf_start_response_t *pNfapiMsg = (nfapi_pnf_start_response_t *)msg;
  return( push32(pNfapiMsg->error_code, ppWritePackedMsg, end) &&
          pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}


static uint8_t pack_nr_pnf_stop_request(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  nfapi_nr_pnf_stop_request_t *pNfapiMsg = (nfapi_nr_pnf_stop_request_t *)msg;
  return ( pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config) );
}



static uint8_t pack_pnf_stop_request(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  nfapi_pnf_stop_request_t *pNfapiMsg = (nfapi_pnf_stop_request_t *)msg;
  return ( pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config) );
}


static uint8_t pack_nr_pnf_stop_response(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  nfapi_nr_pnf_stop_response_t *pNfapiMsg = (nfapi_nr_pnf_stop_response_t *)msg;
  return ( push32(pNfapiMsg->error_code, ppWritePackedMsg, end) &&
           pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}


static uint8_t pack_pnf_stop_response(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  nfapi_pnf_stop_response_t *pNfapiMsg = (nfapi_pnf_stop_response_t *)msg;
  return ( push32(pNfapiMsg->error_code, ppWritePackedMsg, end) &&
           pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_param_request(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  nfapi_param_request_t *pNfapiMsg = (nfapi_param_request_t *)msg;
  return (pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_rf_bands_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_rf_bands_t *value = (nfapi_rf_bands_t *)tlv;
  return ( push16(value->number_rf_bands, ppWritePackedMsg, end) &&
           pusharray16(value->rf_band, NFAPI_MAX_NUM_RF_BANDS, value->number_rf_bands, ppWritePackedMsg, end));
}

static uint8_t unpack_rf_bands_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_rf_bands_t *value = (nfapi_rf_bands_t *)tlv;
  return ( pull16(ppReadPackedMsg, &value->number_rf_bands, end) &&
           pullarray16(ppReadPackedMsg, value->rf_band, NFAPI_MAX_NUM_RF_BANDS, value->number_rf_bands, end));
}

static uint8_t pack_nmm_frequency_bands_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_nmm_frequency_bands_t *value = (nfapi_nmm_frequency_bands_t *)tlv;
  return( push16(value->number_of_rf_bands, ppWritePackedMsg, end) &&
          pusharray16(value->bands, NFAPI_MAX_NMM_FREQUENCY_BANDS, value->number_of_rf_bands, ppWritePackedMsg, end));
}

static uint8_t unpack_nmm_frequency_bands_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_nmm_frequency_bands_t *value = (nfapi_nmm_frequency_bands_t *)tlv;
  return ( pull16(ppReadPackedMsg, &value->number_of_rf_bands, end) &&
           pullarray16(ppReadPackedMsg, value->bands, NFAPI_MAX_NMM_FREQUENCY_BANDS, value->number_of_rf_bands, end));
}

static uint8_t pack_embms_mbsfn_config_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_embms_mbsfn_config_t *value = (nfapi_embms_mbsfn_config_t *)tlv;
  return ( push16(value->num_mbsfn_config, ppWritePackedMsg, end) &&
           pusharray16(value->radioframe_allocation_period, 8,value->num_mbsfn_config,ppWritePackedMsg, end) &&
           pusharray16(value->radioframe_allocation_offset, 8,value->num_mbsfn_config,ppWritePackedMsg, end) &&
           pusharray8(value->fourframes_flag, 8,value->num_mbsfn_config,ppWritePackedMsg, end) &&
           pusharrays32(value->mbsfn_subframeconfig, 8, value->num_mbsfn_config, ppWritePackedMsg, end));
}

static uint8_t pack_param_response(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config)
{
  nfapi_param_response_t *pNfapiMsg = (nfapi_param_response_t *)msg;
  return (
      push8(pNfapiMsg->error_code, ppWritePackedMsg, end) && push8(pNfapiMsg->num_tlv, ppWritePackedMsg, end)
      && pack_tlv(NFAPI_L1_STATUS_PHY_STATE_TAG, &pNfapiMsg->l1_status.phy_state, ppWritePackedMsg, end, &pack_uint16_tlv_value) &&
      // Do we check the phy state and then just fill those sepecified, however
      // we do not know the duplex mode, so just attempt to pack all and assumme
      // that the callee has set the right tlvs
      pack_tlv(NFAPI_PHY_CAPABILITIES_DL_BANDWIDTH_SUPPORT_TAG,
               &(pNfapiMsg->phy_capabilities.dl_bandwidth_support),
               ppWritePackedMsg,
               end,
               &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_PHY_CAPABILITIES_UL_BANDWIDTH_SUPPORT_TAG,
                  &(pNfapiMsg->phy_capabilities.ul_bandwidth_support),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_PHY_CAPABILITIES_DL_MODULATION_SUPPORT_TAG,
                  &(pNfapiMsg->phy_capabilities.dl_modulation_support),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_PHY_CAPABILITIES_UL_MODULATION_SUPPORT_TAG,
                  &(pNfapiMsg->phy_capabilities.ul_modulation_support),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_PHY_CAPABILITIES_PHY_ANTENNA_CAPABILITY_TAG,
                  &(pNfapiMsg->phy_capabilities.phy_antenna_capability),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_PHY_CAPABILITIES_RELEASE_CAPABILITY_TAG,
                  &(pNfapiMsg->phy_capabilities.release_capability),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_PHY_CAPABILITIES_MBSFN_CAPABILITY_TAG,
                  &(pNfapiMsg->phy_capabilities.mbsfn_capability),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      &&
      // laa capability
      pack_tlv(NFAPI_SUBFRAME_CONFIG_DUPLEX_MODE_TAG,
               &(pNfapiMsg->subframe_config.duplex_mode),
               ppWritePackedMsg,
               end,
               &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_SUBFRAME_CONFIG_PCFICH_POWER_OFFSET_TAG,
                  &(pNfapiMsg->subframe_config.pcfich_power_offset),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_SUBFRAME_CONFIG_PB_TAG, &(pNfapiMsg->subframe_config.pb), ppWritePackedMsg, end, &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_SUBFRAME_CONFIG_DL_CYCLIC_PREFIX_TYPE_TAG,
                  &(pNfapiMsg->subframe_config.dl_cyclic_prefix_type),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_SUBFRAME_CONFIG_UL_CYCLIC_PREFIX_TYPE_TAG,
                  &(pNfapiMsg->subframe_config.ul_cyclic_prefix_type),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_RF_CONFIG_DL_CHANNEL_BANDWIDTH_TAG,
                  &(pNfapiMsg->rf_config.dl_channel_bandwidth),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_RF_CONFIG_UL_CHANNEL_BANDWIDTH_TAG,
                  &(pNfapiMsg->rf_config.ul_channel_bandwidth),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_RF_CONFIG_REFERENCE_SIGNAL_POWER_TAG,
                  &(pNfapiMsg->rf_config.reference_signal_power),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_RF_CONFIG_TX_ANTENNA_PORTS_TAG,
                  &(pNfapiMsg->rf_config.tx_antenna_ports),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_RF_CONFIG_RX_ANTENNA_PORTS_TAG,
                  &(pNfapiMsg->rf_config.rx_antenna_ports),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_PHICH_CONFIG_PHICH_RESOURCE_TAG,
                  &(pNfapiMsg->phich_config.phich_resource),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_PHICH_CONFIG_PHICH_DURATION_TAG,
                  &(pNfapiMsg->phich_config.phich_duration),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_PHICH_CONFIG_PHICH_POWER_OFFSET_TAG,
                  &(pNfapiMsg->phich_config.phich_power_offset),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_SCH_CONFIG_PRIMARY_SYNCHRONIZATION_SIGNAL_EPRE_EPRERS_TAG,
                  &(pNfapiMsg->sch_config.primary_synchronization_signal_epre_eprers),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_SCH_CONFIG_SECONDARY_SYNCHRONIZATION_SIGNAL_EPRE_EPRERS_TAG,
                  &(pNfapiMsg->sch_config.secondary_synchronization_signal_epre_eprers),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_SCH_CONFIG_PHYSICAL_CELL_ID_TAG,
                  &(pNfapiMsg->sch_config.physical_cell_id),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_PRACH_CONFIG_CONFIGURATION_INDEX_TAG,
                  &(pNfapiMsg->prach_config.configuration_index),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_PRACH_CONFIG_ROOT_SEQUENCE_INDEX_TAG,
                  &(pNfapiMsg->prach_config.root_sequence_index),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_PRACH_CONFIG_ZERO_CORRELATION_ZONE_CONFIGURATION_TAG,
                  &(pNfapiMsg->prach_config.zero_correlation_zone_configuration),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_PRACH_CONFIG_HIGH_SPEED_FLAG_TAG,
                  &(pNfapiMsg->prach_config.high_speed_flag),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_PRACH_CONFIG_FREQUENCY_OFFSET_TAG,
                  &(pNfapiMsg->prach_config.frequency_offset),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_PUSCH_CONFIG_HOPPING_MODE_TAG,
                  &(pNfapiMsg->pusch_config.hopping_mode),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_PUSCH_CONFIG_HOPPING_OFFSET_TAG,
                  &(pNfapiMsg->pusch_config.hopping_offset),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_PUSCH_CONFIG_NUMBER_OF_SUBBANDS_TAG,
                  &(pNfapiMsg->pusch_config.number_of_subbands),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_PUCCH_CONFIG_DELTA_PUCCH_SHIFT_TAG,
                  &(pNfapiMsg->pucch_config.delta_pucch_shift),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_PUCCH_CONFIG_N_CQI_RB_TAG,
                  &(pNfapiMsg->pucch_config.n_cqi_rb),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_PUCCH_CONFIG_N_AN_CS_TAG, &(pNfapiMsg->pucch_config.n_an_cs), ppWritePackedMsg, end, &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_PUCCH_CONFIG_N1_PUCCH_AN_TAG,
                  &(pNfapiMsg->pucch_config.n1_pucch_an),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_EMBMS_MBSFN_CONFIG_AREA_IDX_TAG,
                  &(pNfapiMsg->embms_sib13_config.mbsfn_area_idx),
                  ppWritePackedMsg,
                  end,
                  &pack_uint8_tlv_value)
      && pack_tlv(NFAPI_EMBMS_MBSFN_CONFIG_AREA_IDR9_TAG,
                  &(pNfapiMsg->embms_sib13_config.mbsfn_area_id_r9),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_EMBMS_MBSFN_CONFIG_TAG,
                  &(pNfapiMsg->embms_mbsfn_config),
                  ppWritePackedMsg,
                  end,
                  &pack_embms_mbsfn_config_value)
      && pack_tlv(NFAPI_FEMBMS_CONFIG_RADIOFRAME_ALLOCATION_PERIOD_TAG,
                  &(pNfapiMsg->fembms_config.radioframe_allocation_period),
                  ppWritePackedMsg,
                  end,
                  &pack_uint8_tlv_value)
      && pack_tlv(NFAPI_FEMBMS_CONFIG_RADIOFRAME_ALLOCATION_OFFSET_TAG,
                  &(pNfapiMsg->fembms_config.radioframe_allocation_offset),
                  ppWritePackedMsg,
                  end,
                  &pack_uint8_tlv_value)
      && pack_tlv(NFAPI_FEMBMS_CONFIG_NON_MBSFN_FLAG_TAG,
                  &(pNfapiMsg->fembms_config.non_mbsfn_config_flag),
                  ppWritePackedMsg,
                  end,
                  &pack_uint8_tlv_value)
      && pack_tlv(NFAPI_FEMBMS_CONFIG_NON_MBSFN_SUBFRAMECONFIG_TAG,
                  &(pNfapiMsg->fembms_config.non_mbsfn_subframeconfig),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_SRS_CONFIG_BANDWIDTH_CONFIGURATION_TAG,
                  &(pNfapiMsg->srs_config.bandwidth_configuration),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_SRS_CONFIG_MAX_UP_PTS_TAG,
                  &(pNfapiMsg->srs_config.max_up_pts),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_SRS_CONFIG_SRS_SUBFRAME_CONFIGURATION_TAG,
                  &(pNfapiMsg->srs_config.srs_subframe_configuration),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_SRS_CONFIG_SRS_ACKNACK_SRS_SIMULTANEOUS_TRANSMISSION_TAG,
                  &(pNfapiMsg->srs_config.srs_acknack_srs_simultaneous_transmission),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_UPLINK_REFERENCE_SIGNAL_CONFIG_UPLINK_RS_HOPPING_TAG,
                  &(pNfapiMsg->uplink_reference_signal_config.uplink_rs_hopping),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_UPLINK_REFERENCE_SIGNAL_CONFIG_GROUP_ASSIGNMENT_TAG,
                  &(pNfapiMsg->uplink_reference_signal_config.group_assignment),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_UPLINK_REFERENCE_SIGNAL_CONFIG_CYCLIC_SHIFT_1_FOR_DRMS_TAG,
                  &(pNfapiMsg->uplink_reference_signal_config.cyclic_shift_1_for_drms),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_TDD_FRAME_STRUCTURE_SUBFRAME_ASSIGNMENT_TAG,
                  &(pNfapiMsg->tdd_frame_structure_config.subframe_assignment),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_TDD_FRAME_STRUCTURE_SPECIAL_SUBFRAME_PATTERNS_TAG,
                  &(pNfapiMsg->tdd_frame_structure_config.special_subframe_patterns),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_L23_CONFIG_DATA_REPORT_MODE_TAG,
                  &(pNfapiMsg->l23_config.data_report_mode),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_L23_CONFIG_SFNSF_TAG, &(pNfapiMsg->l23_config.sfnsf), ppWritePackedMsg, end, &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_NFAPI_P7_VNF_ADDRESS_IPV4_TAG,
                  &(pNfapiMsg->nfapi_config.p7_vnf_address_ipv4),
                  ppWritePackedMsg,
                  end,
                  &pack_ipv4_address_value)
      && pack_tlv(NFAPI_NFAPI_P7_VNF_ADDRESS_IPV6_TAG,
                  &(pNfapiMsg->nfapi_config.p7_vnf_address_ipv6),
                  ppWritePackedMsg,
                  end,
                  &pack_ipv6_address_value)
      && pack_tlv(NFAPI_NFAPI_P7_VNF_PORT_TAG,
                  &(pNfapiMsg->nfapi_config.p7_vnf_port),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_NFAPI_P7_PNF_ADDRESS_IPV4_TAG,
                  &(pNfapiMsg->nfapi_config.p7_pnf_address_ipv4),
                  ppWritePackedMsg,
                  end,
                  &pack_ipv4_address_value)
      && pack_tlv(NFAPI_NFAPI_P7_PNF_ADDRESS_IPV6_TAG,
                  &(pNfapiMsg->nfapi_config.p7_pnf_address_ipv6),
                  ppWritePackedMsg,
                  end,
                  &pack_ipv6_address_value)
      && pack_tlv(NFAPI_NFAPI_P7_PNF_PORT_TAG,
                  &(pNfapiMsg->nfapi_config.p7_pnf_port),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_NFAPI_DOWNLINK_UES_PER_SUBFRAME_TAG,
                  &(pNfapiMsg->nfapi_config.dl_ue_per_sf),
                  ppWritePackedMsg,
                  end,
                  &pack_uint8_tlv_value)
      && pack_tlv(NFAPI_NFAPI_UPLINK_UES_PER_SUBFRAME_TAG,
                  &(pNfapiMsg->nfapi_config.ul_ue_per_sf),
                  ppWritePackedMsg,
                  end,
                  &pack_uint8_tlv_value)
      && pack_tlv(NFAPI_NFAPI_RF_BANDS_TAG, &(pNfapiMsg->nfapi_config.rf_bands), ppWritePackedMsg, end, &pack_rf_bands_value)
      && pack_tlv(NFAPI_NFAPI_TIMING_WINDOW_TAG,
                  &(pNfapiMsg->nfapi_config.timing_window),
                  ppWritePackedMsg,
                  end,
                  &pack_uint8_tlv_value)
      && pack_tlv(NFAPI_NFAPI_TIMING_INFO_MODE_TAG,
                  &(pNfapiMsg->nfapi_config.timing_info_mode),
                  ppWritePackedMsg,
                  end,
                  &pack_uint8_tlv_value)
      && pack_tlv(NFAPI_NFAPI_TIMING_INFO_PERIOD_TAG,
                  &(pNfapiMsg->nfapi_config.timing_info_period),
                  ppWritePackedMsg,
                  end,
                  &pack_uint8_tlv_value)
      && pack_tlv(NFAPI_NFAPI_MAXIMUM_TRANSMIT_POWER_TAG,
                  &(pNfapiMsg->nfapi_config.max_transmit_power),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_NFAPI_EARFCN_TAG, &(pNfapiMsg->nfapi_config.earfcn), ppWritePackedMsg, end, &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_NFAPI_NMM_GSM_FREQUENCY_BANDS_TAG,
                  &(pNfapiMsg->nfapi_config.nmm_gsm_frequency_bands),
                  ppWritePackedMsg,
                  end,
                  &pack_nmm_frequency_bands_value)
      && pack_tlv(NFAPI_NFAPI_NMM_UMTS_FREQUENCY_BANDS_TAG,
                  &(pNfapiMsg->nfapi_config.nmm_umts_frequency_bands),
                  ppWritePackedMsg,
                  end,
                  &pack_nmm_frequency_bands_value)
      && pack_tlv(NFAPI_NFAPI_NMM_LTE_FREQUENCY_BANDS_TAG,
                  &(pNfapiMsg->nfapi_config.nmm_lte_frequency_bands),
                  ppWritePackedMsg,
                  end,
                  &pack_nmm_frequency_bands_value)
      && pack_tlv(NFAPI_NFAPI_NMM_UPLINK_RSSI_SUPPORTED_TAG,
                  &(pNfapiMsg->nfapi_config.nmm_uplink_rssi_supported),
                  ppWritePackedMsg,
                  end,
                  &pack_uint8_tlv_value)
      && pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

static uint8_t pack_config_request(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config)
{
  nfapi_config_request_t *pNfapiMsg = (nfapi_config_request_t *)msg;
  return (
      push8(pNfapiMsg->num_tlv, ppWritePackedMsg, end) &&
      // Do we check the phy state and then just fill those sepecified, however
      // we do not know the duplex mode, so just attempt to pack all and assumme
      // that the callee has set the right tlvs
      pack_tlv(NFAPI_SUBFRAME_CONFIG_DUPLEX_MODE_TAG,
               &(pNfapiMsg->subframe_config.duplex_mode),
               ppWritePackedMsg,
               end,
               &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_SUBFRAME_CONFIG_PCFICH_POWER_OFFSET_TAG,
                  &(pNfapiMsg->subframe_config.pcfich_power_offset),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_SUBFRAME_CONFIG_PB_TAG, &(pNfapiMsg->subframe_config.pb), ppWritePackedMsg, end, &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_SUBFRAME_CONFIG_DL_CYCLIC_PREFIX_TYPE_TAG,
                  &(pNfapiMsg->subframe_config.dl_cyclic_prefix_type),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_SUBFRAME_CONFIG_UL_CYCLIC_PREFIX_TYPE_TAG,
                  &(pNfapiMsg->subframe_config.ul_cyclic_prefix_type),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_RF_CONFIG_DL_CHANNEL_BANDWIDTH_TAG,
                  &(pNfapiMsg->rf_config.dl_channel_bandwidth),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_RF_CONFIG_UL_CHANNEL_BANDWIDTH_TAG,
                  &(pNfapiMsg->rf_config.ul_channel_bandwidth),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_RF_CONFIG_REFERENCE_SIGNAL_POWER_TAG,
                  &(pNfapiMsg->rf_config.reference_signal_power),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_RF_CONFIG_TX_ANTENNA_PORTS_TAG,
                  &(pNfapiMsg->rf_config.tx_antenna_ports),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_RF_CONFIG_RX_ANTENNA_PORTS_TAG,
                  &(pNfapiMsg->rf_config.rx_antenna_ports),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_PHICH_CONFIG_PHICH_RESOURCE_TAG,
                  &(pNfapiMsg->phich_config.phich_resource),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_PHICH_CONFIG_PHICH_DURATION_TAG,
                  &(pNfapiMsg->phich_config.phich_duration),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_PHICH_CONFIG_PHICH_POWER_OFFSET_TAG,
                  &(pNfapiMsg->phich_config.phich_power_offset),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_SCH_CONFIG_PRIMARY_SYNCHRONIZATION_SIGNAL_EPRE_EPRERS_TAG,
                  &(pNfapiMsg->sch_config.primary_synchronization_signal_epre_eprers),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_SCH_CONFIG_SECONDARY_SYNCHRONIZATION_SIGNAL_EPRE_EPRERS_TAG,
                  &(pNfapiMsg->sch_config.secondary_synchronization_signal_epre_eprers),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_SCH_CONFIG_PHYSICAL_CELL_ID_TAG,
                  &(pNfapiMsg->sch_config.physical_cell_id),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_PRACH_CONFIG_CONFIGURATION_INDEX_TAG,
                  &(pNfapiMsg->prach_config.configuration_index),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_PRACH_CONFIG_ROOT_SEQUENCE_INDEX_TAG,
                  &(pNfapiMsg->prach_config.root_sequence_index),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_PRACH_CONFIG_ZERO_CORRELATION_ZONE_CONFIGURATION_TAG,
                  &(pNfapiMsg->prach_config.zero_correlation_zone_configuration),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_PRACH_CONFIG_HIGH_SPEED_FLAG_TAG,
                  &(pNfapiMsg->prach_config.high_speed_flag),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_PRACH_CONFIG_FREQUENCY_OFFSET_TAG,
                  &(pNfapiMsg->prach_config.frequency_offset),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_PUSCH_CONFIG_HOPPING_MODE_TAG,
                  &(pNfapiMsg->pusch_config.hopping_mode),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_PUSCH_CONFIG_HOPPING_OFFSET_TAG,
                  &(pNfapiMsg->pusch_config.hopping_offset),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_PUSCH_CONFIG_NUMBER_OF_SUBBANDS_TAG,
                  &(pNfapiMsg->pusch_config.number_of_subbands),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_PUCCH_CONFIG_DELTA_PUCCH_SHIFT_TAG,
                  &(pNfapiMsg->pucch_config.delta_pucch_shift),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_PUCCH_CONFIG_N_CQI_RB_TAG,
                  &(pNfapiMsg->pucch_config.n_cqi_rb),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_PUCCH_CONFIG_N_AN_CS_TAG, &(pNfapiMsg->pucch_config.n_an_cs), ppWritePackedMsg, end, &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_PUCCH_CONFIG_N1_PUCCH_AN_TAG,
                  &(pNfapiMsg->pucch_config.n1_pucch_an),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_SRS_CONFIG_BANDWIDTH_CONFIGURATION_TAG,
                  &(pNfapiMsg->srs_config.bandwidth_configuration),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_SRS_CONFIG_MAX_UP_PTS_TAG,
                  &(pNfapiMsg->srs_config.max_up_pts),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_SRS_CONFIG_SRS_SUBFRAME_CONFIGURATION_TAG,
                  &(pNfapiMsg->srs_config.srs_subframe_configuration),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_SRS_CONFIG_SRS_ACKNACK_SRS_SIMULTANEOUS_TRANSMISSION_TAG,
                  &(pNfapiMsg->srs_config.srs_acknack_srs_simultaneous_transmission),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_UPLINK_REFERENCE_SIGNAL_CONFIG_UPLINK_RS_HOPPING_TAG,
                  &(pNfapiMsg->uplink_reference_signal_config.uplink_rs_hopping),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_UPLINK_REFERENCE_SIGNAL_CONFIG_GROUP_ASSIGNMENT_TAG,
                  &(pNfapiMsg->uplink_reference_signal_config.group_assignment),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_UPLINK_REFERENCE_SIGNAL_CONFIG_CYCLIC_SHIFT_1_FOR_DRMS_TAG,
                  &(pNfapiMsg->uplink_reference_signal_config.cyclic_shift_1_for_drms),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_LAA_CONFIG_ED_THRESHOLD_FOR_LBT_FOR_PDSCH_TAG,
                  &(pNfapiMsg->laa_config.ed_threshold_lbt_pdsch),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_LAA_CONFIG_ED_THRESHOLD_FOR_LBT_FOR_DRS_TAG,
                  &(pNfapiMsg->laa_config.ed_threshold_lbt_drs),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_LAA_CONFIG_PD_THRESHOLD_TAG,
                  &(pNfapiMsg->laa_config.pd_threshold),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_LAA_CONFIG_MULTI_CARRIER_TYPE_TAG,
                  &(pNfapiMsg->laa_config.multi_carrier_type),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_LAA_CONFIG_MULTI_CARRIER_TX_TAG,
                  &(pNfapiMsg->laa_config.multi_carrier_tx),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_LAA_CONFIG_MULTI_CARRIER_FREEZE_TAG,
                  &(pNfapiMsg->laa_config.multi_carrier_freeze),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_LAA_CONFIG_TX_ANTENNA_PORTS_FOR_DRS_TAG,
                  &(pNfapiMsg->laa_config.tx_antenna_ports_drs),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_LAA_CONFIG_TRANSMISSION_POWER_FOR_DRS_TAG,
                  &(pNfapiMsg->laa_config.tx_power_drs),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_EMTC_CONFIG_PBCH_REPETITIONS_ENABLE_R13_TAG,
                  &(pNfapiMsg->emtc_config.pbch_repetitions_enable_r13),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_EMTC_CONFIG_PRACH_CATM_ROOT_SEQUENCE_INDEX_TAG,
                  &(pNfapiMsg->emtc_config.prach_catm_root_sequence_index),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_EMTC_CONFIG_PRACH_CATM_ZERO_CORRELATION_ZONE_CONFIGURATION_TAG,
                  &(pNfapiMsg->emtc_config.prach_catm_zero_correlation_zone_configuration),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_EMTC_CONFIG_PRACH_CATM_HIGH_SPEED_FLAG,
                  &(pNfapiMsg->emtc_config.prach_catm_high_speed_flag),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_ENABLE_TAG,
                  &(pNfapiMsg->emtc_config.prach_ce_level_0_enable),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_CONFIGURATION_INDEX_TAG,
                  &(pNfapiMsg->emtc_config.prach_ce_level_0_configuration_index),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_FREQUENCY_OFFSET_TAG,
                  &(pNfapiMsg->emtc_config.prach_ce_level_0_frequency_offset),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG,
                  &(pNfapiMsg->emtc_config.prach_ce_level_0_number_of_repetitions_per_attempt),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_STARTING_SUBFRAME_PERIODICITY_TAG,
                  &(pNfapiMsg->emtc_config.prach_ce_level_0_starting_subframe_periodicity),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_HOPPING_ENABLE_TAG,
                  &(pNfapiMsg->emtc_config.prach_ce_level_0_hopping_enable),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_HOPPING_OFFSET_TAG,
                  &(pNfapiMsg->emtc_config.prach_ce_level_0_hopping_offset),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_ENABLE_TAG,
                  &(pNfapiMsg->emtc_config.prach_ce_level_1_enable),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_CONFIGURATION_INDEX_TAG,
                  &(pNfapiMsg->emtc_config.prach_ce_level_1_configuration_index),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_FREQUENCY_OFFSET_TAG,
                  &(pNfapiMsg->emtc_config.prach_ce_level_1_frequency_offset),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG,
                  &(pNfapiMsg->emtc_config.prach_ce_level_1_number_of_repetitions_per_attempt),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_STARTING_SUBFRAME_PERIODICITY_TAG,
                  &(pNfapiMsg->emtc_config.prach_ce_level_1_starting_subframe_periodicity),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_HOPPING_ENABLE_TAG,
                  &(pNfapiMsg->emtc_config.prach_ce_level_1_hopping_enable),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_HOPPING_OFFSET_TAG,
                  &(pNfapiMsg->emtc_config.prach_ce_level_1_hopping_offset),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_ENABLE_TAG,
                  &(pNfapiMsg->emtc_config.prach_ce_level_2_enable),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_CONFIGURATION_INDEX_TAG,
                  &(pNfapiMsg->emtc_config.prach_ce_level_2_configuration_index),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_FREQUENCY_OFFSET_TAG,
                  &(pNfapiMsg->emtc_config.prach_ce_level_2_frequency_offset),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG,
                  &(pNfapiMsg->emtc_config.prach_ce_level_2_number_of_repetitions_per_attempt),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_STARTING_SUBFRAME_PERIODICITY_TAG,
                  &(pNfapiMsg->emtc_config.prach_ce_level_2_starting_subframe_periodicity),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_HOPPING_ENABLE_TAG,
                  &(pNfapiMsg->emtc_config.prach_ce_level_2_hopping_enable),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_HOPPING_OFFSET_TAG,
                  &(pNfapiMsg->emtc_config.prach_ce_level_2_hopping_offset),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_ENABLE_TAG,
                  &(pNfapiMsg->emtc_config.prach_ce_level_3_enable),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_CONFIGURATION_INDEX_TAG,
                  &(pNfapiMsg->emtc_config.prach_ce_level_3_configuration_index),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_FREQUENCY_OFFSET_TAG,
                  &(pNfapiMsg->emtc_config.prach_ce_level_3_frequency_offset),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG,
                  &(pNfapiMsg->emtc_config.prach_ce_level_3_number_of_repetitions_per_attempt),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_STARTING_SUBFRAME_PERIODICITY_TAG,
                  &(pNfapiMsg->emtc_config.prach_ce_level_3_starting_subframe_periodicity),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_HOPPING_ENABLE_TAG,
                  &(pNfapiMsg->emtc_config.prach_ce_level_3_hopping_enable),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_HOPPING_OFFSET_TAG,
                  &(pNfapiMsg->emtc_config.prach_ce_level_3_hopping_offset),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_EMTC_CONFIG_PUCCH_INTERVAL_ULHOPPINGCONFIGCOMMONMODEA_TAG,
                  &(pNfapiMsg->emtc_config.pucch_interval_ulhoppingconfigcommonmodea),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_EMTC_CONFIG_PUCCH_INTERVAL_ULHOPPINGCONFIGCOMMONMODEB_TAG,
                  &(pNfapiMsg->emtc_config.pucch_interval_ulhoppingconfigcommonmodeb),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_TDD_FRAME_STRUCTURE_SUBFRAME_ASSIGNMENT_TAG,
                  &(pNfapiMsg->tdd_frame_structure_config.subframe_assignment),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_TDD_FRAME_STRUCTURE_SPECIAL_SUBFRAME_PATTERNS_TAG,
                  &(pNfapiMsg->tdd_frame_structure_config.special_subframe_patterns),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_L23_CONFIG_DATA_REPORT_MODE_TAG,
                  &(pNfapiMsg->l23_config.data_report_mode),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_L23_CONFIG_SFNSF_TAG, &(pNfapiMsg->l23_config.sfnsf), ppWritePackedMsg, end, &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_NFAPI_P7_VNF_ADDRESS_IPV4_TAG,
                  &(pNfapiMsg->nfapi_config.p7_vnf_address_ipv4),
                  ppWritePackedMsg,
                  end,
                  &pack_ipv4_address_value)
      && pack_tlv(NFAPI_NFAPI_P7_VNF_ADDRESS_IPV6_TAG,
                  &(pNfapiMsg->nfapi_config.p7_vnf_address_ipv6),
                  ppWritePackedMsg,
                  end,
                  &pack_ipv6_address_value)
      && pack_tlv(NFAPI_NFAPI_P7_VNF_PORT_TAG,
                  &(pNfapiMsg->nfapi_config.p7_vnf_port),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_NFAPI_P7_PNF_ADDRESS_IPV4_TAG,
                  &(pNfapiMsg->nfapi_config.p7_pnf_address_ipv4),
                  ppWritePackedMsg,
                  end,
                  &pack_ipv4_address_value)
      && pack_tlv(NFAPI_NFAPI_P7_PNF_ADDRESS_IPV6_TAG,
                  &(pNfapiMsg->nfapi_config.p7_pnf_address_ipv6),
                  ppWritePackedMsg,
                  end,
                  &pack_ipv6_address_value)
      && pack_tlv(NFAPI_NFAPI_P7_PNF_PORT_TAG,
                  &(pNfapiMsg->nfapi_config.p7_pnf_port),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_NFAPI_DOWNLINK_UES_PER_SUBFRAME_TAG,
                  &(pNfapiMsg->nfapi_config.dl_ue_per_sf),
                  ppWritePackedMsg,
                  end,
                  &pack_uint8_tlv_value)
      && pack_tlv(NFAPI_NFAPI_UPLINK_UES_PER_SUBFRAME_TAG,
                  &(pNfapiMsg->nfapi_config.ul_ue_per_sf),
                  ppWritePackedMsg,
                  end,
                  &pack_uint8_tlv_value)
      && pack_tlv(NFAPI_PHY_RF_BANDS_TAG, &(pNfapiMsg->nfapi_config.rf_bands), ppWritePackedMsg, end, &pack_rf_bands_value)
      && pack_tlv(NFAPI_NFAPI_TIMING_WINDOW_TAG,
                  &(pNfapiMsg->nfapi_config.timing_window),
                  ppWritePackedMsg,
                  end,
                  &pack_uint8_tlv_value)
      && pack_tlv(NFAPI_NFAPI_TIMING_INFO_MODE_TAG,
                  &(pNfapiMsg->nfapi_config.timing_info_mode),
                  ppWritePackedMsg,
                  end,
                  &pack_uint8_tlv_value)
      && pack_tlv(NFAPI_NFAPI_TIMING_INFO_PERIOD_TAG,
                  &(pNfapiMsg->nfapi_config.timing_info_period),
                  ppWritePackedMsg,
                  end,
                  &pack_uint8_tlv_value)
      && pack_tlv(NFAPI_NFAPI_MAXIMUM_TRANSMIT_POWER_TAG,
                  &(pNfapiMsg->nfapi_config.max_transmit_power),
                  ppWritePackedMsg,
                  end,
                  &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_NFAPI_EARFCN_TAG, &(pNfapiMsg->nfapi_config.earfcn), ppWritePackedMsg, end, &pack_uint16_tlv_value)
      && pack_tlv(NFAPI_NFAPI_NMM_GSM_FREQUENCY_BANDS_TAG,
                  &(pNfapiMsg->nfapi_config.nmm_gsm_frequency_bands),
                  ppWritePackedMsg,
                  end,
                  &pack_nmm_frequency_bands_value)
      && pack_tlv(NFAPI_NFAPI_NMM_UMTS_FREQUENCY_BANDS_TAG,
                  &(pNfapiMsg->nfapi_config.nmm_umts_frequency_bands),
                  ppWritePackedMsg,
                  end,
                  &pack_nmm_frequency_bands_value)
      && pack_tlv(NFAPI_NFAPI_NMM_LTE_FREQUENCY_BANDS_TAG,
                  &(pNfapiMsg->nfapi_config.nmm_lte_frequency_bands),
                  ppWritePackedMsg,
                  end,
                  &pack_nmm_frequency_bands_value)
      && pack_tlv(NFAPI_NFAPI_NMM_UPLINK_RSSI_SUPPORTED_TAG,
                  &(pNfapiMsg->nfapi_config.nmm_uplink_rssi_supported),
                  ppWritePackedMsg,
                  end,
                  &pack_uint8_tlv_value));
}

static uint8_t pack_config_response(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  nfapi_config_response_t *pNfapiMsg = (nfapi_config_response_t *)msg;
  return ( push32(pNfapiMsg->error_code, ppWritePackedMsg, end) &&
           pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config) );
}

static uint8_t pack_start_request(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  nfapi_start_request_t *pNfapiMsg = (nfapi_start_request_t *)msg;
  return pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config);
}

static uint8_t pack_start_response(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  nfapi_start_response_t *pNfapiMsg = (nfapi_start_response_t *)msg;
  return ( push32(pNfapiMsg->error_code, ppWritePackedMsg, end ) &&
           pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config) );
}

static uint8_t pack_stop_request(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config)
{
  nfapi_stop_request_t *pNfapiMsg = (nfapi_stop_request_t *)msg;
  return pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config);
}

static uint8_t pack_recevied_interference_power_measurement_value(void *tlv, uint8_t **ppWritePackedMsg, uint8_t *end) {
  nfapi_received_interference_power_measurement_t *value = (nfapi_received_interference_power_measurement_t *)tlv;
  return  ( push16(value->number_of_resource_blocks, ppWritePackedMsg, end) &&
            pusharrays16(value->received_interference_power, NFAPI_MAX_RECEIVED_INTERFERENCE_POWER_RESULTS, value->number_of_resource_blocks, ppWritePackedMsg, end));
}

static uint8_t pack_measurement_response(void *msg, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  nfapi_measurement_response_t *pNfapiMsg = (nfapi_measurement_response_t *)msg;
  return( push32(pNfapiMsg->error_code, ppWritePackedMsg, end) &&
          pack_tlv(NFAPI_MEASUREMENT_RESPONSE_DL_RS_POWER_MEASUREMENT_TAG, &(pNfapiMsg->dl_rs_tx_power_measurement), ppWritePackedMsg, end, &pack_int16_tlv_value) &&
          pack_tlv(NFAPI_MEASUREMENT_RESPONSE_RECEIVED_INTERFERENCE_POWER_MEASUREMENT_TAG, &(pNfapiMsg->received_interference_power_measurement), ppWritePackedMsg, end,
                   &pack_recevied_interference_power_measurement_value) &&
          pack_tlv(NFAPI_MEASUREMENT_RESPONSE_THERMAL_NOISE_MEASUREMENT_TAG, &(pNfapiMsg->thermal_noise_power_measurement), ppWritePackedMsg, end, &pack_uint16_tlv_value) &&
          pack_vendor_extension_tlv(pNfapiMsg->vendor_extension, ppWritePackedMsg, end, config));
}

uint8_t pack_nr_p5_message_body(nfapi_nr_p4_p5_message_header_t *header,
                                uint8_t **ppWritePackedMsg,
                                uint8_t *end,
                                nfapi_p4_p5_codec_config_t *config)
{
  uint8_t result = 0;

  // look for the specific message
  switch (header->message_id) {
    case NFAPI_NR_PHY_MSG_TYPE_PNF_PARAM_REQUEST:
      result = pack_nr_pnf_param_request(header, ppWritePackedMsg, end, config);
      break;

    case NFAPI_PNF_PARAM_RESPONSE:
      result = pack_nr_pnf_param_response(header, ppWritePackedMsg, end, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_PNF_CONFIG_REQUEST:
      result = pack_nr_pnf_config_request(header, ppWritePackedMsg, end, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_PNF_CONFIG_RESPONSE:
      result = pack_nr_pnf_config_response(header, ppWritePackedMsg, end, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_PNF_START_REQUEST:
      result = pack_nr_pnf_start_request(header, ppWritePackedMsg, end, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_PNF_START_RESPONSE:
      result = pack_nr_pnf_start_response(header, ppWritePackedMsg, end, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_PNF_STOP_REQUEST:
      result = pack_nr_pnf_stop_request(header, ppWritePackedMsg, end, config);
      break;

    case NFAPI_PNF_STOP_RESPONSE:
      result = pack_nr_pnf_stop_response(header, ppWritePackedMsg, end, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_PARAM_REQUEST:
      result = pack_nr_param_request(header, ppWritePackedMsg, end, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_PARAM_RESPONSE:
      result = pack_nr_param_response(header, ppWritePackedMsg, end, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_CONFIG_REQUEST:
      result = pack_nr_config_request(header, ppWritePackedMsg, end, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_CONFIG_RESPONSE:
      result = pack_nr_config_response(header, ppWritePackedMsg, end, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_START_REQUEST:
      result = pack_nr_start_request(header, ppWritePackedMsg, end, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_START_RESPONSE:
      result = pack_nr_start_response(header, ppWritePackedMsg, end, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_STOP_REQUEST:
      result = pack_stop_request(header, ppWritePackedMsg, end, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_STOP_RESPONSE:
      result = pack_stop_response(header, ppWritePackedMsg, end, config);
      break;

    default: {
      if (header->message_id >= NFAPI_VENDOR_EXT_MSG_MIN && header->message_id <= NFAPI_VENDOR_EXT_MSG_MAX) {
        if (config && config->pack_p4_p5_vendor_extension) {
          result = (config->pack_p4_p5_vendor_extension)(header, ppWritePackedMsg, end, config);
        } else {
          NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s VE NFAPI message ID %d. No ve ecoder provided\n", __FUNCTION__, header->message_id);
        }
      } else {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s NFAPI Unknown message ID %d\n", __FUNCTION__, header->message_id);
      }
    } break;
  }

  return result;
}

static uint8_t pack_p5_message_body(nfapi_p4_p5_message_header_t *header, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  uint8_t result = 0;

  // look for the specific message
  switch (header->message_id) {
    case NFAPI_PNF_PARAM_REQUEST:
      result = pack_pnf_param_request(header, ppWritePackedMsg, end, config);
      break;

    case NFAPI_PNF_PARAM_RESPONSE:
      result = pack_pnf_param_response(header, ppWritePackedMsg, end, config);
      break;

    case NFAPI_PNF_CONFIG_REQUEST:
      result = pack_pnf_config_request(header, ppWritePackedMsg, end, config);
      break;

    case NFAPI_PNF_CONFIG_RESPONSE:
      result = pack_pnf_config_response(header, ppWritePackedMsg, end, config);
      break;

    case NFAPI_PNF_START_REQUEST:
      result = pack_pnf_start_request(header, ppWritePackedMsg, end, config);
      break;

    case NFAPI_PNF_START_RESPONSE:
      result = pack_pnf_start_response(header, ppWritePackedMsg, end, config);
      break;

    case NFAPI_PNF_STOP_REQUEST:
      result = pack_pnf_stop_request(header, ppWritePackedMsg, end, config);
      break;

    case NFAPI_PNF_STOP_RESPONSE:
      result = pack_pnf_stop_response(header, ppWritePackedMsg, end, config);
      break;

    case NFAPI_PARAM_REQUEST:
      result = pack_param_request(header, ppWritePackedMsg, end, config);
      break;

    case NFAPI_PARAM_RESPONSE:
      result = pack_param_response(header, ppWritePackedMsg, end, config);
      break;

    case NFAPI_CONFIG_REQUEST:
      result = pack_config_request(header, ppWritePackedMsg, end, config);
      break;

    case NFAPI_CONFIG_RESPONSE:
      result = pack_config_response(header, ppWritePackedMsg, end, config);
      break;

    case NFAPI_START_REQUEST:
      result = pack_start_request(header, ppWritePackedMsg, end, config);
      break;

    case NFAPI_START_RESPONSE:
      result = pack_start_response(header, ppWritePackedMsg, end, config);
      break;

    case NFAPI_STOP_REQUEST:
      result = pack_stop_request(header, ppWritePackedMsg, end, config);
      break;

    case NFAPI_STOP_RESPONSE:
      result = pack_stop_response(header, ppWritePackedMsg, end, config);
      break;

    case NFAPI_MEASUREMENT_REQUEST:
      result = pack_measurement_request(header, ppWritePackedMsg, end, config);
      break;

    case NFAPI_MEASUREMENT_RESPONSE:
      result = pack_measurement_response(header, ppWritePackedMsg, end, config);
      break;

    default: {
      if(header->message_id >= NFAPI_VENDOR_EXT_MSG_MIN &&
          header->message_id <= NFAPI_VENDOR_EXT_MSG_MAX) {
        if(config && config->pack_p4_p5_vendor_extension) {
          result = (config->pack_p4_p5_vendor_extension)(header, ppWritePackedMsg, end, config);
        } else {
          NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s VE NFAPI message ID %d. No ve ecoder provided\n", __FUNCTION__, header->message_id);
        }
      } else {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s NFAPI Unknown message ID %d\n", __FUNCTION__, header->message_id);
      }
    }
    break;
  }

  return result;
}

// Main pack function - public

int nfapi_nr_p5_message_pack(void *pMessageBuf,
                             uint32_t messageBufLen,
                             void *pPackedBuf,
                             uint32_t packedBufLen,
                             nfapi_p4_p5_codec_config_t *config)
{
  nfapi_nr_p4_p5_message_header_t *pMessageHeader = pMessageBuf;
  uint8_t *pWritePackedMessage = pPackedBuf;

  if (pMessageBuf == NULL || pPackedBuf == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P5 Pack supplied pointers are null\n");
    return -1;
  }

  uint8_t *pPackMessageEnd = pPackedBuf + packedBufLen;
  uint8_t *pPackedLengthField = &pWritePackedMessage[4];
  uint8_t *pPacketBodyFieldStart = &pWritePackedMessage[10];
  uint8_t *pPacketBodyField = &pWritePackedMessage[10];

  // pack the message
  if (push16(pMessageHeader->phy_id, &pWritePackedMessage, pPackMessageEnd)
      && push16(pMessageHeader->message_id, &pWritePackedMessage, pPackMessageEnd)
      && push32(0, &pWritePackedMessage, pPackMessageEnd) && push16(pMessageHeader->spare, &pWritePackedMessage, pPackMessageEnd)
      && pack_nr_p5_message_body(pMessageHeader, &pPacketBodyField, pPackMessageEnd, config)) {
    // to check if whole message is bigger than the buffer provided
    uint32_t packedMsgLen = get_packed_msg_len((uintptr_t)pPackedBuf, (uintptr_t)pPacketBodyField);
    // obtain the length of the message body to pack
    uint32_t packedBodyLen = get_packed_msg_len((uintptr_t)pPacketBodyFieldStart, (uintptr_t)pPacketBodyField);

    if (packedMsgLen > packedBufLen) {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "Packed message length error %d, buffer supplied %d\n", packedMsgLen, packedBufLen);
      return -1;
    }

    // Update the message length in the header
    if (!push32(packedBodyLen, &pPackedLengthField, pPackMessageEnd))
      return -1;

    // return the packed length
    return (packedMsgLen);
  } else {
    // Failed to pack the meassage
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P5 Failed to pack message\n");
    return -1;
  }
}

int nfapi_p5_message_pack(void *pMessageBuf, uint32_t messageBufLen, void *pPackedBuf, uint32_t packedBufLen, nfapi_p4_p5_codec_config_t *config) {
  nfapi_p4_p5_message_header_t *pMessageHeader = pMessageBuf;
  uint8_t *pWritePackedMessage = pPackedBuf;
  uint8_t *pPackedLengthField = &pWritePackedMessage[4];
  uint32_t packedMsgLen;
  uint16_t packedMsgLen16;

  if (pMessageBuf == NULL || pPackedBuf == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P5 Pack supplied pointers are null\n");
    return -1;
  }

  uint8_t *pPackMessageEnd = pPackedBuf + packedBufLen;

  // pack the message
  if(push16(pMessageHeader->phy_id, &pWritePackedMessage, pPackMessageEnd) &&
      push16(pMessageHeader->message_id, &pWritePackedMessage, pPackMessageEnd) &&
      push16(0/*pMessageHeader->message_length*/, &pWritePackedMessage, pPackMessageEnd) &&
      push16(pMessageHeader->spare, &pWritePackedMessage, pPackMessageEnd) &&
      pack_p5_message_body(pMessageHeader, &pWritePackedMessage, pPackMessageEnd, config)) {
    // check for a valid message length
    packedMsgLen = get_packed_msg_len((uintptr_t)pPackedBuf, (uintptr_t)pWritePackedMessage);

    if (packedMsgLen > 0xFFFF || packedMsgLen > packedBufLen) {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "Packed message length error %d, buffer supplied %d\n", packedMsgLen, packedBufLen);
      return -1;
    } else {
      packedMsgLen16 = (uint16_t)packedMsgLen;
    }

    // Update the message length in the header
    if(!push16(packedMsgLen16, &pPackedLengthField, pPackMessageEnd))
      return -1;

    // return the packed length
    return (packedMsgLen);
  } else {
    // Failed to pack the meassage
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P5 Failed to pack message\n");
    return -1;
  }
}

// Unpack routines

static uint8_t unpack_nr_pnf_param_request(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config)
{
  nfapi_nr_pnf_param_request_t *pNfapiMsg = (nfapi_nr_pnf_param_request_t *)msg;
  return unpack_nr_tlv_list(NULL, 0, ppReadPackedMsg, end, config, &(pNfapiMsg->vendor_extension));
}

static uint8_t  unpack_pnf_param_request(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config) {
  nfapi_pnf_param_request_t *pNfapiMsg = (nfapi_pnf_param_request_t *)msg;
  return unpack_tlv_list(NULL, 0, ppReadPackedMsg, end, config, &(pNfapiMsg->vendor_extension));
}

static uint8_t unpack_pnf_rf_info(void *elem, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_pnf_rf_info_t *rf = (nfapi_pnf_rf_info_t *)elem;
  return( pull16(ppReadPackedMsg, &rf->rf_config_index, end) &&
          pull16(ppReadPackedMsg, &rf->band, end) &&
          pulls16(ppReadPackedMsg, &rf->maximum_transmit_power, end) &&
          pulls16(ppReadPackedMsg, &rf->minimum_transmit_power, end) &&
          pull8(ppReadPackedMsg, &rf->number_of_antennas_suppported, end) &&
          pull32(ppReadPackedMsg, &rf->minimum_downlink_frequency, end) &&
          pull32(ppReadPackedMsg, &rf->maximum_downlink_frequency, end) &&
          pull32(ppReadPackedMsg, &rf->minimum_uplink_frequency, end) &&
          pull32(ppReadPackedMsg, &rf->maximum_uplink_frequency, end));
}

static uint8_t unpack_pnf_rf_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_pnf_rf_t *value = (nfapi_pnf_rf_t *)tlv;
  return ( pull16(ppReadPackedMsg, &value->number_of_rfs, end) &&
           unpackarray(ppReadPackedMsg, value->rf, sizeof(nfapi_pnf_rf_info_t), NFAPI_MAX_PNF_RF, value->number_of_rfs, end, &unpack_pnf_rf_info));
}

static uint8_t unpack_pnf_phy_rel10_info(void *elem, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_pnf_phy_rel10_info_t *phy = (nfapi_pnf_phy_rel10_info_t *)elem;
  return( pull16(ppReadPackedMsg, &phy->phy_config_index, end) &&
          pull16(ppReadPackedMsg, &phy->transmission_mode_7_supported, end) &&
          pull16(ppReadPackedMsg, &phy->transmission_mode_8_supported, end) &&
          pull16(ppReadPackedMsg, &phy->two_antenna_ports_for_pucch, end) &&
          pull16(ppReadPackedMsg, &phy->transmission_mode_9_supported, end) &&
          pull16(ppReadPackedMsg, &phy->simultaneous_pucch_pusch, end) &&
          pull16(ppReadPackedMsg, &phy->four_layer_tx_with_tm3_and_tm4, end));
}

static uint8_t unpack_pnf_phy_rel10_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_pnf_phy_rel10_t *value = (nfapi_pnf_phy_rel10_t *)tlv;
  return ( pull16(ppReadPackedMsg, &value->number_of_phys, end) &&
           unpackarray(ppReadPackedMsg, value->phy, sizeof(nfapi_pnf_phy_rel10_info_t), NFAPI_MAX_PNF_PHY, value->number_of_phys, end, &unpack_pnf_phy_rel10_info));
}

static uint8_t unpack_pnf_phy_rel11_info(void *elem, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_pnf_phy_rel11_info_t *phy = (nfapi_pnf_phy_rel11_info_t *)elem;
  return( pull16(ppReadPackedMsg, &phy->phy_config_index, end) &&
          pull16(ppReadPackedMsg, &phy->edpcch_supported, end) &&
          pull16(ppReadPackedMsg, &phy->multi_ack_csi_reporting, end ) &&
          pull16(ppReadPackedMsg, &phy->pucch_tx_diversity, end) &&
          pull16(ppReadPackedMsg, &phy->ul_comp_supported, end) &&
          pull16(ppReadPackedMsg, &phy->transmission_mode_5_supported, end));
}

static uint8_t unpack_pnf_phy_rel11_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_pnf_phy_rel11_t *value = (nfapi_pnf_phy_rel11_t *)tlv;
  return ( pull16(ppReadPackedMsg, &value->number_of_phys, end) &&
           unpackarray(ppReadPackedMsg, value->phy, sizeof(nfapi_pnf_phy_rel11_info_t), NFAPI_MAX_PNF_PHY, value->number_of_phys, end, &unpack_pnf_phy_rel11_info));
}

static uint8_t unpack_phy_phy_rel12_info(void *elem, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_pnf_phy_rel12_info_t *phy = (nfapi_pnf_phy_rel12_info_t *)elem;
  return( pull16(ppReadPackedMsg, &phy->phy_config_index, end) &&
          pull16(ppReadPackedMsg, &phy->csi_subframe_set, end) &&
          pull16(ppReadPackedMsg, &phy->enhanced_4tx_codebook, end) &&
          pull16(ppReadPackedMsg, &phy->drs_supported, end) &&
          pull16(ppReadPackedMsg, &phy->ul_64qam_supported, end) &&
          pull16(ppReadPackedMsg, &phy->transmission_mode_10_supported, end) &&
          pull16(ppReadPackedMsg, &phy->alternative_bts_indices, end));
}

static uint8_t unpack_pnf_phy_rel12_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_pnf_phy_rel12_t *value = (nfapi_pnf_phy_rel12_t *)tlv;
  return (pull16(ppReadPackedMsg, &value->number_of_phys, end) &&
          unpackarray(ppReadPackedMsg, value->phy, sizeof(nfapi_pnf_phy_rel12_t), NFAPI_MAX_PNF_PHY, value->number_of_phys, end, &unpack_phy_phy_rel12_info));
}

static uint8_t unpack_pnf_phy_rel13_info(void *elem, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_pnf_phy_rel13_info_t *phy = (nfapi_pnf_phy_rel13_info_t *)elem;
  return( pull16(ppReadPackedMsg, &phy->phy_config_index, end) &&
          pull16(ppReadPackedMsg, &phy->pucch_format4_supported, end) &&
          pull16(ppReadPackedMsg, &phy->pucch_format5_supported, end) &&
          pull16(ppReadPackedMsg, &phy->more_than_5_ca_support, end) &&
          pull16(ppReadPackedMsg, &phy->laa_supported, end) &&
          pull16(ppReadPackedMsg, &phy->laa_ending_in_dwpts_supported, end) &&
          pull16(ppReadPackedMsg, &phy->laa_starting_in_second_slot_supported, end) &&
          pull16(ppReadPackedMsg, &phy->beamforming_supported, end) &&
          pull16(ppReadPackedMsg, &phy->csi_rs_enhancement_supported, end) &&
          pull16(ppReadPackedMsg, &phy->drms_enhancement_supported, end) &&
          pull16(ppReadPackedMsg, &phy->srs_enhancement_supported, end));
}

static uint8_t unpack_pnf_phy_rel13_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_pnf_phy_rel13_t *value = (nfapi_pnf_phy_rel13_t *)tlv;
  return ( pull16(ppReadPackedMsg, &value->number_of_phys, end) &&
           unpackarray(ppReadPackedMsg, value->phy, sizeof(nfapi_pnf_phy_rel13_info_t), NFAPI_MAX_PNF_PHY, value->number_of_phys, end, &unpack_pnf_phy_rel13_info));
}

static uint8_t unpack_pnf_phy_rel13_nb_info_info(void *elem, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_pnf_phy_rel13_nb_iot_info_t *phy = (nfapi_pnf_phy_rel13_nb_iot_info_t *)elem;
  return( pull16(ppReadPackedMsg, &phy->phy_config_index, end) &&
          pull16(ppReadPackedMsg, &phy->number_of_rfs, end) &&
          unpackarray(ppReadPackedMsg, phy->rf_config, sizeof(nfapi_rf_config_info_t), NFAPI_MAX_PNF_PHY_RF_CONFIG, phy->number_of_rfs, end, &unpack_rf_config_info) &&
          pull16(ppReadPackedMsg, &phy->number_of_rf_exclusions, end) &&
          unpackarray(ppReadPackedMsg, phy->excluded_rf_config, sizeof(nfapi_rf_config_info_t), NFAPI_MAX_PNF_PHY_RF_CONFIG, phy->number_of_rf_exclusions, end, &unpack_rf_config_info) &&
          pull8(ppReadPackedMsg, &phy->number_of_dl_layers_supported, end) &&
          pull8(ppReadPackedMsg, &phy->number_of_ul_layers_supported, end) &&
          pull16(ppReadPackedMsg, &phy->maximum_3gpp_release_supported, end) &&
          pull8(ppReadPackedMsg, &phy->nmm_modes_supported, end));
}

static uint8_t unpack_pnf_phy_rel13_nb_iot_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_pnf_phy_rel13_nb_iot_t *value = (nfapi_pnf_phy_rel13_nb_iot_t *)tlv;
  return ( pull16(ppReadPackedMsg, &value->number_of_phys, end) &&
           unpackarray(ppReadPackedMsg, value->phy, sizeof(nfapi_pnf_phy_rel13_nb_iot_info_t), NFAPI_MAX_PNF_PHY, value->number_of_phys, end, &unpack_pnf_phy_rel13_nb_info_info));
}

static uint8_t unpack_nr_pnf_param_response(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config)
{
  nfapi_nr_pnf_param_response_t *pNfapiMsg = (nfapi_nr_pnf_param_response_t *)msg;
  unpack_tlv_t unpack_fns[] = {
      {NFAPI_PNF_PARAM_GENERAL_TAG, &pNfapiMsg->pnf_param_general, &unpack_pnf_param_general_value},
      {NFAPI_PNF_PHY_TAG, &pNfapiMsg->pnf_phy, &unpack_pnf_phy_value},
  };
  return (pull8(ppReadPackedMsg, (uint8_t *)&pNfapiMsg->error_code, end)
          && pull8(ppReadPackedMsg, (uint8_t *)&pNfapiMsg->num_tlvs, end)
          && unpack_nr_tlv_list(unpack_fns,
                                sizeof(unpack_fns) / sizeof(unpack_tlv_t),
                                ppReadPackedMsg,
                                end,
                                config,
                                &pNfapiMsg->vendor_extension));
}

static uint8_t unpack_pnf_param_response(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config) {
  nfapi_pnf_param_response_t *pNfapiMsg = (nfapi_pnf_param_response_t *)msg;
  unpack_tlv_t unpack_fns[] = {
    { NFAPI_PNF_PARAM_GENERAL_TAG, &pNfapiMsg->pnf_param_general, &unpack_pnf_param_general_value},
    { NFAPI_PNF_PHY_TAG, &pNfapiMsg->pnf_phy, &unpack_pnf_phy_value},
    { NFAPI_PNF_RF_TAG, &pNfapiMsg->pnf_rf, &unpack_pnf_rf_value},
    { NFAPI_PNF_PHY_REL10_TAG, &pNfapiMsg->pnf_phy_rel10, &unpack_pnf_phy_rel10_value},
    { NFAPI_PNF_PHY_REL11_TAG, &pNfapiMsg->pnf_phy_rel11, &unpack_pnf_phy_rel11_value},
    { NFAPI_PNF_PHY_REL12_TAG, &pNfapiMsg->pnf_phy_rel12, &unpack_pnf_phy_rel12_value},
    { NFAPI_PNF_PHY_REL13_TAG, &pNfapiMsg->pnf_phy_rel13, &unpack_pnf_phy_rel13_value},
    { NFAPI_PNF_PHY_REL13_NB_IOT_TAG, &pNfapiMsg->pnf_phy_rel13_nb_iot, &unpack_pnf_phy_rel13_nb_iot_value},

  };
  return ( pull32(ppReadPackedMsg, &pNfapiMsg->error_code, end) &&
           unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));
}

static uint8_t unpack_nr_pnf_config_request(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config)
{
  nfapi_nr_pnf_config_request_t *pNfapiMsg = (nfapi_nr_pnf_config_request_t *)msg;
  pull8(ppReadPackedMsg, &pNfapiMsg->num_tlvs, end);
  unpack_tlv_t unpack_fns[] = {
      {NFAPI_PNF_PHY_RF_TAG, &pNfapiMsg->pnf_phy_rf_config, &unpack_pnf_phy_rf_config_value},
  };
  return unpack_nr_tlv_list(unpack_fns,
                            sizeof(unpack_fns) / sizeof(unpack_tlv_t),
                            ppReadPackedMsg,
                            end,
                            config,
                            &pNfapiMsg->vendor_extension);
}

static uint8_t unpack_pnf_config_request(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config) {
  nfapi_pnf_config_request_t *pNfapiMsg = (nfapi_pnf_config_request_t *)msg;
  unpack_tlv_t unpack_fns[] = {
    { NFAPI_PNF_PHY_RF_TAG, &pNfapiMsg->pnf_phy_rf_config, &unpack_pnf_phy_rf_config_value},
  };
  return (pull8(ppReadPackedMsg, &pNfapiMsg->num_tlvs, end) &&
          unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));
}

static uint8_t unpack_nr_pnf_config_response(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config)
{
  nfapi_nr_pnf_config_response_t *pNfapiMsg = (nfapi_nr_pnf_config_response_t *)msg;
  return (pull8(ppReadPackedMsg, &pNfapiMsg->error_code, end)
          && unpack_nr_tlv_list(NULL, 0, ppReadPackedMsg, end, config, &(pNfapiMsg->vendor_extension)));
}

static uint8_t unpack_pnf_config_response(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config) {
  nfapi_pnf_config_response_t *pNfapiMsg = (nfapi_pnf_config_response_t *)msg;
  return ( pull32(ppReadPackedMsg, &pNfapiMsg->error_code, end) &&
           unpack_tlv_list(NULL, 0, ppReadPackedMsg, end, config, &(pNfapiMsg->vendor_extension)));
}

static uint8_t unpack_nr_pnf_start_request(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config)
{
  nfapi_nr_pnf_start_request_t *pNfapiMsg = (nfapi_nr_pnf_start_request_t *)msg;
  return unpack_nr_tlv_list(NULL, 0, ppReadPackedMsg, end, config, &(pNfapiMsg->vendor_extension));
}

static uint8_t unpack_pnf_start_request(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config) {
  nfapi_pnf_start_request_t *pNfapiMsg = (nfapi_pnf_start_request_t *)msg;
  return unpack_tlv_list(NULL, 0, ppReadPackedMsg, end, config, &(pNfapiMsg->vendor_extension));
}

static uint8_t unpack_pnf_start_response(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config) {
  nfapi_pnf_start_response_t *pNfapiMsg = (nfapi_pnf_start_response_t *)msg;
  return ( pull32(ppReadPackedMsg, &pNfapiMsg->error_code, end ) &&
           unpack_tlv_list(NULL, 0, ppReadPackedMsg, end, config, &(pNfapiMsg->vendor_extension)));
}

static uint8_t unpack_nr_pnf_start_response(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config)
{
  nfapi_nr_pnf_start_response_t *pNfapiMsg = (nfapi_nr_pnf_start_response_t *)msg;
  return (pull32(ppReadPackedMsg, &pNfapiMsg->error_code, end)
          && unpack_nr_tlv_list(NULL, 0, ppReadPackedMsg, end, config, &(pNfapiMsg->vendor_extension)));
}

static uint8_t unpack_pnf_stop_request(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config) {
  nfapi_pnf_stop_request_t *pNfapiMsg = (nfapi_pnf_stop_request_t *)msg;
  return unpack_tlv_list(NULL, 0, ppReadPackedMsg, end, config, &(pNfapiMsg->vendor_extension));
}

static uint8_t unpack_pnf_stop_response(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config) {
  nfapi_pnf_stop_response_t *pNfapiMsg = (nfapi_pnf_stop_response_t *)msg;
  return ( pull32(ppReadPackedMsg, &pNfapiMsg->error_code, end) &&
           unpack_tlv_list(NULL, 0, ppReadPackedMsg, end, config, &(pNfapiMsg->vendor_extension)));
}

static uint8_t unpack_param_request(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config) {
  nfapi_param_request_t *pNfapiMsg = (nfapi_param_request_t *)msg;
  return unpack_tlv_list(NULL, 0, ppReadPackedMsg, end, config, &(pNfapiMsg->vendor_extension));
}

static uint8_t unpack_param_response(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config)
{
  nfapi_param_response_t *pNfapiMsg = (nfapi_param_response_t *)msg;
  unpack_tlv_t unpack_fns[] = {
      {NFAPI_L1_STATUS_PHY_STATE_TAG, &pNfapiMsg->l1_status.phy_state, &unpack_uint16_tlv_value},

      {NFAPI_PHY_CAPABILITIES_DL_BANDWIDTH_SUPPORT_TAG,
       &pNfapiMsg->phy_capabilities.dl_bandwidth_support,
       &unpack_uint16_tlv_value},
      {NFAPI_PHY_CAPABILITIES_UL_BANDWIDTH_SUPPORT_TAG,
       &pNfapiMsg->phy_capabilities.ul_bandwidth_support,
       &unpack_uint16_tlv_value},
      {NFAPI_PHY_CAPABILITIES_DL_MODULATION_SUPPORT_TAG,
       &pNfapiMsg->phy_capabilities.dl_modulation_support,
       &unpack_uint16_tlv_value},
      {NFAPI_PHY_CAPABILITIES_UL_MODULATION_SUPPORT_TAG,
       &pNfapiMsg->phy_capabilities.ul_modulation_support,
       &unpack_uint16_tlv_value},
      {NFAPI_PHY_CAPABILITIES_PHY_ANTENNA_CAPABILITY_TAG,
       &pNfapiMsg->phy_capabilities.phy_antenna_capability,
       &unpack_uint16_tlv_value},
      {NFAPI_PHY_CAPABILITIES_RELEASE_CAPABILITY_TAG, &pNfapiMsg->phy_capabilities.release_capability, &unpack_uint16_tlv_value},
      {NFAPI_PHY_CAPABILITIES_MBSFN_CAPABILITY_TAG, &pNfapiMsg->phy_capabilities.mbsfn_capability, &unpack_uint16_tlv_value},

      {NFAPI_LAA_CAPABILITY_LAA_SUPPORT_TAG, &pNfapiMsg->laa_capability.laa_support, &unpack_uint16_tlv_value},
      {NFAPI_LAA_CAPABILITY_PD_SENSING_LBT_SUPPORT_TAG,
       &pNfapiMsg->laa_capability.pd_sensing_lbt_support,
       &unpack_uint16_tlv_value},
      {NFAPI_LAA_CAPABILITY_MULTI_CARRIER_LBT_SUPPORT_TAG,
       &pNfapiMsg->laa_capability.multi_carrier_lbt_support,
       &unpack_uint16_tlv_value},
      {NFAPI_LAA_CAPABILITY_PARTIAL_SF_SUPPORT_TAG, &pNfapiMsg->laa_capability.partial_sf_support, &unpack_uint16_tlv_value},

      {NFAPI_SUBFRAME_CONFIG_DUPLEX_MODE_TAG, &pNfapiMsg->subframe_config.duplex_mode, &unpack_uint16_tlv_value},
      {NFAPI_SUBFRAME_CONFIG_PCFICH_POWER_OFFSET_TAG, &pNfapiMsg->subframe_config.pcfich_power_offset, &unpack_uint16_tlv_value},
      {NFAPI_SUBFRAME_CONFIG_PB_TAG, &pNfapiMsg->subframe_config.pb, &unpack_uint16_tlv_value},
      {NFAPI_SUBFRAME_CONFIG_DL_CYCLIC_PREFIX_TYPE_TAG,
       &pNfapiMsg->subframe_config.dl_cyclic_prefix_type,
       &unpack_uint16_tlv_value},
      {NFAPI_SUBFRAME_CONFIG_UL_CYCLIC_PREFIX_TYPE_TAG,
       &pNfapiMsg->subframe_config.ul_cyclic_prefix_type,
       &unpack_uint16_tlv_value},

      {NFAPI_RF_CONFIG_DL_CHANNEL_BANDWIDTH_TAG, &pNfapiMsg->rf_config.dl_channel_bandwidth, &unpack_uint16_tlv_value},
      {NFAPI_RF_CONFIG_UL_CHANNEL_BANDWIDTH_TAG, &pNfapiMsg->rf_config.ul_channel_bandwidth, &unpack_uint16_tlv_value},
      {NFAPI_RF_CONFIG_REFERENCE_SIGNAL_POWER_TAG, &pNfapiMsg->rf_config.reference_signal_power, &unpack_uint16_tlv_value},
      {NFAPI_RF_CONFIG_TX_ANTENNA_PORTS_TAG, &pNfapiMsg->rf_config.tx_antenna_ports, &unpack_uint16_tlv_value},
      {NFAPI_RF_CONFIG_RX_ANTENNA_PORTS_TAG, &pNfapiMsg->rf_config.rx_antenna_ports, &unpack_uint16_tlv_value},

      {NFAPI_PHICH_CONFIG_PHICH_RESOURCE_TAG, &pNfapiMsg->phich_config.phich_resource, &unpack_uint16_tlv_value},
      {NFAPI_PHICH_CONFIG_PHICH_DURATION_TAG, &pNfapiMsg->phich_config.phich_duration, &unpack_uint16_tlv_value},
      {NFAPI_PHICH_CONFIG_PHICH_POWER_OFFSET_TAG, &pNfapiMsg->phich_config.phich_power_offset, &unpack_uint16_tlv_value},

      {NFAPI_SCH_CONFIG_PRIMARY_SYNCHRONIZATION_SIGNAL_EPRE_EPRERS_TAG,
       &pNfapiMsg->sch_config.primary_synchronization_signal_epre_eprers,
       &unpack_uint16_tlv_value},
      {NFAPI_SCH_CONFIG_SECONDARY_SYNCHRONIZATION_SIGNAL_EPRE_EPRERS_TAG,
       &pNfapiMsg->sch_config.secondary_synchronization_signal_epre_eprers,
       &unpack_uint16_tlv_value},
      {NFAPI_SCH_CONFIG_PHYSICAL_CELL_ID_TAG, &pNfapiMsg->sch_config.physical_cell_id, &unpack_uint16_tlv_value},

      {NFAPI_PRACH_CONFIG_CONFIGURATION_INDEX_TAG, &pNfapiMsg->prach_config.configuration_index, &unpack_uint16_tlv_value},
      {NFAPI_PRACH_CONFIG_ROOT_SEQUENCE_INDEX_TAG, &pNfapiMsg->prach_config.root_sequence_index, &unpack_uint16_tlv_value},
      {NFAPI_PRACH_CONFIG_ZERO_CORRELATION_ZONE_CONFIGURATION_TAG,
       &pNfapiMsg->prach_config.zero_correlation_zone_configuration,
       &unpack_uint16_tlv_value},
      {NFAPI_PRACH_CONFIG_HIGH_SPEED_FLAG_TAG, &pNfapiMsg->prach_config.high_speed_flag, &unpack_uint16_tlv_value},
      {NFAPI_PRACH_CONFIG_FREQUENCY_OFFSET_TAG, &pNfapiMsg->prach_config.frequency_offset, &unpack_uint16_tlv_value},

      {NFAPI_PUSCH_CONFIG_HOPPING_MODE_TAG, &pNfapiMsg->pusch_config.hopping_mode, &unpack_uint16_tlv_value},
      {NFAPI_PUSCH_CONFIG_HOPPING_OFFSET_TAG, &pNfapiMsg->pusch_config.hopping_offset, &unpack_uint16_tlv_value},
      {NFAPI_PUSCH_CONFIG_NUMBER_OF_SUBBANDS_TAG, &pNfapiMsg->pusch_config.number_of_subbands, &unpack_uint16_tlv_value},

      {NFAPI_PUCCH_CONFIG_DELTA_PUCCH_SHIFT_TAG, &pNfapiMsg->pucch_config.delta_pucch_shift, &unpack_uint16_tlv_value},
      {NFAPI_PUCCH_CONFIG_N_CQI_RB_TAG, &pNfapiMsg->pucch_config.n_cqi_rb, &unpack_uint16_tlv_value},
      {NFAPI_PUCCH_CONFIG_N_AN_CS_TAG, &pNfapiMsg->pucch_config.n_an_cs, &unpack_uint16_tlv_value},
      {NFAPI_PUCCH_CONFIG_N1_PUCCH_AN_TAG, &pNfapiMsg->pucch_config.n1_pucch_an, &unpack_uint16_tlv_value},

      {NFAPI_SRS_CONFIG_BANDWIDTH_CONFIGURATION_TAG, &pNfapiMsg->srs_config.bandwidth_configuration, &unpack_uint16_tlv_value},
      {NFAPI_SRS_CONFIG_MAX_UP_PTS_TAG, &pNfapiMsg->srs_config.max_up_pts, &unpack_uint16_tlv_value},
      {NFAPI_SRS_CONFIG_SRS_SUBFRAME_CONFIGURATION_TAG,
       &pNfapiMsg->srs_config.srs_subframe_configuration,
       &unpack_uint16_tlv_value},
      {NFAPI_SRS_CONFIG_SRS_ACKNACK_SRS_SIMULTANEOUS_TRANSMISSION_TAG,
       &pNfapiMsg->srs_config.srs_acknack_srs_simultaneous_transmission,
       &unpack_uint16_tlv_value},

      {NFAPI_UPLINK_REFERENCE_SIGNAL_CONFIG_UPLINK_RS_HOPPING_TAG,
       &pNfapiMsg->uplink_reference_signal_config.uplink_rs_hopping,
       &unpack_uint16_tlv_value},
      {NFAPI_UPLINK_REFERENCE_SIGNAL_CONFIG_GROUP_ASSIGNMENT_TAG,
       &pNfapiMsg->uplink_reference_signal_config.group_assignment,
       &unpack_uint16_tlv_value},
      {NFAPI_UPLINK_REFERENCE_SIGNAL_CONFIG_CYCLIC_SHIFT_1_FOR_DRMS_TAG,
       &pNfapiMsg->uplink_reference_signal_config.cyclic_shift_1_for_drms,
       &unpack_uint16_tlv_value},

      {NFAPI_TDD_FRAME_STRUCTURE_SUBFRAME_ASSIGNMENT_TAG,
       &pNfapiMsg->tdd_frame_structure_config.subframe_assignment,
       &unpack_uint16_tlv_value},
      {NFAPI_TDD_FRAME_STRUCTURE_SPECIAL_SUBFRAME_PATTERNS_TAG,
       &pNfapiMsg->tdd_frame_structure_config.special_subframe_patterns,
       &unpack_uint16_tlv_value},

      {NFAPI_L23_CONFIG_DATA_REPORT_MODE_TAG, &pNfapiMsg->l23_config.data_report_mode, &unpack_uint16_tlv_value},
      {NFAPI_L23_CONFIG_SFNSF_TAG, &pNfapiMsg->l23_config.sfnsf, &unpack_uint16_tlv_value},

      {NFAPI_NFAPI_P7_VNF_ADDRESS_IPV4_TAG, &pNfapiMsg->nfapi_config.p7_vnf_address_ipv4, &unpack_ipv4_address_value},
      {NFAPI_NFAPI_P7_VNF_ADDRESS_IPV6_TAG, &pNfapiMsg->nfapi_config.p7_vnf_address_ipv6, &unpack_ipv6_address_value},
      {NFAPI_NFAPI_P7_VNF_PORT_TAG, &pNfapiMsg->nfapi_config.p7_vnf_port, &unpack_uint16_tlv_value},
      {NFAPI_NFAPI_P7_PNF_ADDRESS_IPV4_TAG, &pNfapiMsg->nfapi_config.p7_pnf_address_ipv4, &unpack_ipv4_address_value},
      {NFAPI_NFAPI_P7_PNF_ADDRESS_IPV6_TAG, &pNfapiMsg->nfapi_config.p7_pnf_address_ipv6, &unpack_ipv6_address_value},
      {NFAPI_NFAPI_P7_PNF_PORT_TAG, &pNfapiMsg->nfapi_config.p7_pnf_port, &unpack_uint16_tlv_value},
      {NFAPI_NFAPI_DOWNLINK_UES_PER_SUBFRAME_TAG, &pNfapiMsg->nfapi_config.dl_ue_per_sf, &unpack_uint8_tlv_value},
      {NFAPI_NFAPI_UPLINK_UES_PER_SUBFRAME_TAG, &pNfapiMsg->nfapi_config.ul_ue_per_sf, &unpack_uint8_tlv_value},
      {NFAPI_NFAPI_RF_BANDS_TAG, &pNfapiMsg->nfapi_config.rf_bands, &unpack_rf_bands_value},
      {NFAPI_NFAPI_TIMING_WINDOW_TAG, &pNfapiMsg->nfapi_config.timing_window, &unpack_uint8_tlv_value},
      {NFAPI_NFAPI_TIMING_INFO_MODE_TAG, &pNfapiMsg->nfapi_config.timing_info_mode, &unpack_uint8_tlv_value},
      {NFAPI_NFAPI_TIMING_INFO_PERIOD_TAG, &pNfapiMsg->nfapi_config.timing_info_period, &unpack_uint8_tlv_value},
      {NFAPI_NFAPI_MAXIMUM_TRANSMIT_POWER_TAG, &pNfapiMsg->nfapi_config.max_transmit_power, &unpack_uint16_tlv_value},
      {NFAPI_NFAPI_EARFCN_TAG, &pNfapiMsg->nfapi_config.earfcn, &unpack_uint16_tlv_value},
      {NFAPI_NFAPI_NMM_GSM_FREQUENCY_BANDS_TAG,
       &pNfapiMsg->nfapi_config.nmm_gsm_frequency_bands,
       &unpack_nmm_frequency_bands_value},
      {NFAPI_NFAPI_NMM_UMTS_FREQUENCY_BANDS_TAG,
       &pNfapiMsg->nfapi_config.nmm_umts_frequency_bands,
       &unpack_nmm_frequency_bands_value},
      {NFAPI_NFAPI_NMM_LTE_FREQUENCY_BANDS_TAG,
       &pNfapiMsg->nfapi_config.nmm_lte_frequency_bands,
       &unpack_nmm_frequency_bands_value},
      {NFAPI_NFAPI_NMM_UPLINK_RSSI_SUPPORTED_TAG, &pNfapiMsg->nfapi_config.nmm_uplink_rssi_supported, &unpack_uint8_tlv_value},

  };
  return (pull8(ppReadPackedMsg, &pNfapiMsg->error_code, end) && pull8(ppReadPackedMsg, &pNfapiMsg->num_tlv, end)
          && unpack_tlv_list(unpack_fns,
                             sizeof(unpack_fns) / sizeof(unpack_tlv_t),
                             ppReadPackedMsg,
                             end,
                             config,
                             &pNfapiMsg->vendor_extension));
}

static uint8_t unpack_config_request(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config)
{
  nfapi_config_request_t *pNfapiMsg = (nfapi_config_request_t *)msg;
  unpack_tlv_t unpack_fns[] = {
      {NFAPI_SUBFRAME_CONFIG_DUPLEX_MODE_TAG, &pNfapiMsg->subframe_config.duplex_mode, &unpack_uint16_tlv_value},
      {NFAPI_SUBFRAME_CONFIG_PCFICH_POWER_OFFSET_TAG, &pNfapiMsg->subframe_config.pcfich_power_offset, &unpack_uint16_tlv_value},
      {NFAPI_SUBFRAME_CONFIG_PB_TAG, &pNfapiMsg->subframe_config.pb, &unpack_uint16_tlv_value},
      {NFAPI_SUBFRAME_CONFIG_DL_CYCLIC_PREFIX_TYPE_TAG,
       &pNfapiMsg->subframe_config.dl_cyclic_prefix_type,
       &unpack_uint16_tlv_value},
      {NFAPI_SUBFRAME_CONFIG_UL_CYCLIC_PREFIX_TYPE_TAG,
       &pNfapiMsg->subframe_config.ul_cyclic_prefix_type,
       &unpack_uint16_tlv_value},

      {NFAPI_RF_CONFIG_DL_CHANNEL_BANDWIDTH_TAG, &pNfapiMsg->rf_config.dl_channel_bandwidth, &unpack_uint16_tlv_value},
      {NFAPI_RF_CONFIG_UL_CHANNEL_BANDWIDTH_TAG, &pNfapiMsg->rf_config.ul_channel_bandwidth, &unpack_uint16_tlv_value},
      {NFAPI_RF_CONFIG_REFERENCE_SIGNAL_POWER_TAG, &pNfapiMsg->rf_config.reference_signal_power, &unpack_uint16_tlv_value},
      {NFAPI_RF_CONFIG_TX_ANTENNA_PORTS_TAG, &pNfapiMsg->rf_config.tx_antenna_ports, &unpack_uint16_tlv_value},
      {NFAPI_RF_CONFIG_RX_ANTENNA_PORTS_TAG, &pNfapiMsg->rf_config.rx_antenna_ports, &unpack_uint16_tlv_value},

      {NFAPI_PHICH_CONFIG_PHICH_RESOURCE_TAG, &pNfapiMsg->phich_config.phich_resource, &unpack_uint16_tlv_value},
      {NFAPI_PHICH_CONFIG_PHICH_DURATION_TAG, &pNfapiMsg->phich_config.phich_duration, &unpack_uint16_tlv_value},
      {NFAPI_PHICH_CONFIG_PHICH_POWER_OFFSET_TAG, &pNfapiMsg->phich_config.phich_power_offset, &unpack_uint16_tlv_value},

      {NFAPI_SCH_CONFIG_PRIMARY_SYNCHRONIZATION_SIGNAL_EPRE_EPRERS_TAG,
       &pNfapiMsg->sch_config.primary_synchronization_signal_epre_eprers,
       &unpack_uint16_tlv_value},
      {NFAPI_SCH_CONFIG_SECONDARY_SYNCHRONIZATION_SIGNAL_EPRE_EPRERS_TAG,
       &pNfapiMsg->sch_config.secondary_synchronization_signal_epre_eprers,
       &unpack_uint16_tlv_value},
      {NFAPI_SCH_CONFIG_PHYSICAL_CELL_ID_TAG, &pNfapiMsg->sch_config.physical_cell_id, &unpack_uint16_tlv_value},

      {NFAPI_PRACH_CONFIG_CONFIGURATION_INDEX_TAG, &pNfapiMsg->prach_config.configuration_index, &unpack_uint16_tlv_value},
      {NFAPI_PRACH_CONFIG_ROOT_SEQUENCE_INDEX_TAG, &pNfapiMsg->prach_config.root_sequence_index, &unpack_uint16_tlv_value},
      {NFAPI_PRACH_CONFIG_ZERO_CORRELATION_ZONE_CONFIGURATION_TAG,
       &pNfapiMsg->prach_config.zero_correlation_zone_configuration,
       &unpack_uint16_tlv_value},
      {NFAPI_PRACH_CONFIG_HIGH_SPEED_FLAG_TAG, &pNfapiMsg->prach_config.high_speed_flag, &unpack_uint16_tlv_value},
      {NFAPI_PRACH_CONFIG_FREQUENCY_OFFSET_TAG, &pNfapiMsg->prach_config.frequency_offset, &unpack_uint16_tlv_value},

      {NFAPI_PUSCH_CONFIG_HOPPING_MODE_TAG, &pNfapiMsg->pusch_config.hopping_mode, &unpack_uint16_tlv_value},
      {NFAPI_PUSCH_CONFIG_HOPPING_OFFSET_TAG, &pNfapiMsg->pusch_config.hopping_offset, &unpack_uint16_tlv_value},
      {NFAPI_PUSCH_CONFIG_NUMBER_OF_SUBBANDS_TAG, &pNfapiMsg->pusch_config.number_of_subbands, &unpack_uint16_tlv_value},

      {NFAPI_PUCCH_CONFIG_DELTA_PUCCH_SHIFT_TAG, &pNfapiMsg->pucch_config.delta_pucch_shift, &unpack_uint16_tlv_value},
      {NFAPI_PUCCH_CONFIG_N_CQI_RB_TAG, &pNfapiMsg->pucch_config.n_cqi_rb, &unpack_uint16_tlv_value},
      {NFAPI_PUCCH_CONFIG_N_AN_CS_TAG, &pNfapiMsg->pucch_config.n_an_cs, &unpack_uint16_tlv_value},
      {NFAPI_PUCCH_CONFIG_N1_PUCCH_AN_TAG, &pNfapiMsg->pucch_config.n1_pucch_an, &unpack_uint16_tlv_value},

      {NFAPI_SRS_CONFIG_BANDWIDTH_CONFIGURATION_TAG, &pNfapiMsg->srs_config.bandwidth_configuration, &unpack_uint16_tlv_value},
      {NFAPI_SRS_CONFIG_MAX_UP_PTS_TAG, &pNfapiMsg->srs_config.max_up_pts, &unpack_uint16_tlv_value},
      {NFAPI_SRS_CONFIG_SRS_SUBFRAME_CONFIGURATION_TAG,
       &pNfapiMsg->srs_config.srs_subframe_configuration,
       &unpack_uint16_tlv_value},
      {NFAPI_SRS_CONFIG_SRS_ACKNACK_SRS_SIMULTANEOUS_TRANSMISSION_TAG,
       &pNfapiMsg->srs_config.srs_acknack_srs_simultaneous_transmission,
       &unpack_uint16_tlv_value},

      {NFAPI_UPLINK_REFERENCE_SIGNAL_CONFIG_UPLINK_RS_HOPPING_TAG,
       &pNfapiMsg->uplink_reference_signal_config.uplink_rs_hopping,
       &unpack_uint16_tlv_value},
      {NFAPI_UPLINK_REFERENCE_SIGNAL_CONFIG_GROUP_ASSIGNMENT_TAG,
       &pNfapiMsg->uplink_reference_signal_config.group_assignment,
       &unpack_uint16_tlv_value},
      {NFAPI_UPLINK_REFERENCE_SIGNAL_CONFIG_CYCLIC_SHIFT_1_FOR_DRMS_TAG,
       &pNfapiMsg->uplink_reference_signal_config.cyclic_shift_1_for_drms,
       &unpack_uint16_tlv_value},

      {NFAPI_LAA_CONFIG_ED_THRESHOLD_FOR_LBT_FOR_PDSCH_TAG,
       &pNfapiMsg->laa_config.ed_threshold_lbt_pdsch,
       &unpack_uint16_tlv_value},
      {NFAPI_LAA_CONFIG_ED_THRESHOLD_FOR_LBT_FOR_DRS_TAG, &pNfapiMsg->laa_config.ed_threshold_lbt_drs, &unpack_uint16_tlv_value},
      {NFAPI_LAA_CONFIG_PD_THRESHOLD_TAG, &pNfapiMsg->laa_config.pd_threshold, &unpack_uint16_tlv_value},
      {NFAPI_LAA_CONFIG_MULTI_CARRIER_TYPE_TAG, &pNfapiMsg->laa_config.multi_carrier_type, &unpack_uint16_tlv_value},
      {NFAPI_LAA_CONFIG_MULTI_CARRIER_TX_TAG, &pNfapiMsg->laa_config.multi_carrier_tx, &unpack_uint16_tlv_value},
      {NFAPI_LAA_CONFIG_MULTI_CARRIER_FREEZE_TAG, &pNfapiMsg->laa_config.multi_carrier_freeze, &unpack_uint16_tlv_value},
      {NFAPI_LAA_CONFIG_TX_ANTENNA_PORTS_FOR_DRS_TAG, &pNfapiMsg->laa_config.tx_antenna_ports_drs, &unpack_uint16_tlv_value},
      {NFAPI_LAA_CONFIG_TRANSMISSION_POWER_FOR_DRS_TAG, &pNfapiMsg->laa_config.tx_power_drs, &unpack_uint16_tlv_value},

      {NFAPI_EMTC_CONFIG_PBCH_REPETITIONS_ENABLE_R13_TAG,
       &pNfapiMsg->emtc_config.pbch_repetitions_enable_r13,
       &unpack_uint16_tlv_value},
      {NFAPI_EMTC_CONFIG_PRACH_CATM_ROOT_SEQUENCE_INDEX_TAG,
       &pNfapiMsg->emtc_config.prach_catm_root_sequence_index,
       &unpack_uint16_tlv_value},
      {NFAPI_EMTC_CONFIG_PRACH_CATM_ZERO_CORRELATION_ZONE_CONFIGURATION_TAG,
       &pNfapiMsg->emtc_config.prach_catm_zero_correlation_zone_configuration,
       &unpack_uint16_tlv_value},
      {NFAPI_EMTC_CONFIG_PRACH_CATM_HIGH_SPEED_FLAG, &pNfapiMsg->emtc_config.prach_catm_high_speed_flag, &unpack_uint16_tlv_value},
      {NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_ENABLE_TAG, &pNfapiMsg->emtc_config.prach_ce_level_0_enable, &unpack_uint16_tlv_value},
      {NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_CONFIGURATION_INDEX_TAG,
       &pNfapiMsg->emtc_config.prach_ce_level_0_configuration_index,
       &unpack_uint16_tlv_value},
      {NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_FREQUENCY_OFFSET_TAG,
       &pNfapiMsg->emtc_config.prach_ce_level_0_frequency_offset,
       &unpack_uint16_tlv_value},
      {NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG,
       &pNfapiMsg->emtc_config.prach_ce_level_0_number_of_repetitions_per_attempt,
       &unpack_uint16_tlv_value},
      {NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_STARTING_SUBFRAME_PERIODICITY_TAG,
       &pNfapiMsg->emtc_config.prach_ce_level_0_starting_subframe_periodicity,
       &unpack_uint16_tlv_value},
      {NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_HOPPING_ENABLE_TAG,
       &pNfapiMsg->emtc_config.prach_ce_level_0_hopping_enable,
       &unpack_uint16_tlv_value},
      {NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_0_HOPPING_OFFSET_TAG,
       &pNfapiMsg->emtc_config.prach_ce_level_0_hopping_offset,
       &unpack_uint16_tlv_value},
      {NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_ENABLE_TAG, &pNfapiMsg->emtc_config.prach_ce_level_1_enable, &unpack_uint16_tlv_value},
      {NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_CONFIGURATION_INDEX_TAG,
       &pNfapiMsg->emtc_config.prach_ce_level_1_configuration_index,
       &unpack_uint16_tlv_value},
      {NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_FREQUENCY_OFFSET_TAG,
       &pNfapiMsg->emtc_config.prach_ce_level_1_frequency_offset,
       &unpack_uint16_tlv_value},
      {NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG,
       &pNfapiMsg->emtc_config.prach_ce_level_1_number_of_repetitions_per_attempt,
       &unpack_uint16_tlv_value},
      {NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_STARTING_SUBFRAME_PERIODICITY_TAG,
       &pNfapiMsg->emtc_config.prach_ce_level_1_starting_subframe_periodicity,
       &unpack_uint16_tlv_value},
      {NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_HOPPING_ENABLE_TAG,
       &pNfapiMsg->emtc_config.prach_ce_level_1_hopping_enable,
       &unpack_uint16_tlv_value},
      {NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_1_HOPPING_OFFSET_TAG,
       &pNfapiMsg->emtc_config.prach_ce_level_1_hopping_offset,
       &unpack_uint16_tlv_value},
      {NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_ENABLE_TAG, &pNfapiMsg->emtc_config.prach_ce_level_2_enable, &unpack_uint16_tlv_value},
      {NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_CONFIGURATION_INDEX_TAG,
       &pNfapiMsg->emtc_config.prach_ce_level_2_configuration_index,
       &unpack_uint16_tlv_value},
      {NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_FREQUENCY_OFFSET_TAG,
       &pNfapiMsg->emtc_config.prach_ce_level_2_frequency_offset,
       &unpack_uint16_tlv_value},
      {NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG,
       &pNfapiMsg->emtc_config.prach_ce_level_2_number_of_repetitions_per_attempt,
       &unpack_uint16_tlv_value},
      {NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_STARTING_SUBFRAME_PERIODICITY_TAG,
       &pNfapiMsg->emtc_config.prach_ce_level_2_starting_subframe_periodicity,
       &unpack_uint16_tlv_value},
      {NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_HOPPING_ENABLE_TAG,
       &pNfapiMsg->emtc_config.prach_ce_level_2_hopping_enable,
       &unpack_uint16_tlv_value},
      {NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_2_HOPPING_OFFSET_TAG,
       &pNfapiMsg->emtc_config.prach_ce_level_2_hopping_offset,
       &unpack_uint16_tlv_value},
      {NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_ENABLE_TAG, &pNfapiMsg->emtc_config.prach_ce_level_3_enable, &unpack_uint16_tlv_value},
      {NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_CONFIGURATION_INDEX_TAG,
       &pNfapiMsg->emtc_config.prach_ce_level_3_configuration_index,
       &unpack_uint16_tlv_value},
      {NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_FREQUENCY_OFFSET_TAG,
       &pNfapiMsg->emtc_config.prach_ce_level_3_frequency_offset,
       &unpack_uint16_tlv_value},
      {NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_NUMBER_OF_REPETITIONS_PER_ATTEMPT_TAG,
       &pNfapiMsg->emtc_config.prach_ce_level_3_number_of_repetitions_per_attempt,
       &unpack_uint16_tlv_value},
      {NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_STARTING_SUBFRAME_PERIODICITY_TAG,
       &pNfapiMsg->emtc_config.prach_ce_level_3_starting_subframe_periodicity,
       &unpack_uint16_tlv_value},
      {NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_HOPPING_ENABLE_TAG,
       &pNfapiMsg->emtc_config.prach_ce_level_3_hopping_enable,
       &unpack_uint16_tlv_value},
      {NFAPI_EMTC_CONFIG_PRACH_CE_LEVEL_3_HOPPING_OFFSET_TAG,
       &pNfapiMsg->emtc_config.prach_ce_level_3_hopping_offset,
       &unpack_uint16_tlv_value},
      {NFAPI_EMTC_CONFIG_PUCCH_INTERVAL_ULHOPPINGCONFIGCOMMONMODEA_TAG,
       &pNfapiMsg->emtc_config.pucch_interval_ulhoppingconfigcommonmodea,
       &unpack_uint16_tlv_value},
      {NFAPI_EMTC_CONFIG_PUCCH_INTERVAL_ULHOPPINGCONFIGCOMMONMODEB_TAG,
       &pNfapiMsg->emtc_config.pucch_interval_ulhoppingconfigcommonmodeb,
       &unpack_uint16_tlv_value},

      {NFAPI_TDD_FRAME_STRUCTURE_SUBFRAME_ASSIGNMENT_TAG,
       &pNfapiMsg->tdd_frame_structure_config.subframe_assignment,
       &unpack_uint16_tlv_value},
      {NFAPI_TDD_FRAME_STRUCTURE_SPECIAL_SUBFRAME_PATTERNS_TAG,
       &pNfapiMsg->tdd_frame_structure_config.special_subframe_patterns,
       &unpack_uint16_tlv_value},

      {NFAPI_L23_CONFIG_DATA_REPORT_MODE_TAG, &pNfapiMsg->l23_config.data_report_mode, &unpack_uint16_tlv_value},
      {NFAPI_L23_CONFIG_SFNSF_TAG, &pNfapiMsg->l23_config.sfnsf, &unpack_uint16_tlv_value},

      {NFAPI_NFAPI_P7_VNF_ADDRESS_IPV4_TAG, &pNfapiMsg->nfapi_config.p7_vnf_address_ipv4, &unpack_ipv4_address_value},
      {NFAPI_NFAPI_P7_VNF_ADDRESS_IPV6_TAG, &pNfapiMsg->nfapi_config.p7_vnf_address_ipv6, &unpack_ipv6_address_value},
      {NFAPI_NFAPI_P7_VNF_PORT_TAG, &pNfapiMsg->nfapi_config.p7_vnf_port, &unpack_uint16_tlv_value},
      {NFAPI_NFAPI_P7_PNF_ADDRESS_IPV4_TAG, &pNfapiMsg->nfapi_config.p7_pnf_address_ipv4, &unpack_ipv4_address_value},
      {NFAPI_NFAPI_P7_PNF_ADDRESS_IPV6_TAG, &pNfapiMsg->nfapi_config.p7_pnf_address_ipv6, &unpack_ipv6_address_value},
      {NFAPI_NFAPI_P7_PNF_PORT_TAG, &pNfapiMsg->nfapi_config.p7_pnf_port, &unpack_uint16_tlv_value},
      {NFAPI_NFAPI_DOWNLINK_UES_PER_SUBFRAME_TAG, &pNfapiMsg->nfapi_config.dl_ue_per_sf, &unpack_uint8_tlv_value},
      {NFAPI_NFAPI_UPLINK_UES_PER_SUBFRAME_TAG, &pNfapiMsg->nfapi_config.ul_ue_per_sf, &unpack_uint8_tlv_value},
      {NFAPI_NFAPI_RF_BANDS_TAG, &pNfapiMsg->nfapi_config.rf_bands, &unpack_rf_bands_value},
      {NFAPI_NFAPI_TIMING_WINDOW_TAG, &pNfapiMsg->nfapi_config.timing_window, &unpack_uint8_tlv_value},
      {NFAPI_NFAPI_TIMING_INFO_MODE_TAG, &pNfapiMsg->nfapi_config.timing_info_mode, &unpack_uint8_tlv_value},
      {NFAPI_NFAPI_TIMING_INFO_PERIOD_TAG, &pNfapiMsg->nfapi_config.timing_info_period, &unpack_uint8_tlv_value},
      {NFAPI_NFAPI_MAXIMUM_TRANSMIT_POWER_TAG, &pNfapiMsg->nfapi_config.max_transmit_power, &unpack_uint16_tlv_value},
      {NFAPI_NFAPI_EARFCN_TAG, &pNfapiMsg->nfapi_config.earfcn, &unpack_uint16_tlv_value},
      {NFAPI_NFAPI_NMM_GSM_FREQUENCY_BANDS_TAG,
       &pNfapiMsg->nfapi_config.nmm_gsm_frequency_bands,
       &unpack_nmm_frequency_bands_value},
      {NFAPI_NFAPI_NMM_UMTS_FREQUENCY_BANDS_TAG,
       &pNfapiMsg->nfapi_config.nmm_umts_frequency_bands,
       &unpack_nmm_frequency_bands_value},
      {NFAPI_NFAPI_NMM_LTE_FREQUENCY_BANDS_TAG,
       &pNfapiMsg->nfapi_config.nmm_lte_frequency_bands,
       &unpack_nmm_frequency_bands_value},
      {NFAPI_NFAPI_NMM_UPLINK_RSSI_SUPPORTED_TAG, &pNfapiMsg->nfapi_config.nmm_uplink_rssi_supported, &unpack_uint8_tlv_value},

  };
  return (pull8(ppReadPackedMsg, &pNfapiMsg->num_tlv, end)
          && unpack_tlv_list(unpack_fns,
                             sizeof(unpack_fns) / sizeof(unpack_tlv_t),
                             ppReadPackedMsg,
                             end,
                             config,
                             &pNfapiMsg->vendor_extension));
}

static uint8_t unpack_config_response(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config)
{
  nfapi_config_response_t *pNfapiMsg = (nfapi_config_response_t *)msg;
  return (pull32(ppReadPackedMsg, &pNfapiMsg->error_code, end)
          && unpack_tlv_list(NULL, 0, ppReadPackedMsg, end, config, &(pNfapiMsg->vendor_extension)));
}

static uint8_t unpack_start_request(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config) {
  nfapi_start_request_t *pNfapiMsg = ( nfapi_start_request_t *)msg;
  return unpack_tlv_list(NULL, 0, ppReadPackedMsg, end, config, &(pNfapiMsg->vendor_extension));
}

static uint8_t unpack_start_response(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config) {
  nfapi_start_response_t *pNfapiMsg = (nfapi_start_response_t *)msg;
  return ( pull32(ppReadPackedMsg, &pNfapiMsg->error_code, end) &&
           unpack_tlv_list(NULL, 0, ppReadPackedMsg, end, config, &(pNfapiMsg->vendor_extension)));
}

static uint8_t unpack_stop_request(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config) {
  nfapi_stop_request_t *pNfapiMsg = (nfapi_stop_request_t *)msg;
  return unpack_tlv_list(NULL, 0, ppReadPackedMsg, end, config, &(pNfapiMsg->vendor_extension));
}

static uint8_t unpack_received_interference_power_measurement_value(void *tlv, uint8_t **ppReadPackedMsg, uint8_t *end) {
  nfapi_received_interference_power_measurement_t *value = (nfapi_received_interference_power_measurement_t *)tlv;
  return ( pull16(ppReadPackedMsg, &value->number_of_resource_blocks, end) &&
           pullarrays16(ppReadPackedMsg, value->received_interference_power,  NFAPI_MAX_RECEIVED_INTERFERENCE_POWER_RESULTS, value->number_of_resource_blocks, end));
}

static uint8_t unpack_measurement_response(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p4_p5_codec_config_t *config) {
  nfapi_measurement_response_t *pNfapiMsg = (nfapi_measurement_response_t *)msg;
  unpack_tlv_t unpack_fns[] = {
    { NFAPI_MEASUREMENT_RESPONSE_DL_RS_POWER_MEASUREMENT_TAG, &pNfapiMsg->dl_rs_tx_power_measurement, &unpack_int16_tlv_value},
    { NFAPI_MEASUREMENT_RESPONSE_RECEIVED_INTERFERENCE_POWER_MEASUREMENT_TAG, &pNfapiMsg->received_interference_power_measurement, &unpack_received_interference_power_measurement_value},
    { NFAPI_MEASUREMENT_RESPONSE_THERMAL_NOISE_MEASUREMENT_TAG, &pNfapiMsg->thermal_noise_power_measurement, &unpack_int16_tlv_value},
  };
  return ( pull32(ppReadPackedMsg, &pNfapiMsg->error_code, end) &&
           unpack_tlv_list(unpack_fns, sizeof(unpack_fns)/sizeof(unpack_tlv_t), ppReadPackedMsg, end, config, &pNfapiMsg->vendor_extension));
}

// unpack length check

static int check_nr_unpack_length(nfapi_nr_phy_msg_type_e msgId, uint32_t unpackedBufLen) {
  int retLen = 0;

  switch (msgId) {
    case NFAPI_NR_PHY_MSG_TYPE_PNF_PARAM_REQUEST:
      if (unpackedBufLen >= sizeof(nfapi_pnf_param_request_t))
        retLen = sizeof(nfapi_pnf_param_request_t);

      break;

    case NFAPI_NR_PHY_MSG_TYPE_PNF_PARAM_RESPONSE:
      if (unpackedBufLen >= sizeof(nfapi_nr_pnf_param_response_t))
        retLen = sizeof(nfapi_nr_pnf_param_response_t);

      break;

    case NFAPI_NR_PHY_MSG_TYPE_PNF_CONFIG_REQUEST:
      if (unpackedBufLen >= sizeof(nfapi_nr_pnf_config_request_t))
        retLen = sizeof(nfapi_nr_pnf_config_request_t);

      break;

    case NFAPI_NR_PHY_MSG_TYPE_PNF_CONFIG_RESPONSE:
      if (unpackedBufLen >= sizeof(nfapi_nr_pnf_config_response_t))
        retLen = sizeof(nfapi_nr_pnf_config_response_t);

      break;

    case NFAPI_NR_PHY_MSG_TYPE_PNF_START_REQUEST:
      if (unpackedBufLen >= sizeof(nfapi_nr_pnf_start_request_t))
        retLen = sizeof(nfapi_nr_pnf_start_request_t);

      break;

    case NFAPI_NR_PHY_MSG_TYPE_PNF_START_RESPONSE:
      if (unpackedBufLen >= sizeof(nfapi_nr_pnf_start_response_t))
        retLen = sizeof(nfapi_nr_pnf_start_response_t);

      break;

    case NFAPI_NR_PHY_MSG_TYPE_PNF_STOP_REQUEST:
      if (unpackedBufLen >= sizeof(nfapi_nr_pnf_stop_request_t))
        retLen = sizeof(nfapi_nr_pnf_stop_request_t);

      break;

    case NFAPI_NR_PHY_MSG_TYPE_PNF_STOP_RESPONSE:
      if (unpackedBufLen >= sizeof(nfapi_nr_pnf_stop_response_t))
        retLen = sizeof(nfapi_nr_pnf_stop_response_t);

      break;
    case NFAPI_NR_PHY_MSG_TYPE_PARAM_REQUEST:
    case NFAPI_NR_PHY_MSG_TYPE_PARAM_RESPONSE:
    case NFAPI_NR_PHY_MSG_TYPE_CONFIG_REQUEST:
    case NFAPI_NR_PHY_MSG_TYPE_CONFIG_RESPONSE:
    case NFAPI_NR_PHY_MSG_TYPE_START_REQUEST:
    case NFAPI_NR_PHY_MSG_TYPE_START_RESPONSE:
    case NFAPI_NR_PHY_MSG_TYPE_STOP_REQUEST:
    case NFAPI_NR_PHY_MSG_TYPE_STOP_INDICATION:
    case NFAPI_NR_PHY_MSG_TYPE_ERROR_INDICATION:
      retLen = check_nr_fapi_unpack_length(msgId, unpackedBufLen);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_STOP_RESPONSE:
      if (unpackedBufLen >= sizeof(nfapi_stop_response_t))
        retLen = sizeof(nfapi_stop_response_t);

      break;

    default:
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s Unknown message ID %d\n", __FUNCTION__, msgId);
      break;
  }

  return retLen;
}

static int check_unpack_length(nfapi_message_id_e msgId, uint32_t unpackedBufLen) {
  int retLen = 0;

  switch (msgId) {
    case NFAPI_PNF_PARAM_REQUEST:
      if (unpackedBufLen >= sizeof(nfapi_pnf_param_request_t))
        retLen = sizeof(nfapi_pnf_param_request_t);

      break;

    case NFAPI_PNF_PARAM_RESPONSE:
      if (unpackedBufLen >= sizeof(nfapi_pnf_param_response_t))
        retLen = sizeof(nfapi_pnf_param_response_t);

      break;

    case NFAPI_PNF_CONFIG_REQUEST:
      if (unpackedBufLen >= sizeof(nfapi_pnf_config_request_t))
        retLen = sizeof(nfapi_pnf_config_request_t);

      break;

    case NFAPI_PNF_CONFIG_RESPONSE:
      if (unpackedBufLen >= sizeof(nfapi_pnf_config_response_t))
        retLen = sizeof(nfapi_pnf_config_response_t);

      break;

    case NFAPI_PNF_START_REQUEST:
      if (unpackedBufLen >= sizeof(nfapi_pnf_start_request_t))
        retLen = sizeof(nfapi_pnf_start_request_t);

      break;

    case NFAPI_PNF_START_RESPONSE:
      if (unpackedBufLen >= sizeof(nfapi_pnf_start_response_t))
        retLen = sizeof(nfapi_pnf_start_response_t);

      break;

    case NFAPI_PNF_STOP_REQUEST:
      if (unpackedBufLen >= sizeof(nfapi_pnf_stop_request_t))
        retLen = sizeof(nfapi_pnf_stop_request_t);

      break;

    case NFAPI_PNF_STOP_RESPONSE:
      if (unpackedBufLen >= sizeof(nfapi_pnf_stop_response_t))
        retLen = sizeof(nfapi_pnf_stop_response_t);

      break;

    case NFAPI_PARAM_REQUEST:
      if (unpackedBufLen >= sizeof(nfapi_param_request_t))
        retLen = sizeof(nfapi_param_request_t);

      break;

    case NFAPI_PARAM_RESPONSE:
      if (unpackedBufLen >= sizeof(nfapi_param_response_t))
        retLen = sizeof(nfapi_param_response_t);

      break;

    case NFAPI_CONFIG_REQUEST:
      if (unpackedBufLen >= sizeof(nfapi_config_request_t))
        retLen = sizeof(nfapi_config_request_t);

      break;

    case NFAPI_CONFIG_RESPONSE:
      if (unpackedBufLen >= sizeof(nfapi_config_response_t))
        retLen = sizeof(nfapi_config_response_t);

      break;

    case NFAPI_START_REQUEST:
      if (unpackedBufLen >= sizeof( nfapi_start_request_t))
        retLen = sizeof( nfapi_start_request_t);

      break;

    case NFAPI_START_RESPONSE:
      if (unpackedBufLen >= sizeof(nfapi_start_response_t))
        retLen = sizeof(nfapi_start_response_t);

      break;

    case NFAPI_STOP_REQUEST:
      if (unpackedBufLen >= sizeof(nfapi_stop_request_t))
        retLen = sizeof(nfapi_stop_request_t);

      break;

    case NFAPI_STOP_RESPONSE:
      if (unpackedBufLen >= sizeof(nfapi_stop_response_t))
        retLen = sizeof(nfapi_stop_response_t);

      break;

    case NFAPI_MEASUREMENT_REQUEST:
      if (unpackedBufLen >= sizeof(nfapi_measurement_request_t))
        retLen = sizeof(nfapi_measurement_request_t);

      break;

    case NFAPI_MEASUREMENT_RESPONSE:
      if (unpackedBufLen >= sizeof(nfapi_measurement_response_t))
        retLen = sizeof(nfapi_measurement_response_t);

      break;

    default:
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s Unknown message ID %d\n", __FUNCTION__, msgId);
      break;
  }

  return retLen;
}

// Main unpack functions - public

int nfapi_p5_message_header_unpack(void *pMessageBuf,
                                   uint32_t messageBufLen,
                                   void *pUnpackedBuf,
                                   uint32_t unpackedBufLen,
                                   nfapi_p4_p5_codec_config_t *config)
{
  nfapi_p4_p5_message_header_t *pMessageHeader = pUnpackedBuf;
  uint8_t *pReadPackedMessage = pMessageBuf;

  if (pMessageBuf == NULL || pUnpackedBuf == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P5 header unpack supplied pointers are null\n");
    return -1;
  }

  uint8_t *end = pMessageBuf + messageBufLen;

  if (messageBufLen < NFAPI_HEADER_LENGTH || unpackedBufLen < sizeof(nfapi_p4_p5_message_header_t)) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P5 header unpack supplied message buffer is too small %d, %d\n", messageBufLen, unpackedBufLen);
    return -1;
  }

  // process the header
  if (!(pull16(&pReadPackedMessage, &pMessageHeader->phy_id, end) && pull16(&pReadPackedMessage, &pMessageHeader->message_id, end)
        && pull16(&pReadPackedMessage, &pMessageHeader->message_length, end)
        && pull16(&pReadPackedMessage, &pMessageHeader->spare, end))) {
    return -1;
  }

  return 8;
}

int nfapi_nr_p5_message_header_unpack(void *pMessageBuf,
                                      uint32_t messageBufLen,
                                      void *pUnpackedBuf,
                                      uint32_t unpackedBufLen,
                                      nfapi_p4_p5_codec_config_t *config)
{
  nfapi_nr_p4_p5_message_header_t *pMessageHeader = pUnpackedBuf;
  uint8_t *pReadPackedMessage = pMessageBuf;

  if (pMessageBuf == NULL || pUnpackedBuf == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P5 header unpack supplied pointers are null\n");
    return -1;
  }

  uint8_t *end = pMessageBuf + messageBufLen;

  if (messageBufLen < NFAPI_NR_P5_HEADER_LENGTH || unpackedBufLen < sizeof(nfapi_nr_p4_p5_message_header_t)) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P5 header unpack supplied message buffer is too small %d, %d\n", messageBufLen, unpackedBufLen);
    return -1;
  }
  // process the header
  if (!(pull16(&pReadPackedMessage, &pMessageHeader->phy_id, end) && pull16(&pReadPackedMessage, &pMessageHeader->message_id, end)
        && pull32(&pReadPackedMessage, &pMessageHeader->message_length, end)
        && pull16(&pReadPackedMessage, &pMessageHeader->spare, end))) {
    return -1;
  }
  return NFAPI_NR_P5_HEADER_LENGTH;
}

int nfapi_nr_p5_message_unpack(void *pMessageBuf,
                               uint32_t messageBufLen,
                               void *pUnpackedBuf,
                               uint32_t unpackedBufLen,
                               nfapi_p4_p5_codec_config_t *config)
{
  if (pMessageBuf == NULL || pUnpackedBuf == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P5 unpack supplied pointers are null\n");
    return -1;
  }
  if (messageBufLen < NFAPI_NR_P5_HEADER_LENGTH || unpackedBufLen < sizeof(nfapi_nr_p4_p5_message_header_t)) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P5 unpack supplied message buffer is too small %d, %d\n", messageBufLen, unpackedBufLen);
    return -1;
  }

  nfapi_nr_p4_p5_message_header_t *pMessageHeader = pUnpackedBuf;
  // clean the supplied buffer for - tag value blanking
  (void)memset(pUnpackedBuf, 0, unpackedBufLen);
  // process the header
  if (!nfapi_nr_p5_message_header_unpack(pMessageBuf, messageBufLen, pMessageHeader, unpackedBufLen, config)) {
    return -1;
  }
  uint8_t *pReadPackedMessage = pMessageBuf + NFAPI_NR_P5_HEADER_LENGTH;
  uint8_t *end = (uint8_t *)pMessageBuf + messageBufLen;
  int result = -1;

  if (check_nr_unpack_length(pMessageHeader->message_id, unpackedBufLen) == 0) {
    // the unpack buffer is not big enough for the struct
    return -1;
  }

  // look for the specific message
  switch (pMessageHeader->message_id) {
    case NFAPI_NR_PHY_MSG_TYPE_PNF_PARAM_REQUEST:
      result = unpack_nr_pnf_param_request(&pReadPackedMessage, end, pMessageHeader, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_PNF_PARAM_RESPONSE:
      result = unpack_nr_pnf_param_response(&pReadPackedMessage, end, pMessageHeader, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_PNF_CONFIG_REQUEST:
      result = unpack_nr_pnf_config_request(&pReadPackedMessage, end, pMessageHeader, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_PNF_CONFIG_RESPONSE:
      result = unpack_nr_pnf_config_response(&pReadPackedMessage, end, pMessageHeader, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_PNF_START_REQUEST:
      result = unpack_nr_pnf_start_request(&pReadPackedMessage, end, pMessageHeader, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_PNF_START_RESPONSE:
      result = unpack_nr_pnf_start_response(&pReadPackedMessage, end, pMessageHeader, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_PNF_STOP_REQUEST:
      result = unpack_pnf_stop_request(&pReadPackedMessage, end, pMessageHeader, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_PNF_STOP_RESPONSE:
      result = unpack_pnf_stop_response(&pReadPackedMessage, end, pMessageHeader, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_PARAM_REQUEST:
      result = unpack_nr_param_request(&pReadPackedMessage, end, pMessageHeader, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_PARAM_RESPONSE:
      result = unpack_nr_param_response(&pReadPackedMessage, end, pMessageHeader, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_CONFIG_REQUEST:
      result = unpack_nr_config_request(&pReadPackedMessage, end, pMessageHeader, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_CONFIG_RESPONSE:
      result = unpack_nr_config_response(&pReadPackedMessage, end, pMessageHeader, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_START_REQUEST:
      result = unpack_nr_start_request(&pReadPackedMessage, end, pMessageHeader, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_START_RESPONSE:
      result = unpack_nr_start_response(&pReadPackedMessage, end, pMessageHeader, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_STOP_REQUEST:
      result = unpack_stop_request(&pReadPackedMessage, end, pMessageHeader, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_STOP_RESPONSE:
      result = unpack_stop_response(&pReadPackedMessage, end, pMessageHeader, config);
      break;

    case NFAPI_MEASUREMENT_REQUEST:
      result = unpack_measurement_request(&pReadPackedMessage, end, pMessageHeader, config);
      break;

    case NFAPI_MEASUREMENT_RESPONSE:
      result = unpack_measurement_response(&pReadPackedMessage, end, pMessageHeader, config);
      break;

    default:
      if (pMessageHeader->message_id >= NFAPI_VENDOR_EXT_MSG_MIN && pMessageHeader->message_id <= NFAPI_VENDOR_EXT_MSG_MAX) {
        if (config && config->unpack_p4_p5_vendor_extension) {
          result = (config->unpack_p4_p5_vendor_extension)(pMessageHeader, &pReadPackedMessage, end, config);
        } else {
          NFAPI_TRACE(NFAPI_TRACE_ERROR,
                      "%s VE NFAPI message ID %d. No ve decoder provided\n",
                      __FUNCTION__,
                      pMessageHeader->message_id);
        }
      } else {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s NFAPI Unknown P5 message ID %d\n", __FUNCTION__, pMessageHeader->message_id);
      }

      break;
  }

  return result;
}

int nfapi_p5_message_unpack(void *pMessageBuf,
                            uint32_t messageBufLen,
                            void *pUnpackedBuf,
                            uint32_t unpackedBufLen,
                            nfapi_p4_p5_codec_config_t *config)
{
  nfapi_p4_p5_message_header_t *pMessageHeader = pUnpackedBuf;
  uint8_t *pReadPackedMessage = pMessageBuf;

  if (pMessageBuf == NULL || pUnpackedBuf == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P5 unpack supplied pointers are null\n");
    return -1;
  }

  uint8_t *end = (uint8_t *)pMessageBuf + messageBufLen;

  if (messageBufLen < NFAPI_HEADER_LENGTH || unpackedBufLen < sizeof(nfapi_p4_p5_message_header_t)) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P5 unpack supplied message buffer is too small %d, %d\n", messageBufLen, unpackedBufLen);
    return -1;
  }

  uint8_t *ptr = pReadPackedMessage;
  printf("\n Read LTE message unpack: ");

  while (ptr < end) {
    printf(" %02x ", *ptr);
    ptr++;
  }

  printf("\n");
  // clean the supplied buffer for - tag value blanking
  (void)memset(pUnpackedBuf, 0, unpackedBufLen);

  // process the header
  if (!(pull16(&pReadPackedMessage, &pMessageHeader->phy_id, end) && pull16(&pReadPackedMessage, &pMessageHeader->message_id, end)
        && pull16(&pReadPackedMessage, &pMessageHeader->message_length, end)
        && pull16(&pReadPackedMessage, &pMessageHeader->spare, end))) {
    // failed to read the header
    return -1;
  }

  int result = -1;

  if (check_unpack_length(pMessageHeader->message_id, unpackedBufLen) == 0) {
    // the unpack buffer is not big enough for the struct
    return -1;
  }

  // look for the specific message
  switch (pMessageHeader->message_id) {
    case NFAPI_PNF_PARAM_REQUEST:
      result = unpack_pnf_param_request(&pReadPackedMessage, end, pMessageHeader, config);
      break;

    case NFAPI_PNF_PARAM_RESPONSE:
      result = unpack_pnf_param_response(&pReadPackedMessage, end, pMessageHeader, config);
      break;

    case NFAPI_PNF_CONFIG_REQUEST:
      result = unpack_pnf_config_request(&pReadPackedMessage, end, pMessageHeader, config);
      break;

    case NFAPI_PNF_CONFIG_RESPONSE:
      result = unpack_pnf_config_response(&pReadPackedMessage, end, pMessageHeader, config);
      break;

    case NFAPI_PNF_START_REQUEST:
      result = unpack_pnf_start_request(&pReadPackedMessage, end, pMessageHeader, config);
      break;

    case NFAPI_PNF_START_RESPONSE:
      result = unpack_pnf_start_response(&pReadPackedMessage, end, pMessageHeader, config);
      break;

    case NFAPI_PNF_STOP_REQUEST:
      result = unpack_pnf_stop_request(&pReadPackedMessage, end, pMessageHeader, config);
      break;

    case NFAPI_PNF_STOP_RESPONSE:
      result = unpack_pnf_stop_response(&pReadPackedMessage, end, pMessageHeader, config);
      break;

    case NFAPI_PARAM_REQUEST:
      result = unpack_param_request(&pReadPackedMessage, end, pMessageHeader, config);
      break;

    case NFAPI_PARAM_RESPONSE:
      result = unpack_param_response(&pReadPackedMessage, end, pMessageHeader, config);
      break;

    case NFAPI_CONFIG_REQUEST:
      result = unpack_config_request(&pReadPackedMessage, end, pMessageHeader, config);
      break;

    case NFAPI_CONFIG_RESPONSE:
      result = unpack_config_response(&pReadPackedMessage, end, pMessageHeader, config);
      break;

    case NFAPI_START_REQUEST:
      result = unpack_start_request(&pReadPackedMessage, end, pMessageHeader, config);
      break;

    case NFAPI_START_RESPONSE:
      result = unpack_start_response(&pReadPackedMessage, end, pMessageHeader, config);
      break;

    case NFAPI_STOP_REQUEST:
      result = unpack_stop_request(&pReadPackedMessage, end, pMessageHeader, config);
      break;

    case NFAPI_STOP_RESPONSE:
      result = unpack_stop_response(&pReadPackedMessage, end, pMessageHeader, config);
      break;

    case NFAPI_MEASUREMENT_REQUEST:
      result = unpack_measurement_request(&pReadPackedMessage, end, pMessageHeader, config);
      break;

    case NFAPI_MEASUREMENT_RESPONSE:
      result = unpack_measurement_response(&pReadPackedMessage, end, pMessageHeader, config);
      break;

    default:
      if (pMessageHeader->message_id >= NFAPI_VENDOR_EXT_MSG_MIN && pMessageHeader->message_id <= NFAPI_VENDOR_EXT_MSG_MAX) {
        if (config && config->unpack_p4_p5_vendor_extension) {
          result = (config->unpack_p4_p5_vendor_extension)(pMessageHeader, &pReadPackedMessage, end, config);
        } else {
          NFAPI_TRACE(NFAPI_TRACE_ERROR,
                      "%s VE NFAPI message ID %d. No ve decoder provided\n",
                      __FUNCTION__,
                      pMessageHeader->message_id);
        }
      } else {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s NFAPI Unknown P5 message ID %d\n", __FUNCTION__, pMessageHeader->message_id);
      }

      break;
  }

  return result;
}
