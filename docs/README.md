# IUP Documentation

API reference documentation for IUP (Portable User Interface).

## Guides

[Attributes](attrib.md) | [Events and Callbacks](call.md) | [Layout](layout.md) | [Keyboard](keyboard.md)

---

## System

[IupOpen](func/iup_open.md), [IupClose](func/iup_close.md), [IupVersion](func/iup_version.md)

---

## Attributes

### Functions

[IupSetAttribute](func/iup_setattribute.md), [IupSetAttributes](func/iup_setattributes.md), [IupResetAttribute](func/iup_resetattribute.md), [IupSetAtt](func/iup_setatt.md), [IupSetAttributeHandle](func/iup_setattributehandle.md)

[IupGetAttribute](func/iup_getattribute.md), [IupGetAllAttributes](func/iup_getallattributes.md), [IupGetAttributes](func/iup_getattributes.md), [IupCopyAttributes](func/iup_copyattributes.md), [IupGetAttributeHandle](func/iup_getattributehandle.md)

[IupSetGlobal](func/iup_setglobal.md), [IupGetGlobal](func/iup_getglobal.md)

[IupStringCompare](func/iup_stringcompare.md)

### Common

[ACTIVE](attrib/iup_active.md), [BGCOLOR](attrib/iup_bgcolor.md), [FGCOLOR](attrib/iup_fgcolor.md), [FONT](attrib/iup_font.md), [THEME](attrib/iup_theme.md), [VISIBLE](attrib/iup_visible.md), [CLIENTSIZE](attrib/iup_clientsize.md), [CLIENTOFFSET](attrib/iup_clientoffset.md), [EXPAND](attrib/iup_expand.md), [MAXSIZE](attrib/iup_maxsize.md), [MINSIZE](attrib/iup_minsize.md), [NATURALSIZE](attrib/iup_naturalsize.md), [RASTERSIZE](attrib/iup_rastersize.md), [SIZE](attrib/iup_size.md), [FLOATING](attrib/iup_floating.md), [POSITION](attrib/iup_position.md), [SCREENPOSITION](attrib/iup_screenposition.md), [NAME](attrib/iup_name.md), [TIP](attrib/iup_tip.md), [TITLE](attrib/iup_title.md), [VALUE](attrib/iup_value.md), [WID](attrib/iup_wid.md), [ZORDER](attrib/iup_zorder.md)

[Drag & Drop](attrib/iup_dragdrop.md)

[Globals](attrib/iup_globals.md)

### Other

[CURSOR](attrib/iup_cursor.md), [FLATSCROLLBAR](attrib/iup_flatscrollbar.md), [FORMATTING](attrib/iup_formatting.md), [ICON](attrib/iup_icon.md), [KEY](attrib/iup_key.md), [MASK](attrib/iup_mask.md), [PARENTDIALOG](attrib/iup_parentdialog.md), [SCROLLBAR](attrib/iup_scrollbar.md), [SHRINK](attrib/iup_shrink.md)

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

[IupFileDlg](dlg/iup_filedlg.md), [IupMessageDlg](dlg/iup_messagedlg.md), [IupColorDlg](dlg/iup_colordlg.md), [IupFontDlg](dlg/iup_fontdlg.md), [IupProgressDlg](dlg/iup_progressdlg.md), [IupAlarm](dlg/iup_alarm.md), [IupGetFile](dlg/iup_getfile.md), [IupGetColor](dlg/iup_getcolor.md), [IupGetParam](dlg/iup_getparam.md), [IupGetText](dlg/iup_gettext.md), [IupListDialog](dlg/iup_listdialog.md), [IupMessage](dlg/iup_message.md), [IupMessageError](dlg/iup_messageerror.md), [IupMessageAlarm](dlg/iup_messagealarm.md)

### Debug

[IupElementPropertiesDialog](dlg/iup_elementpropdialog.md), [IupGlobalsDialog](dlg/iup_globalsdialog.md), [IupClassInfoDialog](dlg/iup_classinfodialog.md)

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

[IupFill](elem/iup_fill.md), [IupSpace](elem/iup_space.md), [IupCbox](elem/iup_cbox.md), [IupGridBox](elem/iup_gridbox.md), [IupMultiBox](elem/iup_multibox.md), [IupHbox](elem/iup_hbox.md), [IupVbox](elem/iup_vbox.md), [IupZbox](elem/iup_zbox.md), [IupRadio](elem/iup_radio.md), [IupNormalizer](elem/iup_normalizer.md), [IupFrame](elem/iup_frame.md), [IupPopover](elem/iup_popover.md), [IupTabs](elem/iup_tabs.md), [IupBackgroundBox](elem/iup_backgroundbox.md), [IupScrollBox](elem/iup_scrollbox.md), [IupDetachBox](elem/iup_detachbox.md), [IupExpander](elem/iup_expander.md), [IupSbox](elem/iup_sbox.md), [IupSplit](elem/iup_split.md)

### Standard

