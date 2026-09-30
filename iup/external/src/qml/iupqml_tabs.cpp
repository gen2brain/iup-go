/** \file
 * \brief Tabs Control - Qt Quick implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QQuickItem>
#include <QQuickStyle>
#include <QTimer>
#include <QString>
#include <QPixmap>
#include <QUrl>
#include <QColor>
#include <QMouseEvent>

#include <cmath>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_drvfont.h"
#include "iup_image.h"
#include "iup_tabs.h"
}

#include "iupqml_drv.h"

static const char* qmlTabButtonQml =
  "TabButton { id: tb; width: implicitWidth; icon.color: \"transparent\"; property string iupTip: \"\"; signal iupRightClick()\n"
  "  ToolTip.visible: hovered && iupTip.length > 0; ToolTip.text: iupTip; ToolTip.delay: 700\n"
  "  TapHandler { acceptedButtons: Qt.RightButton; onTapped: tb.iupRightClick() }\n"
  "  Rectangle { anchors.fill: parent; anchors.margins: 2; color: \"transparent\"; border.color: tb.palette.highlight; border.width: 1; visible: tb.visualFocus }\n"
  "  property bool iupClose: false; signal iupCloseClicked()\n"
  "  property bool iupReorder: false; signal iupDragMoved(point pos); signal iupDragged(point pos)\n"
  "  DragHandler { enabled: tb.iupReorder; target: null\n"
  "    onCentroidChanged: if (active) tb.iupDragMoved(centroid.scenePosition)\n"
  "    onActiveChanged: if (!active) tb.iupDragged(centroid.scenePosition) }\n"
  "  Item { visible: tb.iupClose; width: 16; height: 16; anchors.right: parent.right; anchors.rightMargin: 4; anchors.verticalCenter: parent.verticalCenter; z: 2\n"
  "    Rectangle { anchors.fill: parent; radius: 2; color: tb.palette.mid; visible: closeHover.hovered }\n"
  "    Text { anchors.centerIn: parent; text: \"\\u2715\"; font.pixelSize: 10; color: tb.palette.buttonText }\n"
  "    HoverHandler { id: closeHover }\n"
  "    TapHandler { gesturePolicy: TapHandler.ReleaseWithinBounds; onTapped: tb.iupCloseClicked() }\n"
  "  }\n"
  "}";

static bool qmlTabsUseFallbackStyle()
{
  static int fallback = -1;
  if (fallback < 0)
  {
    QQuickItem* current = iupqmlTemplateItem(IUPQML_IMPORTS "TabButton { }");
    QQuickItem* basic = iupqmlTemplateItem("import QtQuick\nimport QtQuick.Controls.Basic\nTabButton { }");
    fallback = current && basic && QQuickStyle::name() != QLatin1String("Basic") &&
               strcmp(current->metaObject()->className(), basic->metaObject()->className()) == 0 &&
               iupqmlTemplateItem("import QtQuick\nimport QtQuick.Controls.Fusion\nTabButton { }");
  }
  return fallback == 1;
}

static QByteArray qmlTabsQml(const char* body)
{
  QByteArray qml(IUPQML_IMPORTS);
  if (qmlTabsUseFallbackStyle())
    qml += "import QtQuick.Controls.Fusion as IupTabStyle\nIupTabStyle.";
  qml += body;
  return qml;
}

static QQuickItem* qmlTabsGetBar(Ihandle* ih)
{
  return reinterpret_cast<QQuickItem*>(iupAttribGet(ih, "_IUPQML_TABBAR"));
}

static void qmlTabsGetMaxTabSize(Ihandle* ih, int* max_w, int* max_h)
{
  Ihandle* child;
  int pos = 0;
  *max_w = 0;
  *max_h = 0;

  for (child = ih->firstchild; child; child = child->brother, pos++)
  {
    int w = 0, h = 0;
    char* title = iupAttribGet(child, "TABTITLE");
    char* image = iupAttribGet(child, "TABIMAGE");
    if (!title) title = iupAttribGetId(ih, "TABTITLE", pos);
    if (!image) image = iupAttribGetId(ih, "TABIMAGE", pos);
    iupdrvTabsGetTabSize(ih, title, image, &w, &h);
    if (w > *max_w) *max_w = w;
    if (h > *max_h) *max_h = h;
  }
}

static int qmlTabsGetBarHeight(Ihandle* ih)
{
  int max_w, max_h;
  qmlTabsGetMaxTabSize(ih, &max_w, &max_h);
  return 3 + max_h + 3;
}

static int qmlTabsIsVertical(Ihandle* ih)
{
  return ih->data->type == ITABS_LEFT || ih->data->type == ITABS_RIGHT;
}

static int qmlTabsGetBarWidth(Ihandle* ih)
{
  int max_w, max_h;
  qmlTabsGetMaxTabSize(ih, &max_w, &max_h);
  return max_w;
}

static void qmlTabsGetDecorOffset(Ihandle* ih, int* dx, int* dy)
{
  int m = 4, s = 2;

  if (ih->data->type == ITABS_TOP)
  {
    *dx = m;
    *dy = m + qmlTabsGetBarHeight(ih) + s;
  }
  else if (ih->data->type == ITABS_LEFT)
  {
    *dx = m + qmlTabsGetBarWidth(ih) + s;
    *dy = m;
  }
  else
  {
    *dx = m;
    *dy = m;
  }
}

static void qmlTabsGetDecorSize(Ihandle* ih, int* w, int* h)
{
  int m = 4, s = 2;

  if (qmlTabsIsVertical(ih))
  {
    *w = m + qmlTabsGetBarWidth(ih) + s + m;
    *h = m + m;
  }
  else
  {
    *w = m + m;
    *h = m + qmlTabsGetBarHeight(ih) + s + m;
  }
}

/****************************************************************************
 * Driver Functions
 ****************************************************************************/

