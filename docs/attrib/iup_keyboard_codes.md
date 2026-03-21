### **Keyboard Codes**

The table below shows the IUP codification of common keys in a keyboard.
Each key is represented by an integer value, defined in the "iupkey.h" file header, which should be included in the application to use the key definitions.
These keys are used in K_ANY and KEYPRESS_CB callbacks to inform the key that was pressed on the keyboard.

From the definition in the table, change the prefix to K_s\*, K_c\*, K_m\* and K_y\* to add the respective modifier (Shift, Control, Alt and Sys).
Sys in Windows is the Windows key and in Mac is the Apple key.
Check the "iupkey.h" file header for all the definitions.

IUP provides definitions only for common control keys and ASCii characters.
Other key combinations are accessed using the macros described below.
Also, the global attribute "MODKEYSTATE" can be used to detect the combination of two or more modifiers.
Notice that some key combinations are never available because they are restricted by the system.
Notice that all of this does not affect the **IupText** and **IupMultiline** text input.

The **iup_isprint(key)** macro informs if a key can be directly used as a printable character.
The **iup_isXkey(key)** macro informs if a given key is an extended code.
The **iup_isShiftXkey(key)** macro informs if a given key is an extended code using the Shift modifier, the **iup_isCtrlXkey(key)** macro for the Ctrl modifier, the **iup_isAltXkey(key)** macro for the Alt modifier, and the **iup_isSysXkey(key)** macro for the Sys modifier.
To obtain a key code for a generic combination you can start with the base key from the table and combine it repeated times using the macros **iup_XkeyShift(key)**, **iup_XkeyCtrl(key)**, **iup_XkeyAlt(key)** and **iup_XkeySys(key)**.

Note: GTK in Windows does not generate the Win modifier key, the K_Print and the K_Pause keys (up to GTK version 2.8.18).

|          Key | Code / Callback |
|-------------:|-----------------|
|        Space | K_SP            |
|            ! | K_exclam        |
|            " | K_quotedbl      |
|           \# | K_numbersign    |
|           \$ | K_dollar        |
|            % | K_percent       |
|            & | K_ampersand     |
|            ' | K_apostrophe    |
|            ( | K_parentleft    |
|            ) | K_parentright   |
|           \* | K_asterisk      |
|           \+ | K_plus          |
|            , | K_comma         |
|           \- | K_minus         |
|            . | K_period        |
|            / | K_slash         |
|            0 | K_0             |
|            1 | K_1             |
|            2 | K_2             |
|            3 | K_3             |
|            4 | K_4             |
|            5 | K_5             |
|            6 | K_6             |
|            7 | K_7             |
|            8 | K_8             |
|            9 | K_9             |
|            : | K_colon         |
|            ; | K_semicolon     |
|           \< | K_less          |
|            = | K_equal         |
|           \> | K_greater       |
|            ? | K_question      |
|            @ | K_at            |
|            A | K_A             |
|            B | K_B             |
|            C | K_C             |
|            D | K_D             |
|            E | K_E             |
|            F | K_F             |
|            G | K_G             |
|            H | K_H             |
|            I | K_I             |
|            J | K_J             |
|            K | K_K             |
|            L | K_L             |
|            M | K_M             |
|            N | K_N             |
|            O | K_O             |
|            P | K_P             |
|            Q | K_Q             |
|            R | K_R             |
|            S | K_S             |
|            T | K_T             |
|            U | K_U             |
|            V | K_V             |
|            W | K_W             |
|            X | K_X             |
|            Y | K_Y             |
|            Z | K_Z             |
|           \[ | K_bracketleft   |
|           \\ | K_backslash     |
|           \] | K_bracketright  |
|            ^ | K_circum        |
|           \_ | K_underscore    |
|           \` | K_grave         |
|            a | K_a             |
|            b | K_b             |
|            c | K_c             |
|            d | K_d             |
|            e | K_e             |
|            f | K_f             |
|            g | K_g             |
|            h | K_h             |
|            i | K_i             |
|            j | K_j             |
|            k | K_k             |
|            l | K_l             |
|            m | K_m             |
|            n | K_n             |
|            o | K_o             |
|            p | K_p             |
|            q | K_q             |
|            r | K_r             |
|            s | K_s             |
|            t | K_t             |
|            u | K_u             |
|            v | K_v             |
|            w | K_w             |
|            x | K_x             |
|            y | K_y             |
|            z | K_z             |
|            { | K_braceleft     |
|           \| | K_bar           |
|            } | K_braceright    |
|            ~ | K_tilde         |
|          Esc | K_ESC           |
|        Enter | K_CR            |
|    BackSpace | K_BS            |
|       Insert | K_INS           |
|          Del | K_DEL           |
|          Tab | K_TAB           |
|         Home | K_HOME          |
|     Up Arrow | K_UP            |
|         PgUp | K_PGUP          |
|   Left Arrow | K_LEFT          |
|       Middle | K_MIDDLE        |
|  Right Arrow | K_RIGHT         |
|          End | K_END           |
|   Down Arrow | K_DOWN          |
|         PgDn | K_PGDN          |
|        Pause | K_PAUSE         |
| Print Screen | K_Print         |
| Context Menu | K_Menu          |
|            ´ | K_acute         |
|            ç | K_ccedilla      |
|            ¨ | K_diaeresis     |
|           F1 | K_F1            |
|           F2 | K_F2            |
|           F3 | K_F3            |
|           F4 | K_F4            |
|           F5 | K_F5            |
|           F6 | K_F6            |
|           F7 | K_F7            |
|           F8 | K_F8            |
|           F9 | K_F9            |
|          F10 | K_F10           |
|          F11 | K_F11           |
|          F12 | K_F12           |
|   Left Shift | K_LSHIFT        |
|  Right Shift | K_RSHIFT        |
|    Left Ctrl | K_LCTRL         |
|   Right Ctrl | K_RCTRL         |
|     Left Alt | K_LALT          |
|    Right Alt | K_RALT          |
|  Scroll Lock | K_SCROLL        |
|     Num Lock | K_NUM           |
|    Caps Lock | K_CAPS          |
