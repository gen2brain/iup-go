# IUP Documentation

API reference for IUP (Portable User Interface).

## Guides

- [Attributes](attrib.md) - attribute concept, inheritance, replay rules
- [Events and Callbacks](call.md) - callback dispatch model and main loop
- [Layout](layout.md) - abstract layout, natural size, EXPAND
- [Keyboard](keyboard.md) - focus traversal, mnemonics, accelerators

---

## System

[IupOpen](func/iup_open.md), [IupClose](func/iup_close.md), [IupVersion](func/iup_version.md)

---

## Attributes

### Functions

[IupSetAttribute](func/iup_setattribute.md), [IupSetAttributes](func/iup_setattributes.md), [IupResetAttribute](func/iup_resetattribute.md), [IupSetAtt](func/iup_setatt.md), [IupSetAttributeHandle](func/iup_setattributehandle.md)

[IupGetAttribute](func/iup_getattribute.md), [IupGetAllAttributes](func/iup_getallattributes.md), [IupGetAttributes](func/iup_getattributes.md), [IupCopyAttributes](func/iup_copyattributes.md), [IupGetAttributeHandle](func/iup_getattributehandle.md)

[IupSetGlobal](func/iup_setglobal.md), [IupGetGlobal](func/iup_getglobal.md)

### Common

[ACTIVE](attrib/iup_active.md), [BGCOLOR](attrib/iup_bgcolor.md), [FGCOLOR](attrib/iup_fgcolor.md), [FONT](attrib/iup_font.md), [THEME](attrib/iup_theme.md), [VISIBLE](attrib/iup_visible.md), [CLIENTSIZE](attrib/iup_clientsize.md), [CLIENTOFFSET](attrib/iup_clientoffset.md), [EXPAND](attrib/iup_expand.md), [MAXSIZE](attrib/iup_maxsize.md), [MINSIZE](attrib/iup_minsize.md), [NATURALSIZE](attrib/iup_naturalsize.md), [RASTERSIZE](attrib/iup_rastersize.md), [SIZE](attrib/iup_size.md), [FLOATING](attrib/iup_floating.md), [POSITION](attrib/iup_position.md), [SCREENPOSITION](attrib/iup_screenposition.md), [NAME](attrib/iup_name.md), [TIP](attrib/iup_tip.md), [TITLE](attrib/iup_title.md), [VALUE](attrib/iup_value.md), [WID](attrib/iup_wid.md), [ZORDER](attrib/iup_zorder.md)

[Drag & Drop](attrib/iup_dragdrop.md), [Globals](attrib/iup_globals.md)

### Other

[CURSOR](attrib/iup_cursor.md), [FORMATTING](attrib/iup_formatting.md), [ICON](attrib/iup_icon.md), [KEY](attrib/iup_key.md), [MASK](attrib/iup_mask.md), [PARENTDIALOG](attrib/iup_parentdialog.md), [SCROLLBAR](attrib/iup_scrollbar.md), [SHRINK](attrib/iup_shrink.md)

[DX](attrib/iup_dx.md), [DY](attrib/iup_dy.md), [POSX](attrib/iup_posx.md), [POSY](attrib/iup_posy.md), [XMIN](attrib/iup_xmin.md), [XMAX](attrib/iup_xmax.md), [YMIN](attrib/iup_ymin.md), [YMAX](attrib/iup_ymax.md)

---

## Events and Callbacks

### Functions

[IupMainLoop](func/iup_mainloop.md), [IupMainLoopLevel](func/iup_mainlooplevel.md), [IupLoopStep](func/iup_loopstep.md), [IupExitLoop](func/iup_exitloop.md), [IupPostMessage](func/iup_postmessage.md), [IupFlush](func/iup_flush.md)

[IupGetCallback](func/iup_getcallback.md), [IupSetCallback](func/iup_setcallback.md), [IupSetCallbacks](func/iup_setcallbacks.md), [IupGetFunction](func/iup_getfunction.md), [IupSetFunction](func/iup_setfunction.md)

[IupRecordInput](func/iup_recordinput.md), [IupPlayInput](func/iup_playinput.md)

### Common

