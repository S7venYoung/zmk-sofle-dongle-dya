/* SPDX-License-Identifier: MIT */
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/logging/log.h>
#include <zmk/hid.h>
#include "fight_status.h"

LOG_MODULE_REGISTER(fight_telemetry, CONFIG_ZMK_LOG_LEVEL);
static struct bt_le_ext_adv *advertiser;
static struct k_work_delayable work;
static uint8_t frame[17] = {0xff,0xff,0xab,0xce,1};
static uint16_t sequence;
static bool started;

static void broadcast(struct k_work *unused) {
    ARG_UNUSED(unused);
    int err;
    if (!advertiser) {
        const struct bt_le_adv_param params = {
            .id = BT_ID_DEFAULT,
            .options = BT_LE_ADV_OPT_USE_IDENTITY | BT_LE_ADV_OPT_SCANNABLE,
            .interval_min = 0x00a0, .interval_max = 0x00b0,
        };
        err = bt_le_ext_adv_create(&params, NULL, &advertiser);
        if (err) goto retry;
        bt_addr_le_t ids[CONFIG_BT_ID_MAX];
        size_t count = ARRAY_SIZE(ids);
        bt_id_get(ids, &count);
        if (count) {
            for (int i=0;i<6;i++) frame[6+(i%4)] ^= ids[0].a.val[i];
        }
    }
    uint8_t wpm[2], battery[2], valid;
    fight_telemetry_read(wpm, battery, &valid);
    frame[5]=valid;
    frame[10]=wpm[0]; frame[11]=wpm[1];
    frame[12]=battery[0]; frame[13]=battery[1];
    struct zmk_hid_keyboard_report *report=zmk_hid_get_keyboard_report();
    frame[14]=report ? report->body.modifiers : 0;
    frame[15]=(uint8_t)sequence; frame[16]=(uint8_t)(sequence++>>8);
    const struct bt_data data[]={BT_DATA(BT_DATA_MANUFACTURER_DATA,frame,sizeof(frame))};
    const struct bt_data response[]={BT_DATA(BT_DATA_NAME_COMPLETE,"CUBE-FIGHT",10)};
    err=bt_le_ext_adv_set_data(advertiser,data,ARRAY_SIZE(data),response,ARRAY_SIZE(response));
    if (!err && !started) {
        const struct bt_le_ext_adv_start_param start={0};
        err=bt_le_ext_adv_start(advertiser,&start);
        if (!err) started=true;
    }
retry:
    if (err) LOG_WRN("Fight broadcast retry: %d",err);
    k_work_reschedule(&work,K_MSEC(err ? 1000 : 200));
}

static int init(void) {
    k_work_init_delayable(&work,broadcast);
    // Bluetooth initializes asynchronously in ZMK; errors retry safely.
    k_work_schedule(&work,K_SECONDS(3));
    return 0;
}
SYS_INIT(init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