extern "C" IUP_SDK_API int iupdrvTabsExtraDecor(Ihandle* ih)
{
  (void)ih;
  return 0;
}

extern "C" IUP_SDK_API int iupdrvTabsExtraMargin(void)
{
  return 0;
}

extern "C" IUP_SDK_API int iupdrvTabsGetLineCountAttrib(Ihandle* ih)
{
  (void)ih;
  return 1;
}

static QList<Ihandle*> qmlTabsSlots(Ihandle* ih)
{
  QList<Ihandle*> list;
  for (Ihandle* c = ih->firstchild; c; c = c->brother)
    list.append(c);
  auto* removed = reinterpret_cast<Ihandle*>(iupAttribGet(ih, "_IUPQML_REMOVED_CHILD"));
  if (removed)
    list.insert(qBound(0, iupAttribGetInt(ih, "_IUPQML_REMOVED_POS"), static_cast<int>(list.size())), removed);
  return list;
}

static int qmlTabsButtonIndex(Ihandle* ih, Ihandle* child)
{
  int index = 0;
  for (Ihandle* c : qmlTabsSlots(ih))
  {
    if (c == child)
      break;
    auto* button = reinterpret_cast<QQuickItem*>(iupAttribGet(c, "_IUPQML_TABBUTTON"));
    if (button && button->parentItem())
      index++;
  }
  return index;
}

static Ihandle* qmlTabsChildFromIndex(Ihandle* ih, int index)
{
  int i = 0;
  for (Ihandle* c : qmlTabsSlots(ih))
  {
    auto* button = reinterpret_cast<QQuickItem*>(iupAttribGet(c, "_IUPQML_TABBUTTON"));
    if (button && button->parentItem())
    {
      if (i == index)
        return c;
      i++;
    }
  }
  return nullptr;
}

static void qmlTabsShowPage(Ihandle* ih, Ihandle* child)
{
  for (Ihandle* c = ih->firstchild; c; c = c->brother)
  {
    auto* container = reinterpret_cast<QQuickItem*>(iupAttribGet(c, "_IUPTAB_CONTAINER"));
    if (container)
      container->setVisible(c == child);
  }
}

extern "C" IUP_SDK_API void iupdrvTabsSetCurrentTab(Ihandle* ih, int pos)
{
  QQuickItem* bar = qmlTabsGetBar(ih);
  Ihandle* child = qmlTabsSlots(ih).value(pos);
  if (!bar || !child)
    return;

  iupAttribSet(ih, "_IUPQML_IGNORE_CHANGE", "1");
  bar->setProperty("currentIndex", qmlTabsButtonIndex(ih, child));
  qmlTabsShowPage(ih, child);
  iupAttribSet(ih, "_IUPQML_PREV_CHILD", reinterpret_cast<char*>(child));
  iupAttribSet(ih, "_IUPQML_IGNORE_CHANGE", nullptr);
}

extern "C" IUP_SDK_API int iupdrvTabsGetCurrentTab(Ihandle* ih)
{
  QQuickItem* bar = qmlTabsGetBar(ih);
  if (!bar)
    return -1;

  Ihandle* child = qmlTabsChildFromIndex(ih, bar->property("currentIndex").toInt());
  return child ? static_cast<int>(qmlTabsSlots(ih).indexOf(child)) : -1;
}

static QQuickItem* qmlTabsButtonTemplate()
{
  return iupqmlTemplateItem(qmlTabsQml("TabButton { }").constData());
}

static int qmlTabsShowClose(Ihandle* ih, Ihandle* child)
{
  char* value = iupAttribGet(child, "SHOWCLOSE");
  return value ? iupStrBoolean(value) : ih->data->show_close;
}

static void qmlTabsApplyButtonPadding(Ihandle* ih, Ihandle* child, QQuickItem* button)
{
  QQuickItem* tpl = qmlTabsButtonTemplate();
  int horiz = ih->data->horiz_padding, vert = ih->data->vert_padding;
  int show_close = qmlTabsShowClose(ih, child);
  button->setProperty("iupClose", show_close ? true : false);

  if ((horiz == 0 && vert == 0 && !show_close) || !tpl)
  {
    iupqmlRestorePaddings(button, tpl);
    return;
  }

  iupqmlSetPaddings(button, horiz + tpl->property("leftPadding").toDouble(), horiz + tpl->property("rightPadding").toDouble() + (show_close ? 20 : 0),
                    vert + tpl->property("topPadding").toDouble(), vert + tpl->property("bottomPadding").toDouble());
}

static void qmlTabsCloseClicked(Ihandle* ih, Ihandle* child)
{
  if (!iupObjectCheck(ih) || !iupObjectCheck(child))
    return;

  int pos = IupGetChildPos(ih, child);
  IFni cb = reinterpret_cast<IFni>(IupGetCallback(ih, "TABCLOSE_CB"));
  int ret = cb ? cb(ih, pos) : IUP_DEFAULT;

  if (ret == IUP_CONTINUE)
  {
    QTimer::singleShot(0, [ih, child]() {
      if (iupObjectCheck(ih) && iupObjectCheck(child))
      {
        IupDestroy(child);
        IupRefreshChildren(ih);
      }
    });
  }
  else if (ret == IUP_DEFAULT)
    IupSetAttributeId(ih, "TABVISIBLE", pos, "NO");
}

static int qmlTabsSetShowCloseAttrib(Ihandle* ih, int pos, const char* value)
{
  if (pos == IUP_INVALID_ID)
  {
    ih->data->show_close = iupStrBoolean(value);
    if (ih->handle)
    {
      for (Ihandle* child = ih->firstchild; child; child = child->brother)
      {
        auto* button = reinterpret_cast<QQuickItem*>(iupAttribGet(child, "_IUPQML_TABBUTTON"));
        if (button)
          qmlTabsApplyButtonPadding(ih, child, button);
      }
    }
    return 1;
  }

  Ihandle* child = IupGetChild(ih, pos);
  if (child)
  {
    iupAttribSetStr(child, "SHOWCLOSE", value);
    auto* button = reinterpret_cast<QQuickItem*>(iupAttribGet(child, "_IUPQML_TABBUTTON"));
    if (button)
      qmlTabsApplyButtonPadding(ih, child, button);
  }
  return 0;
}

