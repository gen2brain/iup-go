/** \file
 * \brief WinUI Driver - Common Function Declarations
 *
 * - ih->handle stores the native widget directly (raw COM pointer for WinUI controls, HWND for dialogs).
 * - Auxiliary data (event tokens, etc.) is stored in IUP attributes as pointers to small structs allocated per-control.
 *
 * See Copyright Notice in "iup.h"
 */

#ifndef __IUPWINUI_DRV_H
#define __IUPWINUI_DRV_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "iup.h"

#ifndef __IUP_OBJECT_H
#include "iup_object.h"
#endif

/****************************************************************************
 * Macros and Type Definitions
 ****************************************************************************/

#define iupwinuiColorFromDouble(_x) ((int)((_x)*255.0))
#define iupwinuiColorToDouble(_x) ((double)(_x)/255.0)

/****************************************************************************
 * Color Management
 ****************************************************************************/

void iupwinuiSetBgColor(InativeHandle* handle, unsigned char r, unsigned char g, unsigned char b);
void iupwinuiSetFgColor(InativeHandle* handle, unsigned char r, unsigned char g, unsigned char b);

/****************************************************************************
 * Widget Management
 ****************************************************************************/

void iupwinuiAddToParent(Ihandle* ih);

/****************************************************************************
 * Focus Management
 ****************************************************************************/

void iupwinuiSetCanFocus(void* widget, int can);

/****************************************************************************
 * Key Input Handling
 ****************************************************************************/

int iupwinuiKeyDecode(int keyval);
int iupwinuiKeyEvent(Ihandle* ih, int wincode, int press);
void iupwinuiButtonKeySetStatus(int modifiers, int button, char* status, int doubleclick);

/****************************************************************************
 * Font Management
 ****************************************************************************/

void iupwinuiFontInit(void);
void iupwinuiFontFinish(void);
char* iupwinuiGetFontAttrib(Ihandle* ih);
int iupwinuiSetFontAttrib(Ihandle* ih, const char* value);
float iupwinuiFontGetMultilineLineHeightF(Ihandle* ih);

/****************************************************************************
 * System Utilities
 ****************************************************************************/

int iupwinuiIsSystemDarkMode(void);
void iupwinuiSetGlobalColors(void);

/****************************************************************************
 * Application Management
 ****************************************************************************/

void* iupwinuiGetDispatcherQueue(void);
void iupwinuiLoopCleanup(void);
BOOL iupwinuiContentPreTranslateMessage(const MSG* msg);
void iupwinuiProcessPendingMessages(void);
void iupwinuiBringWindowToForeground(HWND hwnd);


#ifdef __cplusplus
}

#include "pch.h"
#include <string>
#include <memory>
#include "iup_attrib.h"

/****************************************************************************
 * Handle Type Check
 *
 * ih->handle stores raw COM pointers for WinUI controls, but some controls
 * (Dialog, GLCanvas, WebBrowser) use raw HWNDs instead.
 ****************************************************************************/

inline bool winuiHandleIsHWND(Ihandle* ih)
{
  return IupClassMatch(ih, "dialog") ||
         iupAttribGet(ih, "_IUP_GLCONTROLDATA") != NULL ||
         IupClassMatch(ih, "webbrowser");
}

/****************************************************************************
 * String Conversion Helpers (C++ only)
 ****************************************************************************/

winrt::hstring iupwinuiStringToHString(const char* str);
char* iupwinuiHStringToString(const winrt::hstring& str);
std::wstring iupwinuiStringToWString(const char* str);

/****************************************************************************
 * Mnemonic Processing Helper
 *
 * Processes mnemonic markers (&) in text:
 * - Strips & from the text (returns processed hstring)
 * - Returns the mnemonic character if found (in c parameter)
 ****************************************************************************/

winrt::hstring iupwinuiProcessMnemonic(const char* str, char* c);

/****************************************************************************
 * Parent Canvas Helper
 ****************************************************************************/

winrt::Microsoft::UI::Xaml::Controls::Canvas iupwinuiGetParentCanvas(Ihandle* ih);