[IDLE_ACTION](call/iup_idle_action.md), [GLOBALCTRLFUNC_CB](call/iup_globalctrlfunc_cb.md), [ENTRY_POINT](call/iup_entry_point.md), [EXIT_CB](call/iup_exit_cb.md), [MAP_CB](call/iup_map_cb.md), [UNMAP_CB](call/iup_unmap_cb.md), [DESTROY_CB](call/iup_destroy_cb.md), [LDESTROY_CB](call/iup_ldestroy_cb.md), [GETFOCUS_CB](call/iup_getfocus_cb.md), [KILLFOCUS_CB](call/iup_killfocus_cb.md), [ENTERWINDOW_CB](call/iup_enterwindow_cb.md), [LEAVEWINDOW_CB](call/iup_leavewindow_cb.md), [K_ANY](call/iup_k_any.md), [HELP_CB](call/iup_help_cb.md), [ACTION](call/iup_action.md)

### Other

[BUTTON_CB](call/iup_button_cb.md), [CLOSE_CB](call/iup_close_cb.md), [DROPFILES_CB](call/iup_dropfiles_cb.md), [HIGHLIGHT_CB](call/iup_highlight_cb.md), [KEYPRESS_CB](call/iup_keypress_cb.md), [MENUCLOSE_CB](call/iup_menuclose_cb.md), [MOTION_CB](call/iup_motion_cb.md), [OPEN_CB](call/iup_open_cb.md), [RESIZE_CB](call/iup_resize_cb.md), [SCROLL_CB](call/iup_scroll_cb.md), [SHOW_CB](call/iup_show_cb.md), [WHEEL_CB](call/iup_wheel_cb.md)

---

## Dialogs

### Reference

[IupDialog](dlg/iup_dialog.md), [IupPopup](func/iup_popup.md), [IupShow](func/iup_show.md), [IupShowXY](func/iup_showxy.md), [IupHide](func/iup_hide.md)

### Predefined

| Dialog                                     | Description                                     |
|--------------------------------------------|-------------------------------------------------|
| [IupFileDlg](dlg/iup_filedlg.md)           | File or directory chooser                       |
| [IupMessageDlg](dlg/iup_messagedlg.md)     | Message dialog (info, warning, error, question) |
| [IupColorDlg](dlg/iup_colordlg.md)         | Color picker                                    |
| [IupFontDlg](dlg/iup_fontdlg.md)           | Font picker                                     |
| [IupProgressDlg](dlg/iup_progressdlg.md)   | Progress dialog for long-running operations     |
| [IupAlarm](dlg/iup_alarm.md)               | Modal message with up to three buttons          |
| [IupGetFile](dlg/iup_getfile.md)           | Convenience wrapper around IupFileDlg           |
| [IupGetColor](dlg/iup_getcolor.md)         | Convenience wrapper around IupColorDlg          |
| [IupGetParam](dlg/iup_getparam.md)         | Modal dialog capturing typed parameter values   |
| [IupGetText](dlg/iup_gettext.md)           | Modal multi-line text editor                    |
| [IupListDialog](dlg/iup_listdialog.md)     | Modal single or multi selection list            |
| [IupMessage](dlg/iup_message.md)           | Convenience wrapper around IupMessageDlg        |
| [IupMessageError](dlg/iup_messageerror.md) | IupMessage with DIALOGTYPE=ERROR                |
| [IupMessageAlarm](dlg/iup_messagealarm.md) | Question-style IupMessage with custom buttons   |

### Debug

| Dialog                                                     | Description                                           |
|------------------------------------------------------------|-------------------------------------------------------|
| [IupElementPropertiesDialog](dlg/iup_elementpropdialog.md) | Run-time editor for an element's attributes           |
| [IupGlobalsDialog](dlg/iup_globalsdialog.md)               | Browser for global attributes, functions, names       |
| [IupClassInfoDialog](dlg/iup_classinfodialog.md)           | Browser for registered classes, attributes, callbacks |

---

## Layout Composition

### Hierarchy

[IupAppend](func/iup_append.md), [IupDetach](func/iup_detach.md), [IupInsert](func/iup_insert.md), [IupReparent](func/iup_reparent.md)

