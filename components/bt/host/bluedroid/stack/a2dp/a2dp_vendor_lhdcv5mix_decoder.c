/**
 * SPDX-FileCopyrightText: 2025 The Android Open Source Project
 * 
 * SPDX-License-Identifier: Apache-2.0
 * 
 * a2dp_vendor_lhdcv5mix_decoder.c
 * 
 * 本文件依赖由O2C14开发的外部库libldac-dec中的ldacBT.h
 * 本文件的用意是借用外部库libldac-dec来趋近LHDC V5的解码效果
 * 当前代码会报错"BT_LOG: a2dp_lhdcv5_decoder_decode_packet: decode error. result = 6"
 */

#include "common/bt_trace.h"
#include "stack/a2dp_vendor_lhdcv5.h"
#include "stack/a2dp_vendor_lhdc_constants.h"
#include "stack/a2dp_vendor_lhdcv5_constants.h"
#include "stack/a2dp_vendor_lhdcv5_decoder.h"

#include <ldacBT.h> // O2C14开发的ldac解码器


#if (defined(LHDCV5_DEC_INCLUDED) && LHDCV5_DEC_INCLUDED == TRUE)

#define MAX_QUALITY (LDACBT_EQMID_HQ)

typedef struct {
    HANDLE_LDAC_BT ldac_handle;
    bool has_ldac_handle;
    LDACBT_SMPL_FMT_T pcm_fmt;

    decoded_data_callback_t decode_callback;
} tA2DP_LHDCV5_DECODER_CB;

static tA2DP_LHDCV5_DECODER_CB a2dp_lhdcv5_decoder_cb;


bool a2dp_lhdcv5_decoder_init(decoded_data_callback_t decode_callback) {

    HANDLE_LDAC_BT hndl = ldacBT_get_handle();
    if (!hndl) {
        APPL_TRACE_ERROR("%s: could not get decoder handle", __func__);
        a2dp_lhdcv5_decoder_cb.has_ldac_handle = false;
        return false;
    }

    a2dp_lhdcv5_decoder_cb.ldac_handle = hndl;
    a2dp_lhdcv5_decoder_cb.has_ldac_handle = true;
    a2dp_lhdcv5_decoder_cb.pcm_fmt = LDACBT_SMPL_FMT_S32;
    a2dp_lhdcv5_decoder_cb.decode_callback = decode_callback;
    return true;
}

void a2dp_lhdcv5_decoder_cleanup() {
    if (a2dp_lhdcv5_decoder_cb.has_ldac_handle) {
        HANDLE_LDAC_BT hndl = a2dp_lhdcv5_decoder_cb.ldac_handle;
        ldacBT_close_handle(hndl);
        ldacBT_free_handle(hndl);
    }

    memset(&a2dp_lhdcv5_decoder_cb, 0, sizeof(a2dp_lhdcv5_decoder_cb));
    a2dp_lhdcv5_decoder_cb.has_ldac_handle = false;
}

ssize_t a2dp_lhdcv5_decoder_decode_packet_header(BT_HDR* p_buf) {
    size_t header_len = sizeof(struct media_packet_header) +
                        A2DP_LHDC_MPL_HDR_LEN;
    p_buf->offset += header_len;
    p_buf->len -= header_len;
    return 0;
}

