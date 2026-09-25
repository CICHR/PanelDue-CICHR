/*
 * UserInterfaceConstants.hpp
 *
 *  Created on: 10 Jan 2017
 *      Author: David
*/

#ifndef SRC_UI_USERINTERFACECONSTANTS_HPP_
#define SRC_UI_USERINTERFACECONSTANTS_HPP_

#include <UI/DisplaySize.hpp>
#include <Configuration.hpp>

static const char* _ecv_array const axisNames[] = { "X", "Y", "Z", "U", "V", "W" };
constexpr size_t MaxTotalAxes = 15;		// This needs to be kept in sync with the maximum in RRF for any build configuration
constexpr size_t MaxHeatersPerTool = 8;
static const char* _ecv_array const babystepAmounts[] = { "0.01", "0.02", "0.05", "0.1" };
static const float _ecv_array babystepAmountsF[] = { 0.01, 0.02, 0.05, 0.1 };
constexpr int8_t NoTool = -1;

#if DISPLAY_X == 480

const unsigned int MaxSlots = 5;
#define MaxDisplayableAxes	(4)

const PixelNumber margin = 2;
const PixelNumber textButtonMargin = 1;
const PixelNumber iconButtonMargin = 1;
const PixelNumber outlinePixels = 2;
const PixelNumber fieldSpacing = 6;
const PixelNumber mainContentLeft = 0;
const PixelNumber mainContentWidth = DISPLAY_X;
const PixelNumber statusFieldWidth = 156;
const PixelNumber bedColumn = 114;

const PixelNumber rowTextHeight = 21;	// height of the font we use
const PixelNumber rowHeight = 28;
const PixelNumber moveButtonRowSpacing = 12;
const PixelNumber extrudeButtonRowSpacing = 12;
const PixelNumber fileButtonRowSpacing = 8;
const PixelNumber keyboardButtonRowSpacing = 6;		// small enough to show 2 lines of messages

const PixelNumber speedTextWidth = 70;
const PixelNumber efactorTextWidth = 30;
const PixelNumber percentageWidth = 60;
const PixelNumber e1FactorXpos = 140, e2FactorXpos = 250;

const PixelNumber messageTimeWidth = 60;

const PixelNumber popupY = 192;
const PixelNumber popupSideMargin = 10;
const PixelNumber popupTopMargin = 10;
const PixelNumber keyboardTopMargin = 8;

const PixelNumber popupFieldSpacing = 10;

const PixelNumber axisLabelWidth = 26;
const PixelNumber firstMessageRow = margin + rowHeight + 3;		// adjust this to get a whole number of message rows below the keyboard

const PixelNumber progressBarHeight = 10;
const PixelNumber closeButtonWidth = 40;

const PixelNumber touchCalibMargin = 15;

const PixelNumber ColourGradientWidth = 2 * 128;

extern uint8_t glcd19x21[];				// declare which fonts we will be using
#define DEFAULT_FONT	glcd19x21

#elif DISPLAY_X == 800
const unsigned int MaxSlots = 12;
#define MaxDisplayableAxes (6)

const PixelNumber margin = 4;
const PixelNumber textButtonMargin = 1;
const PixelNumber iconButtonMargin = 2;
const PixelNumber outlinePixels = 1;
const PixelNumber fieldSpacing = 10;

// The left rail and top bar use the same geometry as the browser preview.
const PixelNumber navBarWidth = 86;
const PixelNumber mainContentLeft = navBarWidth;
const PixelNumber mainContentWidth = DISPLAY_X - mainContentLeft;
const PixelNumber navButtonLeft = 1;
const PixelNumber navButtonWidth = 84;
const PixelNumber navButtonHeight = 74;
const PixelNumber navButtonGap = 5;
const PixelNumber navButtonTop = 4;
const PixelNumber topBarHeight = 56;
const PixelNumber statusFieldWidth = 180;
const PixelNumber globalStopWidth = 164;
const PixelNumber globalStopLeft = DisplayX - statusFieldWidth - 14 - globalStopWidth - 8;
const PixelNumber bedColumn = mainContentLeft + 120;

