#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Widgets/InlinedGroupBox.moc.h>
#include <QBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOptionToolButton>

plQtInlinedGroupBox::plQtInlinedGroupBox(QWidget* pParent)
  : plQtGroupBoxBase(pParent, false)
{
  QHBoxLayout* pRootLayout = new QHBoxLayout(this);
  pRootLayout->setContentsMargins(0, 1, 0, 1);
  pRootLayout->setSpacing(0);

  m_pContent = new QWidget(this);
  QSizePolicy sp(QSizePolicy::Expanding, QSizePolicy::Preferred);
  sp.setHorizontalStretch(2);
  sp.setVerticalStretch(0);
  m_pContent->setSizePolicy(sp);

  m_pHeader = new QFrame(this);
  QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
  sizePolicy1.setHorizontalStretch(0);
  sizePolicy1.setVerticalStretch(0);
  m_pHeader->setSizePolicy(sizePolicy1);

  QHBoxLayout* pHeaderLayout = new QHBoxLayout(m_pHeader);
  pHeaderLayout->setSpacing(0);
  pHeaderLayout->setContentsMargins(0, 0, 0, 0);
  pHeaderLayout->setAlignment(Qt::AlignTop);

  pRootLayout->addSpacerItem(new QSpacerItem(0, 0));
  pRootLayout->setStretch(0, 1);
  pRootLayout->addWidget(m_pContent);
  pRootLayout->addWidget(m_pHeader);

  installEventFilter(this);
}

void plQtInlinedGroupBox::SetTitle(plStringView sTitle)
{
  plQtGroupBoxBase::SetTitle(sTitle);
  update();
}

void plQtInlinedGroupBox::SetIcon(const QIcon& icon)
{
  plQtGroupBoxBase::SetIcon(icon);
  update();
}

void plQtInlinedGroupBox::SetFillColor(const QColor& color)
{
  plQtGroupBoxBase::SetFillColor(color);
  if (color.isValid())
    layout()->setContentsMargins(0, 1, 0, 1);
  else
    layout()->setContentsMargins(0, 0, 0, 0);
  update();
}

void plQtInlinedGroupBox::SetCollapseState(bool bCollapsed) {}

bool plQtInlinedGroupBox::GetCollapseState() const
{
  return false;
}

QWidget* plQtInlinedGroupBox::GetContent()
{
  return m_pContent;
}

QWidget* plQtInlinedGroupBox::GetHeader()
{
  return m_pHeader;
}

void plQtInlinedGroupBox::paintEvent(QPaintEvent* event)
{
  const QPalette& pal = palette();
  QWidget::paintEvent(event);

  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing);

  QRect wr = contentsRect();

  if (m_FillColor.isValid())
  {
    QRectF wrAdjusted = wr;
    wrAdjusted.adjust(0.5, 0.5, Rounding, -0.5);
    QPainterPath oPath;
    oPath.addRoundedRect(wrAdjusted, Rounding, Rounding);
    p.fillPath(oPath, pal.shadow());
  }

  DrawHeader(p, wr.adjusted(Rounding, 0, 0, 0));
}

bool plQtInlinedGroupBox::eventFilter(QObject* object, QEvent* event)
{
  switch (event->type())
  {
    case QEvent::Type::MouseButtonPress:
      HeaderMousePress(static_cast<QMouseEvent*>(event));
      return true;
    case QEvent::Type::MouseMove:
      HeaderMouseMove(static_cast<QMouseEvent*>(event));
      return true;
    case QEvent::Type::MouseButtonRelease:
      HeaderMouseRelease(static_cast<QMouseEvent*>(event));
      return true;
    default:
      break;
  }
  return false;
}
