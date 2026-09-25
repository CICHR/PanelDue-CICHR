/*
 * ColourSchemes.cpp
 *
 * Modern UI palette for PanelDue 800x480 displays.
 * The three original theme slots are retained so existing NVRAM settings
 * remain valid, but all three now use a contemporary flat/dark visual style.
 */

#include "ColourSchemes.hpp"
#include <Icons/Icons.hpp>

// Common colours
const Colour
	grey = UTFT::fromRGB(128, 128, 128),
	darkGrey = UTFT::fromRGB(60, 60, 60),
	lightGrey = UTFT::fromRGB(100, 100, 100),
	midGrey = UTFT::fromRGB(80, 80, 80),
	veryDarkGrey = UTFT::fromRGB(40, 40, 40),
	red = UTFT::fromRGB(255, 0, 0),
	lightRed = UTFT::fromRGB(255, 128, 128),
	darkRed = UTFT::fromRGB(128, 0, 0),
	yellow = UTFT::fromRGB(128, 128, 0),
	lightYellow = UTFT::fromRGB(255, 255, 128),
	darkYellow = UTFT::fromRGB(64, 64, 0),
	lightOrange = UTFT::fromRGB(255, 224, 192),
	orange = UTFT::fromRGB(255, 199, 89),
	darkOrange = UTFT::fromRGB(128, 64, 0),
	green = UTFT::fromRGB(0, 255, 0),
	lightGreen = UTFT::fromRGB(192, 255, 192),
	midGreen = UTFT::fromRGB(0, 160, 0),
	darkGreen = UTFT::fromRGB(0, 96, 0),
	turquoise = UTFT::fromRGB(0, 128, 128),
	blue = UTFT::fromRGB(0, 0, 255),
	byzantine = UTFT::fromRGB(170, 24, 170),
	lightBlue = UTFT::fromRGB(224, 224, 255),
	darkBlue = UTFT::fromRGB(0, 0, 64);

// Modern UI base palette. Keep colours reasonably separated in RGB565 so the
// 5" PanelDue still has clear contrast without relying on gradients.
static const Colour
	// Values intentionally mirror preview/style.css (quantised to RGB565 by UTFT).
	modernBg        = UTFT::fromRGB(17, 18, 20),      // #111214
	modernSurface   = UTFT::fromRGB(32, 33, 38),      // #202126
	modernSurface2  = UTFT::fromRGB(41, 43, 49),      // #292B31
	modernBorder    = UTFT::fromRGB(59, 62, 71),      // #3B3E47
	modernText      = UTFT::fromRGB(242, 243, 245),   // #F2F3F5
	modernTextDim   = UTFT::fromRGB(146, 150, 160),   // #9296A0
	modernAccent    = UTFT::fromRGB(74, 143, 231),    // #4A8FE7
	modernAccent2   = UTFT::fromRGB(50, 109, 168),    // #326DA8
	modernSuccess   = UTFT::fromRGB(94, 207, 155),
	modernWarning   = UTFT::fromRGB(231, 168, 58),
	modernDanger    = UTFT::fromRGB(226, 87, 87),
	modernDangerDim = UTFT::fromRGB(84, 43, 46),
	modernHeat      = UTFT::fromRGB(80, 48, 43),
	modernStandby   = UTFT::fromRGB(70, 59, 39),
	modernTune      = UTFT::fromRGB(41, 78, 66);

