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

void GLViewport::setMeshData(const QByteArray &data)
{
  QMutexLocker lk(&m_eventMutex);
  m_meshData = data;
  ++m_meshVersion;
  lk.unlock();
  update(); // 触发重绘，渲染线程会在 synchronize() 里取走数据
}

bool GLViewport::takeMesh(QByteArray &out, int &version)
{
  QMutexLocker lk(&m_eventMutex);
  if (version == m_meshVersion)
    return false; // 无新数据
  version = m_meshVersion;
  out = m_meshData;
  return true;
}

// ---- Mouse / wheel handlers ------------------------------------------------

void GLViewport::mousePressEvent(QMouseEvent *event)
{
  QMutexLocker lk(&m_eventMutex);
  m_events.append({InputEvent::Press,
                   event->button(), event->buttons(),
                   (float)event->position().x(),
                   (float)event->position().y(), 0.f});
  event->accept(); // 必须 accept，否则事件冗泡丢失
  update();
}

void GLViewport::mouseMoveEvent(QMouseEvent *event)
{
  QMutexLocker lk(&m_eventMutex);
  m_events.append({InputEvent::Move,
                   Qt::NoButton, event->buttons(),
                   (float)event->position().x(),
                   (float)event->position().y(), 0.f});
  event->accept();
  update();
}

void GLViewport::mouseReleaseEvent(QMouseEvent *event)
{
  QMutexLocker lk(&m_eventMutex);
  m_events.append({InputEvent::Release,
                   event->button(), event->buttons(),
                   (float)event->position().x(),
                   (float)event->position().y(), 0.f});
  event->accept();
  update();
}

void GLViewport::wheelEvent(QWheelEvent *event)
{
  QMutexLocker lk(&m_eventMutex);
  m_events.append({InputEvent::Wheel,
                   Qt::NoButton, Qt::NoButton,
                   0.f, 0.f,
                   (float)event->angleDelta().y()});
  event->accept();
  update();
}

void GLViewport::requestFitView(float cx, float cy, float cz, float r)
{
  QMutexLocker lk(&m_eventMutex);
  InputEvent e;
  e.type     = InputEvent::FitView;
  e.fitCX    = cx;
  e.fitCY    = cy;
  e.fitCZ    = cz;
  e.fitRadius = r;
  m_events.append(e);
  update();
}