[IupGetParent](func/iup_getparent.md), [IupGetChild](func/iup_getchild.md), [IupGetChildPos](func/iup_getchildpos.md), [IupGetChildCount](func/iup_getchildcount.md), [IupGetNextChild](func/iup_getnextchild.md), [IupGetBrother](func/iup_getbrother.md), [IupGetDialog](func/iup_getdialog.md), [IupGetDialogChild](func/iup_getdialogchild.md)

### Management

[IupRefresh](func/iup_refresh.md), [IupRefreshChildren](func/iup_refreshchildren.md)

---

## Controls

### Containers

| Element                                       | Description                                         |
|-----------------------------------------------|-----------------------------------------------------|
| [IupFill](elem/iup_fill.md)                   | Empty space that expands to fill the remaining area |
| [IupSpace](elem/iup_space.md)                 | Empty space with a fixed size                       |
| [IupCbox](elem/iup_cbox.md)                   | Absolute-positioned container (CX/CY coordinates)   |
| [IupGridBox](elem/iup_gridbox.md)             | Regular grid arranging children in lines or columns |
| [IupMultiBox](elem/iup_multibox.md)           | Irregular grid container                            |
| [IupHbox](elem/iup_hbox.md)                   | Arranges children horizontally                      |
| [IupVbox](elem/iup_vbox.md)                   | Arranges children vertically                        |
| [IupZbox](elem/iup_zbox.md)                   | Stack of layers; only one child visible at a time   |
| [IupRadio](elem/iup_radio.md)                 | Groups mutually exclusive toggles                   |
| [IupNormalizer](elem/iup_normalizer.md)       | Equalizes natural sizes across a list of controls   |
| [IupFrame](elem/iup_frame.md)                 | Bordered container with an optional title           |
| [IupPopover](elem/iup_popover.md)             | Floating container anchored to another element      |
| [IupTabs](elem/iup_tabs.md)                   | Tabbed container (notebook)                         |
| [IupBackgroundBox](elem/iup_backgroundbox.md) | Plain native container with no decorations          |
| [IupScrollBox](elem/iup_scrollbox.md)         | Scrollable container                                |
| [IupDetachBox](elem/iup_detachbox.md)         | Detachable container; drag opens it in a new dialog |
| [IupExpander](elem/iup_expander.md)           | Collapsible container with a header bar             |
| [IupSbox](elem/iup_sbox.md)                   | Resizable container in one direction                |
| [IupSplit](elem/iup_split.md)                 | Splits the client area into two resizable panes     |

### Standard

| Element                                       | Description                                                  |
|-----------------------------------------------|--------------------------------------------------------------|
| [IupAnimatedLabel](elem/iup_animatedlabel.md) | Label cycling through frames of an animation                 |
| [IupButton](elem/iup_button.md)               | Push button with text and/or image                           |
| [IupCalendar](elem/iup_calendar.md)           | Month calendar for date selection                            |
| [IupCanvas](elem/iup_canvas.md)               | Drawing area                                                 |
| [IupColorbar](elem/iup_colorbar.md)           | Palette of color cells with primary/secondary selection      |
| [IupColorBrowser](elem/iup_colorbrowser.md)   | HSI color picker on a cylindrical projection of the RGB cube |
| [IupDatePick](elem/iup_datepick.md)           | Date editor with a drop-down calendar                        |
| [IupDial](elem/iup_dial.md)                   | Rotary dial for angular values                               |
| [IupLabel](elem/iup_label.md)                 | Static text, image, or separator                             |
| [IupSeparator](elem/iup_separator.md)         | Visual separator line                                        |
| [IupLink](elem/iup_link.md)                   | Underlined clickable text (URL)                              |
| [IupList](elem/iup_list.md)                   | List, drop-down or combo box with optional edit field        |
| [IupProgressBar](elem/iup_progressbar.md)     | Native progress indicator                                    |
| [IupScrollbar](elem/iup_scrollbar.md)         | Standalone scrollbar control                                 |
| [IupSpin](elem/iup_spin.md)                   | Up/down arrow buttons; IupSpinBox wraps any element          |
| [IupTable](elem/iup_table.md)                 | Native table widget for tabular data                         |
| [IupText](elem/iup_text.md)                   | Single or multi-line editable text                           |
| [IupToggle](elem/iup_toggle.md)               | Two-state on/off button or switch                            |
| [IupTree](elem/iup_tree.md)                   | Hierarchical view of branches and leaves                     |
| [IupVal](elem/iup_val.md)                     | Slider/trackbar selecting a value in a range                 |

