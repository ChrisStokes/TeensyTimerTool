#pragma once
//#include "../../ITimerChannel.h"
#include "Arduino.h"
#include "TMRChannel.h"
//#include "ErrorHandling/error_codes.h"

//#include "config.h"
//#include "imxrt.h"
#include <array>
#include <vector>

namespace TeensyTimerTool
{

    class TMRChainChannel : public TMRChannel
    {
     public:
        inline TMRChainChannel(IMXRT_TMR_CH_t* regs, callback_t* cbStorage);
        //  inline TMRChainChannel(callback_t* cbStorage,...);
        inline TMRChainChannel(callback_t* cbStorage, IMXRT_TMR_t* ch0, unsigned c1);
        inline TMRChainChannel(callback_t* cbStorage, IMXRT_TMR_t* ch0, unsigned c1, unsigned c2);
        inline TMRChainChannel(callback_t* cbStorage, IMXRT_TMR_t* ch0, unsigned c1, unsigned c2, unsigned c3);
        inline TMRChainChannel(callback_t* cbStorage, IMXRT_TMR_t* ch0, unsigned c1, unsigned c2, unsigned c3, unsigned c4);

        inline virtual ~TMRChainChannel();

        inline errorCode begin(callback_t cb, uint32_t tcnt, bool periodic) override;
        inline errorCode begin(callback_t cb, float tcnt, bool periodic) override;
        inline errorCode start() override;
        inline errorCode stop() override;

        inline errorCode trigger(uint32_t tcnt) override;
        inline errorCode trigger(float tcnt) override;

        inline float getMaxPeriod() override;

        inline errorCode setPeriod(uint32_t us) override;
        inline errorCode setCurrentPeriod(uint32_t us) override;
        inline errorCode setNextPeriod(uint32_t us) override;
        //inline void setPrescaler(uint32_t psc); // psc 0..7 -> prescaler: 1..128

     protected:
        IMXRT_TMR_CH_t* regs;

        IMXRT_TMR_t* module;
        unsigned channel[4];
        unsigned nrOfChannels;

        callback_t** pCallback = nullptr;
        // float pscValue;
        // uint32_t pscBits;

        inline float_t microsecondToCounter(const float_t us) const;
        inline float_t counterToMicrosecond(const float_t cnt) const;

        inline errorCode _setCurrentPeriod(const uint16_t cnt);
        inline void _setNextPeriod(const uint16_t cnt);
    };

    // IMPLEMENTATION ==============================================

    TMRChainChannel::TMRChainChannel(IMXRT_TMR_CH_t* regs, callback_t* cbStorage)
        : TMRChannel(regs, cbStorage)
    {
        this->regs = regs;
        setPrescaler(TMR_DEFAULT_PSC);
    }

    TMRChainChannel::TMRChainChannel(callback_t* cbStorage, IMXRT_TMR_t* module, unsigned c0)
        : TMRChannel(nullptr, cbStorage+1)
    {
        this->module = module;
        this->nrOfChannels = 1;
        setPrescaler(TMR_DEFAULT_PSC);
        channel[0] = c0;
    }

    TMRChainChannel::TMRChainChannel(callback_t* cbStorage, IMXRT_TMR_t* module, unsigned c0, unsigned c1)
        : TMRChainChannel(cbStorage, module, c0)
    {
        nrOfChannels = 2;
        channel[1] = c1;
    }

    TMRChainChannel::TMRChainChannel(callback_t* cbStorage, IMXRT_TMR_t* module, unsigned c0, unsigned c1, unsigned c2)
        : TMRChainChannel(cbStorage, module, c0, c1)
    {
        nrOfChannels = 3;
        channel[2] = c2;
    }

    TMRChainChannel::TMRChainChannel(callback_t* cbStorage, IMXRT_TMR_t* module, unsigned c0, unsigned c1, unsigned c2, unsigned c3)
        : TMRChainChannel(cbStorage, module, c0, c1, c2)
    {
        nrOfChannels = 4;
        channel[3] = c3;
    }

    TMRChainChannel::~TMRChainChannel()
    {
    }

    errorCode TMRChainChannel::stop()
    {
        regs->CSCTRL &= ~TMR_CSCTRL_TCF1EN;
        return errorCode::OK;
    }

    errorCode TMRChainChannel::begin(callback_t cb, uint32_t tcnt, bool periodic)
    {
        return begin(cb, (float)tcnt, periodic);
    }

