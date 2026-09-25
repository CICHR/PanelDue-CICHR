/*
 * UserInterface.cpp
 *
 *  Created on: 7 Jan 2017
 *      Author: David
*/

#include <UI/UserInterface.hpp>

#include <ctype.h>
#include <cstdlib>

#include "Configuration.hpp"
#include "FileManager.hpp"
#include "FlashData.hpp"

#include "Hardware/Buzzer.hpp"
#include "Hardware/Reset.hpp"
#include "Hardware/SerialIo.hpp"
#include "Hardware/SysTick.hpp"

#include "Icons/Icons.hpp"
#include "Library/Misc.hpp"
#include "ObjectModel/BedOrChamber.hpp"
#include "ObjectModel/PrinterStatus.hpp"
#include "PanelDue.hpp"
#include "Version.hpp"

#include <General/SafeVsnprintf.h>
#include <General/SimpleMath.h>
#include <General/String.h>
#include <General/StringFunctions.h>

#include <ObjectModel/Axis.hpp>
#include <ObjectModel/Utils.hpp>

#include <UI/MessageLog.hpp>
#include <UI/Popup.hpp>
#include <UI/UserInterfaceConstants.hpp>

MainWindow mgr;

#define DEBUG 0
#include "Debug.hpp"

// Public fields
TextField *fwVersionField, *userCommandField, *ipAddressField;
IntegerField *freeMem;
StaticTextField *touchCalibInstruction, *debugField;
StaticTextField *messageTextFields[numMessageRows], *messageTimeFields[numMessageRows];

static const ColourScheme *colours;

// Private fields
static const size_t machineNameLength = 30;
static const size_t printingFileLength = 40;
static const size_t zprobeBufLength = 12;
static const size_t generatedByTextLength = 50;
static const size_t lastModifiedTextLength = 20;
static const size_t printTimeTextLength = 12;		// e.g. 11h 55m
static const size_t controlPageMacroTextLength = 50;
static const size_t ipAddressLength = 45;	// IPv4 needs max 15 but IPv6 can go up to 45

static String<ipAddressLength> ipAddress;

struct FileListButtons
{
	SingleButton *scrollLeftButton, *scrollRightButton, *folderUpButton;
	IntegerField *errorField;
};

static StaticTextField *fileListPopupNoFiles;

static FileListButtons filesListButtons, macrosListButtons;
static SingleButton *changeCardButton;

static TextButton *filenameButtons[NumDisplayedFiles];
static TextButton *macroButtons[NumDisplayedMacros];
static TextButton *controlPageMacroButtons[NumControlPageMacroButtons];
static String<controlPageMacroTextLength> controlPageMacroText[NumControlPageMacroButtons];

static PopupWindow *setTempPopup, *setRPMPopup, *movePopup, *extrudePopup, *fileListPopup, *macrosPopup, *fileDetailPopup, *baudPopup,
		*volumePopup, *infoTimeoutPopup, *screensaverTimeoutPopup, *babystepAmountPopup, *feedrateAmountPopup, *areYouSurePopup, *keyboardPopup, *languagePopup, *coloursPopup, *screensaverPopup, *firmwareUpdatePopup;
static StaticTextField *areYouSureTextField, *areYouSureQueryField;
static DisplayField *emptyRoot, *baseRoot, *headerRoot, *commonRoot, *controlRoot, *printRoot, *messageRoot, *setupRoot;
static SingleButton *homeAllButton, *bedCompButton;
#if DISPLAY_X == 800
static VectorIconButton *homeButtons[MaxDisplayableAxes], *toolButtons[MaxSlots];
static VectorIconButton *modernJogZTopButton = nullptr, *modernJogZBottomButton = nullptr;
static VectorIconButton *homeMacroActions[MaxSlots] = { nullptr };
#else
static IconButtonWithText *homeButtons[MaxDisplayableAxes], *toolButtons[MaxSlots];
#endif

static float axisMaxVal = 0.0;
static float axisMinValues[MaxTotalAxes] = { 0.0f };
static float axisMaxValues[MaxTotalAxes] = { 0.0f };
static float axisPositionValues[MaxTotalAxes] = { 0.0f };
#if DISPLAY_X == 800
static BedMapField *moveBedMap = nullptr;
static StaticTextField *moveToolField = nullptr;
static StaticTextField *dashboardStatusField = nullptr;
static StaticTextField *homeNotHomedField = nullptr;
static StaticTextField *homeMachineTitleField = nullptr;
static String<48> homeNotHomedText;
static ProgressBar *dashboardProgressBar = nullptr;
static IntegerField *printPercentField = nullptr;
static TextField *printElapsedField = nullptr;
static String<20> printElapsedText;
static String<20> moveToolText;
static VectorIconButton *printTempTitles[MaxSlots] = { nullptr };
static FloatField *printCurrentTemps[MaxSlots] = { nullptr };
static ButtonPress modernJogStepPress, modernFeedratePress;
static float modernJogStep = 10.0f;
static int modernFeedrateMmS = 25;

static constexpr size_t MaxPrintTempControls = 8;
static constexpr size_t MaxPrintFanControls = 8;
static constexpr size_t MaxPrintGpOutControls = 8;
static CompactPercentButton *printFanButtons[MaxPrintFanControls] = { nullptr };
static CompactPercentButton *printGpOutButtons[MaxPrintGpOutControls] = { nullptr };
static StaticTextField *printFanNameFields[MaxPrintFanControls] = { nullptr };
static StaticTextField *printGpOutNameFields[MaxPrintGpOutControls] = { nullptr };

static CompactPercentButton *homeAuxValues[MaxSlots] = { nullptr };
static HomeSlotPickerField *homeSlotPicker = nullptr;
static PopupWindow *homeSlotPopup = nullptr;
static size_t homeSlotBeingConfigured = MaxSlots;
static int lastFanPercent[MaxPrintFanControls] = { 0 };
static int lastGpOutPercent[MaxPrintGpOutControls] = { 0 };
static uint8_t availableFanMask = 0;
static uint8_t availableGpOutMask = 0;
static const char* const homeFanLabels[MaxPrintFanControls] = { "F0","F1","F2","F3","F4","F5","F6","F7" };
static const char* const outputNames[MaxPrintGpOutControls] = { "Out0","Out1","Out2","Out3","Out4","Out5","Out6","Out7" };
static String<20> fanNames[MaxPrintFanControls];
static String<24> gpOutNames[MaxPrintGpOutControls];
static String<40> homeMacroNames[8];
static String<64> homeMacroFiles[8];
static uint8_t availableHomeMacroMask = 0;
static const char* const extrusionFactorLabels[MaxSlots] = { "E0 ", "E1 ", "E2 ", "E3 ", "E4 ", "E5 ", "E6 ", "E7 ", "E8 ", "E9 ", "E10 ", "E11 " };

static StaticTextField *adjustPopupNameField = nullptr;
static StaticTextField *adjustPopupValueField = nullptr;
static VectorIconButton *adjustPopupIconField = nullptr;
static StaticTextField *adjustRpmPopupNameField = nullptr;
static StaticTextField *adjustRpmPopupValueField = nullptr;
static VectorIconButton *adjustRpmPopupIconField = nullptr;
static String<32> adjustPopupNameText;
static String<24> adjustPopupValueText;

static CharButtonRow *consoleKeyboardRows[4] = { nullptr };
static const char* _ecv_array const * _ecv_array consoleCurrentKeyboard = nullptr;
#endif
// Popup keyboard command field exists on all display sizes.
static TextField *popupUserCommandField = nullptr;
static FloatField *controlTabAxisPos[MaxDisplayableAxes];
#if DISPLAY_X == 800
static FloatField *printTabAxisPos[MaxDisplayableAxes];
#endif
static FloatField *movePopupAxisPos[MaxDisplayableAxes];
static FloatField *currentTemps[MaxSlots];
static FloatField *fpHeightField, *fpLayerHeightField, *babystepOffsetField;
static TextButtonWithLabel *babystepMinusButton, *babystepPlusButton;
static IntegerField *fpSizeField, *fpFilamentField, *filePopupTitleField;
static ProgressBar *printProgressBar;
static SingleButton *tabControl, *tabStatus, *tabMsg, *tabSetup;
static ButtonBase *filesButton, *pauseButton, *resumeButton, *cancelButton, *babystepButton, *reprintButton;
static TextField *timeLeftField, *zProbe;
static TextField *fpNameField, *fpGeneratedByField, *fpLastModifiedField, *fpPrintTimeField;
DrawDirect *fpThumbnail;
#if DISPLAY_X != 800
static StaticTextField *moveAxisRows[MaxDisplayableAxes];
#endif
static StaticTextField *nameField, *statusField;
static StaticTextField *screensaverText;
static IntegerButton *activeTemps[MaxSlots], *standbyTemps[MaxSlots];
static IntegerButton *spd, *extrusionFactors[MaxSlots], *fanSpeed, *baudRateButton, *volumeButton, *infoTimeoutButton, *screensaverTimeoutButton, *feedrateAmountButton;
static TextButton *languageButton, *coloursButton, *dimmingTypeButton, *heaterCombiningButton, *logLevelButton, *invertZButton;
static TextButtonWithLabel *babystepAmountButton;
static SingleButton *moveButton, *extrudeButton, *macroButton;
static PopupWindow *babystepPopup;
static AlertPopup *alertPopup;
static CharButtonRow *keyboardRows[4];
static const char* _ecv_array const * _ecv_array currentKeyboard;
static void (*keyboardDataHandler)(const char *data) = nullptr;

static ButtonBase * null currentTab = nullptr;

static ButtonPress currentButton;
static ButtonPress fieldBeingAdjusted;
static ButtonPress currentExtrudeRatePress, currentExtrudeAmountPress;

static String<machineNameLength> machineName;
static String<printingFileLength> printingFile;
static bool lastJobFileNameAvailable = false;
static String<zprobeBufLength> zprobeBuf;
static String<generatedByTextLength> generatedByText;
static String<lastModifiedTextLength> lastModifiedText;
static String<printTimeTextLength> printTimeText;

const size_t maxUserCommandLength = 40;					// max length of a user gcode command
const size_t numUserCommandBuffers = 6;					// number of command history buffers plus one

static String<maxUserCommandLength> userCommandBuffers[numUserCommandBuffers];
static size_t currentUserCommandBuffer = 0, currentHistoryBuffer = 0;

static unsigned int numToolColsUsed = 0;
static unsigned int numHeaterAndToolColumns = 0;
static int oldIntValue;
static Event eventToConfirm = evNull;
static uint8_t numVisibleAxes = 0;						// initialise to 0 so we refresh the macros list when we receive the number of axes
static uint8_t numDisplayedAxes = 0;
static bool isDelta = false;

const char* _ecv_array null currentFile = nullptr;			// file whose info is displayed in the file info popup
const StringTable * strings = &LanguageTables[0];
static bool keyboardIsDisplayed = false;
static bool keyboardShifted = false;

int32_t alertMode = -1;									// the mode of the current alert, or -1 if no alert displayed
uint32_t alertTicks = 0;
uint32_t infoTimeout = DefaultInfoTimeout;				// info timeout in seconds, 0 means don't display into messages at all
uint32_t whenAlertReceived;
bool displayingResponse = false;						// true if displaying a response

static PixelNumber screensaverTextWidth = 0;
static uint32_t lastScreensaverMoved = 0;

static uint8_t currentWorkplaceNumber = OM::MaxTotalWorkplaces;
static int8_t currentTool = -2;							// Initialized to a value never returned by RRF to have the logic for "no tool" applied at startup
#if DISPLAY_X == 800
enum class UiMachineMode : uint8_t { Fff, Cnc, Laser };
static UiMachineMode uiMachineMode = UiMachineMode::Fff;

static VectorIcon ModernToolModeIcon(bool hasSpindle)
{
	if (uiMachineMode == UiMachineMode::Laser)
	{
		return VectorIcon::Laser;
	}
	if (uiMachineMode == UiMachineMode::Cnc || hasSpindle)
	{
		return VectorIcon::Cnc;
	}
	return VectorIcon::Nozzle;
}
#endif
static bool allAxesHomed = false;
static const bool isLandscape = true; 					// Once portrait mode is enabled, this needs to be de-const-ed

#ifdef SUPPORT_ENCODER

# include "Hardware/RotaryEncoder.hpp"

static RotaryEncoder *encoder;
static uint32_t lastEncoderCommandSentAt = 0;
#endif

inline PixelNumber CalcWidth(unsigned int numCols, PixelNumber displayWidth = mainContentWidth)
{
	return (displayWidth - 2 * margin + fieldSpacing)/numCols - fieldSpacing;
}

inline PixelNumber CalcXPos(unsigned int col, PixelNumber width, int offset = mainContentLeft)
{
	return col * (width + fieldSpacing) + margin + offset;
}

// Add a text button with a string parameter
TextButton *AddTextButton(PixelNumber row, unsigned int col, unsigned int numCols, const char* _ecv_array text, Event evt, const char* param, PixelNumber displayWidth = mainContentWidth)
{
	PixelNumber width = CalcWidth(numCols, displayWidth);
	PixelNumber xpos = CalcXPos(col, width);
	TextButton *f = new TextButton(row - 2, xpos, width, text, evt, param);
	mgr.AddField(f);
	return f;
}

// Add a text button with an int parameter
TextButton *AddTextButton(PixelNumber row, unsigned int col, unsigned int numCols, const char* _ecv_array text, Event evt, int param, PixelNumber displayWidth = mainContentWidth)
{
	PixelNumber width = CalcWidth(numCols, displayWidth);
	PixelNumber xpos = CalcXPos(col, width);
	TextButton *f = new TextButton(row - 2, xpos, width, text, evt, param);
	mgr.AddField(f);
	return f;
}

// Add an integer button
IntegerButton *AddIntegerButton(PixelNumber row, unsigned int col, unsigned int numCols, const char * _ecv_array null label, const char * _ecv_array null units, Event evt, PixelNumber displayWidth = mainContentWidth)
{
	PixelNumber width = CalcWidth(numCols, displayWidth);
	PixelNumber xpos = CalcXPos(col, width);
	IntegerButton *f = new IntegerButton(row - 2, xpos, width, label, units);
	f->SetEvent(evt, 0);
	mgr.AddField(f);
	return f;
}

// Add an icon button with a string parameter
IconButton *AddIconButton(PixelNumber row, unsigned int col, unsigned int numCols, Icon icon, Event evt, const char* param, PixelNumber displayWidth = mainContentWidth)
{
	PixelNumber width = CalcWidth(numCols, displayWidth);
	PixelNumber xpos = CalcXPos(col, width);
	IconButton *f = new IconButton(row - 2, xpos, width, icon, evt, param);
	mgr.AddField(f);
	return f;
}

// Add an icon button with an int parameter
IconButton *AddIconButton(PixelNumber row, unsigned int col, unsigned int numCols, Icon icon, Event evt, int param, PixelNumber displayWidth = mainContentWidth)
{
	PixelNumber width = CalcWidth(numCols, displayWidth);
	PixelNumber xpos = CalcXPos(col, width);
	IconButton *f = new IconButton(row - 2, xpos, width, icon, evt, param);
	mgr.AddField(f);
	return f;
}

// Add an icon button with a string parameter
IconButtonWithText *AddIconButtonWithText(PixelNumber row, unsigned int col, unsigned int numCols, Icon icon, Event evt, const char * text, const char* param, PixelNumber displayWidth = mainContentWidth)
{
	PixelNumber width = CalcWidth(numCols, displayWidth);
	PixelNumber xpos = CalcXPos(col, width);
	IconButtonWithText *f = new IconButtonWithText(row - 2, xpos, width, icon, evt, text, param);
	mgr.AddField(f);
	return f;
}

// Add an icon button with an int parameter
IconButtonWithText *AddIconButtonWithText(PixelNumber row, unsigned int col, unsigned int numCols, Icon icon, Event evt, int intVal, const int param, PixelNumber displayWidth = mainContentWidth)
{
	PixelNumber width = CalcWidth(numCols, displayWidth);
	PixelNumber xpos = CalcXPos(col, width);
	IconButtonWithText *f = new IconButtonWithText(row - 2, xpos, width, icon, evt, intVal, param);
	mgr.AddField(f);
	return f;
}

// Add an icon button with an int parameter
IconButtonWithText *AddIconButtonWithText(PixelNumber row, unsigned int col, unsigned int numCols, Icon icon, Event evt, const char * text, const int param, PixelNumber displayWidth = mainContentWidth)
{
	PixelNumber width = CalcWidth(numCols, displayWidth);
	PixelNumber xpos = CalcXPos(col, width);
	IconButtonWithText *f = new IconButtonWithText(row - 2, xpos, width, icon, evt, text, param);
	mgr.AddField(f);
	return f;
}

// Create a row of text buttons.
// Optionally, set one to 'pressed' and return that one.
// Set the colours before calling this
ButtonPress CreateStringButtonRow(
		Window * parentWindow,
		PixelNumber top,
		PixelNumber left,
		PixelNumber totalWidth,
		PixelNumber spacing,
		unsigned int numButtons,
		const char* _ecv_array const text[],
		const char* _ecv_array const params[],
		Event evt,
		int selected = -1,
		bool textButtonForAxis = false,
		DisplayField** firstButton = nullptr)
{
	const PixelNumber step = (totalWidth + spacing)/numButtons;
	ButtonPress bp;
	// Since Window->AddField prepends fields in the linked list we start with the last element
	for (int i = numButtons - 1; i >= 0; --i)
	{
		TextButton *tp =
				textButtonForAxis
				? new TextButtonForAxis(top, left + i * step, step - spacing, text[i], evt, params[i])
				: new TextButton(		top, left + i * step, step - spacing, text[i], evt, params[i]);
		parentWindow->AddField(tp);
		if ((int)i == selected)
		{
			tp->Press(true, 0);
			bp = ButtonPress(tp, 0);
		}
		if (firstButton != nullptr && i == 0)
		{
			*firstButton = tp;
		}
	}
	return bp;
}

#if 0  // currently unused
// Create a row of icon buttons.
// Set the colours before calling this
void CreateIconButtonRow(Window * pf, PixelNumber top, PixelNumber left, PixelNumber totalWidth, PixelNumber spacing, unsigned int numButtons,
									const Icon icons[], const char* _ecv_array const params[], Event evt)
{
	const PixelNumber step = (totalWidth + spacing)/numButtons;
	for (unsigned int i = 0; i < numButtons; ++i)
	{
		pf->AddField(new IconButton(top, left + i * step, step - spacing, icons[i], evt, params[i]));
	}
}
#endif

// Create a popup bar with string parameters
PopupWindow *CreateStringPopupBar(const ColourScheme& colours, PixelNumber width, unsigned int numEntries, const char* const text[], const char* const params[], Event ev)
{
	PopupWindow *pf = new PopupWindow(popupBarHeight, width, colours.popupBackColour, colours.popupBorderColour);
	DisplayField::SetDefaultColours(colours.popupButtonTextColour, colours.popupButtonBackColour);
	PixelNumber step = (width - 2 * popupSideMargin + popupFieldSpacing)/numEntries;
	for (unsigned int i = 0; i < numEntries; ++i)
	{
		pf->AddField(new TextButton(popupTopMargin, popupSideMargin + i * step, step - popupFieldSpacing, text[i], ev, params[i]));
	}
	return pf;
}

// Create a popup bar with integer parameters
// If the 'params' parameter is null then we use 0, 1, 2.. at the parameters
PopupWindow *CreateIntPopupBar(const ColourScheme& colours, PixelNumber width, unsigned int numEntries, const char* const text[], const int * null params, Event ev, Event zeroEv)
{
	PopupWindow *pf = new PopupWindow(popupBarHeight, width, colours.popupBackColour, colours.popupBorderColour);
	DisplayField::SetDefaultColours(colours.popupButtonTextColour, colours.popupButtonBackColour);
	PixelNumber step = (width - 2 * popupSideMargin + popupFieldSpacing)/numEntries;
	for (unsigned int i = 0; i < numEntries; ++i)
	{
		const int iParam = (params == nullptr) ? (int)i : params[i];
		pf->AddField(new TextButton(popupSideMargin, popupSideMargin + i * step, step - popupFieldSpacing, text[i], (params && params[i] == 0) ? zeroEv : ev, iParam));
	}
	return pf;
}

// Nasty hack to work around bug in RepRapFirmware 1.09k and earlier
// The M23 and M30 commands don't work if we send the full path, because "0:/gcodes/" gets prepended regardless.
const char * _ecv_array StripPrefix(const char * _ecv_array dir)
{
	if (GetFirmwareFeatures().IsBitSet(noGcodesFolder))			// if running RepRapFirmware
	{
		const size_t len = strlen(dir);
		if (len >= 8 && memcmp(dir, "/gcodes/", 8) == 0)
		{
			dir += 8;
		}
		else if (len >= 10 && memcmp(dir, "0:/gcodes/", 10) == 0)
		{
			dir += 10;
		}
		else if (strcmp(dir, "/gcodes") == 0 || strcmp(dir, "0:/gcodes") == 0)
		{
			dir += len;
		}
	}
	return dir;
}


static void SendGcode(const char *data)
{
	SerialIo::Sendf("%s\n", data);
}

static void PopupEditData(const char *data)
{
	alertPopup->UpdateData(data);
	dbg("received data %s\n", data);
	mgr.ClearPopup(true, keyboardPopup);
}

// Adjust the brightness
static void ChangeBrightness(bool up)
{
	int adjust = max<int>(1, nvData.GetBrightness() / 5);
	if (!up)
	{
		adjust = -adjust;
	}
	SetBrightness(nvData.GetBrightness() + adjust);
}


#if DISPLAY_X == 800
static bool FindAxisIndexByLetter(char letter, size_t& result)
{
	for (size_t i = 0; i < MaxTotalAxes; ++i)
	{
		const OM::Axis *axis = OM::GetAxis(i);
		if (axis != nullptr && axis->letter[0] == letter)
		{
			result = i;
			return true;
		}
	}
	return false;
}

static void UpdateMoveBedMapGeometry()
{
	if (moveBedMap == nullptr)
	{
		return;
	}
	size_t xi = 0, yi = 1;
	FindAxisIndexByLetter('X', xi);
	FindAxisIndexByLetter('Y', yi);
	float xmin = axisMinValues[xi], xmax = axisMaxValues[xi];
	float ymin = axisMinValues[yi], ymax = axisMaxValues[yi];
	if (xmax <= xmin + 0.01f) { xmin = 0.0f; xmax = 300.0f; }
	if (ymax <= ymin + 0.01f) { ymin = 0.0f; ymax = 300.0f; }

	// Preserve the actual X:Y travel aspect ratio. Delta/linear-delta beds are
	// presented as circles; Cartesian/CoreXY-style beds remain rectangles.
	const float xRange = xmax - xmin, yRange = ymax - ymin;
	const int boxX = 26, boxY = 92, boxW = 378, boxH = 310;
	int mapW = boxW, mapH = boxH;
	if (isDelta)
	{
		mapW = mapH = min(boxW, boxH);
	}
	else if (xRange > 0.01f && yRange > 0.01f)
	{
		const float scale = min((float)boxW/xRange, (float)boxH/yRange);
		mapW = max(80, (int)(xRange * scale + 0.5f));
		mapH = max(80, (int)(yRange * scale + 0.5f));
	}
	const int mapX = boxX + (boxW - mapW)/2;
	const int mapY = boxY + (boxH - mapH)/2;
	moveBedMap->SetFrame((PixelNumber)mapX, (PixelNumber)mapY, (PixelNumber)mapW, (PixelNumber)mapH, isDelta);
	moveBedMap->SetBounds(xmin, xmax, ymin, ymax);
	moveBedMap->SetPosition(axisPositionValues[xi], axisPositionValues[yi]);
}
#endif

void UI::SetAxisMin(size_t index, float val)
{
	if (index >= MaxTotalAxes)
	{
		return;
	}
	axisMinValues[index] = val;
#if DISPLAY_X == 800
	UpdateMoveBedMapGeometry();
#endif
}

void UI::SetAxisMax(size_t index, float val)
{
	if (index >= MaxTotalAxes)
	{
		return;
	}
	axisMaxValues[index] = val;
	axisMaxVal = max(axisMaxVal, val);
#if DISPLAY_X == 800
	UpdateMoveBedMapGeometry();
#endif
}


// Cycle through available display dimmer types
static void ChangeDisplayDimmerType()
{
	DisplayDimmerType newType = (DisplayDimmerType) ((uint8_t)nvData.GetDisplayDimmerType() + 1);
	if (newType == DisplayDimmerType::NumTypes)
	{
		newType = (DisplayDimmerType)0;
	}
	nvData.SetDisplayDimmerType(newType);
}

// Cyce through available heater combine types and repaint
static void ChangeHeaterCombineType()
{
	HeaterCombineType newType = (HeaterCombineType) ((uint8_t)nvData.GetHeaterCombineType() + 1);
	if (newType == HeaterCombineType::NumTypes)
	{
		newType = (HeaterCombineType)0;
	}
	nvData.SetHeaterCombineType(newType);
	UI::AllToolsSeen();
}

// Update an integer field, provided it isn't the one being adjusted
// Don't update it if the value hasn't changed, because that makes the display flicker unnecessarily
static void UpdateField(IntegerButton *f, int val)
{
	if (f != fieldBeingAdjusted.GetButton() && f->GetValue() != val)
	{
		f->SetValue(val);
	}
}

#if DISPLAY_X == 800
static void UpdateField(CompactPercentButton *f, int val)
{
	if (f != fieldBeingAdjusted.GetButton() && f->GetValue() != val)
	{
		f->SetValue(val);
	}
}

static const char* GetFanDisplayName(size_t fanIndex)
{
	if (fanIndex < MaxPrintFanControls && !fanNames[fanIndex].IsEmpty())
	{
		return fanNames[fanIndex].c_str();
	}
	return (fanIndex < MaxPrintFanControls) ? homeFanLabels[fanIndex] : "Fan";
}

static const char* GetGpOutDisplayName(size_t outputIndex)
{
	if (outputIndex < MaxPrintGpOutControls && !gpOutNames[outputIndex].IsEmpty())
	{
		return gpOutNames[outputIndex].c_str();
	}
	return (outputIndex < MaxPrintGpOutControls) ? outputNames[outputIndex] : "Out";
}

static const char* GetHomeMacroDisplayName(size_t macroIndex)
{
	return (macroIndex < 8 && !homeMacroNames[macroIndex].IsEmpty()) ? homeMacroNames[macroIndex].c_str() : "Macro";
}

static void UpdateNotHomedSummary()
{
	if (homeNotHomedField == nullptr) { return; }
	homeNotHomedText.copy("NOT HOMED:");
	bool any = false;
	OM::IterateAxesWhile([&any](OM::Axis*& axis, size_t)
	{
		if (axis->visible && !axis->homed)
		{
			homeNotHomedText.cat(" ");
			homeNotHomedText.cat(axis->letter);
			any = true;
		}
		return true;
	});
	if (!any) { homeNotHomedText.copy("ALL AXES HOMED"); }
	homeNotHomedField->SetValue(homeNotHomedText.c_str());
	if (homeMachineTitleField != nullptr)
	{
		homeMachineTitleField->SetValue(any ? "MACHINE" : "MACHINE POSITION");
	}
}

static CompactPercentButton* FindCompactPercentButton(ButtonBase* button)
{
	if (button == nullptr) { return nullptr; }
	for (size_t i = 0; i < MaxPrintFanControls; ++i)
	{
		if (printFanButtons[i] == button) { return printFanButtons[i]; }
	}
	for (size_t i = 0; i < MaxPrintGpOutControls; ++i)
	{
		if (printGpOutButtons[i] == button) { return printGpOutButtons[i]; }
	}
	for (size_t i = 0; i < MaxSlots; ++i)
	{
		if (homeAuxValues[i] == button) { return homeAuxValues[i]; }
	}
	return nullptr;
}
#endif

static int GetAdjustableIntValue(ButtonBase* button)
{
#if DISPLAY_X == 800
	CompactPercentButton* const compact = FindCompactPercentButton(button);
	if (compact != nullptr) { return compact->GetValue(); }
#endif
	return static_cast<IntegerButton*>(button)->GetValue();
}

static void SetAdjustableIntValue(ButtonBase* button, int value)
{
#if DISPLAY_X == 800
	CompactPercentButton* const compact = FindCompactPercentButton(button);
	if (compact != nullptr)
	{
		compact->SetValue(value);
		return;
	}
#endif
	static_cast<IntegerButton*>(button)->SetValue(value);
}

