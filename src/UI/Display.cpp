/*
 * Display.cpp
 *
 * Created: 04/11/2014 09:42:47
 *  Author: David
*/

#include <UI/Display.hpp>
#include "Icons/Icons.hpp"
#include "General/SimpleMath.h"

#undef min
#undef max
#undef array
#undef result
#include <algorithm>

#define DEBUG 0
#include "Debug.hpp"

extern UTFT lcd;

const int maxXerror = 8, maxYerror = 8;		// how close (in pixels) the X and Y coordinates of a touch event need to be to the outline of the button for us to allow it


namespace
{
void FillRoundedBox(int x1, int y1, int x2, int y2, int radius, Colour colour)
{
	if (x2 < x1 || y2 < y1)
	{
		return;
	}
	const int maxRadiusX = (x2 - x1) / 2;
	const int maxRadiusY = (y2 - y1) / 2;
	if (radius > maxRadiusX) radius = maxRadiusX;
	if (radius > maxRadiusY) radius = maxRadiusY;
	if (radius < 2)
	{
		lcd.setColor(colour);
		lcd.fillRect(x1, y1, x2, y2);
		return;
	}
	lcd.setColor(colour);
	lcd.fillRect(x1 + radius, y1, x2 - radius, y2);
	lcd.fillRect(x1, y1 + radius, x2, y2 - radius);
	lcd.fillCircle(x1 + radius, y1 + radius, radius);
	lcd.fillCircle(x2 - radius, y1 + radius, radius);
	lcd.fillCircle(x1 + radius, y2 - radius, radius);
	lcd.fillCircle(x2 - radius, y2 - radius, radius);
}

void DrawModernBox(int x1, int y1, int x2, int y2, int radius, Colour fill, Colour border)
{
	FillRoundedBox(x1, y1, x2, y2, radius, border);
	if (x2 - x1 > 2 && y2 - y1 > 2)
	{
		FillRoundedBox(x1 + 1, y1 + 1, x2 - 1, y2 - 1, radius > 2 ? radius - 1 : radius, fill);
	}
}
}

// Static fields of class DisplayField
LcdFont DisplayField::defaultFont = nullptr;
Colour DisplayField::defaultFcolour = white;
Colour DisplayField::defaultBcolour = black;
Colour DisplayField::defaultButtonBorderColour = black;
Colour DisplayField::defaultGradColour = 0;
Colour DisplayField::defaultPressedBackColour = black;
Colour DisplayField::defaultPressedGradColour = 0;
Palette DisplayField::defaultIconPalette = IconPaletteLight;

DisplayField::DisplayField(PixelNumber py, PixelNumber px, PixelNumber pw)
	: y(py), x(px), width(pw), fcolour(defaultFcolour), bcolour(defaultBcolour),
		changed(true), visible(true), underlined(false), border(false), textRows(1), next(nullptr)
{
}

void DisplayField::SetTextRows(const char * _ecv_array null t)
{
	unsigned int rows = 1;
	if (t != nullptr)
	{
		while (*t != 0)
		{
			if (*t == '\n')
			{
				++rows;
			}
			++t;
		}
	}
	textRows = rows;
}

void DisplayField::SetPositionAndWidth(PixelNumber newX, PixelNumber newWidth)
{
	if (x == newX && width == newWidth)
	{
		return;
	}
	x = newX;
	width = newWidth;
	changed = true;
}

void DisplayField::SetPosition(PixelNumber x, PixelNumber y)
{
	if (this->x == x && this->y == y)
	{
		return;
	}
	this->x = x;
	this->y = y;
	changed = true;
}

ModernPanelField::ModernPanelField(PixelNumber py, PixelNumber px, PixelNumber pw, PixelNumber ph,
		Colour fill, Colour border, uint8_t pradius)
	: DisplayField(py, px, pw), height(ph), borderColour(border), radius(pradius)
{
	bcolour = fill;
}

void ModernPanelField::Refresh(bool full, PixelNumber xOffset, PixelNumber yOffset)
{
	if (full || changed)
	{
		const int x1 = (int)x + (int)xOffset;
		const int y1 = (int)y + (int)yOffset;
		const int x2 = x1 + (int)width - 1;
		const int y2 = y1 + (int)height - 1;
#if DISPLAY_X == 800
		DrawModernBox(x1, y1, x2, y2, radius, bcolour, borderColour);
#else
		lcd.setColor(bcolour);
		lcd.fillRoundRect(x1, y1, x2, y2);
		lcd.setColor(borderColour);
		lcd.drawRoundRect(x1, y1, x2, y2);
#endif
		changed = false;
	}
}

/*static*/ void DisplayField::SetDefaultColours(Colour pf, Colour pb, Colour pbb, Colour pg, Colour pbp, Colour pgp, Palette pal)
{
	defaultFcolour = pf;
	defaultBcolour = pb;
	defaultButtonBorderColour = pbb;
	defaultGradColour = pg;
	defaultPressedBackColour = pbp;
	defaultPressedGradColour = pgp;
	defaultIconPalette = pal;
}

/*static*/ PixelNumber DisplayField::GetTextWidth(const char* _ecv_array s, PixelNumber maxWidth)
{
	lcd.setFont(DisplayField::defaultFont);
	lcd.setTextPos(0, 9999, maxWidth);
	lcd.printf("%s", s);						// dummy print to get text width
	return lcd.getTextX();
}

/*static*/ PixelNumber DisplayField::GetTextWidth(const char* _ecv_array s, PixelNumber maxWidth, size_t maxChars)
{
	lcd.setTextPos(0, 9999, maxWidth);
	lcd.printf("%.*s", maxChars, s);				// dummy print to get text width
	return lcd.getTextX();
}

void DisplayField::Show(bool v)
{
	if (visible != v)
	{
		visible = changed = v;
	}
}

// Find the best match to a touch event in a list of fields
ButtonPress DisplayField::FindEvent(PixelNumber x, PixelNumber y, DisplayField * null p)
{
	int bestError = maxXerror + maxYerror;
	ButtonPress best;
	while (p != nullptr)
	{
		p->CheckEvent(x, y, bestError, best);
		p = p->next;
	}
	return best;
}

void DisplayField::SetColours(Colour pf, Colour pb)
{
	if (fcolour != pf || bcolour != pb)
	{
		fcolour = pf;
		bcolour = pb;
		changed = true;
	}
}

// ButtonPress class methods
ButtonPress::ButtonPress() : button(nullptr), index(0) { }

ButtonPress::ButtonPress(ButtonBase *b, unsigned int pi) : button(b), index(pi) { }

void ButtonPress::Set(ButtonBase *b, unsigned int pi)
{
	button = b;
	index = pi;
}

void ButtonPress::Clear()
{
	button = nullptr;
	index = 0;
}

event_t ButtonPress::GetEvent() const
{
	return button->GetEvent();
}

int ButtonPress::GetIParam() const
{
	return button->GetIParam(index);
}

const char* _ecv_array ButtonPress::GetSParam() const
{
	return button->GetSParam(index);
}

bool ButtonPress::operator==(const ButtonPress& other) const { return button == other.button && index == other.index; }

// Window class methods
Window::Window(Colour pb)
	: root(nullptr), next(nullptr), backgroundColour(pb)
{
}

// Prepend a field to the linked list of displayed fields
void Window::AddField(DisplayField *d)
{
	d->parent = this;
	d->next = root;
	root = d;
}

bool Window::ObscuredByPopup(const DisplayField *p) const
{
	return next != nullptr
			&& (  (   p->GetMaxY() >= next->Ypos() && p->GetMinY() < next->Ypos() + next->GetHeight()
				   && p->GetMaxX() >= next->Xpos() && p->GetMinX() < next->Xpos() + next->GetWidth()
				  )
				|| next->ObscuredByPopup(p)
			   );
}

bool Window::Visible(const DisplayField *p) const
{
	return p->IsVisible() && !ObscuredByPopup(p);
}

// Get the field that has been touched, or nullptr if we can't find one
ButtonPress Window::FindEvent(PixelNumber x, PixelNumber y)
{
	return (next != nullptr) ? next->FindEvent(x, y)
			: (x < Xpos() || y < Ypos()) ? ButtonPress()
				: DisplayField::FindEvent(x - Xpos(), y - Ypos(), root);
}

// Get the field that has been touched, but search only outside the popup
ButtonPress Window::FindEventOutsidePopup(PixelNumber x, PixelNumber y)
{
	if (next == nullptr) return ButtonPress();

	ButtonPress f = DisplayField::FindEvent(x, y, root);
	return (f.IsValid() && Visible(f.GetButton())) ? f : ButtonPress();
}

void Window::SetPopup(PopupWindow * p, PixelNumber px, PixelNumber py, bool redraw, const PixelNumber displayX, const PixelNumber displayY)
{
	if (px == AutoPlace)
	{
		px = (displayX - p->GetWidth())/2;
	}
	if (py == AutoPlace)
	{
		py = (displayY - p->GetHeight())/2;
	}
	p->SetPos(px, py);
	Window *pw = this;
	while (pw->next != nullptr)
	{
		if (pw->next == p)
		{
			if (redraw)
			{
				p->Refresh(true);
			}
			return;				// popup is already displayed
		}
		pw = pw->next;
	}
	p->next = nullptr;			// ensure no nested popup
	pw->next = p;
	if (redraw)
	{
		p->Refresh(true);
	}
}