const PixelNumber rowTextHeight = 21;	// Liberation Sans 19x21, much closer to preview typography
const PixelNumber rowHeight = 34;
const PixelNumber moveButtonRowSpacing = 20;
const PixelNumber extrudeButtonRowSpacing = 20;
const PixelNumber fileButtonRowSpacing = 12;
const PixelNumber keyboardButtonRowSpacing = 5;
// Console keyboard uses finger-sized keys without changing the
// legacy popup keyboard used by editable RRF dialogs.
const PixelNumber consoleKeyboardY = 244;
const PixelNumber consoleKeyHeight = 36;
const PixelNumber consoleKeyWidth = 46;
const PixelNumber consoleKeyHStep = 50;
const PixelNumber consoleKeyVStep = 41;

const PixelNumber speedTextWidth = 105;
const PixelNumber efactorTextWidth = 45;
const PixelNumber percentageWidth = 90;
const PixelNumber e1FactorXpos = 220, e2FactorXpos = 375;

const PixelNumber messageTimeWidth = 90;

const PixelNumber popupY = 345;
const PixelNumber popupSideMargin = 20;
const PixelNumber popupTopMargin = 20;
const PixelNumber keyboardTopMargin = 8;
const PixelNumber popupFieldSpacing = 20;

const PixelNumber axisLabelWidth = 40;
const PixelNumber firstMessageRow = topBarHeight + 14;

const PixelNumber progressBarHeight = 12;
const PixelNumber closeButtonWidth = 66;

const PixelNumber touchCalibMargin = 22;

const PixelNumber ColourGradientWidth = 3 * 128;

extern uint8_t glcd19x21[];
extern uint8_t glcd28x32[];
#define DEFAULT_FONT	glcd19x21
#define UI_LARGE_FONT	glcd28x32

#else

#error Unsupported DISPLAY_X value

#endif

const PixelNumber buttonHeight = rowTextHeight + 4;
const PixelNumber tempButtonWidth = (DISPLAY_X + fieldSpacing - bedColumn)/MaxSlots - fieldSpacing;

const PixelNumber row1 = 0;										// we don't need a top margin
const PixelNumber row2 = row1 + rowHeight - 2;					// the top row never has buttons so it can be shorter
const PixelNumber row3 = row2 + rowHeight;
const PixelNumber row4 = row3 + rowHeight;
const PixelNumber row5 = row4 + rowHeight;
const PixelNumber row6 = row5 + rowHeight;
const PixelNumber row6p3 = row6 + (rowHeight/3);
const PixelNumber row7 = row6 + rowHeight;
const PixelNumber row7p7 = row7 + ((2 * rowHeight)/3);
const PixelNumber row8 = row7 + rowHeight;
const PixelNumber row8p7 = row8 + ((2 * rowHeight)/3);
const PixelNumber row9 = row8 + rowHeight;
const PixelNumber rowTabs = DisplayY - rowTextHeight - 2;		// align bottom navigation exactly with the display edge
const PixelNumber labelRowAdjust = 2;							// how much to drop non-button fields to line up with buttons

const PixelNumber ColourGradientLeftPos = DISPLAY_X - ColourGradientWidth - margin;
const PixelNumber ColourGradientTopPos = row2;
const PixelNumber ColourGradientHeight = rowTextHeight;

const PixelNumber stateColumnWdith = mainContentWidth / 4;

const PixelNumber speedColumn = mainContentLeft + margin;
const PixelNumber fanColumn = mainContentLeft + margin + stateColumnWdith;
const PixelNumber babystepColumn = mainContentLeft + margin + stateColumnWdith * 2;
const PixelNumber cancelColumn = mainContentLeft + margin + stateColumnWdith * 2;
const PixelNumber resumeColumn = mainContentLeft + margin + stateColumnWdith * 3;
const PixelNumber pauseColumn = mainContentLeft + margin + stateColumnWdith * 3;

const PixelNumber fullPopupWidth = DisplayX - (2 * margin);
const PixelNumber fullPopupHeight = DisplayY - (2 * margin);
const PixelNumber popupBarHeight = buttonHeight + (2 * popupTopMargin);

const PixelNumber tempPopupBarWidth = (3 * fullPopupWidth)/4;
const PixelNumber rpmPopupBarWidth = fullPopupWidth;
const PixelNumber fileInfoPopupWidth = fullPopupWidth - (4 * margin),
				  fileInfoPopupHeight = (10 * rowTextHeight) + buttonHeight + (2 * popupTopMargin) + 3;
