#ifndef FLASH_DATA
#define FLASH_DATA 1

#include <cstdint>
#include <cstddef>

#include "Hardware/Backlight.hpp"
#include "UI/MessageLog.hpp"
#include "General/SimpleMath.h"
#include "UI/Display.hpp"

enum class DisplayDimmerType : uint8_t
{
	never = 0,				// never dim the display
	onIdle, 				// only display when printer status is idle
	always,					// default - always dim
	NumTypes
};

enum class HeaterCombineType : uint8_t
{
	notCombined = 0,
	combined,
	NumTypes
};

struct FlashData;

extern FlashData nvData, savedNvData;


struct FlashData
{
	// The magic value should be changed whenever the layout of the NVRAM changes
	// We now use a different magic value for each display size, to force the "touch the spot" screen to be displayed when you change the display size
	static const uint32_t magicVal = 0x3AB64A50 + DISPLAY_TYPE;
	static const uint32_t muggleVal = 0xFFFFFFFF;

	alignas(4) uint32_t magic;
	uint32_t baudRate;
	uint16_t xmin;
	uint16_t xmax;
	uint16_t ymin;
	uint16_t ymax;
	DisplayOrientation lcdOrientation;
	DisplayOrientation touchOrientation;
	uint8_t touchVolume;
	uint8_t language;
	uint8_t colourScheme;
	uint8_t brightness;
	DisplayDimmerType displayDimmerType;
	uint8_t infoTimeout;
	uint32_t screensaverTimeout;
	uint8_t babystepAmountIndex;
	uint16_t feedrate;
	HeaterCombineType heaterCombineType;
	MessageLog::LogLevel logLevel;
	// Persistent configuration of the Home dashboard slots.
	// The separate marker lets firmware distinguish new data from padding bytes
	// Keep the layout compatible with stored touch calibration data.
	static constexpr uint16_t homeSlotsMagicVal = 0x48A5;
	uint16_t homeSlotsMagic;
	// 0x00 = automatic/empty, 0x10..0x17 = FAN0..FAN7,
	// 0x20..0x27 = OUT0..OUT7, 0x30..0x37 = root macro 0..7.
	uint8_t homeSlotConfig[12];
	alignas(4) char dummy;								// must be at a multiple of 4 bytes from the start because flash is read/written in whole dwords

	FlashData() : magic(muggleVal) { SetDefaults(); }
	bool operator==(const FlashData& other);
	bool operator!=(const FlashData& other) { return !operator==(other); }
	bool IsValid() const;
	void SetInvalid() { magic = muggleVal; }
	void SetDefaults();
	void Load();
	void Save() const;

	bool IsSaveNeeded();

	bool SetColourScheme(uint8_t newColours);
	void SetInfoTimeout(uint8_t newInfoTimeout) { nvData.infoTimeout = newInfoTimeout; }
	bool SetLanguage(uint8_t newLanguage);

	void SetDisplayDimmerType(DisplayDimmerType newType) { nvData.displayDimmerType = newType; }
	DisplayDimmerType GetDisplayDimmerType() { return nvData.displayDimmerType; }

	void SetVolume(uint8_t newVolume) { nvData.touchVolume = newVolume; }
	uint32_t GetVolume() { return nvData.touchVolume; }

	void SetScreensaverTimeout(uint32_t screensaverTimeout) { nvData.screensaverTimeout = screensaverTimeout; }
	uint32_t GetScreensaverTimeout() { return nvData.screensaverTimeout; }

	void SetBabystepAmountIndex(uint8_t babystepAmountIndex) { nvData.babystepAmountIndex = babystepAmountIndex; }
	uint8_t GetBabystepAmountIndex() { return nvData.babystepAmountIndex; }

	void SetFeedrate(uint16_t feedrate) { nvData.feedrate = feedrate; }
	uint16_t GetFeedrate() { return nvData.feedrate; }

	void SetHeaterCombineType(HeaterCombineType combine) { nvData.heaterCombineType = combine; }
	HeaterCombineType GetHeaterCombineType() { return nvData.heaterCombineType; }

	void SetLogLevel(MessageLog::LogLevel logLevel) { nvData.logLevel = logLevel; }
	MessageLog::LogLevel GetLogLevel() { return nvData.logLevel; }

	void SetBaudRate(uint32_t rate) { nvData.baudRate = rate; }
	uint32_t GetBaudRate() { return nvData.baudRate; }

	void SetBrightness(uint32_t percent) { nvData.brightness =
		constrain<int>(percent, Backlight::MinBrightness, Backlight::MaxBrightness); }
	int GetBrightness() { return (int)nvData.brightness; }

	bool GetInvertZ() const
	{
		return (homeSlotsMagic & 0x7FFFu) == homeSlotsMagicVal && (homeSlotsMagic & 0x8000u) != 0;
	}
	void SetInvertZ(bool enabled)
	{
		if ((homeSlotsMagic & 0x7FFFu) != homeSlotsMagicVal)
		{
			homeSlotsMagic = homeSlotsMagicVal;
			for (uint8_t &v : homeSlotConfig) { v = 0; }
		}
		homeSlotsMagic = enabled ? (homeSlotsMagic | 0x8000u) : (homeSlotsMagic & 0x7FFFu);
	}

	uint8_t GetHomeSlotConfig(size_t slot) const
	{
		if (slot >= 12 || (homeSlotsMagic & 0x7FFFu) != homeSlotsMagicVal) { return 0; }
		const uint8_t v = homeSlotConfig[slot];
		const uint8_t kind = v & 0xF0;
		const uint8_t index = v & 0x0F;
		return (v == 0 || ((kind == 0x10 || kind == 0x20 || kind == 0x30) && index < 8)) ? v : 0;
	}
	void SetHomeSlotConfig(size_t slot, uint8_t value)
	{
		if (slot >= 12) { return; }
		if ((homeSlotsMagic & 0x7FFFu) != homeSlotsMagicVal)
		{
			homeSlotsMagic = homeSlotsMagicVal;
			for (uint8_t &v : homeSlotConfig) { v = 0; }
		}
		homeSlotConfig[slot] = value;
	}

};

#endif /* ifndef FLASH_DATA */
