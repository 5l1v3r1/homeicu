/*
Functions

The MAX30101 is highly configurable, and there are a large number of functions 
exposed in the library to the user. Checkout the MAX30105.h file for all the 
functions, but here are the major ones. Read the MAX30101 datasheet for more information.

The library supports the following functions:

.begin(wirePort, i2cSpeed) - If you have a platform with multiple I2C ports, 
pass the port object when you call begin. You can increase the I2C speed to 400kHz 
by including I2C_SPEED_FAST when you call .begin() as well.

.setup() - Initializes the sensor with various settings. See the Example 2 from the MAX30105
 hookup guide for a good explanation of the options.

.getRed() - Returns the immediate red value
.getIR() - Returns the immediate IR value
.getGreen() - Returns the immediate Green value
.available() - Returns how many new samples are available
.readTemperature() - Returns the temperature of the IC in C
.softReset() - Resets everything including data and configuration
.shutDown() - Powers down the IC but retains all configuration
.wakeUp() - Opposite of shutDown
.setLEDMode(mode) - Configure the sensor to use 1 (Red only), 2 (Red + IR), or 3 (Red + IR + Green) LEDs
.setADCRange(adcRange) - Set ADC to be at 2048, 4096, 8192, or 16384
.setSampleRate(sampleRate) - Configure the sample rate: 50, 100, 200, 400, 800, 1000, 1600, 3200

Interrupts

.getINT1() - Returns the main interrupt group
.getINT2() - Returns the temp ready interrupt

Enable/disable individual interrupts. See page 13 and 14 of the datasheet for an explanation of each interrupt:
.enableAFULL()
.disableAFULL()
.enableDATARDY()
.disableDATARDY()
.enableALCOVF()
.disableALCOVF()
.enablePROXINT()
.disablePROXINT()
.enableDIETEMPRDY()
.disableDIETEMPRDY()

FIFO

The MAX30101 has a 32 byte FIFO (first-in first-out) buffer. This allows us do other things on our 
microcontroller while the sensor is taking measurements.

.check() - Call regularly to pull data in from sensor
.nextSample() - Advances the FIFO
.getFIFORed() - Returns the FIFO sample pointed to by tail
.getFIFOIR() - Returns the FIFO sample pointed to by tail
.getFIFOGreen() - Returns the FIFO sample pointed to by tail
*/

/***************************************************
  This is a library written for the Maxim MAX3010X Optical Smoke Detector
  It should also work with the MAX30102. However, the MAX30102 does not have a Green LED.

  These sensors use I2C to communicate, as well as a single (optional)
  interrupt line that is not currently supported in this driver.

  Written by Peter Jansen and Nathan Seidle (SparkFun)
  BSD license, all text above must be included in any redistribution.
 *****************************************************/
#include "firmware.h"
#include <Wire.h>
#include "spo2_max3010x.h"

MAX3010X::MAX3010X() {
  // Constructor
}

boolean MAX3010X::begin(TwoWire &wirePort, uint8_t i2c_read_addr, uint8_t i2c_write_addr) {
  // Note by ZWang: disabled speed setting, and make the whole I2C same speed
  _i2cPort = &wirePort; //Grab which port the user wants us to use
  _i2c_read_addr  = i2c_read_addr;
  _i2c_write_addr = i2c_write_addr;

  writeRegister8( i2c_write_addr,  MAX3010X_MODECONFIG, 0x40); //reset

  // Step 1: Initial Communication and Verification
  // Check that a board is connected

  // Device ID and Revision
  if (readRegister8(_i2c_read_addr, MAX3010X_PARTID) != MAX_30105_EXPECTEDPARTID) {
    // Error -- Part ID read from MAX30105 does not match expected part ID.
    // This may mean there is a physical connectivity problem (broken wire, unpowered, etc).
    return false;
  }

  // Populate revision ID
  revisionID = readRegister8(_i2c_read_addr, MAX3010X_REVISIONID);
  Serial.printf("SPO2 Rev: 0x%x\r\n", revisionID);

  return true;
}

