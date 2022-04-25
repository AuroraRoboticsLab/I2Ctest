# I2Ctest
This is a tool for interactively creating I2C (Inter-IC Protocol, also known as Two-Wire Interface) transactions, and storing the transactions and expected data as tests.  To use the tool, open I2Ctest/I2Ctest.ino in the Arduino or Energia IDE and flash a microcontroller, then wire the microcontroller to your I2C device. 

Here are the commands this tool accepts interactively via the serial port:
```
Commands:
    addr 0x68: Set I2C 7-bit device address (in hex)
    n_write 2: Set number of bytes sent to device (in hex)
    n_read 4: Set number of bytes read from device (in hex)
    write 0x0F 0x11: Set the bytes to write to the device (in hex)
    show: Show current I2C transaction (as a C struct initializer)
    run: Send I2C transaction to hardware, and show results as a test
```

Here's a typical test result:

```
const static I2C_test_brief MPU6050 = {
/*tx*/  {
    /* addr */ 0x68,
    /* n_write */ 1,
    /* n_read */ 6,
    /* write */ { 0x3B}
  }
,
  /*expect*/ { 0x3, 0x74, 0x1B, 0x6C, 0x3A, 0x14}
};
```

This tool is designed to support the interactive setup, configuration, bring-up testing, acceptance and integration testing, and even operational use of I2C devices. 




