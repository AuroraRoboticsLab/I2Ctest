/*
 Keyboard-driven I2C test driver:
    Takes commands via serial input
    Prints the resulting data as a test initializer. 

 On an Arduino Uno, hook up GND,  A4 (SDA), A5 (SCL)
 On an Arduino Mega2560, hook up GND, 20 (SDA), 21 (SCL)


Changelog:
  2022-04-25: Created header "I2Ctest.h"
  2022-04-25: Added semicolon after run, for easier copy-paste into code.
  2022-04-22: Initial version

 Dr. Orion Lawlor, lawlor@alaska.edu, 2022-04-22 (Public Domain)
*/
#include "I2Ctest.h"

/* Example I2C transaction to a nonexistent device */
const static I2C_test_brief test_null = {
/*tx*/  {
    /* addr */ 0x10,
    /* n_write */ 1,
    /* n_read */ 1,
    /* write */ { 0x0}
  }
, 
  /*expect*/ { 0xFF}
};

/* Example I2C transaction to boot an MPU-6050 (turn off low power mode) */
const static I2C_transaction_brief boot_MPU = {
  /* addr */ 0x68,
  /* n_write */ 2,
  /* n_read */ 0, 
  /* write */ { 0x6B, 0 }
              /* register PWR_MGMT_1, 0 = power on */
};

/* Example I2C transaction to read accelerometer data from an MPU-6050 IMU */
const static I2C_transaction_brief read_MPU = {
  /* addr */ 0x68,
  /* n_write */ 1,
  /* n_read */ 6,  
  /* write */ { 0x3B }, 
    /* register ACCEL_XOUT_H */
  /* the 6 bytes of returned data are XH/L, YH/L, ZH/L */
};

/************** Code to send an I2C transaction to the hardware **********/

#include <Wire.h> /* Arduino / Energia I2C library */
/* Send this transaction out the I2C bus */
void I2C_transaction_run(const I2C_transaction_brief *tx,int max_read,I2C_byte *read_data)
{
  if (tx->n_write>0) {
    Wire.beginTransmission(tx->addr);
    int limit=sizeof(tx->write);
    if (tx->n_write<limit) limit=tx->n_write;
    for (int i=0;i<limit;i++)
      Wire.write(tx->write[i]);

    bool release=tx->n_read<=0;
    Wire.endTransmission(release);
  }

  if (tx->n_read>0 && max_read>0 && read_data!=0) {
    int limit=max_read;
    if (tx->n_read>limit) limit=tx->n_read;
    Wire.requestFrom((int)tx->addr, limit, (int)true);
    for (int i=0;i<limit;i++)
      read_data[i]=Wire.read();
  }
}

/* Run an I2C test. Returns the number of bytes different than expected, 
   or 0 if everything was as expected. 
   Prints results out to serial port on test failure.
*/
int I2C_test_check(const I2C_test_brief *test)
{
  int diffs=0, first_diff=0;
  int n_read=test->tx.n_read;
  I2C_data_brief actual;
  I2C_transaction_run(&test->tx,sizeof(actual),actual);
  for (int i=0;i<n_read;i++)
    if (actual[i]!=test->expect[i]) {
      diffs++;
      if (diffs==1) first_diff=i; //<- location of first difference
    }
  
  if (diffs>0) {
    Serial.print("I2C test failure.  Transaction = ");
    I2C_transaction_print((const I2C_transaction_generic *)&test->tx);
    
    Serial.print(" expected data "); 
    I2C_byte_print(n_read,test->expect);
    Serial.println();
    
    Serial.print("   actual data ");
    I2C_byte_print(n_read,actual);
    Serial.println();
  }
      
  return diffs;
}

/*********** Serial Output Interface ***********
 A central hack here: the data structures get printed
 like C struct/array initializers, so you can just paste
 them into C code and have it fill out a struct for you.
*/