### Additional

| Element                                                                                                               | Description                                             |
|-----------------------------------------------------------------------------------------------------------------------|---------------------------------------------------------|
| [IupGLCanvas](ctrl/iup_glcanvas.md)                                                                                   | OpenGL drawing surface                                  |
| [IupGLBackgroundBox](ctrl/iup_glbackgroundbox.md)                                                                     | Background container with OpenGL enabled                |
| [IupPlot](ctrl/iup_plot.md)                                                                                           | 2D plot of one or more data sets                        |
| [IupWebBrowser](ctrl/iup_web.md)                                                                                      | Embedded web browser view                               |
| [IupMatrix](ctrl/iup_matrix.md) ([Attributes](ctrl/iup_matrix_attrib.md), [Callbacks](ctrl/iup_matrix_cb.md))         | Custom-drawn matrix of alphanumeric cells               |
| [IupMatrixEx](ctrl/iup_matrixex.md)                                                                                   | IupMatrix extension (clipboard, undo, find, sort, ...)  |
| [IupMatrixList](ctrl/iup_matrixlist.md)                                                                               | List built on IupMatrix; adds color and check boxes     |
| [IupCells](ctrl/iup_cells.md)                                                                                         | Application-driven cell grid for custom drawing         |
| [IupGauge](ctrl/iup_gauge.md)                                                                                         | Custom-drawn percent gauge                              |
| [IupDropButton](ctrl/iup_dropbutton.md)                                                                               | Button with a drop-down area hosting arbitrary children |
| [IupFlatButton](ctrl/iup_flatbutton.md)                                                                               | Custom-drawn button without native decorations          |
| [IupFlatLabel](ctrl/iup_flatlabel.md)                                                                                 | Custom-drawn label without native decorations           |
| [IupFlatToggle](ctrl/iup_flattoggle.md)                                                                               | Custom-drawn toggle without native decorations          |
| [IupFlatFrame](ctrl/iup_flatframe.md)                                                                                 | Custom-drawn frame with title                           |
| [IupFlatTabs](ctrl/iup_flattabs.md)                                                                                   | Custom-drawn IupTabs equivalent                         |
| [IupFlatScrollBox](ctrl/iup_flatscrollbox.md)                                                                         | IupScrollBox with custom-drawn scrollbars               |
| [IupFlatList](ctrl/iup_flatlist.md)                                                                                   | Custom-drawn IupList equivalent                         |
| [IupFlatTree](ctrl/iup_flattree.md) ([Attributes](ctrl/iup_flattree_attrib.md), [Callbacks](ctrl/iup_flattree_cb.md)) | Custom-drawn IupTree equivalent                         |
| [IupFlatVal](ctrl/iup_flatval.md)                                                                                     | Custom-drawn IupVal equivalent                          |

### Management

[IupMap](func/iup_map.md), [IupUnmap](func/iup_unmap.md)

[IupCreate](func/iup_create.md), [IupDestroy](func/iup_destroy.md)

[IupGetAllClasses](func/iup_getallclasses.md), [IupGetClassName](func/iup_getclassname.md), [IupGetClassType](func/iup_getclasstype.md), [IupClassMatch](func/iup_classmatch.md), [IupGetClassAttributes](func/iup_getclassattributes.md), [IupGetClassCallbacks](func/iup_getclasscallbacks.md), [IupSaveClassAttributes](func/iup_saveclassattributes.md), [IupCopyClassAttributes](func/iup_copyclassattributes.md), [IupSetClassDefaultAttribute](func/iup_setclassdefaultattribute.md)

[IupUpdate](func/iup_update.md), [IupRedraw](func/iup_redraw.md)

[IupConvertXYToPos](func/iup_convertxytopos.md)

---

## Resources