void Window::ClearPopup(bool redraw, PopupWindow *whichOne)
{
	if (next != nullptr)
	{
		// Find the penultimate window
		Window *pw = this;
		while (pw->next->next != nullptr)
		{
			pw = pw->next;
		}

		if (whichOne == nullptr || whichOne == pw->next)
		{
			const PixelNumber xmin = pw->next->Xpos(), xmax = xmin + pw->next->GetWidth() - 1, ymin = pw->next->Ypos(), ymax = ymin + pw->next->GetHeight() - 1;
			bool popupWasContained = pw->Contains(xmin, ymin, xmax, ymax);
			if (popupWasContained)
			{
				// Clear the area that was occupied by the last window to the background colour of the penultimate window
				lcd.setColor(pw->backgroundColour);
				lcd.fillRoundRect(xmin, ymin, xmax, ymax);
			}

			// Detach the last window
			pw->next = nullptr;

			if (redraw)
			{
				if (popupWasContained)
				{
					// Re-display the fields of the penultimate window that were obscured
					for (DisplayField * null pp = pw->root; pp != nullptr; pp = pp->next)
					{
						if (pp->IsVisible())
						{
							pp->Refresh(true, pw->Xpos(), pw->Ypos());
						}
					}
				}
				else
				{
					Refresh(true);		// redraw everything
				}
			}
		}
	}
}

bool Window::IsPopupActive(const PopupWindow *popup)
{
	for (PopupWindow *pw = next; pw; pw = pw->next)
	{
		if (pw == popup)
		{
			return true;
		}
	}
	return false;
}


// Redraw the specified field
void Window::Redraw(DisplayField *f)
{
	for (DisplayField * null p = root; p != nullptr; p = p->next)
	{
		if (p == f)
		{
			// The field belongs to this window
			if (!ObscuredByPopup(p))
			{
				if (p->IsVisible())
				{
					p->Refresh(true, Xpos(), Ypos());
				}
				else
				{
					lcd.setColor(backgroundColour);
					lcd.fillRect(p->GetMinX() + Xpos(), p->GetMinY() + Ypos(), p->GetMaxX() + Xpos(), p->GetMaxY() + Ypos());
				}
			}
			return;
		}
	}

	// Else we didn't find the field in our window, so look in nested windows
	if (next != nullptr)
	{
		next->Redraw(f);
	}
}

void Window::Show(DisplayField * null f, bool v)
{
	if (f != nullptr && (f->IsVisible() != v || f->HasChanged()))
	{
		f->Show(v);

		// Check whether the field is currently in the display list, if so then show or hide it
		for (DisplayField *p = root; p != nullptr; p = p->next)
		{
			if (p == f)
			{
				if (ObscuredByPopup(f))
				{
					// nothing to do
				}
				else if (v)
				{
					f->Refresh(true, Xpos(), Ypos());
				}
				else
				{
					lcd.setColor(backgroundColour);
					lcd.fillRect(f->GetMinX(), f->GetMinY(), f->GetMaxX(), f->GetMaxY());
				}
				return;
			}
		}

		// Else we didn't find it, so maybe it is in a popup field
		if (next != nullptr)
		{
			next->Redraw(f);
		}
	}
}

// Show the button as pressed or not
void Window::Press(ButtonPress bp, bool v)
{
	if (bp.IsValid())
	{
		bp.GetButton()->Press(v, bp.GetIndex());
		if (bp.GetButton()->IsVisible())		// need to check this in case we are releasing the button and it has gone invisible since we pressed it
		{
			Redraw(bp.GetButton());
		}
	}
}

MainWindow::MainWindow() : Window(black), staticLeftMargin(0)
{
}

void MainWindow::Init(Colour bc)
{
	backgroundColour = bc;
}

// Refresh all fields. If 'full' is true then we rewrite them all, else we just rewrite those that have changed.
void MainWindow::Refresh(bool full)
{
	if (full)
	{
		lcd.fillScr(backgroundColour, staticLeftMargin);
	}

	for (DisplayField * null pp = root; pp != nullptr; pp = pp->next)
	{
		if (Visible(pp))
		{
			pp->Refresh(full, 0, 0);
		}
	}
	if (next != nullptr)
	{
		next->Refresh(full);
	}
}

bool MainWindow::Contains(PixelNumber xmin, PixelNumber ymin, PixelNumber xmax, PixelNumber ymax) const
{
	UNUSED(xmin); UNUSED(ymin); UNUSED(xmax); UNUSED(ymax);
	return true;
}

void MainWindow::ClearAllPopups()
{
	while (next != nullptr)
	{
		ClearPopup(true);
	}
}

PopupWindow::PopupWindow(PixelNumber ph, PixelNumber pw, Colour pb, Colour pBorder, bool roundCorners)
	: Window(pb), height(ph), width(pw), borderColour(pBorder), roundedCorners(roundCorners)
{
}

void PopupWindow::Refresh(bool full)
{
	if (full)
	{
		// Draw a rectangle inside the border
		lcd.setColor(backgroundColour);
		if (roundedCorners)
		{
			lcd.fillRoundRect(xPos + 1, yPos + 2, xPos + width - 2, yPos + height - 3);
		}
		else
		{
			lcd.fillRect(xPos, yPos, xPos + width, yPos + height);
		}

		// Flat single-pixel popup border.
		lcd.setColor(borderColour);
		if (roundedCorners)
		{
			lcd.drawRoundRect(xPos, yPos, xPos + width - 1, yPos + height - 1);
		}
		else
		{
			lcd.drawRect(xPos, yPos, xPos + width - 1, yPos + height - 1);
		}
	}

	for (DisplayField * null p = root; p != nullptr; p = p->next)
	{
		if (p->IsVisible() && (full || !ObscuredByPopup(p)))
		{
			p->Refresh(full, xPos, yPos);
		}
	}

	if (next != nullptr)
	{
		next->Refresh(full);
	}
}

bool PopupWindow::Contains(PixelNumber xmin, PixelNumber ymin, PixelNumber xmax, PixelNumber ymax) const
{
	return xPos + 2 <= xmin && yPos + 2 <= ymin && xPos + width >= xmax + 3 && yPos + height >= ymax + 3;
}

void ColourGradientField::Refresh(bool full, PixelNumber xOffset, PixelNumber yOffset)
{
	if (full)
	{
		PixelNumber px = x + xOffset;
		const PixelNumber py = y + yOffset;
		const PixelNumber lineRepeat = width/128;
		for (PixelNumber i = 0; i < 32; ++i)
		{
			lcd.setColor(i << 11);
			for (PixelNumber j = 0; j < lineRepeat; ++j)
			{
				lcd.drawLine(px, py, px, py + height - 1);
				++px;
			}
		}
		for (PixelNumber i = 0; i < 64; ++i)
		{
			lcd.setColor(i << 5);
			for (PixelNumber j = 0; j < lineRepeat; ++j)
			{
				lcd.drawLine(px, py, px, py + height - 1);
				++px;
			}
		}
		for (PixelNumber i = 0; i < 32; ++i)
		{
			lcd.setColor(i);
			for (PixelNumber j = 0; j < lineRepeat; ++j)
			{
				lcd.drawLine(px, py, px, py + height - 1);
				++px;
			}
		}
	}
}

PixelNumber FieldWithText::GetHeight() const
{
	PixelNumber height = UTFT::GetFontHeight(font) * textRows;
	height += (textRows - 1) * 2;		// 2px space between lines
	if (underlined)
	{
		height += 2;					// one space and the underline
	}
	if (border)
	{
		height += 4;					// one space abd border top and bottom
	}
	return height;
}

