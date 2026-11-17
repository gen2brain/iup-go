package io.github.gen2brain.iupgo;

import android.app.Activity;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.DialogInterface;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.Keep;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.view.ContextThemeWrapper;

import com.google.android.material.button.MaterialButton;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;


public final class IupMessageDialogHelper
{
    private IupMessageDialogHelper() {}

    public static final int RESP_CANCELED = 0;
    public static final int RESP_HELP     = -1;


    @Keep
    public static int showModal(Object parentHandle,
                                String title, String message, String dialogType,
                                String buttons, int buttonDefault, boolean hasHelp,
                                String okText, String cancelText, String yesText,
                                String noText, String retryText, String helpText,
                                String buttonStyle, String cornerStyle, String[] customTitles)
    {
        Activity activity = resolveActivity(parentHandle);
        if (activity == null) activity = IupApplication.getIupApplication().getCurrentActivity();
        Context ctx;
        if (activity instanceof androidx.appcompat.app.AppCompatActivity) ctx = activity;
        else if (activity != null) ctx = new ContextThemeWrapper(activity, R.style.AppTheme);
        else ctx = new ContextThemeWrapper(IupApplication.getIupApplication(), R.style.AppTheme);

        final int[] result = { RESP_CANCELED };
        final boolean[] done = { false };
        final AlertDialog[] shown = { null };

        DialogInterface.OnClickListener click = (d, which) -> {
            int r = RESP_CANCELED;
            if (which == DialogInterface.BUTTON_POSITIVE) r = 1;
            else if (which == DialogInterface.BUTTON_NEGATIVE) r = 2;
            else if (which == DialogInterface.BUTTON_NEUTRAL) r = 3;
            result[0] = r;
        };

        MaterialAlertDialogBuilder b = new MaterialAlertDialogBuilder(ctx);
        if (title != null && !title.isEmpty()) b.setTitle(title);
        if (message != null) b.setMessage(message);

        int iconRes = 0;
        if ("ERROR".equalsIgnoreCase(dialogType))
            iconRes = R.drawable.iup_ic_dialog_error;
        else if ("WARNING".equalsIgnoreCase(dialogType))
            iconRes = R.drawable.iup_ic_dialog_warning;
        else if ("INFORMATION".equalsIgnoreCase(dialogType))
            iconRes = R.drawable.iup_ic_dialog_info;
        else if ("QUESTION".equalsIgnoreCase(dialogType))
            iconRes = R.drawable.iup_ic_dialog_question;
        if (iconRes != 0)
        {
            /* Drop theme iconTint so semantic colors (red/amber/blue) survive. */
            android.graphics.drawable.Drawable icon = androidx.core.content.ContextCompat.getDrawable(ctx, iconRes);
            if (icon != null)
            {
                icon = icon.mutate();
                icon.setTintList(null);
                b.setIcon(icon);
            }
        }

        /* IUP 1/2/3 maps to Positive/Negative/Neutral. HELP only in 1-button mode. */
        if (customTitles != null && customTitles.length > 3)
        {
            b.setView(stackedButtons(ctx, customTitles, buttonDefault, buttonStyle, cornerStyle, which -> {
                result[0] = which;
                if (shown[0] != null) shown[0].dismiss();
            }));
        }
        else if (customTitles != null && customTitles.length > 0)
        {
            b.setPositiveButton(customTitles[0], click);
            if (customTitles.length > 1)
                b.setNegativeButton(customTitles[1], click);
            if (customTitles.length > 2)
                b.setNeutralButton(customTitles[2], click);
            else if (customTitles.length == 1 && hasHelp)
                b.setNeutralButton(helpText, (d, which) -> result[0] = RESP_HELP);
        }
        else if ("OKCANCEL".equalsIgnoreCase(buttons))
        {
            b.setPositiveButton(okText, click);
            b.setNegativeButton(cancelText, click);
        }
        else if ("RETRYCANCEL".equalsIgnoreCase(buttons))
        {
            b.setPositiveButton(retryText, click);
            b.setNegativeButton(cancelText, click);
        }
        else if ("YESNO".equalsIgnoreCase(buttons))
        {
            b.setPositiveButton(yesText, click);
            b.setNegativeButton(noText, click);
        }
        else if ("YESNOCANCEL".equalsIgnoreCase(buttons))
        {
            b.setPositiveButton(yesText, click);
            b.setNegativeButton(noText, click);
            b.setNeutralButton(cancelText, click);
        }
        else
        {
            b.setPositiveButton(okText, click);
            if (hasHelp)
                b.setNeutralButton(helpText, (d, which) -> result[0] = RESP_HELP);
        }

        b.setCancelable(false);

        AlertDialog dialog = b.create();
        shown[0] = dialog;
        dialog.setOnDismissListener(d -> done[0] = true);
        dialog.show();

        if (customTitles == null || customTitles.length <= 3)
            styleActionButtons(dialog, buttonDefault, buttonStyle, cornerStyle);

        IupCommon.pumpUntilDone(done);

        return result[0];
    }