extern "C" IUP_SDK_API void iupdrvTabsGetTabSize(Ihandle* ih, const char* tab_title, const char* tab_image, int* tab_width, int* tab_height)
{
  QQuickItem* button = qmlTabsButtonTemplate();
  int width = 0, height = 0;

  if (button)
  {
    QFont* font = iupqmlGetIhFont(ih);
    if (font)
      button->setProperty("font", QVariant::fromValue(*font));
    button->setProperty("text", QString::fromUtf8(tab_title ? tab_title : ""));

    if (tab_image)
    {
      auto* pixmap = static_cast<QPixmap*>(iupImageGetImage(tab_image, ih, 0, nullptr));
      if (pixmap)
      {
        int img_w, img_h;
        iupTabsScaleImageSize(ih, pixmap->width(), pixmap->height(), &img_w, &img_h);
        iupqmlSetProperty(button, "icon.source", QUrl(iupqmlImageUrl(pixmap)));
        iupqmlSetProperty(button, "icon.width", img_w);
        iupqmlSetProperty(button, "icon.height", img_h);
      }
    }
    else
      iupqmlSetProperty(button, "icon.source", QUrl());

    width = static_cast<int>(std::ceil(button->implicitWidth())) + 2 * ih->data->horiz_padding;
    height = static_cast<int>(std::ceil(button->implicitHeight())) + 2 * ih->data->vert_padding;
  }

  if (width <= 0 || height <= 0)
  {
    if (tab_title)
    {
      width = iupdrvFontGetStringWidth(ih, tab_title);
      iupdrvFontGetCharSize(ih, nullptr, &height);
    }
    width += 24;
    height += 8;
  }

  if (tab_width) *tab_width = width + (ih->data->show_close ? 20 : 0);
  if (tab_height) *tab_height = height;
}

extern "C" IUP_SDK_API int iupdrvTabsIsTabVisible(Ihandle* child, int pos)
{
  auto* button = reinterpret_cast<QQuickItem*>(iupAttribGet(child, "_IUPQML_TABBUTTON"));
  (void)pos;
  return (button && button->parentItem()) ? 1 : 0;
}

/****************************************************************************
 * Layout
 ****************************************************************************/

static void qmlTabsLayoutPages(Ihandle* ih)
{
  QQuickItem* bar = qmlTabsGetBar(ih);
  if (!bar)
    return;

  int dx, dy, dw, dh;
  qmlTabsGetDecorOffset(ih, &dx, &dy);
  qmlTabsGetDecorSize(ih, &dw, &dh);

  int client_w = ih->currentwidth - dw;
  int client_h = ih->currentheight - dh;
  if (client_w < 0) client_w = 0;
  if (client_h < 0) client_h = 0;

  auto* frame = reinterpret_cast<QQuickItem*>(iupAttribGet(ih, "_IUPQML_TABFRAME"));
  if (qmlTabsIsVertical(ih))
  {
    int bar_w = qmlTabsGetBarWidth(ih);
    int bar_x = ih->data->type == ITABS_RIGHT ? ih->currentwidth - bar_w : 0;
    bar->setPosition(QPointF(bar_x, 0));
    bar->setSize(QSizeF(bar_w, ih->currentheight));
    for (Ihandle* c = ih->firstchild; c; c = c->brother)
    {
      auto* button = reinterpret_cast<QQuickItem*>(iupAttribGet(c, "_IUPQML_TABBUTTON"));
      if (button)
        button->setWidth(bar_w);
    }
    if (frame)
    {
      frame->setPosition(QPointF(ih->data->type == ITABS_RIGHT ? 0 : bar_w - 1, 0));
      frame->setSize(QSizeF(ih->currentwidth - bar_w + 1, ih->currentheight));
    }
  }
  else
  {
    int bar_h = qmlTabsGetBarHeight(ih);
    bar->setPosition(QPointF(0, ih->data->type == ITABS_BOTTOM ? ih->currentheight - bar_h : 0));
    bar->setSize(QSizeF(ih->currentwidth, bar_h));
    if (frame)
    {
      frame->setPosition(QPointF(0, ih->data->type == ITABS_BOTTOM ? 0 : bar_h - 1));
      frame->setSize(QSizeF(ih->currentwidth, ih->currentheight - bar_h + 1));
    }
  }

  for (Ihandle* c = ih->firstchild; c; c = c->brother)
  {
    auto* container = reinterpret_cast<QQuickItem*>(iupAttribGet(c, "_IUPTAB_CONTAINER"));
    if (container)
    {
      container->setPosition(QPointF(dx, dy));
      container->setSize(QSizeF(client_w, client_h));
    }
  }
}

static void qmlTabsLayoutUpdateMethod(Ihandle* ih)
{
  iupdrvBaseLayoutUpdateMethod(ih);
  qmlTabsLayoutPages(ih);
}

/****************************************************************************
 * Callbacks
 ****************************************************************************/

static QQuickItem* qmlTabsFocusItem(Ihandle* ih)
{
  QQuickItem* bar = qmlTabsGetBar(ih);
  Ihandle* child = bar ? qmlTabsChildFromIndex(ih, bar->property("currentIndex").toInt()) : nullptr;
  return child ? reinterpret_cast<QQuickItem*>(iupAttribGet(child, "_IUPQML_TABBUTTON")) : bar;
}

