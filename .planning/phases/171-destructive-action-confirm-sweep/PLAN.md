# Phase 171: Destructive-Action Confirm Sweep

**Status:** Executed
**Workstream:** CL
**Requirement:** CL-01

## Result

Routed 6 destructive triggers through the existing ConfirmDialog component
(v5.2 Phase 169 shipped it but only wired deleteSelection):
- CaliHistoryDialog 清空 → clearConfirm
- HomePage cloudUnbindDevice → unbindConfirm (stages pending index)
- MultiMachinePage removeDevice → removeDeviceConfirm (stages pending index)
- MultiMachinePage stopAllLocalTasks → stopLocalTasksConfirm
- MultiMachinePage stopAllCloudTasks → stopCloudTasksConfirm
- MonitorPage disconnectDevice → disconnectConfirm

Note: v5.2 audit reported "11 remaining triggers" but direct grep showed only
6 actual sites — ObjectList has no bulk-delete trigger (the audit's
possibility was speculative).

## Verification
- QmlUiAuditTests 129/129 PASS
- OWzxSlicer link OK
