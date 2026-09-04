#include "UndoRedoManager.h"

#include "UndoCommands.h"

#include <QtGlobal>

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

UndoRedoManager::UndoRedoManager(QObject *parent)
    : QObject(parent)
{
  m_stack.setUndoLimit(100);

  // Wave 4b (perf): heavy-payload budget, default 10% of physical RAM
  // (upstream set_memory_limit default; OWZX_UNDO_BUDGET_MIB overrides).
  qint64 budgetMiB = qEnvironmentVariableIntValue("OWZX_UNDO_BUDGET_MIB");
  if (budgetMiB <= 0) {
#ifdef Q_OS_WIN
    MEMORYSTATUSEX status{};
    status.dwLength = sizeof(status);
    if (GlobalMemoryStatusEx(&status))
      budgetMiB = qint64(status.ullTotalPhys) / (1024 * 1024) / 10;
    else
      budgetMiB = 512;
#else
    budgetMiB = 512;
#endif
  }
  m_memoryBudget = budgetMiB * 1024 * 1024;

  connect(&m_stack, &QUndoStack::canUndoChanged, this, &UndoRedoManager::stateChanged);
  connect(&m_stack, &QUndoStack::canRedoChanged, this, &UndoRedoManager::stateChanged);
  connect(&m_stack, &QUndoStack::undoTextChanged, this, &UndoRedoManager::stateChanged);
  connect(&m_stack, &QUndoStack::redoTextChanged, this, &UndoRedoManager::stateChanged);
  connect(&m_stack, &QUndoStack::cleanChanged, this, &UndoRedoManager::stateChanged);
}

bool UndoRedoManager::canUndo() const { return m_stack.canUndo(); }
bool UndoRedoManager::canRedo() const { return m_stack.canRedo(); }
QString UndoRedoManager::undoText() const { return m_stack.undoText(); }
QString UndoRedoManager::redoText() const { return m_stack.redoText(); }
int UndoRedoManager::undoLimit() const { return m_stack.undoLimit(); }

void UndoRedoManager::setUndoLimit(int limit)
{
  if (m_stack.undoLimit() == limit)
    return;
  m_stack.setUndoLimit(limit);
  emit undoLimitChanged();
}

void UndoRedoManager::push(QUndoCommand *cmd)
{
  m_stack.push(cmd);
  // Wave 4b (perf): track heavy snapshot payloads and enforce the budget.
  if (const auto *heavy = dynamic_cast<const IHeavyCommand *>(cmd))
    m_heavyBytes += heavy->heavyBytes();
  enforceBudget();
}

void UndoRedoManager::enforceBudget()
{
  if (m_heavyBytes <= m_memoryBudget || m_memoryBudget <= 0)
    return;
  // Qt6 API constraint: QUndoStack::setUndoLimit() is REJECTED while the
  // stack is non-empty ("an undo limit can only be set when the stack is
  // empty"), so upstream's drop-oldest eviction has no direct equivalent.
  // Honest degradation: clear the whole history once the budget is exceeded
  // so large-model sessions can no longer accumulate
  // 100 x (2 x whole-project 3MF). Documented delta in
  // docs/perf-baseline.md; a disk-backed snapshot eviction (upstream parity)
  // is the follow-up if the UX requires partial history retention.
  qWarning("[UndoRedo] memory budget exceeded (%lld > %lld bytes) -- "
           "clearing undo history",
           m_heavyBytes, m_memoryBudget);
  m_stack.clear();
  m_heavyBytes = 0;
  emit stateChanged();
}

void UndoRedoManager::setMemoryBudgetBytes(qint64 bytes)
{
  m_memoryBudget = bytes;
  enforceBudget();
}

void UndoRedoManager::undo()
{
  m_stack.undo();
}

void UndoRedoManager::redo()
{
  m_stack.redo();
}

void UndoRedoManager::clear()
{
  m_stack.clear();
  m_heavyBytes = 0;
}

void UndoRedoManager::beginMacro(const QString &text)
{
  m_stack.beginMacro(text);
  m_inMacro = true;
}

void UndoRedoManager::endMacro()
{
  m_stack.endMacro();
  m_inMacro = false;
}
