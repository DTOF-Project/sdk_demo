/**
 *
 */
#ifndef _DTOF_REG_H_
#define _DTOF_REG_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

// #define dtof_uint16_t uint16_t
#define DTOF_REG0 0x0
    typedef struct dtof_addressREG0
    {
        dtof_uint16_t chipID : 16;
    } dtof_addressREG0_t;

#define DTOF_REG1 0x1
    typedef struct dtof_addressREG1
    {
        dtof_uint16_t chip_addr : 15;
        dtof_uint16_t regOrOtpCA : 1;
    } dtof_addressREG1_t;

#define DTOF_REG2 0x2
    typedef struct dtof_addressREG2
    {
        dtof_uint16_t wakeup : 1;
        dtof_uint16_t res : 15;
    } dtof_addressREG2_t;

// 状态机相关寄存器，
// 1.sleep 需要一上来就关闭（会导致配置失败）
// 2. wakeup 是低功耗的情况调用？ 是不是任何时候都可以访问？ 还是在bypassMCU==1 的时候才可以访问？
// （wakeup 只有外部mcu 可以操作 在 从 auto 模式唤醒 进入fdlay的状态）
#define DTOF_REG3 0x3
    typedef struct dtof_addressREG3
    {
#define RUN_MODE_FREERUN 0
#define RUN_MODE_RUNONCE 1
#define RUN_MODE_RUN_DELAY 2
#define RUN_MODE_HOLD 3
        dtof_uint16_t runMod : 3;
        dtof_uint16_t cfgDoneR : 1;
        dtof_uint16_t sleep : 1;
        dtof_uint16_t ignrLock : 1;
        dtof_uint16_t rmRoiDc : 1;
        dtof_uint16_t extTrgMod : 1;
#define RNG_TIME_2 2
#define RNG_TIME_3 3
        dtof_uint16_t rngTime : 6;
        dtof_uint16_t rmvNeXt : 1;
        dtof_uint16_t bstNeXt : 1;
    } dtof_addressREG3_t;

#define DTOF_REG4 0x4
    typedef struct dtof_addressREG4
    {
        dtof_uint16_t pcRST : 16;
    } dtof_addressREG4_t;

#define DTOF_REG5 0x5
#define RESET_KEY_FROM_OUTTER 0X27EA
#define BYPASS_KEY_FROM_INNER 0X17B9
    // 这里是inner MCU 和outter MCU
    // 交互的寄存器
    // 外部mcu 读最高位的时候 如果bit 15 是1 的时候 bypass ==1
    // 否则的话 就是 bypass ==0
    typedef struct dtof_addressREG5
    {
        dtof_uint16_t mcuDin_value : 8;
        dtof_uint16_t mcuDin_cmd : 6;
        dtof_uint16_t intMCU : 2;
    } dtof_addressREG5_t;

#define CMD_3_USE_REG_RESET_AND_ENCKSM 0X401
#define DTOF_REG6 0x6
    // otpBpmEn--bypass 的来源有两个 一个寄存器一个OTP，这里选择两个的 优先级（）
    typedef struct dtof_addressREG6
    {
        dtof_uint16_t otpBpmEn : 1;
        dtof_uint16_t mCPOL : 1;
        dtof_uint16_t mCPHA : 1;
        dtof_uint16_t clkDiv_SPIM : 3;
        dtof_uint16_t spimEn : 1;
        dtof_uint16_t enBE : 1;
        dtof_uint16_t pcRSTen : 1;
#define CJTAGBK_KEEP 0
#define CJTAGBK_NO_KEEP 1
        dtof_uint16_t cJtagBk : 1;
        dtof_uint16_t enChksm : 1;
        dtof_uint16_t res : 5;
    } dtof_addressREG6_t;

#define DTOF_REG7 0x7
    typedef struct dtof_addressREG7
    {
        dtof_uint16_t chksmRst : 16;
    } dtof_addressREG7_t;

#define DTOF_REG79 0x4f
    // 状态机相关 清中断
    typedef struct dtof_addressREG79
    {
        dtof_uint16_t sspVldH : 10;
        dtof_uint16_t res : 6;
    } dtof_addressREG79_t;

#define DTOF_REG80 0x50
    // 1 内外数据交互的地方，外部MCU 可以实时获取的
    // 2.通过读写内容保证数据的有效性
    // 3. 内部MCU通过31fifo的高位的写1 代表这plate 切换给外部MCU
    typedef struct dtof_addressREG80
    {
        dtof_uint16_t FIFO0 : 16;
    } dtof_addressREG80_t;

#define DTOF_REG81 0x51
    typedef struct dtof_addressREG81
    {
        dtof_uint16_t FIFO1 : 16;
    } dtof_addressREG81_t;

#define DTOF_REG82 0x52
    typedef struct dtof_addressREG82
    {
        dtof_uint16_t FIFO2 : 16;
    } dtof_addressREG82_t;

#define DTOF_REG83 0x53
    typedef struct dtof_addressREG83
    {
        dtof_uint16_t FIFO3 : 16;
    } dtof_addressREG83_t;

#define DTOF_REG84 0x54
    typedef struct dtof_addressREG84
    {
        dtof_uint16_t FIFO4 : 16;
    } dtof_addressREG84_t;

#define DTOF_REG85 0x55
    typedef struct dtof_addressREG85
    {
        dtof_uint16_t FIFO5 : 16;
    } dtof_addressREG85_t;

#define DTOF_REG86 0x56
    typedef struct dtof_addressREG86
    {
        dtof_uint16_t FIFO6 : 16;
    } dtof_addressREG86_t;

#define DTOF_REG87 0x57
    typedef struct dtof_addressREG87
    {
        dtof_uint16_t FIFO7 : 16;
    } dtof_addressREG87_t;

#define DTOF_REG88 0x58
    typedef struct dtof_addressREG88
    {
        dtof_uint16_t FIFO8 : 16;
    } dtof_addressREG88_t;

#define DTOF_REG89 0x59
    typedef struct dtof_addressREG89
    {
        dtof_uint16_t FIFO9 : 16;
    } dtof_addressREG89_t;

#define DTOF_REG90 0x5a
    typedef struct dtof_addressREG90
    {
        dtof_uint16_t FIFO10 : 16;
    } dtof_addressREG90_t;

#define DTOF_REG91 0x5b
    typedef struct dtof_addressREG91
    {
        dtof_uint16_t FIFO11 : 16;
    } dtof_addressREG91_t;

#define DTOF_REG92 0x5c
    typedef struct dtof_addressREG92
    {
        dtof_uint16_t FIFO12 : 16;
    } dtof_addressREG92_t;

#define DTOF_REG93 0x5d
    typedef struct dtof_addressREG93
    {
        dtof_uint16_t FIFO13 : 16;
    } dtof_addressREG93_t;

#define DTOF_REG94 0x5e
    typedef struct dtof_addressREG94
    {
        dtof_uint16_t FIFO14 : 16;
    } dtof_addressREG94_t;

#define DTOF_REG95 0x5f
    typedef struct dtof_addressREG95
    {
        dtof_uint16_t FIFO15 : 16;
    } dtof_addressREG95_t;

#define DTOF_REG96 0x60
    typedef struct dtof_addressREG96
    {
        dtof_uint16_t FIFO16 : 16;
    } dtof_addressREG96_t;

#define DTOF_REG97 0x61
    typedef struct dtof_addressREG97
    {
        dtof_uint16_t FIFO17 : 16;
    } dtof_addressREG97_t;

#define DTOF_REG98 0x62
    typedef struct dtof_addressREG98
    {
        dtof_uint16_t FIFO18 : 16;
    } dtof_addressREG98_t;

#define DTOF_REG99 0x63
    typedef struct dtof_addressREG99
    {
        dtof_uint16_t FIFO19 : 16;
    } dtof_addressREG99_t;

#define DTOF_REG100 0x64
    typedef struct dtof_addressREG100
    {
        dtof_uint16_t FIFO20 : 16;
    } dtof_addressREG100_t;

#define DTOF_REG101 0x65
    typedef struct dtof_addressREG101
    {
        dtof_uint16_t FIFO21 : 16;
    } dtof_addressREG101_t;