static int qmlTabsMoveCurrent(Ihandle* ih, QKeyEvent* key)
{
  int next = qmlTabsIsVertical(ih) ? Qt::Key_Down : Qt::Key_Right;
  int prev = qmlTabsIsVertical(ih) ? Qt::Key_Up : Qt::Key_Left;
  int step = key->key() == next ? 1 : key->key() == prev ? -1 : 0;
  if (!step || (key->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier)))
    return 0;

  QQuickItem* bar = qmlTabsGetBar(ih);
  if (!bar)
    return 0;

  for (int i = bar->property("currentIndex").toInt() + step; ; i += step)
  {
    Ihandle* child = qmlTabsChildFromIndex(ih, i);
    if (!child)
      return 1;

    auto* button = reinterpret_cast<QQuickItem*>(iupAttribGet(child, "_IUPQML_TABBUTTON"));
    if (button && button->isEnabled())
    {
      bar->setProperty("currentIndex", i);
      button->forceActiveFocus(Qt::TabFocusReason);
      return 1;
    }
  }
}

static void qmlTabsCurrentChanged(Ihandle* ih)
{
  QQuickItem* bar = qmlTabsGetBar(ih);
  if (!bar || iupAttribGet(ih, "_IUPQML_IGNORE_CHANGE"))
    return;

  int index = bar->property("currentIndex").toInt();
  Ihandle* child = qmlTabsChildFromIndex(ih, index);
  auto* prev_child = reinterpret_cast<Ihandle*>(iupAttribGet(ih, "_IUPQML_PREV_CHILD"));
  if (!child)
    return;

  qmlTabsShowPage(ih, child);
  iupAttribSet(ih, "_IUPQML_PREV_CHILD", reinterpret_cast<char*>(child));

  if (prev_child == child)
    return;

  if (!prev_child)
    prev_child = child;

  auto cb = reinterpret_cast<IFnnn>(IupGetCallback(ih, "TABCHANGE_CB"));
  if (cb)
    cb(ih, child, prev_child);
  else
  {
    auto cb2 = reinterpret_cast<IFnii>(IupGetCallback(ih, "TABCHANGEPOS_CB"));
    if (cb2 && prev_child)
      cb2(ih, IupGetChildPos(ih, child), IupGetChildPos(ih, prev_child));
  }
}

class IupQmlTabsKeyFilter : public QObject
{
public:
  Ihandle* ih;
  IupQmlTabsKeyFilter(QObject* parent, Ihandle* handle) : QObject(parent), ih(handle) {}

  bool eventFilter(QObject* obj, QEvent* event) override
  {
    if (event->type() != QEvent::KeyPress || !iupObjectCheck(ih))
      return false;

    auto* key = static_cast<QKeyEvent*>(event);
    if (iupqmlKeyPressEvent(qobject_cast<QQuickItem*>(obj), key, ih))
      return true;
    return iupObjectCheck(ih) && qmlTabsMoveCurrent(ih, key);
  }
};

static void qmlTabsActiveFocusChanged(Ihandle* ih, bool active)
{
  QFocusEvent evt(active ? QEvent::FocusIn : QEvent::FocusOut);
  iupqmlFocusInOutEvent(&evt, ih);
}

/****************************************************************************
 * Attributes
 ****************************************************************************/

static int qmlTabsSetTabPaddingAttrib(Ihandle* ih, const char* value)
{
  int horiz_padding = 0, vert_padding = 0;
  iupStrToIntInt(value, &horiz_padding, &vert_padding, 'x');

  ih->data->horiz_padding = horiz_padding;
  ih->data->vert_padding = vert_padding;

  if (ih->handle)
  {
    for (Ihandle* child = ih->firstchild; child; child = child->brother)
    {
      auto* button = reinterpret_cast<QQuickItem*>(iupAttribGet(child, "_IUPQML_TABBUTTON"));
      if (button)
        qmlTabsApplyButtonPadding(ih, child, button);
    }
    qmlTabsLayoutPages(ih);
    return 0;
  }

  return 1;
}

static char* qmlTabsGetTabPaddingAttrib(Ihandle* ih)
{
  return iupStrReturnIntInt(ih->data->horiz_padding, ih->data->vert_padding, 'x');
}

static int qmlTabsSetTabTypeAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle)
    return 0;

  if (iupStrEqualNoCase(value, "BOTTOM"))
    ih->data->type = ITABS_BOTTOM;
  else if (iupStrEqualNoCase(value, "LEFT"))
    ih->data->type = ITABS_LEFT;
  else if (iupStrEqualNoCase(value, "RIGHT"))
    ih->data->type = ITABS_RIGHT;
  else
    ih->data->type = ITABS_TOP;

  return 0;
}

static void qmlTabsUpdateButton(Ihandle* ih, Ihandle* child)
{
  auto* button = reinterpret_cast<QQuickItem*>(iupAttribGet(child, "_IUPQML_TABBUTTON"));
  if (!button)
    return;

  char* title = iupAttribGet(child, "TABTITLE");
  char* image = iupAttribGet(child, "TABIMAGE");

  button->setProperty("text", QString::fromUtf8(title ? title : ""));
  iupqmlMnemonicUpdate(button);

  if (image)
  {
    auto* pixmap = static_cast<QPixmap*>(iupImageGetImage(image, ih, 0, nullptr));
    if (pixmap)
    {
      int img_w, img_h;
      iupTabsScaleImageSize(ih, pixmap->width(), pixmap->height(), &img_w, &img_h);
      iupqmlSetProperty(button, "icon.source", QUrl(iupqmlImageUrl(pixmap)));
      iupqmlSetProperty(button, "icon.width", img_w);
      iupqmlSetProperty(button, "icon.height", img_h);
      return;
    }
  }
  iupqmlSetProperty(button, "icon.source", QUrl());
}

