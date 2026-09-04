#pragma once

#include <QObject>
#include <QUndoStack>
#include <QVector3D>
#include <QString>
#include <QSet>

class ProjectServiceMock;

/// Central undo/redo manager wrapping QUndoStack (对齐上游 UndoRedo 框架).
/// Owned by BackendContext, shared with EditorViewModel via pointer injection.
class UndoRedoManager final : public QObject
{
  Q_OBJECT

  /// Exposed to QML for keyboard shortcut binding
  Q_PROPERTY(bool canUndo READ canUndo NOTIFY stateChanged)
  Q_PROPERTY(bool canRedo READ canRedo NOTIFY stateChanged)
  Q_PROPERTY(QString undoText READ undoText NOTIFY stateChanged)
  Q_PROPERTY(QString redoText READ redoText NOTIFY stateChanged)
  Q_PROPERTY(int undoLimit READ undoLimit WRITE setUndoLimit NOTIFY undoLimitChanged)
  /// Wave 4b (perf): tracked heavy-payload bytes currently referenced by the
  /// stack (diagnostic; the real eviction knob is the memory budget).
  Q_PROPERTY(qint64 heavyBytes READ heavyBytes NOTIFY stateChanged)

public:
  explicit UndoRedoManager(QObject *parent = nullptr);

  bool canUndo() const;
  bool canRedo() const;
  QString undoText() const;
  QString redoText() const;
  int undoLimit() const;
  void setUndoLimit(int limit);

  /// Push a command onto the stack. Takes ownership.
  void push(QUndoCommand *cmd);

  /// Wave 4b (perf): heavy-payload accounting + budget eviction.
  qint64 heavyBytes() const { return m_heavyBytes; }
  qint64 memoryBudgetBytes() const { return m_memoryBudget; }
  void setMemoryBudgetBytes(qint64 bytes);

  Q_INVOKABLE void undo();
  Q_INVOKABLE void redo();
  Q_INVOKABLE void clear();

  /// Begin a macro (group multiple commands into one undo step).
  /// Each beginMacro() must be matched by endMacro().
  Q_INVOKABLE void beginMacro(const QString &text);
  Q_INVOKABLE void endMacro();

  /// Check if currently inside a macro
  bool isInMacro() const { return m_inMacro; }

  QUndoStack *stack() { return &m_stack; }

signals:
  void stateChanged();
  void undoLimitChanged();

private:
  QUndoStack m_stack;
  bool m_inMacro = false;
  // Wave 4b (perf): heavy-payload budget. The default is 10% of physical
  // RAM (same default as upstream set_memory_limit; env override
  // OWZX_UNDO_BUDGET_MIB). Exceeding it shrinks the QUndoStack limit so the
  // OLDEST commands are evicted (QUndoStack drops from the stack bottom).
  qint64 m_heavyBytes = 0;
  qint64 m_memoryBudget = 0;
  void enforceBudget();
};
