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

/*! \file FGSRegistrationType.c
 * \brief 5GS Registration Type for registration request procedures
 * \author Yoshio INOUE, Masayuki HARADA
 * \email yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
 * \date 2020
 * \version 0.1
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "FGSRegistrationType.h"

#define NO_FOLLOW_ON_REQUEST 0x0
#define FOLLOW_ON_REQUEST 0x08

int decode_5gs_registration_type(FGSRegistrationType *fgsregistrationtype, uint8_t iei, uint8_t value, uint32_t len)
{
  int decoded = 0;
  uint8_t *buffer = &value;
  if (iei > 0) {
    CHECK_IEI_DECODER((*buffer & 0xf0), iei);
  }

  *fgsregistrationtype = *buffer & 0x7;
  decoded++;

  return decoded;
}

/**
 * @brief Encode 5GS registration type (9.11.3.7 of 3GPP TS 24.501)
 *        Note: mandatory IE (IEI = 0)
 */
int encode_5gs_registration_type(const FGSRegistrationType *fgsregistrationtype, bool follow_on_request)
{
  // Follow-on request bit
  uint8_t foR = follow_on_request ? FOLLOW_ON_REQUEST : NO_FOLLOW_ON_REQUEST;
  // Return encoded value
  return (foR | (*fgsregistrationtype & 0x7));
}
