/* Data structures to represent I2C transactions & tests.

This code can be compiled as either C99 or C++. 

 Dr. Orion Lawlor, lawlor@alaska.edu, 2022-04-25 (Public Domain)
*/
#ifndef __I2CTEST_I2CTEST_H
#define __I2CTEST_I2CTEST_H 1

/* Represents data to write/read via I2C */
typedef unsigned char I2C_byte;
/* Represents a length field (16 bits long) */
typedef int16_t I2C_length;

/* Limit length of "brief" byte arrays.
 (Some limit is needed to avoid variable-size array allocation,
  which is a mess in portable plain C.) */
#define I2C_limit_brief 16

/* Stores a string of bytes */
typedef I2C_byte I2C_data_brief[I2C_limit_brief];

/* Points to a general-purpose I2C bus transaction. */
struct I2C_transaction_generic {
  I2C_byte addr; // I2C 7-bit address of device
  I2C_length n_write; // number of bytes to write to the device
  I2C_length n_read; // number of bytes to read from the device (0 if none)
  I2C_byte write[]; // data to write (variable-size array)
};

/* Stores a brief I2C bus transaction */
struct I2C_transaction_brief {
  I2C_byte addr; // I2C 7-bit address of device
  I2C_length n_write; // number of bytes to write to the device
  I2C_length n_read; // number of bytes to read from the device (0 if none)
  I2C_data_brief write; // data to write (fixed-size array)
};

/* Records a brief I2C test: data to send and receive */
struct I2C_test_brief {
  I2C_transaction_brief tx; // what to write to device
  I2C_data_brief expect; // expected data to receive (fixed-size array)
};

#endif

