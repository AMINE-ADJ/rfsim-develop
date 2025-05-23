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

#include "RegistrationComplete.h"
#include <stdint.h>
#include "SORTransparentContainer.h"

int decode_registration_complete(registration_complete_msg *registration_complete, const uint8_t *buffer, uint32_t len)
{
  uint32_t decoded = 0;
  int decoded_result = 0;

  /* Decoding mandatory fields */
  if ((decoded_result =
           decode_sor_transparent_container(registration_complete->sortransparentcontainer, 0, buffer + decoded, len - decoded))
      < 0)
    return decoded_result;
  else
    decoded += decoded_result;

  return decoded;
}

int encode_registration_complete(const registration_complete_msg *registration_complete, uint8_t *buffer, uint32_t len)
{
  int encoded = 0;
  int encode_result = 0;

  if (!registration_complete->sortransparentcontainer)
    return encoded;

  if ((encode_result =
           encode_sor_transparent_container(registration_complete->sortransparentcontainer, 0, buffer + encoded, len - encoded))
      < 0) // Return in case of error
    return encode_result;
  else
    encoded += encode_result;

  return encoded;
}