void FieldWithText::Refresh(bool full, PixelNumber xOffset, PixelNumber yOffset)
{
	if (full || changed)
	{
		xOffset += x;
		yOffset += y;
		PixelNumber textWidth = width;
		if (border)
		{
			if (full)
			{
				lcd.setColor(fcolour);
				lcd.drawRect(xOffset, yOffset, xOffset + width - 1, yOffset + GetHeight() - 1);
			}
			xOffset += 2;
			yOffset += 2;
			textWidth -= 4;
		}

		lcd.setFont(font);
		lcd.setColor(fcolour);
		lcd.setBackColor(bcolour);

		// Do a dummy print to get the text width. Needed for underlining and for centre- or right-aligned text.
		lcd.setTextPos(0, 9999, textWidth);
		PrintText();
		const PixelNumber actualWidth = lcd.getTextX();
		const PixelNumber underlineY = yOffset + UTFT::GetFontHeight(font) + 1;
		if (underlined)
		{
			// Remove previous underlining
			lcd.setColor(bcolour);
			lcd.drawLine(xOffset, underlineY, xOffset + textWidth - 1, underlineY);
			lcd.setColor(fcolour);
		}

		lcd.setTextPos(xOffset, yOffset, xOffset + textWidth);
		if (align == TextAlignment::Left)
		{
			PrintText();
			lcd.clearToMargin();
			if (underlined)
			{
				lcd.drawLine(xOffset, underlineY, xOffset + actualWidth, underlineY);
			}
		}
		else
		{
			lcd.clearToMargin();
			PixelNumber spare = textWidth - actualWidth;
			if (align == TextAlignment::Centre)
			{
				const PixelNumber textX = xOffset + spare/2;
				lcd.setTextPos(textX, yOffset, xOffset + textWidth);
				PrintText();
				if (underlined)
				{
					lcd.drawLine(textX, underlineY, textX + actualWidth - 1, underlineY);
				}
			}
			else
			{
				// Must be right aligned. Try to add a right margin of up to 3 pixels for better appearance.
				if (spare <= 3)
				{
					spare = 0;
				}
				else
				{
					spare -= 3;
				}
				const PixelNumber textX = xOffset + spare;
				lcd.setTextPos(textX, yOffset, xOffset + textWidth);
				PrintText();
				if (underlined)
				{
					lcd.drawLine(textX, underlineY, textX + actualWidth, underlineY);
				}
			}
		}
		changed = false;
	}
}

void TextField::PrintText() const
{
	if (label != nullptr)
	{
		lcd.printf("%s", label);
	}
	if (text != nullptr)
	{
		lcd.printf("%s", text);
	}
}

void FloatField::PrintText() const
{
	if (label != nullptr)
	{
		lcd.printf("%s", label);
	}
	lcd.printf("%.*f", numDecimals, static_cast<double>(val));
	if (units != nullptr)
	{
		lcd.printf("%s", units);
	}
}

void IntegerField::PrintText() const
{
	if (label != nullptr)
	{
		lcd.printf("%s", label);
	}
	lcd.printf("%d", val);
	if (units != nullptr)
	{
		lcd.printf("%s", units);
	}
}

void StaticTextField::PrintText() const
{
	if (text != nullptr)
	{
		lcd.printf("%s", text);
	}
}

ButtonBase::ButtonBase(PixelNumber py, PixelNumber px, PixelNumber pw)
	: DisplayField(py, px, pw),
	  borderColour(defaultButtonBorderColour), gradColour(defaultGradColour),
	  pressedBackColour(defaultPressedBackColour), pressedGradColour(defaultPressedGradColour), evt(nullEvent), pressed(false)
{
}

PixelNumber ButtonBase::textMargin = 1;
PixelNumber ButtonBase::iconMargin = 1;

void ButtonBase::DrawOutline(PixelNumber xOffset, PixelNumber yOffset, bool isPressed) const
{
	// Modern flat button: no bevel/gradient, just a solid rounded surface and a
	// restrained one-pixel outline. Pressed state is indicated by the accent fill.
	const PixelNumber x1 = x + xOffset;
	const PixelNumber y1 = y + yOffset;
	const PixelNumber x2 = x1 + width - 1;
	const PixelNumber y2 = y1 + GetHeight() - 1;

#if DISPLAY_X == 800
	const int h = (int)GetHeight();
	const int w = (int)width;
	int radius = 12;
	if (h < 38) radius = 8;
	if (w < 48) radius = 7;
	DrawModernBox((int)x1, (int)y1, (int)x2, (int)y2, radius,
			isPressed ? pressedBackColour : bcolour, borderColour);
#else
	lcd.setColor(isPressed ? pressedBackColour : bcolour);
	lcd.fillRoundRect(x1, y1, x2, y2);
	lcd.setColor(borderColour);
	lcd.drawRoundRect(x1, y1, x2, y2);
#endif
}

void ButtonBase::CheckEvent(PixelNumber x, PixelNumber y, int& bestError, ButtonPress& best) /*override*/
{
	if (IsVisible() && GetEvent() != nullEvent)
	{
		const int xError = (x < GetMinX()) ? GetMinX() - x
								: (x > GetMaxX()) ? x - GetMaxX()
									: 0;
		if (xError < maxXerror)
		{
			const int yError = (y < GetMinY()) ? GetMinY() - y
									: (y > GetMaxY()) ? y - GetMaxY()
										: 0;
			if (yError < maxYerror && xError + yError < bestError)
			{
				bestError = xError + yError;
				best.Set(this, 0);
			}
		}
	}
}

SingleButton::SingleButton(PixelNumber py, PixelNumber px, PixelNumber pw)
	: ButtonBase(py, px, pw)
{
	param.sParam = nullptr;
}

void SingleButton::DrawOutline(PixelNumber xOffset, PixelNumber yOffset) const
{
	ButtonBase::DrawOutline(xOffset, yOffset, pressed);
}

void SingleButton::Press(bool p, int index) /*override*/
{
	UNUSED(index);
	if (p != pressed)
	{
		pressed = p;
		changed = true;
	}
}