#if DISPLAY_X == 800
static void UpdateAdjustmentPopup(const ButtonPress& bp)
{
	if (!bp.IsValid() || adjustPopupNameField == nullptr || adjustPopupValueField == nullptr || adjustPopupIconField == nullptr)
	{
		return;
	}

	const event_t ev = bp.GetEvent();
	int index = 0;
	VectorIcon icon = VectorIcon::Output;
	const char* units = "%";

	switch (ev)
	{
	case evAdjustToolActiveTemp:
		index = bp.GetIParam();
		adjustPopupNameText.printf("Tool %d - active", index);
		icon = VectorIcon::Nozzle;
		units = "°C";
		break;
	case evAdjustToolStandbyTemp:
		index = bp.GetIParam();
		adjustPopupNameText.printf("Tool %d - standby", index);
		icon = VectorIcon::Nozzle;
		units = "°C";
		break;
	case evAdjustBedActiveTemp:
		index = bp.GetIParam();
		adjustPopupNameText.printf("Bed %d - active", index);
		icon = VectorIcon::Bed;
		units = "°C";
		break;
	case evAdjustBedStandbyTemp:
		index = bp.GetIParam();
		adjustPopupNameText.printf("Bed %d - standby", index);
		icon = VectorIcon::Bed;
		units = "°C";
		break;
	case evAdjustChamberActiveTemp:
		index = bp.GetIParam();
		adjustPopupNameText.printf("Chamber %d - active", index);
		icon = VectorIcon::Chamber;
		units = "°C";
		break;
	case evAdjustChamberStandbyTemp:
		index = bp.GetIParam();
		adjustPopupNameText.printf("Chamber %d - standby", index);
		icon = VectorIcon::Chamber;
		units = "°C";
		break;
	case evAdjustFan:
		index = bp.GetIParam();
		adjustPopupNameText.copy(GetFanDisplayName((size_t)index));
		icon = VectorIcon::Fan;
		break;
	case evAdjustGpOut:
		index = bp.GetIParam();
		adjustPopupNameText.copy(GetGpOutDisplayName((size_t)index));
		icon = VectorIcon::Output;
		break;
	case evAdjustSpeed:
		adjustPopupNameText.copy("Print speed");
		icon = VectorIcon::Print;
		break;
	case evExtrusionFactor:
		index = bp.GetIParam();
		adjustPopupNameText.printf("Extrusion factor %d", index);
		icon = VectorIcon::Extrude;
		break;
	case evAdjustActiveRPM:
		index = bp.GetIParam();
		adjustPopupNameText.printf("Spindle %d", index);
		icon = VectorIcon::Spindle;
		units = "rpm";
		break;
	default:
		adjustPopupNameText.copy("Value");
		break;
	}

	adjustPopupValueText.printf("%d %s", GetAdjustableIntValue(bp.GetButton()), units);
	StaticTextField* const nameField = (ev == evAdjustActiveRPM && adjustRpmPopupNameField != nullptr)
		? adjustRpmPopupNameField : adjustPopupNameField;
	StaticTextField* const valueField = (ev == evAdjustActiveRPM && adjustRpmPopupValueField != nullptr)
		? adjustRpmPopupValueField : adjustPopupValueField;
	VectorIconButton* const iconField = (ev == evAdjustActiveRPM && adjustRpmPopupIconField != nullptr)
		? adjustRpmPopupIconField : adjustPopupIconField;
	nameField->SetValue(adjustPopupNameText.c_str(), true);
	valueField->SetValue(adjustPopupValueText.c_str(), true);
	iconField->SetIcon(icon);
}
#endif

static void PopupAreYouSure(Event ev, const char* text, const char* query = strings->areYouSure)
{
	eventToConfirm = ev;
	if (isLandscape)
	{
		areYouSureTextField->SetValue(text);
		areYouSureQueryField->SetValue(query);
		mgr.SetPopup(areYouSurePopup, AutoPlace, AutoPlace);
	}
}

static void CreateIntegerAdjustPopup(const ColourScheme& colours)
{
	static const char* const tempPopupText[] = {"-5", "-1", strings->set, "+1", "+5"};
	static const int tempPopupParams[] = { -5, -1, 0, 1, 5 };
#if DISPLAY_X == 800
	const PixelNumber popupHeight = 120;
	setTempPopup = new PopupWindow(popupHeight, tempPopupBarWidth, colours.popupBackColour, colours.popupBorderColour);
	DisplayField::SetDefaultColours(colours.popupButtonTextColour, colours.popupButtonBackColour,
			colours.popupBorderColour, 0, colours.buttonPressedBackColour, 0, colours.pal);
	adjustPopupIconField = new VectorIconButton(12, 20, 58, 54, VectorIcon::Nozzle, evNull, nullptr, DEFAULT_FONT, 0);
	setTempPopup->AddField(adjustPopupIconField);

	DisplayField::SetDefaultColours(colours.popupTextColour, colours.popupBackColour);
	adjustPopupNameField = new StaticTextField(12, 94, tempPopupBarWidth - 114, TextAlignment::Left, "Value");
	setTempPopup->AddField(adjustPopupNameField);
	DisplayField::SetDefaultFont(UI_LARGE_FONT);
	adjustPopupValueField = new StaticTextField(36, 94, tempPopupBarWidth - 114, TextAlignment::Left, "0 %");
	setTempPopup->AddField(adjustPopupValueField);
	DisplayField::SetDefaultFont(DEFAULT_FONT);

	DisplayField::SetDefaultColours(colours.popupButtonTextColour, colours.popupButtonBackColour,
			colours.popupBorderColour, 0, colours.buttonPressedBackColour, 0, colours.pal);
	const PixelNumber step = (tempPopupBarWidth - 2 * popupSideMargin + popupFieldSpacing) / ARRAY_SIZE(tempPopupText);
	for (unsigned int i = 0; i < ARRAY_SIZE(tempPopupText); ++i)
	{
		setTempPopup->AddField(new TextButton(82, popupSideMargin + i * step, step - popupFieldSpacing,
				tempPopupText[i], tempPopupParams[i] == 0 ? evSetInt : evAdjustInt, tempPopupParams[i]));
	}
#else
	setTempPopup = CreateIntPopupBar(colours, tempPopupBarWidth, 5, tempPopupText, tempPopupParams, evAdjustInt, evSetInt);
#endif
}

static void CreateIntegerRPMAdjustPopup(const ColourScheme& colours)
{
	static const char* const rpmPopupText[] = {"-1000", "-100", "-10", strings->set, "+10", "+100", "+1000"};
	static const int rpmPopupParams[] = { -1000, -100, -10, 0, 10, 100, 1000 };
#if DISPLAY_X == 800
	const PixelNumber popupHeight = 120;
	setRPMPopup = new PopupWindow(popupHeight, rpmPopupBarWidth, colours.popupBackColour, colours.popupBorderColour);
	DisplayField::SetDefaultColours(colours.popupButtonTextColour, colours.popupButtonBackColour,
			colours.popupBorderColour, 0, colours.buttonPressedBackColour, 0, colours.pal);
	adjustRpmPopupIconField = new VectorIconButton(12, 20, 58, 54, VectorIcon::Spindle, evNull, nullptr, DEFAULT_FONT, 0);
	setRPMPopup->AddField(adjustRpmPopupIconField);

	DisplayField::SetDefaultColours(colours.popupTextColour, colours.popupBackColour);
	adjustRpmPopupNameField = new StaticTextField(12, 94, rpmPopupBarWidth - 114, TextAlignment::Left, "Spindle");
	setRPMPopup->AddField(adjustRpmPopupNameField);
	DisplayField::SetDefaultFont(UI_LARGE_FONT);
	adjustRpmPopupValueField = new StaticTextField(36, 94, rpmPopupBarWidth - 114, TextAlignment::Left, "0 rpm");
	setRPMPopup->AddField(adjustRpmPopupValueField);
	DisplayField::SetDefaultFont(DEFAULT_FONT);

	DisplayField::SetDefaultColours(colours.popupButtonTextColour, colours.popupButtonBackColour,
			colours.popupBorderColour, 0, colours.buttonPressedBackColour, 0, colours.pal);
	const PixelNumber step = (rpmPopupBarWidth - 2 * popupSideMargin + popupFieldSpacing) / ARRAY_SIZE(rpmPopupText);
	for (unsigned int i = 0; i < ARRAY_SIZE(rpmPopupText); ++i)
	{
		setRPMPopup->AddField(new TextButton(82, popupSideMargin + i * step, step - popupFieldSpacing,
				rpmPopupText[i], rpmPopupParams[i] == 0 ? evSetInt : evAdjustInt, rpmPopupParams[i]));
	}
#else
	setRPMPopup = CreateIntPopupBar(colours, rpmPopupBarWidth, 7, rpmPopupText, rpmPopupParams, evAdjustInt, evSetInt);
#endif
}

// Create the movement popup window
static void CreateMovePopup(const ColourScheme& colours)
{
#if DISPLAY_X == 800
	// The Motion workspace uses the full 800x480 area without an auto-place gutter,
	// and every control lives inside one of three bounded cards.
	movePopup = new StandardPopupWindow(movePopupHeight, movePopupWidth, colours.popupBackColour,
			colours.popupBorderColour, colours.popupTextColour, colours.buttonImageBackColour, nullptr, 12);
	DisplayField::SetDefaultColours(colours.stopButtonTextColour, colours.stopButtonBackColour,
			colours.stopButtonBackColour, 0, colours.stopButtonBackColour, 0, colours.pal);
	movePopup->AddField(new VectorIconButton(8, 20, 164, 40,
			VectorIcon::Stop, evEmergencyStop, "STOP", DEFAULT_FONT, 0));
	DisplayField::SetDefaultColours(colours.popupTextColour, colours.popupBackColour);
	movePopup->AddField(new StaticTextField(17, 220, 350, TextAlignment::Centre, "MOTION"));

	// Left: work-area preview uses nearly the full available height.
	DisplayField::SetDefaultColours(colours.popupTextColour, colours.buttonTextBackColour);
	movePopup->AddField(new StaticTextField(66, 26, 360, TextAlignment::Left, "WORK AREA"));
	DisplayField::SetDefaultColours(colours.popupTextColour, colours.popupInfoBackColour,
			colours.popupBorderColour, 0, colours.buttonPressedBackColour, 0, colours.pal);
	moveBedMap = new BedMapField(92, 26, 378, 310, evMoveBedMap,
			UTFT::fromRGB(95, 100, 111), UTFT::fromRGB(255, 79, 80));
	movePopup->AddField(moveBedMap);
	UpdateMoveBedMapGeometry();

	DisplayField::SetDefaultColours(colours.popupTextColour, colours.buttonTextBackColour);
	for (size_t i = 0; i < MaxDisplayableAxes; ++i)
	{
		const PixelNumber px = 26 + (i < 3 ? (PixelNumber)i * 124 : 0);
		FloatField *f = new FloatField(430, px, 118, TextAlignment::Left, 1, axisNames[i], " mm", true);
		movePopupAxisPos[i] = f;
		movePopup->AddField(f);
		f->Show(i < 3);
	}
	movePopup->AddField(new ModernPanelField(56, 14, 402, 410,
			colours.buttonTextBackColour, colours.popupBorderColour, 14));

	// Right/top: labelled XY cross and independent Z jog. All buttons are
	// deliberately inset from the panel edge so -Y can never cross the border.
	DisplayField::SetDefaultColours(colours.popupTextColour, colours.buttonTextBackColour);
	movePopup->AddField(new StaticTextField(66, 442, 54, TextAlignment::Left, "JOG"));
	movePopup->AddField(new StaticTextField(66, 688, 78, TextAlignment::Centre, "Z"));
	DisplayField::SetDefaultColours(colours.popupButtonTextColour, colours.popupButtonBackColour,
			colours.popupBorderColour, 0, colours.buttonPressedBackColour, 0, colours.pal);
	// The XY controls are positioned so their touch areas do not overlap.
	movePopup->AddField(new VectorIconButton(74, 507, 78, 48, VectorIcon::ArrowUp, evModernJog, "+Y", DEFAULT_FONT, 2));
	movePopup->AddField(new VectorIconButton(128, 442, 78, 48, VectorIcon::ArrowLeft, evModernJog, "-X", DEFAULT_FONT, 0));
	movePopup->AddField(new VectorIconButton(128, 572, 78, 48, VectorIcon::ArrowRight, evModernJog, "+X", DEFAULT_FONT, 1));
	movePopup->AddField(new VectorIconButton(182, 507, 78, 48, VectorIcon::ArrowDown, evModernJog, "-Y", DEFAULT_FONT, 3));
	modernJogZTopButton = new VectorIconButton(96, 684, 88, 48, VectorIcon::ArrowUp, evModernJog, nvData.GetInvertZ() ? "-Z" : "Z+", DEFAULT_FONT, 4);
	modernJogZBottomButton = new VectorIconButton(158, 684, 88, 48, VectorIcon::ArrowDown, evModernJog, nvData.GetInvertZ() ? "Z+" : "-Z", DEFAULT_FONT, 5);
	movePopup->AddField(modernJogZTopButton);
	movePopup->AddField(modernJogZBottomButton);
	movePopup->AddField(new ModernPanelField(56, 428, 358, 180,
			colours.buttonTextBackColour, colours.popupBorderColour, 14));

	// Right/middle: STEP and SPEED live on the same card surface as the rest of
	// Motion; no black legacy label strips are used.
	DisplayField::SetDefaultColours(colours.popupTextColour, colours.buttonTextBackColour);
	movePopup->AddField(new StaticTextField(252, 442, 120, TextAlignment::Left, "STEP  mm"));
	DisplayField::SetDefaultColours(colours.popupButtonTextColour, colours.popupButtonBackColour,
			colours.popupBorderColour, 0, colours.buttonPressedBackColour, 0, colours.pal);
	static const char * const stepValues[] = { "0.1", "1", "10", "100" };
	for (int i = 0; i < 4; ++i)
	{
		TextButton * const b = new TextButton(274, 442 + i * 76, 70, stepValues[i], evModernJogStep, stepValues[i]);
		movePopup->AddField(b);
		if (i == 2)
		{
			b->Press(true, 0);
			modernJogStepPress.Set(b, 0);
		}
	}

	DisplayField::SetDefaultColours(colours.popupTextColour, colours.buttonTextBackColour);
	movePopup->AddField(new StaticTextField(318, 442, 180, TextAlignment::Left, "SPEED F  mm/s"));
	DisplayField::SetDefaultColours(colours.popupButtonTextColour, colours.popupButtonBackColour,
			colours.popupBorderColour, 0, colours.buttonPressedBackColour, 0, colours.pal);
	static const char * const feedValues[] = { "1", "5", "10", "25", "50" };
	for (int i = 0; i < 5; ++i)
	{
		TextButton * const b = new TextButton(340, 442 + i * 64, 58, feedValues[i], evModernFeedrate, feedValues[i]);
		movePopup->AddField(b);
		if (i == 3)
		{
			b->Press(true, 0);
			modernFeedratePress.Set(b, 0);
		}
	}
	movePopup->AddField(new ModernPanelField(244, 428, 358, 144,
			colours.buttonTextBackColour, colours.popupBorderColour, 14));

	// Right/bottom: active tool + large extrusion actions. At 132px width the
	// full RETRACT/EXTRUDE labels fit below the glyph instead of being cropped.
	moveToolText.copy("T--");
	DisplayField::SetDefaultColours(colours.popupTextColour, colours.buttonTextBackColour);
	moveToolField = new StaticTextField(418, 444, 52, TextAlignment::Centre, moveToolText.c_str());
	movePopup->AddField(moveToolField);
	movePopup->AddField(new ModernPanelField(404, 438, 60, 56,
			colours.buttonTextBackColour, colours.popupBorderColour, 10));
	DisplayField::SetDefaultColours(colours.popupButtonTextColour, colours.popupButtonBackColour,
			colours.popupBorderColour, 0, colours.buttonPressedBackColour, 0, colours.pal);
	movePopup->AddField(new VectorIconButton(402, 506, 132, 60, VectorIcon::Retract, evRetract, "RETRACT", DEFAULT_FONT, 0));
	movePopup->AddField(new VectorIconButton(402, 646, 132, 60, VectorIcon::Extrude, evExtrude, "EXTRUDE", DEFAULT_FONT, 0));
	movePopup->AddField(new ModernPanelField(396, 428, 358, 70,
			colours.buttonTextBackColour, colours.popupBorderColour, 14));
#else
	static const char * _ecv_array const xyJogValues[] = { "-100", "-10", "-1", "-0.1", "0.1",  "1", "10", "100" };
	static const char * _ecv_array const zJogValues[] = { "-50", "-5", "-0.5", "-0.05", "0.05",  "0.5", "5", "50" };

	movePopup = new StandardPopupWindow(movePopupHeight, movePopupWidth, colours.popupBackColour, colours.popupBorderColour, colours.popupTextColour, colours.buttonImageBackColour, strings->moveHead);
	PixelNumber ypos = popupTopMargin + buttonHeight + moveButtonRowSpacing;
	const PixelNumber axisPosYpos = ypos + (MaxDisplayableAxes - 1) * (buttonHeight + moveButtonRowSpacing);
	const PixelNumber xpos = popupSideMargin + axisLabelWidth;
	PixelNumber column = popupSideMargin + margin;
	PixelNumber xyFieldWidth = (DISPLAY_X - (2 * margin) - (MaxDisplayableAxes * fieldSpacing))/(MaxDisplayableAxes + 1);

	for (size_t i = 0; i < MaxDisplayableAxes; ++i)
	{
		DisplayField::SetDefaultColours(colours.popupButtonTextColour, colours.popupButtonBackColour);
		const char * _ecv_array const * _ecv_array values = (axisNames[i][0] == 'Z') ? zJogValues : xyJogValues;
		CreateStringButtonRow(movePopup, ypos, xpos, movePopupWidth - xpos - popupSideMargin, fieldSpacing, 8, values, values, evMoveAxis, -1, true);
		DisplayField::SetDefaultColours(colours.popupTextColour, colours.popupBackColour);
		StaticTextField * const tf = new StaticTextField(ypos + labelRowAdjust, popupSideMargin, axisLabelWidth, TextAlignment::Left, axisNames[i]);
		movePopup->AddField(tf);
		moveAxisRows[i] = tf;
		UI::ShowAxis(i, i < MIN_AXES, axisNames[i]);
		DisplayField::SetDefaultColours(colours.popupTextColour, colours.popupInfoBackColour);
		FloatField *f = new FloatField(axisPosYpos, column, xyFieldWidth, TextAlignment::Left, 2, axisNames[i]);
		movePopupAxisPos[i] = f;
		movePopup->AddField(f);
		f->Show(i < MIN_AXES);
		column += xyFieldWidth + fieldSpacing;
		ypos += buttonHeight + moveButtonRowSpacing;
	}
#endif
}

// Create the extrusion controls popup
static void CreateExtrudePopup(const ColourScheme& colours)
{
	static const char * _ecv_array extrudeAmountValues[] = { "100", "50", "20", "10", "5",  "1" };
	static const char * _ecv_array extrudeSpeedValues[] = { "50", "20", "10", "5", "2", "1", "0.5" };
	static const char * _ecv_array extrudeSpeedParams[] = { "3000", "1200", "600", "300", "120", "60", "30" };		// must be extrudeSpeedValues * 60

	extrudePopup = new StandardPopupWindow(extrudePopupHeight, extrudePopupWidth, colours.popupBackColour, colours.popupBorderColour, colours.popupTextColour, colours.buttonImageBackColour, strings->extrusionAmount);
	PixelNumber ypos = popupTopMargin + buttonHeight + extrudeButtonRowSpacing;
	DisplayField::SetDefaultColours(colours.popupButtonTextColour, colours.popupButtonBackColour);
	currentExtrudeAmountPress = CreateStringButtonRow(extrudePopup, ypos, popupSideMargin, extrudePopupWidth - 2 * popupSideMargin, fieldSpacing, 6, extrudeAmountValues, extrudeAmountValues, evExtrudeAmount, 3);
	ypos += buttonHeight + extrudeButtonRowSpacing;
	DisplayField::SetDefaultColours(colours.popupTextColour, colours.popupBackColour);
	extrudePopup->AddField(new StaticTextField(ypos + labelRowAdjust, popupSideMargin, extrudePopupWidth - 2 * popupSideMargin, TextAlignment::Centre, strings->extrusionSpeed));
	ypos += buttonHeight + extrudeButtonRowSpacing;
	DisplayField::SetDefaultColours(colours.popupButtonTextColour, colours.popupButtonBackColour);
	currentExtrudeRatePress = CreateStringButtonRow(
			extrudePopup,
			ypos,
			popupSideMargin,
			extrudePopupWidth - 2 * popupSideMargin,
			fieldSpacing,
			ARRAY_SIZE(extrudeSpeedValues),
			extrudeSpeedValues,
			extrudeSpeedParams,
			evExtrudeRate,
			ARRAY_SIZE(extrudeSpeedValues) / 2);

	ypos += buttonHeight + extrudeButtonRowSpacing;
	extrudePopup->AddField(new TextButton(ypos, popupSideMargin, extrudePopupWidth/3 - 2 * popupSideMargin, strings->extrude, evExtrude));
	extrudePopup->AddField(new TextButton(ypos, (2 * extrudePopupWidth)/3 + popupSideMargin, extrudePopupWidth/3 - 2 * popupSideMargin, strings->retract, evRetract));
}

// Create a popup used to list files pr macros
PopupWindow *CreateFileListPopup(FileListButtons& controlButtons, TextButton ** _ecv_array fileButtons, unsigned int numRows, unsigned int numCols, const ColourScheme& colours, bool filesNotMacros,
		PixelNumber popupHeight = fileListPopupHeight, PixelNumber popupWidth = fileListPopupWidth)
pre(fileButtons.lim == numRows * numCols)
{
	PopupWindow * const popup = new StandardPopupWindow(popupHeight, popupWidth, colours.popupBackColour, colours.popupBorderColour, colours.popupTextColour, colours.buttonImageBackColour, nullptr);
	const PixelNumber closeButtonPos = popupWidth - closeButtonWidth - popupSideMargin;
	const PixelNumber navButtonWidth = (closeButtonPos - popupSideMargin)/7;
	const PixelNumber upButtonPos = closeButtonPos - navButtonWidth - fieldSpacing;
	const PixelNumber rightButtonPos = upButtonPos - navButtonWidth - fieldSpacing;
	const PixelNumber leftButtonPos = rightButtonPos - navButtonWidth - fieldSpacing;
	const PixelNumber textPos = popupSideMargin + navButtonWidth;
	const PixelNumber changeButtonPos = popupSideMargin;
#if DISPLAY_X == 800
	const PixelNumber popupStopWidth = 88;
	const PixelNumber titleTextPos = textPos + popupStopWidth + fieldSpacing;
	DisplayField::SetDefaultColours(colours.stopButtonTextColour, colours.stopButtonBackColour,
			colours.stopButtonBackColour, 0, colours.stopButtonBackColour, 0, colours.pal);
	popup->AddField(new VectorIconButton(popupTopMargin, textPos, popupStopWidth, buttonHeight,
			VectorIcon::Stop, evEmergencyStop, "STOP", DEFAULT_FONT, 0));
#else
	const PixelNumber titleTextPos = textPos;
#endif

	DisplayField::SetDefaultColours(colours.popupTextColour, colours.popupBackColour);
	if (filesNotMacros)
	{
		popup->AddField(filePopupTitleField = new IntegerField(popupTopMargin + labelRowAdjust, titleTextPos, leftButtonPos - titleTextPos, TextAlignment::Centre, strings->filesOnCard, nullptr));
		popup->AddField(fileListPopupNoFiles = new StaticTextField(popupHeight / 2 - popupTopMargin, popupSideMargin, popupWidth, TextAlignment::Centre, strings->noFilesFound));
		fileListPopupNoFiles->Show(false);
	}
	else
	{
		popup->AddField(new StaticTextField(popupTopMargin + labelRowAdjust, titleTextPos, leftButtonPos - titleTextPos, TextAlignment::Centre, strings->macros));
	}

	DisplayField::SetDefaultColours(colours.popupButtonTextColour, colours.buttonImageBackColour);
	if (filesNotMacros)
	{
		popup->AddField(changeCardButton = new IconButton(popupTopMargin, changeButtonPos, navButtonWidth, IconFiles, evChangeCard, 0));
	}

	const Event scrollEvent = (filesNotMacros) ? evScrollFiles : evScrollMacros;

	DisplayField::SetDefaultColours(colours.popupButtonTextColour, colours.popupButtonBackColour);
	popup->AddField(controlButtons.scrollLeftButton = new TextButton(popupTopMargin, leftButtonPos, navButtonWidth, LEFT_ARROW, scrollEvent, -1));
	controlButtons.scrollLeftButton->Show(false);
	popup->AddField(controlButtons.scrollRightButton = new TextButton(popupTopMargin, rightButtonPos, navButtonWidth, RIGHT_ARROW, scrollEvent, 1));
	controlButtons.scrollRightButton->Show(false);
	popup->AddField(controlButtons.folderUpButton = new TextButton(popupTopMargin, upButtonPos, navButtonWidth, UP_ARROW, (filesNotMacros) ? evFilesUp : evMacrosUp));
	controlButtons.folderUpButton->Show(false);

	const PixelNumber fileFieldWidth = (popupWidth + fieldSpacing - (2 * popupSideMargin))/numCols;
	for (unsigned int c = 0; c < numCols; ++c)
	{
		PixelNumber row = popupTopMargin;
		for (unsigned int r = 0; r < numRows; ++r)
		{
			row += buttonHeight + fileButtonRowSpacing;
			TextButton *t = new TextButton(row, (fileFieldWidth * c) + popupSideMargin, fileFieldWidth - fieldSpacing, nullptr, evNull);
			t->Show(false);
			popup->AddField(t);
			*fileButtons = t;
			++fileButtons;
		}
	}

	controlButtons.errorField = new IntegerField(popupTopMargin + 2 * (buttonHeight + fileButtonRowSpacing), popupSideMargin, popupWidth - (2 * popupSideMargin),
							TextAlignment::Centre, strings->error, strings->accessingSdCard);
	controlButtons.errorField->Show(false);
	popup->AddField(controlButtons.errorField);
	return popup;
}

static void ThumbnailRefreshNotify(bool full, bool changed)
{
	UNUSED(changed);

	if (!full || !currentFile)
		return;

	dbg("full %d changed %d currentFile %s\n", full, changed, currentFile);
	SerialIo::Sendf(GetFirmwareFeatures().IsBitSet(noM20M36) ? "M408 S36 P" : "M36 ");			// ask for the file info
	SerialIo::SendFilename(CondStripDrive(FileManager::GetFilesDir()), currentFile);
	SerialIo::SendChar('\n');
}

// Create the popup window used to display the file dialog
static void CreateFileActionPopup(const ColourScheme& colours)
{
	PixelNumber y_start, height;
	PixelNumber x_start, width;

	fileDetailPopup = new StandardPopupWindow(fileInfoPopupHeight, fileInfoPopupWidth, colours.popupBackColour, colours.popupBorderColour, colours.popupTextColour, colours.buttonImageBackColour, nullptr);
	DisplayField::SetDefaultColours(colours.popupTextColour, colours.popupBackColour);

	PixelNumber ypos = popupTopMargin + 1;
	fpNameField = new TextField(ypos, popupSideMargin, fileInfoPopupWidth - closeButtonWidth - 3 * popupSideMargin, TextAlignment::Left, strings->fileName);
	ypos += rowTextHeight + 3;
	fpGeneratedByField = new TextField(ypos, popupSideMargin, fileInfoPopupWidth - 2 * popupSideMargin, TextAlignment::Left, strings->generatedBy, generatedByText.c_str());
	ypos += rowTextHeight;

	y_start = ypos + 3;
	height = 7 * rowTextHeight + (2 * rowTextHeight) / 3;

	x_start = fileInfoPopupWidth - popupSideMargin * 3 / 2 - fileInfoPopupWidth / 3;
	width = fileInfoPopupWidth / 3 + 5;

	fpThumbnail = new DrawDirect(y_start, x_start, height, width, ThumbnailRefreshNotify);

	dbg("y_start %d x_start %d height %d width %d\n", y_start, x_start, height, width);
	dbg("text height %d\n", rowTextHeight);

	fpSizeField = new IntegerField(ypos, popupSideMargin, fileInfoPopupWidth - 2 * popupSideMargin - fileInfoPopupWidth / 3, TextAlignment::Left, strings->fileSize, " b");
	ypos += rowTextHeight;
	fpLayerHeightField = new FloatField(ypos, popupSideMargin, fileInfoPopupWidth - 2 * popupSideMargin - fileInfoPopupWidth / 3, TextAlignment::Left, 2, strings->layerHeight, "mm");
	ypos += rowTextHeight;
	fpHeightField = new FloatField(ypos, popupSideMargin, fileInfoPopupWidth - 2 * popupSideMargin - fileInfoPopupWidth / 3, TextAlignment::Left, 1, strings->objectHeight, "mm");
	ypos += rowTextHeight;
	fpFilamentField = new IntegerField(ypos, popupSideMargin, fileInfoPopupWidth - 2 * popupSideMargin - fileInfoPopupWidth / 3, TextAlignment::Left, strings->filamentNeeded, "mm");
	ypos += rowTextHeight;
	fpLastModifiedField = new TextField(ypos, popupSideMargin, fileInfoPopupWidth - 2 * popupSideMargin - fileInfoPopupWidth / 3, TextAlignment::Left, strings->lastModified, lastModifiedText.c_str());
	ypos += rowTextHeight;
	fpPrintTimeField = new TextField(ypos, popupSideMargin, fileInfoPopupWidth - 2 * popupSideMargin - fileInfoPopupWidth / 3, TextAlignment::Left, strings->estimatedPrintTime, printTimeText.c_str());
	fileDetailPopup->AddField(fpNameField);
	fileDetailPopup->AddField(fpSizeField);
	fileDetailPopup->AddField(fpLayerHeightField);
	fileDetailPopup->AddField(fpHeightField);
	fileDetailPopup->AddField(fpFilamentField);
	fileDetailPopup->AddField(fpGeneratedByField);
	fileDetailPopup->AddField(fpLastModifiedField);
	fileDetailPopup->AddField(fpPrintTimeField);
	fileDetailPopup->AddField(fpThumbnail);

	// Add the buttons
	DisplayField::SetDefaultColours(colours.popupButtonTextColour, colours.popupButtonBackColour);
	fileDetailPopup->AddField(new TextButton(popupTopMargin + 10 * rowTextHeight, popupSideMargin, fileInfoPopupWidth/3 - 2 * popupSideMargin, strings->print, evPrintFile));
	fileDetailPopup->AddField(new TextButton(popupTopMargin + 10 * rowTextHeight, fileInfoPopupWidth/3 + popupSideMargin, fileInfoPopupWidth/3 - 2 * popupSideMargin, strings->simulate, evSimulateFile));
	fileDetailPopup->AddField(new IconButton(popupTopMargin + 10 * rowTextHeight, (2 * fileInfoPopupWidth)/3 + popupSideMargin, fileInfoPopupWidth/3 - 2 * popupSideMargin, IconTrash, evDeleteFile));
}

