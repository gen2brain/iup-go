/** \file
 * \brief Qt Quick Driver Core - Initialization and Setup
 *
 * See Copyright Notice in "iup.h"
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <clocale>

#if defined(__linux__)
#include <unistd.h>
#endif

#include <QGuiApplication>
#include <QFontDatabase>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlProperty>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQuickStyle>
#include <QQuickImageProvider>
#include <QPalette>
#include <QPixmap>
#include <QStyleHints>
#include <QHash>
#include <QMetaMethod>
#include <QCoreApplication>
#include <QEventLoop>

#include <QtGui/qguiapplication_platform.h>

#if !defined(Q_OS_WIN) && !defined(Q_OS_MACOS) && !defined(Q_OS_HAIKU)
  #define IUP_QML_HAS_WAYLAND_APP 1
#endif

extern "C" {
#include "iup.h"
#include "iup_str.h"
#include "iup_drv.h"
#include "iup_object.h"
#include "iup_globalattrib.h"
#include "iup_dlglist.h"
}

#include "iupqml_drv.h"

#ifdef IUPX11_USE_DLOPEN
#include "iupunix_x11.h"
#elif !defined(_WIN32) && !defined(__APPLE__) && !defined(__HAIKU__)
#include <X11/Xlib.h>
#endif


static QGuiApplication* qml_application = nullptr;
static int qml_application_owned = 0;
static QQmlEngine* qml_engine = nullptr;
static QHash<QByteArray, QQmlComponent*>* qml_components = nullptr;
static QHash<QByteArray, QQuickItem*>* qml_templates = nullptr;
static QQuickWindow* qml_template_window = nullptr;

/****************************************************************************
 * Image Provider
 ****************************************************************************/

class IupQmlImageProvider : public QQuickImageProvider
{
public:
  QHash<QString, QPixmap*> pixmaps;
  QHash<QPixmap*, QString> ids;
  int counter;

  IupQmlImageProvider() : QQuickImageProvider(QQuickImageProvider::Pixmap), counter(0) {}

  QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) override
  {
    QPixmap* pixmap = pixmaps.value(id, nullptr);
    if (!pixmap)
    {
      if (size) *size = QSize(0, 0);
      return {};
    }
    if (size) *size = pixmap->size();

    int w = requestedSize.width(), h = requestedSize.height();
    if ((w <= 0 && h <= 0) || (w == pixmap->width() && h == pixmap->height()))
      return *pixmap;
    if (w <= 0)
      return pixmap->scaledToHeight(h, Qt::SmoothTransformation);
    if (h <= 0)
      return pixmap->scaledToWidth(w, Qt::SmoothTransformation);
    return pixmap->scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation);
  }
};

static IupQmlImageProvider* qml_image_provider = nullptr;

IUP_DRV_API QString iupqmlImageUrl(QPixmap* pixmap)
{
  if (!pixmap || !qml_image_provider)
    return {};

  QString id = qml_image_provider->ids.value(pixmap);
  if (id.isEmpty())
  {
    qml_image_provider->counter++;
    id = QString::number(qml_image_provider->counter);
    qml_image_provider->ids.insert(pixmap, id);
    qml_image_provider->pixmaps.insert(id, pixmap);
  }

  return QString("image://iup/") + id;
}

IUP_DRV_API void iupqmlImageRelease(QPixmap* pixmap)
{
  if (!pixmap || !qml_image_provider)
    return;

  QString id = qml_image_provider->ids.take(pixmap);
  if (!id.isEmpty())
    qml_image_provider->pixmaps.remove(id);
}

/****************************************************************************
 * Application and Engine
 ****************************************************************************/

IUP_DRV_API QGuiApplication* iupqmlGetApplication()
{
  return qml_application;
}

IUP_DRV_API QQmlEngine* iupqmlGetEngine()
{
  if (!qml_engine)
  {
    qml_engine = new QQmlEngine();
    qml_image_provider = new IupQmlImageProvider();
    qml_engine->addImageProvider("iup", qml_image_provider);
    qml_components = new QHash<QByteArray, QQmlComponent*>();
    qml_templates = new QHash<QByteArray, QQuickItem*>();
  }
  return qml_engine;
}

