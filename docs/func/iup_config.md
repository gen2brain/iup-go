## IupConfig

A group of functions to load, store and save application configuration variables.
For example, the list of Recent Files, the last position and size of a dialog, last used parameters in dialogs, etc.

To use the functions in C/C++, you must include the "iup_config.h" header.

Each variable has a key name, a value and a group that it belongs to.
The file is based on a simple configuration file like ".ini" or ".cfg".
Each ground can have more than one key, but all keys in the same group must have different names.
Group and Key names cannot have a period ".". The file syntax is such as:

    [Group]
    Key=Value
    Key=Value
    ...

### Guide

First create a new configuration database using the **IupConfig** constructor.
To destroy it, use the **IupDestroy** function.
Then, when the application is started call **IupConfigLoad** and when the application is about to close, call **IupConfigSave**.

To retrieve variables, use the **IupConfigGetVariable*** functions and after they were changed store them using the **IupConfigSetVariable*** functions.

### Creation

    Ihandle* IupConfig(void);

Returns a new database where the variables will be stored.

### File Storage

    int IupConfigLoad(Ihandle* ih);

    int IupConfigSave(Ihandle* ih);

**ih**: Identifier of the configuration database

**Returns:** an error code. 0= no error; -1=error opening the file; -2=error accessing the file; -3=error during filename construction

Loads or saves the configuration file.

The filename (with path) can be set using a regular attribute called APP_FILENAME.