namespace
{
void DrawThickLine(int x1, int y1, int x2, int y2, unsigned int thickness = 2)
{
	lcd.drawLine(x1, y1, x2, y2);
	if (thickness > 1)
	{
		if (std::abs(x2 - x1) >= std::abs(y2 - y1))
		{
			lcd.drawLine(x1, y1 + 1, x2, y2 + 1);
		}
		else
		{
			lcd.drawLine(x1 + 1, y1, x2 + 1, y2);
		}
	}
}

void DrawChevron(int cx, int cy, int r, bool up, bool left)
{
	if (up)
	{
		DrawThickLine(cx - r, cy + r/2, cx, cy - r/2);
		DrawThickLine(cx, cy - r/2, cx + r, cy + r/2);
	}
	else if (left)
	{
		DrawThickLine(cx + r/2, cy - r, cx - r/2, cy);
		DrawThickLine(cx - r/2, cy, cx + r/2, cy + r);
	}
	else
	{
		// Down chevron.
		DrawThickLine(cx - r, cy - r/2, cx, cy + r/2);
		DrawThickLine(cx, cy + r/2, cx + r, cy - r/2);
	}
}

void DrawVectorGlyph(VectorIcon icon, int cx, int cy, int size, Colour colour)
{
	lcd.setColor(colour);
	const int r = max(6, size/2);
	const int q = max(3, r/2);
	switch (icon)
	{
	case VectorIcon::Logo:
		// PanelDue CICHR compact brand mark used in the persistent sidebar.
		lcd.drawRoundRect(cx - r + 2, cy - r + 1, cx + r - 2, cy + r - 1);
		DrawThickLine(cx - q, cy - q, cx + q, cy - q);
		DrawThickLine(cx - q, cy, cx + q, cy);
		DrawThickLine(cx - q, cy + q, cx + q, cy + q);
		break;
	case VectorIcon::Home:
	case VectorIcon::HomeAll:
		DrawThickLine(cx - r, cy, cx, cy - r);
		DrawThickLine(cx, cy - r, cx + r, cy);
		DrawThickLine(cx - r + 2, cy - 1, cx - r + 2, cy + r);
		DrawThickLine(cx + r - 2, cy - 1, cx + r - 2, cy + r);
		DrawThickLine(cx - r + 2, cy + r, cx + r - 2, cy + r);
		DrawThickLine(cx - q/2, cy + r, cx - q/2, cy + q/2);
		DrawThickLine(cx + q/2, cy + r, cx + q/2, cy + q/2);
		break;
	case VectorIcon::Print:
	case VectorIcon::Nozzle:
		// Neutral nozzle/print glyph.
		lcd.drawRect(cx - r, cy - r, cx + r, cy - q);
		DrawThickLine(cx - q, cy - q + 2, cx - q/2, cy + q);
		DrawThickLine(cx + q, cy - q + 2, cx + q/2, cy + q);
		DrawThickLine(cx - q/2, cy + q, cx + q/2, cy + q);
		DrawThickLine(cx, cy + q, cx, cy + r);
		lcd.fillCircle(cx, cy + r, 2);
		break;
	case VectorIcon::Extrude:
	{
		// Nozzle with three external material-flow chevrons pointing DOWN.
		// The chevrons sit to the right of the nozzle so the nozzle stays legible
		// even on the compact Motion action buttons.
		const int nr = max(5, r - 5);
		const int nx = cx - 5;
		lcd.drawRect(nx - nr, cy - nr, nx + nr, cy - max(3, nr/2));
		const int nq = max(3, nr/2);
		DrawThickLine(nx - nq, cy - nq + 2, nx - nq/2, cy + nq);
		DrawThickLine(nx + nq, cy - nq + 2, nx + nq/2, cy + nq);
		DrawThickLine(nx - nq/2, cy + nq, nx + nq/2, cy + nq);
		DrawThickLine(nx, cy + nq, nx, cy + nr);
		const int ax = cx + r - 2;
		for (int dy = -8; dy <= 8; dy += 8)
		{
			DrawChevron(ax, cy + dy, 3, false, false);
		}
		break;
	}
	case VectorIcon::Console:
		lcd.drawRoundRect(cx - r, cy - r + 1, cx + r, cy + r - 1);
		DrawThickLine(cx - r + 5, cy - q, cx - q, cy);
		DrawThickLine(cx - q, cy, cx - r + 5, cy + q);
		DrawThickLine(cx, cy + q, cx + r - 5, cy + q);
		break;
	case VectorIcon::Settings:
	{
		// Spur-gear silhouette.
		// Two outer points per tooth give each of the eight teeth a short flat crown.
		static const int8_t gearX[32] = {
			0,20,38,43,55,83,92,77,78,98,92,65,55,56,38,15,
			0,-20,-38,-43,-55,-83,-92,-77,-78,-98,-92,-65,-55,-56,-38,-15
		};
		static const int8_t gearY[32] = {
			-78,-98,-92,-65,-55,-56,-38,-15,0,20,38,43,55,83,92,77,
			78,98,92,65,55,56,38,15,0,-20,-38,-43,-55,-83,-92,-77
		};
		for (unsigned int i = 0; i < 32; ++i)
		{
			const unsigned int j = (i + 1) & 31u;
			DrawThickLine(cx + (r * gearX[i])/100, cy + (r * gearY[i])/100,
					cx + (r * gearX[j])/100, cy + (r * gearY[j])/100);
		}
		lcd.drawCircle(cx, cy, max(3, q/2 + 1));
		break;
	}
	case VectorIcon::Files:
		DrawThickLine(cx - r, cy - q, cx - q, cy - q);
		DrawThickLine(cx - q, cy - q, cx - q + 4, cy - r + 2);
		DrawThickLine(cx - q + 4, cy - r + 2, cx + 1, cy - r + 2);
		DrawThickLine(cx + 1, cy - r + 2, cx + 5, cy - q);
		DrawThickLine(cx + 5, cy - q, cx + r, cy - q);
		DrawThickLine(cx - r, cy - q, cx - r, cy + r);
		DrawThickLine(cx - r, cy + r, cx + r, cy + r);
		DrawThickLine(cx + r, cy + r, cx + r, cy - q);
		break;
	case VectorIcon::Move:
		DrawThickLine(cx - r, cy, cx + r, cy);
		DrawThickLine(cx, cy - r, cx, cy + r);
		DrawThickLine(cx - r, cy, cx - r + 5, cy - 4);
		DrawThickLine(cx - r, cy, cx - r + 5, cy + 4);
		DrawThickLine(cx + r, cy, cx + r - 5, cy - 4);
		DrawThickLine(cx + r, cy, cx + r - 5, cy + 4);
		DrawThickLine(cx, cy - r, cx - 4, cy - r + 5);
		DrawThickLine(cx, cy - r, cx + 4, cy - r + 5);
		DrawThickLine(cx, cy + r, cx - 4, cy + r - 5);
		DrawThickLine(cx, cy + r, cx + 4, cy + r - 5);
		break;
	case VectorIcon::Retract:
	{
		// Same nozzle with three external chevrons pointing UP.
		const int nr = max(5, r - 5);
		const int nx = cx - 5;
		lcd.drawRect(nx - nr, cy - nr, nx + nr, cy - max(3, nr/2));
		const int nq = max(3, nr/2);
		DrawThickLine(nx - nq, cy - nq + 2, nx - nq/2, cy + nq);
		DrawThickLine(nx + nq, cy - nq + 2, nx + nq/2, cy + nq);
		DrawThickLine(nx - nq/2, cy + nq, nx + nq/2, cy + nq);
		DrawThickLine(nx, cy + nq, nx, cy + nr);
		const int ax = cx + r - 2;
		for (int dy = -8; dy <= 8; dy += 8)
		{
			DrawChevron(ax, cy + dy, 3, true, false);
		}
		break;
	}
	case VectorIcon::Macro:
		lcd.drawRoundRect(cx - r, cy - r, cx + r, cy + r);
		DrawThickLine(cx - q, cy - q, cx + q, cy);
		DrawThickLine(cx + q, cy, cx - q, cy + q);
		DrawThickLine(cx - q, cy + q, cx - q, cy - q);
		break;
	case VectorIcon::BedMesh:
	case VectorIcon::Bed:
		lcd.drawRoundRect(cx - r, cy - r + 3, cx + r, cy + r - 3);
		for (int i = -1; i <= 1; ++i)
		{
			lcd.drawLine(cx - r + 3, cy + i*q/2, cx + r - 3, cy + i*q/2);
			lcd.drawLine(cx + i*q, cy - r + 5, cx + i*q, cy + r - 5);
		}
		if (icon == VectorIcon::BedMesh)
		{
			lcd.fillCircle(cx + q, cy - q/2, 2);
			lcd.fillCircle(cx - q, cy + q/2, 2);
		}
		break;
	case VectorIcon::Chamber:
		lcd.drawRoundRect(cx - r, cy - r, cx + r, cy + r);
		lcd.drawCircle(cx, cy, q);
		DrawThickLine(cx, cy - r + 3, cx, cy - q);
		break;
	case VectorIcon::Spindle:
		lcd.drawCircle(cx, cy, r - 2);
		DrawThickLine(cx, cy - r + 4, cx + q, cy + q);
		DrawThickLine(cx + q, cy + q, cx - q, cy + q);
		DrawThickLine(cx - q, cy + q, cx, cy - r + 4);
		break;
	case VectorIcon::Cnc:
	{
		// Bright-line CNC spindle/end-mill icon. Kept visually related to the nozzle glyph.
		const int w = max(5, r - 5);
		lcd.drawRoundRect(cx - w, cy - r, cx + w, cy - q/2);
		DrawThickLine(cx - w + 2, cy - q/2, cx - q/2, cy + q/2);
		DrawThickLine(cx + w - 2, cy - q/2, cx + q/2, cy + q/2);
		DrawThickLine(cx - q/2, cy + q/2, cx + q/2, cy + q/2);
		DrawThickLine(cx, cy + q/2, cx, cy + r - 1);
		DrawThickLine(cx - 3, cy + r - 6, cx + 3, cy + r - 3);
		DrawThickLine(cx + 3, cy + r - 3, cx - 3, cy + r);
		break;
	}
	case VectorIcon::Laser:
	{
		// Bright-line laser head with a narrow beam and a small impact sparkle.
		const int w = max(5, r - 5);
		lcd.drawRoundRect(cx - w, cy - r, cx + w, cy - q/2);
		DrawThickLine(cx - w + 2, cy - q/2, cx - q/2, cy + q/2);
		DrawThickLine(cx + w - 2, cy - q/2, cx + q/2, cy + q/2);
		DrawThickLine(cx - q/2, cy + q/2, cx + q/2, cy + q/2);
		const int sy = cy + r - 5;
		DrawThickLine(cx, cy + q/2, cx, sy - 2);
		DrawThickLine(cx - 5, sy, cx + 5, sy);
		DrawThickLine(cx, sy - 5, cx, sy + 5);
		DrawThickLine(cx - 4, sy - 4, cx + 4, sy + 4);
		DrawThickLine(cx + 4, sy - 4, cx - 4, sy + 4);
		break;
	}
	case VectorIcon::Fan:
		// Four-blade fan glyph with a compact hub.
		lcd.drawCircle(cx, cy, r - 2);
		lcd.fillCircle(cx, cy, 2);
		DrawThickLine(cx + 2, cy - 2, cx + q, cy - r + 4);
		DrawThickLine(cx + 2, cy + 2, cx + r - 4, cy + q);
		DrawThickLine(cx - 2, cy + 2, cx - q, cy + r - 4);
		DrawThickLine(cx - 2, cy - 2, cx - r + 4, cy - q);
		break;
	case VectorIcon::Output:
		// Generic GPIO/output glyph: rounded port with a lightning mark.
		lcd.drawRoundRect(cx - r, cy - r + 2, cx + r, cy + r - 2);
		DrawThickLine(cx + 2, cy - r + 5, cx - q, cy + 1);
		DrawThickLine(cx - q, cy + 1, cx + 1, cy + 1);
		DrawThickLine(cx + 1, cy + 1, cx - 2, cy + r - 5);
		DrawThickLine(cx - 2, cy + r - 5, cx + q, cy - 1);
		break;
	case VectorIcon::Plus:
		DrawThickLine(cx - r + 3, cy, cx + r - 3, cy);
		DrawThickLine(cx, cy - r + 3, cx, cy + r - 3);
		break;
	case VectorIcon::ArrowUp:
		DrawChevron(cx, cy, r - 2, true, false);
		break;
	case VectorIcon::ArrowDown:
		DrawChevron(cx, cy, r - 2, false, false);
		break;
	case VectorIcon::ArrowLeft:
		DrawChevron(cx, cy, r - 2, false, true);
		break;
	case VectorIcon::ArrowRight:
		DrawThickLine(cx - r/2, cy - r + 2, cx + r/2, cy);
		DrawThickLine(cx + r/2, cy, cx - r/2, cy + r - 2);
		break;
	case VectorIcon::Stop:
		// Minimal stop glyph: rounded-square outline with a solid inner mark.
		lcd.drawRoundRect(cx - r, cy - r, cx + r, cy + r);
		lcd.fillRect(cx - q, cy - q, cx + q, cy + q);
		break;
	case VectorIcon::Pause:
		lcd.fillRect(cx - q - 3, cy - r + 2, cx - 3, cy + r - 2);
		lcd.fillRect(cx + 3, cy - r + 2, cx + q + 3, cy + r - 2);
		break;
	case VectorIcon::Play:
		// Open triangular play glyph; thick strokes remain crisp in RGB565.
		DrawThickLine(cx - q, cy - r + 2, cx + r - 2, cy);
		DrawThickLine(cx + r - 2, cy, cx - q, cy + r - 2);
		DrawThickLine(cx - q, cy + r - 2, cx - q, cy - r + 2);
		break;
	case VectorIcon::Close:
		DrawThickLine(cx - r + 3, cy - r + 3, cx + r - 3, cy + r - 3);
		DrawThickLine(cx + r - 3, cy - r + 3, cx - r + 3, cy + r - 3);
		break;
	}
}
}

