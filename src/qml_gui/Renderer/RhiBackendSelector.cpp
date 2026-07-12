#include "RhiBackendSelector.h"

#include <QByteArray>
#include <QLoggingCategory>

#include <memory>

#include <rhi/qrhi.h>
#include <rhi/qrhi_platform.h>

namespace {

struct RhiBackendCandidate
{
  QString name;
  QSGRendererInterface::GraphicsApi graphicsApi = QSGRendererInterface::Unknown;
  QRhi::Implementation implementation = QRhi::Null;
};

struct RhiProbeOwner
{
  std::unique_ptr<QRhi> rhi;
  QRhiD3D12InitParams d3d12Params;
  QRhiD3D11InitParams d3d11Params;
};

QString normalizeRequestedBackend(const QByteArray &value)
{
  const QString requested = QString::fromLocal8Bit(value).trimmed().toLower();
  if (requested == QLatin1String("1")
      || requested == QLatin1String("true")
      || requested == QLatin1String("on")
      || requested == QLatin1String("yes"))
    return QStringLiteral("auto");
  return requested;
}

// Phase 105 (D3D12-01): read the OWZX_D3D12_DEBUG env flag and return true for
// the same truthy values normalizeRequestedBackend accepts ("1"/"true"/"on"/"yes").
// This gates the D3D12 debug layer so Phase 106 can triage the startup
// 0xc0000005 access violation with GPU validation output. The gate is FULLY
// conditional on the env flag (DL-04 / Pitfall 5): when OWZX_D3D12_DEBUG is
// unset, the default OWzxSlicer.exe build behaves identically to pre-Phase-105
// (enableDebugLayer stays false, no perf hit, no release-build leak).
bool d3d12DebugLayerRequested()
{
  if (!qEnvironmentVariableIsSet("OWZX_D3D12_DEBUG"))
    return false;
  const QString value = QString::fromLocal8Bit(qgetenv("OWZX_D3D12_DEBUG")).trimmed().toLower();
  return value == QLatin1String("1")
      || value == QLatin1String("true")
      || value == QLatin1String("on")
      || value == QLatin1String("yes");
}

QVector<RhiBackendCandidate> defaultWindowsCandidates()
{
  // D3D11-first: D3D12 has a rendering crash on startup (Phase 26 isolation:
  // prepare render Segfault under D3D12, D3D11 works). Make D3D11 the safe
  // default for "auto", and D3D12 only via explicit OWZX_RHI_RENDERER=d3d12.
  return {
      {QStringLiteral("d3d11"), QSGRendererInterface::Direct3D11, QRhi::D3D11},
      {QStringLiteral("d3d12"), QSGRendererInterface::Direct3D12, QRhi::D3D12},
  };
}

QVector<RhiBackendCandidate> candidatesForRequest(const QString &requested, QStringList *failures)
{
  const QVector<RhiBackendCandidate> candidates = defaultWindowsCandidates();
  if (requested == QLatin1String("auto"))
    return candidates;

  for (const RhiBackendCandidate &candidate : candidates) {
    if (candidate.name == requested)
      return {candidate};
  }

  failures->append(QStringLiteral("unsupported OWZX_RHI_RENDERER value '%1'").arg(requested));
  return {};
}

bool probeBackend(const RhiBackendCandidate &candidate, QString *failure)
{
  RhiProbeOwner owner;
  QRhiInitParams *params = nullptr;

  switch (candidate.implementation) {
    case QRhi::D3D12:
      params = &owner.d3d12Params;
      // Phase 105 (D3D12-01): enable the D3D12 debug layer for the probe path
      // when OWZX_D3D12_DEBUG is set, BEFORE QRhi::create (DL-02). This path
      // only calls QRhi::create (it does not render), so the debug layer here
      // surfaces device/adapter creation validation. The live render-path
      // crash (RhiViewportRenderer.cpp:282-298 beginPass-after-resourceUpdate)
      // is covered by QSG_RHI_DEBUG set in main_qml.cpp before QGuiApplication
      // (DL-03). Fully env-gated (DL-04 / Pitfall 5): default build unchanged.
      if (d3d12DebugLayerRequested()) {
        owner.d3d12Params.enableDebugLayer = true;
        qInfo("D3D12-01: OWZX_D3D12_DEBUG set; enabling D3D12 debug layer on probe path");
      }
      break;
    case QRhi::D3D11:
      params = &owner.d3d11Params;
      break;
    default:
      *failure = QStringLiteral("unsupported QRhi implementation");
      return false;
  }

  owner.rhi.reset(QRhi::create(candidate.implementation, params));
  if (!owner.rhi) {
    *failure = QStringLiteral("QRhi::create failed");
    return false;
  }

  return true;
}

} // namespace

QString RhiBackendSelection::diagnostics() const
{
  QStringList parts;
  parts.append(QStringLiteral("enabled=%1").arg(enabled ? QStringLiteral("true") : QStringLiteral("false")));
  parts.append(QStringLiteral("requested=%1").arg(requested.isEmpty() ? QStringLiteral("<unset>") : requested));
  parts.append(QStringLiteral("selected=%1").arg(selectedBackend.isEmpty() ? QStringLiteral("<none>") : selectedBackend));

  QStringList attemptParts;
  for (const RhiBackendAttempt &attempt : attempts) {
    if (attempt.success)
      attemptParts.append(QStringLiteral("%1:ok").arg(attempt.name));
    else
      attemptParts.append(QStringLiteral("%1:failed(%2)").arg(attempt.name, attempt.failureReason));
  }
  if (!attemptParts.isEmpty())
    parts.append(QStringLiteral("attempts=[%1]").arg(attemptParts.join(QStringLiteral(", "))));

  if (!failureReasons.isEmpty())
    parts.append(QStringLiteral("failures=[%1]").arg(failureReasons.join(QStringLiteral("; "))));

  return parts.join(QStringLiteral(" "));
}

RhiBackendSelection selectRhiBackendFromEnvironment()
{
  RhiBackendSelection selection;
  if (!qEnvironmentVariableIsSet("OWZX_RHI_RENDERER"))
    return selection;

  const QByteArray value = qgetenv("OWZX_RHI_RENDERER");
  selection.enabled = true;
  selection.requested = normalizeRequestedBackend(value);
  if (selection.requested.isEmpty()) {
    selection.failureReasons.append(QStringLiteral("OWZX_RHI_RENDERER is set but empty"));
    return selection;
  }

#ifndef Q_OS_WIN
  selection.failureReasons.append(QStringLiteral("QRhi renderer gate is currently implemented for Windows only"));
  return selection;
#else
  const QVector<RhiBackendCandidate> candidates = candidatesForRequest(selection.requested, &selection.failureReasons);
  for (const RhiBackendCandidate &candidate : candidates) {
    RhiBackendAttempt attempt;
    attempt.name = candidate.name;
    attempt.graphicsApi = candidate.graphicsApi;

    QString failure;
    attempt.success = probeBackend(candidate, &failure);
    attempt.failureReason = failure;
    selection.attempts.append(attempt);

    if (attempt.success) {
      selection.selectedBackend = candidate.name;
      selection.selectedGraphicsApi = candidate.graphicsApi;
      selection.canUseRhi = true;
      return selection;
    }

    selection.failureReasons.append(QStringLiteral("%1: %2").arg(candidate.name, failure));
  }

  return selection;
#endif
}
