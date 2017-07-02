// #############################################################################
// ### This file is part of the source code for the TwinLights Mood Lamp     ###
// ### https://github.com/Strooom/TwinLights                                 ###
// ### Author(s) : Pascal Roobrouck - @strooom                               ###
// ### License : https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode ###
// #############################################################################

#include "max7221array.h"

Max7221Array::Max7221Array() : devicePosition{ 0, 2, 3, 1 }, deviceOrientation{ 6, 6, 5, 5 }, minCurrentPerLED {0.185}
	{
	}

Max7221Array::~Max7221Array()
	{
	}

void Max7221Array::initialize(void)
	{
	pinMode(csPin, OUTPUT);
//	SPI.begin();													// Initializing SPI hardware
//	SPI.setDataMode(SPI_MODE0);										// Max7221 works in this SPI polarity/phase mode
//	SPI.setClockDivider(SPI_CLOCK_DIV2);							// Max7221 supports up to 10MHx, SPI_CLOCK_DIV2 will result in 8 MHz.

	writeAllDevices(Max7221Register::ScanLimit, 0x07);												// Configure ScanLimit - enabling all digits
	writeAllDevices(Max7221Register::DisplayTest, (uint8_t)displayTestMode::normalOperation);		// set to Normal Operation
	writeAllDevices(Max7221Register::Intensity, 0x00);												// Set to minimum intensity
	writeAllDevices(Max7221Register::ShutDown, (uint8_t)shutDownMode::normalOperation);				// Set to Normal Operation
	}

void Max7221Array::clearDisplayData(void)							// clear all data in displayData, does not issue a refresh
	{
	for (uint32_t i = 0; i < arrayWidth * arrayHeight * 8; i++)		// for all bytes in MCU displayData RAM
		{
		displayData[i] = 0x00;										// set to zero = no pixels on
		}
	}

void Max7221Array::refresh(void)
	{
	uint8_t digitIndex;                        						// iterating over all 8 digits in the device's displayDataRam
	uint8_t deviceIndex;											// iterating over all devices in the array

	for (digitIndex = 0; digitIndex < 8; digitIndex++)
		{
		deviceIndex = arrayWidth * arrayHeight;						// Last device in the daisy-chain needs to be shifted out first..
		digitalWrite(csPin, LOW);									// Start an SPI transfer by setting CS ActiveLow
		while (deviceIndex)											// For all devices in the daisy-chain...
			{
			SPI.write16((((uint8_t)Max7221Register::Digit0 + digitIndex) << 8) | getByte(deviceIndex - 1, digitIndex));				// Transmit a 16-bit word composed of Max7221 register and register-value
			deviceIndex--;											// .. next device..
			}
		digitalWrite(csPin, HIGH);									// End an SPI transfer
		}
	}

void Max7221Array::writeAllDevices(Max7221Register theRegister, uint8_t theData)
	{
	uint8_t deviceIndex;											// index to iterate over all devices in the array
	deviceIndex = arrayWidth * arrayHeight;							// number of devices to iterate over
	digitalWrite(csPin, LOW);										// Start an SPI transfer by setting CS ActiveLow
	while (deviceIndex)												// For all devices in the daisy-chain...
		{
		SPI.write16(((uint16_t)theRegister << 8) | theData);		// Transmit a 16-bit word composed of Max7221 register and register-value
		deviceIndex--;												// ..next device..
		}
	digitalWrite(csPin, HIGH);										// End an SPI transfer
	}

bool  Max7221Array::pixel(uint8_t x, uint8_t y, pixelOperation theOperation)
	{
	bool result = false;											// returned value of the pixel manipulation.. false = pixel is OFF, true = pixel is ON
	if ((x < getWidth()) && (y < getHeight()))						// only of X and Y are within the boundaries of the display..
		{
		uint16_t address = (y / 8) * (arrayWidth * 8) + x;			// calculate an offset address into the MCU displayData RAM
		switch (theOperation)
			{
			case pixelOperation::set:
			default:
				displayData[address] |= (0x01 << (y % 8));			// logical OR to set the pixel
				result = true;
				break;
			case pixelOperation::clear:
				displayData[address] &= ~(0x01 << (y % 8));			// logical AND with inverted mask, to clear the pixel
				result = false;
				break;
			case pixelOperation::toggle:
				displayData[address] ^= (0x01 << (y % 8));			// EXOR to toggle the pixel
				result = displayData[address] & (0x01 << (y % 8));	// read the actual value after manipulation
				break;
			case pixelOperation::get:
				result = displayData[address] & (0x01 << (y % 8));	// just read the value, no manipulation of the current pixel value
				break;
			}
		}
	return result;
	}


