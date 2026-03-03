#include "MainWindow.h"

#include <QCloseEvent>
#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QSettings>
#include <QStackedWidget>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include "pages/MonitorPage.h"
#include "pages/ParameterPage.h"
#include "pages/PreparePage.h"
#include "pages/PreviewPage.h"
#include "theme/AppTheme.h"

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
  setObjectName("MainWindow");
  setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
  setAttribute(Qt::WA_TranslucentBackground, true);
  resize(1440, 860);
  setMinimumSize(1100, 700);
  setStyleSheet(AppTheme::mainStyleSheet());

  buildUi();
  loadWindowState();
}

void MainWindow::buildUi()
{
  auto *rootLayout = new QVBoxLayout(this);
  rootLayout->setContentsMargins(0, 0, 0, 0);
  rootLayout->setSpacing(0);

  auto *rootPanel = new QFrame(this);
  rootPanel->setObjectName("RootPanel");
  auto *panelLayout = new QVBoxLayout(rootPanel);
  panelLayout->setContentsMargins(0, 0, 0, 0);
  panelLayout->setSpacing(0);

  titleBar_ = new QFrame(rootPanel);
  titleBar_->setObjectName("TitleBar");
  titleBar_->setFixedHeight(34);

  auto *titleLayout = new QHBoxLayout(titleBar_);
  titleLayout->setContentsMargins(10, 0, 8, 0);
  titleLayout->setSpacing(6);

  auto *appTitle = new QLabel(QStringLiteral("A  |  三文"), titleBar_);
  appTitle->setObjectName("SectionTitle");
  appTitle->setMinimumWidth(92);
  titleLayout->addWidget(appTitle);

  titleLayout->addSpacing(10);

  const QStringList navTexts = {QStringLiteral("准备"), QStringLiteral("预览"), QStringLiteral("设备"), QStringLiteral("参数")};
  for (int i = 0; i < navTexts.size(); ++i)
  {
    auto *btn = new QPushButton(navTexts.at(i), titleBar_);
    btn->setObjectName("NavBtn");
    btn->setFixedWidth(66);
    navButtons_.append(btn);
    connect(btn, &QPushButton::clicked, this, [this, i]()
            { switchPage(i); });
    titleLayout->addWidget(btn);
  }

  titleLayout->addStretch();

  auto *docLabel = new QLabel(QStringLiteral("未命名"), titleBar_);
  docLabel->setObjectName("MutedText");
  titleLayout->addWidget(docLabel);
  titleLayout->addSpacing(8);

  auto *minBtn = new QToolButton(titleBar_);
  minBtn->setText(QStringLiteral("_"));
  minBtn->setFixedSize(28, 24);
  connect(minBtn, &QToolButton::clicked, this, &QWidget::showMinimized);
  titleLayout->addWidget(minBtn);

  auto *maxBtn = new QToolButton(titleBar_);
  maxBtn->setText(QStringLiteral("□"));
  maxBtn->setFixedSize(28, 24);
  connect(maxBtn, &QToolButton::clicked, this, [this]()
          { isMaximized() ? showNormal() : showMaximized(); });
  titleLayout->addWidget(maxBtn);

  auto *closeBtn = new QToolButton(titleBar_);
  closeBtn->setObjectName("CloseBtn");
  closeBtn->setText(QStringLiteral("×"));
  closeBtn->setFixedSize(28, 24);
  connect(closeBtn, &QToolButton::clicked, this, &QWidget::close);
  titleLayout->addWidget(closeBtn);

  auto *pageBody = new QFrame(rootPanel);
  pageBody->setObjectName("PageBody");
  auto *pageLayout = new QVBoxLayout(pageBody);
  pageLayout->setContentsMargins(0, 0, 0, 0);

  stack_ = new QStackedWidget(pageBody);
  stack_->addWidget(new PreparePage(stack_));
  stack_->addWidget(new PreviewPage(stack_));
  stack_->addWidget(new MonitorPage(stack_));
  stack_->addWidget(new ParameterPage(stack_));
  pageLayout->addWidget(stack_);

  auto *statusBar = new QFrame(rootPanel);
  statusBar->setObjectName("StatusBar");
  statusBar->setFixedHeight(30);
  auto *statusLayout = new QHBoxLayout(statusBar);
  statusLayout->setContentsMargins(12, 0, 12, 0);

  statusLabel_ = new QLabel(QStringLiteral("状态：就绪"), statusBar);
  pageLabel_ = new QLabel(QStringLiteral("页面：准备"), statusBar);
  pageLabel_->setObjectName("MutedText");
  materialLabel_ = new QLabel(QStringLiteral("耗材：38.4 g"), statusBar);
  clockLabel_ = new QLabel(QStringLiteral("--:--:--"), statusBar);

  statusLayout->addWidget(statusLabel_);
  statusLayout->addSpacing(12);
  statusLayout->addWidget(pageLabel_);
  statusLayout->addStretch();
  statusLayout->addWidget(new QLabel(QStringLiteral("预计耗时：01:42:16"), statusBar));
  statusLayout->addSpacing(16);
  statusLayout->addWidget(materialLabel_);
  statusLayout->addSpacing(16);
  statusLayout->addWidget(clockLabel_);

  panelLayout->addWidget(titleBar_);
  panelLayout->addWidget(pageBody, 1);
  panelLayout->addWidget(statusBar);

  rootLayout->addWidget(rootPanel);

  clockTimer_ = new QTimer(this);
  clockTimer_->setInterval(1000);
  connect(clockTimer_, &QTimer::timeout, this, [this]()
          {
        if (clockLabel_ != nullptr) {
            clockLabel_->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
        } });
  clockTimer_->start();

  switchPage(0);
}