#define DTOF_REG102 0x66
    typedef struct dtof_addressREG102
    {
        dtof_uint16_t FIFO22 : 16;
    } dtof_addressREG102_t;

#define DTOF_REG103 0x67
    typedef struct dtof_addressREG103
    {
        dtof_uint16_t FIFO23 : 16;
    } dtof_addressREG103_t;

#define DTOF_REG104 0x68
    typedef struct dtof_addressREG104
    {
        dtof_uint16_t FIFO24 : 16;
    } dtof_addressREG104_t;

#define DTOF_REG105 0x69
    typedef struct dtof_addressREG105
    {
        dtof_uint16_t FIFO25 : 16;
    } dtof_addressREG105_t;

#define DTOF_REG106 0x6a
    typedef struct dtof_addressREG106
    {
        dtof_uint16_t FIFO26 : 16;
    } dtof_addressREG106_t;

#define DTOF_REG107 0x6b
    typedef struct dtof_addressREG107
    {
        dtof_uint16_t FIFO27 : 16;
    } dtof_addressREG107_t;

#define DTOF_REG108 0x6c
    typedef struct dtof_addressREG108
    {
        dtof_uint16_t FIFO28 : 16;
    } dtof_addressREG108_t;

#define DTOF_REG109 0x6d
    typedef struct dtof_addressREG109
    {
        dtof_uint16_t FIFO29 : 16;
    } dtof_addressREG109_t;

#define DTOF_REG110 0x6e
    typedef struct dtof_addressREG110
    {
        dtof_uint16_t FIFO30 : 16;
    } dtof_addressREG110_t;

#define DTOF_REG111 0x6f
    typedef struct dtof_addressREG111
    {
        dtof_uint16_t FIFO31 : 15; // @gqw check bit flag
        dtof_uint16_t FIFO31_trigger : 1;
    } dtof_addressREG111_t;

#define DTOF_REG112 0x70
    // refMap:
    // bit[0]: 1, 表示ref spad tdc的数据输入到ram1中；
    // bit[0]:  0, 表示不输入；
    // bit[1]: reserved
    // ref spad tdc到 ref histogram的通路开关
    typedef struct dtof_addressREG112
    {
        dtof_uint16_t refMap : 2;
        dtof_uint16_t pcgiTail : 7;
        dtof_uint16_t pcgiHead0 : 7;
    } dtof_addressREG112_t;

#define DTOF_REG113 0x71
    typedef struct dtof_addressREG113
    {
        dtof_uint16_t pcgiHead1 : 7;
        dtof_uint16_t res : 9;
    } dtof_addressREG113_t;

#define DTOF_REG114 0x72
    typedef struct dtof_addressREG114
    {
        dtof_uint16_t pcgiFpok0 : 9;
        dtof_uint16_t res : 7;
    } dtof_addressREG114_t;

#define DTOF_REG115 0x73
    typedef struct dtof_addressREG115
    {
        dtof_uint16_t pcgiFpok1 : 9;
        dtof_uint16_t res : 7;
    } dtof_addressREG115_t;

#define DTOF_REG116 0x74
    //  这个是matc寻峰需要减去这个 GD 后做 最后的value
    // 这里的TP 是卷积的选择,这里和16个卷积kernel相关
    // intHld -- 拆分为intHld 、fsmhold
    // xtClkPin -- 外部clock 接受
    typedef struct dtof_addressREG116
    {
        dtof_uint16_t mfkTp : 6;
        dtof_uint16_t i2cModReg : 1;
        dtof_uint16_t ramAlgEnb : 5;
        // dtof_uint16_t intHld : 2;
        dtof_uint16_t intHld : 1;
        dtof_uint16_t fsmhold : 1;
        dtof_uint16_t xtClkPin : 2;
    } dtof_addressREG116_t;

#define DTOF_REG117 0x75
    // softRst、glbSftRst
    // 1. glb reset 在内置MCU的时候 如果操作了的话 相当于一个hardware重启？
    // 2 如果是bypass ==1  的情况下 的操作会对内置MCU 有什么影响？
    // mpdl、mpdr
    // 寻峰相关的  1. L  峰值窗口左边间隔
    // 2. 峰值窗口右边间隔
    typedef struct dtof_addressREG117
    {
        dtof_uint16_t sgrutEn : 1;
        dtof_uint16_t intClr : 1;
        dtof_uint16_t softRst : 1;
        dtof_uint16_t glbSftRst : 1;
        dtof_uint16_t mpdl : 6;
        dtof_uint16_t mpdr : 6;
    } dtof_addressREG117_t;

#define DTOF_REG118 0x76
#define DTOF_FLASHCOUNT_ADDR DTOF_REG118
    // Number of pulses to emit in each frame. This value is multiplied by 16 in hardware. Like a value of 1 means 16 pulses, a value of 2 means 32 pulses, a value of N means N*16 pulses.
    // 设置为1，可以生效；
    // 设置为0x1000以下的值，需要秘钥
    // 设置为0x1000及以上，不影响，可以生效
    typedef struct dtof_addressREG118
    {
        dtof_uint16_t maxFLs : 16;
    } dtof_addressREG118_t;

#define DTOF_REG119 0x77
#define DTOF_ROOF DTOF_REG119
    // when this threshold is reached, the histogram counts will stop accumulating.
    // 最大值可以设为0xffff. 设置为0也相当于是设置了最大值0xffff.
    // agc模式的histogram的最大值，当有任何bin达到最大值时，相当于触发agc，然后根据agcMod寄存器选择是否结束当前UPDH状态
    typedef struct dtof_addressREG119
    {
        dtof_uint16_t roofHgm : 16;
    } dtof_addressREG119_t;

#define DTOF_REG120 0x78
    // atnMode atn模式
    // autonomous mode S3状态的时间：低功耗的时间。
    // LPOSC clock的频率是10Khz  HPOSC clock还是8Mhz. (这里设置的default值为0x4000, 实际生效的值是0x8000，少了最后一位，所以默认是扩大了2倍)
    typedef struct dtof_addressREG120
    {
        dtof_uint16_t atnMode : 1;
        dtof_uint16_t atnTm0 : 15;
    } dtof_addressREG120_t;

#define DTOF_REG121 0x79
    typedef struct dtof_addressREG121
    {
        dtof_uint16_t atnTm1 : 16;
    } dtof_addressREG121_t;

#define DTOF_REG122 0x7a
    typedef struct dtof_addressREG122
    {
        dtof_uint16_t pwupTm : 16;
    } dtof_addressREG122_t;

#define DTOF_REG123 0x7b
    typedef struct dtof_addressREG123
    {
        dtof_uint16_t resreved0 : 16;
    } dtof_addressREG123_t;

#define DTOF_REG124 0x7c
    typedef struct dtof_addressREG124
    {
#define BYPASS_PCGI 1
        dtof_uint16_t finish_pcgi : 1;
        dtof_uint16_t reserve0 : 2;
        dtof_uint16_t set_clk : 1;
#define OPTION0_INT_MODE_DEFAULT 0
#define OPTION1_INT_MODE_FDLY 1
#define OPTION1_INT_MODE_UPDH 3
#define OPTION1_INT_MODE_PCGI 5
#define OPTION2_INT_MODE_IDLE_TO_FDLY 1
#define OPTION2_INT_MODE_UPDH_TO_IDLE 3
#define OPTION2_INT_MODE_PCGI_TO_IDLE 5
#define OPTION2_INT_MODE_FDLY_TO_UPDH 2
#define OPTION2_INT_MODE_PCGI_TO_FDLY 4
#define OPTION2_INT_MODE_UPDH_TO_PCGI 6
        dtof_uint16_t int_mode : 3;
// default option 是默认配置
#define INT_OPTION0 0
#define INT_OPTION1 1
#define INT_OPTION2 2
#define ENABLE_INT_OPTION 1
#define DISABLE_INT_OPTION 0
        dtof_uint16_t int_option2 : 1;
        dtof_uint16_t int_option1 : 1;
        dtof_uint16_t new_int_mode : 1;
        dtof_uint16_t reserve1 : 6;
    } dtof_addressREG124_t;

