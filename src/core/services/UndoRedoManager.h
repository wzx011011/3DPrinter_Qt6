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
};
