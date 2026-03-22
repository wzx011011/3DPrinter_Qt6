#include "UndoRedoManager.h"

UndoRedoManager::UndoRedoManager(QObject *parent)
    : QObject(parent)
{
  m_stack.setUndoLimit(100);

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
