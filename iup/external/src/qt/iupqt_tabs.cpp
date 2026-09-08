/** \file
 * \brief Tabs Control - Qt implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QTabWidget>
#include <QWidget>
#include <QLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QIcon>
#include <QPixmap>
#include <QString>
#include <QMouseEvent>
#include <QEvent>
#include <QProxyStyle>
#include <QStyleOption>
#include <QPainter>
#include <QFontMetrics>
#include <QApplication>

#include <functional>

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

#include "iupqt_drv.h"


/****************************************************************************
 * Custom Tab Style for Horizontal Text in Vertical Tabs
 ****************************************************************************/

class IupQtTabStyle : public QProxyStyle
{
private:
  Ihandle* ih;

public:
  IupQtTabStyle(Ihandle* ih_param) : QProxyStyle(), ih(ih_param) {}

  void drawControl(ControlElement element, const QStyleOption *option,
                   QPainter *painter, const QWidget *widget) const override
  {
    if (element == CE_TabBarTabLabel)
    {
      if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab*>(option))
      {
        bool isVerticalTab = (tab->shape == QTabBar::RoundedWest ||
                              tab->shape == QTabBar::RoundedEast ||
                              tab->shape == QTabBar::TriangularWest ||
                              tab->shape == QTabBar::TriangularEast);

        if (isVerticalTab && ih && ih->data->orientation == ITABS_HORIZONTAL)
        {
          QStyleOptionTab opt(*tab);
          painter->save();

          QString text = opt.text;
          QIcon icon = opt.icon;
          QRect rect = opt.rect;

          QFontMetrics fm(painter->font());
          QSize textSize = fm.size(Qt::TextSingleLine, text);

          int iconSize = 16;
          QRect iconRect, textRect;

          if (!icon.isNull())
          {
            int totalHeight = iconSize + 4 + textSize.height();
            int startY = rect.center().y() - totalHeight / 2;

            iconRect = QRect(rect.center().x() - iconSize / 2, startY, iconSize, iconSize);
            textRect = QRect(rect.left(), startY + iconSize + 4, rect.width(), textSize.height());
          }
          else
          {
            textRect = QRect(rect.left(), rect.center().y() - textSize.height() / 2, rect.width(), textSize.height());
          }

          if (!icon.isNull())
          {
            icon.paint(painter, iconRect);
          }

          Qt::Alignment alignment = Qt::AlignCenter;
          painter->drawText(textRect, alignment, text);

          painter->restore();
          return;
        }
      }
    }

    QProxyStyle::drawControl(element, option, painter, widget);
  }
};

/****************************************************************************
 * Custom Tab Bar with Enhanced Features
 ****************************************************************************/

class IupQtTabBar : public QTabBar
{
private:
  Ihandle* ih;
  std::function<void(int)> closeCallback;

protected:
  void mousePressEvent(QMouseEvent* event) override
  {
    if (event->button() == Qt::RightButton)
    {
      IFni cb = (IFni)IupGetCallback(ih, "RIGHTCLICK_CB");
      if (cb)
      {
        int pos = tabAt(event->pos());
        if (pos >= 0)
          cb(ih, pos);
      }
    }
    QTabBar::mousePressEvent(event);
  }

  void tabInserted(int index) override
  {
    QTabBar::tabInserted(index);
    updateTabCloseButton(index);
  }

  void updateTabCloseButton(int index)
  {
    if (!ih) return;

    Ihandle* child = IupGetChild(ih, index);
    if (!child) return;

    char* child_show_close = iupAttribGet(child, "SHOWCLOSE");
    int do_show_close = child_show_close ? iupStrBoolean(child_show_close) : ih->data->show_close;

    if (do_show_close)
    {
      QToolButton* close_btn = new QToolButton();
      close_btn->setIcon(style()->standardIcon(QStyle::SP_TitleBarCloseButton));
      close_btn->setAutoRaise(true);
      close_btn->setFixedSize(16, 16);

      /* Look up current tab index at click time (not capture time) to handle reordering */
      QObject::connect(close_btn, &QToolButton::clicked, [this, close_btn]() {
        if (closeCallback)
        {
          for (int i = 0; i < count(); i++)
          {
            if (tabButton(i, QTabBar::RightSide) == close_btn)
            {
              closeCallback(i);
              break;
            }
          }
        }
      });

      setTabButton(index, QTabBar::RightSide, close_btn);
    }
    else
    {
      QWidget* button = tabButton(index, QTabBar::RightSide);
      if (button)
      {
        setTabButton(index, QTabBar::RightSide, nullptr);
        delete button;
      }
    }
  }

