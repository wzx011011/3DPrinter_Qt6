#!/bin/bash
# verify_linux.sh — Linux 门禁：canonical verify 的 13 套件 + QmlSmokeTests。
# Windows 侧对应 scripts/auto_verify_with_vcvars.ps1；本脚本是 Linux 等价物。
#
# 用法（仓库根目录）:
#   bash scripts/verify_linux.sh            # 跑全部套件
#   bash scripts/verify_linux.sh CliTests   # 只跑指定套件
#
# 前提: build/ 内已构建好各测试二进制（cmake --build build --target all）。
# 退出码: 0 = 全绿；1 = 有失败；2 = 环境缺失。
#
# 2026-10-25 基线: 14 套件 726 用例全绿（Ubuntu 24.04 / Qt 6.12 / GCC）。

set -u
REPO="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO"
BUILD="${BUILD_DIR:-$REPO/build}"
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-offscreen}"
LOGDIR="${LOGDIR:-/tmp/owzx_linux_gate}"
mkdir -p "$LOGDIR"

# Style-literal ratchet gate (2026-10-25 UI 一致性批次): off-scale spacing /
# radius, raw hex/named colors, raw font sizes, Qt.rgba-family calls may only
# go down vs docs/style-baseline/allowed.json. Fail fast before the suites.
if ! python3 tools/style_literal_gate.py; then
  echo "----------------------------------------"
  echo "Linux 门禁: style-literal ratchet FAILED（见上方明细）"
  exit 2
fi

# QCoreApplication 系套件: 不接受 -platform（会打印 usage 并退出 1）
CORE_ONLY="CliTests QmlUiAuditTests"

DEFAULT_SUITES="CliTests E2EWorkflowTests ViewModelSmokeTests PrepareSceneDataTests \
ObjectPickingTests NavigatorCubeTests CameraParityTests ViewportContextMenuTests \
QmlUiAuditTests PartPlateTests AppToolTests PreviewParserTests GizmoGeometryTests \
QmlSmokeTests"

SUITES="${*:-$DEFAULT_SUITES}"
PASS=0; FAIL=0; FAILED_LIST=""

for S in $SUITES; do
  BIN="$BUILD/$S"
  if [ ! -x "$BIN" ]; then
    echo "MISSING $S (未构建: $BIN)"; FAIL=$((FAIL+1)); FAILED_LIST="$FAILED_LIST $S(missing)"; continue
  fi
  ARGS=""
  case " $CORE_ONLY " in *" $S "*) ARGS="";; *) ARGS="-platform $QT_QPA_PLATFORM";; esac
  timeout "${CASE_TIMEOUT:-600}" "$BIN" $ARGS > "$LOGDIR/$S.log" 2>&1
  RC=$?
  TOTALS=$(grep "Totals:" "$LOGDIR/$S.log" | tail -1)
  NFAIL=$(echo "$TOTALS" | grep -oE "[0-9]+ failed" | head -1 | cut -d' ' -f1)
  if [ "$RC" = "0" ] && [ "${NFAIL:-1}" = "0" ]; then
    NPASS=$(echo "$TOTALS" | grep -oE "[0-9]+ passed" | head -1 | cut -d' ' -f1)
    echo "PASS  $S  (${NPASS:-?} 用例)"
    PASS=$((PASS+1))
  else
    FIRST_FAIL=$(grep -E "^FAIL" "$LOGDIR/$S.log" | head -1 | cut -c1-90)
    echo "FAIL  $S  rc=$RC  ${TOTALS:-无 Totals}  ${FIRST_FAIL}"
    FAIL=$((FAIL+1)); FAILED_LIST="$FAILED_LIST $S"
  fi
done

echo "----------------------------------------"
echo "Linux 门禁: PASS=$PASS FAIL=$FAIL  日志: $LOGDIR"
[ $FAIL -gt 0 ] && echo "失败套件:$FAILED_LIST"
[ $FAIL -eq 0 ]
