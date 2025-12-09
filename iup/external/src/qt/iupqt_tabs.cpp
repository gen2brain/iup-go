/** \file
 * \brief Tabs Control - Qt implementation
 *
 * See Copyright Notice in "iup.h"
 */

#include <QTabWidget>
#include <QTabBar>
#include <QWidget>
#include <QLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QIcon>
#include <QPixmap>
#include <QString>
#include <QMouseEvent>
#include <QEvent>
#include <QStyle>
#include <QProxyStyle>
#include <QStyleOption>
#include <QPainter>
#include <QFontMetrics>
#include <QApplication>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <functional>

extern "C" {
#include "iup.h"
#include "iupcbs.h"
#include "iup_object.h"
#include "iup_layout.h"
#include "iup_attrib.h"
#include "iup_str.h"
#include "iup_dialog.h"
#include "iup_drv.h"
#include "iup_drvfont.h"
#include "iup_stdcontrols.h"
#include "iup_image.h"
#include "iup_tabs.h"
}

#include "iupqt_drv.h"


/****************************************************************************
 * Custom Tab Style for Horizontal Text in Vertical Tabs
 ****************************************************************************/

class IupQtTabStyle : public QProxyStyle
{
public:
  IupQtTabStyle() : QProxyStyle() {}

  void drawControl(ControlElement element, const QStyleOption *option,
                   QPainter *painter, const QWidget *widget) const override
  {
    if (element == CE_TabBarTabLabel)
    {
      if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab*>(option))
      {
        /* Check if this is a vertical tab (West or East position) */
        bool isVertical = (tab->shape == QTabBar::RoundedWest ||
                          tab->shape == QTabBar::RoundedEast ||
                          tab->shape == QTabBar::TriangularWest ||
                          tab->shape == QTabBar::TriangularEast);

        if (isVertical)
        {
          QStyleOptionTab opt(*tab);
          painter->save();

          /* Get text and icon */
          QString text = opt.text;
          QIcon icon = opt.icon;
          QRect rect = opt.rect;

          /* Calculate text size */
          QFontMetrics fm(painter->font());
          QSize textSize = fm.size(Qt::TextSingleLine, text);

          /* Icon size (if present) */
          int iconSize = 16;
          QRect iconRect, textRect;

          if (!icon.isNull())
          {
            /* Position icon above text */
            int totalHeight = iconSize + 4 + textSize.height();
            int startY = rect.center().y() - totalHeight / 2;

            iconRect = QRect(rect.center().x() - iconSize / 2, startY, iconSize, iconSize);
            textRect = QRect(rect.left(), startY + iconSize + 4, rect.width(), textSize.height());
          }
          else
          {
            /* Center text */
            textRect = QRect(rect.left(), rect.center().y() - textSize.height() / 2,
                           rect.width(), textSize.height());
          }

          /* Draw icon if present */
          if (!icon.isNull())
          {
            icon.paint(painter, iconRect);
          }

          /* Draw text horizontally */
          Qt::Alignment alignment = Qt::AlignCenter;
          painter->drawText(textRect, alignment, text);

          painter->restore();
          return; /* Skip default drawing */
        }
      }
    }

    /* For all other cases (horizontal tabs), use default drawing */
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

    char* show_close = iupAttribGet(child, "SHOWCLOSE");
    if (!show_close)
      show_close = iupAttribGet(ih, "SHOWCLOSE");