// Create the "Are you sure?" popup
static void CreateAreYouSurePopup(const ColourScheme& colours)
{
	areYouSurePopup = new PopupWindow(areYouSurePopupHeight, areYouSurePopupWidth, colours.popupBackColour, colours.popupBorderColour);
	DisplayField::SetDefaultColours(colours.popupTextColour, colours.popupBackColour);
	areYouSurePopup->AddField(areYouSureTextField = new StaticTextField(popupSideMargin, margin, areYouSurePopupWidth - 2 * margin, TextAlignment::Centre, nullptr));
	areYouSurePopup->AddField(areYouSureQueryField = new StaticTextField(popupTopMargin + rowHeight, margin, areYouSurePopupWidth - 2 * margin, TextAlignment::Centre, nullptr));

	DisplayField::SetDefaultColours(colours.popupButtonTextColour, colours.popupButtonBackColour);
	areYouSurePopup->AddField(new IconButton(popupTopMargin + 2 * rowHeight, popupSideMargin, areYouSurePopupWidth/2 - 2 * popupSideMargin, IconOk, evYes));
	areYouSurePopup->AddField(new IconButton(popupTopMargin + 2 * rowHeight, areYouSurePopupWidth/2 + 10, areYouSurePopupWidth/2 - 2 * popupSideMargin, IconCancel, evCancel));
}

static void CreateScreensaverPopup()
{
	screensaverPopup = new PopupWindow(max(DisplayX, DisplayY), max(DisplayX, DisplayY), black, black, false);
	DisplayField::SetDefaultColours(white, black);
	static const char * text = "Touch to wake up";
	screensaverTextWidth = DisplayField::GetTextWidth(text, DisplayX);
	screensaverPopup->AddField(screensaverText = new StaticTextField(row1, margin, screensaverTextWidth, TextAlignment::Left, text));
}

static void CreateFirmwareUpdatePopup()
{
	firmwareUpdatePopup = new PopupWindow(max(DisplayX, DisplayY), max(DisplayX, DisplayY), black, black, false);
	DisplayField::SetDefaultColours(white, black);
	static const char * text = "Updating firmware";
	const int textWidth = DisplayField::GetTextWidth(text, DisplayX);
	firmwareUpdatePopup->AddField(new StaticTextField(DisplayY/2-rowHeight/2, DisplayX/2-textWidth/2, textWidth, TextAlignment::Left, text));
}

// Create the baud rate adjustment popup
static void CreateBaudRatePopup(const ColourScheme& colours)
{
	static const char* const baudPopupText[] = { "9600", "19200", "38400", "57600", "115200" };
	static const int baudPopupParams[] = { 9600, 19200, 38400, 57600, 115200 };
	baudPopup = CreateIntPopupBar(colours, fullPopupWidth, 5, baudPopupText, baudPopupParams, evAdjustBaudRate, evAdjustBaudRate);
}

// Create the volume adjustment popup
static void CreateVolumePopup(const ColourScheme& colours)
{
	static_assert(Buzzer::MaxVolume == 5, "MaxVolume assumed to be 5 here");
	static const char* const volumePopupText[Buzzer::MaxVolume + 1] = { "0", "1", "2", "3", "4", "5" };
	volumePopup = CreateIntPopupBar(colours, fullPopupWidth, ARRAY_SIZE(volumePopupText), volumePopupText, nullptr, evAdjustVolume, evAdjustVolume);
}

// Create the volume adjustment popup
static void CreateInfoTimeoutPopup(const ColourScheme& colours)
{
	static const char* const infoTimeoutPopupText[Buzzer::MaxVolume + 1] = { "0", "2", "5", "10" };
	static const int values[] = { 0, 2, 5, 10 };
	infoTimeoutPopup = CreateIntPopupBar(colours, fullPopupWidth, ARRAY_SIZE(infoTimeoutPopupText), infoTimeoutPopupText, values, evAdjustInfoTimeout, evAdjustInfoTimeout);
}

// Create the screensaver timeout adjustment popup
static void CreateScreensaverTimeoutPopup(const ColourScheme& colours)
{
	static const char* const screensaverTimeoutPopupText[Buzzer::MaxVolume + 1] = { "off", "60", "120", "180", "240", "300" };
	static const int values[] = { 0, 60, 120, 180, 240, 300 };
	screensaverTimeoutPopup = CreateIntPopupBar(colours, fullPopupWidth, ARRAY_SIZE(screensaverTimeoutPopupText), screensaverTimeoutPopupText, values, evAdjustScreensaverTimeout, evAdjustScreensaverTimeout);
}

// Create the babystep amount adjustment popup
static void CreateBabystepAmountPopup(const ColourScheme& colours)
{
	static const int values[] = { 0, 1, 2, 3 };
	babystepAmountPopup = CreateIntPopupBar(colours, fullPopupWidth, ARRAY_SIZE(babystepAmounts), babystepAmounts, values, evAdjustBabystepAmount, evAdjustBabystepAmount);
}

// Create the feedrate amount adjustment popup
static void CreateFeedrateAmountPopup(const ColourScheme& colours)
{
	static const char* const feedrateText[] = {"600", "1200", "2400", "6000", "12000"};
	static const int values[] = { 600, 1200, 2400, 6000, 12000 };
	feedrateAmountPopup = CreateIntPopupBar(colours, fullPopupWidth, ARRAY_SIZE(feedrateText), feedrateText, values, evAdjustFeedrate, evAdjustFeedrate);
}

// Create the colour scheme change popup
static void CreateColoursPopup(const ColourScheme& colours)
{
	if (NumColourSchemes >= 2)
	{
		// Put all the colour scheme names in a single _ecv_array for the call to CreateIntPopupBar
		const char* coloursPopupText[NumColourSchemes];
		for (size_t i = 0; i < NumColourSchemes; ++i)
		{
			coloursPopupText[i] = strings->colourSchemeNames[i];
		}
		coloursPopup = CreateIntPopupBar(colours, fullPopupWidth, NumColourSchemes, coloursPopupText, nullptr, evAdjustColours, evAdjustColours);
	}
	else
	{
		coloursPopup = nullptr;
	}
}

// Create the language popup (currently only affects the keyboard layout)
static void CreateLanguagePopup(const ColourScheme& colours)
{
	languagePopup = new PopupWindow(popupBarHeight, fullPopupWidth, colours.popupBackColour, colours.popupBorderColour);
	DisplayField::SetDefaultColours(colours.popupButtonTextColour, colours.popupButtonBackColour);
	PixelNumber step = (fullPopupWidth - 2 * popupSideMargin + popupFieldSpacing)/NumLanguages;
	for (unsigned int i = 0; i < NumLanguages; ++i)
	{
		languagePopup->AddField(new TextButton(popupSideMargin, popupSideMargin + i * step, step - popupFieldSpacing, LanguageTables[i].languageName, evAdjustLanguage, i));
	}
}

// Shared keyboard layouts are used by the Console
// keyboard and for the popup editor used by RRF alert dialogs.
static const char* _ecv_array const keysEN[8] = { "1234567890-+", "QWERTYUIOP[]", "ASDFGHJKL:@", "ZXCVBNM,./", "!\"#$%^&*()_=", "qwertyuiop{}", "asdfghjkl;'", "zxcvbnm<>?" };
static const char* _ecv_array const keysDE[8] = { "1234567890-+", "QWERTZUIOP[]", "ASDFGHJKL:@", "YXCVBNM,./", "!\"#$%^&*()_=", "qwertzuiop{}", "asdfghjkl;'", "yxcvbnm<>?" };
static const char* _ecv_array const keysFR[8] = { "1234567890-+", "AZERTWUIOP[]", "QSDFGHJKLM@", "YXCVBN.,:/", "!\"#$%^&*()_=", "azertwuiop{}", "qsdfghjklm'", "yxcvbn<>;?" };
static const char* _ecv_array const * const keyboardLayouts[] = {
        keysEN, keysDE, keysFR, keysEN, keysEN, keysEN, keysEN, keysEN,
#if USE_CYRILLIC_CHARACTERS
        keysEN, keysEN,
#elif USE_JAPANESE_CHARACTERS
        keysEN,
#endif
};

static const char* _ecv_array const *GetKeyboardLayout(uint32_t language)
{
    if (language >= NumLanguages) language = 0;
    return keyboardLayouts[language];
}

// Create the pop-up keyboard
static void CreateKeyboardPopup(uint32_t language, ColourScheme colours)
{
	static_assert(ARRAY_SIZE(keyboardLayouts) >= NumLanguages, "Wrong number of keyboard entries");

	keyboardPopup = new StandardPopupWindow(keyboardPopupHeight, keyboardPopupWidth, colours.popupBackColour, colours.popupBorderColour, colours.popupInfoTextColour, colours.buttonImageBackColour, nullptr, keyboardTopMargin);

	// Add the text area in which the command is built
	DisplayField::SetDefaultColours(colours.popupInfoTextColour, colours.popupInfoBackColour);		// need a different background colour
	popupUserCommandField = new TextField(keyboardTopMargin + labelRowAdjust, popupSideMargin, keyboardPopupWidth - 2 * popupSideMargin - closeButtonWidth - popupFieldSpacing, TextAlignment::Left, nullptr, "_");
	popupUserCommandField->SetLabel(userCommandBuffers[currentUserCommandBuffer].c_str());
	keyboardPopup->AddField(popupUserCommandField);

	if (language >= NumLanguages)
	{
		language = 0;
	}

	currentKeyboard = GetKeyboardLayout(language);
	PixelNumber row = keyboardTopMargin + keyButtonVStep;

	for (size_t i = 0; i < 4; ++i)
	{
		DisplayField::SetDefaultColours(colours.popupButtonTextColour, colours.popupButtonBackColour);
		// New code using CharButtonRow to economise on RAM at the expense of more flash memory usage
		const PixelNumber column = popupSideMargin + (i * keyButtonHStep)/3;
		keyboardRows[i] = new CharButtonRow(row, column, keyButtonWidth, keyButtonHStep, currentKeyboard[i], evKey);
		keyboardPopup->AddField(keyboardRows[i]);
		DisplayField::SetDefaultColours(colours.popupButtonTextColour, colours.buttonImageBackColour);
		switch (i)
		{
		case 0:
			keyboardPopup->AddField(new IconButton(row, keyboardPopupWidth - popupSideMargin - (5 * keyButtonWidth)/4, (5 * keyButtonWidth)/4, IconBackspace, evBackspace));
			break;

		case 2:
			keyboardPopup->AddField(new TextButton(row, keyboardPopupWidth - popupSideMargin - (3 * keyButtonWidth)/2, (3 * keyButtonWidth)/2, UP_ARROW, evUp));
			break;

		case 3:
			keyboardPopup->AddField(new TextButton(row, keyboardPopupWidth - popupSideMargin - (3 * keyButtonWidth)/2, (3 * keyButtonWidth)/2, DOWN_ARROW, evDown));
			break;

		default:
			break;
		}
		row += keyButtonVStep;
	}

	// Add the shift, space and enter keys
	const PixelNumber keyButtonHSpace = keyButtonHStep - keyButtonWidth;
	const PixelNumber wideKeyButtonWidth = (keyboardPopupWidth - 2 * popupSideMargin - 2 * keyButtonHSpace)/5;
	DisplayField::SetDefaultColours(colours.popupButtonTextColour, colours.popupButtonBackColour);
	keyboardPopup->AddField(new TextButton(row, popupSideMargin, wideKeyButtonWidth, "Shift", evShift, 0));
	keyboardPopup->AddField(new TextButton(row, popupSideMargin + wideKeyButtonWidth + keyButtonHSpace, 2 * wideKeyButtonWidth, "", evKey, (int)' '));
	DisplayField::SetDefaultColours(colours.popupButtonTextColour, colours.buttonImageBackColour);
	keyboardPopup->AddField(new IconButton(row, popupSideMargin + 3 * wideKeyButtonWidth + 2 * keyButtonHSpace, wideKeyButtonWidth, IconEnter, evSendKeyboardCommand));

	keyboardDataHandler = SendGcode;
}

#if DISPLAY_X == 800
// Add a permanently-visible keyboard directly to the Console page. Because it
// is part of messageRoot, the left navigation rail remains fully clickable.
static void CreateConsoleKeyboardFields(uint32_t language, const ColourScheme& colours)
{
	consoleCurrentKeyboard = GetKeyboardLayout(language);
	const PixelNumber baseX = mainContentLeft;
	const PixelNumber baseY = consoleKeyboardY;
	const PixelNumber localWidth = mainContentWidth;

	DisplayField::SetDefaultColours(colours.popupInfoTextColour, colours.popupInfoBackColour);
	userCommandField = new TextField(baseY + 4,
			baseX + popupSideMargin, localWidth - 2*popupSideMargin, TextAlignment::Left, nullptr, "_");
	userCommandField->SetLabel(userCommandBuffers[currentUserCommandBuffer].c_str());
	mgr.AddField(userCommandField);

	// Larger fixed keys for finger typing. We keep the 19x21 font, so every
	// character remains safely inside the rounded key with no font clipping.
	PixelNumber row = baseY + 35;
	for (size_t i = 0; i < 4; ++i)
	{
		DisplayField::SetDefaultColours(colours.popupButtonTextColour, colours.popupButtonBackColour);
		const PixelNumber column = baseX + popupSideMargin + (i * consoleKeyHStep)/3;
		consoleKeyboardRows[i] = new CharButtonRow(row, column, consoleKeyWidth, consoleKeyHStep, consoleCurrentKeyboard[i], evKey);
		consoleKeyboardRows[i]->SetHeight(consoleKeyHeight);
		mgr.AddField(consoleKeyboardRows[i]);
		DisplayField::SetDefaultColours(colours.popupButtonTextColour, colours.buttonImageBackColour);
		switch (i)
		{
		case 0:
		{
			IconButton * const b = new IconButton(row, baseX + localWidth - popupSideMargin - 58, 58, IconBackspace, evBackspace);
			b->SetHeight(consoleKeyHeight);
			mgr.AddField(b);
			break;
		}
		case 2:
		{
			TextButton * const b = new TextButton(row, baseX + localWidth - popupSideMargin - 68, 68, UP_ARROW, evUp);
			b->SetHeight(consoleKeyHeight);
			mgr.AddField(b);
			break;
		}
		case 3:
		{
			TextButton * const b = new TextButton(row, baseX + localWidth - popupSideMargin - 68, 68, DOWN_ARROW, evDown);
			b->SetHeight(consoleKeyHeight);
			mgr.AddField(b);
			break;
		}
		default: break;
		}
		row += consoleKeyVStep;
	}

	const PixelNumber keyGap = consoleKeyHStep - consoleKeyWidth;
	const PixelNumber wideKeyButtonWidth = (localWidth - 2*popupSideMargin - 2*keyGap)/5;
	DisplayField::SetDefaultColours(colours.popupButtonTextColour, colours.popupButtonBackColour);
	TextButton * const shift = new TextButton(row, baseX + popupSideMargin, wideKeyButtonWidth, "Shift", evShift, 0);
	shift->SetHeight(consoleKeyHeight);
	mgr.AddField(shift);
	TextButton * const space = new TextButton(row, baseX + popupSideMargin + wideKeyButtonWidth + keyGap,
			2*wideKeyButtonWidth, "", evKey, (int)' ');
	space->SetHeight(consoleKeyHeight);
	mgr.AddField(space);
	DisplayField::SetDefaultColours(colours.popupButtonTextColour, colours.buttonImageBackColour);
	IconButton * const enter = new IconButton(row, baseX + popupSideMargin + 3*wideKeyButtonWidth + 2*keyGap,
			wideKeyButtonWidth, IconEnter, evSendKeyboardCommand);
	enter->SetHeight(consoleKeyHeight);
	mgr.AddField(enter);
}

#endif

// Create the babystep popup
static void CreateBabystepPopup(const ColourScheme& colours)
{
	babystepPopup = new StandardPopupWindow(babystepPopupHeight, babystepPopupWidth, colours.popupBackColour, colours.popupBorderColour, colours.popupTextColour, colours.buttonImageBackColour,
			strings->babyStepping);
	PixelNumber ypos = popupTopMargin + babystepRowSpacing;
	DisplayField::SetDefaultColours(colours.popupTextColour, colours.popupBackColour);
	babystepPopup->AddField(babystepOffsetField = new FloatField(ypos, popupSideMargin, babystepPopupWidth - 2 * popupSideMargin, TextAlignment::Left, 3, strings->currentZoffset, "mm"));
	ypos += babystepRowSpacing;
	DisplayField::SetDefaultColours(colours.popupTextColour, colours.buttonImageBackColour);
	const PixelNumber width = CalcWidth(2, babystepPopupWidth - 2 * popupSideMargin);
	babystepPopup->AddField(babystepMinusButton = new TextButtonWithLabel(ypos, CalcXPos(0, width, popupSideMargin), width, babystepAmounts[nvData.GetBabystepAmountIndex()], evBabyStepMinus, nullptr, LESS_ARROW " "));
	babystepPopup->AddField(babystepPlusButton = new TextButtonWithLabel(ypos, CalcXPos(1, width, popupSideMargin), width, babystepAmounts[nvData.GetBabystepAmountIndex()], evBabyStepPlus, nullptr, MORE_ARROW " "));
}

#if DISPLAY_X == 800
static PixelNumber HomeCardWidth()
{
	const PixelNumber innerWidth = mainContentWidth - 32;
	return (innerWidth - 3 * 8) / 4;
}

static PixelNumber HomeCardX(size_t slot)
{
	const PixelNumber cardWidth = HomeCardWidth();
	return mainContentLeft + 16 + (slot % 4) * (cardWidth + 8);
}

static PixelNumber HomeCardY(size_t slot)
{
	return 64 + (slot / 4) * (84 + 8);
}

static void SetHomeToolButtonNormalGeometry(size_t slot)
{
	if (slot < MaxSlots && toolButtons[slot] != nullptr)
	{
		const PixelNumber cardWidth = HomeCardWidth();
		// Use three strictly separated vertical zones inside each 84px card:
		// icon/title y+3..22, live temperature y+25..56, setpoints y+59..83.
		// No zone shares a pixel with another, so text/background repainting can
		// never clip the rounded A/S boxes.
		toolButtons[slot]->SetPosition(HomeCardX(slot) + 8, HomeCardY(slot) + 3);
		toolButtons[slot]->SetPositionAndWidth(HomeCardX(slot) + 8, cardWidth - 16);
		toolButtons[slot]->SetHeight(20);
		toolButtons[slot]->SetShowIcon(true);
	}
}

static void ApplyHomeAuxSlot(size_t slot)
{
	if (slot >= MaxSlots || slot < numToolColsUsed || toolButtons[slot] == nullptr || homeAuxValues[slot] == nullptr)
	{
		return;
	}

	const PixelNumber cardWidth = HomeCardWidth();
	const uint8_t config = nvData.GetHomeSlotConfig(slot);
	const uint8_t kind = config & 0xF0;
	const uint8_t index = config & 0x0F;

	mgr.Show(currentTemps[slot], false);
	mgr.Show(activeTemps[slot], false);
	mgr.Show(standbyTemps[slot], false);
	mgr.Show(extrusionFactors[slot], false);
	if (homeMacroActions[slot] != nullptr) { mgr.Show(homeMacroActions[slot], false); }

	toolButtons[slot]->SetPosition(HomeCardX(slot) + 8, HomeCardY(slot) + 5);
	toolButtons[slot]->SetPositionAndWidth(HomeCardX(slot) + 8, cardWidth - 16);
	toolButtons[slot]->SetHeight(32);
	toolButtons[slot]->SetEvent(evConfigureHomeSlot, (int)slot);
	toolButtons[slot]->SetColours(colours->buttonTextColour, colours->buttonImageBackColour);
	mgr.Show(toolButtons[slot], true);

	if (kind == 0x10 && index < MaxPrintFanControls && (availableFanMask & (uint8_t)(1u << index)) != 0)
	{
		toolButtons[slot]->SetShowIcon(false);
		toolButtons[slot]->SetPrintText(true);
		toolButtons[slot]->SetText(GetFanDisplayName(index));
		homeAuxValues[slot]->SetIcon(VectorIcon::Fan);
		homeAuxValues[slot]->SetEvent(evAdjustFan, (int)index);
		homeAuxValues[slot]->SetValue(lastFanPercent[index]);
		mgr.Show(homeAuxValues[slot], true);
	}
	else if (kind == 0x20 && index < MaxPrintGpOutControls && (availableGpOutMask & (uint8_t)(1u << index)) != 0)
	{
		toolButtons[slot]->SetShowIcon(false);
		toolButtons[slot]->SetPrintText(true);
		toolButtons[slot]->SetText(GetGpOutDisplayName(index));
		homeAuxValues[slot]->SetIcon(VectorIcon::Output);
		homeAuxValues[slot]->SetEvent(evAdjustGpOut, (int)index);
		homeAuxValues[slot]->SetValue(lastGpOutPercent[index]);
		mgr.Show(homeAuxValues[slot], true);
	}
	else if (kind == 0x30 && index < 8 && (availableHomeMacroMask & (uint8_t)(1u << index)) != 0)
	{
		toolButtons[slot]->SetShowIcon(false);
		toolButtons[slot]->SetPrintText(true);
		toolButtons[slot]->SetText(GetHomeMacroDisplayName(index));
		homeAuxValues[slot]->SetEvent(evNull, 0);
		mgr.Show(homeAuxValues[slot], false);
		if (homeMacroActions[slot] != nullptr)
		{
			homeMacroActions[slot]->SetEvent(evMacroControlPage, homeMacroFiles[index].c_str());
			mgr.Show(homeMacroActions[slot], true);
		}
	}
	else
	{
		toolButtons[slot]->SetPosition(HomeCardX(slot) + 8, HomeCardY(slot) + 18);
		toolButtons[slot]->SetHeight(48);
		toolButtons[slot]->SetShowIcon(true);
		toolButtons[slot]->SetPrintText(false);
		toolButtons[slot]->SetText(nullptr);
		toolButtons[slot]->SetIcon(VectorIcon::Plus);
		homeAuxValues[slot]->SetEvent(evNull, 0);
		mgr.Show(homeAuxValues[slot], false);
	}
}

static void CreateHomeSlotPopup(const ColourScheme& colours)
{
	homeSlotPopup = new StandardPopupWindow(420, 780, colours.popupBackColour, colours.popupBorderColour,
			colours.popupTextColour, colours.buttonImageBackColour, "HOME SLOT", 8);
	DisplayField::SetDefaultColours(colours.popupButtonTextColour, colours.popupButtonBackColour,
			colours.popupBorderColour, 0, colours.buttonPressedBackColour, 0, colours.pal);
	homeSlotPicker = new HomeSlotPickerField(54, 18, 744, 322, evAssignHomeSlot);
	for (size_t i = 0; i < MaxPrintFanControls; ++i) { homeSlotPicker->SetFanLabel(i, GetFanDisplayName(i)); }
	for (size_t i = 0; i < MaxPrintGpOutControls; ++i) { homeSlotPicker->SetOutputLabel(i, GetGpOutDisplayName(i)); }
	for (size_t i = 0; i < 8; ++i) { homeSlotPicker->SetMacroLabel(i, GetHomeMacroDisplayName(i)); }
	homeSlotPicker->SetAvailability(availableFanMask, availableGpOutMask, availableHomeMacroMask);
	homeSlotPopup->AddField(homeSlotPicker);
}
#endif

// Create the grid of heater icons and temperatures
static void CreateTemperatureGrid(const ColourScheme& colours)
{
#if DISPLAY_X == 800
	// Heater/tool slots include the bed and available tools. The complete
	// 4x2 grid uses every available card position.
	const PixelNumber cardTop = 64;
	const PixelNumber cardHeight = 84;
	const PixelNumber rowGap = 8;
	const PixelNumber cardWidth = HomeCardWidth();

	DisplayField::SetDefaultColours(colours.labelTextColour, colours.defaultBackColour);
	debugField = new StaticTextField(0, 0, 1, TextAlignment::Left, "debug");
	debugField->Show(false);
	mgr.AddField(debugField);

	for (unsigned int i = 0; i < MaxSlots; ++i)
	{
		const unsigned int rowIndex = i / 4;
		const PixelNumber column = HomeCardX(i);
		const PixelNumber y = cardTop + rowIndex * (cardHeight + rowGap);

		DisplayField::SetDefaultColours(colours.buttonTextColour, colours.buttonTextBackColour,
				colours.buttonTextBackColour, 0, colours.buttonPressedBackColour, 0, colours.pal);
		VectorIconButton * const b = new VectorIconButton(y + 3, column + 8, cardWidth - 16, 20,
				i == 0 ? VectorIcon::Bed : VectorIcon::Nozzle, evSelectHead, nullptr, DEFAULT_FONT, (int)i);
		b->Show(false);
		toolButtons[i] = b;
		mgr.AddField(b);

		DisplayField::SetDefaultColours(colours.buttonTextColour, colours.popupButtonBackColour,
				colours.buttonBorderColour, 0, colours.buttonPressedBackColour, 0, colours.pal);
		CompactPercentButton * const aux = new CompactPercentButton(y + 47, column + 8, cardWidth - 16, VectorIcon::Fan);
		aux->SetEvent(evNull, 0);
		aux->Show(false);
		homeAuxValues[i] = aux;
		mgr.AddField(aux);

		DisplayField::SetDefaultColours(colours.buttonTextColour, colours.popupButtonBackColour,
				colours.buttonBorderColour, 0, colours.buttonPressedBackColour, 0, colours.pal);
		VectorIconButton * const macroAction = new VectorIconButton(y + 45, column + 8, cardWidth - 16, 34,
				VectorIcon::Macro, evNull, "RUN", DEFAULT_FONT, 0);
		macroAction->Show(false);
		homeMacroActions[i] = macroAction;
		mgr.AddField(macroAction);

		DisplayField::SetDefaultColours(colours.buttonTextColour, colours.popupButtonBackColour,
				colours.buttonBorderColour, 0, colours.buttonPressedBackColour, 0, colours.pal);
		const PixelNumber targetWidth = (cardWidth - 26) / 2;
		IntegerButton *ib = new IntegerButton(y + 59, column + 8, targetWidth, "A ", "°");
		ib->SetEvent(evAdjustToolActiveTemp, i);
		ib->SetValue(0);
		ib->Show(false);
		activeTemps[i] = ib;
		mgr.AddField(ib);

		ib = new IntegerButton(y + 59, column + 14 + targetWidth, targetWidth, "S ", "°");
		ib->SetEvent(evAdjustToolStandbyTemp, i);
		ib->SetValue(0);
		ib->Show(false);
		standbyTemps[i] = ib;
		mgr.AddField(ib);

		// Live temperature occupies y+25..56 (32px font). A/S start at y+59,
		// leaving a real 2px blank separator between the rendered regions.
		// It is also added after A/S in the linked-list build order so its
		// background is painted before the rounded buttons.
		DisplayField::SetDefaultFont(UI_LARGE_FONT);
		DisplayField::SetDefaultColours(colours.infoTextColour, colours.buttonTextBackColour);
		FloatField * const f = new FloatField(y + 25, column + 8, cardWidth - 16,
				TextAlignment::Centre, 1, nullptr, "°C");
		f->Show(false);
		currentTemps[i] = f;
		mgr.AddField(f);
		DisplayField::SetDefaultFont(DEFAULT_FONT);

		mgr.AddField(new ModernPanelField(y, column, cardWidth, cardHeight,
				colours.buttonTextBackColour, colours.buttonBorderColour, 12));
	}
#else
	// 480x272 layout.
	DisplayField::SetDefaultColours(colours.stopButtonTextColour, colours.stopButtonBackColour);
	mgr.AddField(new TextButton(row2, margin, bedColumn - fieldSpacing - margin - 16, strings->stop, evEmergencyStop));

	DisplayField::SetDefaultColours(colours.labelTextColour, colours.defaultBackColour);
	mgr.AddField(debugField = new StaticTextField(row1 + labelRowAdjust, margin, bedColumn - fieldSpacing - margin, TextAlignment::Left, "debug"));
	mgr.AddField(new StaticTextField(row3 + labelRowAdjust, margin, bedColumn - fieldSpacing - margin, TextAlignment::Right, strings->current));
	mgr.AddField(new StaticTextField(row4 + labelRowAdjust, margin, bedColumn - fieldSpacing - margin, TextAlignment::Right, strings->active));
	mgr.AddField(new StaticTextField(row5 + labelRowAdjust, margin, bedColumn - fieldSpacing - margin, TextAlignment::Right, strings->standby));

	for (unsigned int i = 0; i < MaxSlots; ++i)
	{
		const PixelNumber column = ((tempButtonWidth + fieldSpacing) * i) + bedColumn;
		DisplayField::SetDefaultColours(colours.buttonTextColour, colours.buttonImageBackColour);
		IconButtonWithText * const b = new IconButtonWithText(row2, column, tempButtonWidth, i == 0 ? IconBed : IconNozzle, evSelectHead, i, i);
		b->Show(false);
		toolButtons[i] = b;
		mgr.AddField(b);

		DisplayField::SetDefaultColours(colours.infoTextColour, colours.infoBackColour);
		FloatField * const f = new FloatField(row3 + labelRowAdjust, column, tempButtonWidth, TextAlignment::Centre, 1);
		f->Show(false);
		currentTemps[i] = f;
		mgr.AddField(f);

		DisplayField::SetDefaultColours(colours.buttonTextColour, colours.buttonTextBackColour);
		IntegerButton *ib = new IntegerButton(row4, column, tempButtonWidth);
		ib->SetEvent(evAdjustToolActiveTemp, i);
		ib->SetValue(0);
		ib->Show(false);
		activeTemps[i] = ib;
		mgr.AddField(ib);

		ib = new IntegerButton(row5, column, tempButtonWidth);
		ib->SetEvent(evAdjustToolStandbyTemp, i);
		ib->SetValue(0);
		ib->Show(false);
		standbyTemps[i] = ib;
		mgr.AddField(ib);
	}
#endif
}