  QSize tabSizeHint(int index) const override
  {
    QSize size = QTabBar::tabSizeHint(index);

    /* Qt sizes a vertical tab for rotated text; horizontal text needs the sides swapped */
    QTabBar::Shape tabShape = shape();
    if (tabShape == RoundedWest || tabShape == RoundedEast ||
        tabShape == TriangularWest || tabShape == TriangularEast)
    {
      if (ih && ih->data->orientation == ITABS_HORIZONTAL)
      {
        return QSize(size.height(), size.width());
      }
    }

#ifdef Q_OS_MAC
    /* QMacStyle::tabLayout() reserves icon space on both sides of the text rect,
       while tabSizeHint() counts the icon width once. */
    QIcon icon = tabIcon(index);
    if (!icon.isNull())
    {
      int iconExtent = style()->pixelMetric(QStyle::PM_TabBarIconSize, nullptr, this);
      int stylePadding = style()->pixelMetric(QStyle::PM_TabBarTabHSpace, nullptr, this) / 2;
      size.setWidth(size.width() + iconExtent + stylePadding + 4);
    }
#endif

    return size;
  }

public:
  IupQtTabBar(Ihandle* ih_param) : QTabBar(), ih(ih_param), closeCallback(nullptr)
  {
    setStyle(new IupQtTabStyle(ih_param));
  }

  void setIhandle(Ihandle* ih_param) { ih = ih_param; }

  void setCloseCallback(std::function<void(int)> callback) { closeCallback = callback; }

  void updateAllTabCloseButtons()
  {
    for (int i = 0; i < count(); i++)
      updateTabCloseButton(i);
  }
};

/****************************************************************************
 * Custom Tab Widget
 ****************************************************************************/

class IupQtTabWidget;

static void qtTabsHandleCurrentChanged(IupQtTabWidget* tabs, int index, Ihandle* ih, int* prev_index);
static void qtTabsHandleTabCloseRequested(IupQtTabWidget* tabs, int index, Ihandle* ih);
static void qtTabsHandleTabMoved(int from, int to, Ihandle* ih);

class IupQtTabWidget : public QTabWidget
{
private:
  Ihandle* ih;
  int prev_index;

public:
  IupQtTabWidget(Ihandle* ih_param) : QTabWidget(), ih(ih_param), prev_index(-1)
  {
    IupQtTabBar* custom_bar = new IupQtTabBar(ih);
    setTabBar(custom_bar);

    custom_bar->setCloseCallback([this, ih_param](int index) {
      qtTabsHandleTabCloseRequested(this, index, ih_param);
    });

    QObject::connect(this, &QTabWidget::currentChanged, [this, ih_param](int index) {
      qtTabsHandleCurrentChanged(this, index, ih_param, &prev_index);
    });

    QObject::connect(custom_bar, &QTabBar::tabMoved, [ih_param](int from, int to) {
      qtTabsHandleTabMoved(from, to, ih_param);
    });
  }

  void setIhandle(Ihandle* ih_param)
  {
    ih = ih_param;
    IupQtTabBar* tab_bar = (IupQtTabBar*)tabBar();
    if (tab_bar)
    {
      tab_bar->setIhandle(ih_param);

      tab_bar->setCloseCallback([this, ih_param](int index) {
        qtTabsHandleTabCloseRequested(this, index, ih_param);
      });
    }
  }

  void setPrevIndex(int index) { prev_index = index; }

  void updateAllTabCloseButtons()
  {
    IupQtTabBar* tab_bar = (IupQtTabBar*)tabBar();
    if (tab_bar)
      tab_bar->updateAllTabCloseButtons();
  }
};

/****************************************************************************
 * Static Callback Function Implementations
 ****************************************************************************/

static void qtTabsHandleCurrentChanged(IupQtTabWidget* tabs, int index, Ihandle* ih, int* prev_index)
{
  if (!ih)
    return;

  if (index < 0)
    return;

  QWidget* current_page = tabs->widget(index);
  QWidget* prev_page = *prev_index >= 0 ? tabs->widget(*prev_index) : nullptr;

  Ihandle* child = nullptr;
  Ihandle* prev_child = nullptr;

  for (Ihandle* c = ih->firstchild; c; c = c->brother)
  {
    QWidget* page = (QWidget*)iupAttribGet(c, "_IUPTAB_PAGE");
    if (page == current_page)
      child = c;
    if (page == prev_page)
      prev_child = c;
  }

  if (prev_child)
  {
    QWidget* prev_container = (QWidget*)iupAttribGet(prev_child, "_IUPTAB_CONTAINER");
    if (prev_container)
      prev_container->hide();
  }

  if (child)
  {
    QWidget* container = (QWidget*)iupAttribGet(child, "_IUPTAB_CONTAINER");
    if (container)
      container->show();
  }

  if (!iupAttribGet(ih, "_IUPQT_IGNORE_CHANGE"))
  {
    IFnnn cb = (IFnnn)IupGetCallback(ih, "TABCHANGE_CB");
    if (cb)
    {
      cb(ih, child, prev_child);
    }
    else
    {
      IFnii cb2 = (IFnii)IupGetCallback(ih, "TABCHANGEPOS_CB");
      if (cb2 && *prev_index >= 0)
        cb2(ih, index, *prev_index);
    }
  }

  *prev_index = index;
}

