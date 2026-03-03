#include "CalibrationViewModel.h"
#include "../services/CalibrationServiceMock.h"

CalibrationViewModel::CalibrationViewModel(CalibrationServiceMock *service, QObject *parent)
    : QObject(parent), m_service(service)
{
  if (m_service)
  {
    connect(m_service, &CalibrationServiceMock::progressChanged, this, &CalibrationViewModel::progressChanged);
    connect(m_service, &CalibrationServiceMock::isRunningChanged, this, &CalibrationViewModel::runningChanged);
    connect(m_service, &CalibrationServiceMock::calibrationFinished, this, [this](bool)
            {
      emit runningChanged();
      emit progressChanged(); });
  }

  m_calibItems = {
      {"🏠", tr("首层校准"), tr("调整首层高度"), tr("精确调整首层打印高度，确保良好附着。"), tr("首层高度预览区")},
      {"🌡", tr("热床校正"), tr("PID 热床温度校正"), tr("校正热床 PID 参数，稳定床温。"), tr("温度曲线图")},
      {"🔊", tr("共振补偿"), tr("输入整形/共振测试"), tr("测量并补偿打印机共振以提升质量。"), tr("频率响应图")},
      {"📏", tr("流量校准"), tr("挤出流量线性校准"), tr("校准挤出机流量，减少欠挤或过挤。"), tr("测试块预览")},
      {"⚡", tr("速度校准"), tr("最大速度测试"), tr("测试各轴最大速度与加速度边界。"), tr("速度斜坡图")},
      {"🎯", tr("喷嘴偏移"), tr("XY 喷嘴偏移校准"), tr("多喷头打印机的喷嘴偏移精确对齐。"), tr("偏移测试图案")},
      {"🔄", tr("压力提前"), tr("Linear Advance 校准"), tr("校准压力提前参数，消除拐角鼓包。"), tr("折线测试图")},
  };
  // m_items QVariantList removed - computed on demand in calibrationItems()
}

QVariantList CalibrationViewModel::calibrationItems() const
{
  QVariantList result;
  result.reserve(m_calibItems.size());
  for (const auto &item : m_calibItems)
  {
    result.append(QVariantMap{{"icon", item.icon}, {"name", item.name}, {"desc", item.desc}});
  }
  return result;
}

int CalibrationViewModel::calibItemCount() const { return m_calibItems.size(); }
QString CalibrationViewModel::calibItemIcon(int i) const { return (i >= 0 && i < m_calibItems.size()) ? m_calibItems[i].icon : QString{}; }
QString CalibrationViewModel::calibItemName(int i) const { return (i >= 0 && i < m_calibItems.size()) ? m_calibItems[i].name : QString{}; }
QString CalibrationViewModel::calibItemDesc(int i) const { return (i >= 0 && i < m_calibItems.size()) ? m_calibItems[i].desc : QString{}; }

bool CalibrationViewModel::isRunning() const
{
  return m_service ? m_service->isRunning() : false;
}

int CalibrationViewModel::progress() const
{
  return m_service ? m_service->progress() : 0;
}

QString CalibrationViewModel::selectedTitle() const
{
  if (m_selectedIndex < 0 || m_selectedIndex >= m_calibItems.size())
    return {};
  return m_calibItems[m_selectedIndex].name;
}

QString CalibrationViewModel::selectedDescription() const
{
  if (m_selectedIndex < 0 || m_selectedIndex >= m_calibItems.size())
    return {};
  return m_calibItems[m_selectedIndex].longDesc;
}

QString CalibrationViewModel::selectedPreviewLabel() const
{
  if (m_selectedIndex < 0 || m_selectedIndex >= m_calibItems.size())
    return "选择校准项后在此处显示预览";
  return m_calibItems[m_selectedIndex].previewLabel;
}

void CalibrationViewModel::selectItem(int index)
{
  if (m_selectedIndex != index)
  {
    m_selectedIndex = index;
    emit selectionChanged();
  }
}

void CalibrationViewModel::startCalibration()
{
  if (m_selectedIndex < 0)
    return;
  if (m_service)
    m_service->startCalibration(m_selectedIndex);
  else
  {
    emit runningChanged();
    emit progressChanged();
  }
}

void CalibrationViewModel::cancelCalibration()
{
  if (m_service)
    m_service->cancelCalibration();
  else
  {
    emit runningChanged();
    emit progressChanged();
  }
}

void CalibrationViewModel::resetParameters()
{
  // Reset to defaults (mock)
}