static int qmlTabsSetTabTitleAttrib(Ihandle* ih, int pos, const char* value)
{
  Ihandle* child = IupGetChild(ih, pos);
  if (child)
  {
    iupAttribSetStr(child, "TABTITLE", value);
    if (ih->handle)
      qmlTabsUpdateButton(ih, child);
  }
  return 1;
}

static int qmlTabsSetTabTipAttrib(Ihandle* ih, int pos, const char* value)
{
  Ihandle* child = IupGetChild(ih, pos);
  if (child)
  {
    auto* button = reinterpret_cast<QQuickItem*>(iupAttribGet(child, "_IUPQML_TABBUTTON"));
    if (button)
      button->setProperty("iupTip", value ? QString::fromUtf8(value) : QString());
  }
  return 1;
}

static int qmlTabsSetTabImageAttrib(Ihandle* ih, int pos, const char* value)
{
  Ihandle* child = IupGetChild(ih, pos);
  if (child)
  {
    iupAttribSetStr(child, "TABIMAGE", value);
    if (ih->handle)
      qmlTabsUpdateButton(ih, child);
  }
  return 1;
}

static void qmlTabsInsertButton(Ihandle* ih, Ihandle* child)
{
  QQuickItem* bar = qmlTabsGetBar(ih);
  auto* button = reinterpret_cast<QQuickItem*>(iupAttribGet(child, "_IUPQML_TABBUTTON"));
  if (!bar || !button || button->parentItem())
    return;

  QQuickItem* current = nullptr;
  QMetaObject::invokeMethod(bar, "itemAt", Qt::DirectConnection, Q_RETURN_ARG(QQuickItem*, current), Q_ARG(int, bar->property("currentIndex").toInt()));

  int index = qmlTabsButtonIndex(ih, child);
  QMetaObject::invokeMethod(bar, "insertItem", Qt::DirectConnection, Q_ARG(int, index), Q_ARG(QQuickItem*, button));

  if (current)
  {
    int count = bar->property("count").toInt();
    for (int i = 0; i < count; i++)
    {
      QQuickItem* at = nullptr;
      QMetaObject::invokeMethod(bar, "itemAt", Qt::DirectConnection, Q_RETURN_ARG(QQuickItem*, at), Q_ARG(int, i));
      if (at == current)
      {
        bar->setProperty("currentIndex", i);
        break;
      }
    }
  }
}

static void qmlTabsRemoveButton(Ihandle* ih, Ihandle* child)
{
  QQuickItem* bar = qmlTabsGetBar(ih);
  auto* button = reinterpret_cast<QQuickItem*>(iupAttribGet(child, "_IUPQML_TABBUTTON"));
  if (!bar || !button || !button->parentItem())
    return;

  int index = -1;
  int count = bar->property("count").toInt();
  for (int i = 0; i < count; i++)
  {
    QQuickItem* at = nullptr;
    QMetaObject::invokeMethod(bar, "itemAt", Qt::DirectConnection, Q_RETURN_ARG(QQuickItem*, at), Q_ARG(int, i));
    if (at == button)
    {
      index = i;
      break;
    }
  }
  if (index < 0)
    return;

  QQuickItem* taken = nullptr;
  QMetaObject::invokeMethod(bar, "takeItem", Qt::DirectConnection, Q_RETURN_ARG(QQuickItem*, taken), Q_ARG(int, index));
  button->setParentItem(nullptr);
}

static int qmlTabsDragTarget(Ihandle* ih, QPointF scene, QRectF* target_rect)
{
  int vertical = qmlTabsIsVertical(ih);
  qreal v = vertical ? scene.y() : scene.x();
  int to = -1, first = -1, last = -1, pos = 0;
  QRectF last_rect;
  for (Ihandle* c = ih->firstchild; c; c = c->brother, pos++)
  {
    auto* b = reinterpret_cast<QQuickItem*>(iupAttribGet(c, "_IUPQML_TABBUTTON"));
    if (!b || !b->parentItem() || !b->isVisible())
      continue;
    QRectF r = b->mapRectToScene(QRectF(0, 0, b->width(), b->height()));
    qreal start = vertical ? r.top() : r.left();
    qreal end = vertical ? r.bottom() : r.right();
    if (first < 0 && v < start)
    {
      to = pos;
      *target_rect = r;
    }
    if (first < 0)
      first = pos;
    last = pos;
    last_rect = r;
    if (to < 0 && v >= start && v < end)
    {
      to = pos;
      *target_rect = r;
    }
  }
  if (to < 0 && last >= 0)
  {
    to = last;
    *target_rect = last_rect;
  }
  return to;
}

static QQuickItem* qmlTabsDropLine(Ihandle* ih)
{
  QQuickItem* bar = qmlTabsGetBar(ih);
  if (!bar)
    return nullptr;

  auto* line = bar->property("_iup_dropline").value<QQuickItem*>();
  if (!line)
  {
    line = iupqmlCreateItem("import QtQuick\nRectangle { visible: false; z: 100; color: parent ? parent.palette.highlight : \"blue\" }");
    if (!line)
      return nullptr;
    line->setParentItem(bar);
    line->setParent(bar);
    bar->setProperty("_iup_dropline", QVariant::fromValue<QQuickItem*>(line));
  }
  return line;
}

static void qmlTabsHideDropLine(Ihandle* ih)
{
  QQuickItem* bar = qmlTabsGetBar(ih);
  QQuickItem* line = bar ? bar->property("_iup_dropline").value<QQuickItem*>() : nullptr;
  if (line)
    line->setVisible(false);
}