    if (show_close && iupStrBoolean(show_close))
    {
      /* Create close button */
      QToolButton* close_btn = new QToolButton();
      close_btn->setIcon(style()->standardIcon(QStyle::SP_TitleBarCloseButton));
      close_btn->setAutoRaise(true);
      close_btn->setFixedSize(16, 16);

      /* Use lambda to avoid MOC - captures index and calls callback */
      QObject::connect(close_btn, &QToolButton::clicked, [this, index]() {
        if (closeCallback)
          closeCallback(index);
      });

      setTabButton(index, QTabBar::RightSide, close_btn);
    }
    else
    {
      /* Remove close button if exists */
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

    /* For vertical tabs (West/East), Qt calculates size assuming rotated text */
    /* Since we draw text horizontally, we need wider tabs */
    QTabBar::Shape tabShape = shape();
    if (tabShape == RoundedWest || tabShape == RoundedEast ||
        tabShape == TriangularWest || tabShape == TriangularEast)
    {
      /* Swap width and height to account for horizontal text */
      /* This gives us wider tabs that can fit horizontal text */
      return QSize(size.height(), size.width());
    }

    return size;
  }

public:
  IupQtTabBar(Ihandle* ih_param) : QTabBar(), ih(ih_param), closeCallback(nullptr)
  {
    setStyle(new IupQtTabStyle());
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

/* Forward declaration */
class IupQtTabWidget;

/* Forward declarations for static callback functions */
static void qtTabsHandleCurrentChanged(IupQtTabWidget* tabs, int index, Ihandle* ih, int* prev_index);
static void qtTabsHandleTabCloseRequested(IupQtTabWidget* tabs, int index, Ihandle* ih);

class IupQtTabWidget : public QTabWidget
{
private:
  Ihandle* ih;
  int prev_index;

public:
  IupQtTabWidget(Ihandle* ih_param) : QTabWidget(), ih(ih_param), prev_index(-1)
  {
    /* Replace tab bar with custom one */
    IupQtTabBar* custom_bar = new IupQtTabBar(ih);
    setTabBar(custom_bar);

    /* Set close button callback */
    custom_bar->setCloseCallback([this, ih_param](int index) {
      qtTabsHandleTabCloseRequested(this, index, ih_param);
    });

    /* Connect tab change signal using lambda */
    QObject::connect(this, &QTabWidget::currentChanged, [this, ih_param](int index) {
      qtTabsHandleCurrentChanged(this, index, ih_param, &prev_index);
    });
  }

  void setIhandle(Ihandle* ih_param)
  {
    ih = ih_param;
    IupQtTabBar* tab_bar = (IupQtTabBar*)tabBar();
    if (tab_bar)
    {
      tab_bar->setIhandle(ih_param);

      /* Update close callback with new ih */
      tab_bar->setCloseCallback([this, ih_param](int index) {
        qtTabsHandleTabCloseRequested(this, index, ih_param);
      });
    }
  }

  void setPrevIndex(int index) { prev_index = index; }
  int getPrevIndex() const { return prev_index; }

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
  if (!ih || iupAttribGet(ih, "_IUPQT_IGNORE_CHANGE"))
    return;

  if (index < 0)
    return;

  /* Get page widgets at visual positions (handles tab reordering) */
  QWidget* current_page = tabs->widget(index);
  QWidget* prev_page = *prev_index >= 0 ? tabs->widget(*prev_index) : nullptr;

  /* Find which children correspond to these page widgets */
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

  /* Show/hide page containers */
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

  /* Fire callbacks */
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
    /* Destroy tab and children */
    Ihandle* child = IupGetChild(ih, index);
    if (child)
    {
      IupDestroy(child);
      IupRefreshChildren(ih);
    }
  }
  else if (ret == IUP_DEFAULT)
  {
    /* Hide tab */
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
  /* IUP_IGNORE - do nothing */
}

/****************************************************************************
 * Driver Functions
 ****************************************************************************/

extern "C" int iupdrvTabsExtraDecor(Ihandle* ih)
{
  (void)ih;
  return 0;
}

extern "C" int iupdrvTabsExtraMargin(void)
{
  return 0;
}

extern "C" int iupdrvTabsGetLineCountAttrib(Ihandle* ih)
{
  (void)ih;
  return 1;
}

extern "C" void iupdrvTabsSetCurrentTab(Ihandle* ih, int pos)
{
  IupQtTabWidget* tabs = (IupQtTabWidget*)ih->handle;
  if (!tabs)
    return;

  iupAttribSet(ih, "_IUPQT_IGNORE_CHANGE", "1");
  tabs->setCurrentIndex(pos);
  tabs->setPrevIndex(pos);
  iupAttribSet(ih, "_IUPQT_IGNORE_CHANGE", nullptr);
}

extern "C" int iupdrvTabsGetCurrentTab(Ihandle* ih)
{
  QTabWidget* tabs = (QTabWidget*)ih->handle;
  if (!tabs)
    return -1;

  return tabs->currentIndex();
}

extern "C" void iupdrvTabsGetTabSize(Ihandle* ih, const char* tab_title, const char* tab_image, int* tab_width, int* tab_height)
{
  int width = 0;
  int height = 0;

  /* When mapped, find the matching tab and return its actual size from Qt */
  if (ih->handle)
  {
    QTabWidget* tabWidget = (QTabWidget*)ih->handle;
    QTabBar* tabBar = tabWidget->tabBar();
    if (tabBar && tabBar->count() > 0)
    {
      /* Find the tab that matches this title/image combination */
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

      /* Fallback: use tabBar sizeHint divided by count (shouldn't happen normally) */
      QSize barSize = tabBar->sizeHint();
      width = barSize.width() / tabBar->count();
      height = barSize.height();

      if (tab_width) *tab_width = width;
      if (tab_height) *tab_height = height;
      return;
    }
  }

  /* Not mapped: calculate based on text + image + Qt style metrics */
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
      width += img_w;
      width += 4;  /* Qt adds 4px padding when icon is present */
      if (img_h > height)
        height = img_h;
    }
  }