#define DTOF_REG125 0x7d
    // dbgRUT -- signal router
    // dbgRAM -- debug ram  （抢ram 的 interface）
    // dbgSSP -- SSP == dsp
    typedef struct dtof_addressREG125
    {
        dtof_uint16_t dbgRUT : 1;
        dtof_uint16_t dbgRAM : 1;
        dtof_uint16_t dbgSSP : 1;
        dtof_uint16_t res : 13;
    } dtof_addressREG125_t;

#define DTOF_REG127 0x7f
    // main fdly；
    //  one frame period = fDly + pulse numbers * pulse period + histogram processsing time.
    //  bit0为0的话，agc触发以后，会自动等完少发的pulse的时间，
    //  保证帧率稳定；bit0为1的话，就不等了。
    typedef struct dtof_addressREG127
    {
#define FDLY0_DEFAULT_VALUE 0x740A
        dtof_uint16_t fDlyLimr0 : 16;
    } dtof_addressREG127_t;

#define FDLY1_ZERO_VALUE 0X0
#define DTOF_REG128 0x80
    typedef struct dtof_addressREG128
    {
        dtof_uint16_t fDlyLimr1 : 16;
    } dtof_addressREG128_t;

#define DTOF_REG129 0x81
    // reserved for debug funcitons:
    // bit[0]:spadEn;
    // bit[1]: updhEn;
    // bit[3:2]: reserved.
    // bit[11:4]: reserved (tdcUlim/tdcLlim);
    // bit[12]: subAccEn;
    // bit[13]: trEn, trend removal;
    // bit[15:14]: reserved;
    typedef struct dtof_addressREG129
    {
        // dtof_uint16_t tDlyLimr0 : 16;
        dtof_uint16_t spadEn : 1;
        dtof_uint16_t updhEn : 1;
        dtof_uint16_t reserved0 : 2;
        dtof_uint16_t reserved1 : 8;
        dtof_uint16_t subAccEn : 1;
        dtof_uint16_t trEn : 1;
        dtof_uint16_t reserved2 : 2;
    } dtof_addressREG129_t;

    // @Fred pls check tDlyLimr1
//  bit[1:0]: clkRat;
// bit[2]: dbgPTC, set to use ramdata for dsp debug directly.
// bit[11:3]: reserved;
// bit[14:12]: rrEnb, roundrobin disable;
// bit[15]: reserved;
#define DTOF_REG130 0x82
    typedef struct dtof_addressREG130
    {
        // dtof_uint16_t tDlyLimr1 : 16;
        dtof_uint16_t clkRat : 2;
        dtof_uint16_t dbgPTC : 1;
        dtof_uint16_t reserved0 : 9;
        dtof_uint16_t rrEnb : 3;
        dtof_uint16_t reserved1 : 1;

    } dtof_addressREG130_t;

#define DTOF_REG131 0x83
    typedef struct dtof_addressREG131
    {
        dtof_uint16_t mxHgmEn : 16;
    } dtof_addressREG131_t;

#define DTOF_REG132 0x84
    // 只有最低位有用，其他位保持默认值，使能main的agc功能。
    // 和channel配置有关，
    typedef struct dtof_addressREG132
    {
        dtof_uint16_t mxHgmEnRef : 1;
        dtof_uint16_t agcMod : 1;
        dtof_uint16_t mfAo : 10;
        dtof_uint16_t fmTDM : 3;
        dtof_uint16_t rsvCyp : 1;
    } dtof_addressREG132_t;

#define DTOF_REG133 0x85
    // rng指bin的位置。默认是C0之前的bin的histogram才参与avg、std的计算。
    // Avg、std是指noise。这里不可以配置小于C0，这里的起始计算是从0 开始？
    typedef struct dtof_addressREG133
    {
        dtof_uint16_t rng0 : 11;
        dtof_uint16_t res : 5;
    } dtof_addressREG133_t;

#define DTOF_REG134 0x86
    // binWd
    // 0：不combine
    // 1:  2个bin combine在一起
    // 2：2^2个bin combine在一起
    typedef struct dtof_addressREG134
    {
        dtof_uint16_t binWd : 11;
        dtof_uint16_t splitRoi : 1;
        dtof_uint16_t res : 4;
    } dtof_addressREG134_t;

#define DTOF_REG135 0x87
    typedef struct dtof_addressREG135
    {
        dtof_uint16_t trEn0 : 1;
        dtof_uint16_t trEn1 : 1;
        dtof_uint16_t rng1 : 11;
        dtof_uint16_t res : 3;
    } dtof_addressREG135_t;

/**
 * [bit0~2]:  0: state machine is in IDLE state; 1: state machine is in FDLY state;
3: state machine is in UPDH state; 5: state machine is in PCGI state.
[bit3]:   1: PLL lock is stable;  0: PLL lock is not stable
[bit4]:   0: laser driver is stable; 1: laser driver is not stable, laser power is too high.
[bit5~bit9]: frame id 0~31. 实际上是16bit（fifo里）.  这5个bit是16bit的低5位
[bit10]: cypP, 判断解秘钥是否成功
[bit11~15]: input value of GPIO. ， 5个GPIO pin的值。
*/
#define DTOF_REG136 0x88
#define FSM_STATE_FDLY 1
#define FSM_STATE_UPDH 3
#define FSM_STATE_PCGI 5
    //@gqw dbgStat
    typedef struct dtof_addressREG136
    {
#define PLL_STABLE 1
#define PLL_UNSTABLE 0
        dtof_uint16_t fsm_state : 3;
        dtof_uint16_t pll_status : 1;
        dtof_uint16_t laser_status : 1;
        dtof_uint16_t frame_id : 5;
        dtof_uint16_t dbgcyp : 1;
        dtof_uint16_t gpip_value : 5;
    } dtof_addressREG136_t;

#define DTOF_REG137 0x89
    typedef struct dtof_addressREG137
    {
#define RAMX0_OCCUPY 0
#define RAMX1_OCCUPY 0x01ff
#define PULL_UP_12BIT 0x1000
        dtof_uint16_t bnkStat : 9;
        dtof_uint16_t res : 7;
    } dtof_addressREG137_t;

#define DTOF_REG138 0x8a
    // 这里 是一个 双通道的 mapping 关系，in 和 out 是在一个 通道上的mapping （bit 0 - 8 的 通道问题）
    typedef struct dtof_addressREG138
    {
        dtof_uint16_t pxlRoiMap0 : 16;
    } dtof_addressREG138_t;

#define DTOF_REG139 0x8b
    typedef struct dtof_addressREG139
    {
        dtof_uint16_t pxlRoiMap1 : 16;
    } dtof_addressREG139_t;

#define DTOF_REG140 0x8c
    typedef struct dtof_addressREG140
    {
        dtof_uint16_t pxlRoiMap2 : 16;
    } dtof_addressREG140_t;

#define DTOF_REG141 0x8d
    typedef struct dtof_addressREG141
    {
        dtof_uint16_t pxlRoiMap3 : 16;
    } dtof_addressREG141_t;

#define DTOF_REG142 0x8e
    typedef struct dtof_addressREG142
    {
        dtof_uint16_t pxlRoiMap4 : 16;
    } dtof_addressREG142_t;

#define DTOF_REG143 0x8f
    typedef struct dtof_addressREG143
    {
        dtof_uint16_t pxlRoiMap5 : 16;
    } dtof_addressREG143_t;

#define DTOF_REG144 0x90
    typedef struct dtof_addressREG144
    {
        dtof_uint16_t pxlRoiMap6 : 16;
    } dtof_addressREG144_t;

#define DTOF_REG145 0x91
    typedef struct dtof_addressREG145
    {
        dtof_uint16_t pxlRoiMap7 : 16;
    } dtof_addressREG145_t;

#define DTOF_REG146 0x92
    typedef struct dtof_addressREG146
    {
        dtof_uint16_t pxlRoiMap8 : 16;
    } dtof_addressREG146_t;

#define DTOF_REG147 0x93
    typedef struct dtof_addressREG147
    {
        dtof_uint16_t mfknl0 : 8;
        dtof_uint16_t mfknl1 : 8;
    } dtof_addressREG147_t;