    private static View stackedButtons(Context ctx, String[] titles, int buttonDefault, String buttonStyle, String cornerStyle,
                                       java.util.function.IntConsumer onClick)
    {
        float density = ctx.getResources().getDisplayMetrics().density;
        int side = Math.round(24 * density);
        int gap = Math.round(8 * density);

        android.widget.LinearLayout box = new android.widget.LinearLayout(ctx);
        box.setOrientation(android.widget.LinearLayout.VERTICAL);
        box.setPadding(side, gap, side, 0);

        for (int i = 0; i < titles.length; i++)
        {
            final int which = i + 1;
            MaterialButton button = new MaterialButton(ctx);
            button.setText(titles[i]);
            button.setInsetTop(0);
            button.setInsetBottom(0);
            if (buttonStyle != null && !buttonStyle.isEmpty())
                IupButtonHelper.applyButtonStyle(button, buttonStyle.toUpperCase(java.util.Locale.ROOT));
            else
                IupButtonHelper.applyShowAsDefault(button, which == buttonDefault);
            IupButtonHelper.setCornerStyle(button, cornerStyle);
            button.setOnClickListener(v -> onClick.accept(which));

            android.widget.LinearLayout.LayoutParams lp = new android.widget.LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
            lp.bottomMargin = gap;
            box.addView(button, lp);

            if (which == buttonDefault) button.requestFocus();
        }

        return box;
    }

    private static Activity resolveActivity(Object handle)
    {
        if (handle instanceof Activity) return (Activity) handle;
        if (handle instanceof View)
        {
            Context c = ((View) handle).getContext();
            while (c instanceof ContextWrapper)
            {
                if (c instanceof Activity) return (Activity) c;
                c = ((ContextWrapper) c).getBaseContext();
            }
        }
        return null;
    }


    /* swap AlertDialog's borderless AppCompatButtons for the standard MaterialButton look */
    private static void styleActionButtons(AlertDialog dialog, int buttonDefault, String buttonStyle, String cornerStyle)
    {
        int defaultWhich;
        if (buttonDefault == 3) defaultWhich = DialogInterface.BUTTON_NEUTRAL;
        else if (buttonDefault == 2) defaultWhich = DialogInterface.BUTTON_NEGATIVE;
        else defaultWhich = DialogInterface.BUTTON_POSITIVE;

        replaceWithMaterial(dialog, DialogInterface.BUTTON_POSITIVE, defaultWhich == DialogInterface.BUTTON_POSITIVE, buttonStyle, cornerStyle);
        replaceWithMaterial(dialog, DialogInterface.BUTTON_NEGATIVE, defaultWhich == DialogInterface.BUTTON_NEGATIVE, buttonStyle, cornerStyle);
        replaceWithMaterial(dialog, DialogInterface.BUTTON_NEUTRAL,  defaultWhich == DialogInterface.BUTTON_NEUTRAL, buttonStyle, cornerStyle);
    }

    private static void replaceWithMaterial(AlertDialog dialog, int which, boolean isDefault, String buttonStyle, String cornerStyle)
    {
        final android.widget.Button original = dialog.getButton(which);
        /* Unused slots are inflated GONE; skip or they paint a ghost button. */
        if (original == null || original.getVisibility() != View.VISIBLE) return;
        ViewGroup parent = (ViewGroup) original.getParent();
        if (parent == null) return;

        MaterialButton replacement = new MaterialButton(original.getContext());
        replacement.setText(original.getText());
        replacement.setEnabled(original.isEnabled());
        replacement.setOnClickListener(v -> original.performClick());
        /* Drop the 6dp Material insets, otherwise ButtonBarLayout.allowStacking pushes the row vertical. */
        replacement.setInsetTop(0);
        replacement.setInsetBottom(0);
        replacement.setPadding(original.getPaddingLeft(), original.getPaddingTop(),
                               original.getPaddingRight(), original.getPaddingBottom());

        if (buttonStyle != null && !buttonStyle.isEmpty())
            IupButtonHelper.applyButtonStyle(replacement, buttonStyle.toUpperCase(java.util.Locale.ROOT));
        else
            IupButtonHelper.applyShowAsDefault(replacement, isDefault);

        IupButtonHelper.setCornerStyle(replacement, cornerStyle);

        int idx = parent.indexOfChild(original);
        ViewGroup.LayoutParams lp = original.getLayoutParams();
        parent.removeView(original);
        parent.addView(replacement, idx, lp);

        if (isDefault) replacement.requestFocus();
    }
}
