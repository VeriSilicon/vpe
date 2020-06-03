/* Copyright 2013 Google Inc. All Rights Reserved. */

/* Hantro G2 decoder application programming interface.
 *
 * This file contains the interface for accessing the Hantro G2 video decoder
 * functionality. API is system agnostic and abstracted from the system through
 * Codec Wrapper Layer, that defines the functionality that must be provided by
 * the operating system.
 *
 * Even though API is asynchronous in nature the calls to functions must be
 * considered potentially blocking calls that are released by various hardware
 * events that will allow the pipeline to carry on. For example, DecDecode will
 * block until there are free buffers to carry the input to the decoder
 * hardware.
 *
 * Decoder API relies on client to provide certain functionality during runtime,
 * which is collected as function pointers into struct struct struct struct
 *struct struct DecClientHandle structure.
 * Client must provide this functionality during the initialization of the
 * component.
 */

#ifndef DECAPI_H
#define DECAPI_H

#include "software/inc/basetype.h"
#include "software/inc/dectypes.h"
//#include "software/inc/cwl.h"

/* Decoder client interface. */
typedef const void* ClientInst; /* Opaque pointer to the client. */

/* Function to notify the client that decoder has successfully initialized. */
typedef void ClientInitialized(ClientInst inst);
/* Function to notify about successful decoding of the stream parameters.
 * Decoder expects client to provide needed buffers through DecSetPictureBuffers
 * function to continue decoding the actual stream. */
typedef void ClientHeadersDecoded(ClientInst inst,
                                  struct DecSequenceInfo sequence_info);
/* Function to notify client that a buffer has been consumed by the decoder and
 * it can be handled freely by the client. */
typedef void ClientBufferDecoded(ClientInst inst, struct DecInput* input);
/* Function to notify about picture that is ready to be outputted. Client is
 * expected to notify the decoder when it has finished processing the picture,
 * so decoder can reuse the picture buffer for another picture. */
typedef void ClientPictureReady(ClientInst inst, struct DecPicture picture);
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
  ClientPictureReady* PictureReady;
  ClientEndOfStream* EndOfStream;
  ClientReleased* Released;
  ClientNotifyError* NotifyError;
};

/* Decoder interface. */
typedef const void* DecInst; /* Opaque pointer to the decoder instance. */

/* Function to query build information about the software and hardware build of
 * the underlying hardware decoder. */
struct DecSwHwBuild DecGetBuild(void);

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

/* Function to assign picture buffers for the decoder. When decoder has finished
 * decoding the stream headers and knows which types of buffers and how many of
 * them it will need, it will inform that through the HeadersDecoded callback.
 * Buffers must not be written into until client has successfully called
 * DecRelease or decoder has requested new set of buffers through HeadersDecoded
 * callback. */
enum DecRet DecSetPictureBuffers(DecInst dec_inst,
                                 const struct CWLLinearMem* buffers,
                                 u32 num_of_buffers);

/* Function to tell the decoder that client has finished processing a specific
 * picture that was previously sent to client through the PictureReady callback.
 */
enum DecRet DecPictureConsumed(DecInst dec_inst, struct DecPicture picture);

/* Function to tell the decoder that it should not be expecting any more input
 * stream and Finish decoding and outputting all the buffers that are currently
 * pending in the component. Once decoder has finished outputting the pending
 * pictures it will notify the client about it by calling the EndOfStream
 * callback. */
enum DecRet DecEndOfStream(DecInst dec_inst);

/* Function to release the decoder instance. When the function returns decoder
 * has released all the resources it has acquired. */
void DecRelease(DecInst dec_inst);

#endif /* DECAPI_H */