float Max7221Array::getCurrentConsumption(void)
	{
	uint32_t nmbrLEDsOn = 0;
	for (uint8_t i = 0; i < arrayWidth * arrayHeight * 8; i++)
		{
		for (uint8_t x = 0; x < 8; x++)
			{
			if (displayData[i] & (0x01 << x))
				{
				nmbrLEDsOn++l;
				}
			}
		}
	return ((float) nmbrLEDsOn) * minCurrentPerLED;										// TODO : extend formula to also include actual intensity setting
	}



uint8_t Max7221Array::getByte(uint8_t deviceIndex, uint8_t digitIndex)
	{
	uint8_t position = devicePosition[deviceIndex];										// where is this device positioned into the displayArray
	uint16_t xOffset = (position % arrayWidth) * 8;										// what is the offset in x-axis for this device
	uint16_t yOffset = (position / arrayWidth) * 8;										// what is the offset in y-axis for this device
	uint16_t ramOffset = (yOffset * arrayWidth) + (xOffset);							// what is the offset in the MCU displayData array

	switch (deviceOrientation[deviceIndex])												// depending on device orientation, Device register data must be mapped in one of several ways to MCU displayData
		{
		case 0:
		default:
			return displayData[ramOffset + digitIndex];
			break;

		case 1:
			return displayData[ramOffset + 7 - digitIndex];								// inverting X-axis by reading inverting MCU displayData read addresses (higher digit is lower address)
			break;

		case 2:
			return reverseBitOrder(displayData[ramOffset + digitIndex]);				// inverting Y-axis by reversing bits : MSB becomes LSB and vice versa
			break;

		case 3:
			return reverseBitOrder(displayData[ramOffset + 7 - digitIndex]);			// combining the above 2 techniques
			break;

		case 4:
			return collectByte(ramOffset, digitIndex, 0);								// swapping X and Y-axis y reading collecting a byte  by reading 1 bit from 8 successive displayData addresses
			break;

		case 5:
			return collectByte(ramOffset, 7 - digitIndex, 0);							// combining case 4 and case 1 
			break;

		case 6:
			return reverseBitOrder(collectByte(ramOffset, digitIndex, 0));				// combining case 4 and case 2
			break;

		case 7:
			return reverseBitOrder(collectByte(ramOffset, 7 - digitIndex, 0));			// combining case 4, 1 and 2
			break;
		}
	}


uint8_t Max7221Array::reverseBitOrder(uint8_t input)
	{
	uint8_t output = 0;															// Builds the value to be returned - starts from 0x00
	uint8_t inputMask = 0x01;													// Selects which bit [0..7] we want to collect : this mask iterates from LSB to MSB (0x01 to 0X80)
	uint8_t outputMask = 0x80;													// Mask to set the correct bit in the output : this mask iterates from MSB to LSB (0x80 to 0x01)
	while (inputMask)															// Both masks will iterate over all bit, but in opposite direction
		{
		if (input & inputMask)													// if the bit at this position 'p' is set...
			{
			output = (output | outputMask);										// ...set the bit at the matching '8-p' position
			}
		inputMask = inputMask << 1;												// adjust the mask
		outputMask = outputMask >> 1;											// adjust the mask
		}
	return output;
	}

uint8_t Max7221Array::collectByte(uint16_t offset, uint8_t digitIndex, bool reverse)
	{
	uint8_t output = 0;															// Builds the value to be returned - starts from 0x00
	uint8_t mask = 0x01 << digitIndex;											// Selects which bit [0..7] we want to collect

	for (uint8_t i = 0; i < 8; i++)												// loop over 8 sequential displayData bytes
		{
		if (!reverse)															// reverse==false iterates 'forward', positive X-direction. Otherwise we iterate backwards resulting in negative X-direction
			{
			if (displayData[offset + i] & mask)									// if there is a '1' in this position...
				{
				output = output | (0x01 << i);									// ... then add a '1' in the output
				}
			}
		else
			{
			if (displayData[offset + 7 - i] & mask)								// same as above but reading display-data bytes in the reverse order
				{
				output = output | (0x01 << i);
				}
			}
		}
	return output;
	}


uint16_t Max7221Array::getWidth()
	{
	return arrayWidth * 8;
	}

uint16_t Max7221Array::getHeight()
	{
	return arrayHeight * 8;
	}

