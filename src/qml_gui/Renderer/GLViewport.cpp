#include "GLViewport.h"
#include "GLViewportRenderer.h"
#include "GCodeRenderer.h"

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
  if (m_canvasType == CanvasPreview)
    return new GCodeRenderer();
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

void GLViewport::setPreviewData(const QByteArray &data)
{
  QMutexLocker lk(&m_eventMutex);
  m_previewData = data;
  ++m_previewVersion;
  lk.unlock();
  update();
}

bool GLViewport::takePreviewData(QByteArray &out, int &version)
{
  QMutexLocker lk(&m_eventMutex);
  if (version == m_previewVersion)
    return false;
  version = m_previewVersion;
  out = m_previewData;
  return true;
}

void GLViewport::setLayerMin(int v)
{
  if (m_layerMin == v)
    return;
  m_layerMin = v;
  update();
}

void GLViewport::setLayerMax(int v)
{
  if (m_layerMax == v)
    return;
  m_layerMax = v;
  update();
}

void GLViewport::setMoveEnd(int v)
{
  if (m_moveEnd == v)
    return;
  m_moveEnd = v;
  update();
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
  e.type = InputEvent::FitView;
  e.fitCX = cx;
  e.fitCY = cy;
  e.fitCZ = cz;
  e.fitRadius = r;
  m_events.append(e);
  update();
}

void GLViewport::undo()
{
  QMutexLocker lk(&m_eventMutex);
  InputEvent e;
  e.type = InputEvent::Undo;
  m_events.append(e);
  update();
}

void GLViewport::redo()
{
  QMutexLocker lk(&m_eventMutex);
  InputEvent e;
  e.type = InputEvent::Redo;
  m_events.append(e);
  update();
}

void GLViewport::clearHistory()
{
  QMutexLocker lk(&m_eventMutex);
  InputEvent e;
  e.type = InputEvent::ClearHistory;
  m_events.append(e);
  update();
}
