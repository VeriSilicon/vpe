/*------------------------------------------------------------------------------
--Copyright (c) 2015-2017, VeriSilicon Holdings Co., Ltd. All rights reserved --
--         Copyright (c) 2011-2014, Google Inc. All rights reserved.          --
--         Copyright (c) 2007-2010, Hantro OY. All rights reserved.           --
--                                                                            --
-- This software is confidential and proprietary and may be used only as      --
--   expressly authorized by VeriSilicon in a written licensing agreement.    --
--                                                                            --
--         This entire notice must be reproduced on all copies                --
--                       and may not be removed.                              --
--                                                                            --
--------------------------------------------------------------------------------
-- Redistribution and use in source and binary forms, with or without         --
-- modification, are permitted provided that the following conditions are met:--
--   * Redistributions of source code must retain the above copyright notice, --
--       this list of conditions and the following disclaimer.                --
--   * Redistributions in binary form must reproduce the above copyright      --
--       notice, this list of conditions and the following disclaimer in the  --
--       documentation and/or other materials provided with the distribution. --
--   * Neither the names of Google nor the names of its contributors may be   --
--       used to endorse or promote products derived from this software       --
--       without specific prior written permission.                           --
--------------------------------------------------------------------------------
-- THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"--
-- AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE  --
-- IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE --
-- ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE  --
-- LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR        --
-- CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF       --
-- SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS   --
-- INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN    --
-- CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)    --
-- ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE --
-- POSSIBILITY OF SUCH DAMAGE.                                                --
--------------------------------------------------------------------------------
------------------------------------------------------------------------------*/

#ifndef DECAPI_H
#define DECAPI_H

#include "basetype.h"
#include "dectypes.h"
#include "dwl.h"

/* Decoder client interface. */
typedef const void* ClientInst; /* Opaque pointer to the client. */

/* Function to notify the client that decoder has successfully initialized. */
typedef void ClientInitialized(ClientInst inst);
/* Function to notify about successful decoding of the stream parameters.
 * Decoder expects client to provide needed buffers through DecSetPictureBuffers
 * function to continue decoding the actual stream. */
typedef void ClientHeadersDecoded(ClientInst inst,
                                  struct DecSequenceInfo sequence_info);
/* Functions to request external buffers to be allocated/freed. */
typedef u32 ClientRequestExtBuffers(ClientInst inst);
/* Function to notify client that a buffer has been consumed by the decoder and
 * it can be handled freely by the client. */
typedef void ClientBufferDecoded(ClientInst inst, struct DecInput* input);
/* Function to notify about picture that is ready to be outputted. Client is
 * expected to notify the decoder when it has finished processing the picture,
 * so decoder can reuse the picture buffer for another picture. */
typedef void ClientPictureReady(ClientInst inst, struct DecPicturePpu picture);
/* Function to notify the client that all the pending pictures have been
 * outputted and decoder can be safely shut down. */
typedef void ClientEndOfStream(ClientInst inst);
/* Function to notify the client that decoder has shut down. */
typedef void ClientReleased(ClientInst inst);
/* Function to notify client about error in the decoding process. */
typedef void ClientNotifyError(ClientInst inst, u32 pic_id, enum DecRet rv);

/* Datatype to package information about client instance for decoder's use. */
struct DecClientHandle {
  ClientInst client;
  ClientInitialized* Initialized;
  ClientHeadersDecoded* HeadersDecoded;
  ClientBufferDecoded* BufferDecoded;
#ifdef USE_EXTERNAL_BUFFER
  ClientRequestExtBuffers *ExtBufferReq;
#endif
  ClientPictureReady* PictureReady;
  ClientEndOfStream* EndOfStream;
  ClientReleased* Released;
  ClientNotifyError* NotifyError;
};

/* Decoder interface. */
typedef const void* DecInst; /* Opaque pointer to the decoder instance. */

/* Function to query build information about the software and hardware build of
 * the underlying hardware decoder. */
struct DecSwHwBuild DecGetBuild(const void* dwl);

/* Function to initialize the decoder. Functionality provided by the client to
 * the component are collected into the callbacks parameter. Client is expected
 * to outlive the decoder component, i.e. the callbacks and instance given in
 * callbacks parameter must be valid until client has successfully executed
 * a call to DecRelease function. */
enum DecRet DecInit(enum DecCodec codec, DecInst* decoder,
                    struct DecConfig config, struct DecClientHandle callbacks);

/* Function to dispatch a buffer containing video bitstream to be decoded by the
 * component. Buffer can be reused after the function has returned, during the
 * call the buffer must not be written into. */
enum DecRet DecDecode(DecInst dec_inst, struct DecInput* input);

#ifdef USE_EXTERNAL_BUFFER
enum DecRet DecGetPictureBuffersInfo(DecInst dec_inst, struct DecBufferInfo *info);
#endif

/* Function to assign picture buffers for the decoder. When decoder has finished
 * decoding the stream headers and knows which types of buffers and how many of
 * them it will need, it will inform that through the HeadersDecoded callback.
 * Buffers must not be written into until client has successfully called
 * DecRelease or decoder has requested new set of buffers through HeadersDecoded
 * callback. */
enum DecRet DecSetPictureBuffers(DecInst dec_inst,
                                 const struct DWLLinearMem* buffers,
                                 u32 num_of_buffers);

/* Function to tell the decoder that client has finished processing a specific
 * picture that was previously sent to client through the PictureReady callback.
 */
enum DecRet DecPictureConsumed(DecInst dec_inst, struct DecPicturePpu picture);

/* Function to tell the decoder that it should not be expecting any more input
 * stream and Finish decoding and outputting all the buffers that are currently
 * pending in the component. Once decoder has finished outputting the pending
 * pictures it will notify the client about it by calling the EndOfStream
 * callback. */
enum DecRet DecEndOfStream(DecInst dec_inst);

/* Function to release the decoder instance. When the function returns decoder
 * has released all the resources it has acquired. */
void DecRelease(DecInst dec_inst);

enum DecRet DecUseExtraFrmBuffers(DecInst dec_inst, u32 n);

#endif /* DECAPI_H */