#define DTOF_REG148 0x94
    typedef struct dtof_addressREG148
    {
        dtof_uint16_t mfknl2 : 8;
        dtof_uint16_t mfknl3 : 8;
    } dtof_addressREG148_t;

#define DTOF_REG149 0x95
    typedef struct dtof_addressREG149
    {
        dtof_uint16_t mfknl4 : 8;
        dtof_uint16_t mfknl5 : 8;
    } dtof_addressREG149_t;

#define DTOF_REG150 0x96
    typedef struct dtof_addressREG150
    {
        dtof_uint16_t mfknl6 : 8;
        dtof_uint16_t mfknl7 : 8;
    } dtof_addressREG150_t;

#define DTOF_REG151 0x97
    typedef struct dtof_addressREG151
    {
        dtof_uint16_t mfknl8 : 8;
        dtof_uint16_t mfknl9 : 8;
    } dtof_addressREG151_t;

#define DTOF_REG152 0x98
    typedef struct dtof_addressREG152
    {
        dtof_uint16_t mfknl10 : 8;
        dtof_uint16_t mfknl11 : 8;
    } dtof_addressREG152_t;

#define DTOF_REG153 0x99
    typedef struct dtof_addressREG153
    {
        dtof_uint16_t mfknl12 : 8;
        dtof_uint16_t mfknl13 : 8;
    } dtof_addressREG153_t;

#define DTOF_REG154 0x9a
    typedef struct dtof_addressREG154
    {
        dtof_uint16_t mfknl14 : 8;
        dtof_uint16_t mfknl15 : 8;
    } dtof_addressREG154_t;

#define DTOF_REG162 0xa2
    // 代表感兴趣的区域， qlfAPL 和 qlfAPH
    // 代表这Y轴的上下阈值， 0 的两个value 代表着main 的ROI范围 ， 1 代表着ref 的ROI范围
    typedef struct dtof_addressREG162
    {
        dtof_uint16_t qlfApL : 16;
    } dtof_addressREG162_t;

#define DTOF_REG163 0xa3
    typedef struct dtof_addressREG163
    {
        dtof_uint16_t qlfApH : 16;
    } dtof_addressREG163_t;

#define DTOF_REG164 0xa4
    typedef struct dtof_addressREG164
    {
        dtof_uint16_t qlfDpL0 : 8;
        dtof_uint16_t qlfDpH0 : 8;
    } dtof_addressREG164_t;

#define DTOF_REG165 0xa5
    typedef struct dtof_addressREG165
    {
        dtof_uint16_t qlfDpL1 : 8;
        dtof_uint16_t qlfDpH1 : 8;
    } dtof_addressREG165_t;

#define DTOF_REG166 0xa6
    // 超精读相关 0xa6-0xab
    typedef struct dtof_addressREG166
    {
        dtof_uint16_t subLut0 : 8;
        dtof_uint16_t subLut1 : 8;
    } dtof_addressREG166_t;

#define DTOF_REG167 0xa7
    typedef struct dtof_addressREG167
    {
        dtof_uint16_t subLut2 : 8;
        dtof_uint16_t subLut3 : 8;
    } dtof_addressREG167_t;

#define DTOF_REG168 0xa8
    typedef struct dtof_addressREG168
    {
        dtof_uint16_t subLut4 : 8;
        dtof_uint16_t subLut5 : 8;
    } dtof_addressREG168_t;

#define DTOF_REG169 0xa9
    typedef struct dtof_addressREG169
    {
        dtof_uint16_t subLut6 : 8;
        dtof_uint16_t subLut7 : 8;
    } dtof_addressREG169_t;

#define DTOF_REG170 0xaa
    typedef struct dtof_addressREG170
    {
        dtof_uint16_t subLut8 : 8;
        dtof_uint16_t subLut9 : 8;
    } dtof_addressREG170_t;

#define DTOF_REG171 0xab
    typedef struct dtof_addressREG171
    {
        dtof_uint16_t subLut10 : 8;
        dtof_uint16_t subLut11 : 8;
    } dtof_addressREG171_t;

#define DTOF_REG172 0xac
    // 现在cover glass消除可以减掉64个。 0xac-0xbb
    typedef struct dtof_addressREG172
    {
        dtof_uint16_t neXt0 : 16;
    } dtof_addressREG172_t;

#define DTOF_REG173 0xad
    typedef struct dtof_addressREG173
    {
        dtof_uint16_t neXt1 : 16;
    } dtof_addressREG173_t;

#define DTOF_REG174 0xae
    typedef struct dtof_addressREG174
    {
        dtof_uint16_t neXt2 : 16;
    } dtof_addressREG174_t;

#define DTOF_REG175 0xaf
    typedef struct dtof_addressREG175
    {
        dtof_uint16_t neXt3 : 16;
    } dtof_addressREG175_t;

#define DTOF_REG176 0xb0
    typedef struct dtof_addressREG176
    {
        dtof_uint16_t neXt4 : 16;
    } dtof_addressREG176_t;

#define DTOF_REG177 0xb1
    typedef struct dtof_addressREG177
    {
        dtof_uint16_t neXt5 : 16;
    } dtof_addressREG177_t;

#define DTOF_REG178 0xb2
    typedef struct dtof_addressREG178
    {
        dtof_uint16_t neXt6 : 16;
    } dtof_addressREG178_t;

#define DTOF_REG179 0xb3
    typedef struct dtof_addressREG179
    {
        dtof_uint16_t neXt7 : 16;
    } dtof_addressREG179_t;

#define DTOF_REG180 0xb4
    typedef struct dtof_addressREG180
    {
        dtof_uint16_t neXt8 : 16;
    } dtof_addressREG180_t;

#define DTOF_REG181 0xb5
    typedef struct dtof_addressREG181
    {
        dtof_uint16_t neXt9 : 16;
    } dtof_addressREG181_t;

#define DTOF_REG182 0xb6
    typedef struct dtof_addressREG182
    {
        dtof_uint16_t neXt10 : 16;
    } dtof_addressREG182_t;

#define DTOF_REG183 0xb7
    typedef struct dtof_addressREG183
    {
        dtof_uint16_t neXt11 : 16;
    } dtof_addressREG183_t;

#define DTOF_REG184 0xb8
    typedef struct dtof_addressREG184
    {
        dtof_uint16_t neXt12 : 16;
    } dtof_addressREG184_t;

#define DTOF_REG185 0xb9
    typedef struct dtof_addressREG185
    {
        dtof_uint16_t neXt13 : 16;
    } dtof_addressREG185_t;

#define DTOF_REG186 0xba
    typedef struct dtof_addressREG186
    {
        dtof_uint16_t neXt14 : 16;
    } dtof_addressREG186_t;

#define DTOF_REG187 0xbb
    typedef struct dtof_addressREG187
    {
        dtof_uint16_t neXt15 : 16;
    } dtof_addressREG187_t;

#define DTOF_REG188 0xbc
    // cover glass消除的倍数调整 @gqw 哪个寄存器控制倍数自动调节功能 待确认
    // 1）下面0x81寄存器控制开了倍数自动调节功能，软件只能配置低3位，不能大于7.  高4位硬件自动控制。=》除去符号位，再除去硬件16倍（移4位），只剩3位可配
    // 2）没有开自动的话，只能配置低7位，不能大于127=》最高位为符号位，不配置
    typedef struct dtof_addressREG188
    {
        dtof_uint16_t neXtBstr : 8;
        dtof_uint16_t res : 8;
    } dtof_addressREG188_t;

#define DTOF_REG189 0xbd
    // defaut值是7‘b0001101
    // bit[0]: 控制main要不要使能cover glass removal功能。 1：使能，0： disable
    // bit[1]: 控制Ref要不要使能cover glass removal功能。 1：使能，0： disable
    // bit[2]: 控制main要不要使能调整cover glass增益自动调节功能， 0:自动, 1：不自动
    // bit[3]: 控制Ref要不要使能调整cover glass增益自动调节功能， 0:自动, 1：不自动
    typedef struct dtof_addressREG189
    {
        dtof_uint16_t rmvNextRoi : 8; // neXt ROI based enable bit[1,0], and auto caliberation by last fram bit[3,2]
        dtof_uint16_t xtW : 5;
        dtof_uint16_t res : 3;
    } dtof_addressREG189_t;

