#include "FreeRTOS.h"
#include "task.h"
#include "cyabs_rtos.h"
#include "cyabs_rtos_impl.h"
#include "cybsp.h"
#include "cyhal.h"
#include "cyhal_syspm.h"

#define PD_TICKS_TO_MS(x_ticks) (((TickType_t)(x_ticks) * 1000u) / configTICK_RATE_HZ)

void vApplicationSleep(TickType_t xExpectedIdleTime)
{
    static cyhal_lptimer_t timer;
    cyhal_lptimer_t *lptimer = cyabs_rtos_get_lptimer();
    uint32_t actual_sleep_ms = 0;
    cy_rslt_t result = CY_RSLT_SUCCESS;

    if (lptimer == NULL)
    {
        result = cyhal_lptimer_init(&timer);
        if (result == CY_RSLT_SUCCESS)
        {
            cyabs_rtos_set_lptimer(&timer);
            lptimer = &timer;
        }
        else
        {
            CY_ASSERT(false);
            return;
        }
    }

    {
        uint32_t status = cyhal_system_critical_section_enter();
        eSleepModeStatus sleep_status = eTaskConfirmSleepModeStatus();

        if (sleep_status != eAbortSleep)
        {
            uint32_t sleep_ms = PD_TICKS_TO_MS(xExpectedIdleTime);
            uint32_t sleep_latency =
#if defined(CY_CFG_PWR_SLEEP_LATENCY)
                CY_CFG_PWR_SLEEP_LATENCY +
#endif
                0u;

            /*
             * emUSB on CAT1A needs extra D+ wake/resume handling before Deep Sleep
             * is safe. Keep tickless idle in Sleep mode so USB enumeration stays
             * responsive.
             */
            if (sleep_ms > sleep_latency)
            {
                result = cyhal_syspm_tickless_sleep(lptimer,
                                                    sleep_ms - sleep_latency,
                                                    &actual_sleep_ms);
            }
            else
            {
                result = CY_RTOS_TIMEOUT;
            }

            if (result == CY_RSLT_SUCCESS)
            {
                CY_ASSERT(actual_sleep_ms <= PD_TICKS_TO_MS(xExpectedIdleTime));
                vTaskStepTick(convert_ms_to_ticks(actual_sleep_ms));
            }
        }

        cyhal_system_critical_section_exit(status);
    }
}