/* Print an array of I2C byte data formatted like a C array initializer */
void I2C_byte_print(int n_bytes,const I2C_byte *data) 
{
  Serial.print("{ ");
  for (int i=0;i<n_bytes;i++) {
    Serial.print("0x");
    Serial.print((int)data[i],HEX);
    if (i<n_bytes-1) Serial.print(", ");
  }
  Serial.print("}");
}

/* Print an I2C transaction data structure formatted like a C struct initializer */
void I2C_transaction_print(const I2C_transaction_generic *tx)
{
  Serial.print("  {\n");

  Serial.print("    /* addr */ 0x");
  Serial.print(tx->addr,HEX);
  Serial.print(",\n");

  Serial.print("    /* n_write */ ");
  if (tx->n_write>9) Serial.print("0x");
  Serial.print(tx->n_write,HEX);
  Serial.print(",\n");

  Serial.print("    /* n_read */ ");
  if (tx->n_read>9) Serial.print("0x");
  Serial.print(tx->n_read,HEX);
  Serial.print(",\n");

  Serial.print("    /* write */ ");
  I2C_byte_print(tx->n_write,tx->write);
  Serial.print("\n  }\n");
}

/* Run this I2C transaction, and print the result, 
   formatted like an I2C test struct. 
*/
void I2C_transaction_show(const I2C_transaction_brief *tx,int max_read,I2C_byte *read_data)
{
  Serial.print("const static I2C_test_brief test = {\n/*tx*/");
  I2C_transaction_print((const I2C_transaction_generic *)tx);
  I2C_transaction_run(tx,max_read,read_data);
  Serial.print(", \n  /*expect*/ ");
  I2C_byte_print(tx->n_read,read_data);
  Serial.print("\n};\n");
}


/*********** Serial Command Interface *************
 This reads user serial port chars, and builds and runs I2C tests.
*/

/* This I2C transaction is interactively created */
I2C_test_brief dynamic_test={
/*tx*/  {
    /* addr */ 0x10,
    /* n_write */ 1,
    /* n_read */ 1,
    /* write */ { 0x00}
  }
, 
  /*expect*/ { 0x0}
};

/* Run the current I2C transaction, and print the result formatted as a test. */
void runTest()
{
    I2C_transaction_show(&dynamic_test.tx, I2C_limit_brief,dynamic_test.expect);
}

/* Show the current I2C transaction */
void showTx()
{
    I2C_transaction_print((const I2C_transaction_generic *)&dynamic_test.tx);
}

/* Return the numeric value of the i'th word in this string. 
 Words are separated by a single space.
*/
int parseHexInt(const String &cmd,int target_index)
{
  int index=0;
  int value=0;
  int base=16;
  for (unsigned int i=0;i<cmd.length();i++)
  {
    char c=cmd[i];
    if (index==target_index) {
      if (c==' ') return value; // end of this value
      if (c=='x') continue; // skip over 'x' character

      // Convert hex char to single digit value
      int digit=0;
      if (c>='0' && c<='9') digit+=c-'0';
      else if (c>='a' && c<='f') digit+=c-'a'+10;
      else if (c>='A' && c<='F') digit+=c-'A'+10;
      else return value; // some unknown char in middle of hex
      
      value=value*base+digit; // shift in next digit
    }
    if (c==' ') index++;
  }
  return value;
}

/* Read a series of one-byte values, in hex, starting at this word index */
void parseHexBytes(const String &cmd,int target_index, int n,I2C_byte *dest)
{
  // FIXME: this is O(n^2) for n words, bad for long strings
  for (int i=0;i<n;i++)
    dest[i]=parseHexInt(cmd,target_index+i);
}

/* Make sure value is between 0 and limit, with a warning if it's outside. */
I2C_length warn(I2C_length limit,int value)
{
  if (value<0) {
    Serial.println("WARN: Value can't be negative, making it positive.");
    value=0;
  }
  if (value>limit) {
    Serial.print("WARN: Value ");
    Serial.print(value);
    Serial.print(" is too big.  Clamping to ");
    Serial.println((int)limit);
    value=limit;
  }
  return value;
}