void iupwinuiUpdateControlFont(Ihandle* ih, winrt::Microsoft::UI::Xaml::Controls::Control control);
void iupwinuiUpdateTextBlockFont(Ihandle* ih, winrt::Microsoft::UI::Xaml::Controls::TextBlock textBlock);

void winuiDialogSetMenuBar(Ihandle* ih, winrt::Microsoft::UI::Xaml::Controls::MenuBar menuBar);

winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::UI::Xaml::Controls::MenuFlyoutItemBase> winuiMenuGetItemsCollection(Ihandle* menu);

void iupwinuiCanvasCallAction(Ihandle* ih);

void winuiFrameUpdateBorderColor(Ihandle* ih);
void winuiTableRefreshThemeColors(Ihandle* ih);
void winuiTreeRefreshThemeColors(Ihandle* ih);

void winuiDragSetInProcessData(const char* type, void* data, int size);
void winuiDragDataCleanup(void);

winrt::Microsoft::UI::Xaml::Media::Imaging::WriteableBitmap winuiGetBitmapFromHandle(void* handle);

/****************************************************************************
 * COM Pointer Storage Helpers
 *
 * ih->handle stores raw COM pointers (IUnknown*) for WinUI controls.
 * These helpers manage the pointer lifecycle using WinRT interop.
 *
 *   Map:    winuiStoreHandle(ih, widget)     - stores raw pointer, transfers ownership
 *   Use:    auto w = winuiGetHandle<T>(ih)   - gets temporary wrapper (adds ref)
 *   UnMap:  winuiReleaseHandle<T>(ih)        - releases the stored pointer
 ****************************************************************************/

template<typename T>
inline void winuiStoreHandle(Ihandle* ih, T& widget)
{
  ih->handle = reinterpret_cast<InativeHandle*>(winrt::detach_abi(widget));
  iupwinuiProcessPendingMessages();
}

template<typename T>
inline T winuiGetHandle(Ihandle* ih)
{
  T widget{nullptr};
  if (ih && ih->handle)
  {
    winrt::Windows::Foundation::IInspectable obj{nullptr};
    winrt::copy_from_abi(obj, ih->handle);
    widget = obj.try_as<T>();
  }
  return widget;
}

template<typename T>
inline void winuiReleaseHandle(Ihandle* ih)
{
  if (ih && ih->handle)
  {
    winrt::Windows::Foundation::IInspectable obj{nullptr};
    winrt::attach_abi(obj, ih->handle);
    ih->handle = nullptr;
  }
}

/****************************************************************************
 * Pointer Event Helpers
 *
 * Shared by Canvas, Label, and any control that needs BUTTON_CB support.
 ****************************************************************************/

inline int iupwinuiGetModifierKeys(void)
{
  int keys = 0;
  if (GetKeyState(VK_SHIFT) & 0x8000)
    keys |= MK_SHIFT;
  if (GetKeyState(VK_CONTROL) & 0x8000)
    keys |= MK_CONTROL;
  if (GetKeyState(VK_LBUTTON) & 0x8000)
    keys |= MK_LBUTTON;
  if (GetKeyState(VK_MBUTTON) & 0x8000)
    keys |= MK_MBUTTON;
  if (GetKeyState(VK_RBUTTON) & 0x8000)
    keys |= MK_RBUTTON;
  if (GetKeyState(VK_XBUTTON1) & 0x8000)
    keys |= MK_XBUTTON1;
  if (GetKeyState(VK_XBUTTON2) & 0x8000)
    keys |= MK_XBUTTON2;
  return keys;
}

inline int iupwinuiGetPointerButton(winrt::Microsoft::UI::Input::PointerPointProperties const& props)
{
  if (props.IsLeftButtonPressed())
    return IUP_BUTTON1;
  if (props.IsMiddleButtonPressed())
    return IUP_BUTTON2;
  if (props.IsRightButtonPressed())
    return IUP_BUTTON3;
  return IUP_BUTTON1;
}