VectorIconButton::VectorIconButton(PixelNumber py, PixelNumber px, PixelNumber pw, PixelNumber ph, VectorIcon pi,
					 event_t e, const char * _ecv_array null pt, LcdFont pf, int param)
	: SingleButton(py, px, pw), icon(pi), text(pt), font(pf), height(ph), val(0), printText(pt != nullptr), showIcon(true), useIntValue(false)
{
	SetEvent(e, param);
}

VectorIconButton::VectorIconButton(PixelNumber py, PixelNumber px, PixelNumber pw, PixelNumber ph, VectorIcon pi,
					 event_t e, int textVal, LcdFont pf, int param)
	: SingleButton(py, px, pw), icon(pi), text(nullptr), font(pf), height(ph), val(textVal), printText(true), showIcon(true), useIntValue(true)
{
	SetEvent(e, param);
}

VectorIconButton::VectorIconButton(PixelNumber py, PixelNumber px, PixelNumber pw, PixelNumber ph, VectorIcon pi,
					 event_t e, const char * _ecv_array null pt, LcdFont pf, const char * _ecv_array param)
	: SingleButton(py, px, pw), icon(pi), text(pt), font(pf), height(ph), val(0), printText(pt != nullptr), showIcon(true), useIntValue(false)
{
	SetEvent(e, param);
}

void VectorIconButton::SetText(const char * _ecv_array null newText)
{
	if (text == newText || (text != nullptr && newText != nullptr && strcmp(text, newText) == 0))
	{
		return;
	}
	text = newText;
	useIntValue = false;
	changed = true;
}

void VectorIconButton::Refresh(bool full, PixelNumber xOffset, PixelNumber yOffset)
{
	if (!(full || changed))
	{
		return;
	}
	DrawOutline(xOffset, yOffset);

	const bool hasText = printText && (text != nullptr || useIntValue);
	const PixelNumber textHeight = hasText ? UTFT::GetFontHeight(font) : 0;
	const int left = (int)xOffset + (int)x;
	const int top = (int)yOffset + (int)y;
	const int w = (int)width;
	const int h = (int)height;

	// Use signed layout arithmetic because PixelNumber is uint16_t.
	if (showIcon && hasText && h < (int)textHeight + 34)
	{
		// Compact horizontal layout for short controls (heater/tool and home rows).
		const int iconBox = min(w / 3, h - 10);
		int iconSize = max(16, min(34, iconBox));
		if (icon == VectorIcon::ArrowUp || icon == VectorIcon::ArrowDown ||
			icon == VectorIcon::ArrowLeft || icon == VectorIcon::ArrowRight)
		{
			iconSize = min(iconSize, 24);
		}
		DrawVectorGlyph(icon, left + max(14, w / 4), top + h / 2, iconSize, fcolour);
	}
	else if (showIcon)
	{
		// Tall cards use a vertical icon + label layout. Icon-only navigation gets
		// the whole button, producing a much larger modern sidebar glyph.
		const int textReserve = hasText ? ((int)textHeight + 12) : 0;
		const int iconAreaHeight = max(18, h - textReserve);
		int iconCy = top + iconAreaHeight / 2;
		int iconSize = max(18, min(58, min(w - 16, iconAreaHeight - 10)));
		if (hasText && (icon == VectorIcon::Extrude || icon == VectorIcon::Retract))
		{
			// Motion extrusion actions get a larger but still strictly bounded glyph.
			// Keep a clear 4px gap above the label to prevent colour/font bleeding.
			const int iconBottom = h - (int)textHeight - 10;
			iconCy = top + max(12, iconBottom/2);
			iconSize = min(30, max(24, iconBottom - 2));
		}
		DrawVectorGlyph(icon, left + w / 2, iconCy, iconSize, fcolour);
	}

	if (hasText)
	{
		lcd.setTransparentBackground(true);
		lcd.setColor(fcolour);
		lcd.setFont(font);
		lcd.setTextPos(0, 9999, width > 6 ? width - 6 : width);
		if (useIntValue) lcd.printf("%d", val); else lcd.printf("%s", text);
		const int tw = (int)lcd.getTextX();

		int tx;
		int ty;
		int rightMargin;
		if (showIcon && h < (int)textHeight + 34)
		{
			// Compact horizontal layout: reserve the left ~42% for the glyph.
			const int textLeft = left + (w * 42) / 100;
			const int textW = max(8, left + w - 4 - textLeft);
			tx = textLeft + max(0, (textW - tw) / 2);
			ty = top + max(0, (h - (int)textHeight) / 2);
			rightMargin = left + w - 3;
		}
		else
		{
			tx = left + max(2, (w - tw) / 2);
			ty = showIcon ? (top + h - (int)textHeight - 6)
						 : (top + max(0, (h - (int)textHeight) / 2));
			rightMargin = left + w - 3;
		}
		lcd.setTextPos((uint16_t)max(0, tx), (uint16_t)max(0, ty), (uint16_t)max(0, rightMargin));
		if (useIntValue) lcd.printf("%d", val); else lcd.printf("%s", text);
		lcd.setTransparentBackground(false);
	}
	changed = false;
}

BedMapField::BedMapField(PixelNumber py, PixelNumber px, PixelNumber pw, PixelNumber ph, event_t e, Colour grid, Colour dot)
	: ButtonBase(py, px, pw), height(ph), minX(0.0f), maxX(300.0f), minY(0.0f), maxY(300.0f),
	  posX(0.0f), posY(0.0f), circular(false), gridColour(grid), dotColour(dot)
{
	evt = e;
}

void BedMapField::SetBounds(float pminX, float pmaxX, float pminY, float pmaxY)
{
	if (pmaxX <= pminX + 0.01f || pmaxY <= pminY + 0.01f)
	{
		return;
	}
	if (minX != pminX || maxX != pmaxX || minY != pminY || maxY != pmaxY)
	{
		minX = pminX; maxX = pmaxX; minY = pminY; maxY = pmaxY; changed = true;
	}
}

void BedMapField::SetFrame(PixelNumber px, PixelNumber py, PixelNumber pw, PixelNumber ph, bool isCircular)
{
	if (x != px || y != py || width != pw || height != ph || circular != isCircular)
	{
		x = px;
		y = py;
		width = pw;
		height = ph;
		circular = isCircular;
		changed = true;
	}
}

void BedMapField::SetPosition(float px, float py)
{
	if (fabsf(px - posX) > 0.01f || fabsf(py - posY) > 0.01f)
	{
		posX = px; posY = py; changed = true;
	}
}

void BedMapField::CheckEvent(PixelNumber tx, PixelNumber ty, int& bestError, ButtonPress& best)
{
	if (!visible || evt == nullEvent || tx < x || tx > GetMaxX() || ty < y || ty > GetMaxY())
	{
		return;
	}
	if (circular)
	{
		const int cx = (int)x + (int)width/2;
		const int cy = (int)y + (int)height/2;
		const int dx = (int)tx - cx;
		const int dy = (int)ty - cy;
		const int r = min((int)width, (int)height)/2;
		if (dx*dx + dy*dy > r*r) { return; }
	}
	bestError = 0;
	const unsigned int lx = min<unsigned int>(65535u, (unsigned int)(tx - x));
	const unsigned int ly = min<unsigned int>(65535u, (unsigned int)(ty - y));
	best.Set(this, (lx << 16) | ly);
}

bool BedMapField::TouchToMachine(unsigned int packedTouch, float& px, float& py) const
{
	const unsigned int lx = packedTouch >> 16;
	const unsigned int ly = packedTouch & 0xFFFFu;
	if (width < 8 || height < 8 || maxX <= minX || maxY <= minY)
	{
		return false;
	}
	const float nx = constrain<float>((float)lx / (float)(width - 1), 0.0f, 1.0f);
	const float ny = constrain<float>((float)ly / (float)(height - 1), 0.0f, 1.0f);
	if (circular)
	{
		const float dx = nx - 0.5f;
		const float dy = ny - 0.5f;
		if (dx*dx + dy*dy > 0.25f) { return false; }
	}
	px = minX + nx * (maxX - minX);
	py = maxY - ny * (maxY - minY); // screen Y grows down; machine Y grows up
	return true;
}