/****************************************************************************
 * Component Creation
 ****************************************************************************/

IUP_DRV_API int iupqmlStyleLocked()
{
  return (qml_components && !qml_components->isEmpty()) ? 1 : 0;
}

static void qmlStyleLocked()
{
  static const struct { const char* name; QPalette::ColorRole role; } roles[] = {
    {"window", QPalette::Window}, {"windowText", QPalette::WindowText}, {"base", QPalette::Base},
    {"text", QPalette::Text}, {"highlight", QPalette::Highlight}, {"link", QPalette::Link},
    {"accent", QPalette::Accent},
  };

  if (iupStrEqualNoCase(IupGetGlobal("APPEARANCE"), "SYSTEM"))
  {
    QPalette palette = QGuiApplication::palette();
    QQuickItem* control = iupqmlCreateItem(IUPQML_IMPORTS "Control { }");
    QObject* style = control ? qvariant_cast<QObject*>(control->property("palette")) : nullptr;
    if (style)
    {
      for (const auto& r : roles)
      {
        auto color = style->property(r.name).value<QColor>();
        if (color.isValid())
          palette.setColor(r.role, color);
      }
    }
    delete control;

    if (palette != QGuiApplication::palette())
    {
      QGuiApplication::setPalette(palette);
      iupqmlUpdateSystemPalette();
    }
  }

  iupqmlSetGlobalColors();
}

static QQmlComponent* qmlGetComponent(const char* qml)
{
  QQmlEngine* engine = iupqmlGetEngine();
  QByteArray source(qml);

  QQmlComponent* component = qml_components->value(source, nullptr);
  if (component)
    return component;

  component = new QQmlComponent(engine);
  component->setData(source, QUrl("iup://inline"));

  while (component->isLoading())
    QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents, 10);

  if (component->isError())
  {
    fprintf(stderr, "IUP QML: %s\n", component->errorString().toUtf8().constData());
    delete component;
    return nullptr;
  }

  qml_components->insert(source, component);
  if (qml_components->size() == 1)
    qmlStyleLocked();
  return component;
}

IUP_DRV_API QObject* iupqmlCreateObject(const char* qml)
{
  QQmlComponent* component = qmlGetComponent(qml);
  if (!component)
    return nullptr;

  QObject* obj = component->create();
  if (!obj)
  {
    fprintf(stderr, "IUP QML: %s\n", component->errorString().toUtf8().constData());
    return nullptr;
  }

  QQmlEngine::setObjectOwnership(obj, QQmlEngine::CppOwnership);
  return obj;
}

IUP_DRV_API QQuickItem* iupqmlCreateItem(const char* qml)
{
  QObject* obj = iupqmlCreateObject(qml);
  if (!obj)
    return nullptr;

  QQuickItem* item = qobject_cast<QQuickItem*>(obj);
  if (!item)
  {
    delete obj;
    return nullptr;
  }

  return item;
}

static void qmlPolishTree(QQuickItem* item)
{
  for (QQuickItem* child : item->childItems())
    qmlPolishTree(child);
  item->ensurePolished();
}

IUP_DRV_API void iupqmlMeasureItem(QQuickItem* item)
{
  if (!qml_template_window)
    qml_template_window = new QQuickWindow();
  item->setParentItem(qml_template_window->contentItem());
  qmlPolishTree(item);
}

IUP_DRV_API QQuickItem* iupqmlTemplateItem(const char* qml)
{
  iupqmlGetEngine();

  QByteArray source(qml);
  QQuickItem* item = qml_templates->value(source, nullptr);
  if (item)
    return item;

  item = iupqmlCreateItem(qml);
  if (item)
  {
    iupqmlMeasureItem(item);
    qml_templates->insert(source, item);
  }
  return item;
}