bool a2dp_lhdcv5_decoder_decode_packet(BT_HDR* p_buf, unsigned char* buf, size_t buf_len) {
    HANDLE_LDAC_BT hndl = a2dp_lhdcv5_decoder_cb.ldac_handle;
    const int32_t max_frame_size = (3 - MAX_QUALITY) * 110;

    if (!a2dp_lhdcv5_decoder_cb.has_ldac_handle) {
        return false;
    }

    uint8_t *src = (uint8_t*)((unsigned char *)(p_buf + 1) + p_buf->offset);
    int32_t in_count = p_buf->len;
    int32_t in_used = 0;
    int32_t used_bytes = 0;

    int32_t out_used = 0;
    int32_t out_count = 0;

    int result = 0;

    while ((in_count - in_used) > 0) {
        result = ldacBT_decode(hndl,
                               src + in_used,
                               buf + out_count,
                               a2dp_lhdcv5_decoder_cb.pcm_fmt,
                               max_frame_size,
                               (int *)&used_bytes,
                               (int *)&out_used);
        in_used += used_bytes;
        out_count += out_used;

        if (result != 0 || used_bytes <= 0 || out_used <= 0) {
            break;
        }

        if (out_count > buf_len) {
            LOG_ERROR("%s: buffer full.", __func__);
            out_count = buf_len;
            break;
        }
    }

    if (result != 0) {
        LOG_ERROR("%s: decode error. result = %d", __func__, result);
        return false;
    }

    a2dp_lhdcv5_decoder_cb.decode_callback(buf, out_count);
    return true;
}

void a2dp_lhdcv5_decoder_start() {
    LOG_INFO("Starting LHDC V5 decoder");
}

void a2dp_lhdcv5_decoder_suspend() {
    LOG_INFO("Suspending LHDC V5 decoder");
}

void a2dp_lhdcv5_decoder_configure(const uint8_t* p_codec_info) {
    tA2DP_LHDCV5_CIE cie;
    tA2D_STATUS status;
    status = A2DP_ParseInfoLhdcV5(&cie, (uint8_t*)p_codec_info, false, IS_SNK);
    if (status != A2D_SUCCESS) {
        LOG_ERROR("%s: failed parsing codec info. %d", __func__, status);
        return;
    }

    uint32_t sf = 0;

    if (cie.sampleRate == A2DP_LHDCV5_SAMPLING_FREQ_44100) {
        sf = 44100;
    } else if (cie.sampleRate == A2DP_LHDCV5_SAMPLING_FREQ_48000) {
        sf = 48000;
    } else if (cie.sampleRate == A2DP_LHDCV5_SAMPLING_FREQ_96000) {
        sf = 96000;
    } else if (cie.sampleRate == A2DP_LHDCV5_SAMPLING_FREQ_192000) {
        sf = 192000;
    }
    LOG_INFO("%s: LHDC V5 Sampling frequency = %lu", __func__, sf);


    int cm = 0;
    if (cie.channelMode == A2DP_LHDCV5_CHANNEL_MODE_STEREO) {
        cm = LDACBT_CHANNEL_MODE_STEREO;
        LOG_INFO("%s: LHDC V5 Channel mode: Stereo", __func__);
    } else if (cie.channelMode == A2DP_LHDCV5_CHANNEL_MODE_DUAL) {
        cm = LDACBT_CHANNEL_MODE_DUAL_CHANNEL;
        LOG_INFO("%s: LHDC V5 Channel mode: Dual", __func__);
    } else if (cie.channelMode == A2DP_LHDCV5_CHANNEL_MODE_MONO) {
        cm = LDACBT_CHANNEL_MODE_MONO;
        // cm = LDACBT_CHANNEL_MODE_DUAL_CHANNEL;
        LOG_INFO("%s: LHDC V5 Channel mode: Mono", __func__);
    }

    int res;
    HANDLE_LDAC_BT hndl = a2dp_lhdcv5_decoder_cb.ldac_handle;
    res = ldacBT_init_handle_decode(hndl, cm, sf, 0, 0, 0);
    if (res < 0) {
        int err = ldacBT_get_error_code(hndl);
        APPL_TRACE_ERROR("%s: decoder init failed res = %d, err = %d", __func__,
                         res, LDACBT_API_ERR(err));

        ldacBT_close_handle(hndl);
        ldacBT_free_handle(hndl);
        a2dp_lhdcv5_decoder_cb.ldac_handle = NULL;
        a2dp_lhdcv5_decoder_cb.has_ldac_handle = false;

        return;
    }
}

#endif /* defined(LHDCV5_DEC_INCLUDED) && LHDCV5_DEC_INCLUDED == TRUE) */