But the most interesting is to let the filename be dynamically constructed using the APP_NAME attribute.
In this case, APP_FILENAME must **not** be defined.
If APP_NAME is not set, the global [APPNAME](../attrib/iup_globals.md#appname) attribute is used as a fallback, and if that is also not set, the global [APPID](../attrib/iup_globals.md#appid) attribute is used.
The file name creation will depend on the system and on its usage.

There are two defined usages. First, for a **User Configuration File,** it will be stored on the user Home folder.
Second, as an **Application Configuration File,** it will be stored in the same folder of the executable.
The Home folder is always preferred, but you may need to just load a configuration file saved by the installation for instance.

The **User Configuration File** is the most common usage.
In UNIX, the filename will be "<HOME>/.<APP_NAME>", where "<HOME>" is replaced by the "HOME" environment variable contents, and <APP_NAME> replaced by the APP_NAME attribute value.
In Windows, the filename will be "<HOMEDRIVE><HOMEPATH>\\APP_NAME>.cfg", where HOMEDRIVE and HOMEPATH are also obtained from environment variables.
If the attribute APP_SYSTEMPATH is set to Yes, then it will use the system defined folder for application files, in Windows will be the same folder given by the environment variables with "Application Data\\ or "AppData\Roaming\\ appended.

The **Application Configuration File** is defined by setting the attribute APP_CONFIG to Yes (default is No).
In this case the attribute APP_PATH must also be set.
In UNIX, the filename will be "<APP_PATH>.<APP_NAME>",  and in Windows will be "<APP_PATH><APP_NAME>.cfg".
Notice that the attribute APP_PATH must contain a folder separator "/" at the end.

After the functions are called the attribute FILENAME is set reflecting the constructed filename.

So usually at startup, an application will do:

    Ihandle* config = IupConfig();
    IupSetAttribute(config, "APP_NAME", "MyAppName");
    IupConfigLoad(config);

### Variables

    void IupConfigSetVariableStr(Ihandle* ih, const char* group, const char* key, const char* value); 
    void IupConfigSetVariableStrId(Ihandle* ih, const char* group, const char* key, int id, const char* value);
    void IupConfigSetVariableInt(Ihandle* ih, const char* group, const char* key, int value);
    void IupConfigSetVariableIntId(Ihandle* ih, const char* group, const char* key, int id, int value);
    void IupConfigSetVariableDouble(Ihandle* ih, const char* group, const char* key, double value);
    void IupConfigSetVariableDoubleId(Ihandle* ih, const char* group, const char* key, int id, double value);

    const char* IupConfigGetVariableStr(Ihandle* ih, const char* group, const char* key); 
    const char* IupConfigGetVariableStrId(Ihandle* ih, const char* group, const char* key, int id);
    int    IupConfigGetVariableInt(Ihandle* ih, const char* group, const char* key);
    int    IupConfigGetVariableIntId(Ihandle* ih, const char* group, const char* key, int id);
    double IupConfigGetVariableDouble(Ihandle* ih, const char* group, const char* key);
    double IupConfigGetVariableDoubleId(Ihandle* ih, const char* group, const char* key, int id);

    const char* IupConfigGetVariableStrDef(Ihandle* ih, const char* group, const char* key, const char* def); 
    const char* IupConfigGetVariableStrIdDef(Ihandle* ih, const char* group, const char* key, int id, const char* def);
    int    IupConfigGetVariableIntDef(Ihandle* ih, const char* group, const char* key, int def);
    int    IupConfigGetVariableIntIdDef(Ihandle* ih, const char* group, const char* key, int id, int def);
    double IupConfigGetVariableDoubleDef(Ihandle* ih, const char* group, const char* key, double def);
    double IupConfigGetVariableDoubleIdDef(Ihandle* ih, const char* group, const char* key, int id, double def);

**ih**: Identifier of the configuration database\
**group**: group name of the variable\
**key**: key name of the variable\
**id**: used when the variable has a sequential number\
**value**: value of the variable\
**def**: default value of the variable

**Returns:** the variable value or NULL (or 0 for integer and double) if the variable is not set or does not exist.
When the variable may not exist, you can use the functions with **def** to use a default value.

These functions are very similar to the **IupSetAttribute** and **IupGetAttribute** functions.
Internally, the variables are stored as attributes using a "<GROUP>.<KEY>" combination, that's why a group and key names cannot have periods ".".

    void IupConfigCopy(Ihandle* ih1, Ihandle* ih2, const char* exclude_prefix);

Copy all the variables from config ih1 to ih2, but exclude groups that start with the given prefix (it can be NULL).

### Recent File Menu/List

    void IupConfigRecentInit(Ihandle* ih, Ihandle* menu_list, Icallback recent_cb, int max_recent);

    void IupConfigRecentUpdate(Ihandle* ih, const char* filename);

**ih**: Identifier of the configuration database\
**menu_list**: menu or list where the recent file items will be listed.
Sets the internal RECENTMENU or RECENTLIST attributes.\
**recent_cb**: callback that will be called when a recent file item is selected on the menu.
Sets the internal RECENT_CB callback.\
**max_recent**: the maximum number of recent file items. Sets the internal RECENTMAX attribute.\
**filename**: name of the file that where just saved or open

These functions store and manage a "Recent Files" menu or list for the application.
Call **IupConfigRecentInit** once to initialize the menu or the list.
Then every time a file is open or saved call **IupConfigRecentUpdate** so that the menu or list is updated.
The last file will be always on the top of the list.

Inside the RECENT_CB callback the RECENTFILENAME attribute contains the filename, but the ih handle is not the menu or list, it is the IupConfig handle.
But also inside the callback, the IupConfig will inherit attributes from the menu or list as if it was its parent.

The recent file list is stored by default in the group "Recent" in the configuration file.
To change the default set the internal attribute RECENTNAME, when set all other internal attributes will be stored with this value as a prefix.

### Dialog Position and Size

    void IupConfigDialogShow(Ihandle* ih, Ihandle* dialog, const char* name);

    void IupConfigDialogClosed(Ihandle* ih, Ihandle* dialog, const char* name);

**ih**: Identifier of the configuration database\
**dialog**: the dialog to manage the size and position\
**name**: a name for this dialog

These functions store and manage the position and size of a dialog.
So when the application is run again, the dialog can be shown at its last position and last size.
Use the function **IupConfigDialogShow** to show the dialog adjusting its size and position.
And use the function **IupConfigDialogClosed** to save the last dialog position and size when the dialog is about to be closed, usually inside the dialog CLOSE_CB callback.

**IupConfigDialogShow** does no adjustments if the dialog is already visible, just call **IupShow**.
If the dialog was closed maximized it will be shown maximized.
The default size, at the first time ever the dialog is shown, is maximized.
The dialog size is set only if RESIZE=Yes.

The position is saved in the variables "X" and "Y" of the given group name.
The size is saved in the variables "Width" and "Height" of the given group name.

If your dialog is resizable, and you want to avoid the last size usage because you changed the dialog layout, then reset the "Width" and "Height" variables before calling **IupConfigDialogShow**.

To avoid the dialog size to be maximized, set the variable "Maximized" to 0 before calling **IupConfigDialogShow**.

To use **IupConfigDialogShow** for a modal dialog, call it before calling **IupPopup** with IUP_CURRENT.

### See Also

[IupDestroy](iup_destroy.md), [IupSetAttribute](iup_setattribute.md), [IupGetAttribute](iup_getattribute.md)
