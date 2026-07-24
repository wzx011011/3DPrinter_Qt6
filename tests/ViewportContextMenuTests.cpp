#include <QCoreApplication>
#include <QMouseEvent>
#include <QSignalSpy>
#include <QtTest>
#include <cstring>

#include "qml_gui/Renderer/RhiViewport.h"
#include "qml_gui/Renderer/SoftwareViewport.h"
#include "qml_gui/Renderer/ViewportContextHit.h"

namespace
{
  template <typename Viewport>
  void sendRightClick(Viewport &viewport, const QPointF &position)
  {
    QMouseEvent press(QEvent::MouseButtonPress, position, position,
                      Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&viewport, &press);
    QMouseEvent release(QEvent::MouseButtonRelease, position, position,
                        Qt::RightButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&viewport, &release);
  }

  template <typename Viewport>
  void sendRightDrag(Viewport &viewport, const QPointF &start, const QPointF &end)
  {
    QMouseEvent press(QEvent::MouseButtonPress, start, start,
                      Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&viewport, &press);
    QMouseEvent move(QEvent::MouseMove, end, end,
                     Qt::NoButton, Qt::RightButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&viewport, &move);
    QMouseEvent release(QEvent::MouseButtonRelease, end, end,
                        Qt::RightButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&viewport, &release);
  }

  template <typename T>
  void appendValue(QByteArray &packet, const T &value)
  {
    packet.append(reinterpret_cast<const char *>(&value), sizeof(T));
  }

  QByteArray modelPacket()
  {
    QByteArray packet;
    const qint32 objectCount = 1;
    const qint32 renderId = 101;
    const qint32 triangleCount = 1;
    const float vertices[] = {
        -60.0f, 0.0f, -60.0f,
         60.0f, 0.0f, -60.0f,
          0.0f, 0.0f,  60.0f};
    appendValue(packet, objectCount);
    appendValue(packet, renderId);
    appendValue(packet, triangleCount);
    packet.append(reinterpret_cast<const char *>(vertices), sizeof(vertices));
    const float bounds[] = {
        -60.0f, 0.0f, -60.0f,
         60.0f, 0.0f,  60.0f};
    packet.append(reinterpret_cast<const char *>(bounds), sizeof(bounds));
    return packet;
  }

  template <typename Viewport>
  void configureViewport(Viewport &viewport, bool includeModel)
  {
    viewport.setWidth(220);
    viewport.setHeight(220);
    viewport.setBedWidth(200.0f);
    viewport.setBedDepth(200.0f);
    viewport.setBedOriginX(-100.0f);
    viewport.setBedOriginY(-100.0f);
    viewport.setCurrentPlateIndex(0);
    viewport.setPlateCount(1);
    viewport.setActivePlateObjectIndices(includeModel ? QVariantList{7} : QVariantList{});
    viewport.setMeshBatchSourceObjectIndices(includeModel ? QVariantList{7} : QVariantList{});
    viewport.setMeshBatchVolumeIndices(includeModel ? QVariantList{2} : QVariantList{});
    viewport.setMeshBatchInstanceIndices(includeModel ? QVariantList{3} : QVariantList{});
    viewport.setMeshData(includeModel ? modelPacket() : QByteArray{});
    viewport.requestFitView(0.0f, 0.0f, 0.0f, 100.0f);
    viewport.requestViewPreset(0);
  }

  template <typename Viewport>
  void verifyMenuRequest(Viewport &viewport,
                         const QPointF &position,
                         int expectedTarget,
                         int expectedSource = -1,
                         int expectedVolume = -1,
                         int expectedInstance = -1)
  {
    QSignalSpy spy(&viewport, &Viewport::contextMenuRequested);
    sendRightClick(viewport, position);
    QCOMPARE(spy.count(), 1);
    const QList<QVariant> arguments = spy.takeFirst();
    QCOMPARE(arguments.at(0).toInt(), expectedTarget);
    QCOMPARE(arguments.at(1).toInt(), expectedSource);
    QCOMPARE(arguments.at(2).toInt(), expectedVolume);
    QCOMPARE(arguments.at(3).toInt(), expectedInstance);
    QCOMPARE(arguments.at(4).toInt(), expectedTarget == int(ViewportContextTarget::Empty) ? -1 : 0);
    QCOMPARE(arguments.at(5).toDouble(), position.x());
    QCOMPARE(arguments.at(6).toDouble(), position.y());
  }
}