#define DTOF_REG200 0xc8
    typedef struct dtof_addressREG200
    {
        dtof_uint16_t reserv0 : 16;
    } dtof_addressREG200_t;

#define DTOF_REG201 0xc9
    typedef struct dtof_addressREG201
    {
        dtof_uint16_t reserv1 : 16;
    } dtof_addressREG201_t;

#define DTOF_REG202 0xca
    typedef struct dtof_addressREG202
    {
        dtof_uint16_t reserv2 : 16;
    } dtof_addressREG202_t;

    // #define DTOF_REG203   0xcb
    // typedef struct dtof_addressREG203 {
    //      dtof_uint16_t reserv3 : 15;
    // }dtof_addressREG203_t;

#define DTOF_REG204 0xcc
#define DTOF_SPAD_STARTING_ADDRESS DTOF_REG204
    typedef struct dtof_addressREG204
    {
        dtof_uint16_t spdMskb0 : 16;
    } dtof_addressREG204_t;

#define DTOF_REG205 0xcd
    typedef struct dtof_addressREG205
    {
        dtof_uint16_t spdMskb1 : 16;
    } dtof_addressREG205_t;

#define DTOF_REG206 0xce
    typedef struct dtof_addressREG206
    {
        dtof_uint16_t spdMskb2 : 16;
    } dtof_addressREG206_t;

#define DTOF_REG207 0xcf
    typedef struct dtof_addressREG207
    {
        dtof_uint16_t spdMskb3 : 16;
    } dtof_addressREG207_t;

#define DTOF_REG208 0xd0
    typedef struct dtof_addressREG208
    {
        dtof_uint16_t spdMskb4 : 16;
    } dtof_addressREG208_t;

#define DTOF_REG209 0xd1
    // 1) bit0:
    // 这里需要先配置 spad mask，然后再执行mskReq
    // Toggle的方式, 先读出来，取反，再写回就会生效(0到1，或者1到0), toggle一次就会产生一个RST信号由低到高的脉冲，这样就reset一次；RST脚默认情况下是为低的。
    // 2) bit1 reserved.
    typedef struct dtof_addressREG209
    {
        dtof_uint16_t mskReq : 2;
        dtof_uint16_t res : 14;
    } dtof_addressREG209_t;

#define DTOF_REG210 0xd2
    typedef struct dtof_addressREG210
    {
        dtof_uint16_t xtDfct : 16;
    } dtof_addressREG210_t;

#define DTOF_REG211 0xd3
#define DTOF_EXCK DTOF_REG211
    // int_mode -- 1: intterrupt to GPIO4;0: intterrupt to innerMCU

    // hgJmp_mode：
    // 1: when the user read Ram, if the read ram address reach the threshold(0xFD register's value), the read ram address will start from 0 again. 这时FD的值才有效。
    // 0: not return to address 0. （表示功能关闭） 这个设成0就不会wrap around了，fd就不生效了。

    // clk_pol：
    // 没用了 for debug only.
    // can use this bit to delay the analog clock to meet the time demand.

    // spiModReg：
    // 优先级顺序：spimodreg>i2cmodreg >goio3_pad
    // 只要这个bit=1，一定是spimode；如果bit=0，则check i2cmodreg的值，
    // 为1则是i2cmode；i2cmodreg为0则check GPIO3_PAD

    // trgo_mode：
    // 0: not output
    // 1: output VCSEL trigger signal via GPIO2
    // triggerout的周期是： tdc_clk/(2^rngtime).

    // plltst_mode：
    // 0: not output
    // 1: output PLL clock via GPIO4

    // exCk_mode:
    // 000: pll clock（tdc_clk）
    // 111: 使用external clock
    // 110: 使用low power clock
    // 100: 使用osc clock

    // xtTrgPin:
    // use outside signal to trigger VCSEL emitting
    // 0: GPIO0   1: GPIO1  2: GPIO2  3:GPIO3..  外部触发模式trigger信号选择用哪一个gpio接进来。
    // 与0x02寄存器的bit 2 extTrgMod联合使用。这个功能未实现
    typedef struct dtof_addressREG211
    {
#define INT_TO_GPIO4 1
#define INT_TO_INNER_MCU 0
        dtof_uint16_t int_mode : 1;
        dtof_uint16_t ramDbg : 1; // 去除
        dtof_uint16_t hgJmp_mode : 1;
        dtof_uint16_t dirRamMd : 1;
        dtof_uint16_t pllTstDv : 2;
        dtof_uint16_t clk_pol : 1;
        dtof_uint16_t reserved12 : 1;
        dtof_uint16_t spiModReg : 1;
        dtof_uint16_t trgo_mode : 1;
        dtof_uint16_t plltst_mode : 1;
#define PLL_CLOCK 0B000
#define EXTERNAL_CLOCK 0B111
#define LOW_POWER_CLOCK 0B110
#define OSC_CLOCK 0B100
        dtof_uint16_t exCk_mode : 3;
        dtof_uint16_t xtTrgPin : 2;
    } dtof_addressREG211_t;

#define DTOF_REG212 0xd4
    typedef struct dtof_addressREG212
    {
#define CJTAG_ENABLE 1
#define CJTAG_DISABLE 0
        dtof_uint16_t cJtagEn : 1;
        dtof_uint16_t scan_comp : 1;
        dtof_uint16_t scan_mode : 1;
        dtof_uint16_t mbist_mode : 1;
        dtof_uint16_t reservedD1 : 1;
        dtof_uint16_t DFTRsv : 11;
    } dtof_addressREG212_t;

#define DTOF_REG213 0xd5
    // 内部有个i2cDly[3:0],它的来源由regO.i2cDly[4]和otpDat.i2cDly[4]来决定：
    // regO.i2cDly[4]=1 -> i2cDly[3:0]=regO.i2cDly[3:0];
    // regO.i2cDly[4]=0 -> 看otpDat.i2cDly[4]：
    // 1）若otpDat.i2cDly[4]=0->i2cDly[3:0]=otpDat.i2cDly[3:0];
    // 2）若otpDat.i2cDly[4]=1->i2cDly[3:0]=regO.i2cDly[3:0];
    // 然后根据这个得出来的i2cDly[2]来决定是自动delay还是根据i2cDly[1:0]来delay；
    // bit[2] = 1: 硬
    typedef struct dtof_addressREG213
    {
        dtof_uint16_t gpioDrvS : 2;
        // dtof_uint16_t gpRegOut : 5;
        dtof_uint16_t gpRegOut0 : 1;
        dtof_uint16_t gpRegOut1 : 1;
        dtof_uint16_t gpRegOut2 : 1;
        dtof_uint16_t gpRegOut3 : 1;
        dtof_uint16_t gpRegOut4 : 1;
        dtof_uint16_t gpioCnct : 4;
        dtof_uint16_t i2cDly : 5;
    } dtof_addressREG213_t;

#define DTOF_REG214 0xd6
    typedef struct dtof_addressREG214
    {
#define HROSC_AND_EXTERN_REF_END 0B00
#define HROSC_END_AND_EXTERN_REF 0B01
#define HROSC_END_AND_REF_IIC_SCLK 0B10
#define HROSC_AND_IIC_REF 0B10
#define HROSC_AND_IIC_PVT 0B11
#define OSC_CAL_ENABLE 1
#define OSC_CAL_DISABLE 0
#define OSC_CAL_CLR_START 1
#define OSC_CAL_CLR_END 0
        dtof_uint16_t gpioPinCfg0 : 2;
        dtof_uint16_t gpioPinCfg1 : 2;
        dtof_uint16_t gpioPinCfg2 : 2;
        dtof_uint16_t gpioPinCfg3 : 2;
        dtof_uint16_t gpioPinCfg4 : 2;
        dtof_uint16_t OSC_CAL_MODE : 2;
        dtof_uint16_t OSC_CAL_EN : 1;
        dtof_uint16_t OSC_CAL_CLR : 1;
        dtof_uint16_t osc_cal_reserved : 2;
    } dtof_addressREG214_t;