static void qtTabsHandleTabCloseRequested(IupQtTabWidget* tabs, int index, Ihandle* ih)
{
  if (!ih) return;

  IFni cb = (IFni)IupGetCallback(ih, "TABCLOSE_CB");
  int ret = IUP_DEFAULT;

  if (cb)
    ret = cb(ih, index);

  if (ret == IUP_CONTINUE)
  {
    Ihandle* child = IupGetChild(ih, index);
    if (child)
    {
      IupDestroy(child);
      IupRefreshChildren(ih);
    }
  }
  else if (ret == IUP_DEFAULT)
  {
    Ihandle* child = IupGetChild(ih, index);
    if (child)
    {
      QWidget* tab_page = (QWidget*)iupAttribGet(child, "_IUPTAB_PAGE");
      if (tab_page)
      {
        int idx = tabs->indexOf(tab_page);
        if (idx >= 0)
          tabs->removeTab(idx);
      }
    }
  }
}

static void qtTabsHandleTabMoved(int from, int to, Ihandle* ih)
{
  if (!ih || from == to) return;

  if (iupAttribGet(ih, "_IUPTABS_REORDERING"))
    return;

  IFnii cb = (IFnii)IupGetCallback(ih, "REORDER_CB");
  if (cb && cb(ih, from, to) == IUP_IGNORE)
  {
    iupAttribSet(ih, "_IUPTABS_REORDERING", "1");
    IupQtTabWidget* tabs = (IupQtTabWidget*)ih->handle;
    if (tabs)
      tabs->tabBar()->moveTab(to, from);
    iupAttribSet(ih, "_IUPTABS_REORDERING", NULL);
    return;
  }

  Ihandle* child = IupGetChild(ih, from);
  if (child)
  {
    Ihandle* ref_child;
    if (from < to)
      ref_child = IupGetChild(ih, to + 1);
    else
      ref_child = IupGetChild(ih, to);
    iupAttribSet(ih, "_IUPTABS_REORDERING", "1");
    IupReparent(child, ih, ref_child);
    iupAttribSet(ih, "_IUPTABS_REORDERING", NULL);
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

extern "C" IUP_SDK_API void iupdrvTabsSetCurrentTab(Ihandle* ih, int pos)
{
  IupQtTabWidget* tabs = (IupQtTabWidget*)ih->handle;
  if (!tabs)
    return;

  iupAttribSet(ih, "_IUPQT_IGNORE_CHANGE", "1");
  tabs->setCurrentIndex(pos);
  tabs->setPrevIndex(pos);
  iupAttribSet(ih, "_IUPQT_IGNORE_CHANGE", nullptr);
}

extern "C" IUP_SDK_API int iupdrvTabsGetCurrentTab(Ihandle* ih)
{
  QTabWidget* tabs = (QTabWidget*)ih->handle;
  if (!tabs)
    return -1;

  return tabs->currentIndex();
}

extern "C" IUP_SDK_API void iupdrvTabsGetTabSize(Ihandle* ih, const char* tab_title, const char* tab_image, int* tab_width, int* tab_height)
{
  int width = 0;
  int height = 0;

  if (ih->handle)
  {
    QTabWidget* tabWidget = (QTabWidget*)ih->handle;
    QTabBar* tabBar = tabWidget->tabBar();
    if (tabBar && tabBar->count() > 0)
    {
      QString searchTitle = QString::fromUtf8(tab_title ? tab_title : "");

      for (int i = 0; i < tabBar->count(); i++)
      {
        QString tabText = tabBar->tabText(i);
        QIcon tabIcon = tabBar->tabIcon(i);

        bool titleMatch = (tabText == searchTitle);
        bool imageMatch = (tab_image != nullptr) == (!tabIcon.isNull());

        if (titleMatch && imageMatch)
        {
          QRect rect = tabBar->tabRect(i);
          width = rect.width();
          height = rect.height();

          if (tab_width) *tab_width = width;
          if (tab_height) *tab_height = height;
          return;
        }
      }

      QSize barSize = tabBar->sizeHint();
      width = barSize.width() / tabBar->count();
      height = barSize.height();

      if (tab_width) *tab_width = width;
      if (tab_height) *tab_height = height;
      return;
    }
  }

  int text_width = 0;
  int text_height = 0;

  if (tab_title)
  {
    text_width = iupdrvFontGetStringWidth(ih, tab_title);
    iupdrvFontGetCharSize(ih, NULL, &text_height);
    width = text_width;
    height = text_height;
  }

  if (tab_image)
  {
    void* img = iupImageGetImage(tab_image, ih, 0, NULL);
    if (img)
    {
      int img_w, img_h;
      iupdrvImageGetInfo(img, &img_w, &img_h, NULL);
      iupTabsScaleImageSize(ih, img_w, img_h, &img_w, &img_h);
      width += img_w;
      width += 4;  /* Qt adds 4px padding when icon is present */
      if (img_h > height)
        height = img_h;
    }
  }

  QStyle* style = QApplication::style();
  if (style)
  {
    int hspace = style->pixelMetric(QStyle::PM_TabBarTabHSpace, nullptr, nullptr);
    int vspace = style->pixelMetric(QStyle::PM_TabBarTabVSpace, nullptr, nullptr);

    QStyleOptionTab opt;
    opt.text = QString::fromUtf8(tab_title ? tab_title : "");
    opt.shape = QTabBar::RoundedNorth;

    QSize contentSize(width, height);
    QSize fullSize = style->sizeFromContents(QStyle::CT_TabBarTab, &opt, contentSize, nullptr);

    width = fullSize.width() + hspace;
    height = fullSize.height() + vspace;
  }
  else
  {
    width += 24;
    height += 8;
  }

  if (tab_width) *tab_width = width;
  if (tab_height) *tab_height = height;
}

extern "C" IUP_SDK_API int iupdrvTabsIsTabVisible(Ihandle* child, int pos)
{
  Ihandle* ih = IupGetParent(child);
  if (!ih || !ih->handle)
    return 1;

  QTabWidget* tabs = (QTabWidget*)ih->handle;
  QWidget* tab_page = (QWidget*)iupAttribGet(child, "_IUPTAB_PAGE");

  (void)pos;

  if (tab_page)
    return tabs->indexOf(tab_page) >= 0 ? 1 : 0;

  return 0;
}

/****************************************************************************
 * Update Functions
 ****************************************************************************/

static void qtTabsUpdatePageFont(Ihandle* ih)
{
  QTabWidget* tabs = (QTabWidget*)ih->handle;
  QFont* font = iupqtGetQFont(iupGetFontValue(ih));

  if (font)
  {
    tabs->setFont(*font);
    tabs->tabBar()->setFont(*font);
  }
}

static void qtTabsUpdatePageBgColor(Ihandle* ih, unsigned char r, unsigned char g, unsigned char b)
{
  Ihandle* child;

  for (child = ih->firstchild; child; child = child->brother)
  {
    QWidget* tab_container = (QWidget*)iupAttribGet(child, "_IUPTAB_CONTAINER");
    if (tab_container)
    {
      QPalette palette = tab_container->palette();
      palette.setColor(QPalette::Window, QColor(r, g, b));
      tab_container->setPalette(palette);
      tab_container->setAutoFillBackground(true);
    }
  }
}

static void qtTabsUpdatePageFgColor(Ihandle* ih, unsigned char r, unsigned char g, unsigned char b)
{
  QTabWidget* tabs = (QTabWidget*)ih->handle;
  QTabBar* tab_bar = tabs->tabBar();

  QString styleSheet = tab_bar->styleSheet();
  styleSheet += QString("QTabBar::tab { color: rgb(%1, %2, %3); }").arg(r).arg(g).arg(b);
  tab_bar->setStyleSheet(styleSheet);
}

static void qtTabsUpdateTabType(Ihandle* ih)
{
  QTabWidget* tabs = (QTabWidget*)ih->handle;

  switch (ih->data->type)
  {
    case ITABS_BOTTOM:
      tabs->setTabPosition(QTabWidget::South);
      break;
    case ITABS_LEFT:
      tabs->setTabPosition(QTabWidget::West);
      break;
    case ITABS_RIGHT:
      tabs->setTabPosition(QTabWidget::East);
      break;
    case ITABS_TOP:
    default:
      tabs->setTabPosition(QTabWidget::North);
      break;
  }
}

/****************************************************************************
 * Attribute Setters
 ****************************************************************************/

static int qtTabsSetTabPaddingAttrib(Ihandle* ih, const char* value)
{
  int horiz_padding = 0, vert_padding = 0;
  iupStrToIntInt(value, &horiz_padding, &vert_padding, 'x');

  ih->data->horiz_padding = horiz_padding;
  ih->data->vert_padding = vert_padding;

  if (ih->handle)
  {
    QTabWidget* tabs = (QTabWidget*)ih->handle;
    QString styleSheet = tabs->tabBar()->styleSheet();

    QStringList lines = styleSheet.split(';');
    QStringList filtered;
    for (const QString& line : lines)
    {
      if (!line.contains("padding"))
        filtered.append(line);
    }
    styleSheet = filtered.join(";");

    styleSheet += QString("QTabBar::tab { padding: %1px %2px; }").arg(vert_padding).arg(horiz_padding);
    tabs->tabBar()->setStyleSheet(styleSheet);
    return 0;
  }

  return 1; /* Store until mapped */
}

static char* qtTabsGetTabPaddingAttrib(Ihandle* ih)
{
  return iupStrReturnIntInt(ih->data->horiz_padding, ih->data->vert_padding, 'x');
}

static int qtTabsSetMultilineAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle) /* Allow to set only before mapping */
    return 0;

  if (iupStrBoolean(value))
    ih->data->is_multiline = 1;
  else
  {
    if (ih->data->type == ITABS_BOTTOM || ih->data->type == ITABS_TOP)
      ih->data->is_multiline = 0;
    else
      ih->data->is_multiline = 1;
  }

  return 0;
}

