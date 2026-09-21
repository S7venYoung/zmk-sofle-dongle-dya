#pragma once

#include <stdint.h>

/* Called by the USB Studio RPC endpoint whenever the macOS companion syncs. */
void zmk_codex_metrics_update(uint8_t five_hour_used, int16_t week_used, uint32_t total_tokens,
                              uint32_t reset_in_minutes, uint32_t updated_at);
