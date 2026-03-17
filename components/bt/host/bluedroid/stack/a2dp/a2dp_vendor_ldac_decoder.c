/**
 * SPDX-FileCopyrightText: 2016 The Android Open Source Project
 * 
 * SPDX-License-Identifier: Apache-2.0
 * 
 * 本文件依赖由hegdi和Thealexbarney开发的外部库libldacdec中的ldacdec.h
 * O2C14的ldacBT.h仅用于补充clean up函数
 * 本文件效果不好，播放卡顿，建议用a2dp_vendor_ldacbt_decoder.c取代本文件（CMakeLists.txt中只可启用一个）
 */

#include "common/bt_trace.h"
#include "stack/a2dp_vendor_ldac.h"
#include "stack/a2dp_vendor_ldac_constants.h"
#include "stack/a2dp_vendor_ldac_decoder.h"
#include "ldacdec.h" // 由hegdi与Thealexbarney开发的ldac解码器
#include "ldacBT.h" // O2C14开发的ldac解码器


#if (defined(LDAC_DEC_INCLUDED) && LDAC_DEC_INCLUDED == TRUE)

typedef struct {
    ldacdec_t decoder; // hegdi与Thealexbarney的解码器的结构体

    HANDLE_LDAC_BT ldac_handle; // O2C14的ldac解码器的句柄
    bool has_ldac_handle;
    LDACBT_SMPL_FMT_T pcm_fmt; // O2C14的ldac解码器的pcm格式

    decoded_data_callback_t decode_callback; // 解码回调函数
} tA2DP_LDAC_DECODER_CB;

static tA2DP_LDAC_DECODER_CB a2dp_ldac_decoder_cb;


bool a2dp_ldac_decoder_init(decoded_data_callback_t decode_callback) {
    int res;

    res = ldacdecInit(&a2dp_ldac_decoder_cb.decoder);
    if (res) {
        APPL_TRACE_ERROR("%s: decoder init failed %d", __func__, res);
        return false;
    }
    a2dp_ldac_decoder_cb.decode_callback = decode_callback;
    return true;
}

void a2dp_ldac_decoder_cleanup() {
    if (a2dp_ldac_decoder_cb.has_ldac_handle) {
        HANDLE_LDAC_BT hndl = a2dp_ldac_decoder_cb.ldac_handle;
        ldacBT_close_handle(hndl);
        ldacBT_free_handle(hndl);
    }

    memset(&a2dp_ldac_decoder_cb, 0, sizeof(a2dp_ldac_decoder_cb));
    a2dp_ldac_decoder_cb.has_ldac_handle = false;
}

ssize_t a2dp_ldac_decoder_decode_packet_header(BT_HDR* p_buf) {
    size_t header_len = sizeof(struct media_packet_header) +
                        A2DP_LDAC_MPL_HDR_LEN;
    p_buf->offset += header_len;
    p_buf->len -= header_len;
    return 0;
}

static bool find_sync_word(unsigned char** buf, int *size) {
    while((*size) > 0 && **buf != 0xAA) {
        (*buf)++;
        (*size)--;
    }

    return (*size) > 0;
}


bool a2dp_ldac_decoder_decode_packet(BT_HDR* p_buf, unsigned char* buf, size_t buf_len) {
    ldacdec_t* decoder = &a2dp_ldac_decoder_cb.decoder;
    int ret;

    unsigned char* src = ((unsigned char *)(p_buf + 1) + p_buf->offset);
    int src_size = p_buf->len;
    unsigned char* dst = buf;
    int32_t avail = buf_len;

    while (src_size > 0 && avail > 0) {
        int bytes_used;
        int out_size;

        if (!find_sync_word(&src, &src_size)) {
            break;
        }

        ret = ldacDecode(decoder, src, (void*)dst, &bytes_used);
        if (ret != 0) {
            APPL_TRACE_ERROR("%s: decoder error %d", __func__, ret);
            return false;
        }
        src += bytes_used;
        src_size -= bytes_used;

        frame_t *frame = &decoder->frame;
        out_size = frame->frameSamples * frame->channelCount * sizeof(int16_t);
        dst += out_size;
        avail -= out_size;
    }

    if (src_size > 0 && avail <= 0) {
        LOG_ERROR("%s: Insufficient output buffer size. %d bytes remain.", __func__, src_size);
    }

    size_t len = buf_len - avail;
    len = len <= buf_len ? len : buf_len;
    a2dp_ldac_decoder_cb.decode_callback((uint8_t*)buf, len);
    return true;    
}

void a2dp_ldac_decoder_configure(const uint8_t* p_codec_info) {
    tA2DP_LDAC_CIE cie;
    tA2D_STATUS status;
    status = A2DP_ParseInfoLdac(&cie, (uint8_t*)p_codec_info, false);
    if (status != A2D_SUCCESS) {
        LOG_ERROR("%s: failed parsing codec info. %d", __func__, status);
        return;
    }

    uint32_t sr = 0;
    if (cie.sampleRate == A2DP_LDAC_SAMPLING_FREQ_44100) {
        sr = 44100;
    } else if (cie.sampleRate == A2DP_LDAC_SAMPLING_FREQ_48000) {
        sr = 48000;
    } else if (cie.sampleRate == A2DP_LDAC_SAMPLING_FREQ_88200) {
        sr = 88200;
    } else if (cie.sampleRate == A2DP_LDAC_SAMPLING_FREQ_96000) {
        sr = 96000;
    } else if (cie.sampleRate == A2DP_LDAC_SAMPLING_FREQ_176400) {
        sr = 176400;
    } else if (cie.sampleRate == A2DP_LDAC_SAMPLING_FREQ_192000) {
        sr = 192000;
    }
    LOG_INFO("%s: LDAC Sampling frequency = %lu", __func__, sr);

    if (cie.channelMode == A2DP_LDAC_CHANNEL_MODE_MONO) {
        LOG_INFO("%s: LDAC Channel mode: Mono", __func__);
    } else if (cie.channelMode == A2DP_LDAC_CHANNEL_MODE_DUAL) {
        LOG_INFO("%s: LDAC Channel mode: Dual", __func__);
    } else if (cie.channelMode == A2DP_LDAC_CHANNEL_MODE_STEREO) {
        LOG_INFO("%s: LDAC Channel mode: Stereo", __func__);
    }
}

#endif /* defined(LDAC_DEC_INCLUDED) && LDAC_DEC_INCLUDED == TRUE) */