static void qmlTabsDragMoved(Ihandle* ih, Ihandle* child, QPointF scene)
{
  if (!iupObjectCheck(ih) || !iupObjectCheck(child))
    return;

  QRectF r;
  int from = IupGetChildPos(ih, child);
  int to = qmlTabsDragTarget(ih, scene, &r);
  QQuickItem* line = qmlTabsDropLine(ih);
  if (!line)
    return;

  if (to < 0 || to == from)
  {
    line->setVisible(false);
    return;
  }

  r = line->parentItem()->mapRectFromScene(r);
  if (qmlTabsIsVertical(ih))
  {
    qreal y = from < to ? r.bottom() - 1 : r.top() - 1;
    line->setPosition(QPointF(r.left(), y < 0 ? 0 : y));
    line->setSize(QSizeF(r.width(), 2));
  }
  else
  {
    qreal x = from < to ? r.right() - 1 : r.left() - 1;
    line->setPosition(QPointF(x < 0 ? 0 : x, r.top()));
    line->setSize(QSizeF(2, r.height()));
  }
  line->setVisible(true);
}

static void qmlTabsDragged(Ihandle* ih, Ihandle* child, QPointF scene)
{
  if (!iupObjectCheck(ih) || !iupObjectCheck(child))
    return;

  qmlTabsHideDropLine(ih);

  QRectF r;
  int from = IupGetChildPos(ih, child);
  int to = qmlTabsDragTarget(ih, scene, &r);
  if (to < 0 || from == to)
    return;

  auto cb = reinterpret_cast<IFnii>(IupGetCallback(ih, "REORDER_CB"));
  if (cb && cb(ih, from, to) == IUP_IGNORE)
    return;

  Ihandle* current = IupGetChild(ih, iupdrvTabsGetCurrentTab(ih));
  Ihandle* ref_child = from < to ? IupGetChild(ih, to + 1) : IupGetChild(ih, to);

  QQuickItem* bar = qmlTabsGetBar(ih);
  auto* button = reinterpret_cast<QQuickItem*>(iupAttribGet(child, "_IUPQML_TABBUTTON"));
  int old_index = -1;
  int count = bar ? bar->property("count").toInt() : 0;
  for (int i = 0; i < count; i++)
  {
    QQuickItem* at = nullptr;
    QMetaObject::invokeMethod(bar, "itemAt", Qt::DirectConnection, Q_RETURN_ARG(QQuickItem*, at), Q_ARG(int, i));
    if (at == button)
    {
      old_index = i;
      break;
    }
  }

  iupAttribSet(ih, "_IUPTABS_REORDERING", "1");
  IupReparent(child, ih, ref_child);
  iupAttribSet(ih, "_IUPTABS_REORDERING", nullptr);

  int new_index = qmlTabsButtonIndex(ih, child);
  if (old_index >= 0 && new_index != old_index)
  {
    iupAttribSet(ih, "_IUPQML_IGNORE_CHANGE", "1");
    QMetaObject::invokeMethod(bar, "moveItem", Qt::DirectConnection, Q_ARG(int, old_index), Q_ARG(int, new_index));
    iupAttribSet(ih, "_IUPQML_IGNORE_CHANGE", nullptr);
  }

  if (current)
  {
    iupAttribSet(ih, "_IUPQML_IGNORE_CHANGE", "1");
    iupdrvTabsSetCurrentTab(ih, IupGetChildPos(ih, current));
    iupAttribSet(ih, "_IUPQML_IGNORE_CHANGE", nullptr);
  }
}

static int qmlTabsSetAllowReorderAttrib(Ihandle* ih, const char* value)
{
  for (Ihandle* child = ih->firstchild; child; child = child->brother)
  {
    auto* button = reinterpret_cast<QQuickItem*>(iupAttribGet(child, "_IUPQML_TABBUTTON"));
    if (button)
      button->setProperty("iupReorder", iupStrBoolean(value) ? true : false);
  }
  return 1;
}


static int qmlTabsSetTabVisibleAttrib(Ihandle* ih, int pos, const char* value)
{
  Ihandle* child = IupGetChild(ih, pos);
  if (!child || !ih->handle)
    return 0;

  iupAttribSet(ih, "_IUPQML_IGNORE_CHANGE", "1");
  if (iupStrBoolean(value))
    qmlTabsInsertButton(ih, child);
  else
  {
    iupTabsCheckCurrentTab(ih, pos, 0);
    qmlTabsRemoveButton(ih, child);
  }
  iupAttribSet(ih, "_IUPQML_IGNORE_CHANGE", nullptr);

  return 0;
}

static int qmlTabsSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  if (ih->handle)
  {
    QQuickItem* bar = qmlTabsGetBar(ih);
    if (bar)
      iupqmlUpdateItemFont(ih, bar);
    for (Ihandle* c = ih->firstchild; c; c = c->brother)
    {
      auto* button = reinterpret_cast<QQuickItem*>(iupAttribGet(c, "_IUPQML_TABBUTTON"));
      if (button)
        iupqmlUpdateItemFont(ih, button);
    }
  }

  return 1;
}

static int qmlTabsSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  QQuickItem* bar = qmlTabsGetBar(ih);
  if (bar)
    iupqmlSetPaletteColor(bar, "buttonText", QColor(r, g, b));

  return 1;
}

static int qmlTabsSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  auto* frame = reinterpret_cast<QQuickItem*>(iupAttribGet(ih, "_IUPQML_TABFRAME"));
  if (frame)
    frame->setProperty("color", QColor(r, g, b));

  return 1;
}

static char* qmlTabsGetClientSizeAttrib(Ihandle* ih)
{
  if (ih->handle)
  {
    int dw, dh;
    qmlTabsGetDecorSize(ih, &dw, &dh);
    return iupStrReturnIntInt(ih->currentwidth - dw, ih->currentheight - dh, 'x');
  }
  return nullptr;
}

static char* qmlTabsGetClientOffsetAttrib(Ihandle* ih)
{
  if (ih->handle)
  {
    int dx, dy;
    qmlTabsGetDecorOffset(ih, &dx, &dy);
    return iupStrReturnIntInt(dx, dy, 'x');
  }
  return nullptr;
}