/****************************************************************************
 * Signals
 ****************************************************************************/

class IupQmlSlot : public QObject
{
public:
  std::function<void(void**)> func;

  IupQmlSlot(QObject* parent, std::function<void(void**)> f) : QObject(parent), func(std::move(f)) {}

  int qt_metacall(QMetaObject::Call call, int id, void** args) override
  {
    id = QObject::qt_metacall(call, id, args);
    if (id < 0)
      return id;
    if (call == QMetaObject::InvokeMetaMethod && id == 0)
      func(args);
    return id - 1;
  }
};

IUP_DRV_API QObject* iupqmlConnect(QObject* sender, const char* signal, std::function<void(void**)> func)
{
  if (!sender)
    return nullptr;

  int index = sender->metaObject()->indexOfSignal(QMetaObject::normalizedSignature(signal).constData());
  if (index < 0)
  {
    fprintf(stderr, "IUP QML: no signal %s on %s\n", signal, sender->metaObject()->className());
    return nullptr;
  }

  auto* slot = new IupQmlSlot(sender, std::move(func));
  QMetaObject::connect(sender, index, slot, QObject::staticMetaObject.methodCount(), Qt::DirectConnection, nullptr);
  return slot;
}

/****************************************************************************
 * Properties
 ****************************************************************************/

IUP_DRV_API bool iupqmlSetProperty(QObject* obj, const char* name, const QVariant& value)
{
  if (!obj)
    return false;

  if (strchr(name, '.'))
    return QQmlProperty::write(obj, QString::fromLatin1(name), value);

  return obj->setProperty(name, value);
}

IUP_DRV_API QVariant iupqmlGetProperty(QObject* obj, const char* name)
{
  if (!obj)
    return {};

  if (strchr(name, '.'))
    return QQmlProperty::read(obj, QString::fromLatin1(name));

  return obj->property(name);
}

IUP_DRV_API QQuickItem* iupqmlGetItemProperty(QObject* obj, const char* name)
{
  QVariant v = iupqmlGetProperty(obj, name);
  return qobject_cast<QQuickItem*>(qvariant_cast<QObject*>(v));
}

IUP_DRV_API void iupqmlSetPaletteColor(QObject* obj, const char* role, const QColor& color)
{
  if (!obj)
    return;

  auto* palette = qvariant_cast<QObject*>(obj->property("palette"));
  if (palette)
    palette->setProperty(role, color);
}

IUP_DRV_API void iupqmlSetPaddings(QQuickItem* item, double left, double right, double top, double bottom)
{
  if (!item)
    return;

  item->setProperty("leftPadding", left);
  item->setProperty("rightPadding", right);
  item->setProperty("topPadding", top);
  item->setProperty("bottomPadding", bottom);
  item->setProperty("_iup_paddings", true);
}

IUP_DRV_API void iupqmlRestorePaddings(QQuickItem* item, QQuickItem* tpl)
{
  if (!item || !item->property("_iup_paddings").toBool())
    return;

  if (tpl)
    iupqmlSetPaddings(item, tpl->property("leftPadding").toDouble(), tpl->property("rightPadding").toDouble(),
                      tpl->property("topPadding").toDouble(), tpl->property("bottomPadding").toDouble());
  item->setProperty("_iup_paddings", false);
}

IUP_DRV_API int iupqmlCallMethod(QObject* obj, const char* method, const QVariant& arg1, const QVariant& arg2)
{
  if (!obj)
    return 0;

  const QMetaObject* meta = obj->metaObject();
  for (int i = meta->methodCount() - 1; i >= 0; i--)
  {
    QMetaMethod m = meta->method(i);
    if (m.name() != method)
      continue;

    int count = m.parameterCount();
    if (count == 0 && !arg1.isValid())
      return m.invoke(obj, Qt::DirectConnection) ? 1 : 0;
    if (count == 1 && arg1.isValid() && !arg2.isValid())
      return m.invoke(obj, Qt::DirectConnection, Q_ARG(QVariant, arg1)) ? 1 : 0;
    if (count == 2 && arg2.isValid())
      return m.invoke(obj, Qt::DirectConnection, Q_ARG(QVariant, arg1), Q_ARG(QVariant, arg2)) ? 1 : 0;
  }

  return 0;
}