// Create the extra fields for the Control tab
static void CreateControlTabFields(const ColourScheme& colours)
{
	mgr.SetRoot(commonRoot);

#if DISPLAY_X == 800
	const PixelNumber pageLeft = mainContentLeft + 16;
	const PixelNumber innerWidth = mainContentWidth - 32;

	const PixelNumber machineTop = 340;
	const PixelNumber machineHeight = 66;
	const PixelNumber machineWidth = 2 * HomeCardWidth() + 8;
	const PixelNumber homeGap = 6;
	const PixelNumber homeAreaWidth = innerWidth - machineWidth - 8;
	const PixelNumber homeWidth = (homeAreaWidth - 3 * homeGap) / 4;

	DisplayField::SetDefaultColours(colours.buttonTextColour, colours.notHomedButtonBackColour,
			colours.buttonBorderColour, 0, colours.buttonPressedBackColour, 0, colours.pal);
	homeAllButton = new VectorIconButton(machineTop, pageLeft, homeWidth, machineHeight,
			VectorIcon::HomeAll, evSendCommand, nullptr, DEFAULT_FONT, "G28");
	mgr.AddField(homeAllButton);
	for (size_t i = 0; i < MaxDisplayableAxes; ++i)
	{
		const PixelNumber hx = pageLeft + (PixelNumber)(i + 1) * (homeWidth + homeGap);
		homeButtons[i] = new VectorIconButton(machineTop, hx, homeWidth, machineHeight,
				VectorIcon::HomeAll, evHomeAxis, axisNames[i], DEFAULT_FONT, axisNames[i]);
		homeButtons[i]->SetShowIcon(false);
		homeButtons[i]->Show(i < 3);
		mgr.AddField(homeButtons[i]);
	}

	const PixelNumber machineLeft = pageLeft + homeAreaWidth + 8;
	DisplayField::SetDefaultColours(colours.labelTextColour, colours.buttonTextBackColour);
	homeMachineTitleField = new StaticTextField(machineTop + 5, machineLeft + 12, 112, TextAlignment::Left, "MACHINE");
	mgr.AddField(homeMachineTitleField);
	homeNotHomedText.copy("NOT HOMED: X Y Z");
	DisplayField::SetDefaultColours(colours.errorTextColour, colours.notHomedButtonBackColour);
	homeNotHomedField = new StaticTextField(machineTop + 5, machineLeft + 132, machineWidth - 144, TextAlignment::Right, homeNotHomedText.c_str());
	mgr.AddField(homeNotHomedField);

	DisplayField::SetDefaultColours(colours.infoTextColour, colours.buttonTextBackColour);
	const PixelNumber axisLeft = machineLeft + 12;
	const PixelNumber axisStep = (machineWidth - 24) / 3;
	for (size_t i = 0; i < MaxDisplayableAxes; ++i)
	{
		FloatField * const f = new FloatField(machineTop + 34, axisLeft + (PixelNumber)i * axisStep,
				axisStep - 4, TextAlignment::Left, 1, axisNames[i], " mm");
		controlTabAxisPos[i] = f;
		mgr.AddField(f);
		f->Show(i < 3);
	}

	zprobeBuf[0] = 0;
	zProbe = new TextField(0, 0, 1, TextAlignment::Left, "P", zprobeBuf.c_str());
	zProbe->Show(false);
	mgr.AddField(zProbe);
	dashboardStatusField = nullptr;
	dashboardProgressBar = nullptr;
	mgr.AddField(new ModernPanelField(machineTop, machineLeft, machineWidth, machineHeight,
			colours.buttonTextBackColour, colours.buttonBorderColour, 12));

	bedCompButton = new VectorIconButton(0, 0, 1, 1, VectorIcon::BedMesh, evSendCommand, nullptr, DEFAULT_FONT, "G32");
	bedCompButton->Show(false);
	mgr.AddField(bedCompButton);

	const PixelNumber actionTop = 414;
	const PixelNumber actionHeight = 58;
	const PixelNumber actionGap = 8;
	const PixelNumber actionWidth = (innerWidth - 3 * actionGap) / 4;
	DisplayField::SetDefaultColours(colours.buttonTextColour, colours.buttonTextBackColour,
			colours.buttonBorderColour, 0, colours.buttonPressedBackColour, 0, colours.pal);
	filesButton = new VectorIconButton(actionTop, pageLeft, actionWidth, actionHeight,
			VectorIcon::Files, evListFiles, "Files", DEFAULT_FONT, 0);
	moveButton = new VectorIconButton(actionTop, pageLeft + (actionWidth + actionGap), actionWidth, actionHeight,
			VectorIcon::Move, evMovePopup, "Move", DEFAULT_FONT, 0);
	extrudeButton = new VectorIconButton(actionTop, pageLeft + 2 * (actionWidth + actionGap), actionWidth, actionHeight,
			VectorIcon::Extrude, evExtrudePopup, "Extrude", DEFAULT_FONT, 0);
	macroButton = new VectorIconButton(actionTop, pageLeft + 3 * (actionWidth + actionGap), actionWidth, actionHeight,
			VectorIcon::Macro, evListMacros, "Macros", DEFAULT_FONT, 0);
	mgr.AddField(filesButton);
	mgr.AddField(moveButton);
	mgr.AddField(extrudeButton);
	mgr.AddField(macroButton);
#else
	DisplayField::SetDefaultColours(colours.infoTextColour, colours.infoBackColour);
	PixelNumber column = mainContentLeft + margin;
	PixelNumber xyFieldWidth = (mainContentWidth - (2 * margin) - (MaxDisplayableAxes * fieldSpacing))/(MaxDisplayableAxes + 1);
	for (size_t i = 0; i < MaxDisplayableAxes; ++i)
	{
		FloatField * const f = new FloatField(row6p3 + labelRowAdjust, column, xyFieldWidth, TextAlignment::Left, 2, axisNames[i]);
		controlTabAxisPos[i] = f;
		mgr.AddField(f);
		f->Show(i < MIN_AXES);
		column += xyFieldWidth + fieldSpacing;
	}
	zprobeBuf[0] = 0;
	mgr.AddField(zProbe = new TextField(row6p3 + labelRowAdjust, column, DISPLAY_X - column - margin, TextAlignment::Left, "P", zprobeBuf.c_str()));
	DisplayField::SetDefaultColours(colours.buttonTextColour, colours.notHomedButtonBackColour);
	homeAllButton = AddIconButton(row7p7, 0, MaxDisplayableAxes + 2, IconHomeAll, evSendCommand, "G28");
	homeButtons[0] = AddIconButtonWithText(row7p7, 1, MaxDisplayableAxes + 2, IconHomeAll, evHomeAxis, axisNames[0], axisNames[0]);
	homeButtons[1] = AddIconButtonWithText(row7p7, 2, MaxDisplayableAxes + 2, IconHomeAll, evHomeAxis, axisNames[1], axisNames[1]);
	homeButtons[2] = AddIconButtonWithText(row7p7, 3, MaxDisplayableAxes + 2, IconHomeAll, evHomeAxis, axisNames[2], axisNames[2]);
#if MaxDisplayableAxes > 3
	homeButtons[3] = AddIconButtonWithText(row7p7, 4, MaxDisplayableAxes + 2, IconHomeAll, evHomeAxis, axisNames[3], axisNames[3]);
	homeButtons[3]->Show(false);
#endif
	DisplayField::SetDefaultColours(colours.buttonTextColour, colours.buttonImageBackColour);
	bedCompButton = AddIconButton(row7p7, MaxDisplayableAxes + 1, MaxDisplayableAxes + 2, IconBedComp, evSendCommand, "G32");
	filesButton = AddIconButton(row8p7, 0, 4, IconFiles, evListFiles, nullptr);
	DisplayField::SetDefaultColours(colours.buttonTextColour, colours.buttonTextBackColour);
	moveButton = AddTextButton(row8p7, 1, 4, strings->move, evMovePopup, nullptr);
	extrudeButton = AddTextButton(row8p7, 2, 4, strings->extrusion, evExtrudePopup, nullptr);
	macroButton = AddTextButton(row8p7, 3, 4, strings->macro, evListMacros, nullptr);
#endif

	// Keep the existing macro model for compatibility, but these auxiliary quick
	// macro buttons remain hidden until the RRF macro list explicitly enables them.
	for (size_t i = 0; i < NumControlPageMacroButtons; ++i)
	{
		TextButton * const b = controlPageMacroButtons[i] = new TextButton(row2 + i * rowHeight, 999, 99, nullptr, evNull);
		b->Show(false);
		mgr.AddField(b);
	}
	controlRoot = mgr.GetRoot();
}

// Create the fields for the Printing tab
static void CreatePrintingTabFields(const ColourScheme& colours)
{
#if DISPLAY_X == 800
	mgr.SetRoot(headerRoot);
#else
	mgr.SetRoot(commonRoot);
#endif

#if DISPLAY_X == 800
	const PixelNumber pageLeft = mainContentLeft + 16;
	const PixelNumber innerWidth = mainContentWidth - 32;
	const PixelNumber gap = 6;

	for (unsigned int i = 0; i < MaxSlots; ++i)
	{
		IntegerButton * const ib = new IntegerButton(0, 0, 1, "", "%");
		ib->SetValue(100);
		ib->SetEvent(evExtrusionFactor, i);
		ib->Show(false);
		extrusionFactors[i] = ib;
		mgr.AddField(ib);
	}

	const PixelNumber tempTop = 64;
	const PixelNumber tempHeight = 58;
	const PixelNumber tempGap = 4;
	const PixelNumber tempWidth = (innerWidth - 7 * tempGap) / 8;
	for (size_t i = 0; i < MaxPrintTempControls; ++i)
	{
		const PixelNumber x = pageLeft + (PixelNumber)i * (tempWidth + tempGap);
		DisplayField::SetDefaultColours(colours.buttonTextColour, colours.buttonTextBackColour,
				colours.buttonTextBackColour, 0, colours.buttonPressedBackColour, 0, colours.pal);
		printTempTitles[i] = new VectorIconButton(tempTop + 3, x + 4, tempWidth - 8, 21,
				i == 0 ? VectorIcon::Bed : VectorIcon::Nozzle, evNull, (int)i, DEFAULT_FONT, 0);
		printTempTitles[i]->Show(false);
		mgr.AddField(printTempTitles[i]);

		DisplayField::SetDefaultColours(colours.infoTextColour, colours.buttonTextBackColour);
		printCurrentTemps[i] = new FloatField(tempTop + 31, x + 3, tempWidth - 6,
				TextAlignment::Centre, 1, nullptr, "°C");
		printCurrentTemps[i]->Show(false);
		mgr.AddField(printCurrentTemps[i]);

		mgr.AddField(new ModernPanelField(tempTop, x, tempWidth, tempHeight,
				colours.buttonTextBackColour, colours.buttonBorderColour, 10));
	}

	const PixelNumber jobTop = 128;
	const PixelNumber jobHeight = 66;
	const PixelNumber jobRightX = pageLeft + 338;
	const PixelNumber jobRightWidth = innerWidth - 350;
	DisplayField::SetDefaultColours(colours.labelTextColour, colours.buttonTextBackColour);
	mgr.AddField(new StaticTextField(jobTop + 5, pageLeft + 12, 200, TextAlignment::Left, "JOB / POSITION"));
	DisplayField::SetDefaultColours(colours.infoTextColour, colours.buttonTextBackColour);
	for (size_t i = 0; i < MaxDisplayableAxes; ++i)
	{
		const PixelNumber x = pageLeft + 12 + (PixelNumber)i * 106;
		FloatField * const f = new FloatField(jobTop + 31, x, 100, TextAlignment::Left, 1, axisNames[i], " mm");
		printTabAxisPos[i] = f;
		mgr.AddField(f);
		f->Show(i < 3);
	}
	printElapsedText.copy("0:00:00");
	printElapsedField = new TextField(jobTop + 5, jobRightX, 150, TextAlignment::Left, "Time ", printElapsedText.c_str());
	mgr.AddField(printElapsedField);
	timeLeftField = new TextField(jobTop + 5, jobRightX + 154, jobRightWidth - 154, TextAlignment::Left, "ETA ", "");
	mgr.AddField(timeLeftField);
	mgr.Show(timeLeftField, false);
	printPercentField = new IntegerField(jobTop + 31, jobRightX, 150, TextAlignment::Left, "Progress ", "%");
	mgr.AddField(printPercentField);
	printPercentField->SetValue(0);
	DisplayField::SetDefaultColours(colours.progressBarColour, colours.progressBarBackColour);
	printProgressBar = new ProgressBar(jobTop + 55, jobRightX, 7, jobRightWidth);
	mgr.AddField(printProgressBar);
	mgr.Show(printProgressBar, false);
	mgr.AddField(new ModernPanelField(jobTop, pageLeft, innerWidth, jobHeight,
			colours.buttonTextBackColour, colours.buttonBorderColour, 12));

	const PixelNumber actionTop = 200;
	const PixelNumber actionHeight = 38;
	const PixelNumber actionWidth = (innerWidth - 4 * gap) / 5;
	DisplayField::SetDefaultColours(colours.buttonTextColour, colours.buttonTextBackColour,
			colours.buttonBorderColour, 0, colours.buttonPressedBackColour, 0, colours.pal);
	babystepButton = new VectorIconButton(actionTop, pageLeft, actionWidth, actionHeight,
			VectorIcon::Move, evBabyStepPopup, "Baby Z", DEFAULT_FONT, 0);
	mgr.AddField(babystepButton);
	DisplayField::SetDefaultColours(colours.buttonTextColour, colours.pauseButtonBackColour,
			colours.buttonBorderColour, 0, colours.pauseButtonBackColour, 0, colours.pal);
	pauseButton = new VectorIconButton(actionTop, pageLeft + actionWidth + gap, actionWidth, actionHeight,
			VectorIcon::Pause, evPausePrint, "Pause", DEFAULT_FONT, "M25");
	mgr.AddField(pauseButton);
	DisplayField::SetDefaultColours(colours.buttonTextColour, colours.resumeButtonBackColour,
			colours.buttonBorderColour, 0, colours.resumeButtonBackColour, 0, colours.pal);
	resumeButton = new VectorIconButton(actionTop, pageLeft + actionWidth + gap, actionWidth, actionHeight,
			VectorIcon::Play, evResumePrint, "Resume", DEFAULT_FONT, "M24");
	mgr.AddField(resumeButton);
	DisplayField::SetDefaultColours(colours.buttonTextColour, colours.resetButtonBackColour,
			colours.buttonBorderColour, 0, colours.resetButtonBackColour, 0, colours.pal);
	cancelButton = new VectorIconButton(actionTop, pageLeft + 2 * (actionWidth + gap), actionWidth, actionHeight,
			VectorIcon::Stop, evReset, "Cancel", DEFAULT_FONT, "M0");
	mgr.AddField(cancelButton);
	DisplayField::SetDefaultColours(colours.buttonTextColour, colours.buttonTextBackColour,
			colours.buttonBorderColour, 0, colours.buttonPressedBackColour, 0, colours.pal);
	reprintButton = new VectorIconButton(actionTop, pageLeft + 3 * (actionWidth + gap), actionWidth, actionHeight,
			VectorIcon::Print, evReprint, "Reprint", DEFAULT_FONT, 0);
	reprintButton->Show(false);
	mgr.AddField(reprintButton);
	spd = new IntegerButton(actionTop, pageLeft + 4 * (actionWidth + gap), actionWidth, "Speed ", "%");
	spd->SetValue(100);
	spd->SetEvent(evAdjustSpeed, "M220 S");
	mgr.AddField(spd);

	const PixelNumber ioWidth = (innerWidth - 3 * 8) / 4;
	const PixelNumber ioNameHeight = 0;
	const PixelNumber tuneLabelY = 243;
	const PixelNumber tuneRow = 259;
	const PixelNumber fanLabelY = 321;
	const PixelNumber fanRow0 = 338;
	const PixelNumber fanRow1 = 366;
	const PixelNumber outLabelY = 396;
	const PixelNumber outRow0 = 413;
	const PixelNumber outRow1 = 441;
	DisplayField::SetDefaultColours(colours.labelTextColour, colours.defaultBackColour);
	mgr.AddField(new StaticTextField(tuneLabelY, pageLeft, 120, TextAlignment::Left, "TUNING"));
	mgr.AddField(new StaticTextField(fanLabelY, pageLeft, 120, TextAlignment::Left, "FANS"));
	mgr.AddField(new StaticTextField(outLabelY, pageLeft, 160, TextAlignment::Left, "OUTPUTS"));

	// Speed and extrusion-factor controls use a compact 7-column grid.
	const PixelNumber tuneGap = 5;
	const PixelNumber tuneWidth = (innerWidth - 6 * tuneGap) / 7;
	spd->SetPositionAndWidth(pageLeft, tuneWidth);
	spd->SetPosition(pageLeft, tuneRow);
	spd->SetHeight(28);
	spd->SetLabel("SPD ");
	for (size_t i = 0; i < MaxSlots; ++i)
	{
		if (extrusionFactors[i] != nullptr)
		{
			const size_t n = i + 1;
			const size_t col = n % 7u;
			const size_t row = n / 7u;
			const PixelNumber ex = pageLeft + (PixelNumber)col * (tuneWidth + tuneGap);
			extrusionFactors[i]->SetPositionAndWidth(ex, tuneWidth);
			extrusionFactors[i]->SetPosition(ex, tuneRow + (PixelNumber)row * 30);
			extrusionFactors[i]->SetHeight(28);
		}
	}

	for (size_t i = 0; i < MaxPrintFanControls; ++i)
	{
		const size_t col = i & 3u;
		const PixelNumber rowTop = (i < 4) ? fanRow0 : fanRow1;
		const PixelNumber x = pageLeft + (PixelNumber)col * (ioWidth + 8);
		printFanNameFields[i] = nullptr;
		DisplayField::SetDefaultColours(colours.buttonTextColour, colours.buttonTextBackColour,
				colours.buttonBorderColour, 0, colours.buttonPressedBackColour, 0, colours.pal);
		CompactPercentButton * const b = new CompactPercentButton(rowTop + ioNameHeight, x, ioWidth, GetFanDisplayName(i));
		b->SetEvent(evAdjustFan, (int)i);
		b->SetValue(0);
		b->Show(false);
		printFanButtons[i] = b;
		mgr.AddField(b);
	}
	fanSpeed = nullptr;

	for (size_t i = 0; i < MaxPrintGpOutControls; ++i)
	{
		const size_t col = i & 3u;
		const PixelNumber rowTop = (i < 4) ? outRow0 : outRow1;
		const PixelNumber x = pageLeft + (PixelNumber)col * (ioWidth + 8);
		printGpOutNameFields[i] = nullptr;
		DisplayField::SetDefaultColours(colours.buttonTextColour, colours.buttonTextBackColour,
				colours.buttonBorderColour, 0, colours.buttonPressedBackColour, 0, colours.pal);
		CompactPercentButton * const b = new CompactPercentButton(rowTop + ioNameHeight, x, ioWidth, GetGpOutDisplayName(i));
		b->SetEvent(evAdjustGpOut, (int)i);
		b->SetValue(0);
		b->Show(false);
		printGpOutButtons[i] = b;
		mgr.AddField(b);
	}
#else
	// Original compact PanelDue print layout.
	DisplayField::SetDefaultColours(colours.labelTextColour, colours.defaultBackColour);
	mgr.AddField(new StaticTextField(row6 + labelRowAdjust, mainContentLeft + margin,
		bedColumn - mainContentLeft - fieldSpacing - margin, TextAlignment::Right, strings->extruderPercent));

	DisplayField::SetDefaultColours(colours.buttonTextColour, colours.buttonTextBackColour);
	for (unsigned int i = 0; i < MaxSlots; ++i)
	{
		const PixelNumber column = ((tempButtonWidth + fieldSpacing) * i) + bedColumn;
		IntegerButton * const ib = new IntegerButton(row6, column, tempButtonWidth);
		ib->SetValue(100);
		ib->SetEvent(evExtrusionFactor, i);
		ib->Show(false);
		extrusionFactors[i] = ib;
		mgr.AddField(ib);
	}

	mgr.AddField(spd = new IntegerButton(row7, speedColumn, stateColumnWdith - fieldSpacing, strings->speed, "%"));
	spd->SetValue(100);
	spd->SetEvent(evAdjustSpeed, "M220 S");
	mgr.AddField(fanSpeed = new IntegerButton(row7, fanColumn, stateColumnWdith - fieldSpacing, strings->fan, "%"));
	fanSpeed->SetEvent(evAdjustFan, 0);
	fanSpeed->SetValue(0);
	babystepButton = new TextButton(row7, babystepColumn, stateColumnWdith - fieldSpacing, strings->babystep, evBabyStepPopup);
	mgr.AddField(babystepButton);
	DisplayField::SetDefaultColours(colours.buttonTextColour, colours.resetButtonBackColour);
	cancelButton = new TextButton(row7, cancelColumn, stateColumnWdith - fieldSpacing, strings->cancel, evReset, "M0");
	mgr.AddField(cancelButton);
	DisplayField::SetDefaultColours(colours.buttonTextColour, colours.pauseButtonBackColour);
	pauseButton = new TextButton(row7, pauseColumn, stateColumnWdith - (2 * margin), strings->pause, evPausePrint, "M25");
	mgr.AddField(pauseButton);
	DisplayField::SetDefaultColours(colours.buttonTextColour, colours.resumeButtonBackColour);
	resumeButton = new TextButton(row7, resumeColumn, stateColumnWdith - (2 * margin), strings->resume, evResumePrint, "M24");
	mgr.AddField(resumeButton);
	DisplayField::SetDefaultColours(colours.buttonTextColour, colours.buttonTextBackColour);
	reprintButton = new TextButton(row8, speedColumn, 2 * stateColumnWdith - fieldSpacing, strings->reprint, evReprint);
	reprintButton->Show(false);
	mgr.AddField(reprintButton);
	DisplayField::SetDefaultColours(colours.progressBarColour, colours.progressBarBackColour);
	mgr.AddField(printProgressBar = new ProgressBar(row8 + (rowHeight - progressBarHeight)/2,
			mainContentLeft + margin, progressBarHeight, mainContentWidth - 2 * margin));
	mgr.Show(printProgressBar, false);
	DisplayField::SetDefaultColours(colours.labelTextColour, colours.defaultBackColour);
	mgr.AddField(timeLeftField = new TextField(row9, mainContentLeft + margin,
			mainContentWidth - 2 * margin, TextAlignment::Left, strings->timeRemaining));
	mgr.Show(timeLeftField, false);
#endif

	printRoot = mgr.GetRoot();
}

// Create the fields for the Message tab
static void CreateMessageTabFields(const ColourScheme& colours, uint32_t language)
{
	mgr.SetRoot(baseRoot);
#if DISPLAY_X == 800
	DisplayField::SetDefaultColours(colours.titleBarTextColour, colours.titleBarBackColour);
	mgr.AddField(new StaticTextField(17, mainContentLeft + 18, mainContentWidth - 36,
			TextAlignment::Left, "Console"));
	mgr.AddField(new ModernPanelField(0, mainContentLeft, mainContentWidth, topBarHeight,
			colours.titleBarBackColour, colours.buttonBorderColour, 0));
	DisplayField::SetDefaultColours(colours.labelTextColour, colours.defaultBackColour);
#else
	DisplayField::SetDefaultColours(colours.buttonTextColour, colours.buttonImageBackColour);
	mgr.AddField(new IconButton(margin,  DisplayX - margin - keyboardButtonWidth, keyboardButtonWidth, IconKeyboard, evKeyboard));
	DisplayField::SetDefaultColours(colours.labelTextColour, colours.defaultBackColour);
	mgr.AddField(new StaticTextField(margin + labelRowAdjust, margin, DisplayX - 2 * margin - keyboardButtonWidth, TextAlignment::Centre, strings->messages));
#endif
	PixelNumber row = firstMessageRow;
	for (unsigned int r = 0; r < numMessageRows; ++r)
	{
#if DISPLAY_X == 800
		StaticTextField *t = new StaticTextField(row, mainContentLeft + margin, messageTimeWidth, TextAlignment::Left, nullptr);
#else
		StaticTextField *t = new StaticTextField(row, margin, messageTimeWidth, TextAlignment::Left, nullptr);
#endif
		mgr.AddField(t);
		messageTimeFields[r] = t;
		t = new StaticTextField(row, messageTextX, messageTextWidth, TextAlignment::Left, nullptr);
		mgr.AddField(t);
		messageTextFields[r] = t;
		row += rowTextHeight;
	}
#if DISPLAY_X == 800
	CreateConsoleKeyboardFields(language, colours);
#endif
	messageRoot = mgr.GetRoot();
}

