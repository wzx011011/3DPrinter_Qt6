#include "GLViewport.h"
#include "GLViewportRenderer.h"

#include <QMouseEvent>
#include <QWheelEvent>
#include <utility> // std::exchange

GLViewport::GLViewport(QQuickItem *parent)
    : QQuickFramebufferObject(parent)
{
  setAcceptedMouseButtons(Qt::AllButtons);
  setAcceptHoverEvents(false);
  // FBO content is flipped relative to OpenGL convention; mirror so axes face
  // the right way without inverting in the shader.
  setMirrorVertically(true);
}

void GLViewport::setCanvasType(int t)
{
  if (m_canvasType != t)
  {
    m_canvasType = t;
    emit canvasTypeChanged();
    update();
  }
}

QQuickFramebufferObject::Renderer *GLViewport::createRenderer() const
{
  return new GLViewportRenderer();
}

QList<GLViewport::InputEvent> GLViewport::takeEvents()
{
  QMutexLocker lk(&m_eventMutex);
  return std::exchange(m_events, {});
}

// ---- Mouse / wheel handlers ------------------------------------------------

void GLViewport::mousePressEvent(QMouseEvent *event)
{
  QMutexLocker lk(&m_eventMutex);
  m_events.append({InputEvent::Press,
                   event->button(), event->buttons(),
                   (float)event->position().x(),
                   (float)event->position().y(), 0.f});
  update();
}

void GLViewport::mouseMoveEvent(QMouseEvent *event)
{
  QMutexLocker lk(&m_eventMutex);
  m_events.append({InputEvent::Move,
                   Qt::NoButton, event->buttons(),
                   (float)event->position().x(),
                   (float)event->position().y(), 0.f});
  update();
}

void GLViewport::mouseReleaseEvent(QMouseEvent *event)
{
  QMutexLocker lk(&m_eventMutex);
  m_events.append({InputEvent::Release,
                   event->button(), event->buttons(),
                   (float)event->position().x(),
                   (float)event->position().y(), 0.f});
  update();
}

void GLViewport::wheelEvent(QWheelEvent *event)
{
  QMutexLocker lk(&m_eventMutex);
  m_events.append({InputEvent::Wheel,
                   Qt::NoButton, Qt::NoButton,
                   0.f, 0.f,
                   (float)event->angleDelta().y()});
  update();
}
