#include "MonitorPage.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

MonitorPage::MonitorPage(QWidget *parent)
    : QWidget(parent)
{
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  auto *header = new QFrame(this);
  header->setObjectName("DeviceTopBar");
  header->setFixedHeight(40);
  auto *headerLayout = new QHBoxLayout(header);
  headerLayout->setContentsMargins(12, 6, 12, 6);

  auto *groupBtn = new QPushButton(QStringLiteral("管理分组"), header);
  groupBtn->setObjectName("MiniTag");
  auto *taskBtn = new QPushButton(QStringLiteral("查看任务"), header);
  taskBtn->setObjectName("MiniTag");
  headerLayout->addWidget(groupBtn);
  headerLayout->addWidget(taskBtn);
  headerLayout->addStretch();

  auto *gridBtn = new QToolButton(header);
  gridBtn->setText(QStringLiteral("◫"));
  gridBtn->setFixedSize(24, 24);
  auto *listBtn = new QToolButton(header);
  listBtn->setText(QStringLiteral("☰"));
  listBtn->setFixedSize(24, 24);
  headerLayout->addWidget(gridBtn);
  headerLayout->addWidget(listBtn);

  layout->addWidget(header);

  auto *panel = new QFrame(this);
  panel->setObjectName("DevicePanel");
  auto *panelLayout = new QVBoxLayout(panel);
  panelLayout->setContentsMargins(12, 10, 12, 10);
  panelLayout->setSpacing(8);

  auto *toolbar = new QFrame(panel);
  toolbar->setObjectName("DeviceInnerBar");
  toolbar->setFixedHeight(34);
  auto *tbLayout = new QHBoxLayout(toolbar);
  tbLayout->setContentsMargins(8, 4, 8, 4);

  auto *groupLabel = new QLabel(QStringLiteral("New Group1  ✎"), toolbar);
  tbLayout->addWidget(groupLabel);
  tbLayout->addStretch();
  auto *addPrinterBtn = new QPushButton(QStringLiteral("＋扫描添加"), toolbar);
  addPrinterBtn->setObjectName("MiniTag");
  auto *manualBtn = new QPushButton(QStringLiteral("＋手动添加"), toolbar);
  manualBtn->setObjectName("MiniTag");
  tbLayout->addWidget(addPrinterBtn);
  tbLayout->addWidget(manualBtn);
  panelLayout->addWidget(toolbar);

  auto *listHeader = new QFrame(panel);
  listHeader->setObjectName("DeviceListHeader");
  listHeader->setFixedHeight(28);
  auto *lhLayout = new QHBoxLayout(listHeader);
  lhLayout->setContentsMargins(8, 0, 8, 0);
  auto *nameHeader = new QLabel(QStringLiteral("设备名称/卡片"), listHeader);
  auto *stateHeader = new QLabel(QStringLiteral("设备状态"), listHeader);
  nameHeader->setObjectName("MutedText");
  stateHeader->setObjectName("MutedText");
  lhLayout->addWidget(nameHeader);
  lhLayout->addSpacing(24);
  lhLayout->addWidget(stateHeader);
  lhLayout->addStretch();
  panelLayout->addWidget(listHeader);

  auto *emptyArea = new QFrame(panel);
  emptyArea->setObjectName("DeviceEmptyArea");
  auto *emptyLayout = new QVBoxLayout(emptyArea);
  emptyLayout->setContentsMargins(0, 30, 0, 30);
  emptyLayout->addStretch();
  auto *icon = new QLabel(QStringLiteral("📦"), emptyArea);
  icon->setAlignment(Qt::AlignCenter);
  icon->setStyleSheet("font-size: 56px;");
  auto *emptyText = new QLabel(QStringLiteral("No Data"), emptyArea);
  emptyText->setObjectName("MutedText");
  emptyText->setAlignment(Qt::AlignCenter);
  emptyLayout->addWidget(icon);
  emptyLayout->addWidget(emptyText);
  emptyLayout->addStretch();
  panelLayout->addWidget(emptyArea, 1);

  layout->addWidget(panel, 1);
}