//
// Configuration
//

//Begin Interrupt configuration
uint8_t MAX3010X::getINT1(void) {
  return (readRegister8(_i2c_read_addr, MAX3010X_INTSTAT1));
}
uint8_t MAX3010X::getINT2(void) {
  return (readRegister8(_i2c_read_addr, MAX3010X_INTSTAT2));
}

void MAX3010X::enableAFULL(void) {
  bitMask(MAX3010X_INTENABLE1, MAX3010X_INT_A_FULL_MASK, MAX3010X_INT_A_FULL_ENABLE);
}
void MAX3010X::disableAFULL(void) {
  bitMask(MAX3010X_INTENABLE1, MAX3010X_INT_A_FULL_MASK, MAX3010X_INT_A_FULL_DISABLE);
}

void MAX3010X::enableDATARDY(void) {
  bitMask(MAX3010X_INTENABLE1, MAX3010X_INT_DATA_RDY_MASK, MAX3010X_INT_DATA_RDY_ENABLE);
}
void MAX3010X::disableDATARDY(void) {
  bitMask(MAX3010X_INTENABLE1, MAX3010X_INT_DATA_RDY_MASK, MAX3010X_INT_DATA_RDY_DISABLE);
}

void MAX3010X::enableALCOVF(void) {
  bitMask(MAX3010X_INTENABLE1, MAX3010X_INT_ALC_OVF_MASK, MAX3010X_INT_ALC_OVF_ENABLE);
}
void MAX3010X::disableALCOVF(void) {
  bitMask(MAX3010X_INTENABLE1, MAX3010X_INT_ALC_OVF_MASK, MAX3010X_INT_ALC_OVF_DISABLE);
}

void MAX3010X::enablePROXINT(void) {
  bitMask(MAX3010X_INTENABLE1, MAX3010X_INT_PROX_INT_MASK, MAX3010X_INT_PROX_INT_ENABLE);
}
void MAX3010X::disablePROXINT(void) {
  bitMask(MAX3010X_INTENABLE1, MAX3010X_INT_PROX_INT_MASK, MAX3010X_INT_PROX_INT_DISABLE);
}

void MAX3010X::enableDIETEMPRDY(void) {
  bitMask(MAX3010X_INTENABLE2, MAX3010X_INT_DIE_TEMP_RDY_MASK, MAX3010X_INT_DIE_TEMP_RDY_ENABLE);
}
void MAX3010X::disableDIETEMPRDY(void) {
  bitMask(MAX3010X_INTENABLE2, MAX3010X_INT_DIE_TEMP_RDY_MASK, MAX3010X_INT_DIE_TEMP_RDY_DISABLE);
}

//End Interrupt configuration

void MAX3010X::softReset(void) {
  bitMask(MAX3010X_MODECONFIG, MAX3010X_RESET_MASK, MAX3010X_RESET);

  // Poll for bit to clear, reset is then complete
  // Timeout after 100ms
  unsigned long startTime = millis();
  while (millis() - startTime < 100)
  {
    uint8_t response = readRegister8(_i2c_read_addr, MAX3010X_MODECONFIG);
    if ((response & MAX3010X_RESET) == 0) break; //We're done!
    delay(1); //Let's not over burden the I2C bus
  }
}

void MAX3010X::shutDown(void) {
  // Put IC into low power mode (datasheet pg. 19)
  // During shutdown the IC will continue to respond to I2C commands but will
  // not update with or take new readings (such as temperature)
  bitMask(MAX3010X_MODECONFIG, MAX3010X_SHUTDOWN_MASK, MAX3010X_SHUTDOWN);
}

void MAX3010X::wakeUp(void) {
  // Pull IC out of low power mode (datasheet pg. 19)
  bitMask(MAX3010X_MODECONFIG, MAX3010X_SHUTDOWN_MASK, MAX3010X_WAKEUP);
}

void MAX3010X::setLEDMode(uint8_t mode) {
  // Set which LEDs are used for sampling -- Red only, RED+IR only, or custom.
  // See datasheet, page 19
  bitMask(MAX3010X_MODECONFIG, MAX3010X_MODE_MASK, mode);
}