/****************************************************************************
 * Children
 ****************************************************************************/

static void qmlTabsChildAddedMethod(Ihandle* ih, Ihandle* child)
{
  if (iupAttribGet(ih, "_IUPTABS_REORDERING"))
    return;

  if (!iupAttribGetHandleName(child))
    iupAttribSetHandleName(child);

  if (ih->handle)
  {
    auto* wrapper = reinterpret_cast<QQuickItem*>(ih->handle);
    int pos = IupGetChildPos(ih, child);
    char* tabtitle = iupAttribGet(child, "TABTITLE");
    char* tabimage = iupAttribGet(child, "TABIMAGE");

    if (!tabtitle)
    {
      tabtitle = iupAttribGetId(ih, "TABTITLE", pos);
      if (tabtitle)
        iupAttribSetStr(child, "TABTITLE", tabtitle);
    }

    if (!tabimage)
    {
      tabimage = iupAttribGetId(ih, "TABIMAGE", pos);
      if (tabimage)
        iupAttribSetStr(child, "TABIMAGE", tabimage);
    }

    if (!tabtitle && !tabimage)
      iupAttribSetStr(child, "TABTITLE", "     ");

    QQuickItem* container = iupqmlCreateItem("import QtQuick\nItem { clip: true; visible: false }");
    QQuickItem* button = iupqmlCreateItem(qmlTabsQml(qmlTabButtonQml).constData());
    if (!container || !button)
      return;

    container->setParentItem(wrapper);
    container->setParent(wrapper);
    iupAttribSet(child, "_IUPTAB_CONTAINER", reinterpret_cast<char*>(container));
    iupAttribSet(child, "_IUPQML_TABBUTTON", reinterpret_cast<char*>(button));

    iupqmlUpdateItemFont(ih, button);
    iupqmlUnsetArrowCursors(button);
    qmlTabsApplyButtonPadding(ih, child, button);
    qmlTabsUpdateButton(ih, child);

    {
      char* tip = iupAttribGetId(ih, "TABTIP", pos);
      if (tip)
        button->setProperty("iupTip", QString::fromUtf8(tip));
    }

    button->installEventFilter(new IupQmlTabsKeyFilter(button, ih));
    iupqmlConnect(button, "clicked()", [child](void**) {
      auto* b = reinterpret_cast<QQuickItem*>(iupAttribGet(child, "_IUPQML_TABBUTTON"));
      if (b)
        b->setProperty("checked", true);
    });

    button->setProperty("iupReorder", iupAttribGetBoolean(ih, "ALLOWREORDER") ? true : false);
    iupqmlConnect(button, "iupDragMoved(QPointF)", [ih, child](void** args) {
      qmlTabsDragMoved(ih, child, *static_cast<QPointF*>(args[1]));
    });
    iupqmlConnect(button, "iupDragged(QPointF)", [ih, child](void** args) {
      qmlTabsDragged(ih, child, *static_cast<QPointF*>(args[1]));
    });
    iupqmlConnect(button, "iupCloseClicked()", [ih, child](void**) {
      qmlTabsCloseClicked(ih, child);
    });

    iupqmlConnect(button, "iupRightClick()", [ih, child](void**) {
      IFni cb = reinterpret_cast<IFni>(IupGetCallback(ih, "RIGHTCLICK_CB"));
      if (cb && iupObjectCheck(child))
        cb(ih, IupGetChildPos(ih, child));
    });

    iupAttribSet(ih, "_IUPQML_IGNORE_CHANGE", "1");
    qmlTabsInsertButton(ih, child);
    iupAttribSet(ih, "_IUPQML_IGNORE_CHANGE", nullptr);

    if (pos == iupdrvTabsGetCurrentTab(ih))
    {
      container->setVisible(true);
      if (!iupAttribGet(ih, "_IUPQML_PREV_CHILD"))
        iupAttribSet(ih, "_IUPQML_PREV_CHILD", reinterpret_cast<char*>(child));
    }

    qmlTabsLayoutPages(ih);
  }
}

static void qmlTabsChildRemovedMethod(Ihandle* ih, Ihandle* child, int pos)
{
  if (iupAttribGet(ih, "_IUPTABS_REORDERING"))
    return;

  if (ih->handle)
  {
    auto* button = reinterpret_cast<QQuickItem*>(iupAttribGet(child, "_IUPQML_TABBUTTON"));
    auto* container = reinterpret_cast<QQuickItem*>(iupAttribGet(child, "_IUPTAB_CONTAINER"));

    if (button)
    {
      iupAttribSet(ih, "_IUPQML_REMOVED_CHILD", reinterpret_cast<char*>(child));
      iupAttribSetInt(ih, "_IUPQML_REMOVED_POS", pos);
      iupTabsCheckCurrentTab(ih, pos, 1);
      Ihandle* current = qmlTabsChildFromIndex(ih, qmlTabsGetBar(ih) ? qmlTabsGetBar(ih)->property("currentIndex").toInt() : -1);
      iupAttribSet(ih, "_IUPQML_REMOVED_CHILD", nullptr);

      iupAttribSet(ih, "_IUPQML_IGNORE_CHANGE", "1");
      qmlTabsRemoveButton(ih, child);
      if (current && current != child)
      {
        QQuickItem* bar = qmlTabsGetBar(ih);
        if (bar)
          bar->setProperty("currentIndex", qmlTabsButtonIndex(ih, current));
      }
      iupAttribSet(ih, "_IUPQML_IGNORE_CHANGE", nullptr);

      button->deleteLater();
    }

    if (container)
    {
      container->setVisible(false);
      container->setParentItem(nullptr);
      container->deleteLater();
    }

    if (reinterpret_cast<Ihandle*>(iupAttribGet(ih, "_IUPQML_PREV_CHILD")) == child)
      iupAttribSet(ih, "_IUPQML_PREV_CHILD", nullptr);
  }

  child->handle = nullptr;
  iupAttribSet(child, "_IUPTAB_CONTAINER", nullptr);
  iupAttribSet(child, "_IUPQML_TABBUTTON", nullptr);
}