static char* qtTabsGetMultilineAttrib(Ihandle* ih)
{
  return iupStrReturnBoolean(ih->data->is_multiline);
}

static int qtTabsSetTabTypeAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle) /* Allow to set only before mapping */
    return 0;

  /* TABTYPE sets only the tab position, not the text orientation */
  if (iupStrEqualNoCase(value, "BOTTOM"))
    ih->data->type = ITABS_BOTTOM;
  else if (iupStrEqualNoCase(value, "LEFT"))
  {
    ih->data->type = ITABS_LEFT;
    ih->data->is_multiline = 1;
  }
  else if (iupStrEqualNoCase(value, "RIGHT"))
  {
    ih->data->type = ITABS_RIGHT;
    ih->data->is_multiline = 1;
  }
  else /* "TOP" */
    ih->data->type = ITABS_TOP;

  return 0;
}

static int qtTabsSetTabOrientationAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle) /* Allow to set only before mapping */
    return 0;

  if (iupStrEqualNoCase(value, "VERTICAL"))
    ih->data->orientation = ITABS_VERTICAL;
  else  /* "HORIZONTAL" */
    ih->data->orientation = ITABS_HORIZONTAL;

  return 0;
}

static int qtTabsSetTabTitleAttrib(Ihandle* ih, int pos, const char* value)
{
  Ihandle* child = IupGetChild(ih, pos);
  if (child)
  {
    iupAttribSetStr(child, "TABTITLE", value);

    if (ih->handle)
    {
      QTabWidget* tabs = (QTabWidget*)ih->handle;
      QWidget* tab_page = (QWidget*)iupAttribGet(child, "_IUPTAB_PAGE");

      if (tab_page)
      {
        int index = tabs->indexOf(tab_page);
        if (index >= 0)
        {
          QString title = value ? QString::fromUtf8(value) : QString();
          tabs->setTabText(index, title);
        }
      }
    }
  }

  return 1;
}