void MAX3010X::setADCRange(uint8_t adcRange) {
  // adcRange: one of MAX3010X_ADCRANGE_2048, _4096, _8192, _16384
  bitMask(MAX3010X_PARTICLECONFIG, MAX3010X_ADCRANGE_MASK, adcRange);
}

void MAX3010X::setSampleRate(uint8_t sampleRate) {
  // sampleRate: one of MAX3010X_SAMPLERATE_50, _100, _200, _400, _800, _1000, _1600, _3200
  bitMask(MAX3010X_PARTICLECONFIG, MAX3010X_SAMPLERATE_MASK, sampleRate);
}

void MAX3010X::setPulseWidth(uint8_t pulseWidth) {
  // pulseWidth: one of MAX3010X_PULSEWIDTH_69, _188, _215, _411
  bitMask(MAX3010X_PARTICLECONFIG, MAX3010X_PULSEWIDTH_MASK, pulseWidth);
}

// NOTE: Amplitude values: 0x00 = 0mA, 0x7F = 25.4mA, 0xFF = 50mA (typical)
// See datasheet, page 21
void MAX3010X::setPulseAmplitudeRed(uint8_t amplitude) {
  writeRegister8(_i2c_write_addr, MAX3010X_LED1_PULSEAMP, amplitude);
}

void MAX3010X::setPulseAmplitudeIR(uint8_t amplitude) {
  writeRegister8(_i2c_write_addr, MAX3010X_LED2_PULSEAMP, amplitude);
}

void MAX3010X::setPulseAmplitudeGreen(uint8_t amplitude) {
  writeRegister8(_i2c_write_addr, MAX3010X_LED3_PULSEAMP, amplitude);
}

void MAX3010X::setPulseAmplitudeProximity(uint8_t amplitude) {
  writeRegister8(_i2c_write_addr, MAX3010X_LED_PROX_AMP, amplitude);
}

void MAX3010X::setProximityThreshold(uint8_t threshMSB) {
  // Set the IR ADC count that will trigger the beginning of particle-sensing mode.
  // The threshMSB signifies only the 8 most significant-bits of the ADC count.
  // See datasheet, page 24.
  writeRegister8(_i2c_write_addr, MAX3010X_PROXINTTHRESH, threshMSB);
}

//Given a slot number assign a thing to it
//Devices are SLOT_RED_LED or SLOT_RED_PILOT (proximity)
//Assigning a SLOT_RED_LED will pulse LED
//Assigning a SLOT_RED_PILOT will ??
void MAX3010X::enableSlot(uint8_t slotNumber, uint8_t device) {

  uint8_t originalContents;

  switch (slotNumber) {
    case (1):
      bitMask(MAX3010X_MULTILEDCONFIG1, MAX3010X_SLOT1_MASK, device);
      break;
    case (2):
      bitMask(MAX3010X_MULTILEDCONFIG1, MAX3010X_SLOT2_MASK, device << 4);
      break;
    case (3):
      bitMask(MAX3010X_MULTILEDCONFIG2, MAX3010X_SLOT3_MASK, device);
      break;
    case (4):
      bitMask(MAX3010X_MULTILEDCONFIG2, MAX3010X_SLOT4_MASK, device << 4);
      break;
    default:
      //Shouldn't be here!
      break;
  }
}

//Clears all slot assignments
void MAX3010X::disableSlots(void) {
  writeRegister8(_i2c_write_addr, MAX3010X_MULTILEDCONFIG1, 0);
  writeRegister8(_i2c_write_addr, MAX3010X_MULTILEDCONFIG2, 0);
}

//
// FIFO Configuration
//

//Set sample average (Table 3, Page 18)
void MAX3010X::setFIFOAverage(uint8_t numberOfSamples) {
  bitMask(MAX3010X_FIFOCONFIG, MAX3010X_SAMPLEAVG_MASK, numberOfSamples);
}

