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

#ifndef SCTP_MESSAGES_TYPES_H_
#define SCTP_MESSAGES_TYPES_H_

#include <netinet/in.h>
#include <netinet/sctp.h>

#define SCTP_NEW_ASSOCIATION_REQ(mSGpTR)       (mSGpTR)->ittiMsg.sctp_new_association_req
#define SCTP_NEW_ASSOCIATION_REQ_MULTI(mSGpTR) (mSGpTR)->ittiMsg.sctp_new_association_req_multi
#define SCTP_NEW_ASSOCIATION_RESP(mSGpTR)      (mSGpTR)->ittiMsg.sctp_new_association_resp
#define SCTP_NEW_ASSOCIATION_IND(mSGpTR)       (mSGpTR)->ittiMsg.sctp_new_association_ind
#define SCTP_DATA_IND(mSGpTR)                  (mSGpTR)->ittiMsg.sctp_data_ind
#define SCTP_DATA_REQ(mSGpTR)                  (mSGpTR)->ittiMsg.sctp_data_req
#define SCTP_INIT_MSG(mSGpTR)                  (mSGpTR)->ittiMsg.sctp_init
#define SCTP_INIT_MSG_MULTI_REQ(mSGpTR)        (mSGpTR)->ittiMsg.sctp_init_multi
#define SCTP_INIT_MSG_MULTI_CNF(mSGpTR)        (mSGpTR)->ittiMsg.sctp_init_msg_multi_cnf
#define SCTP_CLOSE_ASSOCIATION(mSGpTR)         (mSGpTR)->ittiMsg.sctp_close_association

#define SCTP_STATES \
    ITEM(SCTP_STATE_CLOSED) \
    ITEM(SCTP_STATE_SHUTDOWN) \
    ITEM(SCTP_STATE_ESTABLISHED) \
    ITEM(SCTP_STATE_UNREACHABLE)

typedef enum {
#define ITEM(state) state,
    SCTP_STATES
#undef ITEM
} sctp_state_e;

static const char *sctp_state_s[] __attribute__((unused)) = {
#define ITEM(state) #state,
    SCTP_STATES
#undef ITEM
};

typedef struct sctp_new_association_req_s {
  /* Upper layer connection identifier */
  uint16_t         ulp_cnx_id;

  /* The port to connect to */
  uint16_t         port;
  /* Payload Protocol Identifier to use */
  uint32_t         ppid;

  /* Number of streams used for this association */
  uint16_t in_streams;
  uint16_t out_streams;

  /* Local address to bind to */
  net_ip_address_t local_address;
  /* Remote address to connect to */
  net_ip_address_t remote_address;
} sctp_new_association_req_t;

typedef struct sctp_new_association_req_multi_s {
  /* Upper layer connection identifier */
  uint16_t         ulp_cnx_id;

  /* The port to connect to */
  uint16_t         port;
  /* Payload Protocol Identifier to use */
  uint32_t         ppid;

  /* Number of streams used for this association */
  uint16_t in_streams;
  uint16_t out_streams;

  /* Local address to bind to */
  net_ip_address_t local_address;
  /* Remote address to connect to */
  net_ip_address_t remote_address;

  /* Multi-socket descriptor */
  int multi_sd;
} sctp_new_association_req_multi_t;

typedef struct sctp_init_msg_multi_cnf_s {
  int multi_sd;
} sctp_init_msg_multi_cnf_t;

typedef struct sctp_new_association_ind_s {
  /* Assoc id of the new association */
  sctp_assoc_t assoc_id;

  /* The port used by remote host */
  uint16_t port;

  /* Number of streams used for this association */
  uint16_t in_streams;
  uint16_t out_streams;
} sctp_new_association_ind_t;

typedef struct sctp_new_association_resp_s {
  /* Upper layer connection identifier */
  uint16_t ulp_cnx_id;

  /* SCTP Association ID */
  sctp_assoc_t assoc_id;

  /* Input/output streams */
  uint16_t out_streams;
  uint16_t in_streams;

  /* State of the association at SCTP level */
  sctp_state_e sctp_state;
} sctp_new_association_resp_t;

typedef struct sctp_data_ind_s {
  /* SCTP Association ID */
  sctp_assoc_t assoc_id;

  /* Buffer to send over SCTP */
  uint32_t  buffer_length;
  uint8_t  *buffer;

  /* Streams on which data will be sent/received */
  uint16_t  stream;
} sctp_data_ind_t;

typedef struct sctp_init_s {
  char *bind_address;
  uint16_t  port;
  uint32_t  ppid;
} sctp_init_t;


typedef struct sctp_close_association_s {
  sctp_assoc_t assoc_id;
} sctp_close_association_t;


typedef sctp_data_ind_t sctp_data_req_t;

typedef struct sctp_listener_register_upper_layer_s {
  /* Port to listen to */
  uint16_t port;
  /* Payload protocol identifier
   * Any data received on PPID != will be discarded
   */
  uint32_t ppid;
} sctp_listener_register_upper_layer_t;

#endif /* SCTP_MESSAGES_TYPES_H_ */