#define DTOF_REG215 0xd7
    typedef struct dtof_addressREG215
    {
        dtof_uint16_t oscCntRl : 16;
    } dtof_addressREG215_t;

#define DTOF_REG216 0xd8
    typedef struct dtof_addressREG216
    {
        dtof_uint16_t oscCntRh : 16;
    } dtof_addressREG216_t;

#define DTOF_REG217 0xd9
    typedef struct dtof_addressREG217
    {
        dtof_uint16_t rfcCntRl : 16;
    } dtof_addressREG217_t;

#define DTOF_REG218 0xda
    typedef struct dtof_addressREG218
    {
        dtof_uint16_t rfcCntRh : 16;
    } dtof_addressREG218_t;

#define DTOF_REG219 0xdb
    typedef struct dtof_addressREG219
    {
        dtof_uint16_t pvtCntRl : 16;
    } dtof_addressREG219_t;

#define DTOF_REG220 0xdc
    typedef struct dtof_addressREG220
    {
        dtof_uint16_t pvtCntRh : 16;
    } dtof_addressREG220_t;

#define DTOF_REG221 0xdd
    typedef struct dtof_addressREG221
    {
        dtof_uint16_t tgtCntRl : 16;
    } dtof_addressREG221_t;

#define DTOF_REG222 0xde
    typedef struct dtof_addressREG222
    {
        dtof_uint16_t tgtCntRh : 16;
    } dtof_addressREG222_t;

#define DTOF_REG223 0xdf
    typedef struct dtof_addressREG223
    {
        dtof_uint16_t mkvres : 16;
    } dtof_addressREG223_t;

#define DTOF_REG224 0xe0
    typedef struct dtof_addressREG224
    {
        dtof_uint16_t tdcLlim : 8;
        dtof_uint16_t tdcUlim : 8;
    } dtof_addressREG224_t;

    // no E1
    // #define DTOF_REG225   0xe1
    // typedef struct dtof_addressREG225 {
    //      dtof_uint16_t TDC_MKV_bypass : 1;
    //      dtof_uint16_t TDC_MKV_CTL0 : 3;
    //      dtof_uint16_t TDC_MKV_CTL1 : 3;
    //      dtof_uint16_t TDC_MKV_CTL2 : 3;
    //      dtof_uint16_t TDC_MKV_CTL3 : 3;
    //      dtof_uint16_t TDC_MKV_CTL4 : 3;
    // }dtof_addressREG225_t;

#define DTOF_REG226 0xe2
    // bit9 = 0: 广播模式
    // bit9 = 1: 自治模式
    // bit10是reserved, 其他bit为bin offset的设置值；
    // mfBinOfs0到mfBinOfs8, 如果bit9都设置为0，则只用设置mfBinOfs0即可，
    // 这个offset会应用到其他TDC； 如果某个mfBinOfsX的bit9设置为1，则这个TDC自制，自己设置独立的binofs, 其他TDC还是沿用mfBinOfs0的。
    typedef struct dtof_addressREG226
    {
        dtof_uint16_t mfBinOfs0 : 9;
        dtof_uint16_t mode : 1;
        dtof_uint16_t res : 6;
    } dtof_addressREG226_t;

#define DTOF_REG227 0xe3
    typedef struct dtof_addressREG227
    {
        dtof_uint16_t mfBinOfs1 : 9;
        dtof_uint16_t mode : 1;
        dtof_uint16_t res : 6;
    } dtof_addressREG227_t;

#define DTOF_REG228 0xe4
    typedef struct dtof_addressREG228
    {
        dtof_uint16_t mfBinOfs2 : 9;
        dtof_uint16_t mode : 1;
        dtof_uint16_t res : 6;
    } dtof_addressREG228_t;

#define DTOF_REG229 0xe5
    typedef struct dtof_addressREG229
    {
        dtof_uint16_t mfBinOfs3 : 9;
        dtof_uint16_t mode : 1;
        dtof_uint16_t res : 6;
    } dtof_addressREG229_t;

#define DTOF_REG230 0xe6
    typedef struct dtof_addressREG230
    {
        dtof_uint16_t mfBinOfs4 : 9;
        dtof_uint16_t mode : 1;
        dtof_uint16_t res : 6;
    } dtof_addressREG230_t;

#define DTOF_REG231 0xe7
    typedef struct dtof_addressREG231
    {
        dtof_uint16_t mfBinOfs5 : 9;
        dtof_uint16_t mode : 1;
        dtof_uint16_t res : 6;
    } dtof_addressREG231_t;

#define DTOF_REG232 0xe8
    typedef struct dtof_addressREG232
    {
        dtof_uint16_t mfBinOfs6 : 9;
        dtof_uint16_t mode : 1;
        dtof_uint16_t res : 6;
    } dtof_addressREG232_t;

#define DTOF_REG233 0xe9
    typedef struct dtof_addressREG233
    {
        dtof_uint16_t mfBinOfs7 : 9;
        dtof_uint16_t mode : 1;
        dtof_uint16_t res : 6;
    } dtof_addressREG233_t;

#define DTOF_REG234 0xea
    typedef struct dtof_addressREG234
    {
        dtof_uint16_t mfBinOfs8 : 9;
        dtof_uint16_t mode : 1;
        dtof_uint16_t res : 6;
    } dtof_addressREG234_t;

#define DTOF_REG235 0xeb
    // EB has only 15bit
    typedef struct dtof_addressREG235
    {
#define PVT_OCS_ENABLE 1
#define PVT_OCS_DISABLE 0
#define VQ33_ENABLE 1
        dtof_uint16_t OSC_FTRIM : 8;
        dtof_uint16_t OSC_TCSEL : 3;
        dtof_uint16_t BGR_TBUF_EN : 1;
        dtof_uint16_t VQ33_EN : 1;
        dtof_uint16_t PVTOSC_EN : 1;
        dtof_uint16_t SDA_PU_REG : 1;
        dtof_uint16_t res : 1;
    } dtof_addressREG235_t;

#define DTOF_REG236 0xec
#define DTOF_PLLEN DTOF_REG236
    // PLL_O_CTL:
    // 输出分频；2'b00~2'b11对应2,4,8,16分频.. 默认值是2'b10, 所以就是8分频，
    // 所以1Ghz的pll经过8分频，变成125Mhz，输出给数字模块。如果写成11，
    // 就是16分频，所以给数字的时钟就是62.5Mhz.
    typedef struct dtof_addressREG236
    {
        dtof_uint16_t HROSC_EN : 1;
        dtof_uint16_t HPLDO_EN : 1;
        dtof_uint16_t PLL_EN : 1;
        dtof_uint16_t PLL_IN_CTL : 3;
        dtof_uint16_t PLL_FB_CTL : 8;
#define PLL_O_16_DIVISION 0B11 // default 62.5M
#define PLL_O_8_DIVISION 0B10  // 125M
#define PLL_O_4_DIVISION 0B01  // 250M
#define PLL_O_2_DIVISION 0B00  // 500M
        dtof_uint16_t PLL_O_CTL : 2;
    } dtof_addressREG236_t;

#define DTOF_REG237 0xed
#define DTOF_HVPP DTOF_REG237
    typedef struct dtof_addressREG237
    {
#define HVPP_ENABLE 1
        dtof_uint16_t HVPP_EN : 1;
        dtof_uint16_t HVPP_SEL : 5;
        dtof_uint16_t HVPP_TC_TRIM : 4;
        dtof_uint16_t HVPP_VO_TRIM : 4;
        dtof_uint16_t HVPP_CLK_SEL : 1;
        dtof_uint16_t res : 1;
    } dtof_addressREG237_t;

#define DTOF_REG238 0xee
    typedef struct dtof_addressREG238
    {
        dtof_uint16_t BGR_VTRIM : 5;
        dtof_uint16_t BGR_ITRIM : 4;
        dtof_uint16_t VQ33_VSEL : 6;
        dtof_uint16_t res : 1;
    } dtof_addressREG238_t;