/****************************************************************************
 * Native Handle Access
 ****************************************************************************/

IUP_DRV_API char* iupqmlGetNativeWindowHandle(QQuickWindow* window)
{
  if (!window)
    return nullptr;

  QString platform = QGuiApplication::platformName();

  if (platform == "wayland")
  {
#if QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
    return nullptr;
#endif
  }

  WId native_id = window->winId();
  return reinterpret_cast<char*>(static_cast<uintptr_t>(native_id));
}

IUP_DRV_API const char* iupqmlGetNativeWindowHandleName()
{
  QByteArray platformUtf8 = QGuiApplication::platformName().toUtf8();
  const char* platform = platformUtf8.constData();

  if (strcmp(platform, "xcb") == 0)
    return "XWINDOW";

  if (strcmp(platform, "wayland") == 0)
    return "WL_SURFACE";

  if (strcmp(platform, "windows") == 0)
    return "HWND";

  if (strcmp(platform, "cocoa") == 0)
    return "NSVIEW";

  return "UNKNOWN";
}

IUP_DRV_API char* iupqmlGetNativeWindowHandleAttrib(Ihandle* ih)
{
  QQuickItem* item = iupqmlGetItem(ih);
  if (item)
    return iupqmlGetNativeWindowHandle(item->window());

  return iupqmlGetNativeWindowHandle(iupqmlDialogGetWindow(ih));
}

/****************************************************************************
 * Global Colors
 ****************************************************************************/

static void qmlSetGlobalColorAttrib(const char* name, const QColor& color)
{
  iupGlobalSetDefaultColorAttrib(name, color.red(), color.green(), color.blue());
}

static QPalette qml_last_palette;
static int qml_last_palette_set = 0;

IUP_DRV_API QPalette iupqmlGetPalette()
{
  return QGuiApplication::palette();
}

IUP_DRV_API int iupqmlSystemPaletteChanged()
{
  QPalette palette = QGuiApplication::palette();

  if (qml_last_palette_set && palette == qml_last_palette)
    return 0;

  qml_last_palette = palette;
  qml_last_palette_set = 1;
  return 1;
}

IUP_DRV_API void iupqmlSetGlobalColors()
{
  QPalette palette = iupqmlGetPalette();

  iupqmlSystemPaletteChanged();

  qmlSetGlobalColorAttrib("DLGBGCOLOR", palette.color(QPalette::Window));
  qmlSetGlobalColorAttrib("DLGFGCOLOR", palette.color(QPalette::WindowText));
  qmlSetGlobalColorAttrib("TXTBGCOLOR", palette.color(QPalette::Base));
  qmlSetGlobalColorAttrib("TXTFGCOLOR", palette.color(QPalette::Text));
  qmlSetGlobalColorAttrib("TXTHLCOLOR", palette.color(QPalette::Highlight));
  if (palette.isBrushSet(QPalette::Active, QPalette::Accent))
    qmlSetGlobalColorAttrib("ACCENTCOLOR", palette.color(QPalette::Accent));
  else
    qmlSetGlobalColorAttrib("ACCENTCOLOR", palette.color(QPalette::Highlight));
  qmlSetGlobalColorAttrib("MENUBGCOLOR", palette.color(QPalette::Window));
  qmlSetGlobalColorAttrib("MENUFGCOLOR", palette.color(QPalette::WindowText));
  qmlSetGlobalColorAttrib("LINKFGCOLOR", palette.color(QPalette::Link));
}

