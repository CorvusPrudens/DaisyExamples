#include "daisy_seed.h"

using namespace daisy;

static DaisySeed seed;
static I2CHandle i2c;
static I2CHandle::Config i2c_conf;

// If the tests are successful, each daisy will blink. If the
// tests fail, then one or both won't blink.
// No need to worry about initial syncing -- they'll attempt to
// communicate until a successful transaction is made.
// define either MASTER or SLAVE to set up each side.

// #define MASTER
#define SLAVE

#define SLAVE_ADDR 60

#define BLOCKING_TEST 0
#define DMA_TEST 1
#define DMA_AWAIT 2
#define SUCCESS 3

static int  test_state = BLOCKING_TEST;
static bool succeeded  = false;

#define LEN_BLOCK 4
static uint8_t i2cDataBlock[] = {
    1,
    3,
    3,
    7,
};
static uint8_t i2cDataBlockRec[LEN_BLOCK];

#define LEN_DMA 16
static uint8_t i2cDataDmaTemplate[] = {
    60, 64, 67, 71,
    60, 64, 67, 70,
    60, 63, 67, 70,
    60, 63, 66, 70,
};
static uint8_t DMA_BUFFER_MEM_SECTION i2cDataDma[LEN_DMA];

void blink()
{
    for(int i = 0; i < 5; i++)
    {
        seed.SetLed(true);
        seed.DelayMs(100);
        seed.SetLed(false);
        seed.DelayMs(100);
    }
}

void i2cCallback(void* context, I2CHandle::Result result)
{
    if(result == I2CHandle::Result::OK)
    {
        *(bool*)context = true;
    }
}

void master_test()
{
    switch(test_state)
    {
        case BLOCKING_TEST:
        {
            for(int i = 0; i < LEN_DMA; i++)
            {
                i2cDataDma[i] = i2cDataDmaTemplate[i];
            }

            // Full configuration for clarity
            i2c_conf.periph         = I2CHandle::Config::Peripheral::I2C_1;
            i2c_conf.speed          = I2CHandle::Config::Speed::I2C_100KHZ;
            i2c_conf.pin_config.scl = seed.GetPin(11);
            i2c_conf.pin_config.sda = seed.GetPin(12);
            i2c_conf.mode = I2CHandle::Config::Mode::I2C_MASTER;
            if(i2c.Init(i2c_conf) == I2CHandle::Result::ERR)
                while(1) {}

            // blocking test
            while(
                i2c.TransmitBlocking(SLAVE_ADDR, i2cDataBlock, LEN_BLOCK, 1000)
                == I2CHandle::Result::ERR);
            test_state = DMA_TEST;
        }
        break;

        case DMA_TEST:
        {
            seed.DelayMs(0.1);
            // dma test
            I2CHandle::CallbackFunctionPtr callback = i2cCallback;
            if(i2c.TransmitDma(
                   SLAVE_ADDR, i2cDataDma, LEN_DMA, callback, &succeeded)
               == I2CHandle::Result::ERR)
                while(1) {}
            test_state = DMA_AWAIT;
        }
        break;

        case DMA_AWAIT:
            if(succeeded)
                test_state = SUCCESS;
            break;

        case SUCCESS:
            blink();
            while(1) {}
    }
}

void slave_test()
{
    switch(test_state)
    {
        case BLOCKING_TEST:
        {
            // Setting to known value
            for(int i = 0; i < LEN_DMA; i++)
            {
                i2cDataDma[i] = 0;
            }

            i2c_conf.periph         = I2CHandle::Config::Peripheral::I2C_1;
            i2c_conf.speed          = I2CHandle::Config::Speed::I2C_100KHZ;
            i2c_conf.pin_config.scl = seed.GetPin(11);
            i2c_conf.pin_config.sda = seed.GetPin(12);
            i2c_conf.address = SLAVE_ADDR;
            i2c_conf.mode    = I2CHandle::Config::Mode::I2C_SLAVE;
            if(i2c.Init(i2c_conf) == I2CHandle::Result::ERR)
                while(1) {}

            while(i2c.ReceiveBlocking(0, i2cDataBlockRec, LEN_BLOCK, 1000)
                  == I2CHandle::Result::ERR);

            test_state = DMA_TEST;
        }
        break;

        case DMA_TEST:
        {
            I2CHandle::CallbackFunctionPtr callback = i2cCallback;
            I2CHandle::Result              result
                = i2c.ReceiveDma(0, i2cDataDma, LEN_DMA, callback, &succeeded);
            if(result == I2CHandle::Result::ERR)
                while(1) {}

            test_state = DMA_AWAIT;
        }
        break;

        case DMA_AWAIT:
            if(succeeded)
            {
                for(size_t i = 0; i < LEN_BLOCK; i++)
                {
                    if(i2cDataBlockRec[i] != i2cDataBlock[i])
                        while(1) {}
                }
                for(int i = 0; i < LEN_DMA; i++)
                {
                    if(i2cDataDma[i] != i2cDataDmaTemplate[i])
                        while(1) {}
                }
                test_state = SUCCESS;
            }
            break;

        case SUCCESS:
            blink();
            while(1) {}
    }
}

int main(void)
{
    seed.Configure();
    seed.Init();

    while (1)
    {
        #ifdef MASTER
            master_test();
        #else
            slave_test();
        #endif
    }
}