### Drawing and Images

| Element / Function                                    | Description                                                |
|-------------------------------------------------------|------------------------------------------------------------|
| [IupImage](elem/iup_image.md)                         | In-memory bitmap image (also IupImageRGB and IupImageRGBA) |
| [IupDraw](func/iup_draw.md)                           | Cross-driver immediate-mode drawing API on IupCanvas       |
| [IupImageGetHandle](func/iup_imagegethandle.md)       | Decodes raw image data into an IupImage                    |
| [IupImageSave](func/iup_imagesave.md)                 | Writes an IupImage to a file                               |
| [IupImageSaveToBuffer](func/iup_imagesavetobuffer.md) | Writes an IupImage to memory                               |

### Keyboard

[Keyboard Codes](attrib/iup_keyboard_codes.md), [IupNextField](func/iup_nextfield.md), [IupPreviousField](func/iup_previousfield.md), [IupGetFocus](func/iup_getfocus.md), [IupSetFocus](func/iup_setfocus.md)

### Menus

[IupMenu](elem/iup_menu.md), [IupMenuItem](elem/iup_menuitem.md), [IupMenuSeparator](elem/iup_menuseparator.md), [IupSubmenu](elem/iup_submenu.md)

### Handle Names

[IupSetHandle](func/iup_sethandle.md), [IupGetHandle](func/iup_gethandle.md), [IupGetName](func/iup_getname.md), [IupGetAllNames](func/iup_getallnames.md), [IupGetAllDialogs](func/iup_getalldialogs.md)

### Language

[IupSetLanguage](func/iup_setlanguage.md), [IupGetLanguage](func/iup_getlanguage.md), [IupSetLanguageString](func/iup_setlanguagestring.md), [IupGetLanguageString](func/iup_getlanguagestring.md), [IupSetLanguagePack](func/iup_setlanguagepack.md)

### Non-Visual Elements

| Element                               | Description                                            |
|---------------------------------------|--------------------------------------------------------|
| [IupClipboard](elem/iup_clipboard.md) | System clipboard access                                |
| [IupNotify](elem/iup_notify.md)       | Desktop / system notification                          |
| [IupTimer](elem/iup_timer.md)         | Periodic timer firing on the main thread               |
| [IupThread](elem/iup_thread.md)       | Background thread element                              |
| [IupTray](elem/iup_tray.md)           | System tray / status icon                              |
| [IupUser](elem/iup_user.md)           | Plain element holder for user data or external widgets |
| [IupParam](elem/iup_param.md)         | Single typed parameter for IupParamBox / IupGetParam   |
| [IupParamBox](elem/iup_parambox.md)   | Container composing a set of IupParam elements         |

### Configuration

[IupConfig](func/iup_config.md) creates a configuration database.

| Function                                                                                        | Description                                                               |
|-------------------------------------------------------------------------------------------------|---------------------------------------------------------------------------|
| [IupConfigLoad / IupConfigSave](func/iup_config.md#file-storage)                                | Load or save the configuration file (path derived from APP_NAME)          |
| [IupConfigCopy](func/iup_config.md#variables)                                                   | Copy variables between two databases, optionally excluding a group prefix |
| [IupConfigSetVariableStr / Int / Double (and *Id variants)](func/iup_config.md#variables)       | Store a typed variable under a group/key                                  |
| [IupConfigGetVariableStr / Int / Double (and *Id, *Def variants)](func/iup_config.md#variables) | Read a typed variable, with an optional default value                     |
| [IupConfigRecentInit / IupConfigRecentUpdate](func/iup_config.md#recent-file-menulist)          | Initialize and update a Recent Files menu or list                         |
| [IupConfigDialogShow / IupConfigDialogClosed](func/iup_config.md#dialog-position-and-size)      | Persist a dialog's last position and size across runs                     |

### Utilities

[IupExecute](func/iup_execute.md), [IupExecuteWait](func/iup_executewait.md), [IupHelp](func/iup_help.md), [IupLog](func/iup_log.md), [IupMapFont](func/iup_mapfont.md), [IupUnMapFont](func/iup_unmapfont.md), [IupStringCompare](func/iup_stringcompare.md)