static void qmlSetGlobalAttrib()
{
  QString platform = QGuiApplication::platformName();

  if (platform == "xcb")
  {
    IupSetGlobal("WINDOWING", "X11");

#if !defined(Q_OS_WIN) && !defined(Q_OS_MACOS) && !defined(Q_OS_HAIKU)
    if (auto* x11App = qml_application->nativeInterface<QNativeInterface::QX11Application>())
    {
      Display* xdisplay = x11App->display();
      if (xdisplay)
      {
        IupSetGlobal("XDISPLAY", reinterpret_cast<char*>(xdisplay));
#ifdef IUPX11_USE_DLOPEN
        if (iupX11Open())
#endif
        {
          IupSetGlobal("XSCREEN", reinterpret_cast<char*>(static_cast<intptr_t>(XDefaultScreen(xdisplay))));
          IupSetGlobal("XSERVERVENDOR", XServerVendor(xdisplay));
          IupSetInt(nullptr, "XVENDORRELEASE", XVendorRelease(xdisplay));
        }
      }
    }
#endif
  }
  else if (platform == "wayland")
  {
    IupSetGlobal("WINDOWING", "WAYLAND");

#if defined(IUP_QML_HAS_WAYLAND_APP)
    if (auto* waylandApp = qml_application->nativeInterface<QNativeInterface::QWaylandApplication>())
    {
      void* wl_display = waylandApp->display();
      if (wl_display)
        IupSetGlobal("WL_DISPLAY", static_cast<char*>(wl_display));
    }
#endif
  }
  else if (platform == "windows")
  {
    IupSetGlobal("WINDOWING", "DWM");
  }
  else if (platform == "cocoa")
  {
    IupSetGlobal("WINDOWING", "QUARTZ");
  }
  else
  {
    IupStoreGlobal("WINDOWING", platform.toUtf8().constData());
  }
}

/****************************************************************************
 * Appearance
 ****************************************************************************/

static QPalette qml_system_palette;
static int qml_appearance = IUP_APPEARANCE_SYSTEM;

static int qmlPaletteIsDark(const QPalette& palette)
{
  QColor bg = palette.color(QPalette::Window);
  QColor fg = palette.color(QPalette::WindowText);

  double bg_lum = 0.2126 * bg.redF() + 0.7152 * bg.greenF() + 0.0722 * bg.blueF();
  double fg_lum = 0.2126 * fg.redF() + 0.7152 * fg.greenF() + 0.0722 * fg.blueF();

  return (bg_lum < fg_lum) ? 1 : 0;
}

IUP_DRV_API int iupqmlStyleIsDark()
{
  return qmlPaletteIsDark(QGuiApplication::palette());
}

static QPalette qmlAppearancePalette(int dark)
{
  QColor window_text = dark? QColor(240, 240, 240): QColor(Qt::black);
  QColor background = dark? QColor(50, 50, 50): QColor(239, 239, 239);
  QColor light = background.lighter(150);
  QColor mid = background.darker(130);
  QColor dark_color = background.darker(150);
  QColor base = dark? background.darker(140): QColor(Qt::white);
  QColor text = window_text;
  QColor highlight(48, 140, 198);
  QColor disabled_text = dark? QColor(130, 130, 130): QColor(190, 190, 190);

  QPalette palette(window_text, background, light, dark_color, mid, text, base);

  palette.setBrush(QPalette::Midlight, mid.lighter(110));
  palette.setBrush(QPalette::Button, background);
  palette.setBrush(QPalette::Shadow, dark_color.darker(135));
  palette.setBrush(QPalette::HighlightedText, dark? window_text: QColor(Qt::white));

  palette.setBrush(QPalette::Disabled, QPalette::Text, disabled_text);
  palette.setBrush(QPalette::Disabled, QPalette::WindowText, disabled_text);
  palette.setBrush(QPalette::Disabled, QPalette::ButtonText, disabled_text);
  palette.setBrush(QPalette::Disabled, QPalette::Base, background);
  palette.setBrush(QPalette::Disabled, QPalette::Highlight, QColor(145, 145, 145));

  palette.setBrush(QPalette::Active, QPalette::Highlight, highlight);
  palette.setBrush(QPalette::Inactive, QPalette::Highlight, highlight);
  palette.setBrush(QPalette::Active, QPalette::Accent, highlight);
  palette.setBrush(QPalette::Inactive, QPalette::Accent, highlight);

  if (dark)
    palette.setBrush(QPalette::Link, highlight);

  return palette;
}