//Resets all points to start in a known state
//Page 15 recommends clearing FIFO before beginning a read
void MAX3010X::clearFIFO(void) {
  writeRegister8(_i2c_write_addr, MAX3010X_FIFOWRITEPTR, 0);
  writeRegister8(_i2c_write_addr, MAX3010X_FIFOOVERFLOW, 0);
  writeRegister8(_i2c_write_addr, MAX3010X_FIFOREADPTR, 0);
}

//Enable roll over if FIFO over flows
void MAX3010X::enableFIFORollover(void) {
  bitMask(MAX3010X_FIFOCONFIG, MAX3010X_ROLLOVER_MASK, MAX3010X_ROLLOVER_ENABLE);
}

//Disable roll over if FIFO over flows
void MAX3010X::disableFIFORollover(void) {
  bitMask(MAX3010X_FIFOCONFIG, MAX3010X_ROLLOVER_MASK, MAX3010X_ROLLOVER_DISABLE);
}

//Set number of samples to trigger the almost full interrupt (Page 18)
//Power on default is 32 samples
//Note it is reverse: 0x00 is 32 samples, 0x0F is 17 samples
void MAX3010X::setFIFOAlmostFull(uint8_t numberOfSamples) {
  bitMask(MAX3010X_FIFOCONFIG, MAX3010X_A_FULL_MASK, numberOfSamples);
}

//Read the FIFO Write Pointer
uint8_t MAX3010X::getWritePointer(void) {
  return (readRegister8(_i2c_read_addr, MAX3010X_FIFOWRITEPTR));
}

//Read the FIFO Read Pointer
uint8_t MAX3010X::getReadPointer(void) {
  return (readRegister8(_i2c_read_addr, MAX3010X_FIFOREADPTR));
}


// Die Temperature
// Returns temp in C
float MAX3010X::readTemperature() {
	
  //DIE_TEMP_RDY interrupt must be enabled
  //See issue 19: https://github.com/sparkfun/SparkFun_MAX3010x_Sensor_Library/issues/19
  
  // Step 1: Config die temperature register to take 1 temperature sample
  writeRegister8(_i2c_write_addr, MAX3010X_DIETEMPCONFIG, 0x01);

  // Poll for bit to clear, reading is then complete
  // Timeout after 100ms
  unsigned long startTime = millis();
  while (millis() - startTime < 100)
  {
    //uint8_t response = readRegister8(_i2c_read_addr, MAX3010X_DIETEMPCONFIG); //Original way
    //if ((response & 0x01) == 0) break; //We're done!
    
	//Check to see if DIE_TEMP_RDY interrupt is set
	uint8_t response = readRegister8(_i2c_read_addr, MAX3010X_INTSTAT2);
    if ((response & MAX3010X_INT_DIE_TEMP_RDY_ENABLE) > 0) break; //We're done!
    delay(1); //Let's not over burden the I2C bus
  }

  // Step 2: Read die temperature register (integer)
  int8_t tempInt = readRegister8(_i2c_read_addr, MAX3010X_DIETEMPINT);
  uint8_t tempFrac = readRegister8(_i2c_read_addr, MAX3010X_DIETEMPFRAC); //Causes the clearing of the DIE_TEMP_RDY interrupt

  // Step 3: Calculate temperature (datasheet pg. 23)
  return (float)tempInt + ((float)tempFrac * 0.0625);
}


// Set the PROX_INT_THRESHold
void MAX3010X::setPROXINTTHRESH(uint8_t val) {
  writeRegister8(_i2c_write_addr, MAX3010X_PROXINTTHRESH, val);
}