  /* Query Qt application style for tab metrics */
  QStyle* style = QApplication::style();
  if (style)
  {
    int hspace = style->pixelMetric(QStyle::PM_TabBarTabHSpace, nullptr, nullptr);
    int vspace = style->pixelMetric(QStyle::PM_TabBarTabVSpace, nullptr, nullptr);

    /* Use sizeFromContents to get the full tab size including all style padding */
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
    /* Fallback if no style available */
    width += 24;
    height += 8;
  }

  if (tab_width) *tab_width = width;
  if (tab_height) *tab_height = height;
}

extern "C" int iupdrvTabsIsTabVisible(Ihandle* child, int pos)
{
  Ihandle* ih = IupGetParent(child);
  if (!ih || !ih->handle)
    return 1; /* Before mapping, all tabs are visible by default */

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

  /* Map IUP tab types to Qt tab positions */
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

    /* Remove any existing padding rules */
    QStringList lines = styleSheet.split(';');
    QStringList filtered;
    for (const QString& line : lines)
    {
      if (!line.contains("padding"))
        filtered.append(line);
    }
    styleSheet = filtered.join(";");

    /* Add new padding rule */
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
      ih->data->is_multiline = 1; /* Always true if left/right */
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

  if (iupStrEqualNoCase(value, "BOTTOM"))
  {
    ih->data->type = ITABS_BOTTOM;
    ih->data->orientation = ITABS_HORIZONTAL;
  }
  else if (iupStrEqualNoCase(value, "LEFT"))
  {
    ih->data->type = ITABS_LEFT;
    ih->data->orientation = ITABS_VERTICAL;
    ih->data->is_multiline = 1; /* VERTICAL works only with MULTILINE */
  }
  else if (iupStrEqualNoCase(value, "RIGHT"))
  {
    ih->data->type = ITABS_RIGHT;
    ih->data->orientation = ITABS_VERTICAL;
    ih->data->is_multiline = 1; /* VERTICAL works only with MULTILINE */
  }
  else /* "TOP" */
  {
    ih->data->type = ITABS_TOP;
    ih->data->orientation = ITABS_HORIZONTAL;
  }

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
          /* Re-add the tab */
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
          /* Hide tab */
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
  if (pos == -1)
  {
    /* Global attribute - set for all tabs */
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
    /* Per-tab attribute */
    Ihandle* child = IupGetChild(ih, pos);
    if (child)
      iupAttribSetStr(child, "SHOWCLOSE", value);

    if (ih->handle)
    {
      IupQtTabWidget* tabs = (IupQtTabWidget*)ih->handle;
      IupQtTabBar* tab_bar = (IupQtTabBar*)tabs->tabBar();

      if (iupStrBoolean(value))
      {
        /* Create close button */
        QToolButton* close_btn = new QToolButton();
        close_btn->setIcon(tabs->style()->standardIcon(QStyle::SP_TitleBarCloseButton));
        close_btn->setAutoRaise(true);
        close_btn->setFixedSize(16, 16);

        /* Use lambda to call static function - no MOC needed */
        QObject::connect(close_btn, &QToolButton::clicked, [tabs, pos, ih]() {
          qtTabsHandleTabCloseRequested(tabs, pos, ih);
        });

        tab_bar->setTabButton(pos, QTabBar::RightSide, close_btn);
      }
      else
      {
        /* Remove close button */
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

    /* Subtract tab bar height/width depending on orientation */
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

    /* Offset depends on tab position */
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
  /* Make sure it has at least one name */
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

    /* Create page widget with layout to auto-resize container */
    tab_page = new QWidget();
    QVBoxLayout* pageLayout = new QVBoxLayout(tab_page);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(0);

    /* Create container for child widgets, layout will expand it to fill tab_page */
    tab_container = new QWidget();
    pageLayout->addWidget(tab_container);
    tab_container->show();

    /* Get tab title */
    tabtitle = iupAttribGet(child, "TABTITLE");
    if (!tabtitle)
    {
      tabtitle = iupAttribGetId(ih, "TABTITLE", pos);
      if (tabtitle)
        iupAttribSetStr(child, "TABTITLE", tabtitle);
    }

    /* Get tab image */
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

    /* Insert tab */
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

    /* Store references */
    iupAttribSet(child, "_IUPTAB_CONTAINER", (char*)tab_container);
    iupAttribSet(child, "_IUPTAB_PAGE", (char*)tab_page);

    /* Set background color */
    iupStrToRGB(IupGetAttribute(ih, "BGCOLOR"), &r, &g, &b);
    QPalette palette = tab_container->palette();
    palette.setColor(QPalette::Window, QColor(r, g, b));
    tab_container->setPalette(palette);
    tab_container->setAutoFillBackground(true);

    iupAttribSet(ih, "_IUPQT_IGNORE_CHANGE", nullptr);

    /* Hide container if not current tab */
    if (pos != iupdrvTabsGetCurrentTab(ih))
      tab_container->hide();
  }
}

static void qtTabsChildRemovedMethod(Ihandle* ih, Ihandle* child, int pos)
{
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

  if (!tabs)
    return IUP_ERROR;

  ih->handle = (InativeHandle*)tabs;
  tabs->setIhandle(ih);

  /* Set tab bar properties */
  tabs->setTabsClosable(false); /* We handle close buttons manually for per-tab control */
  tabs->setMovable(false); /* Will be set by ALLOWREORDER attribute */

  /* Set tab position */
  qtTabsUpdateTabType(ih);

  /* Set multiline behavior (limited support in Qt) */
  /* Qt doesn't have traditional multiline tabs, but we can control scroll buttons */
  if (ih->data->is_multiline)
  {
    tabs->tabBar()->setUsesScrollButtons(false);
    tabs->tabBar()->setExpanding(false);
  }
  else
  {
    /* Enable scrollable tabs by default (matches GTK behavior) */
    tabs->tabBar()->setUsesScrollButtons(true);
    tabs->tabBar()->setExpanding(false);
  }

  /* Apply tab padding if set */
  if (ih->data->horiz_padding != 0 || ih->data->vert_padding != 0)
  {
    QString styleSheet = QString(
      "QTabBar::tab { padding: %1px %2px; }"
    ).arg(ih->data->vert_padding).arg(ih->data->horiz_padding);
    tabs->tabBar()->setStyleSheet(styleSheet);
  }

  /* Add to parent */
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

extern "C" void iupdrvTabsInitClass(Iclass* ic)
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
  iupClassRegisterAttributeId(ic, "TABIMAGE", nullptr, qtTabsSetTabImageAttrib, IUPAF_IHANDLENAME | IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "TABVISIBLE", iupTabsGetTabVisibleAttrib, qtTabsSetTabVisibleAttrib, IUPAF_NO_INHERIT);
  iupClassRegisterAttributeId(ic, "SHOWCLOSE", nullptr, qtTabsSetShowCloseAttrib, IUPAF_NO_INHERIT);

  /* Read-only attributes */
  iupClassRegisterAttribute(ic, "CLIENTSIZE", qtTabsGetClientSizeAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);
  iupClassRegisterAttribute(ic, "CLIENTOFFSET", qtTabsGetClientOffsetAttrib, nullptr, nullptr, nullptr, IUPAF_READONLY | IUPAF_NO_INHERIT);

  /* IupTabs Callbacks */
  iupClassRegisterCallback(ic, "TABCHANGE_CB", "nn");
  iupClassRegisterCallback(ic, "TABCHANGEPOS_CB", "ii");
  iupClassRegisterCallback(ic, "TABCLOSE_CB", "i");
  iupClassRegisterCallback(ic, "RIGHTCLICK_CB", "i");
}
