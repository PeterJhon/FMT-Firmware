/******************************************************************************
 * Copyright 2020-2024 The Firmament Authors. All Rights Reserved.
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

#include "drv_canfd.h"

#include "canfd_config.h"
#include "hal/can/can.h"

enum {
#ifdef BSP_USING_CANFD0
    CANFD0_INDEX,
#endif
#ifdef BSP_USING_CANFD1
    CANFD1_INDEX,
#endif
};

static struct ifx_canfd_config can_config[] = {
#ifdef BSP_USING_CANFD0
    CANFD0_CONFIG,
#endif
#ifdef BSP_USING_CANFD1
    CANFD1_CONFIG,
#endif
};

static struct ifx_canfd can_obj[sizeof(can_config) / sizeof(can_config[0])] = {
    0
};

#define IFX_CANFD_MAX_DATA_LEN (64U)

#define IFX_CANFD_ERR_MASK \
    (CY_CANFD_RX_FIFO_0_WATERMARK_REACHED | CY_CANFD_RX_FIFO_0_FULL | CY_CANFD_RX_FIFO_0_MSG_LOST | CY_CANFD_RX_FIFO_1_WATERMARK_REACHED | CY_CANFD_RX_FIFO_1_FULL | CY_CANFD_RX_FIFO_1_MSG_LOST | CY_CANFD_TX_FIFO_1_WATERMARK_REACHED | CY_CANFD_TX_FIFO_1_FULL | CY_CANFD_TX_FIFO_1_MSG_LOST | CY_CANFD_TIMESTAMP_WRAPAROUND | CY_CANFD_MRAM_ACCESS_FAILURE | CY_CANFD_TIMEOUT_OCCURRED | CY_CANFD_BIT_ERROR_CORRECTED | CY_CANFD_BIT_ERROR_UNCORRECTED | CY_CANFD_ERROR_LOG_OVERFLOW | CY_CANFD_ERROR_PASSIVE | CY_CANFD_WARNING_STATUS | CY_CANFD_BUS_OFF_STATUS | CY_CANFD_WATCHDOG_INTERRUPT | CY_CANFD_PROTOCOL_ERROR_ARB_PHASE | CY_CANFD_PROTOCOL_ERROR_DATA_PHASE | CY_CANFD_ACCESS_RESERVED_ADDR)

static uint8_t ifx_canfd_dlc_to_len(uint8_t dlc)
{
    switch (dlc) {
    case 0:
        return 0;
    case 1:
        return 1;
    case 2:
        return 2;
    case 3:
        return 3;
    case 4:
        return 4;
    case 5:
        return 5;
    case 6:
        return 6;
    case 7:
        return 7;
    case 8:
        return 8;
    case 9:
        return 12;
    case 10:
        return 16;
    case 11:
        return 20;
    case 12:
        return 24;
    case 13:
        return 32;
    case 14:
        return 48;
    case 15:
        return 64;
    default:
        return 0;
    }
}

static uint8_t ifx_canfd_len_to_dlc(uint8_t len)
{
    if (len <= 8) {
        return len;
    }
    if (len <= 12) {
        return 9;
    }
    if (len <= 16) {
        return 10;
    }
    if (len <= 20) {
        return 11;
    }
    if (len <= 24) {
        return 12;
    }
    if (len <= 32) {
        return 13;
    }
    if (len <= 48) {
        return 14;
    }
    return 15;
}

static void ifx_canfd_irq_handler(struct ifx_canfd* can)
{
    Cy_CANFD_IrqHandler(can->config->base, can->config->channel, &can->context);
}

static void ifx_canfd_tx_callback(struct ifx_canfd* can)
{
    hal_can_notify(&can->can_dev, CAN_EVENT_TX_DONE, RT_NULL);
}

static void ifx_canfd_rx_callback(struct ifx_canfd* can, bool rxFIFOMsg,
                                  uint8_t msgBufOrRxFIFONum,
                                  cy_stc_canfd_rx_buffer_t* rxBuffer)
{
    can_msg msg;

    if (rxBuffer->r0_f->xtd == CY_CANFD_XTD_EXTENDED_ID) {
        msg.ext_id = rxBuffer->r0_f->id;
        msg.std_id = 0;
        msg.id_type = CAN_ID_EXTENDED;
    } else {
        msg.std_id = rxBuffer->r0_f->id;
        msg.ext_id = 0;
        msg.id_type = CAN_ID_STANDARD;
    }
    msg.frame_type = (rxBuffer->r0_f->rtr == CY_CANFD_RTR_REMOTE_FRAME)
        ? CAN_FRAME_REMOTE
        : CAN_FRAME_DATA;

    uint8_t data_len = ifx_canfd_dlc_to_len((uint8_t)rxBuffer->r1_f->dlc);
    msg.data_len = (data_len > 8) ? 8 : data_len;
    memcpy(msg.data, (uint8_t*)rxBuffer->data_area_f, msg.data_len);

    hal_can_notify(&can->can_dev, CAN_EVENT_RX_IND, &msg);
}

static void ifx_canfd_error_callback(struct ifx_canfd* can,
                                     uint32_t errorMask)
{
    Cy_CANFD_ClearInterrupt(can->config->base, can->config->channel, errorMask & IFX_CANFD_ERR_MASK);
}

#ifdef BSP_USING_CANFD0
void canfd0_isr_callback(void)
{
    rt_interrupt_enter();
    ifx_canfd_irq_handler(&can_obj[CANFD0_INDEX]);
    rt_interrupt_leave();
}

static void canfd0_tx_callback(void)
{
    ifx_canfd_tx_callback(&can_obj[CANFD0_INDEX]);
}

void canfd0_rx_callback(bool rxFIFOMsg, uint8_t msgBufOrRxFIFONum,
                        cy_stc_canfd_rx_buffer_t* rxBuffer)
{
    ifx_canfd_rx_callback(&can_obj[CANFD0_INDEX], rxFIFOMsg, msgBufOrRxFIFONum, rxBuffer);
}

static void canfd0_error_callback(uint32_t errorMask)
{
    ifx_canfd_error_callback(&can_obj[CANFD0_INDEX], errorMask);
}
#endif

#ifdef BSP_USING_CANFD1
void canfd1_isr_callback(void)
{
    rt_interrupt_enter();
    ifx_canfd_irq_handler(&can_obj[CANFD1_INDEX]);
    rt_interrupt_leave();
}

static void canfd1_tx_callback(void)
{
    ifx_canfd_tx_callback(&can_obj[CANFD1_INDEX]);
}

void canfd1_rx_callback(bool rxFIFOMsg, uint8_t msgBufOrRxFIFONum,
                        cy_stc_canfd_rx_buffer_t* rxBuffer)
{
    ifx_canfd_rx_callback(&can_obj[CANFD1_INDEX], rxFIFOMsg, msgBufOrRxFIFONum, rxBuffer);
}

static void canfd1_error_callback(uint32_t errorMask)
{
    ifx_canfd_error_callback(&can_obj[CANFD1_INDEX], errorMask);
}
#endif

static void ifx_canfd_set_interrupt_mask(struct ifx_canfd* can)
{
    Cy_CANFD_SetInterruptMask(can->config->base, can->config->channel, can->irq_mask);
    Cy_CANFD_SetInterruptLine(can->config->base, can->config->channel, CY_CANFD_INTERRUPT_LINE_0_EN);
    Cy_CANFD_EnableInterruptLine(can->config->base, can->config->channel, CY_CANFD_INTERRUPT_LINE_0_EN);
}

static rt_err_t ifx_canfd_configure(can_dev_t can_dev,
                                    struct can_configure* cfg)
{
    cy_en_canfd_status_t status;
    struct ifx_canfd* can;

    RT_ASSERT(can_dev != RT_NULL);
    RT_ASSERT(cfg != RT_NULL);

    can = rt_container_of(can_dev, struct ifx_canfd, can_dev);
    RT_ASSERT(can != RT_NULL);
    RT_ASSERT(can->config != RT_NULL);
    RT_ASSERT(can->config->canfd_config != RT_NULL);

    can->user_tx_cb = can->config->canfd_config->txCallback;
    can->user_rx_cb = can->config->canfd_config->rxCallback;
    can->user_err_cb = can->config->canfd_config->errorCallback;

    Cy_CANFD_EnableMRAM(can->config->base, can->config->channel_mask, can->config->mram_delay_us);

    status = Cy_CANFD_Init(can->config->base, can->config->channel, can->config->canfd_config, &can->context);
    if (status != CY_CANFD_SUCCESS) {
        return -RT_ERROR;
    }

    can->context.canFDInterruptHandling.canFDTxInterruptFunction = can->tx_cb;
    can->context.canFDInterruptHandling.canFDRxInterruptFunction = can->rx_cb;
    can->context.canFDInterruptHandling.canFDErrorInterruptFunction = can->err_cb;

    Cy_CANFD_ConfigChangesEnable(can->config->base, can->config->channel);
    Cy_CANFD_TestModeConfig(can->config->base, can->config->channel, can->config->test_mode);
    Cy_CANFD_ConfigChangesDisable(can->config->base, can->config->channel);

    can->tx_buffer.t0_f = &can->tx_t0;
    can->tx_buffer.t1_f = &can->tx_t1;
    can->tx_buffer.data_area_f = can->tx_data;

    return RT_EOK;
}

static rt_err_t ifx_canfd_control(can_dev_t can_dev, int cmd, void* arg)
{
    struct ifx_canfd* can;

    RT_ASSERT(can_dev != RT_NULL);
    can = rt_container_of(can_dev, struct ifx_canfd, can_dev);
    RT_ASSERT(can != RT_NULL);
    RT_ASSERT(can->config != RT_NULL);

    switch (cmd) {
    case CAN_CLOSE_DEVICE: {
        can->irq_mask &= ~(CY_CANFD_RX_BUFFER_NEW_MESSAGE | CY_CANFD_RX_FIFO_0_NEW_MESSAGE | CY_CANFD_RX_FIFO_1_NEW_MESSAGE | CY_CANFD_TRANSMISSION_COMPLETE | CY_CANFD_TRANSMISSION_CANCEL_FINISHED | IFX_CANFD_ERR_MASK);
        ifx_canfd_set_interrupt_mask(can);
        NVIC_ClearPendingIRQ(can->config->irq);
        break;
    }

    case CAN_OPEN_DEVICE: {
        can->irq_mask |= (CY_CANFD_RX_BUFFER_NEW_MESSAGE | CY_CANFD_RX_FIFO_0_NEW_MESSAGE | CY_CANFD_RX_FIFO_1_NEW_MESSAGE | CY_CANFD_TRANSMISSION_COMPLETE | CY_CANFD_TRANSMISSION_CANCEL_FINISHED | IFX_CANFD_ERR_MASK);
        ifx_canfd_set_interrupt_mask(can);
        if ((can->config->irq_cfg == RT_NULL) || (can->config->isr == RT_NULL)) {
            return -RT_ERROR;
        }

        Cy_SysInt_Init(can->config->irq_cfg, can->config->isr);
        NVIC_EnableIRQ(can->config->irq);

        break;
    }

    case CAN_SET_RX_FILTER: {
        struct can_filter* filter = (struct can_filter*)arg;
        (void)filter;
        return RT_EOK;
    }

    default:
        break;
    }

    return RT_EOK;
}

static int ifx_canfd_sendmsg(can_dev_t can_dev, const can_msg_t msg)
{
    struct ifx_canfd* can = rt_container_of(can_dev, struct ifx_canfd, can_dev);

    uint32_t id;
    if (msg->id_type == CAN_ID_EXTENDED) {
        id = msg->ext_id & 0x1FFFFFFF;
        can->tx_t0.xtd = CY_CANFD_XTD_EXTENDED_ID;
    } else {
        id = msg->std_id & 0x7FF;
        can->tx_t0.xtd = CY_CANFD_XTD_STANDARD_ID;
    }
    can->tx_t0.id = id;
    can->tx_t0.esi = CY_CANFD_ESI_ERROR_ACTIVE;
    can->tx_t1.dlc = ifx_canfd_len_to_dlc(msg->data_len);
    can->tx_t1.fdf = can->config->canfd_config->canFDMode
        ? CY_CANFD_FDF_CAN_FD_FRAME
        : CY_CANFD_FDF_STANDARD_FRAME;

    can->tx_t1.brs = can->config->enable_brs;
    can->tx_t1.efc = false;
    can->tx_t1.mm = 0;

    memset(can->tx_data, 0, sizeof(can->tx_data));
    memcpy(can->tx_data, msg->data, msg->data_len);
    if (Cy_CANFD_UpdateAndTransmitMsgBuffer(
            can->config->base, can->config->channel, &can->tx_buffer, can->config->tx_buffer_index, &can->context)
        != CY_CANFD_SUCCESS) {
        return -RT_ERROR;
    }

    return RT_EOK;
}

static int ifx_canfd_recvmsg(can_dev_t can_dev, can_msg_t msg)
{
    (void)can_dev;
    (void)msg;
    return 0;
}

const static struct can_ops ifx_can_ops = { .configure = ifx_canfd_configure,
                                            .control = ifx_canfd_control,
                                            .sendmsg = ifx_canfd_sendmsg,
                                            .recvmsg = ifx_canfd_recvmsg };

static void ifx_canfd_get_config(void)
{
    struct can_configure config = CAN_DEFAULT_CONFIG;

    for (rt_size_t i = 0; i < (sizeof(can_obj) / sizeof(can_obj[0])); i++) {
        can_obj[i].can_dev.config = config;
    }
}

rt_err_t drv_fdcan_init(void)
{
    rt_err_t result = RT_EOK;
    rt_size_t obj_num = sizeof(can_obj) / sizeof(struct ifx_canfd);

    ifx_canfd_get_config();

    for (rt_size_t i = 0; i < obj_num; i++) {
        can_obj[i].config = &can_config[i];
        can_obj[i].can_dev.ops = &ifx_can_ops;

#ifdef BSP_USING_CANFD0
        if (i == CANFD0_INDEX) {
            can_obj[i].tx_cb = canfd0_tx_callback;
            can_obj[i].rx_cb = canfd0_rx_callback;
            can_obj[i].err_cb = canfd0_error_callback;
            can_obj[i].config->isr = canfd0_isr_callback;
        }
#endif

#ifdef BSP_USING_CANFD1
        if (i == CANFD1_INDEX) {
            can_obj[i].tx_cb = canfd1_tx_callback;
            can_obj[i].rx_cb = canfd1_rx_callback;
            can_obj[i].err_cb = canfd1_error_callback;
            can_obj[i].config->isr = canfd1_isr_callback;
        }
#endif

        result = hal_can_register(&can_obj[i].can_dev, can_obj[i].config->name, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX, RT_NULL);
        RT_ASSERT(result == RT_EOK);
    }

    return result;
}