//Setup the sensor
//The MAX3010X has many settings. By default we select:
// Sample Average = 4
// Mode = MultiLED
// ADC Range = 16384 (62.5pA per LSB)
// Sample rate = 50
//Use the default setup if you are just getting started with the MAX3010X sensor
void MAX3010X::setup(byte powerLevel, byte sampleAverage, byte ledMode, int sampleRate, int pulseWidth, int adcRange) {
  softReset(); //Reset all configuration, threshold, and data registers to POR values

  //FIFO Configuration
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
  //The chip will average multiple samples of same type together if you wish
  if (sampleAverage == 1) setFIFOAverage(MAX3010X_SAMPLEAVG_1); //No averaging per FIFO record
  else if (sampleAverage == 2) setFIFOAverage(MAX3010X_SAMPLEAVG_2);
  else if (sampleAverage == 4) setFIFOAverage(MAX3010X_SAMPLEAVG_4);
  else if (sampleAverage == 8) setFIFOAverage(MAX3010X_SAMPLEAVG_8);
  else if (sampleAverage == 16) setFIFOAverage(MAX3010X_SAMPLEAVG_16);
  else if (sampleAverage == 32) setFIFOAverage(MAX3010X_SAMPLEAVG_32);
  else setFIFOAverage(MAX3010X_SAMPLEAVG_4);

  //setFIFOAlmostFull(2); //Set to 30 samples to trigger an 'Almost Full' interrupt
  enableFIFORollover(); //Allow FIFO to wrap/roll over
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

  //Mode Configuration
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
  if (ledMode == 3) setLEDMode(MAX3010X_MODE_MULTILED); //Watch all three LED channels
  else if (ledMode == 2) setLEDMode(MAX3010X_MODE_REDIRONLY); //Red and IR
  else setLEDMode(MAX3010X_MODE_REDONLY); //Red only
  activeLEDs = ledMode; //Used to control how many bytes to read from FIFO buffer
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

  //Particle Sensing Configuration
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
  if(adcRange < 4096) setADCRange(MAX3010X_ADCRANGE_2048); //7.81pA per LSB
  else if(adcRange < 8192) setADCRange(MAX3010X_ADCRANGE_4096); //15.63pA per LSB
  else if(adcRange < 16384) setADCRange(MAX3010X_ADCRANGE_8192); //31.25pA per LSB
  else if(adcRange == 16384) setADCRange(MAX3010X_ADCRANGE_16384); //62.5pA per LSB
  else setADCRange(MAX3010X_ADCRANGE_2048);

  if (sampleRate < 100) setSampleRate(MAX3010X_SAMPLERATE_50); //Take 50 samples per second
  else if (sampleRate < 200) setSampleRate(MAX3010X_SAMPLERATE_100);
  else if (sampleRate < 400) setSampleRate(MAX3010X_SAMPLERATE_200);
  else if (sampleRate < 800) setSampleRate(MAX3010X_SAMPLERATE_400);
  else if (sampleRate < 1000) setSampleRate(MAX3010X_SAMPLERATE_800);
  else if (sampleRate < 1600) setSampleRate(MAX3010X_SAMPLERATE_1000);
  else if (sampleRate < 3200) setSampleRate(MAX3010X_SAMPLERATE_1600);
  else if (sampleRate == 3200) setSampleRate(MAX3010X_SAMPLERATE_3200);
  else setSampleRate(MAX3010X_SAMPLERATE_50);

  //The longer the pulse width the longer range of detection you'll have
  //At 69us and 0.4mA it's about 2 inches
  //At 411us and 0.4mA it's about 6 inches
  if (pulseWidth < 118) setPulseWidth(MAX3010X_PULSEWIDTH_69); //Page 26, Gets us 15 bit resolution
  else if (pulseWidth < 215) setPulseWidth(MAX3010X_PULSEWIDTH_118); //16 bit resolution
  else if (pulseWidth < 411) setPulseWidth(MAX3010X_PULSEWIDTH_215); //17 bit resolution
  else if (pulseWidth == 411) setPulseWidth(MAX3010X_PULSEWIDTH_411); //18 bit resolution
  else setPulseWidth(MAX3010X_PULSEWIDTH_69);
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

  //LED Pulse Amplitude Configuration
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
  //Default is 0x1F which gets us 6.4mA
  //powerLevel = 0x02, 0.4mA - Presence detection of ~4 inch
  //powerLevel = 0x1F, 6.4mA - Presence detection of ~8 inch
  //powerLevel = 0x7F, 25.4mA - Presence detection of ~8 inch
  //powerLevel = 0xFF, 50.0mA - Presence detection of ~12 inch

  setPulseAmplitudeRed(powerLevel);
  setPulseAmplitudeIR(powerLevel);
  setPulseAmplitudeGreen(powerLevel);
  setPulseAmplitudeProximity(powerLevel);

  //Multi-LED Mode Configuration, Enable the reading of the three LEDs
  enableSlot(1, SLOT_RED_LED);
  if (ledMode > 1) enableSlot(2, SLOT_IR_LED);
  if (ledMode > 2) enableSlot(3, SLOT_GREEN_LED);

  clearFIFO(); //Reset the FIFO before we begin checking the sensor
}