inline int iupwinuiGetPointerReleasedButton(winrt::Microsoft::UI::Input::PointerPointProperties const& props)
{
  if (!props.IsLeftButtonPressed())
    return IUP_BUTTON1;
  if (!props.IsMiddleButtonPressed())
    return IUP_BUTTON2;
  if (!props.IsRightButtonPressed())
    return IUP_BUTTON3;
  return IUP_BUTTON1;
}

/****************************************************************************
 * Auxiliary Data Structures
 *
 * These structs store data that cannot be stored in ih->handle
 * (event tokens, secondary widgets, flags). They are allocated when needed
 * and stored via IUP attributes.
 ****************************************************************************/

#define IUPWINUI_DIALOG_AUX "_IUPWINUI_DIALOG_AUX"
#define IUPWINUI_BUTTON_AUX "_IUPWINUI_BUTTON_AUX"
#define IUPWINUI_TOGGLE_AUX "_IUPWINUI_TOGGLE_AUX"
#define IUPWINUI_TEXT_AUX "_IUPWINUI_TEXT_AUX"
#define IUPWINUI_FRAME_AUX "_IUPWINUI_FRAME_AUX"
#define IUPWINUI_LIST_AUX "_IUPWINUI_LIST_AUX"
#define IUPWINUI_TREE_AUX "_IUPWINUI_TREE_AUX"
#define IUPWINUI_MENU_AUX "_IUPWINUI_MENU_AUX"
#define IUPWINUI_ITEM_AUX "_IUPWINUI_ITEM_AUX"
#define IUPWINUI_TABLE_AUX "_IUPWINUI_TABLE_AUX"
#define IUPWINUI_LABEL_AUX "_IUPWINUI_LABEL_AUX"
#define IUPWINUI_POPOVER_AUX "_IUPWINUI_POPOVER_AUX"
#define IUPWINUI_CANVAS_AUX "_IUPWINUI_CANVAS_AUX"

struct IupWinUIDialogAux
{
  winrt::Microsoft::UI::Xaml::Hosting::DesktopWindowXamlSource xamlSource;
  winrt::Microsoft::UI::Content::DesktopChildSiteBridge siteBridge;
  winrt::Microsoft::UI::Xaml::Controls::Grid rootPanel;
  winrt::Microsoft::UI::Xaml::Controls::Canvas contentCanvas;
  winrt::Microsoft::UI::Xaml::Controls::MenuBar menuBar;
  winrt::event_token takeFocusToken;
  winrt::event_token gotFocusToken;
  HWND islandHwnd;
  bool isVisible;
  bool windowCreated;

  IupWinUIDialogAux() : xamlSource(nullptr), siteBridge(nullptr),
                        rootPanel(nullptr), contentCanvas(nullptr), menuBar(nullptr),
                        takeFocusToken{}, gotFocusToken{},
                        islandHwnd(NULL),
                        isVisible(false), windowCreated(false) {}
};

struct IupWinUIButtonAux
{
  winrt::event_token clickToken;
  winrt::event_token pointerPressedToken;
  winrt::event_token captureLostToken;
  winrt::event_token buttonPressedToken;
  winrt::event_token buttonReleasedToken;
  winrt::event_token buttonCaptureLostToken;
  winrt::event_token keyDownToken;

  IupWinUIButtonAux() : clickToken{}, pointerPressedToken{}, captureLostToken{}, buttonPressedToken{}, buttonReleasedToken{}, buttonCaptureLostToken{}, keyDownToken{} {}
};

enum IupWinUIToggleType
{
  IUPWINUI_TOGGLE_CHECKBOX,
  IUPWINUI_TOGGLE_RADIOBUTTON,
  IUPWINUI_TOGGLE_TOGGLEBUTTON,
  IUPWINUI_TOGGLE_TOGGLESWITCH
};

struct IupWinUIToggleAux
{
  winrt::event_token checkedToken;
  winrt::event_token uncheckedToken;
  winrt::event_token clickToken;
  winrt::event_token toggledToken;
  winrt::event_token keyDownToken;
  IupWinUIToggleType controlType;

  IupWinUIToggleAux() : checkedToken{}, uncheckedToken{}, clickToken{}, toggledToken{},
                        keyDownToken{}, controlType(IUPWINUI_TOGGLE_CHECKBOX) {}
};

