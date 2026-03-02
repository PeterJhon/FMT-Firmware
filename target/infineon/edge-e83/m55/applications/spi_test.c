#include "hal/pin/pin.h"
#include "hal/spi/spi.h"
#include <firmament.h>

#define GET_PIN(PORTx, PIN) ((((uint8_t)(PORTx)) << 3U) + ((uint8_t)(PIN)))
char rx_buf[4] = { 0 };
static struct rt_spi_device spi1_dev;

void drv_spi_device_attach(void)
{
    rt_spi_bus_attach_device(&spi1_dev, "spi1_dev", "spi1", (void*)GET_PIN(16, 3));
}

static void spi_test_sample(void)
{
    struct rt_spi_device *spi_dev;

    uint8_t tx_buf[16] = {
        0xAA, 0x55, 0x00, 0xFF,
        0x11, 0x22, 0x33, 0x44,
        0x55, 0x66, 0x77, 0x88,
        0x99, 0xAA, 0xBB, 0xCC
    };

    uint8_t rx_buf[16] = {0};

    drv_spi_device_attach();

    spi_dev = (struct rt_spi_device *)rt_device_find("spi1_dev");
    if (!spi_dev)
    {
        rt_kprintf("spi device not found!\n");
        return;
    }

    struct rt_spi_configuration spi_cfg = {
        .data_width = 8,
        .max_hz = 1000000,                
        .mode = RT_SPI_MODE_3 | RT_SPI_MSB
    };

    rt_spi_configure(spi_dev, &spi_cfg);

    rt_spi_transfer(spi_dev, tx_buf, rx_buf, sizeof(tx_buf));

    rt_kprintf("TX: ");
    for (int i = 0; i < sizeof(tx_buf); i++)
        rt_kprintf("%02X ", tx_buf[i]);

    rt_kprintf("\nRX: ");
    for (int i = 0; i < sizeof(rx_buf); i++)
        rt_kprintf("%02X ", rx_buf[i]);

    rt_kprintf("\n");
}

MSH_CMD_EXPORT(spi_test_sample, SPI send abc and receive test);


// #include "hal/pin/pin.h"
// #include "hal/spi/spi.h"
// #include <firmament.h>

// #define GET_PIN(PORTx, PIN) ((((uint8_t)(PORTx)) << 3U) + ((uint8_t)(PIN)))
// char rx_buf[4] = { 0 };
// static struct rt_spi_device spi1_dev;

// void drv_spi_device_attach(void)
// {
//     rt_spi_bus_attach_device(&spi1_dev, "spi1_dev", "spi1", (void*)GET_PIN(16, 3));
// }

// extern rt_err_t spi_write_reg8(rt_device_t spi_device, uint8_t reg, uint8_t val);

// static void spi_test_sample(void)
// {
//     struct rt_spi_device* spi_dev;
//     drv_spi_device_attach();
//     spi_dev = (struct rt_spi_device*)rt_device_find("spi1_dev");
//     if (!spi_dev) {
//         rt_kprintf("spi device not found!\n");
//         return;
//     }

//     struct rt_spi_configuration spi_cfg;
//     spi_cfg.data_width = 8;
//     spi_cfg.max_hz = 1000000; // 1 MHz
//     spi_cfg.mode = RT_SPI_MODE_3 | RT_SPI_MSB ;
//     rt_spi_configure(spi_dev, &spi_cfg);
//     // rt_spi_transfer(spi_dev, tx_buf, rx_buf, 3);
//     // rt_kprintf("send: %s, recv: %s\n", tx_buf, rx_buf);


//     uint8_t send_buffer[2];
//     uint8_t recv_buffer[2];

//     uint8_t send_buffer1[4];
//     send_buffer1[0]=0x76;
//     send_buffer1[1]=0x00;
//     send_buffer1[2]=0x11;
//     send_buffer1[3]=0x01;

//     send_buffer[0] = 0xF5;
//     send_buffer[1] = 0xFF;

//     // spi_write_reg8(spi_dev, 0x76, 0x00);
//     // spi_write_reg8(spi_dev, 0x11, 0x01);

//     rt_spi_send_then_send(spi_dev,&send_buffer1[0],1,&send_buffer1[1],1);
//     rt_spi_send_then_send(spi_dev,&send_buffer1[2],1,&send_buffer1[3],1);

//     rt_thread_mdelay(500);
    
//     // rt_spi_transfer(spi_dev, &send_buffer[0], recv_buffer, 1);
//     // rt_spi_transfer(spi_dev, &send_buffer[1], recv_buffer, 1);

//     // rt_spi_transfer(spi_dev, send_buffer, recv_buffer, 2);

//     rt_spi_send_then_recv(spi_dev,send_buffer,1,recv_buffer,1);
//     rt_kprintf("recv_buffer[0]:%#x\n", recv_buffer[0]);
    
//     // rt_kprintf("recv_buffer[1]:%#x\n", recv_buffer[1]);
// }

// MSH_CMD_EXPORT(spi_test_sample, SPI send abc and receive test);