    errorCode TMRChainChannel::begin(callback_t cb, float tcnt, bool periodic)
    {
        //const float_t t = microsecondToCounter(tcnt);
        // uint16_t reload;
        // if (t > 0xFFFF)
        // {
        //     postError(errorCode::periodOverflow);
        //     reload = 0xFFFE;
        // } else
        // {
        //     reload = (uint16_t)t - 1;
        // }

        // base channel

        Serial.printf("nr: %d\n", nrOfChannels);

        IMXRT_TMR_CH_t* baseCh = &(module->CH[channel[0]]);

        baseCh->CTRL = 0x0000;
        baseCh->LOAD = 0x0000;
        baseCh->COMP1 = 0xFFFF;
        baseCh->CMPLD1 = 0xFFFF;

        for (unsigned i = 1; i < nrOfChannels; i++)
        {
            IMXRT_TMR_CH_t* ch = &(module->CH[channel[i]]);
            ch->CTRL = 0x0000;
            ch->LOAD = 0x0000;
            ch->COMP1 = 0x0010;
            ch->CMPLD1 = 0x0010;

            unsigned cntrOut = 0b0100 + channel[i-1]; // connect channel to output of previous channel
            ch->CTRL = TMR_CTRL_CM(0b111) | TMR_CTRL_PCS(cntrOut) | TMR_CTRL_LENGTH;

            Serial.println(cntrOut);
        }


        setCallback(cb);

        // if (!periodic)
        //     regs->CTRL = TMR_CTRL_CM(1) | TMR_CTRL_PCS(pscBits) | TMR_CTRL_ONCE | TMR_CTRL_LENGTH;

        // else

        baseCh->CTRL = TMR_CTRL_CM(1) | TMR_CTRL_PCS(pscBits) | TMR_CTRL_LENGTH;

        //Serial.printf("base %X %X %p\n",pscBits, 0b1111, baseCh);
        //start();

        //regs->CTRL = TMR_CTRL_CM(0b111) | TMR_CTRL_PCS(0b0101) | TMR_CTRL_LENGTH;

        return errorCode::OK;

        //return t > 0xFFFF ? errorCode::periodOverflow : errorCode::OK;
    }

    errorCode TMRChainChannel::start()
    {
        IMXRT_TMR_CH_t* ch = nullptr;

        for (unsigned i = 0; i < nrOfChannels; i++)
        {
            ch = &(module->CH[channel[i]]);
            ch->CNTR = 0x0000;
            ch->CSCTRL &= ~TMR_CSCTRL_TCF1;
            Serial.printf("start %p\n", ch);
        }
        ch->CSCTRL |= TMR_CSCTRL_TCF1EN; // Enable interrupt for last channel

        return errorCode::OK;
    }

    errorCode TMRChainChannel::trigger(uint32_t tcnt)
    {
        return trigger((float)tcnt);
    }

    errorCode TMRChainChannel::trigger(float tcnt) // quick and dirty, should be optimized
    {
        const float_t t = microsecondToCounter(tcnt);
        uint16_t reload = t > 0xFFFF ? 0xFFFF : (uint16_t)t;

        regs->CTRL = 0x0000;
        regs->LOAD = 0x0000;
        regs->COMP1 = reload;
        regs->CMPLD1 = reload;
        regs->CNTR = 0x0000;

        regs->CSCTRL &= ~TMR_CSCTRL_TCF1;
        regs->CSCTRL |= TMR_CSCTRL_TCF1EN;

        regs->CTRL = TMR_CTRL_CM(1) | TMR_CTRL_PCS(pscBits) | TMR_CTRL_ONCE | TMR_CTRL_LENGTH;

        return errorCode::OK;
    }

    // void TMRChainChannel::setPrescaler(uint32_t psc) // psc 0..7 -> prescaler: 1..128
    // {
    //     pscValue = 1 << (psc & 0b0111);
    //     pscBits = 0b1000 | (psc & 0b0111);
    // }

    float TMRChainChannel::getMaxPeriod()
    {
        return pscValue / 150.0f * 0xFFFE;
    }

    void TMRChainChannel::_setNextPeriod(const uint16_t cnt)
    {
        regs->CMPLD1 = cnt;
    }

    errorCode TMRChainChannel::_setCurrentPeriod(const uint16_t cnt)
    {

        regs->COMP1 = cnt;

        //Do we need to wait some cycle for IP bus to update here / cache flush?
        //asm volatile("dsb");

        if (regs->CNTR > cnt)
        {
            //if counter alrready went over setted value force a triggering
            regs->CNTR = cnt;
            return errorCode::triggeredLate;
        }

        else
        {
            return errorCode::OK;
        }
    }

    errorCode TMRChainChannel::setCurrentPeriod(uint32_t us)
    {
        const float_t t = microsecondToCounter(us);

        if (t <= 0xFFFF)
        {
            return _setCurrentPeriod(t);
        } else
        {
            return errorCode::periodOverflow;
        }
    }

    errorCode TMRChainChannel::setNextPeriod(uint32_t us)
    {
        const float_t t = microsecondToCounter(us);

        if (t <= 0xFFFF)
        {
            _setNextPeriod(t);
            return errorCode::OK;
        } else
        {
            return errorCode::periodOverflow;
        }
    }

    errorCode TMRChainChannel::setPeriod(uint32_t us)
    {
        const float_t t = microsecondToCounter(us);

        if (t <= 0xFFFF)
        {
            _setNextPeriod(t);
            return _setCurrentPeriod(t);

        } else
        {
            return errorCode::periodOverflow;
        }
    }

    float_t TMRChainChannel::microsecondToCounter(const float_t us) const
    {
        return us * 150.0f / pscValue;
    }

    float_t TMRChainChannel::counterToMicrosecond(const float_t cnt) const
    {
        return cnt * pscValue / 150.0f;
    }

} // namespace TeensyTimerTool
