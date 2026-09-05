/** \file
 * \brief Haiku Driver Control Look. Wraps be_control_look so the BeOS controls
 * draw with the forced APPEARANCE palette without changing the system colors.
 *
 * See Copyright Notice in "iup.h"
 */

#include <ControlLook.h>
#include <InterfaceDefs.h>

#include "iup.h"
#include "iup_object.h"

#include "iuphaiku_drv.h"


static const color_which kBaseColors[] = {
  B_PANEL_BACKGROUND_COLOR, B_CONTROL_BACKGROUND_COLOR, B_SCROLL_BAR_THUMB_COLOR,
  B_DOCUMENT_BACKGROUND_COLOR, B_LIST_BACKGROUND_COLOR, B_LIST_SELECTED_BACKGROUND_COLOR,
  B_MENU_BACKGROUND_COLOR, B_MENU_SELECTED_BACKGROUND_COLOR
};
static const color_which kTextColors[] = {
  B_PANEL_TEXT_COLOR, B_CONTROL_TEXT_COLOR, B_DOCUMENT_TEXT_COLOR, B_LIST_ITEM_TEXT_COLOR,
  B_LIST_SELECTED_ITEM_TEXT_COLOR, B_MENU_ITEM_TEXT_COLOR, B_MENU_SELECTED_ITEM_TEXT_COLOR, B_WINDOW_TEXT_COLOR
};
static const int kBaseCount = (int)(sizeof(kBaseColors) / sizeof(kBaseColors[0]));
static const int kTextCount = (int)(sizeof(kTextColors) / sizeof(kTextColors[0]));

class IupHaikuControlLook : public BControlLook
{
public:
  explicit IupHaikuControlLook(BControlLook* base) : fBase(base), fActive(false) {}
  ~IupHaikuControlLook() override { delete fBase; }

  void Update()
  {
    fActive = iuphaikuColorForced() ? true : false;
    for (int i = 0; i < kBaseCount; i++)
    {
      fSystemBase[i] = ui_color(kBaseColors[i]);
      fForcedBase[i] = iuphaikuColor(kBaseColors[i]);
    }
    for (int i = 0; i < kTextCount; i++)
    {
      fSystemText[i] = ui_color(kTextColors[i]);
      fForcedText[i] = iuphaikuColor(kTextColors[i]);
    }
  }

  rgb_color Map(const rgb_color& c) const
  {
    if (!fActive)
      return c;
    for (int i = 0; i < kBaseCount; i++)
    {
      if (c == fSystemBase[i])
        return fForcedBase[i];
    }
    return c;
  }

  const rgb_color* MapText(const rgb_color* c, rgb_color* storage) const
  {
    if (!c || !fActive)
      return c;
    for (int i = 0; i < kTextCount; i++)
    {
      if (*c == fSystemText[i])
      {
        *storage = fForcedText[i];
        return storage;
      }
    }
    return c;
  }

  BAlignment DefaultLabelAlignment() const override { return fBase->DefaultLabelAlignment(); }
  float DefaultLabelSpacing() const override { return fBase->DefaultLabelSpacing(); }
  float DefaultItemSpacing() const override { return fBase->DefaultItemSpacing(); }
  uint32 Flags(BControl* control) const override { return fBase->Flags(control); }

  void DrawButtonFrame(BView* view, BRect& rect, const BRect& updateRect, const rgb_color& base, const rgb_color& background, uint32 flags, uint32 borders) override
  { fBase->DrawButtonFrame(view, rect, updateRect, Map(base), Map(background), flags, borders); }
  void DrawButtonFrame(BView* view, BRect& rect, const BRect& updateRect, float radius, const rgb_color& base, const rgb_color& background, uint32 flags, uint32 borders) override
  { fBase->DrawButtonFrame(view, rect, updateRect, radius, Map(base), Map(background), flags, borders); }
  void DrawButtonFrame(BView* view, BRect& rect, const BRect& updateRect, float leftTopRadius, float rightTopRadius, float leftBottomRadius, float rightBottomRadius, const rgb_color& base, const rgb_color& background, uint32 flags, uint32 borders) override
  { fBase->DrawButtonFrame(view, rect, updateRect, leftTopRadius, rightTopRadius, leftBottomRadius, rightBottomRadius, Map(base), Map(background), flags, borders); }