IUP_DRV_API void iupqmlUpdateWindowPalette(QQuickWindow* window)
{
  static const struct { const char* name; QPalette::ColorGroup group; } groups[] = {
    {"active", QPalette::Active}, {"inactive", QPalette::Inactive}, {"disabled", QPalette::Disabled},
  };
  static const struct { const char* name; QPalette::ColorRole role; } roles[] = {
    {"alternateBase", QPalette::AlternateBase}, {"base", QPalette::Base}, {"brightText", QPalette::BrightText},
    {"button", QPalette::Button}, {"buttonText", QPalette::ButtonText}, {"dark", QPalette::Dark},
    {"highlight", QPalette::Highlight}, {"highlightedText", QPalette::HighlightedText}, {"light", QPalette::Light},
    {"link", QPalette::Link}, {"linkVisited", QPalette::LinkVisited}, {"mid", QPalette::Mid},
    {"midlight", QPalette::Midlight}, {"shadow", QPalette::Shadow}, {"text", QPalette::Text},
    {"toolTipBase", QPalette::ToolTipBase}, {"toolTipText", QPalette::ToolTipText}, {"window", QPalette::Window},
    {"windowText", QPalette::WindowText}, {"placeholderText", QPalette::PlaceholderText}, {"accent", QPalette::Accent},
  };

  auto* palette = window ? qvariant_cast<QObject*>(window->property("palette")) : nullptr;
  if (!palette)
    return;

  QPalette app_palette = QGuiApplication::palette();
  for (const auto& g : groups)
  {
    auto* group = qvariant_cast<QObject*>(palette->property(g.name));
    if (!group)
      continue;

    const QMetaObject* meta = group->metaObject();
    for (const auto& r : roles)
    {
      if (qml_appearance == IUP_APPEARANCE_SYSTEM)
        meta->property(meta->indexOfProperty(r.name)).reset(group);
      else
        group->setProperty(r.name, app_palette.color(g.group, r.role));
    }
  }
}

extern "C" IUP_SDK_API void iupdrvSetAppearance(int appearance)
{
  qml_appearance = appearance;

  if (appearance == IUP_APPEARANCE_DARK)
    QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Dark);
  else if (appearance == IUP_APPEARANCE_LIGHT)
    QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
  else
    QGuiApplication::styleHints()->unsetColorScheme();

  if (appearance == IUP_APPEARANCE_SYSTEM)
    QGuiApplication::setPalette(qml_system_palette);
  else
  {
    int want_dark = (appearance == IUP_APPEARANCE_DARK)? 1: 0;
    if (qmlPaletteIsDark(QGuiApplication::palette()) != want_dark)
      QGuiApplication::setPalette(qmlAppearancePalette(want_dark));
  }

  for (Ihandle* dlg = iupDlgListFirst(); dlg; dlg = iupDlgListNext())
  {
    if (dlg->handle)
      iupqmlUpdateWindowPalette(reinterpret_cast<QQuickWindow*>(dlg->handle));
  }

  iupqmlSetGlobalColors();
}

IUP_DRV_API void iupqmlUpdateSystemPalette()
{
  if (qml_appearance == IUP_APPEARANCE_SYSTEM)
    qml_system_palette = QGuiApplication::palette();
}

extern "C" IUP_SDK_API int iupdrvIsSystemDarkMode(void)
{
  return qmlPaletteIsDark(qml_system_palette);
}

/****************************************************************************
 * Driver Initialization
 ****************************************************************************/