static int qtTabsSetTabTipAttrib(Ihandle* ih, int pos, const char* value)
{
  Ihandle* child = IupGetChild(ih, pos);
  if (child && ih->handle)
  {
    QTabWidget* tabs = (QTabWidget*)ih->handle;
    QWidget* tab_page = (QWidget*)iupAttribGet(child, "_IUPTAB_PAGE");

    if (tab_page)
    {
      int index = tabs->indexOf(tab_page);
      if (index >= 0)
        tabs->setTabToolTip(index, value ? QString::fromUtf8(value) : QString());
    }
  }

  return 0;
}

static int qtTabsSetTabImageAttrib(Ihandle* ih, int pos, const char* value)
{
  Ihandle* child = IupGetChild(ih, pos);
  if (child)
  {
    iupAttribSetStr(child, "TABIMAGE", value);

    if (ih->handle)
    {
      QTabWidget* tabs = (QTabWidget*)ih->handle;
      QWidget* tab_page = (QWidget*)iupAttribGet(child, "_IUPTAB_PAGE");

      if (tab_page)
      {
        int index = tabs->indexOf(tab_page);
        if (index >= 0)
        {
          if (value)
          {
            QPixmap* pixbuf = (QPixmap*)iupImageGetImage(value, ih, 0, nullptr);
            if (pixbuf)
              tabs->setTabIcon(index, QIcon(*pixbuf));
            else
              tabs->setTabIcon(index, QIcon());
          }
          else
            tabs->setTabIcon(index, QIcon());
        }
      }
    }
  }

  return 1;
}