const PixelNumber areYouSurePopupWidth = DisplayX - 80,
				  areYouSurePopupHeight = (3 * rowHeight) + (2 * popupTopMargin);

#if DISPLAY_X == 800
// Motion is fullscreen and captures all touch input.
const PixelNumber movePopupWidth = DisplayX;
const PixelNumber movePopupHeight = DisplayY;
#else
const PixelNumber movePopupWidth = fullPopupWidth;
const PixelNumber movePopupHeight = fullPopupHeight;
#endif

const PixelNumber extrudePopupWidth = fullPopupWidth;
const PixelNumber extrudePopupHeight = (5 * buttonHeight) + (4 * extrudeButtonRowSpacing) + (2 * popupTopMargin);

const PixelNumber keyboardButtonWidth = DisplayX/5;
#if DISPLAY_X == 800
const PixelNumber keyboardPopupWidth = mainContentWidth;
#else
const PixelNumber keyboardPopupWidth = fullPopupWidth;
#endif
const PixelNumber keyButtonWidth = (keyboardPopupWidth - 2 * popupSideMargin)/16;
const PixelNumber keyButtonHStep = (keyboardPopupWidth - 2 * popupSideMargin - keyButtonWidth)/12;
const PixelNumber keyButtonVStep = buttonHeight + keyboardButtonRowSpacing;
const PixelNumber keyboardPopupHeight = (5 * keyButtonVStep) + (2 * keyboardTopMargin) + buttonHeight;
#if DISPLAY_X == 800
const PixelNumber keyboardPopupY = DisplayY - keyboardPopupHeight;
#else
const PixelNumber keyboardPopupY = margin;
#endif

const unsigned int NumFileColumns = 1;
const unsigned int NumFileRows = (fullPopupHeight - (2 * popupTopMargin) + fileButtonRowSpacing)/(buttonHeight + fileButtonRowSpacing) - 1;
const unsigned int NumDisplayedFiles = NumFileColumns * NumFileRows;

const PixelNumber fileListPopupWidth = fullPopupWidth;
const PixelNumber fileListPopupHeight = ((NumFileRows + 1) * buttonHeight) + (NumFileRows * fileButtonRowSpacing) + (2 * popupTopMargin);

const unsigned int NumMacroColumns = 2;
const unsigned int NumMacroRows = (fullPopupHeight - (2 * popupTopMargin) + fileButtonRowSpacing)/(buttonHeight + fileButtonRowSpacing) - 1;
const unsigned int NumDisplayedMacros = NumMacroColumns * NumMacroRows;

const PixelNumber MacroListPopupWidth = fullPopupWidth;
const PixelNumber MacroListPopupHeight = ((NumMacroRows + 1) * buttonHeight) + (NumMacroRows * fileButtonRowSpacing) + (2 * popupTopMargin);

#if DISPLAY_X == 800
const unsigned int numMessageRows = (consoleKeyboardY - firstMessageRow - 8)/rowTextHeight;
#else
const unsigned int numMessageRows = (rowTabs - margin - rowHeight)/rowTextHeight;
#endif
const PixelNumber messageTextX = mainContentLeft + margin + messageTimeWidth + 2;
const PixelNumber messageTextWidth = DisplayX - margin - messageTextX;

const unsigned int NumControlPageMacroButtons = 4;
const PixelNumber minControlPageMacroButtonsWidth = (tempButtonWidth * 3)/2;
const PixelNumber maxControlPageMacroButtonsWidth = DisplayX/2 - 2 * margin;

const PixelNumber alertPopupWidth = fullPopupWidth - 6 * margin;
const PixelNumber alertPopupHeight = 2 * popupTopMargin + 6 * rowTextHeight + 3 * buttonHeight + 2 * moveButtonRowSpacing;

const PixelNumber babystepPopupWidth = (2 * fullPopupWidth)/3;
const PixelNumber babystepPopupHeight = 3 * rowHeight + 2 * popupTopMargin;
const PixelNumber babystepRowSpacing = rowHeight;

#endif /* SRC_UI_USERINTERFACECONSTANTS_HPP_ */