  void DrawButtonBackground(BView* view, BRect& rect, const BRect& updateRect, const rgb_color& base, uint32 flags, uint32 borders, orientation orientation) override
  { fBase->DrawButtonBackground(view, rect, updateRect, Map(base), flags, borders, orientation); }
  void DrawButtonBackground(BView* view, BRect& rect, const BRect& updateRect, float radius, const rgb_color& base, uint32 flags, uint32 borders, orientation orientation) override
  { fBase->DrawButtonBackground(view, rect, updateRect, radius, Map(base), flags, borders, orientation); }
  void DrawButtonBackground(BView* view, BRect& rect, const BRect& updateRect, float leftTopRadius, float rightTopRadius, float leftBottomRadius, float rightBottomRadius, const rgb_color& base, uint32 flags, uint32 borders, orientation orientation) override
  { fBase->DrawButtonBackground(view, rect, updateRect, leftTopRadius, rightTopRadius, leftBottomRadius, rightBottomRadius, Map(base), flags, borders, orientation); }

  void DrawMenuBarBackground(BView* view, BRect& rect, const BRect& updateRect, const rgb_color& base, uint32 flags, uint32 borders) override
  { fBase->DrawMenuBarBackground(view, rect, updateRect, Map(base), flags, borders); }

  void DrawMenuFieldFrame(BView* view, BRect& rect, const BRect& updateRect, const rgb_color& base, const rgb_color& background, uint32 flags, uint32 borders) override
  { fBase->DrawMenuFieldFrame(view, rect, updateRect, Map(base), Map(background), flags, borders); }
  void DrawMenuFieldFrame(BView* view, BRect& rect, const BRect& updateRect, float radius, const rgb_color& base, const rgb_color& background, uint32 flags, uint32 borders) override
  { fBase->DrawMenuFieldFrame(view, rect, updateRect, radius, Map(base), Map(background), flags, borders); }
  void DrawMenuFieldFrame(BView* view, BRect& rect, const BRect& updateRect, float leftTopRadius, float rightTopRadius, float leftBottomRadius, float rightBottomRadius, const rgb_color& base, const rgb_color& background, uint32 flags, uint32 borders) override
  { fBase->DrawMenuFieldFrame(view, rect, updateRect, leftTopRadius, rightTopRadius, leftBottomRadius, rightBottomRadius, Map(base), Map(background), flags, borders); }

  void DrawMenuFieldBackground(BView* view, BRect& rect, const BRect& updateRect, const rgb_color& base, bool popupIndicator, uint32 flags) override
  { fBase->DrawMenuFieldBackground(view, rect, updateRect, Map(base), popupIndicator, flags); }
  void DrawMenuFieldBackground(BView* view, BRect& rect, const BRect& updateRect, float radius, const rgb_color& base, bool popupIndicator, uint32 flags) override
  { fBase->DrawMenuFieldBackground(view, rect, updateRect, radius, Map(base), popupIndicator, flags); }
  void DrawMenuFieldBackground(BView* view, BRect& rect, const BRect& updateRect, float leftTopRadius, float rightTopRadius, float leftBottomRadius, float rightBottomRadius, const rgb_color& base, bool popupIndicator, uint32 flags) override
  { fBase->DrawMenuFieldBackground(view, rect, updateRect, leftTopRadius, rightTopRadius, leftBottomRadius, rightBottomRadius, Map(base), popupIndicator, flags); }
  void DrawMenuFieldBackground(BView* view, BRect& rect, const BRect& updateRect, const rgb_color& base, uint32 flags, uint32 borders) override
  { fBase->DrawMenuFieldBackground(view, rect, updateRect, Map(base), flags, borders); }

  void DrawMenuBackground(BView* view, BRect& rect, const BRect& updateRect, const rgb_color& base, uint32 flags, uint32 borders) override
  { fBase->DrawMenuBackground(view, rect, updateRect, Map(base), flags, borders); }
  void DrawMenuItemBackground(BView* view, BRect& rect, const BRect& updateRect, const rgb_color& base, uint32 flags, uint32 borders) override
  { fBase->DrawMenuItemBackground(view, rect, updateRect, Map(base), flags, borders); }

  void DrawStatusBar(BView* view, BRect& rect, const BRect& updateRect, const rgb_color& base, const rgb_color& barColor, float progressPosition) override
  { fBase->DrawStatusBar(view, rect, updateRect, Map(base), barColor, progressPosition); }
  void DrawCheckBox(BView* view, BRect& rect, const BRect& updateRect, const rgb_color& base, uint32 flags) override
  { fBase->DrawCheckBox(view, rect, updateRect, Map(base), flags); }
  void DrawRadioButton(BView* view, BRect& rect, const BRect& updateRect, const rgb_color& base, uint32 flags) override
  { fBase->DrawRadioButton(view, rect, updateRect, Map(base), flags); }

