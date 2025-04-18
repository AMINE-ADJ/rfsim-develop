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

#ifndef TLV_ENCODER_H_
#define TLV_ENCODER_H_

#include <arpa/inet.h>  // htonl, htons
#include <string.h>     // memcpy

#define ENCODE_U8(buffer, value, size) \
  do {                                 \
    *(uint8_t*)(buffer) = value;       \
    size += sizeof(uint8_t);           \
  } while (0)

/* Safely encodes a 16-bit value into a buffer, handling
   misalignment by memcpy 2 bytes to buffer in network
   byte order (big-endian). */
#define ENCODE_U16(buffer, value, size)        \
  do {                                         \
    uint16_t _val = htons(value);              \
    memcpy((buffer), &_val, sizeof(uint16_t)); \
    size += sizeof(uint16_t);                  \
  } while (0)

/* Safely encodes a 24-bit value into a buffer, handling
   misalignment by using htonl and memcpy to copy 3 bytes
   in network byte order (big-endian). */
#define ENCODE_U24(buffer, value, size)         \
  do {                                          \
    uint32_t _val = htonl(value);               \
    memcpy((buffer), ((uint8_t*)&_val) + 1, 3); \
    size += sizeof(uint8_t) + sizeof(uint16_t); \
  } while (0)

#define ENCODE_U32(buffer, value, size) \
  do {                                  \
    uint32_t tmp = htonl(value);        \
    memcpy(buffer, &tmp, sizeof(tmp));  \
    size += sizeof(uint32_t);           \
  } while (0)

#define IES_ENCODE_U8(buffer, encoded, value)   \
    ENCODE_U8(buffer + encoded, value, encoded)

#define IES_ENCODE_U16(buffer, encoded, value)    \
    ENCODE_U16(buffer + encoded, value, encoded)

#define IES_ENCODE_U24(buffer, encoded, value)    \
    ENCODE_U24(buffer + encoded, value, encoded)

#define IES_ENCODE_U32(buffer, encoded, value)    \
    ENCODE_U32(buffer + encoded, value, encoded)

typedef enum {
  TLV_ENCODE_ERROR_OK                      =  0,
  TLV_ENCODE_VALUE_DOESNT_MATCH            = -1,
  /* Fatal errors - message should not be sent */
  TLV_ENCODE_OCTET_STRING_TOO_LONG_FOR_IEI = -10,
  TLV_ENCODE_WRONG_MESSAGE_TYPE            = -11,
  TLV_ENCODE_PROTOCOL_NOT_SUPPORTED        = -12,
  TLV_ENCODE_BUFFER_TOO_SHORT              = -13,
  TLV_ENCODE_BUFFER_NULL                   = -14,
} tlv_encoder_error_code;

/* Defines error code limit below which message should be sent because
 * it cannot be further processed */
#define TLV_ENCODE_FATAL_ERROR  (TLV_ENCODE_VALUE_DOESNT_MATCH)

#ifdef ENABLE_TESTS
#define TLV_ENC_ERROR(...) fprintf(stderr, "TLV Encoder: " __VA_ARGS__)
#else
#define TLV_ENC_ERROR(...) // Do nothing
#endif

#define CHECK_PDU_POINTER_AND_LENGTH_ENCODER(bUFFER, mINIMUMlENGTH, lENGTH)                                          \
  do {                                                                                                               \
    if ((bUFFER) == NULL) {                                                                                          \
      TLV_ENC_ERROR("Got NULL pointer for the payload\n");                                                           \
      return TLV_ENCODE_BUFFER_NULL;                                                                                 \
    }                                                                                                                \
    if ((lENGTH) < (mINIMUMlENGTH)) {                                                                                \
      TLV_ENC_ERROR("(%s:%d) Expecting at least %d bytes, got %u\n", __FILE__, __LINE__, (mINIMUMlENGTH), (lENGTH)); \
      return TLV_ENCODE_BUFFER_TOO_SHORT;                                                                            \
    }                                                                                                                \
  } while (0)

#define CHECK_PDU_POINTER_ENCODER(bUFFER)                  \
  do {                                                     \
    if ((bUFFER) == NULL) {                                \
      TLV_ENC_ERROR("Got NULL pointer for the payload\n"); \
      errorCodeEncoder = TLV_ENCODE_BUFFER_NULL;           \
      return TLV_ENCODE_BUFFER_NULL;                       \
    }                                                      \
  } while (0)

#endif /* define (TLV_ENCODER_H_) */

