// #############################################################################
// ### This file is part of the source code for the TwinLights Mood Lamp     ###
// ### https://github.com/Strooom/TwinLights                                 ###
// ### Author(s) : Pascal Roobrouck - @strooom                               ###
// ### License : https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode ###
// #############################################################################

// Class to support a display built from an Array of (dot-matrix) LED displays driven from daisy-chained Max7221 controllers
// Notes :
// * this class is intended for dot-matrix style displays, (instead of 7-segment), so all the BCD stuff and Scan Limit is not supported
// * connection is intended via hardware SPI : SCK, MOSI are hardwared to the MCU, CS can be any GPIO, MISO is not being used as we only write to the display, never read from it.
// * all Max7221 (and their dot-matrix LEDs) devices may be connected in whatever sequence and rotation/mirroring, making it easy to connect them physically. If necessary, the bit-manipulations to render the contents of the MCU displayDataRam correctly on the device is handled by this class.
// * in order to keep current consumption under control, the total current consumption can be calculated from the MCU displayData, but you need to measure the current consumption for 1 device with a multimeter.
// Example Current consumption measurements :
// * Shutdown mode : 2.9 mA
// * Normal Mode - no pixels : 8.63 mA
// * 1 pixel, Intensity 15 = Max : 2.75 mA
// * 64 pixels, Intensity 15 : 170 mA
// * 64 pixels, Intensity 0 : 16 mA


#include <SPI.h>			// needed because this class needs to call hardware SPI routines to write it's MCU displayData to the Max7221 devices

#ifndef max7221array_h
#define max7221array_h

#define invertX	1			// image on the Dot-Matrix LED needs to be flipped along its X-axis (X-axis should run horizontal, left to right)
#define invertY	2			// image on the Dot-Matrix LED needs to be flipped along its Y-axis (Y-axis) should run vertical, top to bottom)
#define swapXY	4			// image on the Dot-Matrix LED needs its X- and Y-axis swapped
// All possible deviceOrientations are combinations of the above 3 orthogonal transformations :
// 0 : No Transformation
// 1 : invert the X-axis
// 2 : invert the Y-axis
// 3 : invert both the X-axis and Y-axis = rotate 180deg
// 4 : Swap X- and Y-axis
// 5 : Swap X- and Y-axis and invert the X-axis = rotate 90deg clockwise
// 6 : Swap X- and Y-axis and invert the Y-axis = rotate 90deg counterclockwise
// 7 : Swap X- and Y-axis and invert both the X-axis and Y-axis


enum class pixelOperation : uint8_t { set, clear, toggle, get, nmbrOperations };		// list of supported operations on pixels in the MCU displayData RAM
																						// list of Max7221 control registers
enum class Max7221Register : uint8_t { NoOp, Digit0, Digit1, Digit2, Digit3, Digit4, Digit5, Digit6, Digit7, DecodeMode, Intensity, ScanLimit, ShutDown, Unused, DisplayTest, nmbrRegisters };
enum class displayTestMode : uint8_t { normalOperation, DisplayTestmode, nmbrModes };	// list of Max7221 modes for displayTest
enum class shutDownMode : uint8_t { shutDown, normalOperation, nmbrModes };				// list of Max7221 modes for shutDown
enum class rotation : uint8_t {none, deg90, deg180, deg270, nmbrRotations};				// possible rotations for the whole display - for future use


class Max7221Array
	{
    private:
		static const uint8_t csPin = 2;									                // Which GPIO controls the ChipSelect line for selecting the slaves
		static const uint8_t arrayWidth = 2;								            // How many Max7221 are there 'horizontally'
		static const uint8_t arrayHeight = 2;								            // How many Max7221 are there 'vertically'
		static const uint8_t nmbrOfDevices = arrayWidth * arrayHeight;  				// Should be a maximum of 256 as they will be addressed with a uint8_t

//		rotation displayRotation = rotation::none;			                			// How is the display rotated - for future use
//		uint8_t intensity;																// What is the intensity of the LEDs - for future use

		const uint8_t devicePosition[arrayWidth * arrayHeight];							// Mapping the devices position in the SPI-chain to its position in the display
		const uint8_t deviceOrientation[arrayWidth * arrayHeight];						// How are the devices rotated - mirrored ?

		float displayData[8 * arrayWidth * arrayHeight];			        			// Need an array of size 8 * arrayWidth * arrayHeight, maximal 256 * 8 = 2048 bytes

		static const float minCurrentPerLED;									       	// milliAmps consumed for one LED, at minimum intensity
		uint8_t getByte(uint8_t deviceIndex, uint8_t digitIndex);						// Collects a byte of data from MCU displayRam as needed for a device's displayRam register
		uint8_t collectByte(uint16_t offset, uint8_t digitIndex, bool reverse);			// Helper function for getByte : swapping X and Y axis means you assemble a byte by taking 1 bit over 8 sequential displayData bytes
		uint8_t reverseBitOrder(uint8_t input);											// Reverses the order of bits in a byte : effectively swaps the Y-axis

		void writeAllDevices(Max7221Register theRegister, uint8_t theData);				// Write 1 value to 1 register for all devices


	public:
		Max7221Array();																	// Constructor
		~Max7221Array();																// Destructor

		uint16_t getWidth(void);														// Get the width of the display in pixels 
		uint16_t getHeight(void);														// Get the height of the display in pixels 

		void initialize(void);															// Initializes the display : configure all Max7221 devices. Assumes SPI has been initialized already
		void clearDisplayData(void);													// Clears the MCU displayData to all zeroes = a blank display
		void refresh(void);																// Refreshes the data from MCU RAM to Display registers

		bool pixel(uint8_t x, uint8_t y, pixelOperation theOperation);					// Set, clear, toggle or get pixels in the MCU displayData RAM

		float getCurrentConsumption(void);												// returns the number of milliAmps current consumed with current displayData contents.
	};


#endif


