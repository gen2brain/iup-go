package io.github.gen2brain.iupgo;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;


public final class IupNotifyReceiver extends BroadcastReceiver
{
    @Override
    public void onReceive(Context context, Intent intent)
    {
        if (intent == null) return;
        long ih = intent.getLongExtra(IupNotifyHelper.EXTRA_IHANDLE, 0);
        if (ih == 0) return;

        String act = intent.getAction();
        if (IupNotifyHelper.ACTION_NOTIFY.equals(act))
        {
            int actionIndex = intent.getIntExtra(IupNotifyHelper.EXTRA_ACTION, 0);
            IupNotifyHelper.dispatchAction(ih, actionIndex);
            int notifyId = intent.getIntExtra(IupNotifyHelper.EXTRA_NOTIFY_ID, 0);
            if (actionIndex != 0 && notifyId != 0)
                IupNotifyHelper.close(notifyId);
        }
        else if (IupNotifyHelper.ACTION_DELETE.equals(act))
            IupNotifyHelper.dispatchClose(ih, 2 /* dismissed-by-user */);
    }
}