  void DrawScrollBarBackground(BView* view, BRect& rect1, BRect& rect2, const BRect& updateRect, const rgb_color& base, uint32 flags, orientation orientation) override
  { fBase->DrawScrollBarBackground(view, rect1, rect2, updateRect, Map(base), flags, orientation); }
  void DrawScrollBarBackground(BView* view, BRect& rect, const BRect& updateRect, const rgb_color& base, uint32 flags, orientation orientation) override
  { fBase->DrawScrollBarBackground(view, rect, updateRect, Map(base), flags, orientation); }
  void DrawScrollViewFrame(BView* view, BRect& rect, const BRect& updateRect, BRect verticalScrollBarFrame, BRect horizontalScrollBarFrame, const rgb_color& base, border_style borderStyle, uint32 flags, uint32 borders) override
  { fBase->DrawScrollViewFrame(view, rect, updateRect, verticalScrollBarFrame, horizontalScrollBarFrame, Map(base), borderStyle, flags, borders); }
  void DrawArrowShape(BView* view, BRect& rect, const BRect& updateRect, const rgb_color& base, uint32 direction, uint32 flags, float tint) override
  { fBase->DrawArrowShape(view, rect, updateRect, Map(base), direction, flags, tint); }

  rgb_color SliderBarColor(const rgb_color& base) override { return fBase->SliderBarColor(Map(base)); }
  void DrawSliderBar(BView* view, BRect rect, const BRect& updateRect, const rgb_color& base, rgb_color leftFillColor, rgb_color rightFillColor, float sliderScale, uint32 flags, orientation orientation) override
  { fBase->DrawSliderBar(view, rect, updateRect, Map(base), leftFillColor, rightFillColor, sliderScale, flags, orientation); }
  void DrawSliderBar(BView* view, BRect rect, const BRect& updateRect, const rgb_color& base, rgb_color fillColor, uint32 flags, orientation orientation) override
  { fBase->DrawSliderBar(view, rect, updateRect, Map(base), fillColor, flags, orientation); }
  void DrawSliderThumb(BView* view, BRect& rect, const BRect& updateRect, const rgb_color& base, uint32 flags, orientation orientation) override
  { fBase->DrawSliderThumb(view, rect, updateRect, Map(base), flags, orientation); }
  void DrawSliderTriangle(BView* view, BRect& rect, const BRect& updateRect, const rgb_color& base, uint32 flags, orientation orientation) override
  { fBase->DrawSliderTriangle(view, rect, updateRect, Map(base), flags, orientation); }
  void DrawSliderTriangle(BView* view, BRect& rect, const BRect& updateRect, const rgb_color& base, const rgb_color& fill, uint32 flags, orientation orientation) override
  { fBase->DrawSliderTriangle(view, rect, updateRect, Map(base), fill, flags, orientation); }
  void DrawSliderHashMarks(BView* view, BRect& rect, const BRect& updateRect, const rgb_color& base, int32 count, hash_mark_location location, uint32 flags, orientation orientation) override
  { fBase->DrawSliderHashMarks(view, rect, updateRect, Map(base), count, location, flags, orientation); }

  void DrawActiveTab(BView* view, BRect& rect, const BRect& updateRect, const rgb_color& base, uint32 flags, uint32 borders, uint32 side, int32 index, int32 selected, int32 first, int32 last) override
  { fBase->DrawActiveTab(view, rect, updateRect, Map(base), flags, borders, side, index, selected, first, last); }
  void DrawInactiveTab(BView* view, BRect& rect, const BRect& updateRect, const rgb_color& base, uint32 flags, uint32 borders, uint32 side, int32 index, int32 selected, int32 first, int32 last) override
  { fBase->DrawInactiveTab(view, rect, updateRect, Map(base), flags, borders, side, index, selected, first, last); }
  void DrawSplitter(BView* view, BRect& rect, const BRect& updateRect, const rgb_color& base, orientation orientation, uint32 flags, uint32 borders) override
  { fBase->DrawSplitter(view, rect, updateRect, Map(base), orientation, flags, borders); }

  void DrawBorder(BView* view, BRect& rect, const BRect& updateRect, const rgb_color& base, border_style borderStyle, uint32 flags, uint32 borders) override
  { fBase->DrawBorder(view, rect, updateRect, Map(base), borderStyle, flags, borders); }
  void DrawRaisedBorder(BView* view, BRect& rect, const BRect& updateRect, const rgb_color& base, uint32 flags, uint32 borders) override
  { fBase->DrawRaisedBorder(view, rect, updateRect, Map(base), flags, borders); }
  void DrawGroupFrame(BView* view, BRect& rect, const BRect& updateRect, const rgb_color& base, uint32 borders) override
  { fBase->DrawGroupFrame(view, rect, updateRect, Map(base), borders); }
  void DrawTextControlBorder(BView* view, BRect& rect, const BRect& updateRect, const rgb_color& base, uint32 flags, uint32 borders) override
  { fBase->DrawTextControlBorder(view, rect, updateRect, Map(base), flags, borders); }