class ViewportContextMenuTests final : public QObject
{
  Q_OBJECT

private slots:
  void rhiAndSoftwareExposeTheSameContextSignal();
  void directRightClickClassificationMatchesAcrossRenderers();
  void dragToolLayerAndWipeSuppressContextRequests();
};

void ViewportContextMenuTests::rhiAndSoftwareExposeTheSameContextSignal()
{
  RhiViewport rhi;
  SoftwareViewport software;
  QVERIFY(rhi.metaObject()->indexOfSignal("contextMenuRequested(int,int,int,int,int,double,double)") >= 0);
  QVERIFY(software.metaObject()->indexOfSignal("contextMenuRequested(int,int,int,int,int,double,double)") >= 0);
}

void ViewportContextMenuTests::directRightClickClassificationMatchesAcrossRenderers()
{
  RhiViewport rhi;
  SoftwareViewport software;
  const QPointF center(110.0, 110.0);
  const QPointF outside(5.0, 5.0);

  configureViewport(rhi, true);
  configureViewport(software, true);
  verifyMenuRequest(rhi, center, int(ViewportContextTarget::Part), 7, 2, 3);
  verifyMenuRequest(software, center, int(ViewportContextTarget::Part), 7, 2, 3);

  configureViewport(rhi, false);
  configureViewport(software, false);
  verifyMenuRequest(rhi, center, int(ViewportContextTarget::Plate));
  verifyMenuRequest(software, center, int(ViewportContextTarget::Plate));
  rhi.setGizmoMode(RhiViewport::GizmoMove);
  software.setGizmoMode(SoftwareViewport::GizmoMove);
  verifyMenuRequest(rhi, center, int(ViewportContextTarget::Plate));
  verifyMenuRequest(software, center, int(ViewportContextTarget::Plate));
  verifyMenuRequest(rhi, outside, int(ViewportContextTarget::Empty));
  verifyMenuRequest(software, outside, int(ViewportContextTarget::Empty));
}

void ViewportContextMenuTests::dragToolLayerAndWipeSuppressContextRequests()
{
  RhiViewport rhi;
  SoftwareViewport software;
  const QPointF center(110.0, 110.0);
  configureViewport(rhi, false);
  configureViewport(software, false);

  QSignalSpy rhiSpy(&rhi, &RhiViewport::contextMenuRequested);
  QSignalSpy softwareSpy(&software, &SoftwareViewport::contextMenuRequested);

  sendRightDrag(rhi, center, QPointF(130.0, 130.0));
  sendRightDrag(software, center, QPointF(130.0, 130.0));
  QCOMPARE(rhiSpy.count(), 0);
  QCOMPARE(softwareSpy.count(), 0);

  rhi.setLayerEditingInputActive(true);
  software.setLayerEditingInputActive(true);
  sendRightClick(rhi, center);
  sendRightClick(software, center);
  QCOMPARE(rhiSpy.count(), 0);
  QCOMPARE(softwareSpy.count(), 0);

  rhi.setLayerEditingInputActive(false);
  software.setLayerEditingInputActive(false);
  rhi.setContextToolInputCaptured(true);
  software.setContextToolInputCaptured(true);
  sendRightClick(rhi, center);
  sendRightClick(software, center);
  QCOMPARE(rhiSpy.count(), 0);
  QCOMPARE(softwareSpy.count(), 0);

  rhi.setContextToolInputCaptured(false);
  software.setContextToolInputCaptured(false);
  rhi.setShowWipeTower(true);
  software.setShowWipeTower(true);
  rhi.setWipeTowerWidth(30.0f);
  rhi.setWipeTowerDepth(30.0f);
  rhi.setWipeTowerX(0.0f);
  rhi.setWipeTowerZ(0.0f);
  software.setWipeTowerWidth(30.0f);
  software.setWipeTowerDepth(30.0f);
  software.setWipeTowerX(0.0f);
  software.setWipeTowerZ(0.0f);
  sendRightClick(rhi, center);
  sendRightClick(software, center);
  QCOMPARE(rhiSpy.count(), 0);
  QCOMPARE(softwareSpy.count(), 0);
}

QTEST_MAIN(ViewportContextMenuTests)
#include "ViewportContextMenuTests.moc"