static int qtTabsSetTabVisibleAttrib(Ihandle* ih, int pos, const char* value)
{
  Ihandle* child = IupGetChild(ih, pos);
  if (!child)
    return 0;

  if (ih->handle)
  {
    QTabWidget* tabs = (QTabWidget*)ih->handle;
    QWidget* tab_page = (QWidget*)iupAttribGet(child, "_IUPTAB_PAGE");

    if (tab_page)
    {
      int index = tabs->indexOf(tab_page);

      if (iupStrBoolean(value))
      {
        if (index < 0)
        {
          char* tabtitle = iupAttribGet(child, "TABTITLE");
          char* tabimage = iupAttribGet(child, "TABIMAGE");

          QString title = tabtitle ? QString::fromUtf8(tabtitle) : QString();
          QIcon icon;

          if (tabimage)
          {
            QPixmap* pixmap = (QPixmap*)iupImageGetImage(tabimage, ih, 0, nullptr);
            if (pixmap)
              icon = QIcon(*pixmap);
          }

          tabs->insertTab(pos, tab_page, icon, title);
        }
      }
      else
      {
        if (index >= 0)
        {
          iupTabsCheckCurrentTab(ih, pos, 0);
          tabs->removeTab(index);
        }
      }
    }
  }

  return 0;
}

static int qtTabsSetShowCloseAttrib(Ihandle* ih, int pos, const char* value)
{
  if (pos == IUP_INVALID_ID)
  {
    ih->data->show_close = iupStrBoolean(value);

    if (ih->handle)
    {
      IupQtTabWidget* tabs = (IupQtTabWidget*)ih->handle;
      tabs->updateAllTabCloseButtons();
    }

    return 1;
  }
  else
  {
    Ihandle* child = IupGetChild(ih, pos);
    if (child)
      iupAttribSetStr(child, "SHOWCLOSE", value);

    if (ih->handle)
    {
      IupQtTabWidget* tabs = (IupQtTabWidget*)ih->handle;
      IupQtTabBar* tab_bar = (IupQtTabBar*)tabs->tabBar();

      if (iupStrBoolean(value))
      {
        QToolButton* close_btn = new QToolButton();
        close_btn->setIcon(tabs->style()->standardIcon(QStyle::SP_TitleBarCloseButton));
        close_btn->setAutoRaise(true);
        close_btn->setFixedSize(16, 16);

        QObject::connect(close_btn, &QToolButton::clicked, [tabs, close_btn, ih]() {
          IupQtTabBar* bar = (IupQtTabBar*)tabs->tabBar();
          for (int i = 0; i < bar->count(); i++)
          {
            if (bar->tabButton(i, QTabBar::RightSide) == close_btn)
            {
              qtTabsHandleTabCloseRequested(tabs, i, ih);
              break;
            }
          }
        });

        tab_bar->setTabButton(pos, QTabBar::RightSide, close_btn);
      }
      else
      {
        QWidget* button = tab_bar->tabButton(pos, QTabBar::RightSide);
        if (button)
        {
          tab_bar->setTabButton(pos, QTabBar::RightSide, nullptr);
          delete button;
        }
      }
    }

    return 0;
  }
}

static int qtTabsSetAllowReorderAttrib(Ihandle* ih, const char* value)
{
  if (ih->handle)
  {
    QTabWidget* tabs = (QTabWidget*)ih->handle;
    tabs->setMovable(iupStrBoolean(value));
  }

  return 1;
}

static int qtTabsSetFontAttrib(Ihandle* ih, const char* value)
{
  if (!iupdrvSetFontAttrib(ih, value))
    return 0;

  if (ih->handle)
    qtTabsUpdatePageFont(ih);

  return 1;
}

static int qtTabsSetFgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  if (ih->handle)
    qtTabsUpdatePageFgColor(ih, r, g, b);

  return 1;
}

static int qtTabsSetBgColorAttrib(Ihandle* ih, const char* value)
{
  unsigned char r, g, b;
  if (!iupStrToRGB(value, &r, &g, &b))
    return 0;

  if (ih->handle)
  {
    QTabWidget* tabs = (QTabWidget*)ih->handle;
    QPalette palette = tabs->palette();
    palette.setColor(QPalette::Window, QColor(r, g, b));
    tabs->setPalette(palette);
    tabs->setAutoFillBackground(true);

    qtTabsUpdatePageBgColor(ih, r, g, b);
  }

  return 1;
}

static char* qtTabsGetClientSizeAttrib(Ihandle* ih)
{
  if (ih->handle)
  {
    QTabWidget* tabs = (QTabWidget*)ih->handle;
    QRect content_rect = tabs->contentsRect();
    QTabBar* tab_bar = tabs->tabBar();

    int width = content_rect.width();
    int height = content_rect.height();

    if (tabs->tabPosition() == QTabWidget::North || tabs->tabPosition() == QTabWidget::South)
      height -= tab_bar->height();
    else
      width -= tab_bar->width();

    return iupStrReturnIntInt(width, height, 'x');
  }

  return nullptr;
}

