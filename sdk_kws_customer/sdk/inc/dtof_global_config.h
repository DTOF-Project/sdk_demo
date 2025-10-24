#ifndef _DTOF_GLOBAL_CONFIG_H_
#define _DTOF_GLOBAL_CONFIG_H_

#define  DTOF_L3 1
// #define  DTOF_A05 1

#ifdef DTOF_L3
#undef DTOF_A05
#endif

// 芯片型号配置
#ifdef DTOF_L3
    #define DTOF_CHIP_ID             0x4120
    #define DTOF_VCCIO_VSEL_AUTO    0x00
    #define DTOF_VCCIO_VSEL_1_2V    0x01
    #define DTOF_VCCIO_VSEL_1_8V    0x02
    #define DTOF_VCCIO_VSEL_3_3V    0x03
#elif defined(DTOF_A05)
    #define DTOF_CHIP_ID             0x0001
    #define DTOF_VCCIO_VSEL_AUTO    0x04
    #define DTOF_VCCIO_VSEL_1_2V    0x05
    #define DTOF_VCCIO_VSEL_1_8V    0x06
    #define DTOF_VCCIO_VSEL_3_3V    0x07
#endif


#endif