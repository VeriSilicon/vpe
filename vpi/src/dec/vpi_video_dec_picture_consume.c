/*
 * Copyright 2019 VeriSilicon, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "vpi_log.h"
#include "vpi_video_dec_picture_consume.h"

void init_dec_pic_wait_consume_list(VpiDecCtx *vpi_ctx)
{
    uint32_t i;
    pthread_mutex_init(&vpi_ctx->consume_mutex, NULL);

    pthread_mutex_lock(&vpi_ctx->consume_mutex);
    for (i = 0; i < MAX_WAIT_FOR_CONSUME_BUFFERS; i++) {
        vpi_ctx->wait_for_consume_list[i].pic              = NULL;
        vpi_ctx->wait_for_consume_list[i].wait_for_consume = 0;
    }
    vpi_ctx->wait_consume_num = 0;
    pthread_mutex_unlock(&vpi_ctx->consume_mutex);
}

uint32_t find_dec_pic_wait_consume_index(VpiDecCtx *vpi_ctx, uint8_t *data)
{
    uint32_t i;

    pthread_mutex_lock(&vpi_ctx->consume_mutex);
    for (i = 0; i < MAX_WAIT_FOR_CONSUME_BUFFERS; i++) {
        if ((uint8_t *)vpi_ctx->wait_for_consume_list[i].pic == data) {
            break;
        }
    }

    ASSERT(i < MAX_WAIT_FOR_CONSUME_BUFFERS);
    pthread_mutex_unlock(&vpi_ctx->consume_mutex);
    return i;
}

uint32_t find_dec_pic_wait_consume_empty_index(VpiDecCtx *vpi_ctx)
{
    uint32_t i;

    pthread_mutex_lock(&vpi_ctx->consume_mutex);
    for (i = 0; i < MAX_WAIT_FOR_CONSUME_BUFFERS; i++) {
        if (vpi_ctx->wait_for_consume_list[i].wait_for_consume == 0) {
            break;
        }
    }

    ASSERT(i < MAX_WAIT_FOR_CONSUME_BUFFERS);
    pthread_mutex_unlock(&vpi_ctx->consume_mutex);

    return i;
}

void add_dec_pic_wait_consume_list(VpiDecCtx *vpi_ctx, void *data)
{
    uint32_t id;

    id = find_dec_pic_wait_consume_empty_index(vpi_ctx);

    VPILOGD("AddDecPicWaitConsumeList pic(@ %p) @ %d \n", data, id);

    pthread_mutex_lock(&vpi_ctx->consume_mutex);
    vpi_ctx->wait_for_consume_list[id].pic = (struct DecPicturePpu *)data;
    vpi_ctx->wait_for_consume_list[id].wait_for_consume = 1;
    if (vpi_ctx->wait_consume_num < MAX_WAIT_FOR_CONSUME_BUFFERS) {
        vpi_ctx->wait_consume_num++;
    }
    pthread_mutex_unlock(&vpi_ctx->consume_mutex);
}

void del_dec_pic_wait_consume_list(VpiDecCtx *vpi_ctx, uint8_t *data)
{
    uint32_t id;
    id = find_dec_pic_wait_consume_index(vpi_ctx, data);

    pthread_mutex_lock(&vpi_ctx->consume_mutex);
    if (vpi_ctx->wait_consume_num == 0) {
        pthread_mutex_unlock(&vpi_ctx->consume_mutex);
        return;
    }
    if (id < MAX_WAIT_FOR_CONSUME_BUFFERS) {
        vpi_ctx->wait_for_consume_list[id].pic              = NULL;
        vpi_ctx->wait_for_consume_list[id].wait_for_consume = 0;
        if (vpi_ctx->wait_consume_num > 0) {
            vpi_ctx->wait_consume_num--;
        }
        VPILOGD("DelDecPicWaitConsumeList pic(@ %p) @ %d \n", data, id);
    }
    pthread_mutex_unlock(&vpi_ctx->consume_mutex);
}

void free_dec_pic_wait_consume_list(VpiDecCtx *vpi_ctx)
{
    uint32_t i;
    struct DecPicturePpu pic;

    pthread_mutex_lock(&vpi_ctx->consume_mutex);
    for (i = 0; i < MAX_WAIT_FOR_CONSUME_BUFFERS; i++) {
        if (vpi_ctx->wait_for_consume_list[i].wait_for_consume == 1) {
            VPILOGD("find pic(@ %p) @ %d NO consume! force consume at close\n",
                vpi_ctx->wait_for_consume_list[i].pic, i);
            if(vpi_ctx->vpi_dec_wrapper.picture_consumed) {
                pic = *((struct DecPicturePpu *)vpi_ctx
                        ->wait_for_consume_list[i].pic);
                vpi_ctx->vpi_dec_wrapper.picture_consumed(vpi_ctx, pic);
            }
            vpi_ctx->wait_for_consume_list[i].wait_for_consume = 0;
            free(vpi_ctx->wait_for_consume_list[i].pic);
            vpi_ctx->wait_for_consume_list[i].pic = NULL;
        }
    }
    vpi_ctx->wait_consume_num = 0;
    pthread_mutex_unlock(&vpi_ctx->consume_mutex);
    pthread_mutex_destroy(&vpi_ctx->consume_mutex);
}
