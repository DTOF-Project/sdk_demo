#include <math.h>
#include <stdint.h>

// determine whether the frame is legal
#define FLOAT_EPSILON 1e-6f
#define DISTANCE_STEP_NUM 6
#define LEGAL_FRAME 1
#define ILLEGAL_FRAME 0

typedef struct
{
    int16_t dis_th;
    float snr_th;
} confidence_param_t;

static void dtof_calc_snr(uint16_t main_cnt, float main_noise, float *snr)
{
    if (main_cnt > main_noise)
    {
        if (main_noise == 0.0f)
        {
            *snr = 0;
        }
        else
        {
            *snr = (main_cnt - main_noise) / sqrtf(main_noise);
        }
    }
    else
    {
        *snr = 0;
    }
}

/**
 * @brief Judge whether the frame is legal or not according to the distance and intensity.
 * @param[in] first_target The first target distance.
 * @param[in] first_intensity The intensity of the first target.
 * @param[in] ambient The ambient light noise.
 * @return LEGAL_FRAME if the frame is legal, ILLEGAL_FRAME if the frame is illegal.
 */
int dtof_clac_confidence(int16_t first_target, uint16_t first_intensity, float ambient)
{
    float snr;

    const confidence_param_t confidence_param[DISTANCE_STEP_NUM] = {
        {0, 40},
        {20, 16},
        {120, 8},
        {600, 4.5f},
        {1300, 6},
        {INT16_MAX, 4}};

    dtof_calc_snr(first_intensity, ambient, &snr);

    if (fabsf(snr) < FLOAT_EPSILON)
    {
        return ILLEGAL_FRAME;
    }

    for (int index = 0; index < DISTANCE_STEP_NUM; ++index)
    {
        if (first_target < confidence_param[index].dis_th)
        {
            if (snr > confidence_param[index].snr_th)
            {
                return LEGAL_FRAME;
            }
            else
            {
                return ILLEGAL_FRAME;
            }
        }
    }

    return ILLEGAL_FRAME;
}