// Create the fields for the Setup tab
static void CreateSetupTabFields(uint32_t language, const ColourScheme& colours)
{
	mgr.SetRoot(baseRoot);
#if DISPLAY_X == 800
	// Wide touch-first settings rows.
	const PixelNumber gap = 10;
	const PixelNumber left = mainContentLeft + 16;
	const PixelNumber width = (mainContentWidth - 32 - gap) / 2;
	const PixelNumber right = left + width + gap;
	const PixelNumber y0 = 82;
	const PixelNumber dy = 39;

	DisplayField::SetDefaultColours(colours.titleBarTextColour, colours.titleBarBackColour);
	mgr.AddField(new StaticTextField(17, mainContentLeft + 18, mainContentWidth - 36, TextAlignment::Left, "Settings"));
	mgr.AddField(new ModernPanelField(0, mainContentLeft, mainContentWidth, topBarHeight,
			colours.titleBarBackColour, colours.buttonBorderColour, 0));

	DisplayField::SetDefaultColours(colours.labelTextColour, colours.defaultBackColour);
	mgr.AddField(fwVersionField = new TextField(59, left, width, TextAlignment::Left, "FW - ", SETTINGS_VERSION_TEXT));
	mgr.AddField(ipAddressField = new TextField(59, right, width, TextAlignment::Right, "IP ", ipAddress.c_str()));
	freeMem = new IntegerField(4, left, 1, TextAlignment::Left, "");
	freeMem->Show(false);
	mgr.AddField(freeMem);

	DisplayField::SetDefaultColours(colours.buttonTextColour, colours.buttonTextBackColour,
			colours.buttonBorderColour, 0, colours.buttonPressedBackColour, 0, colours.pal);
	languageButton = new TextButton(y0 + 0*dy, left, width, LanguageTables[language].languageName, evSetLanguage);
	baudRateButton = new IntegerButton(y0 + 0*dy, right, width, "Baud  ", nullptr);
	baudRateButton->SetEvent(evSetBaudRate, 0); baudRateButton->SetValue(nvData.GetBaudRate());
	mgr.AddField(languageButton); mgr.AddField(baudRateButton);

	volumeButton = new IntegerButton(y0 + 1*dy, left, width, strings->volume, nullptr);
	volumeButton->SetEvent(evSetVolume, 0); volumeButton->SetValue(nvData.GetVolume());
	coloursButton = new TextButton(y0 + 1*dy, right, width, strings->colourSchemeNames[colours.index], evSetColours);
	mgr.AddField(volumeButton); mgr.AddField(coloursButton);

	mgr.AddField(new TextButton(y0 + 2*dy, left, width, strings->brightnessDown, evDimmer));
	mgr.AddField(new TextButton(y0 + 2*dy, right, width, strings->brightnessUp, evBrighter));

	dimmingTypeButton = new TextButton(y0 + 3*dy, left, width,
			strings->displayDimmingNames[(unsigned int)nvData.GetDisplayDimmerType()], evSetDimmingType);
	infoTimeoutButton = new IntegerButton(y0 + 3*dy, right, width, strings->infoTimeout, nullptr);
	infoTimeoutButton->SetEvent(evSetInfoTimeout, 0); infoTimeoutButton->SetValue(infoTimeout);
	mgr.AddField(dimmingTypeButton); mgr.AddField(infoTimeoutButton);

	screensaverTimeoutButton = new IntegerButton(y0 + 4*dy, left, width, strings->screensaverAfter, nullptr);
	screensaverTimeoutButton->SetEvent(evSetScreensaverTimeout, 0); screensaverTimeoutButton->SetValue(nvData.GetScreensaverTimeout()/1000);
	babystepAmountButton = new TextButtonWithLabel(y0 + 4*dy, right, width,
			babystepAmounts[nvData.GetBabystepAmountIndex()], evSetBabystepAmount, nullptr, strings->babystepAmount);
	mgr.AddField(screensaverTimeoutButton); mgr.AddField(babystepAmountButton);

	feedrateAmountButton = new IntegerButton(y0 + 5*dy, left, width, strings->feedrate, nullptr);
	feedrateAmountButton->SetEvent(evSetFeedrate, 0); feedrateAmountButton->SetValue(nvData.GetFeedrate());
	heaterCombiningButton = new TextButton(y0 + 5*dy, right, width,
			strings->heaterCombineTypeNames[(unsigned int)nvData.GetHeaterCombineType()], evSetHeaterCombineType);
	mgr.AddField(feedrateAmountButton); mgr.AddField(heaterCombiningButton);

	logLevelButton = new TextButton(y0 + 6*dy, left, width,
			strings->logLevelNames[(unsigned int)MessageLog::LogLevelGet()], evSetLogLevel);
	mgr.AddField(logLevelButton);
	mgr.AddField(new TextButton(y0 + 6*dy, right, width, strings->calibrateTouch, evCalTouch));

	mgr.AddField(new TextButton(y0 + 7*dy, left, width, strings->mirrorDisplay, evInvertX));
	mgr.AddField(new TextButton(y0 + 7*dy, right, width, strings->invertDisplay, evInvertY));
	invertZButton = new TextButton(y0 + 8*dy, left, width, nvData.GetInvertZ() ? "Invert Z: ON" : "Invert Z: OFF", evInvertZ);
	mgr.AddField(invertZButton);

	DisplayField::SetDefaultColours(colours.stopButtonTextColour, colours.stopButtonBackColour,
			colours.buttonBorderColour, 0, colours.stopButtonBackColour, 0, colours.pal);
	mgr.AddField(new TextButton(y0 + 9*dy, left, 2*width + gap, strings->clearSettings, evFactoryReset));
#else

	mgr.SetRoot(baseRoot);
	DisplayField::SetDefaultColours(colours.labelTextColour, colours.defaultBackColour);
	// The firmware version field doubles up as an area for displaying debug messages, so make it the full width of the display
	mgr.AddField(fwVersionField = new TextField(row1, mainContentLeft + margin, mainContentWidth - margin, TextAlignment::Left, "FW - ", SETTINGS_VERSION_TEXT));
	mgr.AddField(freeMem = new IntegerField(row2, mainContentLeft + margin, mainContentWidth/2 - margin, TextAlignment::Left, "Free RAM: "));

	DisplayField::SetDefaultColours(colours.buttonTextColour, colours.buttonTextBackColour);
	baudRateButton = AddIntegerButton(row3, 0, 3, nullptr, " baud", evSetBaudRate);
	baudRateButton->SetValue(nvData.GetBaudRate());
	volumeButton = AddIntegerButton(row3, 1, 3, strings->volume, nullptr, evSetVolume);
	volumeButton->SetValue(nvData.GetVolume());
	languageButton = AddTextButton(row3, 2, 3, LanguageTables[language].languageName, evSetLanguage, nullptr);
	AddTextButton(row4, 0, 3, strings->calibrateTouch, evCalTouch, nullptr);
	AddTextButton(row4, 1, 3, strings->mirrorDisplay, evInvertX, nullptr);
	AddTextButton(row4, 2, 3, strings->invertDisplay, evInvertY, nullptr);
	coloursButton = AddTextButton(row5, 0, 3, strings->colourSchemeNames[colours.index], evSetColours, nullptr);
	AddTextButton(row5, 1, 3, strings->brightnessDown, evDimmer, nullptr);
	AddTextButton(row5, 2, 3, strings->brightnessUp, evBrighter, nullptr);
	dimmingTypeButton = AddTextButton(row6, 0, 3, strings->displayDimmingNames[(unsigned int)nvData.GetDisplayDimmerType()], evSetDimmingType, nullptr);
	infoTimeoutButton = AddIntegerButton(row6, 1, 3, strings->infoTimeout, nullptr, evSetInfoTimeout);
	infoTimeoutButton->SetValue(infoTimeout);
	AddTextButton(row6, 2, 3, strings->clearSettings, evFactoryReset, nullptr);
	screensaverTimeoutButton = AddIntegerButton(row7, 0, 3, strings->screensaverAfter, nullptr, evSetScreensaverTimeout);
	screensaverTimeoutButton->SetValue(nvData.GetScreensaverTimeout() / 1000);

	const PixelNumber width = CalcWidth(3);
	mgr.AddField(babystepAmountButton = new TextButtonWithLabel(row7, CalcXPos(1, width), width, babystepAmounts[nvData.GetBabystepAmountIndex()], evSetBabystepAmount, nullptr, strings->babystepAmount));

	feedrateAmountButton = AddIntegerButton(row7, 2, 3, strings->feedrate, nullptr, evSetFeedrate);
	feedrateAmountButton->SetValue(nvData.GetFeedrate());

	heaterCombiningButton  = AddTextButton(row8, 0, 3, strings->heaterCombineTypeNames[(unsigned int)nvData.GetHeaterCombineType()], evSetHeaterCombineType, nullptr);
	logLevelButton = AddTextButton(row8, 1, 3, strings->logLevelNames[(unsigned int)MessageLog::LogLevelGet()], evSetLogLevel, nullptr);
	invertZButton = AddTextButton(row8, 2, 3, nvData.GetInvertZ() ? "Invert Z: ON" : "Invert Z: OFF", evInvertZ, nullptr);

	DisplayField::SetDefaultColours(colours.labelTextColour, colours.defaultBackColour);
	mgr.AddField(ipAddressField = new TextField(row9, mainContentLeft + margin, mainContentWidth/2 - margin, TextAlignment::Left, "IP: ", ipAddress.c_str()));
	setupRoot = mgr.GetRoot();

#endif
	setupRoot = mgr.GetRoot();
}

static void CreateCommonFields(const ColourScheme& colours)
{
#if DISPLAY_X == 800
	// Persistent shell copied from the browser preview: 86px rail, tall
	// navigation cards, 31px line icons and compact Liberation Sans labels.
	DisplayField::SetDefaultColours(colours.buttonTextColour, colours.titleBarBackColour,
			colours.titleBarBackColour, 0, colours.buttonPressedBackColour, 0, colours.pal);
	const PixelNumber navStep = navButtonHeight + navButtonGap;
	tabControl = new VectorIconButton(navButtonTop + 0 * navStep, navButtonLeft, navButtonWidth, navButtonHeight,
			VectorIcon::Home, evTabControl, "Home", DEFAULT_FONT, 0);
	tabStatus = new VectorIconButton(navButtonTop + 1 * navStep, navButtonLeft, navButtonWidth, navButtonHeight,
			VectorIcon::Print, evTabStatus, "Print", DEFAULT_FONT, 0);
	tabMsg = new VectorIconButton(navButtonTop + 2 * navStep, navButtonLeft, navButtonWidth, navButtonHeight,
			VectorIcon::Console, evTabMsg, "Console", DEFAULT_FONT, 0);
	tabSetup = new VectorIconButton(navButtonTop + 3 * navStep, navButtonLeft, navButtonWidth, navButtonHeight,
			VectorIcon::Settings, evTabSetup, "Settings", DEFAULT_FONT, 0);
	mgr.AddField(tabControl);
	mgr.AddField(tabStatus);
	mgr.AddField(tabMsg);
	mgr.AddField(tabSetup);

	
	// Navigation starts at y=4, reserving enough rail height for two future pages.

	// Background is added last because fields are prepended by Window::AddField;
	// this makes it draw before the navigation rather than over it.
	mgr.AddField(new ModernPanelField(0, 0, navBarWidth, DisplayY,
			colours.titleBarBackColour, colours.buttonBorderColour, 0));

	// Persistent emergency STOP in the top bar. It belongs to baseRoot, so it is
	// present on Home, Print, Console and Settings. Popup pages add their own STOP.
	DisplayField::SetDefaultColours(colours.stopButtonTextColour, colours.stopButtonBackColour,
			colours.stopButtonBackColour, 0, colours.stopButtonBackColour, 0, colours.pal);
	mgr.AddField(new VectorIconButton(7, globalStopLeft, globalStopWidth, 42,
			VectorIcon::Stop, evEmergencyStop, "STOP", DEFAULT_FONT, 0));
#else
	DisplayField::SetDefaultColours(colours.buttonTextColour, colours.titleBarBackColour, colours.buttonBorderColour, 0,
			colours.buttonPressedBackColour, 0, colours.pal);
	tabControl = AddIconButtonWithText(rowTabs, 0, 4, IconHomeAll, evTabControl, strings->control, 0);
	tabStatus = AddIconButtonWithText(rowTabs, 1, 4, IconNozzle, evTabStatus, strings->status, 0);
	tabMsg = AddIconButtonWithText(rowTabs, 2, 4, IconKeyboard, evTabMsg, strings->console, 0);
	tabSetup = AddIconButtonWithText(rowTabs, 3, 4, IconBedComp, evTabSetup, strings->setup, 0);
#endif
	DisplayField::SetDefaultColours(colours.buttonTextColour, colours.buttonTextBackColour, colours.buttonBorderColour, 0,
			colours.buttonPressedBackColour, 0, colours.pal);
}

static void CreateMainPages(uint32_t language, const ColourScheme& colours)
{
	if (language >= ARRAY_SIZE(LanguageTables))
	{
		language = 0;
	}
	emptyRoot = mgr.GetRoot();
	strings = &LanguageTables[language];
	CreateCommonFields(colours);
	baseRoot = mgr.GetRoot();		// save the root of fields that we usually display

	// Create the fields that are common to the Control and Print pages.
	DisplayField::SetDefaultColours(colours.titleBarTextColour, colours.titleBarBackColour);
#if DISPLAY_X == 800
	mgr.AddField(nameField = new StaticTextField(17, mainContentLeft + 18,
			globalStopLeft - (mainContentLeft + 18) - 8, TextAlignment::Left, machineName.c_str()));
	mgr.AddField(statusField = new StaticTextField(17, DisplayX - statusFieldWidth - 14,
			statusFieldWidth, TextAlignment::Right, nullptr));
	// Add after the text fields so it sits behind them in the prepended field list.
	mgr.AddField(new ModernPanelField(0, mainContentLeft, mainContentWidth, topBarHeight,
			colours.titleBarBackColour, colours.buttonBorderColour, 0));
#else
	mgr.AddField(nameField = new StaticTextField(row1, mainContentLeft + (margin * 2), mainContentWidth - statusFieldWidth - (margin * 3), TextAlignment::Left, machineName.c_str()));
	mgr.AddField(statusField = new StaticTextField(row1, DisplayX - statusFieldWidth, statusFieldWidth - margin, TextAlignment::Right, nullptr));
#endif
	headerRoot = mgr.GetRoot();
	CreateTemperatureGrid(colours);
	commonRoot = mgr.GetRoot();		// save the root of fields that we display on more than one page

	// Create the pages
	CreateControlTabFields(colours);
	CreatePrintingTabFields(colours);
	CreateMessageTabFields(colours, language);
	CreateSetupTabFields(language, colours);
	CreateScreensaverPopup();
	CreateFirmwareUpdatePopup();
}

namespace UI
{
	static void Adjusting(ButtonPress bp)
	{
		fieldBeingAdjusted = bp;
		if (bp == currentButton)
		{
			currentButton.Clear();		// to stop it being released
		}
	}

	static void StopAdjusting()
	{
		if (fieldBeingAdjusted.IsValid())
		{
			mgr.Press(fieldBeingAdjusted, false);
			fieldBeingAdjusted.Clear();
		}
	}

	static void CurrentButtonReleased()
	{
		if (currentButton.IsValid())
		{
			mgr.Press(currentButton, false);
			currentButton.Clear();
		}
	}

	static void ClearAlertOrResponse();

	// Return the number of supported languages
	unsigned int GetNumLanguages()
	{
		return NumLanguages;
	}

	void InitColourScheme(const ColourScheme *scheme)
	{
		colours = scheme;
	}

	// Create all the fields we ever display
	void CreateFields(uint32_t language, const ColourScheme& colours, uint32_t p_infoTimeout)
	{
		infoTimeout = p_infoTimeout;

		// Set up default colours and margins
		mgr.Init(colours.defaultBackColour);
		DisplayField::SetDefaultFont(DEFAULT_FONT);
		ButtonWithText::SetFont(DEFAULT_FONT);
		CharButtonRow::SetFont(DEFAULT_FONT);
		SingleButton::SetTextMargin(textButtonMargin);
		SingleButton::SetIconMargin(iconButtonMargin);

		// Create the pages
		CreateMainPages(language, colours);

		// Create the popup fields
		CreateIntegerAdjustPopup(colours);
		CreateIntegerRPMAdjustPopup(colours);
#if DISPLAY_X == 800
		CreateHomeSlotPopup(colours);
#endif
		CreateMovePopup(colours);
		CreateExtrudePopup(colours);
		fileListPopup = CreateFileListPopup(filesListButtons, filenameButtons, NumFileRows, NumFileColumns, colours, true);
		macrosPopup = CreateFileListPopup(macrosListButtons, macroButtons, NumMacroRows, NumMacroColumns, colours, false);
		CreateFileActionPopup(colours);
		CreateVolumePopup(colours);
		CreateInfoTimeoutPopup(colours);
		CreateScreensaverTimeoutPopup(colours);
		CreateBabystepAmountPopup(colours);
		CreateFeedrateAmountPopup(colours);
		CreateBaudRatePopup(colours);
		CreateColoursPopup(colours);
		CreateAreYouSurePopup(colours);
		CreateKeyboardPopup(language, colours);
		CreateLanguagePopup(colours);
		alertPopup = new AlertPopup(colours);
		CreateBabystepPopup(colours);

		DisplayField::SetDefaultColours(colours.labelTextColour, colours.defaultBackColour);
		touchCalibInstruction = new StaticTextField(DisplayY/2 - 10, 0, DisplayX, TextAlignment::Centre, strings->touchTheSpot);

		mgr.SetRoot(nullptr);

#ifdef SUPPORT_ENCODER
		encoder = new RotaryEncoder(2, 3, 32+6);			// PA2, PA3 and PB6
		encoder->Init(4);
#endif
	}

	// This is called when no job is active/paused
	void ShowFilesButton()
	{
		// First hide everything removed then show everything new
		// otherwise remnants of the to-be-hidden might remain
		mgr.Show(resumeButton,		false);
		mgr.Show(cancelButton,		false);
		mgr.Show(pauseButton,		false);
		mgr.Show(printProgressBar,	false);

		mgr.Show(babystepButton,	true);
		mgr.Show(reprintButton,		lastJobFileNameAvailable);
		mgr.Show(filesButton,		true);
	}

	// This is called when a job is active
	void ShowPauseButton()
	{
		// First hide everything removed then show everything new
		// otherwise remnants of the to-be-hidden might remain
		mgr.Show(resumeButton,		false);
		mgr.Show(cancelButton,		false);
		mgr.Show(filesButton,		false);
		mgr.Show(reprintButton,		false);

		mgr.Show(pauseButton,		true);
		mgr.Show(babystepButton,	true);
		mgr.Show(printProgressBar,	true);
	}

	// This is called when a job is paused
	void ShowResumeAndCancelButtons()
	{
		// First hide everything removed then show everything new
		// otherwise remnants of the to-be-hidden might remain
		mgr.Show(filesButton,		false);
		mgr.Show(pauseButton,		false);
		mgr.Show(reprintButton,		false);
		mgr.Show(babystepButton,	false);

		mgr.Show(cancelButton,		true);
		mgr.Show(resumeButton,		true);
		mgr.Show(printProgressBar,	true);
	}

	// Show or hide an axis on the move button grid and on the axis display
	void ShowAxis(size_t slot, bool b, const char* axisLetter)
	{
		if (slot >= MaxDisplayableAxes)
		{
			return;
		}
#if DISPLAY_X != 800
		// Legacy 4.3" UI: the axis label is followed by eight individual jog buttons.
		DisplayField *f = moveAxisRows[slot];
		for (int i = 0; i < 9 && f != nullptr; ++i)
		{
			mgr.Show(f, b);
			if (i > 0)
			{
				TextButtonForAxis *textButton = static_cast<TextButtonForAxis*>(f);
				textButton->SetAxisLetter(axisLetter[0]);
			}
			f = f->next;
		}
#endif
#if DISPLAY_X == 800
		mgr.Show(controlTabAxisPos[slot], b && slot < 3);
		mgr.Show(printTabAxisPos[slot], b);
		// The modern motion page reserves compact readouts for X/Y/Z only.
		mgr.Show(movePopupAxisPos[slot], b && slot < 3);
		UpdateMoveBedMapGeometry();
#else
		if (numDisplayedAxes < MaxDisplayableAxes)
		{
			mgr.Show(movePopupAxisPos[slot], b);
		}
		else
		{
			for (size_t i = 0; i < MaxDisplayableAxes; ++i)
			{
				mgr.Show(movePopupAxisPos[i], false);
			}
		}
#endif
	}

	void UpdateAxisPosition(size_t axisIndex, float fval)
	{
		if (axisIndex < MaxTotalAxes)
		{
			axisPositionValues[axisIndex] = fval;
#if DISPLAY_X == 800
			UpdateMoveBedMapGeometry();
#endif
			auto axis = OM::GetAxis(axisIndex);
			if (axis != nullptr && axis->slot < MaxDisplayableAxes)
			{
				size_t slot = axis->slot;

				if (axisMaxVal > 1000)
				{
					controlTabAxisPos[slot]->SetNumDecimals(1);
#if DISPLAY_X == 800
					printTabAxisPos[slot]->SetNumDecimals(1);
#endif
					movePopupAxisPos[slot]->SetNumDecimals(1);
				}

				controlTabAxisPos[slot]->SetValue(fval);
#if DISPLAY_X == 800
				printTabAxisPos[slot]->SetValue(fval);
#endif
				movePopupAxisPos[slot]->SetValue(fval);
			}
		}
	}

	void UpdateCurrentTemperature(size_t heaterIndex, float fval)
	{
		OM::Slots heaterSlots;
		OM::GetHeaterSlots(heaterIndex, heaterSlots);
		if (!heaterSlots.IsEmpty())
		{
			const size_t count = heaterSlots.Size();
			for (size_t i = 0; i < count; ++i)
			{
				const size_t slot = heaterSlots[i];
				currentTemps[slot]->SetValue(fval);
#if DISPLAY_X == 800
				if (printCurrentTemps[slot] != nullptr)
				{
					printCurrentTemps[slot]->SetValue(fval);
				}
#endif
			}

			heaterSlots.Clear();
		}
	}

	void UpdateHeaterStatus(const size_t heaterIndex, const OM::HeaterStatus status)
	{
		OM::Slots heaterSlots;
		OM::GetHeaterSlots(heaterIndex, heaterSlots);
		const Colour foregroundColour =	(status == OM::HeaterStatus::fault)
					? colours->errorTextColour
					: colours->infoTextColour;
#if DISPLAY_X == 800
		const Colour backgroundColour = (status == OM::HeaterStatus::fault)
				? colours->errorBackColour : colours->buttonTextBackColour;
		const Colour targetBackColour = colours->popupButtonBackColour;
#else
		const Colour backgroundColour =
					  (status == OM::HeaterStatus::standby) ? colours->standbyBackColour
					: (status == OM::HeaterStatus::active)  ? colours->activeBackColour
					: (status == OM::HeaterStatus::fault)   ? colours->errorBackColour
					: (status == OM::HeaterStatus::tuning)  ? colours->tuningBackColour
					: colours->infoBackColour;
		const Colour targetBackColour = colours->buttonTextBackColour;
#endif
		const Colour activeTargetBackColour = (status == OM::HeaterStatus::active)
				? colours->activeBackColour : targetBackColour;
		const Colour standbyTargetBackColour = (status == OM::HeaterStatus::standby)
				? colours->standbyBackColour : targetBackColour;
		const Colour bedOrChamberBgColor =
				  (status == OM::HeaterStatus::active)  ? colours->activeBackColour
				: (status == OM::HeaterStatus::standby) ? colours->standbyBackColour
				: (status == OM::HeaterStatus::fault)   ? colours->errorBackColour
				: (status == OM::HeaterStatus::tuning)  ? colours->tuningBackColour
				: colours->buttonImageBackColour;
		if (!heaterSlots.IsEmpty())
		{
			const size_t count = heaterSlots.Size();
			for (size_t i = 0; i < count; ++i)
			{
				const size_t slot = heaterSlots[i];
				currentTemps[slot]->SetColours(foregroundColour, backgroundColour);
				activeTemps[slot]->SetColours(colours->buttonTextColour, activeTargetBackColour);
				standbyTemps[slot]->SetColours(colours->buttonTextColour, standbyTargetBackColour);
#if DISPLAY_X == 800
				if (printCurrentTemps[slot] != nullptr)
				{
					printCurrentTemps[slot]->SetColours(foregroundColour, backgroundColour);
				}
#endif

				OM::IterateBedsWhile([&heaterIndex, &status, &foregroundColour, &bedOrChamberBgColor, &slot](OM::Bed*& bed, size_t) {
					if (bed->heater == (int)heaterIndex)
					{
						bed->heaterStatus = status;
						toolButtons[slot]->SetColours(foregroundColour, bedOrChamberBgColor);
						return false;
					}
					return true;
				});
				OM::IterateChambersWhile([&heaterIndex, &status, &foregroundColour, &bedOrChamberBgColor, &slot](OM::Chamber*& chamber, size_t) {
					if (chamber->heater == (int)heaterIndex)
					{
						chamber->heaterStatus = status;
						toolButtons[slot]->SetColours(foregroundColour, bedOrChamberBgColor);
						return false;
					}
					return true;
				});
			}
			heaterSlots.Clear();
		}
	}

#if DISPLAY_X == 800
	void SetMachineMode(const char* mode)
	{
		UiMachineMode next = UiMachineMode::Fff;
		if (strcmp(mode, "Laser") == 0)
		{
			next = UiMachineMode::Laser;
		}
		else if (strcmp(mode, "CNC") == 0)
		{
			next = UiMachineMode::Cnc;
		}

		if (next != uiMachineMode)
		{
			uiMachineMode = next;
			OM::IterateToolsWhile([](OM::Tool*& tool, size_t)
			{
				if (tool != nullptr && tool->slot < MaxSlots && toolButtons[tool->slot] != nullptr)
				{
					const VectorIcon icon = ModernToolModeIcon(tool->spindle != nullptr);
					toolButtons[tool->slot]->SetIcon(icon);
					if (printTempTitles[tool->slot] != nullptr)
					{
						printTempTitles[tool->slot]->SetIcon(icon);
					}
				}
				return true;
			});
		}
	}
#else
	void SetMachineMode(const char* mode)
	{
		UNUSED(mode);
	}
#endif

	void SetCurrentTool(int32_t ival)
	{
		if (ival == currentTool)
		{
			return;
		}
		currentTool = ival;
#if DISPLAY_X == 800
		if (moveToolField != nullptr)
		{
			if (ival >= 0)
			{
				moveToolText.printf("T%ld", (long)ival);
			}
			else
			{
				moveToolText.copy("T--");
			}
			moveToolField->SetValue(moveToolText.c_str());
		}
#endif
	}

	enum TimesLeft { file, filament, slicer, max };
	static int timesLeft[TimesLeft::max];
	static uint32_t simulatedTime;
	static uint32_t jobDuration;
	static uint32_t jobWarmUpDuration;
	static String<50> timesLeftText;

	static const char *GetStatusString(OM::PrinterStatus status)
	{
		unsigned int index = (unsigned int)status;
		if (index >= ARRAY_SIZE(strings->statusValues) || !strings->statusValues[index])
		{
			return "unknown status";
		}

		return strings->statusValues[index];
	}

	void ChangeStatus(OM::PrinterStatus oldStatus, OM::PrinterStatus newStatus)
	{

		if (oldStatus != newStatus)
		{
			const char *fromStatus = GetStatusString(oldStatus);
			const char *toStatus = GetStatusString(newStatus);

			MessageLog::AppendMessageF(MessageLog::LogLevel::Verbose,
					"Info: status changed from %s to %s.", fromStatus, toStatus);
		}

		switch (newStatus)
		{
		case OM::PrinterStatus::printing:
		case OM::PrinterStatus::simulating:
			if (oldStatus != OM::PrinterStatus::paused && oldStatus != OM::PrinterStatus::resuming)
			{
				// Starting a new print, so clear the times
				timesLeft[0] = timesLeft[1] = timesLeft[2] = 0;
				simulatedTime = 0;
			}
			SetLastFileSimulated(newStatus == OM::PrinterStatus::simulating);
			if (oldStatus != newStatus)
			{
				PrintStarted();
			}
			[[fallthrough]];
		case OM::PrinterStatus::paused:
		case OM::PrinterStatus::pausing:
		case OM::PrinterStatus::resuming:
			if (currentTab == tabStatus)
			{
				nameField->SetValue(printingFile.c_str());
			}
			break;

		case OM::PrinterStatus::idle:
			printingFile.Clear();
			nameField->SetValue(machineName.c_str());
			if (IsPrintingStatus(oldStatus))
			{
				mgr.ClearAllPopups();
			}
			[[fallthrough]];
		case OM::PrinterStatus::configuring:
			if (oldStatus == OM::PrinterStatus::flashing)
			{
				mgr.ClearAllPopups();						// clear the firmware update message
			}
			break;

		case OM::PrinterStatus::connecting:
			printingFile.Clear();
			mgr.ClearAllPopups();
			break;

		default:
			nameField->SetValue(machineName.c_str());
			break;
		}
	}

	// Append an amount of time to timesLeftText
	static void AppendTimeLeft(int t)
	{
		if (t <= 0)
		{
			timesLeftText.cat(strings->notAvailable);
		}
		else if (t < 60)
		{
			timesLeftText.catf("%ds", t);
		}
		else if (t < 60 * 60)
		{
			timesLeftText.catf("%dm %02ds", t/60, t%60);
		}
		else
		{
			t /= 60;
			timesLeftText.catf("%dh %02dm", t/60, t%60);
		}
	}

	void UpdateTimesLeftText()
	{
		if (!PrintInProgress())
		{
			return;
		}
		size_t count = 0;
		timesLeftText.Clear();
#if DISPLAY_X == 800
		if (simulatedTime > 0)
		{
			timesLeftText.copy(strings->simulated);
			AppendTimeLeft(simulatedTime + jobWarmUpDuration - jobDuration);
		}
		else if (timesLeft[TimesLeft::slicer] > 0)
		{
			timesLeftText.copy(strings->slicer);
			AppendTimeLeft(timesLeft[TimesLeft::slicer]);
		}
		else if (timesLeft[TimesLeft::filament] > 0)
		{
			timesLeftText.copy(strings->filament);
			AppendTimeLeft(timesLeft[TimesLeft::filament]);
		}
		else if (timesLeft[TimesLeft::file] > 0)
		{
			timesLeftText.copy(strings->file);
			AppendTimeLeft(timesLeft[TimesLeft::file]);
		}
		else
		{
			return;
		}
		timeLeftField->SetValue(timesLeftText.c_str());
		mgr.Show(timeLeftField, true);
		return;
#endif
		if (simulatedTime > 0)
		{
			timesLeftText.copy(strings->simulated);
			AppendTimeLeft(simulatedTime + jobWarmUpDuration - jobDuration);
			++count;
		}
		if (timesLeft[TimesLeft::slicer] > 0)
		{
			if (count > 0) {
				timesLeftText.cat(", ");
			}
			timesLeftText.cat(strings->slicer);
			AppendTimeLeft(timesLeft[TimesLeft::slicer]);
			++count;
		}
		if ((count < 2 || (DisplayX >= 800 && count < 3)) && timesLeft[TimesLeft::filament] > 0)
		{
			if (count > 0) {
				timesLeftText.cat(", ");
			}
			timesLeftText.cat(strings->filament);
			AppendTimeLeft(timesLeft[TimesLeft::filament]);
			++count;
		}
		if ((count < 2 || (DisplayX >= 800 && count < 3)) && timesLeft[TimesLeft::file] > 0)
		{
			if (count > 0) {
				timesLeftText.cat(", ");
			}
			timesLeftText.cat(strings->file);
			AppendTimeLeft(timesLeft[TimesLeft::file]);
			++count;
		}

		timeLeftField->SetValue(timesLeftText.c_str());
		mgr.Show(timeLeftField, true);
	}

	void UpdateTimesLeft(size_t index, unsigned int seconds)
	{
		if (index < (int)ARRAY_SIZE(timesLeft))
		{
			timesLeft[index] = seconds;
			UpdateTimesLeftText();
		}
	}

	void UpdateDuration(uint32_t duration)
	{
		jobDuration = duration;
#if DISPLAY_X == 800
		if (printElapsedField != nullptr)
		{
			// Compact H:MM:SS format always shows seconds and fits the Print card.
			printElapsedText.printf("%lu:%02lu:%02lu", (unsigned long)(duration / 3600),
					(unsigned long)((duration / 60) % 60), (unsigned long)(duration % 60));
			printElapsedField->SetValue(printElapsedText.c_str());
		}
#endif
		UpdateTimesLeftText();
	}

	void UpdateWarmupDuration(uint32_t warmupDuration)
	{
		jobWarmUpDuration = warmupDuration;
		UpdateTimesLeftText();
	}

	void SetSimulatedTime(uint32_t simdTime)
	{
		simulatedTime = simdTime;
		UpdateTimesLeftText();
	}

