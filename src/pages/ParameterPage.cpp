#include "ParameterPage.h"

#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

ParameterPage::ParameterPage(QWidget *parent)
    : QWidget(parent)
{
  auto *layout = new QHBoxLayout(this);
  layout->setContentsMargins(10, 10, 10, 10);
  layout->setSpacing(10);

  auto *left = new QFrame(this);
  left->setObjectName("SidePanel");
  auto *leftLayout = new QVBoxLayout(left);
  auto *title = new QLabel(QStringLiteral("配置列表"), left);
  title->setObjectName("SectionTitle");
  auto *search = new QLineEdit(left);
  search->setPlaceholderText(QStringLiteral("搜索配置..."));
  auto *list = new QListWidget(left);
  list->addItems({QStringLiteral("系统默认 - K1 Max"), QStringLiteral("PLA 高速"), QStringLiteral("PETG 质量优先")});
  auto *profileTree = new QTreeWidget(left);
  profileTree->setHeaderLabels({QStringLiteral("配置层级")});
  auto *sysNode = new QTreeWidgetItem(profileTree, {QStringLiteral("系统配置")});
  new QTreeWidgetItem(sysNode, {QStringLiteral("K1 Max / 0.4mm")});
  new QTreeWidgetItem(sysNode, {QStringLiteral("K1C / 0.4mm")});
  auto *userNode = new QTreeWidgetItem(profileTree, {QStringLiteral("用户配置")});
  new QTreeWidgetItem(userNode, {QStringLiteral("PLA 高速")});
  new QTreeWidgetItem(userNode, {QStringLiteral("PETG 质量优先")});
  profileTree->expandAll();
  leftLayout->addWidget(title);
  leftLayout->addWidget(search);
  leftLayout->addWidget(profileTree, 1);
  leftLayout->addWidget(list, 1);

  auto *right = new QFrame(this);
  right->setObjectName("MainPanel");
  auto *rightLayout = new QVBoxLayout(right);

  auto *detailTitle = new QLabel(QStringLiteral("配置详情"), right);
  detailTitle->setObjectName("SectionTitle");
  rightLayout->addWidget(detailTitle);

  auto *formWrap = new QWidget(right);
  auto *form = new QFormLayout(formWrap);

  auto *quality = new QComboBox(formWrap);
  quality->addItems({QStringLiteral("草稿"), QStringLiteral("标准"), QStringLiteral("高质量")});
  auto *speed = new QComboBox(formWrap);
  speed->addItems({"150 mm/s", "250 mm/s", "350 mm/s"});
  auto *travel = new QComboBox(formWrap);
  travel->addItems({"200 mm/s", "300 mm/s", "500 mm/s"});

  form->addRow(QStringLiteral("质量预设"), quality);
  form->addRow(QStringLiteral("打印速度"), speed);
  form->addRow(QStringLiteral("空驶速度"), travel);

  rightLayout->addWidget(formWrap);
  rightLayout->addStretch();

  auto *actions = new QHBoxLayout();
  actions->addStretch();
  auto *resetBtn = new QPushButton(QStringLiteral("恢复默认"), right);
  auto *saveBtn = new QPushButton(QStringLiteral("保存配置"), right);
  saveBtn->setObjectName("PrimaryBtn");
  actions->addWidget(resetBtn);
  actions->addWidget(saveBtn);
  rightLayout->addLayout(actions);

  layout->addWidget(left, 1);
  layout->addWidget(right, 2);

  connect(search, &QLineEdit::textChanged, this, [list](const QString &text)
          {
        for (int i = 0; i < list->count(); ++i) {
            QListWidgetItem *item = list->item(i);
            const bool visible = text.trimmed().isEmpty() || item->text().contains(text, Qt::CaseInsensitive);
            item->setHidden(!visible);
        } });
}
