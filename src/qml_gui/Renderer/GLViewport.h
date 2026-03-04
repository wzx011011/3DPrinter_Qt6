#pragma once
#include <QQuickFramebufferObject>
#include <QMutex>
#include <QList>
#include <QByteArray>

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
  /// 写入属性: QML 侧绑定 editorVm.meshData，触发网格上传 GPU
  Q_PROPERTY(QByteArray meshData READ meshData WRITE setMeshData)

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

  // --- 网格数据 (GUI 线程写，渲染线程通过 takeMesh 读) ---
  QByteArray meshData() const { QMutexLocker lk(&m_eventMutex); return m_meshData; }
  void setMeshData(const QByteArray &data);
  /** 渲染线程调用: 取走待上传的数据。若版本相同则返回 false。 */
  bool takeMesh(QByteArray &out, int &version);

  // --- Input event queue (consumed by GLViewportRenderer::synchronize) ---
  struct InputEvent
  {
    enum Type
    {
      Press,
      Move,
      Release,
      Wheel,
      FitView   ///< 相机自适应 bbox
    } type;
    Qt::MouseButton button = Qt::NoButton;
    Qt::MouseButtons buttons = Qt::NoButton;
    float x = 0.f;
    float y = 0.f;
    float wheelDelta = 0.f;
    // FitView payload
    float fitCX = 0.f, fitCY = 0.f, fitCZ = 0.f, fitRadius = 100.f;
  };

  /** Atomically swap out & return all pending events. */
  QList<InputEvent> takeEvents();

  /** QML 调用: 令相机自适应包含中心 (cx, cy, cz)、半径 r 的球体 */
  Q_INVOKABLE void requestFitView(float cx, float cy, float cz, float r);

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
  // 网格数据双缓冲
  QByteArray m_meshData;
  int m_meshVersion = 0;
};