void BedMapField::Refresh(bool full, PixelNumber xOffset, PixelNumber yOffset)
{
	if (!(full || changed))
	{
		return;
	}
	const int x1 = xOffset + x;
	const int y1 = yOffset + y;
	const int x2 = x1 + width - 1;
	const int y2 = y1 + height - 1;
	lcd.setColor(bcolour);
	if (circular)
	{
		const int cx = (x1 + x2)/2;
		const int cy = (y1 + y2)/2;
		const int r = min(x2 - x1, y2 - y1)/2;
		lcd.fillCircle(cx, cy, r);
		lcd.setColor(borderColour);
		lcd.drawCircle(cx, cy, r);
		lcd.setColor(gridColour);
		lcd.drawLine(cx - r + 5, cy, cx + r - 5, cy);
		lcd.drawLine(cx, cy - r + 5, cx, cy + r - 5);
		const int r1 = r/3, r2 = (2*r)/3;
		if (r1 > 2) lcd.drawCircle(cx, cy, r1);
		if (r2 > r1) lcd.drawCircle(cx, cy, r2);
	}
	else
	{
		lcd.fillRoundRect(x1, y1, x2, y2);
		lcd.setColor(borderColour);
		lcd.drawRoundRect(x1, y1, x2, y2);
		lcd.setColor(gridColour);
		for (int i = 1; i < 5; ++i)
		{
			const int gx = x1 + (i * (width - 1))/5;
			const int gy = y1 + (i * (height - 1))/5;
			lcd.drawLine(gx, y1 + 6, gx, y2 - 6);
			lcd.drawLine(x1 + 6, gy, x2 - 6, gy);
		}
	}
	const float nx = constrain<float>((posX - minX)/(maxX - minX), 0.0f, 1.0f);
	const float ny = constrain<float>((posY - minY)/(maxY - minY), 0.0f, 1.0f);
	const int dotX = x1 + (int)(nx * (width - 1));
	const int dotY = y2 - (int)(ny * (height - 1));
	lcd.setColor(dotColour);
	lcd.fillCircle(dotX, dotY, 6);
	lcd.setColor(white);
	lcd.fillCircle(dotX, dotY, 2);
	changed = false;
}

/*static*/ LcdFont ButtonWithText::font;

PixelNumber ButtonWithText::GetHeight() const
{
	PixelNumber ret = (UTFT::GetFontHeight(font) + 2) * textRows - 2;	// height of the text
	ret += 2 * textMargin + 2;											// add the border height
	return customHeight != 0 ? customHeight : ret;
}

void ButtonWithText::Refresh(bool full, PixelNumber xOffset, PixelNumber yOffset)
{
	if (full || changed)
	{
		DrawOutline(xOffset, yOffset);
		lcd.setTransparentBackground(true);
		lcd.setColor(fcolour);
		lcd.setFont(font);
		unsigned int rowsLeft = textRows;
		size_t offset = 0;
		const PixelNumber naturalHeight = ((UTFT::GetFontHeight(font) + 2) * textRows - 2) + 2 * textMargin + 2;
		const PixelNumber extraY = (GetHeight() > naturalHeight) ? (GetHeight() - naturalHeight)/2 : 0;
		PixelNumber rowY = y + yOffset + textMargin + 1 + extraY;
		do
		{
			lcd.setTextPos(0, 9999, width - 6);
			PrintText(offset);							// dummy print to get text width
			PixelNumber spare = width - 6 - lcd.getTextX();
			lcd.setTextPos(x + xOffset + 3 + spare/2, rowY, x + xOffset + width - 3);	// text is always centre-aligned
			offset += PrintText(offset) + 1;
			rowY += UTFT::GetFontHeight(font) + 2;
		} while (--rowsLeft != 0);
		lcd.setTransparentBackground(false);
		changed = false;
	}
}

CharButton::CharButton(PixelNumber py, PixelNumber px, PixelNumber pw, char pc, event_t e)
	: ButtonWithText(py, px, pw)
{
	SetEvent(e, (int)pc);
}

size_t CharButton::PrintText(size_t offset) const
{
	UNUSED(offset);
	return lcd.write((char)GetIParam(0));
}

TextButton::TextButton(PixelNumber py, PixelNumber px, PixelNumber pw, const char * _ecv_array null pt, event_t e, int param)
	: ButtonWithText(py, px, pw), text(pt)
{
	SetTextRows(pt);
	SetEvent(e, param);
}

TextButton::TextButton(PixelNumber py, PixelNumber px, PixelNumber pw, const char * _ecv_array null pt, event_t e, const char * _ecv_array param)
	: ButtonWithText(py, px, pw), text(pt)
{
	SetEvent(e, param);
}

size_t TextButton::PrintText(size_t offset) const
{
	if (text != nullptr)
	{
		return lcd.printf("%s", text + offset);
	}
	return 0;
}

TextButtonWithLabel::TextButtonWithLabel(PixelNumber py, PixelNumber px, PixelNumber pw, const char * _ecv_array null pt, event_t e, int param, const char* _ecv_array null label)
	: TextButton(py - 2, px, pw, pt, e, param), label(label)
{
}

TextButtonWithLabel::TextButtonWithLabel(PixelNumber py, PixelNumber px, PixelNumber pw, const char * _ecv_array null pt, event_t e, const char * _ecv_array param, const char* _ecv_array null label)
	: TextButton(py - 2, px, pw, pt, e, param), label(label)
{
}

size_t TextButtonWithLabel::PrintText(size_t offset) const
{
	size_t w = 0;
	if (label != nullptr)
	{
		w += lcd.printf("%s", label);
	}
	w += TextButton::PrintText(offset);
	return w;
}

IconButton::IconButton(PixelNumber py, PixelNumber px, PixelNumber pw, Icon ic, event_t e, int param)
	: SingleButton(py, px, pw), icon(ic)
{
	SetEvent(e, param);
}

IconButton::IconButton(PixelNumber py, PixelNumber px, PixelNumber pw, Icon ic, event_t e, const char * _ecv_array param)
: SingleButton(py, px, pw), icon(ic)
{
	SetEvent(e, param);
}

void IconButton::Refresh(bool full, PixelNumber xOffset, PixelNumber yOffset)
{
	if (full || changed)
	{
		DrawOutline(xOffset, yOffset);
		const uint16_t sx = GetIconWidth(icon), sy = GetIconHeight(icon);
		lcd.setTransparentBackground(true);
		const PixelNumber iconY = yOffset + y + ((GetHeight() > sy) ? (GetHeight() - sy)/2 : 0);
		lcd.drawBitmap4(xOffset + x + (width - sx)/2, iconY, sx, sy, GetIconData(icon), defaultIconPalette);
		lcd.setTransparentBackground(false);
		changed = false;
	}
}

IconButtonWithText::IconButtonWithText(PixelNumber py, PixelNumber px, PixelNumber pw, Icon ic, event_t e, const char * text, int param)
	: IconButton(py, px, pw, ic, e, param), font(DisplayField::defaultFont), text(text), val(0), printText(true), drawIcon(true)
{
}

IconButtonWithText::IconButtonWithText(PixelNumber py, PixelNumber px, PixelNumber pw, Icon ic, event_t e, const char * text, const char * _ecv_array param)
	: IconButton(py, px, pw, ic, e, param), font(DisplayField::defaultFont), text(text), val(0), printText(true), drawIcon(true)
{
}

IconButtonWithText::IconButtonWithText(PixelNumber py, PixelNumber px, PixelNumber pw, Icon ic, event_t e, int textVal, int param)
	: IconButton(py, px, pw, ic, e, param), font(DisplayField::defaultFont), text(nullptr), val(textVal), printText(true), drawIcon(true)
{
}

IconButtonWithText::IconButtonWithText(PixelNumber py, PixelNumber px, PixelNumber pw, Icon ic, event_t e, int textVal, const char * _ecv_array param)
	: IconButton(py, px, pw, ic, e, param), font(DisplayField::defaultFont), text(nullptr), val(textVal), printText(true), drawIcon(true)
{
}

size_t IconButtonWithText::PrintText() const
{
	size_t ret = 0;
	if (!printText)
	{
		return ret;
	}
	if (text != nullptr)
	{
		ret += lcd.printf("%s", text);
	}
	else {
		ret += lcd.printf("%d", val);
	}
	return ret;
}

