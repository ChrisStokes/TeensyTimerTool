#pragma once

#include "TMRChainChannel.h"
#include "TMRChannel.h"
#include "imxrt.h"

namespace TeensyTimerTool
{
    template <unsigned moduleNr>
    class TMR_t
    {
     public:
        static ITimerChannel* getTimer16(); // single channel
        static ITimerChannel* getTimer32(); // 2 channels chained 32bit
        static ITimerChannel* getTimer64(); // 4 channes chained  64bit

     protected:
        static bool isInitialized;
        static void initialize();
        static void isr();
        static callback_t callbacks[4];

        // the following is calculated at compile time
        static constexpr IRQ_NUMBER_t irq = moduleNr == 0 ? IRQ_QTIMER1 : moduleNr == 1 ? IRQ_QTIMER2 : moduleNr == 2 ? IRQ_QTIMER3 : IRQ_QTIMER4;
        static IMXRT_TMR_t* const pTMR;
        static IMXRT_TMR_CH_t* const pCH0;
        static IMXRT_TMR_CH_t* const pCH1;
        static IMXRT_TMR_CH_t* const pCH2;
        static IMXRT_TMR_CH_t* const pCH3;

        static_assert(moduleNr < 4, "Module number < 4 required");
    };

    // IMPLEMENTATION ==================================================================

    template <unsigned moduleNr>
    IMXRT_TMR_t* const TMR_t<moduleNr>::pTMR = moduleNr == 0 ? &IMXRT_TMR1 : moduleNr == 1 ? &IMXRT_TMR2 : moduleNr == 2 ? &IMXRT_TMR3 : &IMXRT_TMR4;
    template <unsigned moduleNr>
    IMXRT_TMR_CH_t* const TMR_t<moduleNr>::pCH0 = &pTMR->CH[0];
    template <unsigned moduleNr>
    IMXRT_TMR_CH_t* const TMR_t<moduleNr>::pCH1 = &pTMR->CH[1];
    template <unsigned moduleNr>
    IMXRT_TMR_CH_t* const TMR_t<moduleNr>::pCH2 = &pTMR->CH[2];
    template <unsigned moduleNr>
    IMXRT_TMR_CH_t* const TMR_t<moduleNr>::pCH3 = &pTMR->CH[3];

    template <unsigned moduleNr>
    void TMR_t<moduleNr>::initialize()
    {
        if (!isInitialized)
        {
            for (unsigned chNr = 0; chNr < 4; chNr++)
            {
                pTMR->CH[chNr].CTRL = 0x0000;
                callbacks[chNr] = nullptr;
            }
            attachInterruptVector(irq, isr); // start
            NVIC_ENABLE_IRQ(irq);
            isInitialized = true;
        }
    }

    template <unsigned moduleNr>
    ITimerChannel* TMR_t<moduleNr>::getTimer16()
    {
        initialize();

        for (unsigned chNr = 0; chNr < 4; chNr++)
        {
            IMXRT_TMR_CH_t* pCh = &pTMR->CH[chNr];
            if (pCh->CTRL == 0x0000)
            {
                return new TMRChannel(pCh, &callbacks[chNr]);
            }
        }
        return nullptr; // no free channel
    }

    template <unsigned moduleNr>
    ITimerChannel* TMR_t<moduleNr>::getTimer32()
    {
        initialize();
        return new TMRChainChannel(&callbacks[0], pTMR, 0,1);
        //return nullptr;

        for (unsigned i = 0; i < 2; i++)
        {
            IMXRT_TMR_CH_t* pCh = &pTMR->CH[i * 2];
            if (pCh->CTRL == 0x0000 && (pCh + 1)->CTRL == 0x0000)
            {
                return new TMRChannel(pCh, &callbacks[i * 2]);
            }
        }

        return nullptr; // no free channel
    }

    template <unsigned moduleNr>
    ITimerChannel* TMR_t<moduleNr>::getTimer64()
    {
        return nullptr;
    }

    template <unsigned m>
    void TMR_t<m>::isr()
    {
        // no loop to gain some time by avoiding indirections and pointer calculations
        if (callbacks[0] != nullptr && pCH0->CSCTRL & TMR_CSCTRL_TCF1)
        {
            pCH0->CSCTRL &= ~TMR_CSCTRL_TCF1;
            callbacks[0]();
        }

        if (callbacks[1] != nullptr && pCH1->CSCTRL & TMR_CSCTRL_TCF1)
        {
            pCH1->CSCTRL &= ~TMR_CSCTRL_TCF1;
            callbacks[1]();
        }

        if (callbacks[2] != nullptr && pCH2->CSCTRL & TMR_CSCTRL_TCF1)
        {
            pCH2->CSCTRL &= ~TMR_CSCTRL_TCF1;
            callbacks[2]();
        }

        if (callbacks[3] != nullptr && pCH3->CSCTRL & TMR_CSCTRL_TCF1)
        {
            pCH3->CSCTRL &= ~TMR_CSCTRL_TCF1;
            callbacks[3]();
        }
        asm volatile("dsb"); //wait until register changes propagated through the cache
    }

    template <unsigned m>
    bool TMR_t<m>::isInitialized = false;

    template <unsigned m>
    callback_t TMR_t<m>::callbacks[4];
}
