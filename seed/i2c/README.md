# i2c

## Author

Gabriel Ball

## Description

Test bench for i2c slave and master with blocking and dma transfers. To make testing simple, tie the digital 3.3v lines of both daisies together and power one up to run the test. A successful test will result in both boards blinking 5 times. A failure has occurred if one or both don't blink. This test uses the I2C_1 peripheral. Just switch the MASTER / SLAVE define to flash each one. 

Don't forget to add pull ups!
