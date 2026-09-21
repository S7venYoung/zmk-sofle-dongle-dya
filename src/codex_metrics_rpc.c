#include <pb_decode.h>
#include <pb_encode.h>

#include <zephyr/logging/log.h>

#include <zmk/codex_metrics.h>
#include <zmk/studio/custom.h>

#include <s7venyoung/codex_metrics/codex_metrics.pb.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static bool codex_metrics_handle_request(const zmk_custom_CallRequest *raw_request,
                                         pb_callback_t *encode_response);

static struct zmk_rpc_custom_subsystem_meta codex_metrics_meta = {
    ZMK_RPC_CUSTOM_SUBSYSTEM_UI_URLS(),
    .security = ZMK_STUDIO_RPC_HANDLER_UNSECURED,
};

/* This identifier is intentionally the one used by ProspectorCodexMacOS. */
ZMK_RPC_CUSTOM_SUBSYSTEM(s7venyoung__codex_metrics, &codex_metrics_meta,
                         codex_metrics_handle_request);
ZMK_RPC_CUSTOM_SUBSYSTEM_RESPONSE_BUFFER(s7venyoung__codex_metrics,
                                         s7venyoung_codex_metrics_Response);

static bool codex_metrics_handle_request(const zmk_custom_CallRequest *raw_request,
                                         pb_callback_t *encode_response) {
    s7venyoung_codex_metrics_Response *response =
        ZMK_RPC_CUSTOM_SUBSYSTEM_RESPONSE_BUFFER_ALLOCATE(s7venyoung__codex_metrics,
                                                           encode_response);
    s7venyoung_codex_metrics_Request request = s7venyoung_codex_metrics_Request_init_zero;
    pb_istream_t stream =
        pb_istream_from_buffer(raw_request->payload.bytes, raw_request->payload.size);

    if (!pb_decode(&stream, s7venyoung_codex_metrics_Request_fields, &request) ||
        !request.has_update) {
        return false;
    }

    zmk_codex_metrics_update(request.update.five_hour_used, request.update.total_tokens,
                             request.update.updated_at);
    *response = (s7venyoung_codex_metrics_Response)s7venyoung_codex_metrics_Response_init_zero;
    response->accepted = true;
    return true;
}