void IconButtonWithText::Refresh(bool full, PixelNumber xOffset, PixelNumber yOffset)
{
	if (full || changed)
	{
		DrawOutline(xOffset, yOffset);
		const uint16_t	sx = GetIconWidth(icon),
						sy = drawIcon ? GetIconHeight(icon) : 0;

		lcd.setFont(font);
		lcd.setTextPos(0, 9999, width - 6);
		PrintText();							// dummy print to get text width
		const PixelNumber textWidth = lcd.getTextX() + 6;	// add three pixels on each side

		// Print the icon
		lcd.setTransparentBackground(true);
		const PixelNumber iconXOffset = xOffset + x + (width - (sx+textWidth))/2;
		if (drawIcon)
		{
			lcd.drawBitmap4(iconXOffset, yOffset + y + iconMargin + 1, sx, sy, GetIconData(icon), defaultIconPalette);
		}

		// Print the text
		const PixelNumber textX = iconXOffset + sx + 3;
		const PixelNumber rowY = y + yOffset + textMargin + 1;
		lcd.setTextPos(textX, rowY, textX + textWidth);
		lcd.setColor(fcolour);
		PrintText();
		lcd.setTransparentBackground(false);

		changed = false;
	}
}

size_t IntegerButton::PrintText(size_t offset) const
{
	UNUSED(offset);
	size_t ret = 0;
	if (label != nullptr)
	{
		ret += lcd.printf("%s", label);
	}
	ret += lcd.printf("%d", val);
	if (units != nullptr)
	{
		ret += lcd.printf("%s", units);
	}
	return ret;
}

void CompactPercentButton::Refresh(bool full, PixelNumber xOffset, PixelNumber yOffset)
{
	if (!(full || changed))
	{
		return;
	}
	DrawOutline(xOffset, yOffset);
	lcd.setTransparentBackground(true);
	lcd.setColor(fcolour);
	lcd.setFont(font);

	const PixelNumber left = xOffset + x;
	const PixelNumber top = yOffset + y + 2;
	const PixelNumber right = left + width - 1;

	if (useIcon)
	{
		DrawVectorGlyph(icon, left + 14, top + (UTFT::GetFontHeight(font) / 2), 14, fcolour);
	}
	else if (prefix != nullptr)
	{
		lcd.setTextPos(left + 2, top, right - 2);
		lcd.printf("%s", prefix);
	}

	lcd.setTextPos(0, 9999, width);
	lcd.printf("%d%%", val);
	const PixelNumber valueWidth = lcd.getTextX();
	const PixelNumber valueX = (valueWidth + 2 < width) ? right - valueWidth - 1 : left + 2;
	lcd.setTextPos(valueX, top, right - 1);
	lcd.printf("%d%%", val);
	lcd.setTransparentBackground(false);
	changed = false;
}


HomeSlotPickerField::HomeSlotPickerField(PixelNumber py, PixelNumber px, PixelNumber pw, PixelNumber ph, event_t e)
	: ButtonBase(py, px, pw), height(ph), selected(0), fanMask(0), outputMask(0), macroMask(0)
{
	for (size_t i = 0; i < 8; ++i)
	{
		fanLabels[i] = nullptr;
		outputLabels[i] = nullptr;
		macroLabels[i] = nullptr;
	}
	evt = e;
}

unsigned int HomeSlotPickerField::CountBits(uint8_t mask) const
{
	unsigned int count = 0;
	while (mask != 0)
	{
		count += (mask & 1u);
		mask >>= 1;
	}
	return count;
}

uint8_t HomeSlotPickerField::ConfigForChoice(unsigned int choice) const
{
	if (choice == 0) { return 0; }
	unsigned int ordinal = choice - 1;
	for (unsigned int i = 0; i < 8; ++i)
	{
		if ((fanMask & (uint8_t)(1u << i)) != 0)
		{
			if (ordinal == 0) { return (uint8_t)(0x10 + i); }
			--ordinal;
		}
	}
	for (unsigned int i = 0; i < 8; ++i)
	{
		if ((outputMask & (uint8_t)(1u << i)) != 0)
		{
			if (ordinal == 0) { return (uint8_t)(0x20 + i); }
			--ordinal;
		}
	}
	for (unsigned int i = 0; i < 8; ++i)
	{
		if ((macroMask & (uint8_t)(1u << i)) != 0)
		{
			if (ordinal == 0) { return (uint8_t)(0x30 + i); }
			--ordinal;
		}
	}
	return 0;
}

int HomeSlotPickerField::GetIParam(unsigned int index) const
{
	return ConfigForChoice(index);
}

void HomeSlotPickerField::CheckEvent(PixelNumber tx, PixelNumber ty, int& bestError, ButtonPress& best)
{
	if (!visible || evt == nullEvent || tx < x || tx > GetMaxX() || ty < y || ty > GetMaxY())
	{
		return;
	}

	const PixelNumber localX = tx - x;
	const PixelNumber localY = ty - y;
	const PixelNumber buttonHeight = 30;
	const PixelNumber rowStep = 34;
	const PixelNumber gap = 6;
	const PixelNumber autoY = 0;
	const PixelNumber fanY = 64;
	const PixelNumber outputY = 158;
	const PixelNumber macroY = 252;

	if (localY >= autoY && localY < autoY + buttonHeight)
	{
		bestError = 0;
		best.Set(this, 0);
		return;
	}

	const uint8_t masks[3] = { fanMask, outputMask, macroMask };
	const PixelNumber starts[3] = { fanY, outputY, macroY };
	unsigned int choiceBase = 1;

	for (unsigned int group = 0; group < 3; ++group)
	{
		const unsigned int count = CountBits(masks[group]);
		unsigned int ordinal = 0;
		for (unsigned int index = 0; index < 8; ++index)
		{
			if ((masks[group] & (uint8_t)(1u << index)) == 0)
			{
				continue;
			}

			const unsigned int row = ordinal / 4;
			const unsigned int rowStart = row * 4;
			const unsigned int rowCount = min<unsigned int>(4, count - rowStart);
			const unsigned int column = ordinal - rowStart;
			const PixelNumber usableWidth = width - (rowCount - 1) * gap;
			const PixelNumber baseWidth = usableWidth / rowCount;
			const PixelNumber bx = column * (baseWidth + gap);
			const PixelNumber bw = (column + 1 == rowCount) ? width - bx : baseWidth;
			const PixelNumber by = starts[group] + row * rowStep;

			if (localX >= bx && localX < bx + bw && localY >= by && localY < by + buttonHeight)
			{
				bestError = 0;
				best.Set(this, choiceBase + ordinal);
				return;
			}
			++ordinal;
		}
		choiceBase += count;
	}
}

void HomeSlotPickerField::Refresh(bool full, PixelNumber xOffset, PixelNumber yOffset)
{
	if (!(full || changed)) { return; }
	static const char* const defaultFanLabels[8] = { "F0", "F1", "F2", "F3", "F4", "F5", "F6", "F7" };
	static const char* const defaultOutputLabels[8] = { "Out0", "Out1", "Out2", "Out3", "Out4", "Out5", "Out6", "Out7" };
	const PixelNumber buttonHeight = 30;
	const PixelNumber rowStep = 34;
	const PixelNumber gap = 6;
	const PixelNumber left = xOffset + x;
	const PixelNumber top = yOffset + y;
	const PixelNumber autoY = 0;
	const PixelNumber fanTitleY = 40;
	const PixelNumber fanY = 64;
	const PixelNumber outputTitleY = 134;
	const PixelNumber outputY = 158;
	const PixelNumber macroTitleY = 228;
	const PixelNumber macroY = 252;
	const unsigned int fanCount = CountBits(fanMask);
	const unsigned int outputCount = CountBits(outputMask);
	const unsigned int macroCount = CountBits(macroMask);

	lcd.setFont(defaultFont);
	lcd.setTransparentBackground(true);

	const PixelNumber autoX1 = left + 2;
	const PixelNumber autoY1 = top + autoY + 1;
	const PixelNumber autoX2 = left + width - 3;
	const PixelNumber autoY2 = autoY1 + buttonHeight - 3;
	lcd.setColor(selected == 0 ? pressedBackColour : bcolour);
	lcd.fillRoundRect(autoX1, autoY1, autoX2, autoY2);
	lcd.setColor(borderColour);
	lcd.drawRoundRect(autoX1, autoY1, autoX2, autoY2);
	lcd.setColor(fcolour);
	lcd.setTextPos(0, 9999, width - 8);
	lcd.printf("AUTO / EMPTY");
	const PixelNumber autoTextWidth = lcd.getTextX();
	lcd.setTextPos(autoX1 + max<int>(2, ((int)width - (int)autoTextWidth) / 2), autoY1 + 4, autoX2 - 2);
	lcd.printf("AUTO / EMPTY");

	lcd.setColor(fcolour);
	lcd.setTextPos(left + 2, top + fanTitleY, left + width - 2);
	lcd.printf("FANS");
	lcd.setTextPos(left + 2, top + outputTitleY, left + width - 2);
	lcd.printf("OUTS");
	lcd.setTextPos(left + 2, top + macroTitleY, left + width - 2);
	lcd.printf("MACROS");

	const uint8_t masks[3] = { fanMask, outputMask, macroMask };
	const unsigned int counts[3] = { fanCount, outputCount, macroCount };
	const PixelNumber starts[3] = { fanY, outputY, macroY };
	const char* const* labels[3] = { fanLabels, outputLabels, macroLabels };
	const char* const* defaults[3] = { defaultFanLabels, defaultOutputLabels, nullptr };

	for (unsigned int group = 0; group < 3; ++group)
	{
		unsigned int ordinal = 0;
		for (unsigned int index = 0; index < 8; ++index)
		{
			if ((masks[group] & (uint8_t)(1u << index)) == 0)
			{
				continue;
			}

			const unsigned int row = ordinal / 4;
			const unsigned int rowStart = row * 4;
			const unsigned int rowCount = min<unsigned int>(4, counts[group] - rowStart);
			const unsigned int column = ordinal - rowStart;
			const PixelNumber usableWidth = width - (rowCount - 1) * gap;
			const PixelNumber baseWidth = usableWidth / rowCount;
			const PixelNumber relativeX = column * (baseWidth + gap);
			const PixelNumber bw = (column + 1 == rowCount) ? width - relativeX : baseWidth;
			const PixelNumber bx = left + relativeX;
			const PixelNumber by = top + starts[group] + row * rowStep;
			const uint8_t config = (uint8_t)(((group + 1) << 4) | index);
			const char* label = group == 2 ? "Macro" : defaults[group][index];
			if (labels[group][index] != nullptr && labels[group][index][0] != 0)
			{
				label = labels[group][index];
			}

			const PixelNumber x1 = bx + 2;
			const PixelNumber y1 = by + 1;
			const PixelNumber x2 = bx + bw - 3;
			const PixelNumber y2 = by + buttonHeight - 2;
			lcd.setColor(config == selected ? pressedBackColour : bcolour);
			lcd.fillRoundRect(x1, y1, x2, y2);
			lcd.setColor(borderColour);
			lcd.drawRoundRect(x1, y1, x2, y2);
			lcd.setColor(fcolour);
			lcd.setTextPos(0, 9999, bw - 8);
			lcd.printf("%s", label);
			const PixelNumber textWidth = lcd.getTextX();
			lcd.setTextPos(x1 + max<int>(2, ((int)bw - (int)textWidth) / 2), y1 + 4, x2 - 2);
			lcd.printf("%s", label);
			++ordinal;
		}
	}

	if (fanCount == 0)
	{
		lcd.setColor(fcolour);
		lcd.setTextPos(left + 8, top + fanY + 4, left + width - 8);
		lcd.printf("No fans reported by RRF");
	}
	if (outputCount == 0)
	{
		lcd.setColor(fcolour);
		lcd.setTextPos(left + 8, top + outputY + 4, left + width - 8);
		lcd.printf("No outputs reported by RRF");
	}
	if (macroCount == 0)
	{
		lcd.setColor(fcolour);
		lcd.setTextPos(left + 8, top + macroY + 4, left + width - 8);
		lcd.printf("No root macros loaded");
	}
	lcd.setTransparentBackground(false);
	changed = false;
}