	void SwitchToTab(ButtonBase *newTab) {
		switch (newTab->GetEvent()) {
		case evTabControl:
			mgr.SetRoot(controlRoot);
			nameField->SetValue(machineName.c_str());
			break;
		case evTabStatus:
			mgr.SetRoot(printRoot);
			nameField->SetValue(
					PrintInProgress() ? printingFile.c_str() : machineName.c_str());
			break;
		case evTabMsg:
			mgr.SetRoot(messageRoot);
#if DISPLAY_X == 800
			// The keyboard is part of messageRoot so navigation remains live.
			keyboardDataHandler = SendGcode;
			keyboardIsDisplayed = false;
			keyboardShifted = false;
			for (size_t i = 0; i < 4; ++i)
			{
				if (consoleKeyboardRows[i] != nullptr && consoleCurrentKeyboard != nullptr)
				{
					consoleKeyboardRows[i]->ChangeText(consoleCurrentKeyboard[i]);
				}
			}
			if (userCommandField != nullptr)
			{
				userCommandField->SetLabel(userCommandBuffers[currentUserCommandBuffer].c_str());
			}
#else
			if (keyboardIsDisplayed) {
				keyboardDataHandler = SendGcode;
				mgr.SetPopup(keyboardPopup, AutoPlace, keyboardPopupY, false);
			}
#endif
			break;
		case evTabSetup:
			mgr.SetRoot(setupRoot);
			break;
		default:
			mgr.SetRoot(commonRoot);
			break;
		}
		mgr.Refresh(true);
	}

	// Change to the page indicated. Return true if the page has a permanently-visible button.
	static bool ChangePage(ButtonBase *newTab)
	{
		if (newTab == currentTab)
		{
			mgr.ClearAllPopups();
#if DISPLAY_X == 800
			if (newTab->GetEvent() == evTabMsg)
			{
				// Console is a keyboard-first workspace even when its nav icon is tapped again.
				SwitchToTab(newTab);
			}
#endif
		}
		else
		{
			if (currentTab != nullptr)
			{
				currentTab->Press(false, 0);			// remove highlighting from the old tab
				if (currentTab->GetEvent() == evTabSetup && nvData.IsSaveNeeded())
				{
					SaveSettings();						// leaving the Control tab and we have changed settings, so save them
				}
			}
			newTab->Press(true, 0);						// highlight the new tab
			currentTab = newTab;
			mgr.ClearAllPopups();
			SwitchToTab(newTab);
		}
		return true;
	}

	void ShowFirmwareUpdatePopup()
	{
		mgr.SetPopup(firmwareUpdatePopup);
	}

	void ActivateScreensaver()
	{
		mgr.Show(screensaverText, isLandscape);
		mgr.SetPopup(screensaverPopup);
		lastScreensaverMoved = SystemTick::GetTickCount();
	}

	bool DeactivateScreensaver()
	{
		if (!screensaverPopup->IsPopupActive())
			return false;

		mgr.ClearPopup(true, screensaverPopup);

		return true;
	}

	void AnimateScreensaver()
	{
		if (SystemTick::GetTickCount() - lastScreensaverMoved >= ScreensaverMoveTime)
		{
			static unsigned int seed = SystemTick::GetTickCount();
			const PixelNumber width = isLandscape ? DisplayX : DisplayXP;
			const PixelNumber height = isLandscape ? DisplayY : DisplayYP;
			const PixelNumber availableWidth = (width - 2*margin - screensaverTextWidth);
			const PixelNumber availableHeight = (height - 2*margin - rowTextHeight);
			const PixelNumber x = (rand_r(&seed) % availableWidth);
			const PixelNumber y = (rand_r(&seed) % availableHeight);
			if (isLandscape)
			{
				mgr.Show(screensaverText, false);
				screensaverText->SetPosition(x + margin, y + margin);
				mgr.Show(screensaverText, true);
			}
			lastScreensaverMoved = SystemTick::GetTickCount();
		}
	}

	// Pop up the keyboard (used for editable RRF alerts and legacy callers).
	void ShowKeyboard()
	{
		keyboardDataHandler = SendGcode;
		if (popupUserCommandField != nullptr)
		{
			popupUserCommandField->SetLabel(userCommandBuffers[currentUserCommandBuffer].c_str());
		}
		mgr.SetPopup(keyboardPopup, AutoPlace, keyboardPopupY);
		keyboardIsDisplayed = true;
	}

	static TextField *ActiveKeyboardField()
	{
#if DISPLAY_X == 800
		if (mgr.GetPopup() == keyboardPopup && popupUserCommandField != nullptr)
		{
			return popupUserCommandField;
		}
		return userCommandField;
#else
		return popupUserCommandField;
#endif
	}

	static CharButtonRow **ActiveKeyboardRows()
	{
#if DISPLAY_X == 800
		if (mgr.GetPopup() != keyboardPopup && consoleKeyboardRows[0] != nullptr)
		{
			return consoleKeyboardRows;
		}
#endif
		return keyboardRows;
	}

	static const char* _ecv_array const *ActiveKeyboardLayout()
	{
#if DISPLAY_X == 800
		if (mgr.GetPopup() != keyboardPopup && consoleCurrentKeyboard != nullptr)
		{
			return consoleCurrentKeyboard;
		}
#endif
		return currentKeyboard;
	}

	// This is called when the Cancel button on a popup is pressed
	void PopupCancelled()
	{
		if (mgr.GetPopup() == keyboardPopup)
		{
			keyboardIsDisplayed = false;
		}
	}

	// Return true if polling should be performed
	bool IsSetupTab()
	{
		return currentTab == tabSetup;			// don't poll while we are on the Setup page
	}

	void Tick()
	{
#ifdef SUPPORT_ENCODER
		encoder->Poll();
#endif
	}

#ifdef SUPPORT_ENCODER
	void HandleEncoderChange(const int change)
	{
		bool sent = false;
		if (sent) {
			lastEncoderCommandSentAt = SystemTick::GetTickCount();
		}
	}
#endif

	// This is called in the main spin loop
	void Spin()
	{
#ifdef SUPPORT_ENCODER
		if (SystemTick::GetTickCount() - lastEncoderCommandSentAt >= MinimumEncoderCommandInterval)
		{
			// Check encoder and command movement
			const int ch = encoder->GetChange();
			if (ch != 0)
			{
				HandleEncoderChange(ch);
			}
		}
#endif

		if (currentTab == tabMsg)
		{
			MessageLog::UpdateMessages(false);
		}
		if (alertTicks != 0 && SystemTick::GetTickCount() - whenAlertReceived >= alertTicks)
		{
			ClearAlertOrResponse();
		}
	}

	// This is called when we have just started a file print
	void PrintStarted()
	{
		if (isLandscape)
		{
			ChangePage(tabStatus);
		}
	}

	// This is called when we have just received the name of the file being printed
	void PrintingFilenameChanged(const char data[])
	{
		if (!printingFile.Similar(data))
		{
			printingFile.copy(data);
			if (currentTab == tabStatus && PrintInProgress())
			{
				nameField->SetChanged();
			}
		}
	}

	void LastJobFileNameAvailable(const bool available)
	{
		lastJobFileNameAvailable = available;
		if (!PrintInProgress())
		{
			mgr.Show(reprintButton, available);
		}
	}

	void SetLastFileSimulated(const bool lastFileSimulated)
	{
#if DISPLAY_X == 800
		VectorIconButton* redoButton = static_cast<VectorIconButton*>(reprintButton);
#else
		TextButton* redoButton = static_cast<TextButton*>(reprintButton);
#endif
		redoButton->SetEvent(lastFileSimulated ? evResimulate : evReprint, 0);
		redoButton->SetText(lastFileSimulated ? strings->resimulate : strings->reprint);
	}

	// This is called just before the main polling loop starts. Display the default page.
	void ShowDefaultPage()
	{
		ChangePage(tabControl);
	}

	// Update the fields that are to do with the printing status
	void UpdatePrintingFields()
	{
		OM::PrinterStatus status = GetStatus();
		if (status == OM::PrinterStatus::printing || status == OM::PrinterStatus::simulating)
		{
			ShowPauseButton();
		}
		else if (status == OM::PrinterStatus::paused)
		{
			ShowResumeAndCancelButtons();
		}
		else
		{
			ShowFilesButton();
		}

		// Don't enable the time left field when we start printing, instead this will get enabled when we receive a suitable message
		if (!PrintInProgress())
		{
			mgr.Show(timeLeftField, false);
		}

		const OM::PrinterStatus stat = GetStatus();
		const char * const statusText = ((unsigned int)stat < ARRAY_SIZE(strings->statusValues) && strings->statusValues[(unsigned int)stat])
				? strings->statusValues[(unsigned int)stat] : "unknown status";
		statusField->SetValue(statusText);
#if DISPLAY_X == 800
		if (dashboardStatusField != nullptr)
		{
			dashboardStatusField->SetValue(statusText);
		}
#endif
	}

	// Set the percentage of print completed
	void SetPrintProgressPercent(unsigned int percent)
	{
		printProgressBar->SetPercent((uint8_t)percent);
#if DISPLAY_X == 800
		if (printPercentField != nullptr) printPercentField->SetValue((int)percent);

		if (dashboardProgressBar != nullptr)
		{
			dashboardProgressBar->SetPercent((uint8_t)percent);
		}
#endif
	}

	// Update the geometry or the number of axes
	void UpdateGeometry(unsigned int p_numAxes, bool p_isDelta)
	{
		if (p_numAxes != numVisibleAxes || p_isDelta != isDelta)
		{
			numVisibleAxes = p_numAxes;
			isDelta = p_isDelta;
			FileManager::RefreshMacrosList();
			numDisplayedAxes = 0;
			OM::IterateAxesWhile([](OM::Axis*& axis, size_t)
			{
				axis->slot = MaxTotalAxes;
				if (!axis->visible)
				{
					return true;
				}
				const char * letter = axis->letter;
				if (numDisplayedAxes < MaxDisplayableAxes)
				{
					axis->slot = numDisplayedAxes;
					++numDisplayedAxes;

					// Update axis letter everywhere we display it
					const uint8_t slot = axis->slot;
					controlTabAxisPos	[slot]->SetLabel(letter);
#if DISPLAY_X == 800
					// The PanelDue CICHR motion page has no legacy moveAxisRows[] objects.
					// Touching them here caused a null-pointer HardFault/reset as soon as
					// RRF published the axis list.
					printTabAxisPos		[slot]->SetLabel(letter);
#else
					moveAxisRows		[slot]->SetValue(letter);
#endif
					movePopupAxisPos	[slot]->SetLabel(letter);
					homeButtons			[slot]->SetText(letter);

					// Update axis letter to be sent for homing commands
					homeButtons[slot]->SetEvent(homeButtons[slot]->GetEvent(), letter);
					homeButtons[slot]->SetColours(colours->buttonTextColour, (axis->homed) ? colours->homedButtonBackColour : colours->notHomedButtonBackColour);

					mgr.Show(homeButtons[slot], !isDelta);
					ShowAxis(slot, true, axis->letter);
				}
				// When we get here it's likely to be the initialisation phase
				// and we won't have the babystep amount set
				if (axis->letter[0] == 'Z')
				{
					babystepOffsetField->SetValue(axis->babystep);
				}
				return true;
			});
			// Hide axes possibly shown before
			for (size_t i = numDisplayedAxes; i < MaxDisplayableAxes; ++i)
			{
				mgr.Show(homeButtons[i], false);
				ShowAxis(i, false);
			}
#if DISPLAY_X == 800
			UpdateNotHomedSummary();
#endif
		}
	}

	void UpdateAllHomed()
	{
		bool allHomed = true;
		OM::IterateAxesWhile([&allHomed](OM::Axis*& axis, size_t) {
			if (axis->visible && !axis->homed)
			{
				allHomed = false;
				return false;
			}
			return true;
		});
		if (allHomed != allAxesHomed)
		{
			allAxesHomed = allHomed;
			homeAllButton->SetColours(colours->buttonTextColour, (allAxesHomed) ? colours->homedButtonBackColour : colours->notHomedButtonBackColour);
		}
	}

	// Update the homed status of the specified axis. If the axis is -1 then it represents the "all homed" status.
	void UpdateHomedStatus(size_t axisIndex, bool isHomed)
	{
		OM::Axis *axis = OM::GetOrCreateAxis(axisIndex);
		if (axis == nullptr)
		{
			return;
		}
		axis->homed = isHomed;
		const size_t slot = axis->slot;
		if (slot < MaxDisplayableAxes)
		{
			homeButtons[slot]->SetColours(colours->buttonTextColour, (isHomed) ? colours->homedButtonBackColour : colours->notHomedButtonBackColour);
		}

		UpdateAllHomed();
#if DISPLAY_X == 800
		UpdateNotHomedSummary();
#endif
	}

	// Update the Z probe text
	void UpdateZProbe(const char data[])
	{
		zprobeBuf.copy(data);
		zProbe->SetChanged();
	}

	// Update the machine name
	void UpdateMachineName(const char data[])
	{
		machineName.copy(data);
		nameField->SetChanged();
	}

	// Update the IP address fiels on Setup tab
	void UpdateIP(const char data[])
	{
		ipAddress.copy(data);
		ipAddressField->SetChanged();
	}

	void UpdateFanName(size_t fanIndex, const char data[])
	{
#if DISPLAY_X == 800
		if (fanIndex >= MaxPrintFanControls)
		{
			return;
		}
		if (data != nullptr && data[0] != 0)
		{
			fanNames[fanIndex].copy(data);
		}
		else
		{
			fanNames[fanIndex].Clear();
		}
		const char* const name = GetFanDisplayName(fanIndex);
		if (homeSlotPicker != nullptr) { homeSlotPicker->SetFanLabel(fanIndex, name); }
		if (printFanButtons[fanIndex] != nullptr) { printFanButtons[fanIndex]->SetPrefix(name); }
		for (size_t slot = numToolColsUsed; slot < MaxSlots; ++slot)
		{
			const uint8_t config = nvData.GetHomeSlotConfig(slot);
			if ((config & 0xF0) == 0x10 && (config & 0x0F) == fanIndex && toolButtons[slot] != nullptr)
			{
				toolButtons[slot]->SetText(name);
			}
		}
		if (fieldBeingAdjusted.IsValid() && fieldBeingAdjusted.GetEvent() == evAdjustFan
				&& (size_t)fieldBeingAdjusted.GetIParam() == fanIndex)
		{
			UpdateAdjustmentPopup(fieldBeingAdjusted);
		}
#else
		UNUSED(fanIndex); UNUSED(data);
#endif
	}

	void UpdateFanPercent(size_t fanIndex, int rpm)
	{
#if DISPLAY_X == 800
		if (fanIndex < MaxPrintFanControls)
		{
			const uint8_t bit = (uint8_t)(1u << fanIndex);
			const bool newlyAvailable = (availableFanMask & bit) == 0;
			availableFanMask |= bit;
			lastFanPercent[fanIndex] = rpm;
			if (printFanButtons[fanIndex] != nullptr)
			{
				UpdateField(printFanButtons[fanIndex], rpm);
				mgr.Show(printFanButtons[fanIndex], true);
				if (printFanNameFields[fanIndex] != nullptr) { mgr.Show(printFanNameFields[fanIndex], true); }
			}
			for (size_t slot = numToolColsUsed; slot < MaxSlots; ++slot)
			{
				const uint8_t config = nvData.GetHomeSlotConfig(slot);
				if ((config & 0xF0) == 0x10 && (config & 0x0F) == fanIndex && homeAuxValues[slot] != nullptr)
				{
					if (newlyAvailable) { ApplyHomeAuxSlot(slot); }
					UpdateField(homeAuxValues[slot], rpm);
				}
			}
			if (homeSlotPicker != nullptr) { homeSlotPicker->SetAvailability(availableFanMask, availableGpOutMask, availableHomeMacroMask); }
		}
#else
		if (currentTool == NoTool)
		{
			if (fanIndex == 0) UpdateField(fanSpeed, rpm);
		}
		else
		{
			OM::IterateToolsWhile([&fanIndex, &rpm](OM::Tool*& tool, size_t) {
				if (tool->index == currentTool && tool->fans.IsBitSet(fanIndex) && tool->fans.LowestSetBit() == fanIndex)
				{
					UpdateField(fanSpeed, rpm);
				}
				return true;
			});
		}
#endif
	}

	void UpdateGpOutName(size_t outputIndex, const char data[])
	{
#if DISPLAY_X == 800
		if (outputIndex >= MaxPrintGpOutControls)
		{
			return;
		}
		if (data != nullptr && data[0] != 0)
		{
			gpOutNames[outputIndex].copy(data);
		}
		else
		{
			gpOutNames[outputIndex].Clear();
		}
		const char* const name = GetGpOutDisplayName(outputIndex);
		if (homeSlotPicker != nullptr) { homeSlotPicker->SetOutputLabel(outputIndex, name); }
		if (printGpOutButtons[outputIndex] != nullptr) { printGpOutButtons[outputIndex]->SetPrefix(name); }
		for (size_t slot = numToolColsUsed; slot < MaxSlots; ++slot)
		{
			const uint8_t config = nvData.GetHomeSlotConfig(slot);
			if ((config & 0xF0) == 0x20 && (config & 0x0F) == outputIndex && toolButtons[slot] != nullptr)
			{
				toolButtons[slot]->SetText(name);
			}
		}
		if (fieldBeingAdjusted.IsValid() && fieldBeingAdjusted.GetEvent() == evAdjustGpOut
				&& (size_t)fieldBeingAdjusted.GetIParam() == outputIndex)
		{
			UpdateAdjustmentPopup(fieldBeingAdjusted);
		}
#else
		UNUSED(outputIndex); UNUSED(data);
#endif
	}

	void UpdateGpOutCount(size_t count)
	{
#if DISPLAY_X == 800
		// Sparse gpOut arrays may contain null entries, so availability is established from valid PWM entries.
		UNUSED(count);
		if (homeSlotPicker != nullptr) { homeSlotPicker->SetAvailability(availableFanMask, availableGpOutMask, availableHomeMacroMask); }
#else
		UNUSED(count);
#endif
	}

	void UpdateGpOutPercent(size_t outputIndex, int percent)
	{
#if DISPLAY_X == 800
		if (outputIndex < MaxPrintGpOutControls)
		{
			const uint8_t bit = (uint8_t)(1u << outputIndex);
			const bool newlyAvailable = (availableGpOutMask & bit) == 0;
			availableGpOutMask |= bit;
			lastGpOutPercent[outputIndex] = percent;
			if (newlyAvailable || gpOutNames[outputIndex].IsEmpty())
			{
				RequestGpOutName(outputIndex);
			}
			if (printGpOutButtons[outputIndex] != nullptr)
			{
				UpdateField(printGpOutButtons[outputIndex], percent);
				mgr.Show(printGpOutButtons[outputIndex], true);
				if (printGpOutNameFields[outputIndex] != nullptr) { mgr.Show(printGpOutNameFields[outputIndex], true); }
			}
			for (size_t slot = numToolColsUsed; slot < MaxSlots; ++slot)
			{
				const uint8_t config = nvData.GetHomeSlotConfig(slot);
				if ((config & 0xF0) == 0x20 && (config & 0x0F) == outputIndex && homeAuxValues[slot] != nullptr)
				{
					if (newlyAvailable) { ApplyHomeAuxSlot(slot); }
					UpdateField(homeAuxValues[slot], percent);
				}
			}
			if (homeSlotPicker != nullptr) { homeSlotPicker->SetAvailability(availableFanMask, availableGpOutMask, availableHomeMacroMask); }
		}
#else
		UNUSED(outputIndex); UNUSED(percent);
#endif
	}

	void UpdateToolTemp(size_t toolIndex, size_t toolHeaterIndex, int32_t temp, bool active)
	{
		OM::Tool *tool = OM::GetOrCreateTool(toolIndex);

		// If we do not handle this tool back off
		if (tool == nullptr)
		{
			return;
		}

		tool->UpdateTemp(toolHeaterIndex, temp, active);
		if (toolHeaterIndex == 0 || nvData.GetHeaterCombineType() == HeaterCombineType::notCombined)
		{
			if (tool->slot + toolHeaterIndex < MaxSlots)
			{
				UpdateField((active ? activeTemps : standbyTemps)[tool->slot + toolHeaterIndex], temp);
			}
		}
	}

	void UpdateTemperature(size_t heaterIndex, int ival, IntegerButton** fields)
	{
		OM::Slots heaterSlots;
		OM::GetHeaterSlots(heaterIndex, heaterSlots, false);	// Ignore tools
		if (!heaterSlots.IsEmpty())
		{
			const size_t count = heaterSlots.Size();
			for (size_t i = 0; i < count; ++i)
			{
				UpdateField(fields[heaterSlots[i]], ival);
			}

			heaterSlots.Clear();
		}
	}

	// Update an active temperature
	void UpdateActiveTemperature(size_t index, int ival)
	{
		UpdateTemperature(index, ival, activeTemps);
	}

	// Update a standby temperature
	void UpdateStandbyTemperature(size_t index, int ival)
	{
		UpdateTemperature(index, ival, standbyTemps);
	}

	// Update an extrusion factor
	void UpdateExtrusionFactor(size_t index, int ival)
	{
		OM::IterateToolsWhile([&index, &ival](OM::Tool*& tool, size_t) {
			if (tool->extruders.IsBitSet(index) && tool->slot < MaxSlots)
			{
				UpdateField(extrusionFactors[tool->slot], ival);
			}
			return tool->slot < MaxSlots;
		});
	}

	// Update the print speed factor
	void UpdateSpeedPercent(int ival)
	{
		UpdateField(spd, ival);
	}

	// Process a new message box alert, clearing any existing one
	void ProcessAlert(const Alert& alert)
	{
		if (isLandscape)
		{
			alertPopup->Set(alert);
			mgr.SetPopup(alertPopup, AutoPlace, AutoPlace);
		}
		alertMode = alert.mode;
		displayingResponse = false;
		whenAlertReceived = SystemTick::GetTickCount();
		alertTicks = (alertMode < 2) ? (uint32_t)(alert.timeout * 1000.0) : 0;
	}

	// Process a command to clear a message box alert
	void ClearAlert()
	{
		if (alertMode >= 0)
		{
			alertTicks = 0;
			mgr.ClearPopup(true, alertPopup);
			CurrentAlertModeClear();
			alertMode = -1;
		}
	}

	// Clear a message box alert or response. Called when the user presses the close button or the alert or response times out.
	void ClearAlertOrResponse()
	{
		if (alertMode >= 0 || displayingResponse)
		{
			alertTicks = 0;
			mgr.ClearPopup(true, alertPopup);
			CurrentAlertModeClear();
			alertMode = -1;
			displayingResponse = false;
		}
	}

	bool CanDimDisplay()
	{
		return alertMode < 2;
	}

	void ProcessSimpleAlert(const char* _ecv_array text)
	{
		if (alertMode < 2)												// if the current alert doesn't require acknowledgement
		{
			if (isLandscape)
			{
				alertPopup->Set(strings->message, text, 1, 0);
				mgr.SetPopup(alertPopup, AutoPlace, AutoPlace);
			}
			alertMode = 1;												// a simple alert is like a mode 1 alert without a title
			displayingResponse = false;
			whenAlertReceived = SystemTick::GetTickCount();
			alertTicks = 0;												// no timeout
		}
	}

	// Process a new response. This is treated like a simple alert except that it times out and isn't cleared by a "clear alert" command from the host.
	void NewResponseReceived(const char* _ecv_array text)
	{
		const bool isErrorMessage = StringStartsWith(text, "Error");
		if (   alertMode < 2											// if the current alert doesn't require acknowledgement
			&& !(currentTab == tabSetup || currentTab == tabMsg)		// don't show on setup tab or on console tab
			&& (isErrorMessage || infoTimeout != 0)
		   )
		{
			if (isLandscape)
			{
				alertPopup->Set(strings->response, text, 1, 0);
				mgr.SetPopup(alertPopup, AutoPlace, AutoPlace);
			}
			alertMode = -1;												// make sure that a call to ClearAlert doesn't clear us
			displayingResponse = true;
			whenAlertReceived = SystemTick::GetTickCount();
			alertTicks = isErrorMessage ? 0 : infoTimeout * SystemTick::TicksPerSecond;				// time out if it isn't an error message
		}
	}

	// This is called when the user selects a new file from a list of SD card files
	void FileSelected(const char * _ecv_array null fileName)
	{
		fpNameField->SetValue(fileName);
		// Clear out the old field values, they relate to the previous file we looked at until we process the response
		fpSizeField->SetValue(0);						// would be better to make it blank
		fpHeightField->SetValue(0.0);					// would be better to make it blank
		fpLayerHeightField->SetValue(0.0);				// would be better to make it blank
		fpFilamentField->SetValue(0);					// would be better to make it blank
		generatedByText.Clear();
		fpGeneratedByField->SetChanged();
		lastModifiedText.Clear();
		fpLastModifiedField->SetChanged();
		printTimeText.Clear();
		fpPrintTimeField->SetChanged();
	}

	// This is called when the "generated by" file information has been received
	void UpdateFileGeneratedByText(const char data[])
	{
		generatedByText.copy(data);
		fpGeneratedByField->SetChanged();
	}

	// This is called when the "last modified" file information has been received
	void UpdateFileLastModifiedText(const char data[])
	{
		lastModifiedText.copy(data);
		lastModifiedText.Replace('T', ' ');
		lastModifiedText.Replace('+', '\0');		// ignore time zone if present
		lastModifiedText.Replace('.', '\0');		// ignore decimal seconds if present (DCS 2.0.0 sends them)
		fpLastModifiedField->SetChanged();
	}

	// This is called when the "last modified" file information has been received
	void UpdatePrintTimeText(uint32_t seconds, bool isSimulated)
	{
		bool update = false;
		if (isSimulated)
		{
			printTimeText.Clear();					// prefer simulated to estimated print time
			fpPrintTimeField->SetLabel(strings->simulatedPrintTime);
			update = true;
		}
		else if (printTimeText.IsEmpty())
		{
			fpPrintTimeField->SetLabel(strings->estimatedPrintTime);
			update = true;
		}
		if (update)
		{
			unsigned int minutes = (seconds + 50)/60;
			printTimeText.printf("%dh %02dm", minutes / 60, minutes % 60);
			fpPrintTimeField->SetChanged();
		}
	}

	// This is called when the object height information for the file has been received
	void UpdateFileObjectHeight(float f)
	{
		fpHeightField->SetValue(f);
	}

	// This is called when the layer height information for the file has been received
	void UpdateFileLayerHeight(float f)
	{
		fpLayerHeightField->SetValue(f);
	}

	// This is called when the size of the file has been received
	void UpdateFileSize(int size)
	{
		fpSizeField->SetValue(size);
	}

	// This is called when the filament needed by the file has been received
	void UpdateFileFilament(int len)
	{
		fpFilamentField->SetValue(len);
	}

	bool UpdateFileThumbnailChunk(const struct Thumbnail &thumbnail, uint32_t pixels_offset, const qoi_rgba_t *pixels, size_t pixels_count)
	{
		dbg("offset %d pixels %08x count %d\n", pixels_offset, pixels, pixels_count);
		if (!mgr.IsPopupActive(fileDetailPopup))
		{
			return false;
		}
#define DRAW_TEST 0
#if DRAW_TEST == 1
		qoi_rgba_t pixel[100];

		//memset(pixel, 0xaa, sizeof(pixel));
		for (size_t i = 0; i < ARRAY_SIZE(pixel); i++)
		{
			pixel[i].v = 0;
			pixel[i].rgba.r = 0xaa;
		}

		fpThumbnail->DrawRect(thumbnail.width, thumbnail.height, pixels_offset, pixel, ARRAY_SIZE(pixel));
#elif DRAW_TEST == 2
		int line = 64;
		for (int i = 0; i < line; i++) {
			qoi_rgba_t test_pixels[64];

			for (size_t p = 0; p < ARRAY_SIZE(test_pixels); p++) {
				test_pixels[p].v = 0;
				test_pixels[p].rgba.r = 128 + p;
				test_pixels[p].rgba.g = 64 + i;
			}

			fpThumbnail->DrawRect(ARRAY_SIZE(test_pixels), 1, i * ARRAY_SIZE(test_pixels), test_pixels, ARRAY_SIZE(test_pixels));
		}
#else
		fpThumbnail->DrawRect(thumbnail.width, thumbnail.height, pixels_offset, pixels, pixels_count);
#endif
		return true;
	}

	// Return true if we are displaying file information
	bool IsDisplayingFileInfo()
	{
		return currentFile != nullptr;
	}

	static void DoEmergencyStop()
	{
		// We send M112 for the benefit of old firmware, and F0 0F (an invalid UTF8 sequence) for new firmware
		SerialIo::Sendf("M112 ;" "\xF0" "\x0F" "\n");
		TouchBeep();											// needed when we are called from ProcessTouchOutsidePopup
		Delay(1000);
		SerialIo::Sendf("M999\n");
		Delay(1000);
	}

	// Make this into a template if we need something else than IntegerButton** as list
	size_t GetButtonSlot(IntegerButton** buttonList, ButtonBase* button)
	{
		size_t slot = MaxSlots;
		for (size_t i = 0; i < MaxSlots; ++i)
		{
			if (buttonList[i] == button)
			{
				slot = i;
				break;
			}
		}
		return slot;
	}