static char* qtTabsGetClientOffsetAttrib(Ihandle* ih)
{
  if (ih->handle)
  {
    QTabWidget* tabs = (QTabWidget*)ih->handle;
    QTabBar* tab_bar = tabs->tabBar();

    int x = 0, y = 0;

    if (tabs->tabPosition() == QTabWidget::North)
      y = tab_bar->height();
    else if (tabs->tabPosition() == QTabWidget::West)
      x = tab_bar->width();

    return iupStrReturnIntInt(x, y, 'x');
  }

  return nullptr;
}

/****************************************************************************
 * Child Add/Remove Methods
 ****************************************************************************/

static void qtTabsChildAddedMethod(Ihandle* ih, Ihandle* child)
{
  if (iupAttribGet(ih, "_IUPTABS_REORDERING"))
    return;

  if (!iupAttribGetHandleName(child))
    iupAttribSetHandleName(child);

  if (ih->handle)
  {
    IupQtTabWidget* tabs = (IupQtTabWidget*)ih->handle;
    QWidget* tab_page;
    QWidget* tab_container;
    char* tabtitle;
    char* tabimage;
    int pos;
    unsigned char r, g, b;

    pos = IupGetChildPos(ih, child);

    tab_page = new QWidget();
    QVBoxLayout* pageLayout = new QVBoxLayout(tab_page);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(0);

    tab_container = new QWidget();
    pageLayout->addWidget(tab_container);
    tab_container->show();

    tabtitle = iupAttribGet(child, "TABTITLE");
    if (!tabtitle)
    {
      tabtitle = iupAttribGetId(ih, "TABTITLE", pos);
      if (tabtitle)
        iupAttribSetStr(child, "TABTITLE", tabtitle);
    }

    tabimage = iupAttribGet(child, "TABIMAGE");
    if (!tabimage)
    {
      tabimage = iupAttribGetId(ih, "TABIMAGE", pos);
      if (tabimage)
        iupAttribSetStr(child, "TABIMAGE", tabimage);
    }

    if (!tabtitle && !tabimage)
      tabtitle = (char*)"     ";

    iupAttribSet(ih, "_IUPQT_IGNORE_CHANGE", "1");

    QString title = QString::fromUtf8(tabtitle ? tabtitle : "");

    if (tabimage)
    {
      QPixmap* pixbuf = (QPixmap*)iupImageGetImage(tabimage, ih, 0, nullptr);
      if (pixbuf)
        tabs->insertTab(pos, tab_page, QIcon(*pixbuf), title);
      else
        tabs->insertTab(pos, tab_page, title);
    }
    else
      tabs->insertTab(pos, tab_page, title);

    iupAttribSet(child, "_IUPTAB_CONTAINER", (char*)tab_container);
    iupAttribSet(child, "_IUPTAB_PAGE", (char*)tab_page);

    iupStrToRGB(IupGetAttribute(ih, "BGCOLOR"), &r, &g, &b);
    QPalette palette = tab_container->palette();
    palette.setColor(QPalette::Window, QColor(r, g, b));
    tab_container->setPalette(palette);
    tab_container->setAutoFillBackground(true);

    iupAttribSet(ih, "_IUPQT_IGNORE_CHANGE", nullptr);

    if (pos != iupdrvTabsGetCurrentTab(ih))
      tab_container->hide();
  }
}

static void qtTabsChildRemovedMethod(Ihandle* ih, Ihandle* child, int pos)
{
  if (iupAttribGet(ih, "_IUPTABS_REORDERING"))
    return;

  if (ih->handle)
  {
    QTabWidget* tabs = (QTabWidget*)ih->handle;
    QWidget* tab_page = (QWidget*)iupAttribGet(child, "_IUPTAB_PAGE");

    if (tab_page)
    {
      int index = tabs->indexOf(tab_page);

      if (index >= 0)
      {
        iupTabsCheckCurrentTab(ih, pos, 1);

        iupAttribSet(ih, "_IUPQT_IGNORE_CHANGE", "1");
        tabs->removeTab(index);
        iupAttribSet(ih, "_IUPQT_IGNORE_CHANGE", nullptr);

        delete tab_page;
      }
    }
  }

  child->handle = nullptr;
  iupAttribSet(child, "_IUPTAB_CONTAINER", nullptr);
  iupAttribSet(child, "_IUPTAB_PAGE", nullptr);
}

/****************************************************************************
 * Map Method
 ****************************************************************************/