#define DTOF_REG239 0xef
    typedef struct dtof_addressREG239
    {
        dtof_uint16_t DEGLITCH_SEL : 2;
        dtof_uint16_t trig_out_sel : 1;
        dtof_uint16_t TM_DRV_LTRG : 1;
        dtof_uint16_t TM_EYESFTY : 1;
        dtof_uint16_t LDD_ctrl_r0 : 11;
    } dtof_addressREG239_t;

#define DTOF_REG240 0xf0
    typedef struct dtof_addressREG240
    {
#define DRV_IPKSEL_10MA 0X1
#define DRV_IPKSEL_40MA 0X2
#define DRV_IPKSEL_50MA 0X4
#define DRV_IPKSEL_150MA 0X8
#define DRV_IPKSEL_IR 0XF
        dtof_uint16_t DRV_IPKSEL : 4;
        dtof_uint16_t VCCIO_VSEL : 3; // TODO @gqw need to check with nannan | done 1. DRV_LTRG_EN no use
        dtof_uint16_t LDD_ctrl_r1 : 9;
    } dtof_addressREG240_t;

#define DTOF_REG241 0xf1
    typedef struct dtof_addressREG241
    {
        dtof_uint16_t ATO_SEL : 14;
        dtof_uint16_t spdtst_mode : 2;
    } dtof_addressREG241_t;

#define DTOF_REG242 0xf2
#define DTOF_DRIVER DTOF_REG242
// 1. 需要使能  DRV_PUMP_EN    8bit
// 2. 保持DRV_PUMP_VSEL 配置 为 b'100 (11bit)
// 1 和 2  的 合 为   0x0900
#define DTOF_REG242_FIX 0x0900 // 默认是0x0804 将需要配置的值置0,用于&
    typedef struct dtof_addressREG242
    {
#define PG_ENABLE 1
#define DRV_PUMP_ENABLE 1
#define DRV_PUMP_DISABLE 0
        dtof_uint16_t OSCLP_FTRIM : 3; // TODO @gqw need to check with nannan | done 2. BGRLP_VTRIM no use
        dtof_uint16_t PG_EN : 1;
#define PG_TRIM_IR 0X8
#define PG_TRIM_NEW_MIN 0X7
#define PG_TRIM_HM_MIN 0X0
        dtof_uint16_t pg_trim : 4;
        dtof_uint16_t DRV_PUMP_EN : 1;
        dtof_uint16_t DRV_PUMP_VSEL : 3;
        dtof_uint16_t eyesf_enb : 1;
        dtof_uint16_t res : 3;
    } dtof_addressREG242_t;

#define DTOF_REG243 0xf3
    typedef struct dtof_addressREG243
    {
        dtof_uint16_t eyesf_sta : 1;
        dtof_uint16_t res : 15;
    } dtof_addressREG243_t;

#define DTOF_REG244 0xf4
    typedef struct dtof_addressREG244
    {
        dtof_uint16_t PRIME_CNT : 5;
        dtof_uint16_t LPLDOLoopAMPEN : 1;
        dtof_uint16_t LPLDOI4u_EN : 1;
        dtof_uint16_t PLL_FSEL : 2;
        dtof_uint16_t TZD_SEL_EN : 2;
        dtof_uint16_t reservedA : 3;
        dtof_uint16_t res : 2;
    } dtof_addressREG244_t;

#define DTOF_REG250 0xfa
    typedef struct dtof_addressREG250
    {
        dtof_uint16_t bbboStartA : 16;
    } dtof_addressREG250_t;

#define DTOF_REG251 0xfb
#define OTP_CONTROL_REG_ADDRESS DTOF_REG251
    /*
    PPROG	251	251	FB	PPROG	0	1'b0	RW	OTP program mode
    PWE		251	FB	PWE	1	1'b0	RW	OTP program writen enable
    PTM		251	FB	PTM[1:0]	3:2	2'd0	RW	OTP test mode
    MVPP_EN		251	FB	MVPP_EN	4	1'd0	RH	MVPP enalbe sig
    OTP_EN		251	FB	OTP_EN	5	1'd0	RH	OTP enable for any OTP access
    PA	252	252	FC	PA[7:0]	7:0	8'd0	RW	OTP address
    */
    typedef struct dtof_addressREG251
    {
        dtof_uint16_t PPROG : 1;
        dtof_uint16_t PWE : 1;
        dtof_uint16_t PTM : 2;
        dtof_uint16_t MVPP_EN : 1;
        dtof_uint16_t OTP_EN : 1;
        dtof_uint16_t res : 10;
    } dtof_addressREG251_t;

#define DTOF_REG252 0xfc
    typedef struct dtof_addressREG252
    {
        dtof_uint16_t PA : 8;
        dtof_uint16_t PDIN : 8;
    } dtof_addressREG252_t;

#define DTOF_REG253 0xfd
    typedef struct dtof_addressREG253
    {
        dtof_uint16_t hgWrpThd : 16;
    } dtof_addressREG253_t;

#define DTOF_REG254 0xfe
    typedef struct dtof_addressREG254
    {
        dtof_uint16_t ramStartA : 16;
    } dtof_addressREG254_t;

