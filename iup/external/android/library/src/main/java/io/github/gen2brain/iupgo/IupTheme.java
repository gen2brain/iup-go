package io.github.gen2brain.iupgo;

import java.util.LinkedHashSet;
import java.util.Set;


/* theme-change bus; helpers register a Listener and IupActivity fans out on config change */
public final class IupTheme
{
    private IupTheme() {}

    public interface Listener { void onThemeChanged(); }

    private static final Set<Listener> sListeners = new LinkedHashSet<>();

    public static void register(Listener l)   { if (l != null) sListeners.add(l); }
    public static void unregister(Listener l) { if (l != null) sListeners.remove(l); }

    public static void notifyThemeChanged()
    {
        /* Snapshot first; listeners may register/unregister during dispatch. */
        for (Listener l : new java.util.ArrayList<>(sListeners))
        {
            try { l.onThemeChanged(); }
            catch (Throwable t) { android.util.Log.e("IupTheme", "listener failed", t); }
        }
    }
}