extern "C" IUP_SDK_API int iupdrvOpen(int* argc, char*** argv)
{
  if (!QGuiApplication::instance())
  {
    static int original_argc = 1;
    static int default_argc = 1;
    static char exe_path[4096] = {0};
    static char* default_argv_data[2] = { exe_path, nullptr };
    static char** default_argv = default_argv_data;

    if (exe_path[0] == 0)
    {
#if defined(__linux__)
      ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
      if (len > 0)
        exe_path[len] = '\0';
      else
        iupStrCopyN(exe_path, sizeof(exe_path), "iup-qml");
#else
      iupStrCopyN(exe_path, sizeof(exe_path), "iup-qml");
#endif
    }

    if (!argc || !argv || *argc == 0 || !(*argv))
    {
      default_argc = original_argc;
      argc = &default_argc;
      argv = &default_argv;
    }

    qml_application = new QGuiApplication(*argc, *argv);
    qml_application_owned = 1;

    QFont system_font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    if (system_font != QGuiApplication::font())
      QGuiApplication::setFont(system_font);
  }
  else
  {
    qml_application = qobject_cast<QGuiApplication*>(QGuiApplication::instance());

    if (!qml_application)
      return IUP_ERROR;
  }

  QGuiApplication::setQuitOnLastWindowClosed(false);
  QQuickWindow::setTextRenderType(QQuickWindow::NativeTextRendering);

  setlocale(LC_NUMERIC, "C");

  IupSetGlobal("DRIVER", "QML");

  IupSetfAttribute(nullptr, "QTVERSION", "%s", qVersion());
  IupSetfAttribute(nullptr, "QTDEVVERSION", "%d.%d.%d", QT_VERSION_MAJOR, QT_VERSION_MINOR, QT_VERSION_PATCH);

#ifdef QT_DEBUG
  IupSetGlobal("QTBUILDTYPE", "Debug");
#else
  IupSetGlobal("QTBUILDTYPE", "Release");
#endif

  if (argv && *argv && (*argv)[0] && (*argv)[0][0] != 0)
  {
    IupStoreGlobal("ARGV0", (*argv)[0]);
  }

  qmlSetGlobalAttrib();

  qml_system_palette = QGuiApplication::palette();

  iupqmlSetGlobalColors();

  IupSetGlobal("SHOWMENUIMAGES", "YES");
  IupSetGlobal("HIGHDPI_AWARE", "YES");

  if (QQuickStyle::name().isEmpty())
    QQuickStyle::setStyle("Fusion");

  IupStoreGlobal("QTSTYLE", QQuickStyle::name().toUtf8().constData());

  return IUP_NOERROR;
}

extern "C" IUP_SDK_API int iupdrvSetGlobalAppIDAttrib(const char* value)
{
  static int appid_set = 0;
  if (appid_set || !value || !value[0])
    return 0;

  if (!qml_application)
    return 0;

  QGuiApplication::setDesktopFileName(QString::fromUtf8(value));
  qputenv("RESOURCE_NAME", QByteArray(value));
  appid_set = 1;
  return 1;
}

extern "C" IUP_SDK_API int iupdrvSetGlobalAppNameAttrib(const char* value)
{
  static int appname_set = 0;
  if (appname_set || !value || !value[0])
    return 0;

  if (!qml_application)
    return 0;

  QCoreApplication::setApplicationName(QString::fromUtf8(value));
  appname_set = 1;
  return 1;
}

extern "C" IUP_SDK_API void iupdrvClose(void)
{
  iupqmlLoopCleanup();

  if (qml_engine)
  {
    if (qml_templates)
    {
      for (QQuickItem* item : *qml_templates)
        delete item;
      delete qml_templates;
      qml_templates = nullptr;
    }

    delete qml_template_window;
    qml_template_window = nullptr;

    if (qml_components)
    {
      for (QQmlComponent* component : *qml_components)
        delete component;
      delete qml_components;
      qml_components = nullptr;
    }

    delete qml_engine;
    qml_engine = nullptr;
    qml_image_provider = nullptr;
  }

  if (qml_application_owned)
  {
    delete qml_application;
    qml_application = nullptr;
    qml_application_owned = 0;
  }

#ifdef IUPX11_USE_DLOPEN
  iupX11Close();
#endif
}
