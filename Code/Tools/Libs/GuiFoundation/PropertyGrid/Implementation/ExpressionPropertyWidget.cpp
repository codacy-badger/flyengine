#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/PropertyGrid/Implementation/ExpressionPropertyWidget.moc.h>

#include <Foundation/CodeUtils/Expression/ExpressionParser.h>

plQtPropertyEditorExpressionWidget::plQtPropertyEditorExpressionWidget()
  : plQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  QFont font;
  font.setFamily("Courier");
  font.setFixedPitch(true);
  font.setPointSize(10);

  m_pWidget = new QTextEdit(this);
  m_pWidget->setFont(font);
  m_pWidget->installEventFilter(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pWidget->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
  m_pWidget->setWordWrapMode(QTextOption::NoWrap);
  setFocusProxy(m_pWidget);

  m_pHighlighter = new ExpressionHighlighter(m_pWidget->document());

  m_pLayout->addWidget(m_pWidget);

  connect(m_pWidget, SIGNAL(textChanged()), this, SLOT(on_TextChanged()));
}

plQtPropertyEditorExpressionWidget::~plQtPropertyEditorExpressionWidget()
{
  delete m_pHighlighter;
}

void plQtPropertyEditorExpressionWidget::OnInit()
{
}

void plQtPropertyEditorExpressionWidget::InternalSetValue(const plVariant& value)
{
  plQtScopedBlockSignals b(m_pWidget);

  if (!value.IsValid())
  {
    m_pWidget->setPlaceholderText(QStringLiteral("<Multiple Values>"));
  }
  else
  {
    m_pWidget->setPlaceholderText(QString());

    QString newText = plMakeQString(value.ConvertTo<plString>());
    if (m_pWidget->toPlainText() != newText)
    {
      m_pWidget->setText(newText);
    }
  }
}

void plQtPropertyEditorExpressionWidget::on_TextChanged()
{
  BroadcastValueChanged(plVariant(qtToPlString(m_pWidget->toPlainText())));
}

//////////////////////////////////////////////////////////////////////////

ExpressionHighlighter::ExpressionHighlighter(QTextDocument* pParent)
  : QSyntaxHighlighter(pParent)
{
  // default color scheme
  m_Colors[ExpressionTokenType::Comment] = QColor(87, 166, 74);
  m_Colors[ExpressionTokenType::Number] = QColor(182, 206, 168);
  m_Colors[ExpressionTokenType::Bracket] = QColor(200, 200, 200);
  m_Colors[ExpressionTokenType::Type] = QColor(86, 156, 214);
  m_Colors[ExpressionTokenType::BuiltIn] = QColor(216, 160, 223);
}

void ExpressionHighlighter::highlightBlock(const QString& text)
{
  auto& knownTypes = plExpressionParser::GetKnownTypes();
  auto& builtinFunctions = plExpressionParser::GetBuiltinFunctions();

  // parsing state
  enum
  {
    Start,
    Number,
    Identifier,
    Comment,
  };

  int blockState = previousBlockState();
  int bracketLevel = blockState >> 4;
  int state = blockState & 15;
  if (blockState < 0)
  {
    bracketLevel = 0;
    state = Start;
  }

  int start = 0;
  int i = 0;
  while (i <= text.length())
  {
    QChar ch = (i < text.length()) ? text.at(i) : QChar();
    QChar next = (i < text.length() - 1) ? text.at(i + 1) : QChar();

    switch (state)
    {
      case Start:
        start = i;
        if (ch.isSpace())
        {
          ++i;
        }
        else if (ch.isDigit())
        {
          ++i;
          state = Number;
        }
        else if (ch.isLetter() || ch == '_')
        {
          ++i;
          state = Identifier;
        }
        else if (ch == '/' && next == '*')
        {
          ++i;
          ++i;
          state = Comment;
        }
        else if (ch == '/' && next == '/')
        {
          i = text.length();
          setFormat(start, text.length(), m_Colors[ExpressionTokenType::Comment]);
        }
        else
        {
          if (!QString("(){}[]").contains(ch))
            setFormat(start, 1, m_Colors[ExpressionTokenType::Bracket]);

          if (ch == '{' || ch == '}')
          {
            if (ch == '{')
              bracketLevel++;
            else
              bracketLevel--;
          }
          ++i;
          state = Start;
        }
        break;

      case Number:
        if (ch.isSpace() || !ch.isDigit())
        {
          setFormat(start, i - start, m_Colors[ExpressionTokenType::Number]);
          state = Start;
        }
        else
        {
          ++i;
        }
        break;

      case Identifier:
        if (ch.isSpace() || !(ch.isDigit() || ch.isLetter() || ch == '_'))
        {
          QString token = text.mid(start, i - start).trimmed();
          plTempHashedString tokenHashed(token.toUtf8().data());

          if (knownTypes.Contains(tokenHashed))
          {
            setFormat(start, i - start, m_Colors[ExpressionTokenType::Type]);
          }
          else if (builtinFunctions.Contains(tokenHashed))
          {
            setFormat(start, i - start, m_Colors[ExpressionTokenType::BuiltIn]);
          }
          state = Start;
        }
        else
        {
          ++i;
        }
        break;

      case Comment:
        if (ch == '*' && next == '/')
        {
          ++i;
          ++i;
          setFormat(start, i - start, m_Colors[ExpressionTokenType::Comment]);
          state = Start;
        }
        else
        {
          ++i;
        }
        break;

      default:
        state = Start;
        break;
    }
  }

  if (state == Comment)
    setFormat(start, text.length(), m_Colors[ExpressionTokenType::Comment]);
  else
    state = Start;

  blockState = (state & 15) | (bracketLevel << 4);
  setCurrentBlockState(blockState);
}