	void ProcessRelease(ButtonPress bp)
	{
		if (!bp.IsValid())
		{
			return;
		}

		ButtonBase *f = bp.GetButton();
		Event ev = (Event)(f->GetEvent());

		switch(ev)
		{
		case evTabControl:
		case evTabStatus:
		case evTabMsg:
		case evTabSetup:

		case evExtrudeAmount:
		case evExtrudeRate:
#if DISPLAY_X == 800
		case evModernJogStep:
		case evModernFeedrate:
#endif

		case evAdjustBaudRate:
		case evAdjustVolume:
		case evAdjustInfoTimeout:
		case evAdjustScreensaverTimeout:
		case evAdjustBabystepAmount:
		case evAdjustFeedrate:
		case evAdjustColours:
		case evAdjustLanguage:
			break;
		case evOkAlert:
		case evCloseAlert:
		case evChoiceAlert:
			mgr.Press(bp, false);
			ClearAlertOrResponse();
			break;
		default:
			mgr.Press(bp, false);
			break;
		}
	}

	// Process a touch event
	void ProcessTouch(ButtonPress bp)
	{
		if (bp.IsValid())
		{
			ButtonBase *f = bp.GetButton();
			currentButton = bp;
			mgr.Press(bp, true);
			Event ev = (Event)(f->GetEvent());


			if (bp.GetEvent() != evAdjustVolume)
			{
				TouchBeep();		// give audible feedback of the touch, unless adjusting the volume
			}

			switch(ev)
			{
			case evEmergencyStop:
				DoEmergencyStop();
				break;

			case evTabControl:
			case evTabStatus:
			case evTabMsg:
			case evTabSetup:
				if (ChangePage(f))
				{
					currentButton.Clear();						// keep the button highlighted after it is released
				}
				break;

			case evAdjustToolActiveTemp:
			case evAdjustToolStandbyTemp:
			case evAdjustBedActiveTemp:
			case evAdjustBedStandbyTemp:
			case evAdjustChamberActiveTemp:
			case evAdjustChamberStandbyTemp:
				if (static_cast<IntegerButton*>(f)->GetValue() < 0)
				{
					static_cast<IntegerButton*>(f)->SetValue(0);
				}
				Adjusting(bp);
				if (isLandscape)
				{
#if DISPLAY_X == 800
					UpdateAdjustmentPopup(bp);
#endif
					mgr.SetPopup(setTempPopup, AutoPlace, popupY);
				}
				break;

			case evAdjustActiveRPM:
				Adjusting(bp);
				if (isLandscape)
				{
#if DISPLAY_X == 800
					UpdateAdjustmentPopup(bp);
#endif
					mgr.SetPopup(setRPMPopup, AutoPlace, popupY);
				}
				break;

			case evAdjustSpeed:
			case evExtrusionFactor:
			case evAdjustFan:
			case evAdjustGpOut:
				oldIntValue = GetAdjustableIntValue(bp.GetButton());
				Adjusting(bp);
				if (isLandscape)
				{
#if DISPLAY_X == 800
					UpdateAdjustmentPopup(bp);
#endif
					mgr.SetPopup(setTempPopup, AutoPlace, popupY);
				}
				break;

			case evSetInt:
				if (fieldBeingAdjusted.IsValid())
				{
					int val = GetAdjustableIntValue(fieldBeingAdjusted.GetButton());
					const event_t eventOfFieldBeingAdjusted = fieldBeingAdjusted.GetEvent();
					switch (eventOfFieldBeingAdjusted)
					{
					case evAdjustBedActiveTemp:
					case evAdjustChamberActiveTemp:
						{
							int index = fieldBeingAdjusted.GetIParam();
							const bool isBed = eventOfFieldBeingAdjusted == evAdjustBedActiveTemp;
							SerialIo::Sendf("%s P%d S%d\n", isBed ? "M140" : "M141", index, val);
						}
						break;

					case evAdjustBedStandbyTemp:
					case evAdjustChamberStandbyTemp:
						{
							int index = fieldBeingAdjusted.GetIParam();
							const bool isBed = eventOfFieldBeingAdjusted == evAdjustBedStandbyTemp;
							SerialIo::Sendf("%s P%d R%d\n", isBed ? "M140" : "M141", index, val);
						}
						break;

					case evAdjustToolActiveTemp:
						{
							int toolNumber = fieldBeingAdjusted.GetIParam();
							OM::Tool* tool = OM::GetTool(toolNumber);
							if (tool == nullptr)
							{
								break;
							}

							const bool useM568 = GetFirmwareFeatures().IsBitSet(m568TempAndRPM);
							if (nvData.GetHeaterCombineType() == HeaterCombineType::combined)
							{
								tool->UpdateTemp(0, val, true);
								SerialIo::Sendf("%s P%d S%d\n", (useM568 ? "M568" : "G10"), toolNumber, tool->heaters[0]->activeTemp);
							}
							else
							{

								// Find the slot for this button to determine which heater index it is
								{
									size_t slot = GetButtonSlot(activeTemps, fieldBeingAdjusted.GetButton());
									if (slot >= MaxSlots || (slot - tool->slot) >= MaxSlots)
									{
										break;
									}
									tool->UpdateTemp(slot - tool->slot, val, true);
								}

								String<maxUserCommandLength> heaterTemps;
								if (tool->GetHeaterTemps(heaterTemps.GetRef(), true))
								{
									SerialIo::Sendf("%s P%d S%s\n", (useM568 ? "M568" : "G10"), toolNumber, heaterTemps.c_str());
								}
							}
						}
						break;

					case evAdjustToolStandbyTemp:
						{
							int toolNumber = fieldBeingAdjusted.GetIParam();
							OM::Tool* tool = OM::GetTool(toolNumber);
							if (tool == nullptr)
							{
								break;
							}

							const bool useM568 = GetFirmwareFeatures().IsBitSet(m568TempAndRPM);
							if (nvData.GetHeaterCombineType() == HeaterCombineType::combined)
							{
								tool->UpdateTemp(0, val, false);
								SerialIo::Sendf("%s P%d R%d\n", (useM568 ? "M568" : "G10"), toolNumber, tool->heaters[0]->standbyTemp);
							}
							else
							{

								// Find the slot for this button to determine which heater index it is
								{
									size_t slot = GetButtonSlot(standbyTemps, fieldBeingAdjusted.GetButton());
									if (slot >= MaxSlots || (slot - tool->slot) >= MaxSlots)
									{
										break;
									}
									tool->UpdateTemp(slot - tool->slot, val, false);
								}

								String<maxUserCommandLength> heaterTemps;
								if (tool->GetHeaterTemps(heaterTemps.GetRef(), false))
								{
									SerialIo::Sendf("%s P%d R%s\n", (useM568 ? "M568" : "G10"), toolNumber, heaterTemps.c_str());
								}
							}
						}
						break;

					case evAdjustActiveRPM:
						{
							auto spindle = OM::GetSpindle(fieldBeingAdjusted.GetIParam());
							if (val == 0)
							{
								SerialIo::Sendf("M5 P%d\n", spindle->index);
							}
							else
							{
								SerialIo::Sendf("M%d P%d S%d\n", val < 0 ? 4 : 3, spindle->index, abs(val));
							}
						}
						break;

					case evExtrusionFactor:
						{
							const int extruder = fieldBeingAdjusted.GetIParam();
							SerialIo::Sendf("M221 D%d S%d\n", extruder, val);
						}
						break;

					case evAdjustFan:
						SerialIo::Sendf("M106 P%d S%d\n", fieldBeingAdjusted.GetIParam(), (255 * val)/100);
						break;

					case evAdjustGpOut:
						// RepRapFirmware gpOut PWM is expressed as a fraction 0..1.
						if (val <= 0)
						{
							SerialIo::Sendf("M42 P%d S0\n", fieldBeingAdjusted.GetIParam());
						}
						else if (val >= 100)
						{
							SerialIo::Sendf("M42 P%d S1\n", fieldBeingAdjusted.GetIParam());
						}
						else
						{
							SerialIo::Sendf("M42 P%d S0.%02d\n", fieldBeingAdjusted.GetIParam(), val);
						}
						break;

					default:
						{
							const char* null cmd = fieldBeingAdjusted.GetSParam();
							if (cmd != nullptr)
							{
								SerialIo::Sendf("%s%d\n", cmd, val);
							}
						}
						break;
					}
					mgr.ClearPopup();
					StopAdjusting();
				}
				break;

			case evAdjustInt:
				if (fieldBeingAdjusted.IsValid())
				{
					ButtonBase * const adjustedButton = fieldBeingAdjusted.GetButton();
					const int change = bp.GetIParam();
					int newValue = GetAdjustableIntValue(adjustedButton) + change;
					switch(fieldBeingAdjusted.GetEvent())
					{
					case evAdjustToolActiveTemp:
					case evAdjustToolStandbyTemp:
					case evAdjustBedActiveTemp:
					case evAdjustBedStandbyTemp:
					case evAdjustChamberActiveTemp:
					case evAdjustChamberStandbyTemp:
						newValue = constrain<int>(newValue, 0, 1600);		// some users want to print at high temperatures
						break;

					case evAdjustFan:
					case evAdjustGpOut:
						newValue = constrain<int>(newValue, 0, 100);
						break;

					case evAdjustActiveRPM:
						{
							auto spindle = OM::GetSpindle(fieldBeingAdjusted.GetIParam());
							newValue = constrain<int>(newValue, -spindle->max, spindle->max);

							// If a change will lead us below the min speed for spindle skip to the other side
							if (newValue > (int)-spindle->min && newValue < (int)spindle->min)
							{
								newValue = (change < 0) ? -spindle->min : spindle->min;
							}
						}
						break;

					default:
						break;
					}
					SetAdjustableIntValue(adjustedButton, newValue);
#if DISPLAY_X == 800
					UpdateAdjustmentPopup(fieldBeingAdjusted);
#endif
				}
				break;

#if DISPLAY_X == 800
			case evConfigureHomeSlot:
				{
					const size_t slot = (size_t)bp.GetIParam();
					if (slot < MaxSlots && slot >= numToolColsUsed && homeSlotPopup != nullptr && homeSlotPicker != nullptr)
					{
						homeSlotBeingConfigured = slot;
						FileManager::RefreshMacrosList();
						homeSlotPicker->SetAvailability(availableFanMask, availableGpOutMask, availableHomeMacroMask);
						homeSlotPicker->SetSelected(nvData.GetHomeSlotConfig(slot));
						mgr.SetPopup(homeSlotPopup, AutoPlace, AutoPlace);
					}
				}
				break;

			case evAssignHomeSlot:
				if (homeSlotBeingConfigured < MaxSlots && homeSlotBeingConfigured >= numToolColsUsed)
				{
					nvData.SetHomeSlotConfig(homeSlotBeingConfigured, (uint8_t)bp.GetIParam());
					SaveSettings();
					ApplyHomeAuxSlot(homeSlotBeingConfigured);
				}
				homeSlotBeingConfigured = MaxSlots;
				mgr.ClearPopup();
				currentButton.Clear();
				break;
#endif

			case evMovePopup:
#if DISPLAY_X == 800
				UpdateMoveBedMapGeometry();
				if (moveToolField != nullptr)
				{
					if (currentTool >= 0) moveToolText.printf("T%ld", (long)currentTool);
					else moveToolText.copy("T--");
					moveToolField->SetValue(moveToolText.c_str());
				}
#endif
#if DISPLAY_X == 800
				mgr.SetPopup(movePopup, 0, 0);
#else
				mgr.SetPopup(movePopup, AutoPlace, AutoPlace);
#endif
				break;

#if DISPLAY_X == 800
			case evMoveBedMap:
				{
					float tx, ty;
					if (moveBedMap != nullptr && moveBedMap->TouchToMachine(bp.GetIndex(), tx, ty))
					{
						SerialIo::Sendf("G90 G1 X%.2f Y%.2f F%d\n", (double)tx, (double)ty, modernFeedrateMmS * 60);
					}
				}
				break;

			case evModernJog:
				{
					const int direction = bp.GetIParam();
					char axisLetter = 'X';
					float distance = modernJogStep;
					switch (direction)
					{
					case 0: axisLetter = 'X'; distance = -modernJogStep; break;
					case 1: axisLetter = 'X'; distance =  modernJogStep; break;
					case 2: axisLetter = 'Y'; distance =  modernJogStep; break;
					case 3: axisLetter = 'Y'; distance = -modernJogStep; break;
					case 4: axisLetter = 'Z'; distance = nvData.GetInvertZ() ? -modernJogStep : modernJogStep; break;
					case 5: axisLetter = 'Z'; distance = nvData.GetInvertZ() ? modernJogStep : -modernJogStep; break;
					default: break;
					}
					SerialIo::Sendf("G91 G1 %c%.3f F%d G90\n", axisLetter, (double)distance, modernFeedrateMmS * 60);
				}
				break;

			case evModernJogStep:
				mgr.Press(modernJogStepPress, false);
				mgr.Press(bp, true);
				modernJogStepPress = bp;
				modernJogStep = (float)atof(bp.GetSParam());
				currentButton.Clear();
				break;

			case evModernFeedrate:
				mgr.Press(modernFeedratePress, false);
				mgr.Press(bp, true);
				modernFeedratePress = bp;
				modernFeedrateMmS = atoi(bp.GetSParam());
				if (modernFeedrateMmS < 1) { modernFeedrateMmS = 1; }
				currentButton.Clear();
				break;
#endif

			case evMoveSelectAxis:
				{
					alertPopup->ChangeLetter(bp.GetIParam());
				}
				break;
			case evMoveAxis:
				{
					TextButtonForAxis *textButton = static_cast<TextButtonForAxis*>(bp.GetButton());
					const char letter = textButton->GetAxisLetter();
					if (toupper(letter) == 'Z' && nvData.GetInvertZ())
					{
						SerialIo::Sendf("G91 G1 %s%c%.3f F%d G90\n", islower(letter) ? "'" : "", letter, (double)-atof(bp.GetSParam()), nvData.GetFeedrate());
					}
					else
					{
						SerialIo::Sendf("G91 G1 %s%c%s F%d G90\n", islower(letter) ? "'" : "", letter, bp.GetSParam(), nvData.GetFeedrate());
					}
				}
				break;

			case evExtrudePopup:
				if (isLandscape)
				{
					mgr.SetPopup(extrudePopup, AutoPlace, AutoPlace);
				}
				break;

			case evExtrudeAmount:
				mgr.Press(currentExtrudeAmountPress, false);
				mgr.Press(bp, true);
				currentExtrudeAmountPress = bp;
				currentButton.Clear();						// stop it being released by the timer
				break;

			case evExtrudeRate:
				mgr.Press(currentExtrudeRatePress, false);
				mgr.Press(bp, true);
				currentExtrudeRatePress = bp;
				currentButton.Clear();						// stop it being released by the timer
				break;

			case evExtrude:
			case evRetract:
#if DISPLAY_X == 800
				if (mgr.IsPopupActive(movePopup))
				{
					// In the modern Motion workspace STEP is also the extrusion distance,
					// and SPEED is deliberately expressed to the user in mm/s.
					SerialIo::Sendf("M120 M83 G1 E%s%.3f F%d M121\n",
							(ev == evRetract ? "-" : ""), (double)modernJogStep, modernFeedrateMmS * 60);
				}
				else
#endif
				if (currentExtrudeAmountPress.IsValid() && currentExtrudeRatePress.IsValid())
				{
					SerialIo::Sendf("M120 M83 G1 E%s%s F%s M121\n",
							(ev == evRetract ? "-" : ""),
							currentExtrudeAmountPress.GetSParam(),
							currentExtrudeRatePress.GetSParam());
				}
				break;

			case evBabyStepPopup:
				mgr.SetPopup(babystepPopup, AutoPlace, AutoPlace);
				break;

			case evBabyStepMinus:
			case evBabyStepPlus:
				{
					SerialIo::Sendf("M290 Z%s%s\n", (ev == evBabyStepMinus ? "-" : ""), babystepAmounts[nvData.GetBabystepAmountIndex()]);
					float currentBabystepAmount = babystepOffsetField->GetValue();
					if (ev == evBabyStepMinus)
					{
						currentBabystepAmount -= babystepAmountsF[nvData.GetBabystepAmountIndex()];
					}
					else
					{
						currentBabystepAmount += babystepAmountsF[nvData.GetBabystepAmountIndex()];
					}
					babystepOffsetField->SetValue(currentBabystepAmount);
				}
				break;

			case evListFiles:
				FileManager::DisplayFilesList();
				break;

			case evListMacros:
				FileManager::DisplayMacrosList();
				break;

			case evCalTouch:
				CalibrateTouch();
				break;

			case evFactoryReset:
				PopupAreYouSure(ev, strings->confirmFactoryReset);
				break;

			case evSelectBed:
				{
					int bedIndex = bp.GetIParam();
					const OM::Bed* bed = OM::GetBed(bedIndex);
					if (bed == nullptr || bed->slot >= MaxSlots)
					{
						break;
					}
					const auto slot = bed->slot;
					if (bed->heaterStatus == OM::HeaterStatus::active)			// if bed is active
					{
						SerialIo::Sendf("M144 P%d\n", bedIndex);
					}
					else
					{
						SerialIo::Sendf("M140 P%d S%d\n", bedIndex, activeTemps[slot]->GetValue());
					}
				}
				break;

			case evSelectChamber:
				{
					const int chamberIndex = bp.GetIParam();
					const OM::Chamber* chamber = OM::GetChamber(chamberIndex);
					if (chamber == nullptr || chamber->slot >= MaxSlots)
					{
						break;
					}
					const auto slot = chamber->slot;
					SerialIo::Sendf("M141 P%d S%d\n",
							chamberIndex,
							(chamber->heaterStatus == OM::HeaterStatus::active ? -274 : activeTemps[slot]->GetValue()));
				}
				break;

			case evSelectHead:
				{
					int head = bp.GetIParam();
					// pressing a evSeelctHead button in the middle of active printing is almost always accidental (and fatal to the print job)
					if (GetStatus() != OM::PrinterStatus::printing && GetStatus() != OM::PrinterStatus::simulating)
					{
						if (head == currentTool)		// if head is active
						{
							SerialIo::Sendf("T-1\n");
						}
						else
						{
							SerialIo::Sendf("T%d\n", head);
						}
					}
				}
				break;

			case evFile:
				{
					const char * _ecv_array fileName = bp.GetSParam();
					if (fileName != nullptr)
					{
						if (fileName[0] == '*')
						{
							// It's a directory
							FileManager::RequestFilesSubdir(fileName + 1);
							//??? need to pop up a "wait" box here
						}
						else
						{
							// It's a regular file
							currentFile = fileName;
							FileSelected(currentFile);
							mgr.SetPopup(fileDetailPopup, AutoPlace, AutoPlace);
						}
					}
					else
					{
						ErrorBeep();
					}
				}
				break;

			case evFilesUp:
				FileManager::RequestFilesParentDir();
				break;

			case evMacrosUp:
				FileManager::RequestMacrosParentDir();
				break;

			case evMacro:
			case evMacroControlPage:
				{
					const char *fileName = bp.GetSParam();
					if (fileName != nullptr)
					{
						if (fileName[0] == '*')		// if it's a directory
						{
							FileManager::RequestMacrosSubdir(fileName + 1);
							//??? need to pop up a "wait" box here
						}
						else
						{
							SerialIo::Sendf("M98 P");
							const char * _ecv_array const dir = (ev == evMacroControlPage) ? FileManager::GetMacrosRootDir() : FileManager::GetMacrosDir();
							SerialIo::SendFilename(CondStripDrive(dir), fileName);
							SerialIo::SendChar('\n');
						}
					}
					else
					{
						ErrorBeep();
					}
				}
				break;

			case evPrintFile:
			case evSimulateFile:
				mgr.ClearPopup();			// clear the file info popup
				mgr.ClearPopup();			// clear the file list popup
				if (currentFile != nullptr)
				{
					SerialIo::Sendf((ev == evSimulateFile) ? "M37 P" : "M32 ");
					SerialIo::SendFilename(CondStripDrive(StripPrefix(FileManager::GetFilesDir())), currentFile);
					SerialIo::SendChar('\n');
					PrintingFilenameChanged(currentFile);
					currentFile = nullptr;							// allow the file list to be updated
					CurrentButtonReleased();
					PrintStarted();
				}
				break;

			case evReprint:
			case evResimulate:
				if (lastJobFileNameAvailable)
				{
					SerialIo::Sendf("%s{job.lastFileName}\n", (ev == evResimulate) ? "M37 P" : "M32 ");
					CurrentButtonReleased();
					PrintStarted();
				}
				break;

			case evCancel:
#if DISPLAY_X == 800
				homeSlotBeingConfigured = MaxSlots;
#endif
				eventToConfirm = evNull;
				currentFile = nullptr;
				CurrentButtonReleased();
				PopupCancelled();
				mgr.ClearPopup();
				break;

			case evDeleteFile:
				CurrentButtonReleased();
				PopupAreYouSure(ev, strings->confirmFileDelete);
				break;

			case evSendCommand:
			case evPausePrint:
			case evResumePrint:
			case evReset:
				SerialIo::Sendf("%s\n", bp.GetSParam());
				break;

			case evHomeAxis:
				{
					const char letter = bp.GetSParam()[0];
					SerialIo::Sendf("G28 %s%c0\n", islower(letter) ? "'" : "", letter);
				}
				break;

			case evScrollFiles:
				FileManager::ScrollFiles(bp.GetIParam() * NumFileRows);
				break;

			case evScrollMacros:
				FileManager::ScrollMacros(bp.GetIParam() * NumMacroRows);
				break;

			case evChangeCard:
				(void)FileManager::NextCard();
				break;

			case evKeyboard:
				ShowKeyboard();
				break;

			case evInvertX:
				MirrorDisplay();
				CalibrateTouch();
				break;

			case evInvertY:
				InvertDisplay();
				CalibrateTouch();
				break;

			case evInvertZ:
				nvData.SetInvertZ(!nvData.GetInvertZ());
				invertZButton->SetText(nvData.GetInvertZ() ? "Invert Z: ON" : "Invert Z: OFF");
#if DISPLAY_X == 800
				if (modernJogZTopButton != nullptr) modernJogZTopButton->SetText(nvData.GetInvertZ() ? "-Z" : "Z+");
				if (modernJogZBottomButton != nullptr) modernJogZBottomButton->SetText(nvData.GetInvertZ() ? "Z+" : "-Z");
#endif
				SaveSettings();
				break;

			case evSetBaudRate:
				Adjusting(bp);
				mgr.SetPopup(baudPopup, AutoPlace, popupY);
				break;

			case evAdjustBaudRate:
				{
					const int rate = bp.GetIParam();
					SetBaudRate(rate);
					baudRateButton->SetValue(rate);
				}
				CurrentButtonReleased();
				mgr.ClearPopup();
				StopAdjusting();
				break;

			case evSetVolume:
				Adjusting(bp);
				mgr.SetPopup(volumePopup, AutoPlace, popupY);
				break;

			case evSetInfoTimeout:
				Adjusting(bp);
				mgr.SetPopup(infoTimeoutPopup, AutoPlace, popupY);
				break;

			case evSetScreensaverTimeout:
				Adjusting(bp);
				mgr.SetPopup(screensaverTimeoutPopup, AutoPlace, popupY);
				break;

			case evSetBabystepAmount:
				Adjusting(bp);
				mgr.SetPopup(babystepAmountPopup, AutoPlace, popupY);
				break;

			case evSetFeedrate:
				Adjusting(bp);
				mgr.SetPopup(feedrateAmountPopup, AutoPlace, popupY);
				break;

			case evSetColours:
				if (coloursPopup != nullptr)
				{
					Adjusting(bp);
					mgr.SetPopup(coloursPopup, AutoPlace, popupY);
				}
				break;

			case evBrighter:
			case evDimmer:
				ChangeBrightness(ev == evBrighter);
				break;

			case evAdjustVolume:
				{
					const int newVolume = bp.GetIParam();
					nvData.SetVolume(newVolume);
					volumeButton->SetValue(newVolume);
				}
				TouchBeep();									// give audible feedback of the touch at the new volume level
				break;

			case evAdjustInfoTimeout:
				{
					infoTimeout = bp.GetIParam();
					nvData.SetInfoTimeout(infoTimeout);
					infoTimeoutButton->SetValue(infoTimeout);
				}
				TouchBeep();									// give audible feedback of the touch at the new volume level
				break;

			case evAdjustScreensaverTimeout:
				{
					uint32_t screensaverTimeout = bp.GetIParam();
					nvData.SetScreensaverTimeout(screensaverTimeout * 1000);
					screensaverTimeoutButton->SetValue(screensaverTimeout);
				}
				TouchBeep();									// give audible feedback of the touch at the new volume level
				break;

			case evAdjustBabystepAmount:
				{
					uint32_t babystepAmountIndex = bp.GetIParam();
					nvData.SetBabystepAmountIndex(babystepAmountIndex);
					babystepAmountButton->SetText(babystepAmounts[babystepAmountIndex]);
					babystepMinusButton->SetText(babystepAmounts[babystepAmountIndex]);
					babystepPlusButton->SetText(babystepAmounts[babystepAmountIndex]);
				}
				TouchBeep();									// give audible feedback of the touch at the new volume level
				break;

			case evAdjustFeedrate:
				{
					uint32_t feedrate = bp.GetIParam();
					nvData.SetFeedrate(feedrate);
					feedrateAmountButton->SetValue(feedrate);
				}
				TouchBeep();									// give audible feedback of the touch at the new volume level
				break;

			case evAdjustColours:
				{
					const uint8_t newColours = (uint8_t)bp.GetIParam();
					if (nvData.SetColourScheme(newColours))
					{
						SaveSettings();
						Reset();
					}
				}
				mgr.ClearPopup();
				break;

			case evSetLanguage:
				Adjusting(bp);
				mgr.SetPopup(languagePopup, AutoPlace, popupY);
				break;

			case evAdjustLanguage:
				{
					const uint8_t newLanguage = (uint8_t)bp.GetIParam();
					if (nvData.SetLanguage(newLanguage))
					{
						SaveSettings();
						Reset();
					}
				}
				mgr.ClearPopup();
				break;

			case evSetDimmingType:
				ChangeDisplayDimmerType();
				dimmingTypeButton->SetText(strings->displayDimmingNames[(unsigned int)nvData.GetDisplayDimmerType()]);
				break;

			case evSetHeaterCombineType:
				ChangeHeaterCombineType();
				heaterCombiningButton->SetText(strings->heaterCombineTypeNames[(unsigned int)nvData.GetHeaterCombineType()]);
				break;

			case evSetLogLevel:
				{
					MessageLog::LogLevel logLevel = MessageLog::LogLevelGet();

					logLevel = (MessageLog::LogLevel)(((int)logLevel + 1) % (int)MessageLog::LogLevel::NumTypes);

					logLevelButton->SetText(strings->logLevelNames[(unsigned int)logLevel]);

					nvData.SetLogLevel(logLevel);

					MessageLog::LogLevelSet(logLevel);
				}
				break;

			case evYes:
				CurrentButtonReleased();
				mgr.ClearPopup();								// clear the yes/no popup
				switch (eventToConfirm)
				{
				case evFactoryReset:
					FactoryReset();
					break;

				case evDeleteFile:
					if (currentFile != nullptr)
					{
						mgr.ClearPopup();						// clear the file info popup
						SerialIo::Sendf("M30 ");
						SerialIo::SendFilename(CondStripDrive(StripPrefix(FileManager::GetFilesDir())), currentFile);
						SerialIo::SendChar('\n');
						FileManager::RefreshFilesList();
						currentFile = nullptr;
					}
					break;

				default:
					break;
				}
				eventToConfirm = evNull;
				currentFile = nullptr;
				break;

			case evKey:
				if (!userCommandBuffers[currentUserCommandBuffer].cat((char)bp.GetIParam()))
				{
					TextField * const kf = ActiveKeyboardField();
					if (kf != nullptr) kf->SetChanged();
				}
				break;

			case evShift:
				{
					size_t rowOffset;
					if (keyboardShifted)
					{
						bp.GetButton()->Press(false, 0);
						rowOffset = 0;
					}
					else
					{
						rowOffset = 4;
					}
					CharButtonRow ** const rows = ActiveKeyboardRows();
					const char* _ecv_array const * const layout = ActiveKeyboardLayout();
					for (size_t i = 0; i < 4; ++i)
					{
						if (rows[i] != nullptr) rows[i]->ChangeText(layout[i + rowOffset]);
					}
				}
				keyboardShifted = !keyboardShifted;
				currentButton.Clear();				// make the key sticky
				break;

			case evBackspace:
				if (!userCommandBuffers[currentUserCommandBuffer].IsEmpty())
				{
					userCommandBuffers[currentUserCommandBuffer].Erase(userCommandBuffers[currentUserCommandBuffer].strlen() - 1);
					TextField * const kf = ActiveKeyboardField();
					if (kf != nullptr) kf->SetChanged();
				}
				break;

			case evUp: // TODO new events for moving editor one left or right
				currentHistoryBuffer = (currentHistoryBuffer + numUserCommandBuffers - 1) % numUserCommandBuffers;
				if (currentHistoryBuffer == currentUserCommandBuffer)
				{
					userCommandBuffers[currentUserCommandBuffer].Clear();
				}
				else
				{
					userCommandBuffers[currentUserCommandBuffer].copy(userCommandBuffers[currentHistoryBuffer].c_str());
				}
				{ TextField * const kf = ActiveKeyboardField(); if (kf != nullptr) kf->SetChanged(); }
				break;

			case evDown:
				currentHistoryBuffer = (currentHistoryBuffer + 1) % numUserCommandBuffers;
				if (currentHistoryBuffer == currentUserCommandBuffer)
				{
					userCommandBuffers[currentUserCommandBuffer].Clear();
				}
				else
				{
					userCommandBuffers[currentUserCommandBuffer].copy(userCommandBuffers[currentHistoryBuffer].c_str());
				}
				{ TextField * const kf = ActiveKeyboardField(); if (kf != nullptr) kf->SetChanged(); }
				break;

			case evSendKeyboardCommand:
				if (userCommandBuffers[currentUserCommandBuffer].strlen() != 0)
				{
					if (keyboardDataHandler)
					{
						keyboardDataHandler(userCommandBuffers[currentUserCommandBuffer].c_str());
					}

					// Add the command to the history if it was different frmo the previous command
					size_t prevBuffer = (currentUserCommandBuffer + numUserCommandBuffers - 1) % numUserCommandBuffers;
					if (strcmp(userCommandBuffers[currentUserCommandBuffer].c_str(), userCommandBuffers[prevBuffer].c_str()) != 0)
					{
						currentUserCommandBuffer = (currentUserCommandBuffer + 1) % numUserCommandBuffers;
					}
					currentHistoryBuffer = currentUserCommandBuffer;
					userCommandBuffers[currentUserCommandBuffer].Clear();
					TextField * const kf = ActiveKeyboardField();
					if (kf != nullptr) kf->SetLabel(userCommandBuffers[currentUserCommandBuffer].c_str());
				}
				break;

			case evOkAlert:
				alertPopup->ProcessOkButton();
				break;

			case evCloseAlert:
				SerialIo::Sendf("%s\n", bp.GetSParam());
				break;

			case evChoiceAlert:
				alertPopup->ProcessChoice(bp.GetIParam());
				break;

			case evEditAlert:
				keyboardDataHandler = PopupEditData;
				mgr.SetPopup(keyboardPopup, AutoPlace, keyboardPopupY);
				keyboardIsDisplayed = true;
				break;

			default:
				break;
			}
		}
	}

