#pragma once
#include <QQuickFramebufferObject>
#include <QMutex>
#include <QList>

/**
 * GLViewport — QML-exposed OpenGL viewport.
 *
 * Registered in QML as: import CrealityGL 1.0 → GLViewport {}
 *
 * Supports two roles:
 *   canvasType: GLViewport.CanvasView3D   – 3-D prepare scene
 *   canvasType: GLViewport.CanvasPreview  – G-code preview scene (Phase F)
 */
class GLViewport : public QQuickFramebufferObject
{
  Q_OBJECT
  Q_PROPERTY(int canvasType READ canvasType WRITE setCanvasType NOTIFY canvasTypeChanged)

public:
  enum CanvasType
  {
    CanvasView3D = 0,
    CanvasPreview = 1
  };
  Q_ENUM(CanvasType)

  explicit GLViewport(QQuickItem *parent = nullptr);

  // QQuickFramebufferObject interface
  Renderer *createRenderer() const override;

  // Property accessors
  int canvasType() const { return m_canvasType; }
  void setCanvasType(int t);

  // --- Input event queue (consumed by GLViewportRenderer::synchronize) ---
  struct InputEvent
  {
    enum Type
    {
      Press,
      Move,
      Release,
      Wheel
    } type;
    Qt::MouseButton button = Qt::NoButton;
    Qt::MouseButtons buttons = Qt::NoButton;
    float x = 0.f;
    float y = 0.f;
    float wheelDelta = 0.f;
  };

  /** Atomically swap out & return all pending events. */
  QList<InputEvent> takeEvents();

signals:
  void canvasTypeChanged();

protected:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

private:
  int m_canvasType = CanvasView3D;
  mutable QMutex m_eventMutex;
  QList<InputEvent> m_events;
};