const ColourScheme colourSchemes[] =
{
	// Default dark/blue colour scheme. Slot 0 is used after a factory reset.
	// the new appearance immediately.
	{
		.index = 0,
		.pal = IconPaletteDark,

		.titleBarTextColour = modernText,
		.titleBarBackColour = UTFT::fromRGB(21, 22, 26),
		.labelTextColour = modernTextDim,
		.infoTextColour = modernText,
		.infoBackColour = modernSurface,
		.defaultBackColour = modernBg,
		.activeBackColour = modernHeat,
		.standbyBackColour = modernStandby,
		.tuningBackColour = modernTune,
		.errorTextColour = modernText,
		.errorBackColour = modernDangerDim,

		.popupBorderColour = modernBorder,
		.popupBackColour = modernSurface,
		.popupTextColour = modernText,
		.popupButtonTextColour = modernText,
		.popupButtonBackColour = modernSurface2,
		.popupInfoTextColour = modernText,
		.popupInfoBackColour = modernBg,

		.alertPopupBackColour = modernSurface,
		.alertPopupTextColour = modernText,

		.buttonTextColour = modernText,
		.buttonPressedTextColour = modernText,
		.buttonTextBackColour = modernSurface,
		.buttonImageBackColour = modernSurface,
		.buttonGradColour = 0,
		.buttonPressedBackColour = UTFT::fromRGB(82, 86, 94),
		.buttonPressedGradColour = 0,
		.buttonBorderColour = modernBorder,
		.homedButtonBackColour = UTFT::fromRGB(24, 92, 75),
		.notHomedButtonBackColour = UTFT::fromRGB(86, 66, 25),
		.pauseButtonBackColour = UTFT::fromRGB(116, 77, 17),
		.resumeButtonBackColour = UTFT::fromRGB(20, 111, 79),
		.resetButtonBackColour = modernDangerDim,

		.progressBarColour = UTFT::fromRGB(138, 144, 154),
		.progressBarBackColour = modernSurface2,

		.stopButtonTextColour = modernText,
		.stopButtonBackColour = modernDanger
	},

	// Modern blue variant.
	{
		.index = 1,
		.pal = IconPaletteDark,

		.titleBarTextColour = modernText,
		.titleBarBackColour = UTFT::fromRGB(13, 32, 35),
		.labelTextColour = modernTextDim,
		.infoTextColour = modernText,
		.infoBackColour = UTFT::fromRGB(21, 46, 49),
		.defaultBackColour = UTFT::fromRGB(7, 22, 24),
		.activeBackColour = modernHeat,
		.standbyBackColour = modernStandby,
		.tuningBackColour = modernTune,
		.errorTextColour = modernText,
		.errorBackColour = modernDangerDim,

		.popupBorderColour = UTFT::fromRGB(40, 83, 87),
		.popupBackColour = UTFT::fromRGB(13, 32, 35),
		.popupTextColour = modernText,
		.popupButtonTextColour = modernText,
		.popupButtonBackColour = UTFT::fromRGB(21, 46, 49),
		.popupInfoTextColour = modernText,
		.popupInfoBackColour = UTFT::fromRGB(7, 22, 24),

		.alertPopupBackColour = UTFT::fromRGB(13, 32, 35),
		.alertPopupTextColour = modernText,

		.buttonTextColour = modernText,
		.buttonPressedTextColour = modernText,
		.buttonTextBackColour = UTFT::fromRGB(21, 46, 49),
		.buttonImageBackColour = UTFT::fromRGB(21, 46, 49),
		.buttonGradColour = 0,
		.buttonPressedBackColour = UTFT::fromRGB(0, 119, 128),
		.buttonPressedGradColour = 0,
		.buttonBorderColour = UTFT::fromRGB(40, 83, 87),
		.homedButtonBackColour = UTFT::fromRGB(24, 92, 75),
		.notHomedButtonBackColour = UTFT::fromRGB(86, 66, 25),
		.pauseButtonBackColour = UTFT::fromRGB(116, 77, 17),
		.resumeButtonBackColour = UTFT::fromRGB(20, 111, 79),
		.resetButtonBackColour = modernDangerDim,

		.progressBarColour = UTFT::fromRGB(0, 170, 184),
		.progressBarBackColour = UTFT::fromRGB(21, 46, 49),

		.stopButtonTextColour = modernText,
		.stopButtonBackColour = modernDanger
	},

	// Modern graphite / green variant.
	{
		.index = 2,
		.pal = IconPaletteDark,

		.titleBarTextColour = modernText,
		.titleBarBackColour = UTFT::fromRGB(18, 27, 22),
		.labelTextColour = modernTextDim,
		.infoTextColour = modernText,
		.infoBackColour = UTFT::fromRGB(25, 39, 31),
		.defaultBackColour = UTFT::fromRGB(9, 18, 12),
		.activeBackColour = modernHeat,
		.standbyBackColour = modernStandby,
		.tuningBackColour = modernTune,
		.errorTextColour = modernText,
		.errorBackColour = modernDangerDim,

		.popupBorderColour = UTFT::fromRGB(42, 66, 50),
		.popupBackColour = UTFT::fromRGB(18, 27, 22),
		.popupTextColour = modernText,
		.popupButtonTextColour = modernText,
		.popupButtonBackColour = UTFT::fromRGB(25, 39, 31),
		.popupInfoTextColour = modernText,
		.popupInfoBackColour = UTFT::fromRGB(9, 18, 12),

		.alertPopupBackColour = UTFT::fromRGB(18, 27, 22),
		.alertPopupTextColour = modernText,

		.buttonTextColour = modernText,
		.buttonPressedTextColour = modernText,
		.buttonTextBackColour = UTFT::fromRGB(25, 39, 31),
		.buttonImageBackColour = UTFT::fromRGB(25, 39, 31),
		.buttonGradColour = 0,
		.buttonPressedBackColour = UTFT::fromRGB(22, 92, 55),
		.buttonPressedGradColour = 0,
		.buttonBorderColour = UTFT::fromRGB(42, 66, 50),
		.homedButtonBackColour = UTFT::fromRGB(24, 92, 75),
		.notHomedButtonBackColour = UTFT::fromRGB(86, 66, 25),
		.pauseButtonBackColour = UTFT::fromRGB(116, 77, 17),
		.resumeButtonBackColour = UTFT::fromRGB(20, 111, 79),
		.resetButtonBackColour = modernDangerDim,

		.progressBarColour = UTFT::fromRGB(36, 128, 76),
		.progressBarBackColour = UTFT::fromRGB(25, 39, 31),

		.stopButtonTextColour = modernText,
		.stopButtonBackColour = modernDanger
	}
};

static_assert(NumColourSchemes == sizeof(colourSchemes) / sizeof(colourSchemes[0]), "number of colourSchemes don't match");