/****************************************************************************
 * Map
 ****************************************************************************/

static int qmlTabsMapMethod(Ihandle* ih)
{
  QQuickItem* wrapper = iupqmlCreateItem("import QtQuick\nItem { }");
  if (!wrapper)
    return IUP_ERROR;

  QQuickItem* frame = iupqmlCreateItem("import QtQuick\nRectangle { color: \"transparent\"; border.width: 1; border.color: palette.mid }");
  if (frame)
  {
    frame->setParentItem(wrapper);
    frame->setParent(wrapper);
    iupAttribSet(ih, "_IUPQML_TABFRAME", reinterpret_cast<char*>(frame));
  }

  QQuickItem* bar = iupqmlCreateItem(qmlTabsQml(ih->data->type == ITABS_BOTTOM ?
    "TabBar { position: TabBar.Footer }" : "TabBar { position: TabBar.Header }").constData());
  if (!bar)
  {
    delete wrapper;
    return IUP_ERROR;
  }

  bar->setParentItem(wrapper);
  bar->setParent(wrapper);
  iupAttribSet(ih, "_IUPQML_TABBAR", reinterpret_cast<char*>(bar));
  if (qmlTabsIsVertical(ih))
  {
    iupqmlSetProperty(bar, "contentItem.orientation", static_cast<int>(Qt::Vertical));
    iupqmlSetProperty(bar, "background.visible", false);
  }

  ih->handle = reinterpret_cast<InativeHandle*>(wrapper);

  iupqmlUpdateItemFont(ih, bar);
  iupqmlSetIhandle(bar, ih);

  iupqmlConnect(bar, "currentIndexChanged()", [ih](void**) {
    qmlTabsCurrentChanged(ih);
  });
  iupqmlConnect(bar, "activeFocusChanged(bool)", [ih](void** args) {
    qmlTabsActiveFocusChanged(ih, *static_cast<bool*>(args[1]));
  });
  iupAttribSet(ih, "_IUPQML_FOCUS_FUNC", reinterpret_cast<char*>(qmlTabsFocusItem));

  iupqmlAddToParent(ih);
  iupqmlInstallFilter(ih, wrapper);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
    iupqmlSetCanFocus(bar, 0);

  if (ih->firstchild)
  {
    Ihandle* child;
    auto* current_child = reinterpret_cast<Ihandle*>(iupAttribGet(ih, "_IUPTABS_VALUE_HANDLE"));

    for (child = ih->firstchild; child; child = child->brother)
      qmlTabsChildAddedMethod(ih, child);

    if (current_child)
    {
      int pos = IupGetChildPos(ih, current_child);
      if (pos >= 0)
        iupdrvTabsSetCurrentTab(ih, pos);
      iupAttribSet(ih, "_IUPTABS_VALUE_HANDLE", nullptr);
    }
    else
      iupdrvTabsSetCurrentTab(ih, 0);
  }

  return IUP_NOERROR;
}

static void qmlTabsUnMapMethod(Ihandle* ih)
{
  for (Ihandle* c = ih->firstchild; c; c = c->brother)
  {
    iupAttribSet(c, "_IUPTAB_CONTAINER", nullptr);
    iupAttribSet(c, "_IUPQML_TABBUTTON", nullptr);
  }
  iupAttribSet(ih, "_IUPQML_TABBAR", nullptr);
  iupAttribSet(ih, "_IUPQML_TABFRAME", nullptr);
  iupAttribSet(ih, "_IUPQML_PREV_CHILD", nullptr);
  iupdrvBaseUnMapMethod(ih);
}

extern "C" IUP_SDK_API void iupdrvTabsInitClass(Iclass* ic)
{
  ic->Map = qmlTabsMapMethod;
  ic->UnMap = qmlTabsUnMapMethod;
  ic->LayoutUpdate = qmlTabsLayoutUpdateMethod;
  ic->ChildAdded = qmlTabsChildAddedMethod;
  ic->ChildRemoved = qmlTabsChildRemovedMethod;

  iupClassRegisterAttribute(ic, "FONT", nullptr, qmlTabsSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

  iupClassRegisterAttribute(ic, "BGCOLOR", nullptr, qmlTabsSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", nullptr, qmlTabsSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);

  iupClassRegisterAttribute(ic, "TABTYPE", iupTabsGetTabTypeAttrib, qmlTabsSetTabTypeAttrib, IUPAF_SAMEASSYSTEM, "TOP", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABORIENTATION", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALLOWREORDER", nullptr, qmlTabsSetAllowReorderAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MULTILINE", nullptr, nullptr, nullptr, nullptr, IUPAF_NOT_SUPPORTED | IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABPADDING", qmlTabsGetTabPaddingAttrib, qmlTabsSetTabPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "TABTITLE", iupTabsGetTitleAttrib, qmlTabsSetTabTitleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABTIP", nullptr, qmlTabsSetTabTipAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABIMAGE", nullptr, qmlTabsSetTabImageAttrib, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABVISIBLE", iupTabsGetTabVisibleAttrib, qmlTabsSetTabVisibleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "SHOWCLOSE", nullptr, qmlTabsSetShowCloseAttrib, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "CLIENTSIZE", qmlTabsGetClientSizeAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", qmlTabsGetClientOffsetAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);

  iupClassRegisterCallback(ic, "TABCHANGE_CB", "nn");
  iupClassRegisterCallback(ic, "TABCHANGEPOS_CB", "ii");
  iupClassRegisterCallback(ic, "TABCLOSE_CB", "i");
  iupClassRegisterCallback(ic, "RIGHTCLICK_CB", "i");
}
