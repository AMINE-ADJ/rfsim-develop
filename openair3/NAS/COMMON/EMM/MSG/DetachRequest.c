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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


#include "TLVEncoder.h"
#include "TLVDecoder.h"
#include "DetachRequest.h"

int decode_detach_request(detach_request_msg *detach_request, uint8_t *buffer, uint32_t len)
{
  uint32_t decoded = 0;
  int decoded_result = 0;

  // Check if we got a NULL pointer and if buffer length is >= minimum length expected for the message.
  CHECK_PDU_POINTER_AND_LENGTH_DECODER(buffer, DETACH_REQUEST_MINIMUM_LENGTH, len);

  /* Decoding mandatory fields */
  if ((decoded_result = decode_u8_detach_type(&detach_request->detachtype, 0, *(buffer + decoded) & 0x0f, len - decoded)) < 0)
    return decoded_result;

  if ((decoded_result = decode_nas_key_set_identifier(&detach_request->naskeysetidentifier, 0, *(buffer + decoded) >> 4)) < 0)
    return decoded_result;

  decoded++;

  if ((decoded_result = decode_eps_mobile_identity(&detach_request->gutiorimsi, 0, buffer + decoded, len - decoded)) < 0)
    return decoded_result;
  else
    decoded += decoded_result;

  return decoded;
}

int encode_detach_request(detach_request_msg *detach_request, uint8_t *buffer, uint32_t len)
{
  int encoded = 0;
  int encode_result = 0;

  /* Checking IEI and pointer */
  CHECK_PDU_POINTER_AND_LENGTH_ENCODER(buffer, DETACH_REQUEST_MINIMUM_LENGTH, len);

  *(buffer + encoded) = ((encode_nas_key_set_identifier(&detach_request->naskeysetidentifier, 0) << 4)
                         | (encode_u8_detach_type(&detach_request->detachtype) & 0x0f));
  encoded++;

  if ((encode_result = encode_eps_mobile_identity(&detach_request->gutiorimsi,
                       0, buffer + encoded, len - encoded)) < 0)        //Return in case of error
    return encode_result;
  else
    encoded += encode_result;

  return encoded;
}