struct IupWinUITextAux
{
  winrt::event_token beforeTextChangingToken;
  winrt::event_token textChangedToken;
  winrt::event_token gotFocusToken;
  winrt::event_token lostFocusToken;
  winrt::event_token valueChangedToken;
  winrt::event_token keyDownToken;
  std::wstring savedText;
  bool isPassword;
  bool isMultiline;
  bool isSpin;
  bool isFormatted;

  IupWinUITextAux() : beforeTextChangingToken{}, textChangedToken{}, gotFocusToken{}, lostFocusToken{},
                       valueChangedToken{}, keyDownToken{},
                       isPassword(false), isMultiline(false), isSpin(false), isFormatted(false) {}
};

struct IupWinUIFrameAux
{
  winrt::Microsoft::UI::Xaml::Controls::TextBlock titleBlock;
  winrt::Microsoft::UI::Xaml::Controls::Border titleBorder;
  winrt::Microsoft::UI::Xaml::Controls::Border frameBorder;
  winrt::Microsoft::UI::Xaml::Controls::Canvas innerCanvas;
  bool hasTitle;

  IupWinUIFrameAux() : titleBlock(nullptr), titleBorder(nullptr), frameBorder(nullptr), innerCanvas(nullptr), hasTitle(false) {}
};

struct IupWinUIListAux
{
  winrt::event_token selectionChangedToken;
  winrt::event_token textChangedToken;
  winrt::event_token dropDownOpenedToken;
  winrt::event_token dropDownClosedToken;
  winrt::event_token doubleTappedToken;
  winrt::event_token keyDownToken;
  winrt::event_token containerContentChangingToken;
  winrt::event_token dragItemsStartingToken;
  winrt::event_token dragItemsCompletedToken;
  bool isDropdown;
  bool hasEditbox;
  bool isMultiple;
  bool isVirtual;
  bool ignoreChange;

  IupWinUIListAux() : selectionChangedToken{}, textChangedToken{},
                       dropDownOpenedToken{}, dropDownClosedToken{},
                       doubleTappedToken{}, keyDownToken{},
                       containerContentChangingToken{},
                       dragItemsStartingToken{}, dragItemsCompletedToken{},
                       isDropdown(false), hasEditbox(false), isMultiple(false),
                       isVirtual(false), ignoreChange(false) {}
};

struct IupWinUITreeAux
{
  winrt::event_token expandingToken;
  winrt::event_token collapsedToken;
  winrt::event_token itemInvokedToken;
  winrt::event_token selectionChangedToken;
  winrt::event_token rightTappedToken;
  winrt::event_token keyDownToken;
  winrt::event_token doubleTappedToken;
  winrt::event_token dragItemsStartingToken;
  winrt::event_token dragItemsCompletedToken;
  bool ignoreChange;

  IupWinUITreeAux() : expandingToken{}, collapsedToken{},
                       itemInvokedToken{}, selectionChangedToken{},
                       rightTappedToken{}, keyDownToken{}, doubleTappedToken{},
                       dragItemsStartingToken{}, dragItemsCompletedToken{},
                       ignoreChange(false) {}
};

struct IupWinUIMenuAux
{
  bool isMenuBar;
  bool isPopup;

  IupWinUIMenuAux() : isMenuBar(false), isPopup(false) {}
};

struct IupWinUIItemAux
{
  winrt::event_token clickToken;
  bool isCheckable;

  IupWinUIItemAux() : clickToken{}, isCheckable(false) {}
};

struct IupWinUITableAux
{
  int* col_widths;
  bool* col_width_set;
  char** col_titles;
  char*** cell_values;

  int sort_column;
  int sort_ascending;

  int current_row;
  int current_col;

  int edit_row, edit_col;
  bool editing;
  bool isVirtual;
  bool show_grid;
  bool suppress_callbacks;

  int drag_source_col;
  int drag_target_col;
  bool dragging;
  double drag_start_x;
  double drag_start_y;

  int resize_col;
  double resize_start_x;
  int resize_start_width;