//
// Data Collection
//

//Tell caller how many samples are available
uint8_t MAX3010X::available(void)
{
  int8_t numberOfSamples = sense.head - sense.tail;
  if (numberOfSamples < 0) numberOfSamples += STORAGE_SIZE;

  return (numberOfSamples);
}

//Report the most recent red value
uint32_t MAX3010X::getRed(void)
{
  //Check the sensor for new data for 250ms
  if(safeCheck(250))
    return (sense.red[sense.head]);
  else
    return(0); //Sensor failed to find new data
}

//Report the most recent IR value
uint32_t MAX3010X::getIR(void)
{
  //Check the sensor for new data for 250ms
  if(safeCheck(250))
    return (sense.IR[sense.head]);
  else
    return(0); //Sensor failed to find new data
}

//Report the most recent Green value
uint32_t MAX3010X::getGreen(void)
{
  //Check the sensor for new data for 250ms
  if(safeCheck(250))
    return (sense.green[sense.head]);
  else
    return(0); //Sensor failed to find new data
}

//Report the next Red value in the FIFO
uint32_t MAX3010X::getFIFORed(void)
{
  return (sense.red[sense.tail]);
}

//Report the next IR value in the FIFO
uint32_t MAX3010X::getFIFOIR(void)
{
  return (sense.IR[sense.tail]);
}

//Report the next Green value in the FIFO
uint32_t MAX3010X::getFIFOGreen(void)
{
  return (sense.green[sense.tail]);
}

//Advance the tail
void MAX3010X::nextSample(void)
{
  if(available()) //Only advance the tail if new data is available
  {
    sense.tail++;
    sense.tail %= STORAGE_SIZE; //Wrap condition
  }
}