static int qtTabsMapMethod(Ihandle* ih)
{
  IupQtTabWidget* tabs = new IupQtTabWidget(ih);

  ih->handle = (InativeHandle*)tabs;
  tabs->setIhandle(ih);

  tabs->setTabsClosable(false); /* close buttons are added per tab */
  tabs->setMovable(false);

  if (!iupAttribGetBoolean(ih, "CANFOCUS"))
  {
    iupqtSetCanFocus(tabs, 0);
    iupqtSetCanFocus(tabs->tabBar(), 0);
  }

  qtTabsUpdateTabType(ih);

  {
    int icon_w, icon_h;
    iupTabsGetImageBoxSize(ih, &icon_w, &icon_h);
    if (icon_w > 0 && icon_h > 0)
      tabs->setIconSize(QSize(icon_w, icon_h));
  }

  /* Qt has no multiline tabs, only scroll buttons */
  if (ih->data->is_multiline)
  {
    tabs->tabBar()->setUsesScrollButtons(false);
    tabs->tabBar()->setExpanding(false);
  }
  else
  {
    tabs->tabBar()->setUsesScrollButtons(true);
    tabs->tabBar()->setExpanding(false);
  }

  if (ih->data->horiz_padding != 0 || ih->data->vert_padding != 0)
  {
    QString styleSheet = QString(
      "QTabBar::tab { padding: %1px %2px; }"
    ).arg(ih->data->vert_padding).arg(ih->data->horiz_padding);
    tabs->tabBar()->setStyleSheet(styleSheet);
  }

  iupqtAddToParent(ih);

  /* Create pages and tabs */
  if (ih->firstchild)
  {
    Ihandle* child;
    Ihandle* current_child = (Ihandle*)iupAttribGet(ih, "_IUPTABS_VALUE_HANDLE");

    for (child = ih->firstchild; child; child = child->brother)
      qtTabsChildAddedMethod(ih, child);

    if (current_child)
    {
      int pos = IupGetChildPos(ih, current_child);
      if (pos >= 0)
      {
        tabs->setCurrentIndex(pos);
        tabs->setPrevIndex(pos);
      }

      iupAttribSet(ih, "_IUPTABS_VALUE_HANDLE", nullptr);
    }
    else
    {
      tabs->setPrevIndex(0);
    }
  }

  return IUP_NOERROR;
}

/****************************************************************************
 * Class Initialization
 ****************************************************************************/

extern "C" IUP_SDK_API void iupdrvTabsInitClass(Iclass* ic)
{
  /* Driver Dependent Class functions */
  ic->Map = qtTabsMapMethod;
  ic->ChildAdded = qtTabsChildAddedMethod;
  ic->ChildRemoved = qtTabsChildRemovedMethod;

  /* Driver Dependent Attribute functions */

  /* Common */
  iupClassRegisterAttribute(ic, "FONT", nullptr, qtTabsSetFontAttrib, IUPAF_SAMEASSYSTEM, "DEFAULTFONT", IUPAF_NOT_MAPPED);

  /* Visual */
  iupClassRegisterAttribute(ic, "BGCOLOR", nullptr, qtTabsSetBgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGBGCOLOR", IUPAF_DEFAULT);
  iupClassRegisterAttribute(ic, "FGCOLOR", nullptr, qtTabsSetFgColorAttrib, IUPAF_SAMEASSYSTEM, "DLGFGCOLOR", IUPAF_DEFAULT);

  /* IupTabs only */
  iupClassRegisterAttribute(ic, "TABTYPE", iupTabsGetTabTypeAttrib, qtTabsSetTabTypeAttrib, IUPAF_SAMEASSYSTEM, "TOP", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABORIENTATION", iupTabsGetTabOrientationAttrib, qtTabsSetTabOrientationAttrib, IUPAF_SAMEASSYSTEM, "HORIZONTAL", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "ALLOWREORDER", nullptr, qtTabsSetAllowReorderAttrib, IUPAF_SAMEASSYSTEM, "NO", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "MULTILINE", qtTabsGetMultilineAttrib, qtTabsSetMultilineAttrib, nullptr, nullptr, IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "TABPADDING", qtTabsGetTabPaddingAttrib, qtTabsSetTabPaddingAttrib, IUPAF_SAMEASSYSTEM, "0x0", IUPAF_NOT_MAPPED | IUPAF_NO_INHERIT);

  iupClassRegisterAttributeId(ic, "TABTITLE", iupTabsGetTitleAttrib, qtTabsSetTabTitleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABTIP", NULL, qtTabsSetTabTipAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABIMAGE", nullptr, qtTabsSetTabImageAttrib, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABVISIBLE", iupTabsGetTabVisibleAttrib, qtTabsSetTabVisibleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "SHOWCLOSE", nullptr, qtTabsSetShowCloseAttrib, IUPAF_NOT_MAPPED|IUPAF_NO_INHERIT);

  iupClassRegisterAttribute(ic, "CLIENTSIZE", qtTabsGetClientSizeAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", qtTabsGetClientOffsetAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);

  /* IupTabs Callbacks */
  iupClassRegisterCallback(ic, "TABCHANGE_CB", "nn");
  iupClassRegisterCallback(ic, "TABCHANGEPOS_CB", "ii");
  iupClassRegisterCallback(ic, "TABCLOSE_CB", "i");
  iupClassRegisterCallback(ic, "RIGHTCLICK_CB", "i");
}