[IupAnimatedLabel](elem/iup_animatedlabel.md), [IupButton](elem/iup_button.md), [IupCalendar](elem/iup_calendar.md), [IupCanvas](elem/iup_canvas.md), [IupColorbar](elem/iup_colorbar.md), [IupColorBrowser](elem/iup_colorbrowser.md), [IupDatePick](elem/iup_datepick.md), [IupDial](elem/iup_dial.md), [IupGauge](elem/iup_gauge.md), [IupLabel](elem/iup_label.md), [IupSeparator](elem/iup_separator.md), [IupLink](elem/iup_link.md), [IupList](elem/iup_list.md), [IupProgressBar](elem/iup_progressbar.md), [IupScrollbar](elem/iup_scrollbar.md), [IupSpin](elem/iup_spin.md), [IupTable](elem/iup_table.md), [IupText](elem/iup_text.md), [IupToggle](elem/iup_toggle.md), [IupTree](elem/iup_tree.md), [IupVal](elem/iup_val.md)

### Additional

[IupGLCanvas](ctrl/iup_glcanvas.md), [IupGLBackgroundBox](ctrl/iup_glbackgroundbox.md), [IupPlot](ctrl/iup_plot.md), [IupWebBrowser](ctrl/iup_web.md)

[IupMatrix](ctrl/iup_matrix.md) ([Attributes](ctrl/iup_matrix_attrib.md), [Callbacks](ctrl/iup_matrix_cb.md)), [IupMatrixEx](ctrl/iup_matrixex.md), [IupMatrixList](ctrl/iup_matrixlist.md), [IupCells](ctrl/iup_cells.md)

[IupDropButton](ctrl/iup_dropbutton.md), [IupFlatButton](ctrl/iup_flatbutton.md), [IupFlatLabel](ctrl/iup_flatlabel.md), [IupFlatToggle](ctrl/iup_flattoggle.md), [IupFlatFrame](ctrl/iup_flatframe.md), [IupFlatTabs](ctrl/iup_flattabs.md), [IupFlatScrollBox](ctrl/iup_flatscrollbox.md), [IupFlatList](ctrl/iup_flatlist.md), [IupFlatTree](ctrl/iup_flattree.md) ([Attributes](ctrl/iup_flattree_attrib.md), [Callbacks](ctrl/iup_flattree_cb.md)), [IupFlatVal](ctrl/iup_flatval.md)

### Management

[IupMap](func/iup_map.md), [IupUnmap](func/iup_unmap.md)

[IupCreate](func/iup_create.md), [IupDestroy](func/iup_destroy.md)

[IupGetAllClasses](func/iup_getallclasses.md), [IupGetClassName](func/iup_getclassname.md), [IupGetClassType](func/iup_getclasstype.md), [IupClassMatch](func/iup_classmatch.md), [IupGetClassAttributes](func/iup_getclassattributes.md), [IupGetClassCallbacks](func/iup_getclasscallbacks.md), [IupSaveClassAttributes](func/iup_saveclassattributes.md), [IupCopyClassAttributes](func/iup_copyclassattributes.md), [IupSetClassDefaultAttribute](func/iup_setclassdefaultattribute.md)

[IupUpdate](func/iup_update.md), [IupRedraw](func/iup_redraw.md)

[IupConvertXYToPos](func/iup_convertxytopos.md)

---

## Resources

[IupDraw](func/iup_draw.md), [IupImage](elem/iup_image.md), [IupImageGetHandle](func/iup_imagegethandle.md), [IupImageSave](func/iup_imagesave.md), [IupImageSaveToBuffer](func/iup_imagesavetobuffer.md)

### Keyboard

[Keyboard Codes](attrib/iup_keyboard_codes.md), [IupNextField](func/iup_nextfield.md), [IupPreviousField](func/iup_previousfield.md), [IupGetFocus](func/iup_getfocus.md), [IupSetFocus](func/iup_setfocus.md)

### Menus

[IupMenuItem](elem/iup_menuitem.md), [IupMenu](elem/iup_menu.md), [IupMenuSeparator](elem/iup_menuseparator.md), [IupSubmenu](elem/iup_submenu.md)

### Handle Names

[IupSetHandle](func/iup_sethandle.md), [IupGetHandle](func/iup_gethandle.md), [IupGetName](func/iup_getname.md), [IupGetAllNames](func/iup_getallnames.md), [IupGetAllDialogs](func/iup_getalldialogs.md)

### String Names

[IupSetLanguage](func/iup_setlanguage.md), [IupGetLanguage](func/iup_getlanguage.md), [IupSetLanguageString](func/iup_setlanguagestring.md), [IupGetLanguageString](func/iup_getlanguagestring.md), [IupSetLanguagePack](func/iup_setlanguagepack.md)

### Other

[IupClipboard](elem/iup_clipboard.md), [IupNotify](elem/iup_notify.md), [IupTimer](elem/iup_timer.md), [IupThread](elem/iup_thread.md), [IupTray](elem/iup_tray.md), [IupUser](elem/iup_user.md), [IupParam](elem/iup_param.md), [IupParamBox](elem/iup_parambox.md)

[IupConfig](func/iup_config.md), [IupExecute](func/iup_execute.md), [IupExecuteWait](func/iup_executewait.md), [IupHelp](func/iup_help.md), [IupLog](func/iup_log.md), [IupMapFont](func/iup_mapfont.md), [IupUnMapFont](func/iup_unmapfont.md)