void MainWindow::switchPage(int index)
{
  currentIndex_ = index;
  stack_->setCurrentIndex(index);

  static const QStringList pageNames = {
      QStringLiteral("准备"),
      QStringLiteral("预览"),
      QStringLiteral("设备"),
      QStringLiteral("参数")};
  if (index >= 0 && index < pageNames.size())
  {
    if (statusLabel_ != nullptr)
    {
      statusLabel_->setText(QStringLiteral("状态：已切换到 %1").arg(pageNames.at(index)));
    }
    if (pageLabel_ != nullptr)
    {
      pageLabel_->setText(QStringLiteral("页面：%1").arg(pageNames.at(index)));
    }
  }

  if (materialLabel_ != nullptr)
  {
    materialLabel_->setText(index == 2 ? QStringLiteral("耗材：实时采集中") : QStringLiteral("耗材：38.4 g"));
  }

  refreshNavState();
}

void MainWindow::refreshNavState()
{
  for (int i = 0; i < navButtons_.size(); ++i)
  {
    navButtons_.at(i)->setProperty("active", i == currentIndex_ ? "true" : "false");
    navButtons_.at(i)->style()->unpolish(navButtons_.at(i));
    navButtons_.at(i)->style()->polish(navButtons_.at(i));
    navButtons_.at(i)->update();
  }
}

void MainWindow::loadWindowState()
{
  QSettings settings(QStringLiteral("CrealityDemo"), QStringLiteral("Print7Shell"));
  const QRect geometry = settings.value(QStringLiteral("window/geometry")).toRect();
  if (geometry.isValid())
  {
    setGeometry(geometry);
  }

  const int page = settings.value(QStringLiteral("window/pageIndex"), 0).toInt();
  if (page >= 0 && page < stack_->count())
  {
    switchPage(page);
  }
}

void MainWindow::saveWindowState() const
{
  QSettings settings(QStringLiteral("CrealityDemo"), QStringLiteral("Print7Shell"));
  settings.setValue(QStringLiteral("window/geometry"), geometry());
  settings.setValue(QStringLiteral("window/pageIndex"), currentIndex_);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
  saveWindowState();
  QWidget::closeEvent(event);
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
  if (event->button() == Qt::LeftButton && titleBar_->geometry().contains(event->position().toPoint()))
  {
    dragging_ = true;
    dragOffset_ = event->globalPosition().toPoint() - frameGeometry().topLeft();
  }
  QWidget::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
  if (dragging_ && (event->buttons() & Qt::LeftButton))
  {
    move(event->globalPosition().toPoint() - dragOffset_);
  }
  QWidget::mouseMoveEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
  if (event->button() == Qt::LeftButton)
  {
    dragging_ = false;
  }
  QWidget::mouseReleaseEvent(event);
}