  winrt::event_token selectionChangedToken;
  winrt::event_token doubleTappedToken;
  winrt::event_token keyDownToken;
  winrt::event_token sizeChangedToken;
  winrt::event_token containerContentChangingToken;

  IupWinUITableAux() : col_widths(nullptr), col_width_set(nullptr),
                        col_titles(nullptr), cell_values(nullptr),
                        sort_column(0), sort_ascending(0),
                        current_row(0), current_col(0),
                        edit_row(0), edit_col(0),
                        editing(false), isVirtual(false), show_grid(true),
                        suppress_callbacks(false),
                        drag_source_col(0), drag_target_col(0),
                        dragging(false), drag_start_x(0), drag_start_y(0),
                        resize_col(0), resize_start_x(0), resize_start_width(0),
                        selectionChangedToken{}, doubleTappedToken{},
                        keyDownToken{}, sizeChangedToken{},
                        containerContentChangingToken{} {}
};

struct IupWinUILabelAux
{
  winrt::event_token pointerPressedToken;
  winrt::event_token pointerReleasedToken;
  winrt::event_token pointerEnteredToken;
  winrt::event_token pointerExitedToken;

  IupWinUILabelAux() : pointerPressedToken{}, pointerReleasedToken{},
                        pointerEnteredToken{}, pointerExitedToken{} {}
};

struct IupWinUIPopoverAux
{
  winrt::Microsoft::UI::Xaml::Controls::Flyout flyout;
  winrt::Microsoft::UI::Xaml::Controls::Canvas innerCanvas;
  winrt::event_token closedToken;
  winrt::event_token closingToken;
  bool isVisible;
  bool programmaticClose;

  IupWinUIPopoverAux() : flyout(nullptr), innerCanvas(nullptr), closedToken{}, closingToken{}, isVisible(false), programmaticClose(false) {}
};

struct IupWinUICanvasAux
{
  winrt::Microsoft::UI::Xaml::Controls::Image displayImage;
  winrt::Microsoft::UI::Xaml::Controls::Primitives::ScrollBar sbHoriz;
  winrt::Microsoft::UI::Xaml::Controls::Primitives::ScrollBar sbVert;
  winrt::event_token pointerPressedToken;
  winrt::event_token pointerReleasedToken;
  winrt::event_token pointerMovedToken;
  winrt::event_token pointerWheelToken;
  winrt::event_token pointerEnteredToken;
  winrt::event_token pointerExitedToken;
  winrt::event_token keyDownToken;
  winrt::event_token keyUpToken;
  winrt::event_token gotFocusToken;
  winrt::event_token lostFocusToken;
  winrt::event_token sizeChangedToken;
  winrt::event_token sbHorizScrollToken;
  winrt::event_token sbVertScrollToken;
  std::shared_ptr<bool> alive;

  IupWinUICanvasAux() : displayImage(nullptr),
    sbHoriz(nullptr), sbVert(nullptr),
    pointerPressedToken{}, pointerReleasedToken{}, pointerMovedToken{},
    pointerWheelToken{}, pointerEnteredToken{}, pointerExitedToken{},
    keyDownToken{}, keyUpToken{},
    gotFocusToken{}, lostFocusToken{}, sizeChangedToken{},
    sbHorizScrollToken{}, sbVertScrollToken{},
    alive(std::make_shared<bool>(true)) {}
};

template<typename AuxType>
inline AuxType* winuiGetAux(Ihandle* ih, const char* attr_name)
{
  return reinterpret_cast<AuxType*>(iupAttribGet(ih, attr_name));
}

template<typename AuxType>
inline void winuiSetAux(Ihandle* ih, const char* attr_name, AuxType* aux)
{
  iupAttribSet(ih, attr_name, reinterpret_cast<const char*>(aux));
}

template<typename AuxType>
inline void winuiFreeAux(Ihandle* ih, const char* attr_name)
{
  AuxType* aux = winuiGetAux<AuxType>(ih, attr_name);
  if (aux)
  {
    delete aux;
    iupAttribSet(ih, attr_name, nullptr);
  }
}

#endif

#endif /* __IUPWINUI_DRV_H */