//Polls the sensor for new data
//Call regularly
//If new data is available, it updates the head and tail in the main struct
//Returns number of new samples obtained
uint16_t MAX3010X::check(void)
{
  //Read register FIDO_DATA in (3-byte * number of active LED) chunks
  //Until FIFO_RD_PTR = FIFO_WR_PTR

  byte readPointer = getReadPointer();
  byte writePointer = getWritePointer();

  int numberOfSamples = 0;

  //Do we have new data?
  if (readPointer != writePointer)
  {
    //Calculate the number of readings we need to get from sensor
    numberOfSamples = writePointer - readPointer;
    if (numberOfSamples < 0) numberOfSamples += 32; //Wrap condition

    //We now have the number of readings, now calc bytes to read
    //For this example we are just doing Red and IR (3 bytes each)
    int bytesLeftToRead = numberOfSamples * activeLEDs * 3;

    //Get ready to read a burst of data from the FIFO register
    _i2cPort->beginTransmission(_i2c_write_addr);
    _i2cPort->write(MAX3010X_FIFODATA);
    _i2cPort->endTransmission();

    //We may need to read as many as 288 bytes so we read in blocks no larger than I2C_BUFFER_LENGTH
    //I2C_BUFFER_LENGTH changes based on the platform. 64 bytes for SAMD21, 32 bytes for Uno.
    //Wire.requestFrom() is limited to BUFFER_LENGTH which is 32 on the Uno
    while (bytesLeftToRead > 0)
    {
      uint8_t toGet = bytesLeftToRead;
      if (toGet > I2C_BUFFER_LENGTH)
      {
        //If toGet is 32 this is bad because we read 6 bytes (Red+IR * 3 = 6) at a time
        //32 % 6 = 2 left over. We don't want to request 32 bytes, we want to request 30.
        //32 % 9 (Red+IR+GREEN) = 5 left over. We want to request 27.

        toGet = I2C_BUFFER_LENGTH - (I2C_BUFFER_LENGTH % (activeLEDs * 3)); //Trim toGet to be a multiple of the samples we need to read
      }

      bytesLeftToRead -= toGet;

      //Request toGet number of bytes from sensor
      _i2cPort->requestFrom(_i2c_read_addr, toGet);
      
      while (toGet > 0)
      {
        sense.head++; //Advance the head of the storage struct
        sense.head %= STORAGE_SIZE; //Wrap condition

        byte temp[sizeof(uint32_t)]; //Array of 4 bytes that we will convert into long
        uint32_t tempLong;

        //Burst read three bytes - RED
        temp[3] = 0;
        temp[2] = _i2cPort->read();
        temp[1] = _i2cPort->read();
        temp[0] = _i2cPort->read();

        //Convert array to long
        memcpy(&tempLong, temp, sizeof(tempLong));
		
		    tempLong &= 0x3FFFF; //Zero out all but 18 bits

        sense.red[sense.head] = tempLong; //Store this reading into the sense array

        if (activeLEDs > 1)
        {
          //Burst read three more bytes - IR
          temp[3] = 0;
          temp[2] = _i2cPort->read();
          temp[1] = _i2cPort->read();
          temp[0] = _i2cPort->read();

          //Convert array to long
          memcpy(&tempLong, temp, sizeof(tempLong));

          tempLong &= 0x3FFFF; //Zero out all but 18 bits
              
          sense.IR[sense.head] = tempLong;
        }

        if (activeLEDs > 2)
        {
          //Burst read three more bytes - Green
          temp[3] = 0;
          temp[2] = _i2cPort->read();
          temp[1] = _i2cPort->read();
          temp[0] = _i2cPort->read();

          //Convert array to long
          memcpy(&tempLong, temp, sizeof(tempLong));

		      tempLong &= 0x3FFFF; //Zero out all but 18 bits

          sense.green[sense.head] = tempLong;
        }

        toGet -= activeLEDs * 3;
      }

    } //End while (bytesLeftToRead > 0)

  } //End readPtr != writePtr

  return (numberOfSamples); //Let the world know how much new data we found
}

//Check for new data but give up after a certain amount of time
//Returns true if new data was found
//Returns false if new data was not found
bool MAX3010X::safeCheck(uint8_t maxTimeToCheck)
{
  uint32_t markTime = millis();
  
  while(1)
  {
	if(millis() - markTime > maxTimeToCheck) return(false);

	if(check() == true) //We found new data!
	  return(true);

	delay(1);
  }
}

//Given a register, read it, mask it, and then set the thing
void MAX3010X::bitMask(uint8_t reg, uint8_t mask, uint8_t thing)
{
  // Grab current register context
  uint8_t originalContents = readRegister8(_i2c_read_addr, reg);

  // Zero-out the portions of the register we're interested in
  originalContents = originalContents & mask;

  // Change contents
  writeRegister8(_i2c_write_addr, reg, originalContents | thing);
}

//
// Low-level I2C Communication
//
uint8_t MAX3010X::readRegister8(uint8_t address, uint8_t reg) {
  _i2cPort->beginTransmission(address);
  _i2cPort->write(reg);
  _i2cPort->endTransmission(false);

  _i2cPort->requestFrom((uint8_t)address, (uint8_t)1); // Request 1 byte
  if (_i2cPort->available())
  {
    return(_i2cPort->read());
  }

  return (0); //Fail

}

void MAX3010X::writeRegister8(uint8_t address, uint8_t reg, uint8_t value) {
  _i2cPort->beginTransmission(address);
  _i2cPort->write(reg);
  _i2cPort->write(value);
  _i2cPort->endTransmission();
}