#define DTOF_REG255 0xff
    typedef struct dtof_addressREG262
    {
        dtof_uint16_t reserved0 : 6;
        dtof_uint16_t roiMDqlf0 : 1;
        dtof_uint16_t mmxpF0 : 9;
    } dtof_addressREG262_t;

    typedef struct dtof_addressREG263
    {
        dtof_uint16_t reserved0 : 6;
        dtof_uint16_t roiSDqlf0 : 1;
        dtof_uint16_t smxpF0 : 9;
    } dtof_addressREG263_t;

    typedef struct dtof_addressREG264
    {
        dtof_uint16_t reserved0 : 6;
        dtof_uint16_t roiTDqlf0 : 1;
        dtof_uint16_t tmxpF0 : 9;
    } dtof_addressREG264_t;

    typedef struct dtof_addressREG265
    {
        dtof_uint16_t hgmFlsCntR : 16;
    } dtof_addressREG265_t;

    typedef struct dtof_addressREG266
    {
        dtof_uint16_t reserved0 : 7;
        dtof_uint16_t saSgnR0 : 1;
        dtof_uint16_t subAccR0 : 1;
        dtof_uint16_t reserved1 : 7;
    } dtof_addressREG266_t;

    typedef struct dtof_addressREG267
    {
        dtof_uint16_t reserved0 : 7;
        dtof_uint16_t saSgnR0 : 1;
        dtof_uint16_t subAccR0 : 1;
        dtof_uint16_t reserved1 : 7;
    } dtof_addressREG267_t;

    // #define DTOF_REG268   0x40C
    typedef struct dtof_addressREG268
    {
        dtof_uint16_t reserved0 : 7;
        dtof_uint16_t saSgnR0 : 1;
        dtof_uint16_t subAccR0 : 1;
        dtof_uint16_t reserved1 : 7;
    } dtof_addressREG268_t;

    // fred lei

    // #define DTOF_REG269   0x40D
    typedef struct dtof_addressREG269
    {
        dtof_uint16_t mmxvR0 : 16;
    } dtof_addressREG269_t;

    // #define DTOF_REG270   0x40E
    typedef struct dtof_addressREG270
    {
        dtof_uint16_t smxvR0 : 16;
    } dtof_addressREG270_t;

    // #define DTOF_REG271   0x40F
    typedef struct dtof_addressREG271
    {
        dtof_uint16_t tmxvR0 : 16;
    } dtof_addressREG271_t;

    // #define DTOF_REG272   0x410
    typedef struct dtof_addressREG272
    {
        dtof_uint16_t avgQ0 : 16;
    } dtof_addressREG272_t;

    typedef struct dtof_addressREG273
    {
        dtof_uint16_t stdQ0 : 16;
    } dtof_addressREG273_t;

    typedef struct dtof_addressREG274
    {
        dtof_uint16_t reserved0 : 6;
        dtof_uint16_t roiMDqlf0 : 1;
        dtof_uint16_t mmxpF0 : 9;
    } dtof_addressREG274_t;

    typedef struct dtof_addressREG275
    {
        dtof_uint16_t reserved0 : 6;
        dtof_uint16_t roiSDqlf0 : 1;
        dtof_uint16_t smxpF0 : 9;
    } dtof_addressREG275_t;

    typedef struct dtof_addressREG276
    {
        dtof_uint16_t reserved0 : 6;
        dtof_uint16_t roiTDqlf0 : 1;
        dtof_uint16_t tmxpF0 : 9;
    } dtof_addressREG276_t;

    typedef struct dtof_addressREG277
    {
        dtof_uint16_t hgmFlsCntR : 16;
    } dtof_addressREG277_t;

    typedef struct dtof_addressREG278
    {
        dtof_uint16_t reserved0 : 7;
        dtof_uint16_t saSgnR0 : 1;
        dtof_uint16_t subAccR0 : 1;
        dtof_uint16_t reserved1 : 7;
    } dtof_addressREG278_t;

    typedef struct dtof_addressREG279
    {
        dtof_uint16_t reserved0 : 7;
        dtof_uint16_t saSgnR0 : 1;
        dtof_uint16_t subAccR0 : 1;
        dtof_uint16_t reserved1 : 7;
    } dtof_addressREG279_t;
    typedef struct dtof_addressREG280
    {
        dtof_uint16_t reserved0 : 7;
        dtof_uint16_t saSgnR0 : 1;
        dtof_uint16_t subAccR0 : 1;
        dtof_uint16_t reserved1 : 7;
    } dtof_addressREG280_t;

    typedef struct dtof_addressREG281
    {
        dtof_uint16_t mxvR0 : 16;
    } dtof_addressREG281_t;

    typedef struct dtof_addressREG282
    {
        dtof_uint16_t reserved0 : 7;
        dtof_uint16_t mxpR0 : 9;
    } dtof_addressREG282_t;

    typedef struct dtof_addressREG283
    {
        dtof_uint16_t mxvR1 : 16;
    } dtof_addressREG283_t;

    typedef struct dtof_addressREG284
    {
        dtof_uint16_t reserved0 : 7;
        dtof_uint16_t mxpR1 : 9;
    } dtof_addressREG284_t;

    typedef struct dtof_addressREG285
    {
        dtof_uint16_t mxvR2 : 16;
    } dtof_addressREG285_t;
    typedef struct dtof_addressREG286
    {
        dtof_uint16_t reserved0 : 7;
        dtof_uint16_t mxvR2 : 9;
    } dtof_addressREG286_t;

    typedef struct dtof_addressREG287
    {
        dtof_uint16_t mxvR3 : 16;
    } dtof_addressREG287_t;

    typedef struct dtof_addressREG288
    {
        dtof_uint16_t reserved0 : 7;
        dtof_uint16_t mxpR3 : 9;
    } dtof_addressREG288_t;

    typedef struct dtof_addressREG289
    {
        dtof_uint16_t mxvR4 : 16;
    } dtof_addressREG289_t;

    typedef struct dtof_addressREG290
    {
        dtof_uint16_t reserved0 : 7;
        dtof_uint16_t mxpR4 : 9;
    } dtof_addressREG290_t;

    typedef struct dtof_addressREG291
    {
        dtof_uint16_t mxvR5 : 16;
    } dtof_addressREG291_t;

    typedef struct dtof_addressREG292
    {
        dtof_uint16_t reserved0 : 7;
        dtof_uint16_t mxpR5 : 9;
    } dtof_addressREG292_t;

    typedef struct dtof_addressREG293
    {
        dtof_uint16_t mxvR6 : 16;
    } dtof_addressREG293_t;

    typedef struct dtof_addressREG294
    {
        dtof_uint16_t reserved0 : 7;
        dtof_uint16_t mxpR6 : 9;
    } dtof_addressREG294_t;

    typedef struct dtof_addressREG295
    {
        dtof_uint16_t mxvR7 : 16;
    } dtof_addressREG295_t;

    typedef struct dtof_addressREG296
    {
        dtof_uint16_t reserved0 : 7;
        dtof_uint16_t mxpR7 : 9;
    } dtof_addressREG296_t;

    typedef struct dtof_addressREG297
    {
        dtof_uint16_t mxvR8 : 16;
    } dtof_addressREG297_t;

    typedef struct dtof_addressREG298
    {
        dtof_uint16_t reserved0 : 7;
        dtof_uint16_t mxpR8 : 9;
    } dtof_addressREG298_t;

#define DTOF_REG256 0x400
    typedef struct dtof_addressREG256
    {
        dtof_uint16_t frameID : 16;
#define DTOF_REG257 0x401
        dtof_uint16_t mmxvR0 : 16;
#define DTOF_REG258 0x402
        dtof_uint16_t smxvR0 : 16;
#define DTOF_REG259 0x403
        dtof_uint16_t tmxvR0 : 16;
#define DTOF_REG260 0x404
        dtof_uint16_t avgQ0 : 16;
#define DTOF_REG261 0x405
        dtof_uint16_t stdQ0 : 16;
#define DTOF_REG262 0x406
        dtof_addressREG262_t fifo5;
#define DTOF_REG263 0x407
        dtof_addressREG263_t fifo6;
#define DTOF_REG264 0x408
        dtof_addressREG264_t fifo7;
#define DTOF_REG265 0x409
        dtof_addressREG265_t fifo9;
#define DTOF_REG266 0x40A
        dtof_addressREG266_t fifo0a;
#define DTOF_REG267 0x40B
        dtof_addressREG267_t fifo0b;

#define DTOF_REG268 0x40c
        dtof_addressREG268_t fifo0c;

#define DTOF_REG269 0x40d
        dtof_addressREG269_t fifo0d;

#define DTOF_REG270 0x40e
        dtof_addressREG270_t fifo0e;

#define DTOF_REG271 0x40f
        dtof_addressREG271_t fifo0f;

#define DTOF_REG272 0x410
        dtof_addressREG272_t fifo10;

#define DTOF_REG273 0x411
        dtof_addressREG273_t fifo11;

#define DTOF_REG274 0x412
        dtof_addressREG274_t fifo12;
#define DTOF_REG275 0x413
        dtof_addressREG275_t fifo13;
#define DTOF_REG276 0x414
        dtof_addressREG276_t fifo14;
#define DTOF_REG277 0x415
        dtof_addressREG277_t fifo15;
#define DTOF_REG278 0x416
        dtof_addressREG278_t fifo16;

#define DTOF_REG279 0x417
        dtof_addressREG279_t fifo17;

#define DTOF_REG280 0x418
        dtof_addressREG280_t fifo18;
#define DTOF_REG281 0x419
        dtof_addressREG281_t fifo19;
#define DTOF_REG282 0x41A
        dtof_addressREG282_t fifo1a;

#define DTOF_REG283 0x41B
        dtof_addressREG283_t fifo1b;
#define DTOF_REG284 0x41C
        dtof_addressREG284_t fifo1c;
#define DTOF_REG285 0x41D
        dtof_addressREG285_t fifo1d;
#define DTOF_REG286 0x41E
        dtof_addressREG286_t fifo1e;
#define DTOF_REG287 0x41F
        dtof_addressREG287_t fifo1f;
#define DTOF_REG288 0x420
        dtof_addressREG288_t fifo20;
#define DTOF_REG289 0x421
        dtof_addressREG289_t fifo21;

#define DTOF_REG290 0x422
        dtof_addressREG290_t fifo22;

#define DTOF_REG291 0x423
        dtof_addressREG291_t fifo23;
#define DTOF_REG292 0x424
        dtof_addressREG292_t fifo24;
#define DTOF_REG293 0x425
        dtof_addressREG293_t fifo25;

#define DTOF_REG294 0x426
        dtof_addressREG294_t fifo26;

#define DTOF_REG295 0x427
        dtof_addressREG295_t fifo27;
#define DTOF_REG296 0x428
        dtof_addressREG296_t fifo28;
#define DTOF_REG297 0x429
        dtof_addressREG297_t fifo29;
#define DTOF_REG298 0x430
        dtof_addressREG298_t fifo30;
    } dtof_fifo_struct;

#ifdef __cplusplus
}
#endif

#endif // _DTOF_REG_H_