/* Print the version and help commands */
void showHelp(const String &cmd)
{
  Serial.println(F(
    "Interactive I2C device testing program: \n"
    "           Version 2022-04-25 \n"
    "           By Alaska Space Systems Engineering Program\n"
    "Commands:\n"
    "    addr 0x68: Set I2C 7-bit device address (in hex)\n"
    "    n_write 2: Set number of bytes sent to device (in hex)\n"
    "    n_read 4: Set number of bytes read from device (in hex)\n"
    "    write 0x0F 0x11: Set the bytes to write to the device (in hex)\n"
    "    show: Show current I2C transaction (as a C struct initializer)\n"
    "    run: Send I2C transaction to hardware, and show results as a test\n"
    "Ready for commands!\n\n"
  ));
}

/* Skip the first n space-separated words and handle that command.
 This is hacky but needed because Arduino IDE serial monitor
 seems to strip out newlines, so you get like "addr 0x12  n_write 3".
*/
void recurseCommand(const String &cmd,int target_index)
{
  int index=0; // word index
  for (unsigned int i=0;i<cmd.length();i++)
  {
    char c=cmd[i];
    if (index==target_index) {
      handleCommand(cmd.substring(i));
      return;
    }
    if (c==' ') index++;
  }
}

/* Run this serial command */
void handleCommand(const String &cmd)
{
  Serial.print("// ");
  Serial.println(cmd);
  int nConsumed=2; // number of words in input command that have been handled
  if (cmd.startsWith("?") || cmd.startsWith("help")) {
    showHelp(cmd);
    nConsumed=1000;
  }
  else if (cmd.startsWith("show")) { /* show test */
    showTx();
    nConsumed=1;
  }
  else if (cmd.startsWith("run")) {  /* run test */
    runTest();
    nConsumed=1;
  }
  else if (cmd.startsWith("addr")) {
    dynamic_test.tx.addr=parseHexInt(cmd,1);    
    showTx();
  }
  else if (cmd.startsWith("n_write")) {
    dynamic_test.tx.n_write=warn(I2C_limit_brief,parseHexInt(cmd,1));
    showTx();
  }
  else if (cmd.startsWith("n_read")) {
    dynamic_test.tx.n_read=warn(I2C_limit_brief,parseHexInt(cmd,1));
    showTx();
  }
  else if (cmd.startsWith("write")) {
    parseHexBytes(cmd,1,dynamic_test.tx.n_write,dynamic_test.tx.write);
    showTx();
    nConsumed=1+dynamic_test.tx.write;
  }
  else {
    Serial.print("\nWARN: Unknown serial command '");
    Serial.print(cmd);
    Serial.print("'\n");
    showHelp(cmd);
  }
  Serial.print("\n");
  recurseCommand(cmd,nConsumed);
}

/* Serial commands get filled into this buffer */
String cmd_buffer;
bool cmd_whitespace=false;
void handleCommandChar(char c)
{
  if (c=='\n') { // newline, check for a valid command
    handleCommand(cmd_buffer); 
    cmd_buffer="";
    cmd_whitespace=false;
  }
  else if (c==' '||c=='\t'||c=='*'||c=='/'||c==','||c=='\r'||c=='{'||c=='}') {
    // ignore whitespace, comments, delimeters, etc
    // (Hacky!  But lets you just paste in a C transaction or test initializer)
    cmd_whitespace=true;
  }
  else if (c!='\r') { // normal non-control char, just add it to the string
    if (cmd_whitespace && cmd_buffer!="") cmd_buffer+=" ";
    cmd_whitespace=false;
    cmd_buffer+=(char)c;
  }
}


void setup() {
  Serial.begin(9600);

  Wire.begin();
#if WIRE_HAS_TIMEOUT
  Wire.setWireTimeout(25000,1);
#endif

  //I2C_test_check(&test_null);

  showHelp("");
}

void loop() {
  while (Serial.available()>0) {
    int c=Serial.read();
    if (c<0) break; // serial read error 
    handleCommandChar(c);
  }  
}
