#pragma once
// ---------------------------------------------------------------------------
// GLViewport test stub — used ONLY by VisualRegressionTests.
//
// The real GLViewport (QQuickFramebufferObject) cannot be used in the visual
// regression test because QQuickFramebufferObject requires OpenGL and its
// heap cleanup is incompatible with the D3D11 backend that Qt Quick uses by
// default on Windows.
//
// This stub registers under the same QML URI ("CrealityGL" 1.0 "GLViewport")
// so PreparePage.qml loads without error; it simply renders nothing (a
// transparent item) during screenshot capture.
// ---------------------------------------------------------------------------
#include <QQuickItem>

class GLViewport : public QQuickItem
{
  Q_OBJECT
  Q_PROPERTY(int canvasType READ canvasType WRITE setCanvasType
                 NOTIFY canvasTypeChanged)

public:
  enum CanvasType
  {
    CanvasView3D = 0,
    CanvasPreview = 1
  };
  Q_ENUM(CanvasType)

  explicit GLViewport(QQuickItem *parent = nullptr)
      : QQuickItem(parent)
  {
    // No-op: no GL, no FBO.
    setFlag(ItemHasContents, false);
  }

  int canvasType() const { return m_canvasType; }
  void setCanvasType(int t)
  {
    if (m_canvasType != t)
    {
      m_canvasType = t;
      emit canvasTypeChanged();
    }
  }

signals:
  void canvasTypeChanged();

private:
  int m_canvasType = CanvasView3D;
};