  void DrawLabel(BView* view, const char* label, BRect rect, const BRect& updateRect, const rgb_color& base, uint32 flags, const rgb_color* textColor) override
  { rgb_color tc; fBase->DrawLabel(view, label, rect, updateRect, Map(base), flags, MapText(textColor, &tc)); }
  void DrawLabel(BView* view, const char* label, BRect rect, const BRect& updateRect, const rgb_color& base, uint32 flags, const BAlignment& alignment, const rgb_color* textColor) override
  { rgb_color tc; fBase->DrawLabel(view, label, rect, updateRect, Map(base), flags, alignment, MapText(textColor, &tc)); }
  void DrawLabel(BView* view, const char* label, const rgb_color& base, uint32 flags, const BPoint& where, const rgb_color* textColor) override
  { rgb_color tc; fBase->DrawLabel(view, label, Map(base), flags, where, MapText(textColor, &tc)); }
  void DrawLabel(BView* view, const char* label, const BBitmap* icon, BRect rect, const BRect& updateRect, const rgb_color& base, uint32 flags, const BAlignment& alignment, const rgb_color* textColor) override
  { rgb_color tc; fBase->DrawLabel(view, label, icon, rect, updateRect, Map(base), flags, alignment, MapText(textColor, &tc)); }

  void GetFrameInsets(frame_type frameType, uint32 flags, float& _left, float& _top, float& _right, float& _bottom) override
  { fBase->GetFrameInsets(frameType, flags, _left, _top, _right, _bottom); }
  void GetBackgroundInsets(background_type backgroundType, uint32 flags, float& _left, float& _top, float& _right, float& _bottom) override
  { fBase->GetBackgroundInsets(backgroundType, flags, _left, _top, _right, _bottom); }

  void DrawButtonWithPopUpBackground(BView* view, BRect& rect, const BRect& updateRect, const rgb_color& base, uint32 flags, uint32 borders, orientation orientation) override
  { fBase->DrawButtonWithPopUpBackground(view, rect, updateRect, Map(base), flags, borders, orientation); }
  void DrawButtonWithPopUpBackground(BView* view, BRect& rect, const BRect& updateRect, float radius, const rgb_color& base, uint32 flags, uint32 borders, orientation orientation) override
  { fBase->DrawButtonWithPopUpBackground(view, rect, updateRect, radius, Map(base), flags, borders, orientation); }
  void DrawButtonWithPopUpBackground(BView* view, BRect& rect, const BRect& updateRect, float leftTopRadius, float rightTopRadius, float leftBottomRadius, float rightBottomRadius, const rgb_color& base, uint32 flags, uint32 borders, orientation orientation) override
  { fBase->DrawButtonWithPopUpBackground(view, rect, updateRect, leftTopRadius, rightTopRadius, leftBottomRadius, rightBottomRadius, Map(base), flags, borders, orientation); }

  void DrawTabFrame(BView* view, BRect& rect, const BRect& updateRect, const rgb_color& base, uint32 flags, uint32 borders, border_style borderStyle, uint32 side) override
  { fBase->DrawTabFrame(view, rect, updateRect, Map(base), flags, borders, borderStyle, side); }
  void DrawScrollBarButton(BView* view, BRect rect, const BRect& updateRect, const rgb_color& base, const rgb_color& text, uint32 flags, int32 direction, orientation orientation, bool down) override
  { rgb_color tc; fBase->DrawScrollBarButton(view, rect, updateRect, Map(base), *MapText(&text, &tc), flags, direction, orientation, down); }
  void DrawScrollBarThumb(BView* view, BRect& rect, const BRect& updateRect, const rgb_color& base, uint32 flags, orientation orientation, uint32 knobStyle) override
  { fBase->DrawScrollBarThumb(view, rect, updateRect, Map(base), flags, orientation, knobStyle); }
  void DrawScrollBarBorder(BView* view, BRect rect, const BRect& updateRect, const rgb_color& base, uint32 flags, orientation orientation) override
  { fBase->DrawScrollBarBorder(view, rect, updateRect, Map(base), flags, orientation); }
  float GetScrollBarWidth(orientation orientation) override { return fBase->GetScrollBarWidth(orientation); }

private:
  BControlLook* fBase;
  bool fActive;
  rgb_color fSystemBase[kBaseCount];
  rgb_color fForcedBase[kBaseCount];
  rgb_color fSystemText[kTextCount];
  rgb_color fForcedText[kTextCount];
};

static IupHaikuControlLook* iuphaiku_look = NULL;

IUP_DRV_API void iuphaikuControlLookInstall()
{
  if (iuphaiku_look || !be_control_look)
    return;
  iuphaiku_look = new IupHaikuControlLook(be_control_look);
  iuphaiku_look->Update();
  be_control_look = iuphaiku_look;
}

IUP_DRV_API void iuphaikuControlLookUpdate()
{
  if (iuphaiku_look)
    iuphaiku_look->Update();
}