	// Process a touch event outside the popup on the field being adjusted
	void ProcessTouchOutsidePopup(ButtonPress bp)
	{
		if (!IsSetupTab())
		{
			return;
		}

		if (bp == fieldBeingAdjusted)
		{
			TouchBeep();
			switch(fieldBeingAdjusted.GetEvent())
			{
			case evAdjustSpeed:
			case evExtrusionFactor:
			case evAdjustFan:
			case evAdjustGpOut:
				SetAdjustableIntValue(fieldBeingAdjusted.GetButton(), oldIntValue);
				mgr.ClearPopup();
				StopAdjusting();
				break;

			case evAdjustToolActiveTemp:
			case evAdjustToolStandbyTemp:
			case evAdjustBedActiveTemp:
			case evAdjustBedStandbyTemp:
			case evAdjustChamberActiveTemp:
			case evAdjustChamberStandbyTemp:
			case evAdjustActiveRPM:
			case evSetBaudRate:
			case evSetVolume:
			case evSetInfoTimeout:
			case evSetScreensaverTimeout:
			case evSetFeedrate:
			case evSetBabystepAmount:
			case evSetColours:
				mgr.ClearPopup();
				StopAdjusting();
				break;

			case evSetLanguage:
				mgr.ClearPopup();
				StopAdjusting();
				break;
			}
		}
		else
		{
			switch(bp.GetEvent())
			{
			case evEmergencyStop:
				mgr.Press(bp, true);
				DoEmergencyStop();
				mgr.Press(bp, false);
				break;

			case evTabControl:
			case evTabStatus:
			case evTabMsg:
			case evTabSetup:
				StopAdjusting();
				TouchBeep();
				{
					ButtonBase *btn = bp.GetButton();
					if (ChangePage(btn))
					{
						currentButton.Clear();						// keep the button highlighted after it is released
					}
				}
				break;

			case evSetBaudRate:
			case evSetVolume:
			case evSetInfoTimeout:
			case evSetScreensaverTimeout:
			case evSetFeedrate:
			case evSetBabystepAmount:
			case evSetColours:
			case evSetLanguage:
			case evCalTouch:
			case evInvertX:
			case evInvertY:
			case evInvertZ:
			case evFactoryReset:
				// On the Setup tab, we allow any other button to be pressed to exit the current popup
				StopAdjusting();
				mgr.ClearPopup();
				ProcessTouch(bp);
				break;

			default:
				break;
			}
		}
	}

	// This is called when a button press times out
	void OnButtonPressTimeout()
	{
		if (currentButton.IsValid())
		{
			CurrentButtonReleased();
		}
	}

	void DisplayFilesPopup(int cardNumber, unsigned int numVolumes)
	{
		filePopupTitleField->SetValue(cardNumber);
		mgr.Show(changeCardButton, numVolumes > 1);

		if (isLandscape)
		{
			for (size_t i = 0; i < ARRAY_SIZE(filenameButtons); i++)
			{
				filenameButtons[i]->Press(false, 0);
				filenameButtons[i]->Show(false);
			}
			fileListPopupNoFiles->Show(true);
			mgr.SetPopup(fileListPopup, AutoPlace, AutoPlace);
		}
	}

	void FileListCardButtonUpdate(unsigned int numVolumes)
	{
		mgr.Show(changeCardButton, numVolumes > 1);
	}

	void DisplayMacrosPopup()
	{
		if (isLandscape)
		{
			for (size_t i = 0; i < ARRAY_SIZE(macroButtons); i++)
			{
				macroButtons[i]->Press(false, 0);
				macroButtons[i]->Show(false);
			}
			mgr.SetPopup(macrosPopup, AutoPlace, AutoPlace);
		}
	}

	void FileListLoaded(bool filesNotMacros, int errCode)
	{
		FileListButtons& buttons = (filesNotMacros) ? filesListButtons : macrosListButtons;
		if (errCode == 0)
		{
			mgr.Show(buttons.errorField, false);
		}
		else
		{
			buttons.errorField->SetValue(errCode);
			mgr.Show(buttons.errorField, true);
		}
	}

	void EnableFileNavButtons(bool filesNotMacros, bool scrollEarlier, bool scrollLater, bool parentDir)
	{
		FileListButtons& buttons = (filesNotMacros) ? filesListButtons : macrosListButtons;
		mgr.Show(buttons.scrollLeftButton, scrollEarlier);
		mgr.Show(buttons.scrollRightButton, scrollLater);
		mgr.Show(buttons.folderUpButton, parentDir);
	}

	// Update the specified button in the file or macro buttons list. If 'text' is nullptr then hide the button, else display it.
	void UpdateFileButton(bool filesNotMacros, unsigned int buttonIndex, const char * _ecv_array null text, const char * _ecv_array null param)
	{
#if DISPLAY_X == 800
		if (!filesNotMacros && buttonIndex < 8)
		{
			const uint8_t bit = (uint8_t)(1u << buttonIndex);
			if (text != nullptr && param != nullptr && param[0] != '*')
			{
				homeMacroNames[buttonIndex].copy(text);
				homeMacroFiles[buttonIndex].copy(param);
				availableHomeMacroMask |= bit;
			}
			else
			{
				homeMacroNames[buttonIndex].Clear();
				homeMacroFiles[buttonIndex].Clear();
				availableHomeMacroMask &= (uint8_t)~bit;
			}
			if (homeSlotPicker != nullptr)
			{
				homeSlotPicker->SetMacroLabel(buttonIndex, GetHomeMacroDisplayName(buttonIndex));
				homeSlotPicker->SetAvailability(availableFanMask, availableGpOutMask, availableHomeMacroMask);
			}
			for (size_t slot = numToolColsUsed; slot < MaxSlots; ++slot)
			{
				const uint8_t config = nvData.GetHomeSlotConfig(slot);
				if ((config & 0xF0) == 0x30 && (config & 0x0F) == buttonIndex) { ApplyHomeAuxSlot(slot); }
			}
		}
#endif
		if (filesNotMacros && text)
		{
			fileListPopupNoFiles->Show(false);
		}

		if (buttonIndex < ((filesNotMacros) ? NumDisplayedFiles : NumDisplayedMacros))
		{
			TextButton * const f = ((filesNotMacros) ? filenameButtons : macroButtons)[buttonIndex];
			f->SetText(text);
			f->SetEvent((text == nullptr) ? evNull : (filesNotMacros) ? evFile : evMacro, param);
			mgr.Show(f, text != nullptr);
		}
	}

	// Update the specified button in the macro short list. If 'fileName' is nullptr then hide the button, else display it.
	// Return true if this should be called again for the next button.
	bool UpdateMacroShortList(unsigned int buttonIndex, const char * _ecv_array null fileName)
	{
#if DISPLAY_X == 800
		// The Home page uses configurable dashboard shortcuts.
		// Keep the entire legacy layout branch out of this compilation target.
		UNUSED(buttonIndex); UNUSED(fileName);
		for (TextButton *& b : controlPageMacroButtons) { mgr.Show(b, false); }
		return false;
#else
	#if (DISPLAY_X == 480)
		const bool tooFewSpace = numToolColsUsed >= (MaxSlots - 1);
	#else
		const bool tooFewSpace = numToolColsUsed > (MaxSlots - 2);
	#endif

		if (buttonIndex >= ARRAY_SIZE(controlPageMacroButtons) || numToolColsUsed == 0 || tooFewSpace)
		{
			return false;
		}

		String<controlPageMacroTextLength>& str = controlPageMacroText[buttonIndex];
		str.Clear();
		const bool isFile = (fileName != nullptr);
		if (isFile)
		{
			str.copy(fileName);
		}
		TextButton * const f = controlPageMacroButtons[buttonIndex];
		f->SetText(SkipDigitsAndUnderscore(str.c_str()));
		f->SetEvent((isFile) ? evMacroControlPage : evNull, str.c_str());
		mgr.Show(f, isFile);
		return true;
#endif
	}

	unsigned int GetNumScrolledFiles(bool filesNotMacros)
	{
		return (filesNotMacros) ? NumFileRows : NumMacroRows;
	}

	void AdjustControlPageMacroButtons()
	{
#if DISPLAY_X == 800
		for (TextButton *& b : controlPageMacroButtons) { mgr.Show(b, false); }
		return;
#endif
		const unsigned int n = numToolColsUsed;

		if (n != numHeaterAndToolColumns)
		{
			numHeaterAndToolColumns = n;

			// Adjust the width of the control page macro buttons, or hide them completely if insufficient room
			PixelNumber controlPageMacroButtonsColumn = (PixelNumber)(((tempButtonWidth + fieldSpacing) * n) + bedColumn + fieldSpacing);
			PixelNumber controlPageMacroButtonsWidth = (PixelNumber)((controlPageMacroButtonsColumn >= DisplayX - margin) ? 0 : DisplayX - margin - controlPageMacroButtonsColumn);
			if (controlPageMacroButtonsWidth > maxControlPageMacroButtonsWidth)
			{
				controlPageMacroButtonsColumn += controlPageMacroButtonsWidth - maxControlPageMacroButtonsWidth;
				controlPageMacroButtonsWidth = maxControlPageMacroButtonsWidth;
			}

			bool showControlPageMacroButtons = controlPageMacroButtonsWidth >= minControlPageMacroButtonsWidth;

			for (TextButton *& b : controlPageMacroButtons)
			{
				if (showControlPageMacroButtons)
				{
					b->SetPositionAndWidth(controlPageMacroButtonsColumn, controlPageMacroButtonsWidth);
				}
				mgr.Show(b, showControlPageMacroButtons);
			}

			if (currentTab == tabControl)
			{
				mgr.Refresh(true);
			}
		}
	}

	void ResetToolAndHeaterStates() noexcept
	{
		for (size_t i = 0; i < numToolColsUsed; ++i)
		{
			toolButtons[i]->SetColours(colours->buttonTextColour, colours->buttonImageBackColour);
#if DISPLAY_X == 800
			currentTemps[i]->SetColours(colours->infoTextColour, colours->buttonTextBackColour);
			activeTemps[i]->SetColours(colours->buttonTextColour, colours->popupButtonBackColour);
			standbyTemps[i]->SetColours(colours->buttonTextColour, colours->popupButtonBackColour);
			if (printCurrentTemps[i] != nullptr)
			{
				printCurrentTemps[i]->SetColours(colours->infoTextColour, colours->buttonTextBackColour);
			}
#else
			currentTemps[i]->SetColours(colours->infoTextColour, colours->defaultBackColour);
			activeTemps[i]->SetColours(colours->buttonTextColour, colours->buttonTextBackColour);
			standbyTemps[i]->SetColours(colours->buttonTextColour, colours->buttonTextBackColour);
#endif
		}
	}

	void ManageCurrentActiveStandbyFields(
			size_t& slot,
			const bool showCurrent = false,
			const Event activeEvent = evNull,
			const int activeEventValue = -1,
			const Event standbyEvent = evNull,
			const int standbyEventValue = -1
			)
	{
#if DISPLAY_X == 800
		const bool dashboardVisible = slot < MaxSlots;
		const bool printVisible = slot < MaxPrintTempControls && printCurrentTemps[slot] != nullptr && printTempTitles[slot] != nullptr;
		mgr.Show(currentTemps[slot], dashboardVisible && showCurrent);
		if (printVisible)
		{
			mgr.Show(printCurrentTemps[slot], showCurrent);
			mgr.Show(printTempTitles[slot], showCurrent);
		}
		mgr.Show(activeTemps[slot], dashboardVisible && activeEvent != evNull);
		mgr.Show(standbyTemps[slot], dashboardVisible && standbyEvent != evNull);
#else
		mgr.Show(currentTemps[slot], showCurrent);
		mgr.Show(activeTemps[slot], activeEvent != evNull);
		mgr.Show(standbyTemps[slot], standbyEvent != evNull);
#endif

		activeTemps[slot]->SetEvent(activeEvent, activeEventValue);
		activeTemps[slot]->SetValue(0);
		standbyTemps[slot]->SetEvent(standbyEvent, standbyEventValue);
		standbyTemps[slot]->SetValue(0);
	}

	size_t AddBedOrChamber(OM::BedOrChamber *bedOrChamber, size_t &slot, const bool isBed = true) {
		const size_t count = (isBed ? OM::GetBedCount() : OM::GetChamberCount());
		bedOrChamber->slot = MaxSlots;
		if (slot < MaxSlots && bedOrChamber->heater > -1) {
			bedOrChamber->slot = slot;
#if DISPLAY_X == 800
			SetHomeToolButtonNormalGeometry(slot);
			mgr.Show(homeAuxValues[slot], false);
			mgr.Show(toolButtons[slot], true);
			if (printTempTitles[slot] != nullptr)
			{
				printTempTitles[slot]->SetShowIcon(true);
				printTempTitles[slot]->SetIcon(isBed ? VectorIcon::Bed : VectorIcon::Chamber);
				printTempTitles[slot]->SetIntVal((int)bedOrChamber->index);
				printTempTitles[slot]->SetPrintText(count > 1);
			}
#else
			mgr.Show(toolButtons[slot], true);
#endif
			ManageCurrentActiveStandbyFields(
					slot,
					true,
					isBed ? evAdjustBedActiveTemp : evAdjustChamberActiveTemp,
					bedOrChamber->index,
					isBed ? evAdjustBedStandbyTemp : evAdjustChamberStandbyTemp,
					bedOrChamber->index
					);
			mgr.Show(extrusionFactors[slot], false);
			toolButtons[slot]->SetEvent(isBed ? evSelectBed : evSelectChamber, bedOrChamber->index);
#if DISPLAY_X == 800
			toolButtons[slot]->SetIcon(isBed ? VectorIcon::Bed : VectorIcon::Chamber);
#else
			toolButtons[slot]->SetIcon(isBed ? IconBed : IconChamber);
#endif
			toolButtons[slot]->SetIntVal(bedOrChamber->index);
#if DISPLAY_X == 800
			toolButtons[slot]->SetPrintText(false);
#else
			toolButtons[slot]->SetPrintText(count > 1);
#endif

			++slot;
		}
		return count;
	}

	void AllToolsSeen()
	{
		size_t slot = 0;
		size_t bedCount = 0;
		size_t chamberCount = 0;
		auto firstBed = OM::GetFirstBed();
		if (firstBed != nullptr)
		{
			bedCount = AddBedOrChamber(firstBed, slot);
		}
		OM::IterateToolsWhile([&slot](OM::Tool*& tool, size_t)
		{
			tool->slot = slot;
			const bool hasHeater = tool->heaters[0] != nullptr;
			const bool hasSpindle = tool->spindle != nullptr;
			const bool hasExtruder = tool->extruders.IsNonEmpty();
			if (slot < MaxSlots)
			{
#if DISPLAY_X == 800
				SetHomeToolButtonNormalGeometry(slot);
				mgr.Show(homeAuxValues[slot], false);
				if (printTempTitles[slot] != nullptr)
				{
					printTempTitles[slot]->SetShowIcon(true);
					printTempTitles[slot]->SetIcon(ModernToolModeIcon(hasSpindle));
					printTempTitles[slot]->SetIntVal((int)tool->index);
					printTempTitles[slot]->SetPrintText(true);
				}
#endif
				toolButtons[slot]->SetEvent(evSelectHead, tool->index);
				toolButtons[slot]->SetIntVal(tool->index);
#if DISPLAY_X == 800
				// Show the tool index beside the nozzle/spindle icon so all
				// possible tools are identifiable on the compact heater cards.
				toolButtons[slot]->SetPrintText(true);
#else
				toolButtons[slot]->SetPrintText(true);
#endif
#if DISPLAY_X == 800
				toolButtons[slot]->SetIcon(ModernToolModeIcon(hasSpindle));
#else
				toolButtons[slot]->SetIcon(hasSpindle ? IconSpindle : IconNozzle);
#endif
#if DISPLAY_X == 800
				mgr.Show(toolButtons[slot], true);
#else
				mgr.Show(toolButtons[slot], true);
#endif

#if DISPLAY_X == 800
				mgr.Show(extrusionFactors[slot], hasExtruder);
#else
				mgr.Show(extrusionFactors[slot], hasExtruder);
#endif
				if (hasExtruder)
				{
					const int extruderIndex = (int)tool->extruders.LowestSetBit();
					extrusionFactors[slot]->SetEvent(extrusionFactors[slot]->GetEvent(), extruderIndex);
#if DISPLAY_X == 800
					extrusionFactors[slot]->SetLabel((extruderIndex >= 0 && extruderIndex < (int)MaxSlots) ? extrusionFactorLabels[extruderIndex] : "E ");
#endif
				}

				// Spindle takes precedence
				if (hasSpindle)
				{
					ManageCurrentActiveStandbyFields(slot, true, evAdjustActiveRPM, tool->spindle->index);
					++slot;
				}
				else if (hasHeater)
				{
					if (nvData.GetHeaterCombineType() == HeaterCombineType::notCombined)
					{
						tool->IterateHeaters([&slot, &tool](OM::ToolHeater*, size_t)
						{
							// only one heater per slot can be displayed
							if (slot >= MaxSlots)
							{
								return;
							}
							ManageCurrentActiveStandbyFields(
									slot,
									true,
									evAdjustToolActiveTemp, tool->index,
									evAdjustToolStandbyTemp, tool->index);
							++slot;
						});
					}
					else
					{
						ManageCurrentActiveStandbyFields(
								slot,
								true,
								evAdjustToolActiveTemp, tool->index,
								evAdjustToolStandbyTemp, tool->index);
						++slot;
					}
				}
				else
				{
					// Hides everything by default
					ManageCurrentActiveStandbyFields(slot);
					++slot;
				}
			}
			return slot < MaxSlots;
		});
		auto firstChamber = OM::GetFirstChamber();
		if (firstChamber != nullptr)
		{
			chamberCount = AddBedOrChamber(firstChamber, slot, false);
		}

		// Fill remaining space with additional beds
		if (slot < MaxSlots && bedCount > 1)
		{
			OM::IterateBedsWhile([&slot](OM::Bed*& bed, size_t) {
				AddBedOrChamber(bed, slot);
				return slot < MaxSlots;
			}, 1);
		}

		// Fill remaining space with additional chambers
		if (slot < MaxSlots && chamberCount > 1)
		{
			OM::IterateChambersWhile([&slot](OM::Chamber*& chamber, size_t) {
				AddBedOrChamber(chamber, slot, false);
				return slot < MaxSlots;
			}, 1);
		}

		numToolColsUsed = slot;
		for (size_t i = slot; i < MaxSlots; ++i)
		{
			mgr.Show(currentTemps[i], false);
#if DISPLAY_X == 800
			if (printCurrentTemps[i] != nullptr) { mgr.Show(printCurrentTemps[i], false); }
			if (printTempTitles[i] != nullptr) { mgr.Show(printTempTitles[i], false); }
#endif
			mgr.Show(activeTemps[i], false);
			mgr.Show(standbyTemps[i], false);
			mgr.Show(extrusionFactors[i], false);
#if DISPLAY_X == 800
			ApplyHomeAuxSlot(i);
#else
			mgr.Show(toolButtons[i], false);
#endif
		}
		ResetToolAndHeaterStates();
		AdjustControlPageMacroButtons();
	}

	void SetSpindleActive(size_t spindleIndex, int32_t activeRpm)
	{
		auto spindle = OM::GetOrCreateSpindle(spindleIndex);
		if (spindle == nullptr)
		{
			return;
		}
		spindle->active = abs(activeRpm);
		if (!GetFirmwareFeatures().IsBitSet(m568TempAndRPM))
		{
			if (activeRpm == 0)
			{
				spindle->state = OM::SpindleState::stopped;
			}
			else if (activeRpm > 0)
			{
				spindle->state = OM::SpindleState::forward;
			}
			else
			{
				spindle->state = OM::SpindleState::reverse;
			}
		}

		OM::IterateToolsWhile([spindle](OM::Tool*& tool, size_t) {
			if (tool->slot < MaxSlots && tool->spindle == spindle)
			{
				activeTemps[tool->slot]->SetValue(tool->spindle->active);
			}
			return tool->slot < MaxSlots;
		});
	}

	void UpdateSpindleCurrent(OM::Spindle* spindle)
	{
		OM::IterateToolsWhile([spindle](OM::Tool*& tool, size_t) {
			if (tool->slot < MaxSlots && tool->spindle == spindle)
			{
				const OM::SpindleState state = spindle->state;
				currentTemps[tool->slot]->SetValue(
						(state == OM::SpindleState::stopped)
							? 0
							: (state == OM::SpindleState::forward)
							  	  ? spindle->current
							  	  : -spindle->current);
			}
			return tool->slot < MaxSlots;
		});
	}

	void SetSpindleCurrent(size_t spindleIndex, int32_t current)
	{
		auto spindle = OM::GetOrCreateSpindle(spindleIndex);
		if (spindle == nullptr)
		{
			return;
		}
		spindle->current = abs(current);
		if (!GetFirmwareFeatures().IsBitSet(m568TempAndRPM))
		{
			if (current == 0)
			{
				spindle->state = OM::SpindleState::stopped;
			}
			else if (current > 0)
			{
				spindle->state = OM::SpindleState::forward;
			}
			else
			{
				spindle->state = OM::SpindleState::reverse;
			}
		}
		UpdateSpindleCurrent(spindle);
	}

	void SetSpindleLimit(size_t spindleIndex, uint32_t value, bool max)
	{
		OM::Spindle *spindle = OM::GetOrCreateSpindle(spindleIndex);
		if (spindle != nullptr)
		{
			if (max)
			{
				spindle->max = value;
			}
			else
			{
				spindle->min = value;
			}
		}
	}

	void SetSpindleState(size_t spindleIndex, OM::SpindleState state)
	{
		OM::Spindle* spindle = OM::GetOrCreateSpindle(spindleIndex);
		if (spindle == nullptr)
		{
			return;
		}
		const bool changed = spindle->state != state;
		spindle->state = state;
		if (changed)
		{
			UpdateSpindleCurrent(spindle);
		}
	}

	// This handles the old path where tools were assigned to spindles
	void SetSpindleTool(int8_t spindleNumber, int8_t toolIndex)
	{
		auto sp = OM::GetOrCreateSpindle(spindleNumber);
		if (sp == nullptr)
		{
			return;
		}
		if (toolIndex == -1)
		{
			OM::IterateToolsWhile([sp](OM::Tool*& tool, size_t) {
				if (tool->spindle == sp)
				{
					tool->spindle = nullptr;
				}
				return true;
			});
		}
		else
		{
			OM::Tool *tool = OM::GetOrCreateTool(toolIndex);
			if (tool != nullptr)
			{
				tool->spindle = sp;
			}
		}
	}

	void UpdateToolStatus(size_t toolIndex, OM::ToolStatus status)
	{
		auto tool = OM::GetTool(toolIndex);
		if (tool == nullptr)
		{
			return;
		}
		tool->status = status;
		Colour c = (status == OM::ToolStatus::standby) ? colours->standbyBackColour
					: (status == OM::ToolStatus::active) ? colours->activeBackColour
					: colours->buttonImageBackColour;
		if (tool->slot < MaxSlots)
		{
			toolButtons[tool->slot]->SetColours(colours->buttonTextColour, c);
		}
	}

	void SetToolExtruder(size_t toolIndex, uint8_t extruder)
	{
		OM::Tool *tool = OM::GetOrCreateTool(toolIndex);
		if (tool != nullptr)
		{
			tool->extruders.SetBit(extruder);
		}
	}

	void SetToolFan(size_t toolIndex, uint8_t fan)
	{
		OM::Tool *tool = OM::GetOrCreateTool(toolIndex);
		if (tool != nullptr)
		{
			tool->fans.SetBit(fan);
		}
	}

	bool RemoveToolHeaters(const size_t toolIndex, const uint8_t firstIndexToDelete)
	{
		OM::Tool *tool = OM::GetOrCreateTool(toolIndex);
		if (tool == nullptr)
		{
			return false;
		}
		return tool->RemoveHeatersFrom(firstIndexToDelete) > 0;
	}

	void SetToolHeater(size_t toolIndex, uint8_t toolHeaterIndex, uint8_t heaterIndex)
	{
		OM::Tool *tool = OM::GetOrCreateTool(toolIndex);
		if (tool == nullptr)
		{
			return;
		}
		OM::ToolHeater *toolHeater = tool->GetOrCreateHeater(toolHeaterIndex);
		if (toolHeater == nullptr)
		{
			return;
		}
		toolHeater->heaterIndex = heaterIndex;
	}

	void SetToolOffset(size_t toolIndex, size_t axisIndex, float offset)
	{
		if (axisIndex < MaxTotalAxes)
		{
			OM::Tool *tool = OM::GetOrCreateTool(toolIndex);
			if (tool != nullptr)
			{
				tool->offsets[axisIndex] = offset;
			}
		}
	}

	// This handles the new path were spindles are assigned to tools
	void SetToolSpindle(int8_t toolIndex, int8_t spindleNumber)
	{
		// Old spindles[].tool is handled by SetSpindleTool
		OM::Tool *tool = OM::GetOrCreateTool(toolIndex);
		if (tool == nullptr)
		{
			return;
		}
		if (spindleNumber == -1)
		{
			tool->spindle = nullptr;
		}
		else
		{
			OM::Spindle* spindle = OM::GetSpindle(spindleNumber);
			if (spindle == nullptr)
			{
				return;
			}
			tool->spindle = spindle;
		}
	}

	void SetBabystepOffset(size_t index, float f)
	{
		if (index < MaxTotalAxes)
		{
			OM::Axis *axis = OM::GetOrCreateAxis(index);
			if (axis == nullptr)
			{
				return;
			}
			axis->babystep = f;
			// In first initialization we will see babystep before letter
			// so this won;t be true hence it is also set in UpdateGeometry
			if (axis->letter[0] == 'Z')
			{
				babystepOffsetField->SetValue(f);
			}
		}
	}

	void SetAxisLetter(size_t index, char l)
	{
		if (index < MaxTotalAxes)
		{
			OM::Axis *axis = OM::GetOrCreateAxis(index);
			if (axis != nullptr)
			{
				axis->letter[0] = l;
			}
		}
	}

	void SetAxisVisible(size_t index, bool v)
	{
		if (index < MaxTotalAxes)
		{
			OM::Axis *axis = OM::GetOrCreateAxis(index);
			if (axis != nullptr)
			{
				axis->visible = v;
			}
		}
	}

	void SetAxisWorkplaceOffset(size_t axisIndex, size_t workplaceIndex, float offset)
	{
		if (axisIndex < MaxTotalAxes && workplaceIndex < OM::Workplaces::MaxTotalWorkplaces)
		{
			OM::Axis *axis = OM::GetOrCreateAxis(axisIndex);
			if (axis != nullptr)
			{
				axis->workplaceOffsets[workplaceIndex] = offset;
			}
		}
	}

	void SetCurrentWorkplaceNumber(uint8_t workplaceNumber)
	{
		if (currentWorkplaceNumber == workplaceNumber || workplaceNumber >= OM::Workplaces::MaxTotalWorkplaces)
		{
			return;
		}
		currentWorkplaceNumber = workplaceNumber;
	}

	void SetBedOrChamberHeater(const uint8_t heaterIndex, const int8_t heaterNumber, bool bed)
	{
		if (bed)
		{
			auto bed = OM::GetOrCreateBed(heaterIndex);
			if (bed != nullptr)
			{
				bed->heater = heaterNumber;
			}
		}
		else
		{
			auto chamber = OM::GetOrCreateChamber(heaterIndex);
			if (chamber != nullptr)
			{
				chamber->heater = heaterNumber;
			}
		}
	}
}

// End
