/******************************************************************************
 * Copyright 2020 The Firmament Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/
#include <firmament.h>

#include "hal/can/can.h"

static rt_thread_t can_thread = RT_NULL;

void can_thread_entry(void* parameter)
{
    printf("Hello FMT! This is CAN task for E83 M55.\n");

    rt_device_t can_dev = rt_device_find("canfd0");
    if (!can_dev) {
        printf("canfd0 not found!\n");
        return;
    }

    if (rt_device_open(can_dev, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX) != RT_EOK) {
        printf("Failed to open canfd0!\n");
        return;
    }

    can_msg msg;

    while (1) {
        while (rt_device_read(can_dev, RT_WAITING_FOREVER, &msg, 1)) {
            printf("can recv msg, id:0x%x, data:%x,%x,%x,%x,%x,%x,%x,%x\n",
                   msg.std_id,
                   msg.data[0],
                   msg.data[1],
                   msg.data[2],
                   msg.data[3],
                   msg.data[4],
                   msg.data[5],
                   msg.data[6],
                   msg.data[7]);

            msg.std_id = 0x200;
            msg.id_type = CAN_ID_STANDARD;
            msg.frame_type = CAN_FRAME_DATA;
            msg.data_len = 8;
            msg.data[0] = 0x88;
            msg.data[1] = 0x77;
            msg.data[2] = 0x66;
            msg.data[3] = 0x55;
            msg.data[4] = 0x44;
            msg.data[5] = 0xFF;
            msg.data[6] = 0xEE;
            msg.data[7] = 0xDD;
            rt_device_write(can_dev, 5000, &msg, 1);
        }
    }
}

static int can_start(void)
{
    if (can_thread != RT_NULL) {
        printf("CAN thread already running!\n");
        return -1;
    }

    can_thread = rt_thread_create("can_test",
                                  can_thread_entry,
                                  RT_NULL,
                                  4096,
                                  25,
                                  20);
    if (can_thread != RT_NULL) {
        rt_thread_startup(can_thread);
        printf("CAN thread started!\n");
        return 0;
    } else {
        printf("Failed to create CAN thread!\n");
        return -1;
    }
}

MSH_CMD_EXPORT(can_start, Start CAN test thread);

static int can_stop(void)
{
    if (can_thread == RT_NULL) {
        printf("CAN thread not running!\n");
        return -1;
    }

    rt_thread_delete(can_thread);
    can_thread = RT_NULL;
    printf("CAN thread stopped!\n");
    return 0;
}

MSH_CMD_EXPORT(can_stop, Stop CAN test thread);