size_t FloatButton::PrintText(size_t offset) const
{
	UNUSED(offset);
	size_t ret = lcd.printf("%.*f", numDecimals, static_cast<double>(val));
	if (units != nullptr)
	{
		ret += lcd.printf("%s", units);
	}
	return ret;
}

ButtonRow::ButtonRow(PixelNumber py, PixelNumber px, PixelNumber pw, PixelNumber ps, unsigned int nb, event_t e)
	: ButtonBase(py, px, pw), numButtons(nb), whichPressed(-1), step(ps)
{
	evt = e;
}

/*static*/ LcdFont ButtonRowWithText::font;

ButtonRowWithText::ButtonRowWithText(PixelNumber py, PixelNumber px, PixelNumber pw, PixelNumber ps, unsigned int nb, event_t e)
	: ButtonRow(py, px, pw, ps, nb, e)
{
}

PixelNumber ButtonRowWithText::GetHeight() const
{
	PixelNumber ret = (UTFT::GetFontHeight(font) + 2) * textRows - 2;	// height of the text
	ret += 2 * textMargin + 2;											// add the border height
	return customHeight != 0 ? customHeight : ret;
}

void ButtonRowWithText::Refresh(bool full, PixelNumber xOffset, PixelNumber yOffset)
{
	if (full || changed)
	{
		for (unsigned int i = 0; i < numButtons; ++i)
		{
			const PixelNumber buttonXoffset = xOffset + i * step;
			DrawOutline(buttonXoffset, yOffset, (int)i == whichPressed);
			lcd.setTransparentBackground(true);
			lcd.setColor(fcolour);
			lcd.setFont(font);
			lcd.setTextPos(0, 9999, width - 6);
			PrintText(i);							// dummy print to get text width
			PixelNumber spare = width - 6 - lcd.getTextX();
			const PixelNumber naturalHeight = ((UTFT::GetFontHeight(font) + 2) * textRows - 2) + 2 * textMargin + 2;
			const PixelNumber extraY = (GetHeight() > naturalHeight) ? (GetHeight() - naturalHeight)/2 : 0;
			lcd.setTextPos(x + buttonXoffset + 3 + spare/2, y + yOffset + textMargin + 1 + extraY, x + buttonXoffset + width - 3);	// text is always centre-aligned
			PrintText(i);
			lcd.setTransparentBackground(false);
		}
		changed = false;
	}
}

void CharButtonRow::PrintText(unsigned int n) const
{
	lcd.write(text[n]);
}

CharButtonRow::CharButtonRow(PixelNumber py, PixelNumber px, PixelNumber pw, PixelNumber ps, const char * _ecv_array s, event_t e)
	: ButtonRowWithText(py, px, pw, ps, strlen(s), e), text(s)
{
}

void CharButtonRow::CheckEvent(PixelNumber x, PixelNumber y, int& bestError, ButtonPress& best) /*override*/
{
	if (visible && GetEvent() != nullEvent)
	{
		const int yError = (y < GetMinY()) ? GetMinY() - y
								: (y > GetMaxY()) ? y - GetMaxY()
									: 0;
		if (yError < maxYerror && yError < bestError)
		{
			PixelNumber minX = GetMinX();
			PixelNumber maxX = GetMaxX();
			for (size_t i = 0; i < numButtons; ++i)
			{
				const int xError = (x < minX) ? minX - x
										: (x > maxX) ? x - maxX
											: 0;
				if (xError < maxXerror && xError + yError < bestError)
				{
					bestError = xError + yError;
					best.Set(this, i);
				}
				minX += step;
				maxX += step;
			}
		}
	}
}

void CharButtonRow::Press(bool p, int index) /*override*/
{
	whichPressed = (p) ? index : -1;
}

void CharButtonRow::ChangeText(const char* _ecv_array s)
{
	if (strcmp(text, s) == 0)
	{
		return;
	}
	text = s;
	changed = true;
}

void ProgressBar::Refresh(bool full, PixelNumber xOffset, PixelNumber yOffset)
{
	if (full || changed)
	{
		const PixelNumber x1 = x + xOffset;
		const PixelNumber y1 = y + yOffset;
		const PixelNumber x2 = x1 + width - 1;
		const PixelNumber y2 = y1 + height - 1;
		const PixelNumber pixelsSet = (width * percent) / 100;

		// Redraw the complete bar with rounded indicator ends.
		lcd.setColor(bcolour);
		lcd.fillRoundRect(x1, y1, x2, y2);
		if (pixelsSet > 2)
		{
			lcd.setColor(fcolour);
			const PixelNumber requestedX2 = x1 + pixelsSet - 1;
			const PixelNumber progressX2 = (requestedX2 < x2) ? requestedX2 : x2;
			lcd.fillRoundRect(x1, y1, progressX2, y2);
		}

		changed = false;
		lastNumPixelsSet = pixelsSet;
	}
}

void StaticImageField::Refresh(bool full, PixelNumber xOffset, PixelNumber yOffset)
{
	if (full || changed)
	{
		lcd.drawCompressedBitmap(x + xOffset, y + yOffset, width, height, data);
		changed = false;
	}
}

void DrawDirect::Refresh(bool full, PixelNumber xOffset, PixelNumber yOffset)
{
	// nothing todo
	UNUSED(full); UNUSED(xOffset); UNUSED(yOffset);

	if (refreshNotify)
		refreshNotify(full, changed);

	changed = false;
}

void DrawDirect::DrawRect(PixelNumber widthRect, PixelNumber heightRect, unsigned int pixels_offset, const qoi_rgba_t *pixels, size_t pixels_count)
{
	if (!IsVisible())
	{
		dbg("not visible.\n");
		return;
	}

	if (widthRect > width || heightRect > height)
	{
		dbg("rect does not fit\n");
		return;
	}

	PixelNumber xabs = x;
	PixelNumber yabs = y;

	if (parent)
	{
		xabs += parent->Xpos();
		yabs += parent->Ypos();
	}

	if (widthRect < width)
	{
		xabs += (width - widthRect);
	}

	if (heightRect < height)
	{
		yabs += (height - heightRect) / 2;
	}

	lcd.drawBitmapRgbaStream(xabs, yabs, widthRect, heightRect, pixels_offset, reinterpret_cast<const uint32_t *>(pixels), pixels_count);
	changed = false;
}

// End
