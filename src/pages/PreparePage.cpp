#include "PreparePage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSlider>
#include <QSplitter>
#include <QTabWidget>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>

PreparePage::PreparePage(QWidget *parent)
    : QWidget(parent)
{
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  auto *topTools = new QFrame(this);
  topTools->setObjectName("PrepareTopToolBar");
  topTools->setFixedHeight(40);
  auto *toolsLayout = new QHBoxLayout(topTools);
  toolsLayout->setContentsMargins(8, 6, 8, 6);
  toolsLayout->setSpacing(6);

  const QStringList topToolTexts = {
      QStringLiteral("□"), QStringLiteral("◧"), QStringLiteral("◎"), QStringLiteral("◇"), QStringLiteral("⌗"),
      QStringLiteral("↔"), QStringLiteral("⤢"), QStringLiteral("⊕"), QStringLiteral("⚙"), QStringLiteral("A")};
  for (const QString &text : topToolTexts)
  {
    auto *btn = new QToolButton(topTools);
    btn->setText(text);
    btn->setFixedSize(26, 24);
    toolsLayout->addWidget(btn);
  }
  toolsLayout->addStretch();

  layout->addWidget(topTools);

  auto *splitter = new QSplitter(Qt::Horizontal, this);
  splitter->setChildrenCollapsible(false);
  splitter->setHandleWidth(1);

  auto *leftPanel = new QFrame(splitter);
  leftPanel->setObjectName("PrepareLeftPanel");
  leftPanel->setMinimumWidth(220);
  leftPanel->setMaximumWidth(250);
  auto *leftLayout = new QVBoxLayout(leftPanel);
  leftLayout->setContentsMargins(8, 8, 8, 8);
  leftLayout->setSpacing(6);

  auto *printTitle = new QLabel(QStringLiteral("打印机"), leftPanel);
  printTitle->setObjectName("SectionTitle");
  leftLayout->addWidget(printTitle);

  auto *printerCombo = new QComboBox(leftPanel);
  printerCombo->addItems({QStringLiteral("Creality K1C 0.4 nozzle"), QStringLiteral("K1 Max 0.4 nozzle")});
  leftLayout->addWidget(printerCombo);

  auto *materialCombo = new QComboBox(leftPanel);
  materialCombo->addItems({QStringLiteral("光固化PEI板/喷气板"), QStringLiteral("普通PEI板")});
  leftLayout->addWidget(materialCombo);

  auto *tagRow = new QHBoxLayout();
  auto *allTag = new QPushButton(QStringLiteral("全部"), leftPanel);
  allTag->setObjectName("MiniTag");
  auto *objTag = new QPushButton(QStringLiteral("对象"), leftPanel);
  objTag->setObjectName("MiniTag");
  tagRow->addWidget(allTag);
  tagRow->addWidget(objTag);
  tagRow->addStretch();
  leftLayout->addLayout(tagRow);

  auto *objectTree = new QTreeWidget(leftPanel);
  objectTree->setHeaderLabels({QStringLiteral("模型/零件")});
  auto *root = new QTreeWidgetItem(objectTree, {QStringLiteral("Benchy")});
  new QTreeWidgetItem(root, {QStringLiteral("Body")});
  new QTreeWidgetItem(root, {QStringLiteral("Top")});
  objectTree->expandAll();
  leftLayout->addWidget(objectTree, 1);

  auto *centerPanel = new QFrame(splitter);
  centerPanel->setObjectName("PrepareCenterPanel");
  auto *centerLayout = new QVBoxLayout(centerPanel);
  centerLayout->setContentsMargins(8, 8, 8, 8);
  centerLayout->setSpacing(10);

  auto *viewport = new QFrame(centerPanel);
  viewport->setObjectName("PrepareViewport");
  auto *viewportLayout = new QGridLayout(viewport);
  viewportLayout->setContentsMargins(14, 14, 14, 14);

  auto *viewportText = new QLabel(QStringLiteral("Creality Smooth PEI Plate\n\n"
                                                 "               01\n\n"
                                                 "          （准备页 3D 视图占位）"),
                                  viewport);
  viewportText->setObjectName("MutedText");
  viewportText->setAlignment(Qt::AlignCenter);
  viewportLayout->addWidget(viewportText, 0, 0, 3, 1);

  auto *rightOps = new QVBoxLayout();
  rightOps->setSpacing(6);
  const QStringList ops = {QStringLiteral("＋"), QStringLiteral("☰"), QStringLiteral("AUTO"), QStringLiteral("🔒"), QStringLiteral("⚙")};
  for (const QString &op : ops)
  {
    auto *btn = new QToolButton(viewport);
    btn->setText(op);
    btn->setObjectName("RoundTool");
    btn->setFixedSize(36, 28);
    rightOps->addWidget(btn, 0, Qt::AlignRight);
  }
  rightOps->addStretch();
  viewportLayout->addLayout(rightOps, 0, 1, 3, 1);

  auto *viewHint = new QLabel(QStringLiteral("上\n前"), viewport);
  viewHint->setObjectName("ViewHint");
  viewportLayout->addWidget(viewHint, 0, 2, Qt::AlignTop | Qt::AlignRight);
  centerLayout->addWidget(viewport, 1);

  auto *bottomActions = new QHBoxLayout();
  bottomActions->addStretch();
  auto *sliceRoleBtn = new QPushButton(QStringLiteral("切片角色"), centerPanel);
  sliceRoleBtn->setObjectName("MiniTag");
  auto *sendBtn = new QPushButton(QStringLiteral("发送打印"), centerPanel);
  sendBtn->setObjectName("MiniTag");
  bottomActions->addWidget(sliceRoleBtn);
  bottomActions->addWidget(sendBtn);
  centerLayout->addLayout(bottomActions);

  auto *rightPanel = new QFrame(splitter);
  rightPanel->setObjectName("PrepareRightPanel");
  rightPanel->setMinimumWidth(280);
  rightPanel->setMaximumWidth(320);
  auto *rightLayout = new QVBoxLayout(rightPanel);
  rightLayout->setContentsMargins(0, 0, 0, 0);
  rightLayout->setSpacing(0);

  auto *materialHeader = new QFrame(rightPanel);
  materialHeader->setObjectName("MaterialHeader");
  materialHeader->setFixedHeight(56);
  auto *mhLayout = new QHBoxLayout(materialHeader);
  mhLayout->setContentsMargins(10, 8, 10, 8);
  auto *mhTitle = new QLabel(QStringLiteral("耗材丝"), materialHeader);
  mhTitle->setObjectName("SectionTitle");
  auto *mhCombo = new QComboBox(materialHeader);
  mhCombo->addItems({"Hyper PLA", "PLA"});
  mhLayout->addWidget(mhTitle);
  mhLayout->addStretch();
  mhLayout->addWidget(mhCombo);
  rightLayout->addWidget(materialHeader);

  auto *paramBody = new QFrame(rightPanel);
  paramBody->setObjectName("PrepareParamBody");
  auto *paramLayout = new QVBoxLayout(paramBody);
  paramLayout->setContentsMargins(8, 8, 8, 8);
  paramLayout->setSpacing(8);

  auto *modeRow = new QHBoxLayout();
  auto *processTag = new QPushButton(QStringLiteral("工艺"), paramBody);
  processTag->setObjectName("MiniTag");
  auto *qualityTag = new QPushButton(QStringLiteral("高级"), paramBody);
  qualityTag->setObjectName("MiniTag");
  modeRow->addWidget(processTag);
  modeRow->addWidget(qualityTag);
  modeRow->addStretch();
  paramLayout->addLayout(modeRow);

  auto *presetRow = new QHBoxLayout();
  auto *preset = new QComboBox(paramBody);
  preset->addItems({QStringLiteral("0.20mm Standard @Creality K1C"), QStringLiteral("0.16mm Fine")});
  auto *savePreset = new QToolButton(paramBody);
  savePreset->setText(QStringLiteral("💾"));
  savePreset->setFixedSize(26, 24);
  presetRow->addWidget(preset, 1);
  presetRow->addWidget(savePreset);
  paramLayout->addLayout(presetRow);

  auto *settings = new QFrame(paramBody);
  settings->setObjectName("SettingsCard");
  auto *settingsForm = new QFormLayout(settings);
  settingsForm->setLabelAlignment(Qt::AlignLeft);
  settingsForm->setContentsMargins(8, 8, 8, 8);

  auto *layerHeight = new QLineEdit(QStringLiteral("0.2"), settings);
  auto *lineWidth = new QLineEdit(QStringLiteral("0.2"), settings);
  auto *wallMode = new QComboBox(settings);
  wallMode->addItems({QStringLiteral("对齐"), QStringLiteral("重叠")});

  settingsForm->addRow(QStringLiteral("层高"), layerHeight);
  settingsForm->addRow(QStringLiteral("首层层高"), lineWidth);
  settingsForm->addRow(QStringLiteral("壁线位置"), wallMode);

  auto *chk1 = new QCheckBox(QStringLiteral("填充间隙"), settings);
  chk1->setChecked(true);
  auto *chk2 = new QCheckBox(QStringLiteral("填充重叠"), settings);
  chk2->setChecked(true);
  auto *chk3 = new QCheckBox(QStringLiteral("首层外线尺寸"), settings);
  settingsForm->addRow(chk1);
  settingsForm->addRow(chk2);
  settingsForm->addRow(chk3);

  paramLayout->addWidget(settings, 1);
  rightLayout->addWidget(paramBody, 1);

  splitter->addWidget(leftPanel);
  splitter->addWidget(centerPanel);
  splitter->addWidget(rightPanel);
  splitter->setStretchFactor(0, 17);
  splitter->setStretchFactor(1, 60);
  splitter->setStretchFactor(2, 23);

  layout->addWidget(splitter, 1);
}
