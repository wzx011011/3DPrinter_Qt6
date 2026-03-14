#include "CalibrationViewModel.h"
#include "../services/CalibrationServiceMock.h"
#include "../services/PresetServiceMock.h"

CalibrationViewModel::CalibrationViewModel(CalibrationServiceMock *service, QObject *parent)
    : QObject(parent), m_service(service)
{
    if (m_service)
    {
        connect(m_service, &CalibrationServiceMock::progressChanged, this, &CalibrationViewModel::progressChanged);
        connect(m_service, &CalibrationServiceMock::isRunningChanged, this, &CalibrationViewModel::runningChanged);
        connect(m_service, &CalibrationServiceMock::stepChanged, this, &CalibrationViewModel::stepChanged);
        connect(m_service, &CalibrationServiceMock::statusChanged, this, &CalibrationViewModel::statusChanged);
        connect(m_service, &CalibrationServiceMock::calibrationFinished, this, [this](bool)
            {
                emit runningChanged();
                emit progressChanged();
                emit stepChanged();
                emit statusChanged(m_selectedIndex, selectedStatus());
            });
    }
}

void CalibrationViewModel::setPresetService(PresetServiceMock *service)
{
    m_presetService = service;
    m_selectedFilamentPreset = service ? service->defaultPresetForCategory(1) : QString{};
    emit stateChanged();
}

// --- Item accessors (delegated to service) ---

int CalibrationViewModel::calibItemCount() const
{
    return m_service ? m_service->calibTypeCount() : 0;
}

QString CalibrationViewModel::calibItemIcon(int i) const
{
    return m_service ? m_service->calibTypeIcon(i) : QString{};
}

QString CalibrationViewModel::calibItemName(int i) const
{
    return m_service ? m_service->calibTypeName(i) : QString{};
}

QString CalibrationViewModel::calibItemDesc(int i) const
{
    return m_service ? m_service->calibTypeDesc(i) : QString{};
}

int CalibrationViewModel::calibItemStatus(int i) const
{
    return m_service ? m_service->calibStatus(i) : 0;
}

// --- Step accessors for current selection ---

int CalibrationViewModel::stepCount() const
{
    if (!m_service || m_selectedIndex < 0) return 0;
    return m_service->stepCount(m_selectedIndex);
}

QString CalibrationViewModel::stepTitle(int stepIndex) const
{
    if (!m_service || m_selectedIndex < 0) return {};
    return m_service->stepTitle(m_selectedIndex, stepIndex);
}

QString CalibrationViewModel::stepDesc(int stepIndex) const
{
    if (!m_service || m_selectedIndex < 0) return {};
    return m_service->stepDesc(m_selectedIndex, stepIndex);
}

int CalibrationViewModel::stepState(int stepIndex) const
{
    if (!m_service || m_selectedIndex < 0) return 0;
    return m_service->stepState(m_selectedIndex, stepIndex);
}

// --- Selected item properties ---

void CalibrationViewModel::selectItem(int index)
{
    if (m_selectedIndex == index) return;
    m_selectedIndex = index;
    emit selectionChanged();
}

QString CalibrationViewModel::selectedTitle() const
{
    return m_service ? m_service->calibTypeName(m_selectedIndex) : QString{};
}

QString CalibrationViewModel::selectedDescription() const
{
    return m_service ? m_service->calibTypeLongDesc(m_selectedIndex) : QString{};
}

QString CalibrationViewModel::selectedPreviewLabel() const
{
    return m_service ? m_service->calibTypePreviewLabel(m_selectedIndex) : QString{};
}

QString CalibrationViewModel::selectedCategory() const
{
    return m_service ? m_service->calibTypeCategory(m_selectedIndex) : QString{};
}

bool CalibrationViewModel::isRunning() const
{
    return m_service ? m_service->isRunning() : false;
}

int CalibrationViewModel::progress() const
{
    return m_service ? m_service->progress() : 0;
}

int CalibrationViewModel::currentStepIndex() const
{
    return m_service ? m_service->currentStepIndex() : -1;
}

QString CalibrationViewModel::currentStepTitle() const
{
    if (!m_service || m_selectedIndex < 0) return {};
    int step = m_service->currentStepIndex();
    return m_service->stepTitle(m_selectedIndex, step);
}

QString CalibrationViewModel::currentStepDesc() const
{
    if (!m_service || m_selectedIndex < 0) return {};
    int step = m_service->currentStepIndex();
    return m_service->stepDesc(m_selectedIndex, step);
}

QString CalibrationViewModel::currentStepId() const
{
    if (!m_service || m_selectedIndex < 0) return {};
    int step = m_service->currentStepIndex();
    return m_service->stepId(m_selectedIndex, step);
}

bool CalibrationViewModel::showPresetSelector() const
{
    return currentStepId() == QStringLiteral("preset");
}

int CalibrationViewModel::totalStepCount() const
{
    return stepCount();
}

int CalibrationViewModel::selectedStatus() const
{
    return m_service ? m_service->calibStatus(m_selectedIndex) : 0;
}

QStringList CalibrationViewModel::filamentPresetNames() const
{
    return m_presetService ? m_presetService->presetNamesForCategory(1) : QStringList{};
}

void CalibrationViewModel::setSelectedFilamentPreset(const QString &name)
{
    if (m_selectedFilamentPreset == name) return;
    m_selectedFilamentPreset = name;
    emit stateChanged();
}

// --- Actions ---

void CalibrationViewModel::startCalibration()
{
    if (m_selectedIndex < 0) return;
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

void CalibrationViewModel::goToStep(int stepIndex)
{
    if (m_service)
        m_service->goToStep(stepIndex);
}

void CalibrationViewModel::resetParameters()
{
    if (m_service && m_selectedIndex >= 0)
        m_service->resetCalibration(m_selectedIndex);
}
