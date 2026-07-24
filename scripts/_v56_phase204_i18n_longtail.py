#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
_v56_phase204_i18n_longtail.py
==============================
v5.6 Phase 204 — de/fr/ja/ko long-tail translation to >=85% coverage.

The core script (translate_core_i18n.py) already covered the highest-frequency
core UI subset. This Phase 204 script targets the REMAINING unfinished entries
(the "long tail"), ~496 distinct source strings spanning:

  - English-sourced qsTr strings (Calibration, Import STL, Fit view, ...)
  - Chinese-sourced slicer parameter strings (Z 缝, 回退距离, 悬空风扇, ...)
  - View / gizmo / cut / measure tool labels
  - About / version / shortcut strings
  - Date/time format tokens, unit suffixes (mm, MB, °C)
  - A few GBK-mojibake source strings (e.g. 瀵煎嚭澶辫触 = 导出失败 / "Export failed")

Approach: EXACT-match glossary (no substring concatenation, unlike the v5.3
glossary), reusing the proven regex-in-place-replacement pattern from
translate_core_i18n.py (NOT xml.etree, which reorders attributes and breaks
Qt Linguist format). Idempotent and safe to re-run.

Quality rules:
  - Technical terms PLA/ABS/PETG/TPU/STL/G-code/PEI/HMS/MMU/STEP/OBJ/AMF/3MF/
    SVG/SLA/MQTT/SSDP/SD/API/IP/DNS/OpenGL/QML stay untranslated.
  - Brand / model names (Creality, K1, K2, Ender, CR-10) stay untranslated.
  - Units (mm, °C, mm/s, MB, %) and placeholders (%1, %2) preserved verbatim.
  - Translations are native-quality, domain-accurate 3D-printer UI strings.

Usage:
    python scripts/_v56_phase204_i18n_longtail.py de
    python scripts/_v56_phase204_i18n_longtail.py fr
    python scripts/_v56_phase204_i18n_longtail.py ja
    python scripts/_v56_phase204_i18n_longtail.py ko
    python scripts/_v56_phase204_i18n_longtail.py all
    python scripts/_v56_phase204_i18n_longtail.py de --dry-run   # report only
"""

import os
import re
import sys
import argparse
import xml.sax.saxutils as saxutils

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
I18N = os.path.join(ROOT, 'i18n')

LANGS = ('de', 'fr', 'ja', 'ko')


# ---------------------------------------------------------------------------
# PHASE 204 LONG-TAIL GLOSSARY
#
# Keys are the EXACT escaped <source> text found in the .ts file.
# Translations are written XML-escaped; %1/%2 placeholders are preserved.
# ---------------------------------------------------------------------------
LONGTAIL = {
    # ---- Units / numbers / placeholders (identical across langs; keep) ----
    " mm³": {"de": " mm³", "fr": " mm³", "ja": " mm³", "ko": " mm³"},
    "%1": {"de": "%1", "fr": "%1", "ja": "%1", "ko": "%1"},
    "%1... %2%": {"de": "%1... %2%", "fr": "%1... %2%", "ja": "%1... %2%", "ko": "%1... %2%"},
    "%1ms": {"de": "%1 ms", "fr": "%1 ms", "ja": "%1 ms", "ko": "%1 ms"},
    " mm": {"de": " mm", "fr": " mm", "ja": " mm", "ko": " mm"},
    "A": {"de": "A", "fr": "A", "ja": "A", "ko": "A"},
    "A: ": {"de": "A: ", "fr": "A : ", "ja": "A: ", "ko": "A: "},
    "A：": {"de": "A: ", "fr": "A : ", "ja": "A: ", "ko": "A: "},
    "B": {"de": "B", "fr": "B", "ja": "B", "ko": "B"},
    "B: ": {"de": "B: ", "fr": "B : ", "ja": "B: ", "ko": "B: "},
    "B：": {"de": "B: ", "fr": "B : ", "ja": "B: ", "ko": "B: "},
    "X:": {"de": "X:", "fr": "X :", "ja": "X:", "ko": "X:"},
    "Y:": {"de": "Y:", "fr": "Y :", "ja": "Y:", "ko": "Y:"},
    "Z:": {"de": "Z:", "fr": "Z :", "ja": "Z:", "ko": "Z:"},
    "mm": {"de": "mm", "fr": "mm", "ja": "mm", "ko": "mm"},
    "(Ctrl+Shift+X)": {"de": "(Strg+Umschalt+X)", "fr": "(Ctrl+Maj+X)", "ja": "(Ctrl+Shift+X)", "ko": "(Ctrl+Shift+X)"},
    "(G)": {"de": "(G)", "fr": "(G)", "ja": "(G)", "ko": "(G)"},

    # ---- Single/double-char common labels ----
    "All": {"de": "Alle", "fr": "Tout", "ja": "すべて", "ko": "전체"},
    "busy": {"de": "beschäftigt", "fr": "occupé", "ja": "ビジー", "ko": "사용 중"},
    "Color": {"de": "Farbe", "fr": "Couleur", "ja": "色", "ko": "색상"},
    "Cut": {"de": "Schneiden", "fr": "Couper", "ja": "切断", "ko": "자르기"},
    "Fit": {"de": "Anpassen", "fr": "Ajuster", "ja": "フィット", "ko": "맞춤"},
    "Hollow": {"de": "Aushöhlen", "fr": "Creuser", "ja": "中空", "ko": "속 비움"},
    "Material": {"de": "Material", "fr": "Matériau", "ja": "材料", "ko": "재료"},
    "Max": {"de": "Max", "fr": "Max", "ja": "最大", "ko": "최대"},
    "Min": {"de": "Min", "fr": "Min", "ja": "最小", "ko": "최소"},
    "Move": {"de": "Bewegen", "fr": "Déplacer", "ja": "移動", "ko": "이동"},
    "OK": {"de": "OK", "fr": "OK", "ja": "OK", "ko": "확인"},
    "Path": {"de": "Pfad", "fr": "Chemin", "ja": "パス", "ko": "경로"},
    "Plug": {"de": "Stecker", "fr": "Fiche", "ja": "プラグ", "ko": "플러그"},
    "Ready": {"de": "Bereit", "fr": "Prêt", "ja": "準備完了", "ko": "준비됨"},
    "REC": {"de": "REC", "fr": "REC", "ja": "REC", "ko": "REC"},
    "Rotate": {"de": "Drehen", "fr": "Pivoter", "ja": "回転", "ko": "회전"},
    "Scale": {"de": "Skalieren", "fr": "Mettre à l'échelle", "ja": "スケール", "ko": "크기 조정"},
    "Size": {"de": "Größe", "fr": "Taille", "ja": "サイズ", "ko": "크기"},
    "Snap": {"de": "Einrasten", "fr": "Aimantation", "ja": "スナップ", "ko": "스냅"},
    "Stale": {"de": "Veraltet", "fr": "Périmé", "ja": "古い", "ko": "오래됨"},
    "Trace": {"de": "Nachverfolgen", "fr": "Tracer", "ja": "トレース", "ko": "추적"},
    "Modified": {"de": "Geändert", "fr": "Modifié", "ja": "変更済み", "ko": "수정됨"},
    "Cancel": {"de": "Abbrechen", "fr": "Annuler", "ja": "キャンセル", "ko": "취소"},
    "Default": {"de": "Standard", "fr": "Par défaut", "ja": "デフォルト", "ko": "기본값"},
    "Format": {"de": "Format", "fr": "Format", "ja": "形式", "ko": "형식"},
    "Measure": {"de": "Messen", "fr": "Mesurer", "ja": "測定", "ko": "측정"},
    "Progress": {"de": "Fortschritt", "fr": "Progression", "ja": "進行状況", "ko": "진행률"},
    "Settings": {"de": "Einstellungen", "fr": "Paramètres", "ja": "設定", "ko": "설정"},
    "Printer": {"de": "Drucker", "fr": "Imprimante", "ja": "プリンター", "ko": "프린터"},
    "Process": {"de": "Prozess", "fr": "Processus", "ja": "プロセス", "ko": "공정"},
    "Failed": {"de": "Fehlgeschlagen", "fr": "Échec", "ja": "失敗", "ko": "실패"},
    "Error": {"de": "Fehler", "fr": "Erreur", "ja": "エラー", "ko": "오류"},
    "Info": {"de": "Info", "fr": "Info", "ja": "情報", "ko": "정보"},
    "Warning": {"de": "Warnung", "fr": "Avertissement", "ja": "警告", "ko": "경고"},
    "Debug": {"de": "Debug", "fr": "Débogage", "ja": "デバッグ", "ko": "디버그"},
    "About": {"de": "Über", "fr": "À propos", "ja": "バージョン情報", "ko": "정보"},
    "Dowel": {"de": "Dübel", "fr": "Tourillon", "ja": "ダボ", "ko": "다우엘"},
    "Frustum": {"de": "Kegelstumpf", "fr": "Tronc de cône", "ja": "円錐台", "ko": "절두체"},
    "Prism": {"de": "Prisma", "fr": "Prisme", "ja": "角柱", "ko": "각기둥"},
    "Emboss": {"de": "Prägen", "fr": "Embossage", "ja": "エンボス", "ko": "엠보싱"},
    "HMS": {"de": "HMS", "fr": "HMS", "ja": "HMS", "ko": "HMS"},
    "PC": {"de": "PC", "fr": "PC", "ja": "PC", "ko": "PC"},
    "PEI": {"de": "PEI", "fr": "PEI", "ja": "PEI", "ko": "PEI"},
    "EP": {"de": "EP", "fr": "EP", "ja": "EP", "ko": "EP"},
    "SVG": {"de": "SVG", "fr": "SVG", "ja": "SVG", "ko": "SVG"},
    "Sliced": {"de": "Geslict", "fr": "Tranché", "ja": "スライス済み", "ko": "슬라이스됨"},
    "Unavailable": {"de": "Nicht verfügbar", "fr": "Indisponible", "ja": "利用不可", "ko": "사용 불가"},

    # ---- Calibration (EN-sourced) ----
    "Calibration": {"de": "Kalibrierung", "fr": "Étalonnage", "ja": "キャリブレーション", "ko": "캘리브레이션"},
    "Calibration Center": {"de": "Kalibrierungscenter", "fr": "Centre d'étalonnage", "ja": "キャリブレーションセンター", "ko": "캘리브레이션 센터"},
    "Calibration Complete": {"de": "Kalibrierung abgeschlossen", "fr": "Étalonnage terminé", "ja": "キャリブレーション完了", "ko": "캘리브레이션 완료"},
    "Calibration Steps": {"de": "Kalibrierungsschritte", "fr": "Étapes d'étalonnage", "ja": "キャリブレーション手順", "ko": "캘리브레이션 단계"},
    "Calibration finished successfully. Results have been saved.": {
        "de": "Kalibrierung erfolgreich abgeschlossen. Ergebnisse wurden gespeichert.",
        "fr": "Étalonnage terminé avec succès. Les résultats ont été enregistrés.",
        "ja": "キャリブレーションが正常に終了しました。結果は保存されました。",
        "ko": "캘리브레이션이 성공적으로 완료되었습니다. 결과가 저장되었습니다."},
    "Slice Calibration": {"de": "Slice-Kalibrierung", "fr": "Étalonnage du tranchage", "ja": "スライスキャリブレーション", "ko": "슬라이스 캘리브레이션"},
    "SLICE CALIBRATION": {"de": "SLICE-KALIBRIERUNG", "fr": "ÉTALONNAGE DU TRANCHAGE", "ja": "スライスキャリブレーション", "ko": "슬라이스 캘리브레이션"},
    "Hardware Calibration": {"de": "Hardware-Kalibrierung", "fr": "Étalonnage matériel", "ja": "ハードウェアキャリブレーション", "ko": "하드웨어 캘리브레이션"},
    "Hardware calibration pending": {"de": "Hardware-Kalibrierung ausstehend", "fr": "Étalonnage matériel en attente", "ja": "ハードウェアキャリブレーション保留中", "ko": "하드웨어 캘리브레이션 대기 중"},
    "Flow Dynamics": {"de": "Flussdynamik", "fr": "Dynamique de flux", "ja": "フロー ダイナミクス", "ko": "유동 역학"},
    "Flow Rate": {"de": "Flussrate", "fr": "Débit", "ja": "フローレート", "ko": "유량"},
    "Temp Tower": {"de": "Temperaturturm", "fr": "Tour de température", "ja": "温度タワー", "ko": "온도 타워"},
    "Start Calibration": {"de": "Kalibrierung starten", "fr": "Démarrer l'étalonnage", "ja": "キャリブレーションを開始", "ko": "캘리브레이션 시작"},
    "Recalibrate": {"de": "Erneut kalibrieren", "fr": "Réétalonner", "ja": "再キャリブレーション", "ko": "다시 캘리브레이션"},
    "Cancel Calibration": {"de": "Kalibrierung abbrechen", "fr": "Annuler l'étalonnage", "ja": "キャリブレーションをキャンセル", "ko": "캘리브레이션 취소"},
    "Select a calibration type from the left panel": {
        "de": "Wählen Sie links einen Kalibrierungstyp",
        "fr": "Sélectionnez un type d'étalonnage dans le panneau de gauche",
        "ja": "左のパネルからキャリブレーションの種類を選択してください",
        "ko": "왼쪽 패널에서 캘리브레이션 유형을 선택하세요"},
    "校准范围": {"de": "Kalibrierungsbereich", "fr": "Plage d'étalonnage", "ja": "キャリブレーション範囲", "ko": "캘리브레이션 범위"},
    "避免校准区域": {"de": "Kalibrierungsbereich vermeiden", "fr": "Éviter la zone d'étalonnage", "ja": "キャリブレーション領域を避ける", "ko": "캘리브레이션 영역 피기"},

    # ---- Step / progress / completion ----
    "In Progress": {"de": "In Arbeit", "fr": "En cours", "ja": "進行中", "ko": "진행 중"},
    "Completed": {"de": "Abgeschlossen", "fr": "Terminé", "ja": "完了", "ko": "완료됨"},
    "Step %1: %2": {"de": "Schritt %1: %2", "fr": "Étape %1 : %2", "ja": "ステップ %1: %2", "ko": "단계 %1: %2"},
    "Step 1: %1": {"de": "Schritt 1: %1", "fr": "Étape 1 : %1", "ja": "ステップ 1: %1", "ko": "단계 1: %1"},
    "第 %1/%2 步": {"de": "Schritt %1/%2", "fr": "Étape %1/%2", "ja": "ステップ %1/%2", "ko": "단계 %1/%2"},
    "行 %1 / %2": {"de": "Zeile %1 / %2", "fr": "Ligne %1 / %2", "ja": "行 %1 / %2", "ko": "행 %1 / %2"},
    "行 -- / --": {"de": "Zeile -- / --", "fr": "Ligne -- / --", "ja": "行 -- / --", "ko": "행 -- / --"},
    "剩余 %1": {"de": "Verbleibend %1", "fr": "Restant %1", "ja": "残り %1", "ko": "남은 %1"},
    "T%1 — %2": {"de": "T%1 — %2", "fr": "T%1 — %2", "ja": "T%1 — %2", "ko": "T%1 — %2"},

    # ---- Views / gizmo (EN-sourced) ----
    "Front view": {"de": "Vorderansicht", "fr": "Vue de face", "ja": "正面図", "ko": "정면 보기"},
    "Right view": {"de": "Seitenansicht", "fr": "Vue de droite", "ja": "右側面図", "ko": "우측 보기"},
    "Top view": {"de": "Draufsicht", "fr": "Vue de dessus", "ja": "上面図", "ko": "상단 보기"},
    "Isometric view": {"de": "Isometrische Ansicht", "fr": "Vue isométrique", "ja": "等角投影図", "ko": "등각 투영"},
    "Fit view": {"de": "Ansicht anpassen", "fr": "Ajuster la vue", "ja": "表示に合わせる", "ko": "보기 맞춤"},
    "Fit preview": {"de": "Vorschau anpassen", "fr": "Ajuster l'aperçu", "ja": "プレビューに合わせる", "ko": "미리보기 맞춤"},
    "装配视图": {"de": "Assembly-Ansicht", "fr": "Vue d'assemblage", "ja": "アセンブリビュー", "ko": "어셈블리 보기"},
    "切换到装配视图": {"de": "Zur Assembly-Ansicht wechseln", "fr": "Basculer vers la vue d'assemblage", "ja": "アセンブリビューに切り替え", "ko": "어셈블리 보기로 전환"},
    "装配": {"de": "Assembly", "fr": "Assemblage", "ja": "アセンブリ", "ko": "어셈블리"},
    "Mesh boolean": {"de": "Mesh-Boolesche Operation", "fr": "Opération booléenne de maillage", "ja": "メッシュブール演算", "ko": "메시 불 연산"},
    "布尔运算": {"de": "Boolesche Operation", "fr": "Opération booléenne", "ja": "ブール演算", "ko": "불 연산"},
    "执行运算": {"de": "Operation ausführen", "fr": "Exécuter l'opération", "ja": "演算を実行", "ko": "연산 실행"},
    "检测与 Z 轴平行的平面": {"de": "Ebenen parallel zur Z-Achse erkennen", "fr": "Détecter les plans parallèles à l'axe Z", "ja": "Z軸に平行な平面を検出", "ko": "Z축에 평행한 평면 감지"},
    "悬停特征: 圆": {"de": "Merkmal: Kreis", "fr": "Caractéristique : cercle", "ja": "特徴: 円", "ko": "특징: 원"},
    "悬停特征: 平面": {"de": "Merkmal: Ebene", "fr": "Caractéristique : plan", "ja": "特徴: 平面", "ko": "특징: 평면"},
    "悬停特征: 点": {"de": "Merkmal: Punkt", "fr": "Caractéristique : point", "ja": "特徴: 点", "ko": "특징: 점"},
    "悬停特征: 边": {"de": "Merkmal: Kante", "fr": "Caractéristique : arête", "ja": "特徴: 辺", "ko": "특징: 모서리"},
    "候选面: ": {"de": "Kandidatenebene: ", "fr": "Face candidate : ", "ja": "候補面: ", "ko": "후보 면: "},
    "点击网格面拾取特征 (点/边/圆/平面)": {
        "de": "Auf Mesh-Fläche klicken, um Merkmal zu wählen (Punkt/Kante/Kreis/Ebene)",
        "fr": "Cliquez sur une face du maillage pour choisir une caractéristique (point/arête/cercle/plan)",
        "ja": "メッシュ面をクリックして特徴を取得 (点/辺/円/平面)",
        "ko": "메시 면을 클릭하여 특징을 선택 (점/모서리/원/평면)"},
    "点测量": {"de": "Punktmessung", "fr": "Mesure de point", "ja": "点測定", "ko": "점 측정"},
    "特征测量": {"de": "Merkmalsmessung", "fr": "Mesure de caractéristique", "ja": "特徴測定", "ko": "특징 측정"},
    "测量": {"de": "Messen", "fr": "Mesurer", "ja": "測定", "ko": "측정"},
    "对比": {"de": "Vergleich", "fr": "Comparer", "ja": "比較", "ko": "비교"},
    "模式对比": {"de": "Modusvergleich", "fr": "Comparaison de mode", "ja": "モード比較", "ko": "모드 비교"},

    # ---- Object operations (EN-sourced) ----
    "Arrange all objects": {"de": "Alle Objekte anordnen", "fr": "Disposer tous les objets", "ja": "すべてのオブジェクトを配置", "ko": "모든 객체 정렬"},
    "Auto orient": {"de": "Automatisch ausrichten", "fr": "Orientation automatique", "ja": "自動向き設定", "ko": "자동 방향"},
    "Center selected objects": {"de": "Ausgewählte Objekte zentrieren", "fr": "Centrer les objets sélectionnés", "ja": "選択したオブジェクトを中央に配置", "ko": "선택한 객체 가운데 정렬"},
    "Copy selected objects": {"de": "Ausgewählte Objekte kopieren", "fr": "Copier les objets sélectionnés", "ja": "選択したオブジェクトをコピー", "ko": "선택한 객체 복사"},
    "Paste objects": {"de": "Objekte einfügen", "fr": "Coller les objets", "ja": "オブジェクトを貼り付け", "ko": "객체 붙여넣기"},
    "Duplicate selected objects": {"de": "Ausgewählte Objekte duplizieren", "fr": "Dupliquer les objets sélectionnés", "ja": "選択したオブジェクトを複製", "ko": "선택한 객체 복제"},
    "Delete selected objects": {"de": "Ausgewählte Objekte löschen", "fr": "Supprimer les objets sélectionnés", "ja": "選択したオブジェクトを削除", "ko": "선택한 객체 삭제"},
    "Mirror selected objects on X": {"de": "Ausgewählte Objekte an X spiegeln", "fr": "Miroir des objets sélectionnés sur X", "ja": "選択したオブジェクトをX軸でミラー", "ko": "선택한 객체 X축 미러"},
    "Mirror selected objects on Y": {"de": "Ausgewählte Objekte an Y spiegeln", "fr": "Miroir des objets sélectionnés sur Y", "ja": "選択したオブジェクトをY軸でミラー", "ko": "선택한 객체 Y축 미러"},
    "Mirror selected objects on Z": {"de": "Ausgewählte Objekte an Z spiegeln", "fr": "Miroir des objets sélectionnés sur Z", "ja": "選択したオブジェクトをZ軸でミラー", "ko": "선택한 객체 Z축 미러"},
    "Repair selected mesh": {"de": "Ausgewähltes Mesh reparieren", "fr": "Réparer le maillage sélectionné", "ja": "選択したメッシュを修復", "ko": "선택한 메시 수리"},
    "Simplify mesh": {"de": "Mesh vereinfachen", "fr": "Simplifier le maillage", "ja": "メッシュを簡略化", "ko": "메시 단순화"},
    "Split object": {"de": "Objekt teilen", "fr": "Diviser l'objet", "ja": "オブジェクトを分割", "ko": "객체 분할"},
    "Select an object or volume": {"de": "Ein Objekt oder Volumen auswählen", "fr": "Sélectionner un objet ou un volume", "ja": "オブジェクトまたはボリュームを選択", "ko": "객체 또는 볼륨 선택"},
    "Select one or more objects": {"de": "Ein oder mehrere Objekte auswählen", "fr": "Sélectionner un ou plusieurs objets", "ja": "1つ以上のオブジェクトを選択", "ko": "하나 이상의 객체 선택"},
    "Object settings": {"de": "Objekteinstellungen", "fr": "Paramètres de l'objet", "ja": "オブジェクト設定", "ko": "객체 설정"},
    "Load a model before arranging": {"de": "Vor dem Anordnen ein Modell laden", "fr": "Charger un modèle avant de disposer", "ja": "配置前にモデルを読み込んでください", "ko": "정렬 전에 모델을 로드하세요"},
    "Maximum plate count reached": {"de": "Maximale Plattenanzahl erreicht", "fr": "Nombre maximal de platines atteint", "ja": "プレート数の上限に達しました", "ko": "플레이트 최대 개수에 도달했습니다"},

    # ---- Plates (EN-sourced) ----
    "Add plate": {"de": "Druckplatte hinzufügen", "fr": "Ajouter une platine", "ja": "プレートを追加", "ko": "플레이트 추가"},
    "Add model": {"de": "Modell hinzufügen", "fr": "Ajouter un modèle", "ja": "モデルを追加", "ko": "모델 추가"},

    # ---- Import / Export (EN-sourced) ----
    "Import 3MF": {"de": "3MF importieren", "fr": "Importer 3MF", "ja": "3MFをインポート", "ko": "3MF 가져오기"},
    "Import STL": {"de": "STL importieren", "fr": "Importer STL", "ja": "STLをインポート", "ko": "STL 가져오기"},
    "Import OBJ": {"de": "OBJ importieren", "fr": "Importer OBJ", "ja": "OBJをインポート", "ko": "OBJ 가져오기"},
    "Import STEP": {"de": "STEP importieren", "fr": "Importer STEP", "ja": "STEPをインポート", "ko": "STEP 가져오기"},
    "Import AMF": {"de": "AMF importieren", "fr": "Importer AMF", "ja": "AMFをインポート", "ko": "AMF 가져오기"},
    "Export 3MF": {"de": "3MF exportieren", "fr": "Exporter 3MF", "ja": "3MFをエクスポート", "ko": "3MF 내보내기"},
    "Export Model": {"de": "Modell exportieren", "fr": "Exporter le modèle", "ja": "モデルをエクスポート", "ko": "모델 내보내기"},
    "Export G-code": {"de": "G-code exportieren", "fr": "Exporter le G-code", "ja": "G-codeをエクスポート", "ko": "G-code 내보내기"},
    "Export All Plate G-code": {"de": "Alle Druckplatten-G-code exportieren", "fr": "Exporter le G-code de toutes les platines", "ja": "全プレートのG-codeをエクスポート", "ko": "모든 플레이트 G-code 내보내기"},
    "Save Project": {"de": "Projekt speichern", "fr": "Enregistrer le projet", "ja": "プロジェクトを保存", "ko": "프로젝트 저장"},

    # ---- File dialogs / format filters (EN-sourced) ----
    "3MF (*.3mf)": {"de": "3MF (*.3mf)", "fr": "3MF (*.3mf)", "ja": "3MF (*.3mf)", "ko": "3MF (*.3mf)"},
    "STL (*.stl)": {"de": "STL (*.stl)", "fr": "STL (*.stl)", "ja": "STL (*.stl)", "ko": "STL (*.stl)"},
    "OBJ (*.obj)": {"de": "OBJ (*.obj)", "fr": "OBJ (*.obj)", "ja": "OBJ (*.obj)", "ko": "OBJ (*.obj)"},
    "G-code: %1": {"de": "G-code: %1", "fr": "G-code : %1", "ja": "G-code: %1", "ko": "G-code: %1"},
    "G-code": {"de": "G-code", "fr": "G-code", "ja": "G-code", "ko": "G-code"},
    "Custom G-code": {"de": "Benutzerdefinierter G-code", "fr": "G-code personnalisé", "ja": "カスタムG-code", "ko": "사용자 지정 G-code"},
    "; Enter custom G-code here": {"de": "; Hier benutzerdefinierten G-code eingeben", "fr": "; Saisir le G-code personnalisé ici", "ja": "; ここにカスタムG-codeを入力", "ko": "; 여기에 사용자 지정 G-code 입력"},
    "Edit G-code at layer %1:": {"de": "G-code bei Schicht %1 bearbeiten:", "fr": "Modifier le G-code à la couche %1 :", "ja": "層 %1 のG-codeを編集:", "ko": "레이어 %1의 G-code 편집:"},
    "Enter G-code to insert at layer %1:": {"de": "G-code zum Einfügen bei Schicht %1 eingeben:", "fr": "Saisir le G-code à insérer à la couche %1 :", "ja": "層 %1 に挿入するG-codeを入力:", "ko": "레이어 %1에 삽입할 G-code 입력:"},
    "Edit Custom G-code": {"de": "Benutzerdefinierten G-code bearbeiten", "fr": "Modifier le G-code personnalisé", "ja": "カスタムG-codeを編集", "ko": "사용자 지정 G-code 편집"},
    "Add Custom G-code...": {"de": "Benutzerdefinierten G-code hinzufügen...", "fr": "Ajouter un G-code personnalisé...", "ja": "カスタムG-codeを追加...", "ko": "사용자 지정 G-code 추가..."},
    "Delete Custom G-code": {"de": "Benutzerdefinierten G-code löschen", "fr": "Supprimer le G-code personnalisé", "ja": "カスタムG-codeを削除", "ko": "사용자 지정 G-code 삭제"},
    "Add Color Change": {"de": "Farbwechsel hinzufügen", "fr": "Ajouter un changement de couleur", "ja": "色変更を追加", "ko": "색상 변경 추가"},
    "Delete Color Change": {"de": "Farbwechsel löschen", "fr": "Supprimer le changement de couleur", "ja": "色変更を削除", "ko": "색상 변경 삭제"},
    "Add Pause": {"de": "Pause hinzufügen", "fr": "Ajouter une pause", "ja": "一時停止を追加", "ko": "일시정지 추가"},
    "Delete Pause": {"de": "Pause löschen", "fr": "Supprimer la pause", "ja": "一時停止を削除", "ko": "일시정지 삭제"},
    "Delete Filament Change": {"de": "Filamentwechsel löschen", "fr": "Supprimer le changement de filament", "ja": "フィラメント交換を削除", "ko": "필라멘트 교체 삭제"},
    "Delete Custom Template": {"de": "Benutzerdefinierte Vorlage löschen", "fr": "Supprimer le modèle personnalisé", "ja": "カスタムテンプレートを削除", "ko": "사용자 지정 템플릿 삭제"},
    "Add Template": {"de": "Vorlage hinzufügen", "fr": "Ajouter un modèle", "ja": "テンプレートを追加", "ko": "템플릿 추가"},
    "Reset Parameters": {"de": "Parameter zurücksetzen", "fr": "Réinitialiser les paramètres", "ja": "パラメータをリセット", "ko": "매개변수 초기화"},

    # ---- About / version / update (mixed) ----
    "About": {"de": "Über", "fr": "À propos", "ja": "バージョン情報", "ko": "정보"},
    "Check for Updates": {"de": "Nach Updates suchen", "fr": "Rechercher des mises à jour", "ja": "アップデートを確認", "ko": "업데이트 확인"},
    "Documentation": {"de": "Dokumentation", "fr": "Documentation", "ja": "ドキュメント", "ko": "문서"},
    "Shortcut Overview": {"de": "Tastenkürzel-Übersicht", "fr": "Aperçu des raccourcis", "ja": "ショートカット一覧", "ko": "단축키 개요"},
    "新版本可用: %1": {"de": "Neue Version verfügbar: %1", "fr": "Nouvelle version disponible : %1", "ja": "新しいバージョンがあります: %1", "ko": "새 버전 사용 가능: %1"},
    "正在升级...": {"de": "Upgrade läuft...", "fr": "Mise à niveau en cours...", "ja": "アップグレード中...", "ko": "업그레이드 중..."},
    "更新": {"de": "Aktualisieren", "fr": "Mettre à jour", "ja": "更新", "ko": "업데이트"},
    "更新通道": {"de": "Update-Kanal", "fr": "Canal de mise à jour", "ja": "アップデートチャネル", "ko": "업데이트 채널"},
    "下载": {"de": "Herunterladen", "fr": "Télécharger", "ja": "ダウンロード", "ko": "다운로드"},
    "可下载": {"de": "Herunterladbar", "fr": "Téléchargeable", "ja": "ダウンロード可能", "ko": "다운로드 가능"},
    "当前版本：2.4.0-dev (Qt6 Edition)": {"de": "Aktuelle Version: 2.4.0-dev (Qt6 Edition)", "fr": "Version actuelle : 2.4.0-dev (Qt6 Edition)", "ja": "現在のバージョン: 2.4.0-dev (Qt6 Edition)", "ko": "현재 버전: 2.4.0-dev (Qt6 Edition)"},
    "版本 2.4.0-dev  (Qt6 QML)": {"de": "Version 2.4.0-dev  (Qt6 QML)", "fr": "Version 2.4.0-dev  (Qt6 QML)", "ja": "バージョン 2.4.0-dev  (Qt6 QML)", "ko": "버전 2.4.0-dev  (Qt6 QML)"},
    "版本 2.4.0-dev  |  Qt 6.10  |  OWzx": {"de": "Version 2.4.0-dev  |  Qt 6.10  |  OWzx", "fr": "Version 2.4.0-dev  |  Qt 6.10  |  OWzx", "ja": "バージョン 2.4.0-dev  |  Qt 6.10  |  OWzx", "ko": "버전 2.4.0-dev  |  Qt 6.10  |  OWzx"},
    "Qt 6.10 + QML 重写迁移版": {"de": "Qt 6.10 + QML-Neufassung (Migration)", "fr": "Qt 6.10 + QML réécriture (migration)", "ja": "Qt 6.10 + QML 書き換え版 (移行)", "ko": "Qt 6.10 + QML 재작성 (마이그레이션)"},
    "Qt 版本": {"de": "Qt-Version", "fr": "Version de Qt", "ja": "Qtバージョン", "ko": "Qt 버전"},
    "基于 OrcaSlicer 开源版本": {"de": "Basiert auf der Open-Source-Version von OrcaSlicer", "fr": "Basé sur la version open-source d'OrcaSlicer", "ja": "OrcaSlicer オープンソース版をベースにしています", "ko": "OrcaSlicer 오픈소스 버전 기반"},
    "上游基线: 0d4ac73a6f3224a2bf753d7b9e67d7d515bc8557": {"de": "Upstream-Baseline: 0d4ac73a6f3224a2bf753d7b9e67d7d515bc8557", "fr": "Base amont : 0d4ac73a6f3224a2bf753d7b9e67d7d515bc8557", "ja": "アップストリームベースライン: 0d4ac73a6f3224a2bf753d7b9e67d7d515bc8557", "ko": "업스트림 베이스라인: 0d4ac73a6f3224a2bf753d7b9e67d7d515bc8557"},
    "上游基线：OrcaSlicer main branch": {"de": "Upstream-Baseline: OrcaSlicer main branch", "fr": "Base amont : OrcaSlicer main branch", "ja": "アップストリームベースライン: OrcaSlicer main branch", "ko": "업스트림 베이스라인: OrcaSlicer main branch"},
    "固件版本: 1.0.0": {"de": "Firmware-Version: 1.0.0", "fr": "Version du firmware : 1.0.0", "ja": "ファームウェアバージョン: 1.0.0", "ko": "펌웨어 버전: 1.0.0"},
    "关于 OWzx": {"de": "Über OWzx", "fr": "À propos d'OWzx", "ja": "OWzx について", "ko": "OWzx 정보"},
    "关于": {"de": "Über", "fr": "À propos", "ja": "情報", "ko": "정보"},
    "官方网站": {"de": "Offizielle Website", "fr": "Site officiel", "ja": "公式ウェブサイト", "ko": "공식 웹사이트"},
    "开源协议": {"de": "Open-Source-Lizenz", "fr": "Licence open-source", "ja": "オープンソースライセンス", "ko": "오픈소스 라이선스"},
    "登录 OWzx 账号": {"de": "Bei OWzx-Konto anmelden", "fr": "Se connecter au compte OWzx", "ja": "OWzx アカウントにログイン", "ko": "OWzx 계정 로그인"},
    "登录账号": {"de": "Bei Konto anmelden", "fr": "Se connecter au compte", "ja": "アカウントにログイン", "ko": "계정 로그인"},
    "账号与隐私": {"de": "Konto und Datenschutz", "fr": "Compte et confidentialité", "ja": "アカウントとプライバシー", "ko": "계정 및 개인정보"},
    "语言": {"de": "Sprache", "fr": "Langue", "ja": "言語", "ko": "언어"},
    "简体中文": {"de": "Vereinfachtes Chinesisch", "fr": "Chinois simplifié", "ja": "簡体字中国語", "ko": "간체 중국어"},
    "界面主题": {"de": "Oberflächenthema", "fr": "Thème de l'interface", "ja": "インターフェーステーマ", "ko": "인터페이스 테마"},
    "外观": {"de": "Darstellung", "fr": "Apparence", "ja": "外観", "ko": "외관"},
    "启动时显示主页": {"de": "Startseite beim Start anzeigen", "fr": "Afficher la page d'accueil au démarrage", "ja": "起動時にホームを表示", "ko": "시작 시 홈 표시"},
    "启动时检查更新": {"de": "Beim Start nach Updates suchen", "fr": "Vérifier les mises à jour au démarrage", "ja": "起動時にアップデートを確認", "ko": "시작 시 업데이트 확인"},
    "快捷键": {"de": "Tastenkürzel", "fr": "Raccourcis clavier", "ja": "ショートカットキー", "ko": "단축키"},
    "快捷键一览": {"de": "Tastenkürzel-Übersicht", "fr": "Aperçu des raccourcis", "ja": "ショートカット一覧", "ko": "단축키 개요"},
    "快速入口": {"de": "Schnellzugriff", "fr": "Accès rapide", "ja": "クイックアクセス", "ko": "빠른 실행"},
    "每日提示": {"de": "Tipp des Tages", "fr": "Astuce du jour", "ja": "今日のヒント", "ko": "오늘의 팁"},
    "你知道吗": {"de": "Wussten Sie schon?", "fr": "Le saviez-vous ?", "ja": "ご存知ですか?", "ko": "알고 계셨나요?"},

    # ---- Backend / network / connection diagnostics (mixed) ----
    "Backend unavailable": {"de": "Backend nicht verfügbar", "fr": "Backend indisponible", "ja": "バックエンド利用不可", "ko": "백엔드 사용 불가"},
    "DNS 解析": {"de": "DNS-Auflösung", "fr": "Résolution DNS", "ja": "DNS解決", "ko": "DNS 확인"},
    "检查 IP 地址": {"de": "IP-Adresse prüfen", "fr": "Vérifier l'adresse IP", "ja": "IPアドレスを確認", "ko": "IP 주소 확인"},
    "检查访问码": {"de": "Zugriffscode prüfen", "fr": "Vérifier le code d'accès", "ja": "アクセスコードを確認", "ko": "액세스 코드 확인"},
    "局域网访问码": {"de": "LAN-Zugriffscode", "fr": "Code d'accès LAN", "ja": "LANアクセスコード", "ko": "LAN 액세스 코드"},
    "局域网发现 (SSDP)": {"de": "LAN-Erkennung (SSDP)", "fr": "Découverte LAN (SSDP)", "ja": "LAN検出 (SSDP)", "ko": "LAN 검색 (SSDP)"},
    "MQTT 端口": {"de": "MQTT-Port", "fr": "Port MQTT", "ja": "MQTTポート", "ko": "MQTT 포트"},
    "确保防火墙未阻止 MQTT(8883)/lan 通信": {
        "de": "Sicherstellen, dass die Firewall MQTT(8883)/LAN-Kommunikation nicht blockiert",
        "fr": "Assurez-vous que le pare-feu ne bloque pas la communication MQTT(8883)/LAN",
        "ja": "ファイアウォールが MQTT(8883)/lan 通信をブロックしていないことを確認",
        "ko": "방화벽이 MQTT(8883)/lan 통신을 차단하지 않는지 확인"},
    "网络通信插件": {"de": "Netzwerkkommunikations-Plugin", "fr": "Plugin de communication réseau", "ja": "ネットワーク通信プラグイン", "ko": "네트워크 통신 플러그인"},
    "云端连通性": {"de": "Cloud-Konnektivität", "fr": "Connectivité cloud", "ja": "クラウド接続性", "ko": "클라우드 연결"},
    "同步中...": {"de": "Synchronisierung...", "fr": "Synchronisation...", "ja": "同期中...", "ko": "동기화 중..."},
    "从插件市场浏览更多插件": {"de": "Weitere Plugins im Marketplace durchsuchen", "fr": "Parcourir plus de plugins sur le marché", "ja": "マーケットプレースでさらにプラグインを参照", "ko": "마켓플레이스에서 더 많은 플러그인 탐색"},
    "API Key": {"de": "API-Schlüssel", "fr": "Clé API", "ja": "APIキー", "ko": "API 키"},
    "URL:": {"de": "URL:", "fr": "URL :", "ja": "URL:", "ko": "URL:"},
    "Name:": {"de": "Name:", "fr": "Nom :", "ja": "名前:", "ko": "이름:"},

    # ---- AMS / filament group ----
    "Slot %1": {"de": "Slot %1", "fr": "Emplacement %1", "ja": "スロット %1", "ko": "슬롯 %1"},
    "Slot %1:": {"de": "Slot %1:", "fr": "Emplacement %1 :", "ja": "スロット %1:", "ko": "슬롯 %1:"},
    "Extruder %1": {"de": "Extruder %1", "fr": "Extrudeur %1", "ja": "エクストルーダー %1", "ko": "익스트루더 %1"},
    "Filament Group": {"de": "Filamentgruppe", "fr": "Groupe de filaments", "ja": "フィラメントグループ", "ko": "필라멘트 그룹"},
    "MMU 分段": {"de": "MMU-Segmentierung", "fr": "Segmentation MMU", "ja": "MMUセグメンテーション", "ko": "MMU 세분화"},
    "Current filament may be incompatible with printer": {
        "de": "Das aktuelle Filament ist möglicherweise nicht mit dem Drucker kompatibel",
        "fr": "Le filament actuel peut être incompatible avec l'imprimante",
        "ja": "現在のフィラメントはプリンターと互換性がない可能性があります",
        "ko": "현재 필라멘트가 프린터와 호환되지 않을 수 있습니다"},
    "耗材": {"de": "Filament", "fr": "Filament", "ja": "フィラメント", "ko": "필라멘트"},
    "个文件": {"de": "Dateien", "fr": "fichiers", "ja": "ファイル", "ko": "파일"},

    # ---- Slicer params (ZH-sourced) ----
    "层高": {"de": "Schichthöhe", "fr": "Hauteur de couche", "ja": "層の高さ", "ko": "레이어 높이"},
    "线宽": {"de": "Linienbreite", "fr": "Largeur de ligne", "ja": "線幅", "ko": "선 너비"},
    "线宽 ": {"de": "Linienbreite ", "fr": "Largeur de ligne ", "ja": "線幅 ", "ko": "선 너비 "},
    "线宽倍率": {"de": "Linienbreiten-Faktor", "fr": "Facteur de largeur de ligne", "ja": "線幅倍率", "ko": "선 너비 비율"},
    "壁线圈数": {"de": "Wandschleifen", "fr": "Boucles de paroi", "ja": "壁ループ数", "ko": "벽 루프 수"},
    "Wall loops": {"de": "Wandschleifen", "fr": "Boucles de paroi", "ja": "壁ループ数", "ko": "벽 루프 수"},
    "顶层层数": {"de": "Oberschichten", "fr": "Couches supérieures", "ja": "上部層数", "ko": "상단 레이어 수"},
    "Top shell layers": {"de": "Obere Hüllenschichten", "fr": "Couches de coque supérieures", "ja": "上部シェル層数", "ko": "상단 쉘 레이어 수"},
    "Bottom shell layers": {"de": "Untere Hüllenschichten", "fr": "Couches de coque inférieures", "ja": "下部シェル層数", "ko": "하단 쉘 레이어 수"},
    "底层层数": {"de": "Unterschichten", "fr": "Couches inférieures", "ja": "下部層数", "ko": "하단 레이어 수"},
    "Top layer": {"de": "Oberschicht", "fr": "Couche supérieure", "ja": "最上層", "ko": "상단 레이어"},
    "First layer": {"de": "Erste Schicht", "fr": "Première couche", "ja": "最初の層", "ko": "첫 레이어"},
    "Initial layer height": {"de": "Höhe der ersten Schicht", "fr": "Hauteur de la première couche", "ja": "最初の層の高さ", "ko": "첫 레이어 높이"},
    "Initial layer line width": {"de": "Linienbreite der ersten Schicht", "fr": "Largeur de ligne de la première couche", "ja": "最初の層の線幅", "ko": "첫 레이어 선 너비"},
    "Initial layer speed": {"de": "Geschwindigkeit der ersten Schicht", "fr": "Vitesse de la première couche", "ja": "最初の層の速度", "ko": "첫 레이어 속도"},
    "首层线宽": {"de": "Linienbreite der ersten Schicht", "fr": "Largeur de ligne de la première couche", "ja": "最初の層の線幅", "ko": "첫 레이어 선 너비"},
    "首层速度 (mm/s)": {"de": "Geschwindigkeit der ersten Schicht (mm/s)", "fr": "Vitesse de la première couche (mm/s)", "ja": "最初の層の速度 (mm/s)", "ko": "첫 레이어 속도 (mm/s)"},
    "Layer height": {"de": "Schichthöhe", "fr": "Hauteur de couche", "ja": "層の高さ", "ko": "레이어 높이"},
    "Line width": {"de": "Linienbreite", "fr": "Largeur de ligne", "ja": "線幅", "ko": "선 너비"},
    "Sparse infill density": {"de": "Füllungsdichte", "fr": "Densité de remplissage", "ja": "インフィル密度", "ko": "채움 밀도"},
    "Sparse infill pattern": {"de": "Füllungsmuster", "fr": "Motif de remplissage", "ja": "インフィルパターン", "ko": "채움 패턴"},
    "Sparse infill speed": {"de": "Füllungsgeschwindigkeit", "fr": "Vitesse de remplissage", "ja": "インフィル速度", "ko": "채움 속도"},
    "Support density": {"de": "Stützstrukturdichte", "fr": "Densité du support", "ja": "サポート密度", "ko": "서포트 밀도"},
    "Support type": {"de": "Stützstrukturtyp", "fr": "Type de support", "ja": "サポートタイプ", "ko": "서포트 유형"},
    "Enable support": {"de": "Stützstruktur aktivieren", "fr": "Activer le support", "ja": "サポートを有効化", "ko": "서포트 활성화"},
    "Support painting": {"de": "Stützstruktur malen", "fr": "Peinture de support", "ja": "サポートペイント", "ko": "서포트 페인팅"},
    "Seam painting": {"de": "Naht malen", "fr": "Peinture de couture", "ja": "シームペイント", "ko": "심 페인팅"},
    "Inner wall speed": {"de": "Innenwand-Geschwindigkeit", "fr": "Vitesse de paroi interne", "ja": "内壁速度", "ko": "내벽 속도"},
    "Outer wall speed": {"de": "Außenwand-Geschwindigkeit", "fr": "Vitesse de paroi externe", "ja": "外壁速度", "ko": "외벽 속도"},
    "Travel speed": {"de": "Eilganggeschwindigkeit", "fr": "Vitesse de déplacement", "ja": "移動速度", "ko": "이동 속도"},
    "Top surface speed": {"de": "Oberflächengeschwindigkeit", "fr": "Vitesse de surface supérieure", "ja": "上面速度", "ko": "상면 속도"},
    "Pressure Advance": {"de": "Pressure Advance", "fr": "Pressure Advance", "ja": "Pressure Advance", "ko": "Pressure Advance"},
    "Nozzle Diameter": {"de": "Düsendurchmesser", "fr": "Diamètre de buse", "ja": "ノズル径", "ko": "노즐 지름"},
    "Brim": {"de": "Brim", "fr": "Bordure", "ja": "ブリム", "ko": "브림"},
    "Brim 宽度 (mm)": {"de": "Brim-Breite (mm)", "fr": "Largeur du brim (mm)", "ja": "ブリム幅 (mm)", "ko": "브림 너비 (mm)"},
    "Brim 的宽度。": {"de": "Breite des Brims.", "fr": "Largeur du brim.", "ja": "ブリムの幅。", "ko": "브림의 너비."},
    "回退距离 (mm)": {"de": "Rückzugsabstand (mm)", "fr": "Distance de rétraction (mm)", "ja": "リトラクション距離 (mm)", "ko": "리트랙션 거리 (mm)"},
    "回退距离（毫米）。": {"de": "Rückzugsabstand (Millimeter).", "fr": "Distance de rétraction (millimètres).", "ja": "リトラクション距離（ミリメートル）。", "ko": "리트랙션 거리(밀리미터)."},
    "回退速度 (mm/s)": {"de": "Rückzugsgeschwindigkeit (mm/s)", "fr": "Vitesse de rétraction (mm/s)", "ja": "リトラクション速度 (mm/s)", "ko": "리트랙션 속도 (mm/s)"},
    "回退速度（毫米/秒）。": {"de": "Rückzugsgeschwindigkeit (Millimeter/Sekunde).", "fr": "Vitesse de rétraction (millimètres/seconde).", "ja": "リトラクション速度（ミリメートル/秒）。", "ko": "리트랙션 속도(밀리미터/초)."},
    "换料回退": {"de": "Filamentwechsel-Rückzug", "fr": "Rétraction de changement de filament", "ja": "フィラメント交換リトラクション", "ko": "필라멘트 교체 리트랙션"},
    "换料回退 (mm)": {"de": "Filamentwechsel-Rückzug (mm)", "fr": "Rétraction de changement de filament (mm)", "ja": "フィラメント交換リトラクション (mm)", "ko": "필라멘트 교체 리트랙션 (mm)"},
    "换层回退": {"de": "Schichtwechsel-Rückzug", "fr": "Rétraction de changement de couche", "ja": "層切替リトラクション", "ko": "레이어 변경 리트랙션"},
    "擦拭": {"de": "Wischen", "fr": "Essuyage", "ja": "ワイプ", "ko": "와이프"},
    "擦拭距离 (mm)": {"de": "Wischabstand (mm)", "fr": "Distance d'essuyage (mm)", "ja": "ワイプ距離 (mm)", "ko": "와이프 거리 (mm)"},
    "擦洗倍率": {"de": "Wischfaktor", "fr": "Facteur d'essuyage", "ja": "ワイプ倍率", "ko": "와이프 비율"},
    "Z 抬升 (mm)": {"de": "Z-Anhebung (mm)", "fr": "Élévation Z (mm)", "ja": "Zリフト (mm)", "ko": "Z 리프트 (mm)"},
    "Z 缝": {"de": "Z-Naht", "fr": "Couture Z", "ja": "Zシーム", "ko": "Z 심"},
    "Z 缝策略": {"de": "Z-Naht-Strategie", "fr": "Stratégie de couture Z", "ja": "Zシーム戦略", "ko": "Z 심 전략"},
    "Z 缝角落": {"de": "Z-Naht-Ecke", "fr": "Coin de couture Z", "ja": "Zシームコーナー", "ko": "Z 심 모서리"},
    "强制缝线": {"de": "Naht erzwingen", "fr": "Forcer la couture", "ja": "シームを強制", "ko": "심 강제"},
    "阻止缝线": {"de": "Naht blockieren", "fr": "Bloquer la couture", "ja": "シームをブロック", "ko": "심 차단"},
    "减速层时间 (s)": {"de": "Schichtzeit reduzieren (s)", "fr": "Temps de couche réduit (s)", "ja": "レイヤー時間減速 (s)", "ko": "레이어 시간 감속 (s)"},
    "悬空风扇 (%)": {"de": "Überhangslüfter (%)", "fr": "Ventilateur de surplomb (%)", "ja": "オーバーハングファン (%)", "ko": "현수 팬 (%)"},
    "最小风扇 (%)": {"de": "Minimallüfter (%)", "fr": "Ventilateur minimal (%)", "ja": "最小ファン (%)", "ko": "최소 팬 (%)"},
    "风扇 ": {"de": "Lüfter ", "fr": "Ventilateur ", "ja": "ファン ", "ko": "팬 "},
    "风扇延迟 (层)": {"de": "Lüfter-Verzögerung (Schichten)", "fr": "Délai du ventilateur (couches)", "ja": "ファン遅延 (層)", "ko": "팬 지연 (레이어)"},
    "风扇速度 (%)": {"de": "Lüftergeschwindigkeit (%)", "fr": "Vitesse du ventilateur (%)", "ja": "ファン速度 (%)", "ko": "팬 속도 (%)"},
    "熨烫速度 (%)": {"de": "Glättungsgeschwindigkeit (%)", "fr": "Vitesse de lissage (%)", "ja": "アイロン速度 (%)", "ko": "다림질 속도 (%)"},
    "腔体温度 (°C)": {"de": "Kammertemperatur (°C)", "fr": "Température de la chambre (°C)", "ja": "チャンバー温度 (°C)", "ko": "챔버 온도 (°C)"},
    "预热时间 (s)": {"de": "Vorwärmzeit (s)", "fr": "Temps de préchauffage (s)", "ja": "予熱時間 (s)", "ko": "예열 시간 (s)"},
    "逐层降温": {"de": "Schichtweises Abkühlen", "fr": "Refroidissement par couche", "ja": "層ごとに冷却", "ko": "레이어별 냉각"},
    "降温策略": {"de": "Abkühlungsstrategie", "fr": "Stratégie de refroidissement", "ja": "冷却戦略", "ko": "냉각 전략"},
    "冷却中...": {"de": "Kühlen...", "fr": "Refroidissement...", "ja": "冷却中...", "ko": "냉각 중..."},
    "加热中...": {"de": "Aufheizen...", "fr": "Chauffe...", "ja": "加熱中...", "ko": "가열 중..."},
    "外壁线宽": {"de": "Außenwand-Linienbreite", "fr": "Largeur de ligne de paroi externe", "ja": "外壁線幅", "ko": "외벽 선 너비"},
    "外壁优先": {"de": "Außenwand zuerst", "fr": "Paroi externe d'abord", "ja": "外壁優先", "ko": "외벽 우선"},
    "内壁线宽": {"de": "Innenwand-Linienbreite", "fr": "Largeur de ligne de paroi interne", "ja": "内壁線幅", "ko": "내벽 선 너비"},
    "内壁优先": {"de": "Innenwand zuerst", "fr": "Paroi interne d'abord", "ja": "内壁優先", "ko": "내벽 우선"},
    "内壁速度 (mm/s)": {"de": "Innenwand-Geschwindigkeit (mm/s)", "fr": "Vitesse de paroi interne (mm/s)", "ja": "内壁速度 (mm/s)", "ko": "내벽 속도 (mm/s)"},
    "内壁加速度 (mm/s²)": {"de": "Innenwand-Beschleunigung (mm/s²)", "fr": "Accélération de paroi interne (mm/s²)", "ja": "内壁加速度 (mm/s²)", "ko": "내벽 가속도 (mm/s²)"},
    "外壁速度 (mm/s)": {"de": "Außenwand-Geschwindigkeit (mm/s)", "fr": "Vitesse de paroi externe (mm/s)", "ja": "外壁速度 (mm/s)", "ko": "외벽 속도 (mm/s)"},
    "外壁加速度 (mm/s²)": {"de": "Außenwand-Beschleunigung (mm/s²)", "fr": "Accélération de paroi externe (mm/s²)", "ja": "外壁加速度 (mm/s²)", "ko": "외벽 가속도 (mm/s²)"},
    "壁加速度": {"de": "Wandbeschleunigung", "fr": "Accélération des parois", "ja": "壁加速度", "ko": "벽 가속도"},
    "空走加速度": {"de": "Eilgangbeschleunigung", "fr": "Accélération de déplacement", "ja": "移動加速度", "ko": "이동 가속도"},
    "空走加速度 (mm/s²)": {"de": "Eilgangbeschleunigung (mm/s²)", "fr": "Accélération de déplacement (mm/s²)", "ja": "移動加速度 (mm/s²)", "ko": "이동 가속도 (mm/s²)"},
    "空走速度 (mm/s)": {"de": "Eilganggeschwindigkeit (mm/s)", "fr": "Vitesse de déplacement (mm/s)", "ja": "移動速度 (mm/s)", "ko": "이동 속도 (mm/s)"},
    "回填速度 (mm/s)": {"de": "Nachfüllgeschwindigkeit (mm/s)", "fr": "Vitesse de remplissage (mm/s)", "ja": "充填速度 (mm/s)", "ko": "채움 속도 (mm/s)"},
    "加速度 ": {"de": "Beschleunigung ", "fr": "Accélération ", "ja": "加速度 ", "ko": "가속도 "},
    "特殊速度": {"de": "Sondergeschwindigkeit", "fr": "Vitesse spéciale", "ja": "特殊速度", "ko": "특수 속도"},
    "当前层 Z 高度 (mm)": {"de": "Z-Höhe der aktuellen Schicht (mm)", "fr": "Hauteur Z de la couche actuelle (mm)", "ja": "現在の層のZ高さ (mm)", "ko": "현재 레이어 Z 높이 (mm)"},

    # ---- Quality presets (mixed) ----
    "0.12mm 超精细": {"de": "0.12mm Ultrafein", "fr": "0.12mm Ultra-fin", "ja": "0.12mm 超高精度", "ko": "0.12mm 초고정밀"},
    "0.16mm 精细": {"de": "0.16mm Fein", "fr": "0.16mm Fin", "ja": "0.16mm 高精度", "ko": "0.16mm 고정밀"},
    "0.20mm 标准 @Creality K1C": {"de": "0.20mm Standard @Creality K1C", "fr": "0.20mm Standard @Creality K1C", "ja": "0.20mm 標準 @Creality K1C", "ko": "0.20mm 표준 @Creality K1C"},
    "0.24mm 草稿": {"de": "0.24mm Entwurf", "fr": "0.24mm Brouillon", "ja": "0.24mm ドラフト", "ko": "0.24mm 드래프트"},
    "0.28mm 超草稿": {"de": "0.28mm Ultra-Entwurf", "fr": "0.28mm Ultra-brouillon", "ja": "0.28mm 超ドラフト", "ko": "0.28mm 초드래프트"},
    "K 值": {"de": "K-Wert", "fr": "Valeur K", "ja": "K値", "ko": "K값"},
    "N 值": {"de": "N-Wert", "fr": "Valeur N", "ja": "N値", "ko": "N값"},
    "K值: %1": {"de": "K-Wert: %1", "fr": "Valeur K : %1", "ja": "K値: %1", "ko": "K값: %1"},
    "步进倍率": {"de": "Schritt-Faktor", "fr": "Facteur de pas", "ja": "ステップ倍率", "ko": "스텝 비율"},
    "步长": {"de": "Schrittweite", "fr": "Pas", "ja": "ステップ", "ko": "스텝"},

    # ---- Cut / connectors / drill (ZH-sourced) ----
    "切割": {"de": "Schneiden", "fr": "Couper", "ja": "切断", "ko": "자르기"},
    "切割轴": {"de": "Schnittachse", "fr": "Axe de coupe", "ja": "切断軸", "ko": "자르기 축"},
    "平面切割": {"de": "Ebener Schnitt", "fr": "Coupe plane", "ja": "平面切断", "ko": "평면 자르기"},
    "执行切割": {"de": "Schnitt ausführen", "fr": "Exécuter la coupe", "ja": "切断を実行", "ko": "자르기 실행"},
    "执行检测": {"de": "Erkennung ausführen", "fr": "Exécuter la détection", "ja": "検出を実行", "ko": "감지 실행"},
    "执行钻孔": {"de": "Bohren ausführen", "fr": "Exécuter le perçage", "ja": "穴あけを実行", "ko": "드릴 실행"},
    "Advanced cut": {"de": "Erweiterter Schnitt", "fr": "Coupe avancée", "ja": "詳細切断", "ko": "고급 자르기"},
    "平放": {"de": "Flach hinlegen", "fr": "Mettre à plat", "ja": "平面に配置", "ko": "평평하게 놓기"},
    "平行平台": {"de": "Parallel zur Plattform", "fr": "Parallèle au plateau", "ja": "プラットフォームに平行", "ko": "플랫폼에 평행"},
    "保留上半": {"de": "Obere Hälfte behalten", "fr": "Garder la moitié supérieure", "ja": "上半分を保持", "ko": "상단 절반 유지"},
    "保留下半": {"de": "Untere Hälfte behalten", "fr": "Garder la moitié inférieure", "ja": "下半分を保持", "ko": "하단 절반 유지"},
    "保留两侧": {"de": "Beide Seiten behalten", "fr": "Garder les deux côtés", "ja": "両側を保持", "ko": "양쪽 유지"},
    "仅上半部": {"de": "Nur obere Hälfte", "fr": "Moitié supérieure uniquement", "ja": "上半分のみ", "ko": "상단 절반만"},
    "舌槽模式": {"de": "Zungen-Nut-Modus", "fr": "Mode languette-rainure", "ja": "ほぞ-ほぞ穴モード", "ko": "턱-홈 모드"},
    "爆炸比例": {"de": "Explosionsverhältnis", "fr": "Ratio d'explosion", "ja": "展開比率", "ko": "분해 비율"},
    "闭合距离:": {"de": "Schließabstand:", "fr": "Distance de fermeture :", "ja": "閉鎖距離:", "ko": "닫힘 거리:"},
    "角度阈值": {"de": "Winkelschwelle", "fr": "Seuil d'angle", "ja": "角度しきい値", "ko": "각도 임계값"},
    "最锐角": {"de": "Spitzester Winkel", "fr": "Angle le plus aigu", "ja": "最も鋭い角", "ko": "가장 예각"},
    "指定": {"de": "Festlegen", "fr": "Spécifier", "ja": "指定", "ko": "지정"},
    "强制": {"de": "Erzwingen", "fr": "Forcer", "ja": "強制", "ko": "강제"},
    "阻止": {"de": "Blockieren", "fr": "Bloquer", "ja": "ブロック", "ko": "차단"},
    "任意": {"de": "Beliebig", "fr": "Quelconque", "ja": "任意", "ko": "임의"},
    "圆盘": {"de": "Scheibe", "fr": "Disque", "ja": "円盤", "ko": "원판"},
    "圆环体": {"de": "Torus", "fr": "Tore", "ja": "トーラス", "ko": "토러스"},
    "截锥体": {"de": "Kegelstumpf", "fr": "Tronc de cône", "ja": "円錐台", "ko": "절두체"},
    "陀螺": {"de": "Gyroskop", "fr": "Gyroscope", "ja": "ジャイロ", "ko": "자이로"},
    "蜂巢": {"de": "Wabe", "fr": "Nid d'abeille", "ja": "ハニカム", "ko": "허니콤"},
    "树状": {"de": "Baumartig", "fr": "Arborescent", "ja": "ツリー", "ko": "트리"},
    "三角": {"de": "Dreieck", "fr": "Triangle", "ja": "三角", "ko": "삼각"},
    "直线": {"de": "Gerade", "fr": "Ligne droite", "ja": "直線", "ko": "직선"},
    "曲线投影": {"de": "Kurvenprojektion", "fr": "Projection de courbe", "ja": "曲線投影", "ko": "곡선 투영"},
    "沿曲面进行投影（实验性，完整效果将在后续版本提供）": {
        "de": "Projektion entlang der gekrümmten Fläche (experimentell, volle Funktion in späteren Versionen)",
        "fr": "Projection le long de la surface courbe (expérimental, fonction complète dans les versions ultérieures)",
        "ja": "曲面に沿って投影（実験的、完全な効果は今後のバージョンで提供）",
        "ko": "곡면을 따라 투영 (실험적, 전체 효과는 향후 버전에서 제공)"},

    # ---- Hollow / drill params ----
    "最小体积": {"de": "Mindestvolumen", "fr": "Volume minimal", "ja": "最小体積", "ko": "최소 부피"},
    "排水孔半径:": {"de": "Drainageloch-Radius:", "fr": "Rayon du trou de drainage :", "ja": "排水穴半径:", "ko": "배수 구멍 반지름:"},
    "排水孔深度:": {"de": "Drainageloch-Tiefe:", "fr": "Profondeur du trou de drainage :", "ja": "排水穴深さ:", "ko": "배수 구멍 깊이:"},
    "钻孔": {"de": "Bohren", "fr": "Perçage", "ja": "穴あけ", "ko": "드릴"},
    "钻孔半径:": {"de": "Bohr-Radius:", "fr": "Rayon de perçage :", "ja": "穴あけ半径:", "ko": "드릴 반지름:"},
    "钻孔高度:": {"de": "Bohr-Höhe:", "fr": "Hauteur de perçage :", "ja": "穴あけ高さ:", "ko": "드릴 높이:"},

    # ---- Layer range controls ----
    "Move layer range down": {"de": "Schichtbereich nach unten", "fr": "Déplacer la plage de couches vers le bas", "ja": "層範囲を下へ移動", "ko": "레이어 범위 아래로"},
    "Move layer range up": {"de": "Schichtbereich nach oben", "fr": "Déplacer la plage de couches vers le haut", "ja": "層範囲を上へ移動", "ko": "레이어 범위 위로"},
    "层范围 ±1 层": {"de": "Schichtbereich ±1 Schicht", "fr": "Plage de couches ±1 couche", "ja": "層範囲 ±1 層", "ko": "레이어 범위 ±1 레이어"},
    "层范围 ±10 层": {"de": "Schichtbereich ±10 Schichten", "fr": "Plage de couches ±10 couches", "ja": "層範囲 ±10 層", "ko": "레이어 범위 ±10 레이어"},
    "层": {"de": "Schicht", "fr": "Couche", "ja": "層", "ko": "레이어"},
    "层 ": {"de": "Schicht ", "fr": "Couche ", "ja": "層 ", "ko": "레이어 "},
    "层 %1": {"de": "Schicht %1", "fr": "Couche %1", "ja": "層 %1", "ko": "레이어 %1"},
    "层耗时 ": {"de": "Schichtzeit ", "fr": "Temps par couche ", "ja": "層時間 ", "ko": "레이어 시간 "},

    # ---- Date/time format tokens (keep structure, localize labels where helpful) ----
    "公制 (mm)": {"de": "Metrisch (mm)", "fr": "Métrique (mm)", "ja": "メートル法 (mm)", "ko": "미터법 (mm)"},
    "英制 (inch)": {"de": "Imperial (Zoll)", "fr": "Impérial (pouce)", "ja": "ヤード・ポンド法 (インチ)", "ko": "야드파운드법 (인치)"},
    "单位": {"de": "Einheit", "fr": "Unité", "ja": "単位", "ko": "단위"},
    "年份 (YYYY)": {"de": "Jahr (YYYY)", "fr": "Année (YYYY)", "ja": "年 (YYYY)", "ko": "연도 (YYYY)"},
    "月份 (MM)": {"de": "Monat (MM)", "fr": "Mois (MM)", "ja": "月 (MM)", "ko": "월 (MM)"},
    "日期 (DD)": {"de": "Tag (DD)", "fr": "Jour (DD)", "ja": "日 (DD)", "ko": "일 (DD)"},
    "小时 (HH)": {"de": "Stunde (HH)", "fr": "Heure (HH)", "ja": "時間 (HH)", "ko": "시간 (HH)"},
    "分钟": {"de": "Minute", "fr": "Minute", "ja": "分", "ko": "분"},
    "分钟 (mm)": {"de": "Minute (mm)", "fr": "Minute (mm)", "ja": "分 (mm)", "ko": "분 (mm)"},
    "秒 (ss)": {"de": "Sekunde (ss)", "fr": "Seconde (ss)", "ja": "秒 (ss)", "ko": "초 (ss)"},
    "当前日期 (YYYYMMDD)": {"de": "Aktuelles Datum (YYYYMMDD)", "fr": "Date actuelle (YYYYMMDD)", "ja": "現在の日付 (YYYYMMDD)", "ko": "현재 날짜 (YYYYMMDD)"},
    "当前时间 (HHmmss)": {"de": "Aktuelle Zeit (HHmmss)", "fr": "Heure actuelle (HHmmss)", "ja": "現在の時刻 (HHmmss)", "ko": "현재 시간 (HHmmss)"},
    "当前时间戳 (YYYYMMDD-HHmmss)": {"de": "Aktueller Zeitstempel (YYYYMMDD-HHmmss)", "fr": "Horodatage actuel (YYYYMMDD-HHmmss)", "ja": "現在のタイムスタンプ (YYYYMMDD-HHmmss)", "ko": "현재 타임스탬프 (YYYYMMDD-HHmmss)"},
    "时间戳": {"de": "Zeitstempel", "fr": "Horodatage", "ja": "タイムスタンプ", "ko": "타임스탬프"},

    # ---- Misc UI labels (ZH-sourced) ----
    "个": {"de": "Stück", "fr": "unités", "ja": "個", "ko": "개"},
    "全选": {"de": "Alle auswählen", "fr": "Tout sélectionner", "ja": "すべて選択", "ko": "모두 선택"},
    "克隆": {"de": "Duplizieren", "fr": "Cloner", "ja": "複製", "ko": "복제"},
    "克隆选中 (备)": {"de": "Auswahl duplizieren (Backup)", "fr": "Cloner la sélection (backup)", "ja": "選択を複製 (バックアップ)", "ko": "선택 복제 (백업)"},
    "剪切选中": {"de": "Auswahl ausschneiden", "fr": "Couper la sélection", "ja": "選択を切り取り", "ko": "선택 잘라내기"},
    "创建": {"de": "Erstellen", "fr": "Créer", "ja": "作成", "ko": "만들기"},
    "新增": {"de": "Hinzufügen", "fr": "Ajouter", "ja": "追加", "ko": "추가"},
    "添加": {"de": "Hinzufügen", "fr": "Ajouter", "ja": "追加", "ko": "추가"},
    "删除": {"de": "Löschen", "fr": "Supprimer", "ja": "削除", "ko": "삭제"},
    "结束": {"de": "Beenden", "fr": "Fin", "ja": "終了", "ko": "종료"},
    "起始": {"de": "Start", "fr": "Début", "ja": "開始", "ko": "시작"},
    "起 mm": {"de": "Start mm", "fr": "Début mm", "ja": "開始 mm", "ko": "시작 mm"},
    "止 mm": {"de": "Ende mm", "fr": "Fin mm", "ja": "終了 mm", "ko": "종료 mm"},
    "槽": {"de": "Slot", "fr": "Emplacement", "ja": "スロット", "ko": "슬롯"},
    "执行": {"de": "Ausführen", "fr": "Exécuter", "ja": "実行", "ko": "실행"},
    "每": {"de": "Pro", "fr": "Par", "ja": "毎", "ko": "매"},
    "页": {"de": "Seite", "fr": "Page", "ja": "ページ", "ko": "페이지"},
    "页面": {"de": "Seite", "fr": "Page", "ja": "ページ", "ko": "페이지"},
    "总实例数": {"de": "Gesamtinstanzen", "fr": "Total d'instances", "ja": "総インスタンス数", "ko": "전체 인스턴스 수"},
    "范围：": {"de": "Bereich:", "fr": "Plage :", "ja": "範囲:", "ko": "범위:"},
    "键名": {"de": "Schlüsselname", "fr": "Nom de clé", "ja": "キー名", "ko": "키 이름"},
    "属性": {"de": "Eigenschaft", "fr": "Propriété", "ja": "プロパティ", "ko": "속성"},
    "继承自：": {"de": "Geerbt von:", "fr": "Hérité de :", "ja": "継承元:", "ko": "상속됨:"},
    "文件": {"de": "Datei", "fr": "Fichier", "ja": "ファイル", "ko": "파일"},
    "文件路径": {"de": "Dateipfad", "fr": "Chemin du fichier", "ja": "ファイルパス", "ko": "파일 경로"},
    "输出文件": {"de": "Ausgabedatei", "fr": "Fichier de sortie", "ja": "出力ファイル", "ko": "출력 파일"},
    "输出路径": {"de": "Ausgabepfad", "fr": "Chemin de sortie", "ja": "出力パス", "ko": "출력 경로"},
    "质量:": {"de": "Qualität:", "fr": "Qualité :", "ja": "品質:", "ko": "품질:"},
    "厚度:": {"de": "Dicke:", "fr": "Épaisseur :", "ja": "厚さ:", "ko": "두께:"},
    "宽度": {"de": "Breite", "fr": "Largeur", "ja": "幅", "ko": "너비"},
    "高度": {"de": "Höhe", "fr": "Hauteur", "ja": "高さ", "ko": "높이"},
    "粗细": {"de": "Stärke", "fr": "Épaisseur", "ja": "太さ", "ko": "굵기"},
    "字号": {"de": "Schriftgröße", "fr": "Taille de police", "ja": "フォントサイズ", "ko": "글꼴 크기"},
    "字体大小": {"de": "Schriftgröße", "fr": "Taille de police", "ja": "フォントサイズ", "ko": "글꼴 크기"},
    "字体": {"de": "Schriftart", "fr": "Police", "ja": "フォント", "ko": "글꼴"},
    "文字": {"de": "Text", "fr": "Texte", "ja": "テキスト", "ko": "텍스트"},
    "斜体": {"de": "Kursiv", "fr": "Italique", "ja": "斜体", "ko": "이탤릭"},
    "编辑 SVG": {"de": "SVG bearbeiten", "fr": "Modifier le SVG", "ja": "SVGを編集", "ko": "SVG 편집"},
    "编辑文字": {"de": "Text bearbeiten", "fr": "Modifier le texte", "ja": "テキストを編集", "ko": "텍스트 편집"},
    "SVG emboss": {"de": "SVG-Prägung", "fr": "Embossage SVG", "ja": "SVGエンボス", "ko": "SVG 엠보싱"},
    "网格": {"de": "Raster", "fr": "Grille", "ja": "グリッド", "ko": "그리드"},
    "贴附表面": {"de": "Oberfläche anbringen", "fr": "Fixer à la surface", "ja": "表面に貼付", "ko": "표면 부착"},
    "显示/隐藏": {"de": "Anzeigen/Verbergen", "fr": "Afficher/Masquer", "ja": "表示/非表示", "ko": "표시/숨기기"},
    "清除分段": {"de": "Segmentierung löschen", "fr": "Effacer la segmentation", "ja": "セグメントをクリア", "ko": "세그먼트 지우기"},
    "普通": {"de": "Normal", "fr": "Normal", "ja": "通常", "ko": "일반"},
    "便利": {"de": "Praktisch", "fr": "Pratique", "ja": "便利", "ko": "편리"},
    "交替": {"de": "Alternierend", "fr": "Alterné", "ja": "交互", "ko": "교대"},
    "全圈": {"de": "Vollkreis", "fr": "Cercle complet", "ja": "全周", "ko": "전체 원"},
    "内圈": {"de": "Innenkreis", "fr": "Cercle intérieur", "ja": "内周", "ko": "내부 원"},
    "外圈": {"de": "Außenkreis", "fr": "Cercle extérieur", "ja": "外周", "ko": "외부 원"},
    "内角": {"de": "Innenwinkel", "fr": "Angle intérieur", "ja": "内角", "ko": "내각"},
    "外角": {"de": "Außenwinkel", "fr": "Angle extérieur", "ja": "外角", "ko": "외각"},
    "极暗": {"de": "Sehr dunkel", "fr": "Très sombre", "ja": "非常に暗い", "ko": "매우 어두움"},
    "深蓝": {"de": "Dunkelblau", "fr": "Bleu foncé", "ja": "濃紺", "ko": "진한 파랑"},
    "就绪": {"de": "Bereit", "fr": "Prêt", "ja": "準備完了", "ko": "준비됨"},
    "待实现": {"de": "Noch nicht implementiert", "fr": "Pas encore implémenté", "ja": "未実装", "ko": "미구현"},
    "目标平台": {"de": "Zielplattform", "fr": "Plateforme cible", "ja": "ターゲットプラットフォーム", "ko": "대상 플랫폼"},
    "异步执行": {"de": "Asynchrone Ausführung", "fr": "Exécution asynchrone", "ja": "非同期実行", "ko": "비동기 실행"},
    "QML 引擎": {"de": "QML-Engine", "fr": "Moteur QML", "ja": "QMLエンジン", "ko": "QML 엔진"},
    "OpenGL 调试上下文": {"de": "OpenGL-Debug-Kontext", "fr": "Contexte de débogage OpenGL", "ja": "OpenGLデバッグコンテキスト", "ko": "OpenGL 디버그 컨텍스트"},
    "最大日志大小 (MB)": {"de": "Maximale Loggröße (MB)", "fr": "Taille maximale du journal (MB)", "ja": "最大ログサイズ (MB)", "ko": "최대 로그 크기 (MB)"},
    "垂直屏幕": {"de": "Hochformat", "fr": "Écran vertical", "ja": "縦画面", "ko": "세로 화면"},
    "AI 监控": {"de": "AI-Überwachung", "fr": "Surveillance IA", "ja": "AI監視", "ko": "AI 모니터링"},
    "SLA 空洞标记": {"de": "SLA-Hohlraummarkierung", "fr": "Marquage de cavité SLA", "ja": "SLA空洞マーキング", "ko": "SLA 공동 표시"},
    "详细 G-code": {"de": "Detaillierter G-code", "fr": "G-code détaillé", "ja": "詳細 G-code", "ko": "상세 G-code"},
    "帮助": {"de": "Hilfe", "fr": "Aide", "ja": "ヘルプ", "ko": "도움말"},
    "首页": {"de": "Startseite", "fr": "Accueil", "ja": "ホーム", "ko": "홈"},
    "首页": {"de": "Startseite", "fr": "Accueil", "ja": "ホーム", "ko": "홈"},
    "退出": {"de": "Beenden", "fr": "Quitter", "ja": "終了", "ko": "종료"},
    "保存": {"de": "Speichern", "fr": "Enregistrer", "ja": "保存", "ko": "저장"},
    "确定": {"de": "OK", "fr": "OK", "ja": "OK", "ko": "확인"},
    "取消": {"de": "Abbrechen", "fr": "Annuler", "ja": "キャンセル", "ko": "취소"},
    "关闭": {"de": "Schließen", "fr": "Fermer", "ja": "閉じる", "ko": "닫기"},
    "打开": {"de": "Öffnen", "fr": "Ouvrir", "ja": "開く", "ko": "열기"},
    "打印": {"de": "Drucken", "fr": "Imprimer", "ja": "プリント", "ko": "출력"},
    "打印机": {"de": "Drucker", "fr": "Imprimante", "ja": "プリンター", "ko": "프린터"},
    "工艺": {"de": "Prozess", "fr": "Processus", "ja": "プロセス", "ko": "공정"},
    "材料": {"de": "Material", "fr": "Matériau", "ja": "材料", "ko": "재료"},
    "例如：K1 Max": {"de": "z. B.: K1 Max", "fr": "ex. : K1 Max", "ja": "例: K1 Max", "ko": "예: K1 Max"},
    "材质": {"de": "Material", "fr": "Matériau", "ja": "材質", "ko": "재질"},
    "缩放": {"de": "Skalieren", "fr": "Mettre à l'échelle", "ja": "スケール", "ko": "크기 조정"},
    "旋转": {"de": "Rotation", "fr": "Rotation", "ja": "回転", "ko": "회전"},
    "移动": {"de": "Bewegen", "fr": "Déplacer", "ja": "移動", "ko": "이동"},
    "排列": {"de": "Anordnen", "fr": "Disposer", "ja": "配置", "ko": "정렬"},
    "缩放": {"de": "Skalieren", "fr": "Mettre à l'échelle", "ja": "スケール", "ko": "크기 조정"},
    "预览": {"de": "Vorschau", "fr": "Aperçu", "ja": "プレビュー", "ko": "미리보기"},
    "跳到开头": {"de": "Zum Anfang springen", "fr": "Aller au début", "ja": "先頭へジャンプ", "ko": "처음으로 이동"},
    "跳到结尾": {"de": "Zum Ende springen", "fr": "Aller à la fin", "ja": "末尾へジャンプ", "ko": "끝으로 이동"},
    "跳转前100步": {"de": "100 Schritte zurück", "fr": "Sauter 100 pas en arrière", "ja": "100ステップ戻る", "ko": "100스텝 뒤로"},
    "跳转后100步": {"de": "100 Schritte vor", "fr": "Sauter 100 pas en avant", "ja": "100ステップ進む", "ko": "100스텝 앞으로"},
    "预估时间:": {"de": "Geschätzte Zeit:", "fr": "Temps estimé :", "ja": "推定時間:", "ko": "예상 시간:"},
    "无数据": {"de": "Keine Daten", "fr": "Aucune donnée", "ja": "データなし", "ko": "데이터 없음"},
    "No data": {"de": "Keine Daten", "fr": "Aucune donnée", "ja": "データなし", "ko": "데이터 없음"},
    "No matching options": {"de": "Keine passenden Optionen", "fr": "Aucune option correspondante", "ja": "一致するオプションなし", "ko": "일치하는 옵션 없음"},
    "No options": {"de": "Keine Optionen", "fr": "Aucune option", "ja": "オプションなし", "ko": "옵션 없음"},
    "No project": {"de": "Kein Projekt", "fr": "Aucun projet", "ja": "プロジェクトなし", "ko": "프로젝트 없음"},
    "Clipboard is empty": {"de": "Zwischenablage ist leer", "fr": "Le presse-papiers est vide", "ja": "クリップボードが空です", "ko": "클립보드가 비어 있습니다"},
    "A preset with this name already exists. Choose another name.": {
        "de": "Eine Voreinstellung mit diesem Namen existiert bereits. Wählen Sie einen anderen Namen.",
        "fr": "Un préréglage avec ce nom existe déjà. Choisissez un autre nom.",
        "ja": "この名前のプリセットは既に存在します。別の名前を選択してください。",
        "ko": "이 이름의 사전 설정이 이미 존재합니다. 다른 이름을 선택하세요."},
    "Option Groups": {"de": "Optionsgruppen", "fr": "Groupes d'options", "ja": "オプショングループ", "ko": "옵션 그룹"},

    # ---- Preview area / viewport status (EN-sourced) ----
    "Preview area - connect printer to see real data": {
        "de": "Vorschaubereich - Drucker verbinden, um echte Daten zu sehen",
        "fr": "Zone d'aperçu - connectez l'imprimante pour voir les données réelles",
        "ja": "プレビュー領域 - プリンターを接続して実際のデータを表示",
        "ko": "미리보기 영역 - 실제 데이터를 보려면 프린터 연결"},
    "Software viewport - %1 object(s)": {"de": "Software-Viewport - %1 Objekt(e)", "fr": "Viewport logiciel - %1 objet(s)", "ja": "ソフトウェアビューポート - %1 オブジェクト", "ko": "소프트웨어 뷰포트 - %1 객체"},
    "Software viewport - drag to rotate, wheel to zoom": {
        "de": "Software-Viewport - ziehen zum Drehen, Rad zum Zoomen",
        "fr": "Viewport logiciel - glisser pour pivoter, molette pour zoomer",
        "ja": "ソフトウェアビューポート - ドラッグで回転、ホイールでズーム",
        "ko": "소프트웨어 뷰포트 - 드래그로 회전, 휠로 확대"},

    # ---- Mesh repair / analysis status (EN-sourced) ----
    " | %1 errors repaired": {"de": " | %1 Fehler repariert", "fr": " | %1 erreurs réparées", "ja": " | %1 件のエラーを修復", "ko": " | %1개 오류 수리됨"},
    " | %1 non-manifold edges": {"de": " | %1 nicht-mannigfaltige Kanten", "fr": " | %1 arêtes non-manifold", "ja": " | %1 非多様体エッジ", "ko": " | %1 비다양체 모서리"},
    "%1 difference(s)": {"de": "%1 Unterschied(e)", "fr": "%1 différence(s)", "ja": "%1 件の差分", "ko": "%1개 차이"},
    "%1 – %2 mm": {"de": "%1 – %2 mm", "fr": "%1 – %2 mm", "ja": "%1 – %2 mm", "ko": "%1 – %2 mm"},

    # ---- Printer/bed model names (keep, just pass-through) ----
    "Creality K1C 0.4 nozzle": {"de": "Creality K1C 0.4 nozzle", "fr": "Creality K1C 0.4 nozzle", "ja": "Creality K1C 0.4 nozzle", "ko": "Creality K1C 0.4 nozzle"},
    "Creality K2 Plus": {"de": "Creality K2 Plus", "fr": "Creality K2 Plus", "ja": "Creality K2 Plus", "ko": "Creality K2 Plus"},
    "K1 Max 0.4 nozzle": {"de": "K1 Max 0.4 nozzle", "fr": "K1 Max 0.4 nozzle", "ja": "K1 Max 0.4 nozzle", "ko": "K1 Max 0.4 nozzle"},
    "CR-10 Smart Pro 0.4 nozzle": {"de": "CR-10 Smart Pro 0.4 nozzle", "fr": "CR-10 Smart Pro 0.4 nozzle", "ja": "CR-10 Smart Pro 0.4 nozzle", "ko": "CR-10 Smart Pro 0.4 nozzle"},
    "Ender-3 S1 0.4 nozzle": {"de": "Ender-3 S1 0.4 nozzle", "fr": "Ender-3 S1 0.4 nozzle", "ja": "Ender-3 S1 0.4 nozzle", "ko": "Ender-3 S1 0.4 nozzle"},

    # ---- File-size / info chips (keep verbatim) ----
    "12.5 MB": {"de": "12.5 MB", "fr": "12.5 MB", "ja": "12.5 MB", "ko": "12.5 MB"},
    "45.2 MB": {"de": "45.2 MB", "fr": "45.2 MB", "ja": "45.2 MB", "ko": "45.2 MB"},
    "8.3 MB": {"de": "8.3 MB", "fr": "8.3 MB", "ja": "8.3 MB", "ko": "8.3 MB"},

    # ---- Material description (ZH) ----
    "PETG 兼具强度和韧性，透明度可选，适合功能性零件。": {
        "de": "PETG bietet sowohl Festigkeit als auch Zähigkeit, mit optionaler Transparenz, ideal für funktionale Teile.",
        "fr": "Le PETG allie solidité et ténacité, avec une transparence en option, idéal pour les pièces fonctionnelles.",
        "ja": "PETGは強度と靭性を兼ね備え、透明度も選択可能で、機能部品に適しています。",
        "ko": "PETG는 강도와 인성을 겸비하고 투명도 선택이 가능하여 기능 부품에 적합합니다."},

    # ---- GBK-mojibake source (encoded in source code; map to correct meaning) ----
    # 瀵煎嚭澶辫触 is GBK-misrendered "导出失败" = "Export failed"
    "瀵煎嚭澶辫触": {"de": "Export fehlgeschlagen", "fr": "Échec de l'exportation", "ja": "エクスポート失敗", "ko": "내보내기 실패"},

    # ---- Tail cleanup: small set of stragglers seen after first pass ----
    "随机": {"de": "Zufällig", "fr": "Aléatoire", "ja": "ランダム", "ko": "무작위"},
    "Place on face": {"de": "Auf Fläche legen", "fr": "Placer sur une face", "ja": "面に配置", "ko": "면에 배치"},
    "打开已有 3MF/STL 文件": {"de": "Vorhandene 3MF/STL-Datei öffnen", "fr": "Ouvrir un fichier 3MF/STL existant", "ja": "既存の3MF/STLファイルを開く", "ko": "기존 3MF/STL 파일 열기"},
    "已选 %1 项": {"de": "%1 ausgewählt", "fr": "%1 sélectionné(s)", "ja": "%1 件選択", "ko": "%1개 선택됨"},
    " 个": {"de": " Stück", "fr": " unités", "ja": " 個", "ko": "개"},
    "✓ 已实时生效": {"de": "✓ Live aktiv", "fr": "✓ Actif en direct", "ja": "✓ リアルタイム適用済み", "ko": "✓ 실시간 적용됨"},
    "开启": {"de": "Aktivieren", "fr": "Activer", "ja": "有効化", "ko": "활성화"},
    "已强制: %1": {"de": "Erzwungen: %1", "fr": "Forcé : %1", "ja": "強制済み: %1", "ko": "강제됨: %1"},
    "已阻止: %1": {"de": "Blockiert: %1", "fr": "Bloqué : %1", "ja": "ブロック済み: %1", "ko": "차단됨: %1"},
    " 层": {"de": " Schichten", "fr": " couches", "ja": " 層", "ko": " 레이어"},
    "已修改 %1 个参数": {"de": "%1 Parameter geändert", "fr": "%1 paramètre(s) modifié(s)", "ja": "%1 個のパラメータを変更", "ko": "%1개 매개변수 수정됨"},
    "Z 缝位置策略。控制层之间的可见接缝放在哪里。": {
        "de": "Z-Naht-Positionierungsstrategie. Steuert, wo die sichtbare Naht zwischen den Schichten platziert wird.",
        "fr": "Stratégie de positionnement de la couture Z. Contrôle l'emplacement de la couture visible entre les couches.",
        "ja": "Zシーム配置戦略。層間の目視可能なシームの配置を制御します。",
        "ko": "Z 심 배치 전략. 레이어 간 보이는 심의 위치를 제어합니다."},
    "SD 卡文件管理": {"de": "SD-Karten-Dateiverwaltung", "fr": "Gestion des fichiers carte SD", "ja": "SDカードファイル管理", "ko": "SD 카드 파일 관리"},
    "停止录像": {"de": "Aufnahme stoppen", "fr": "Arrêter l'enregistrement", "ja": "録画を停止", "ko": "녹화 중지"},
    "停止延时": {"de": "Zeitraffer stoppen", "fr": "Arrêter le timelapse", "ja": "タイムラプスを停止", "ko": "타임랩스 중지"},
    "%1 条未读": {"de": "%1 ungelesen", "fr": "%1 non lu(s)", "ja": "%1 件未読", "ko": "%1개 읽지 않음"},
    "● 未保存": {"de": "● Nicht gespeichert", "fr": "● Non enregistré", "ja": "● 未保存", "ko": "● 저장 안 됨"},
}


def get_lang_dict(lang):
    out = {}
    for src, langs in LONGTAIL.items():
        if lang in langs:
            out[src] = langs[lang]
    return out


def translate_file(lang, dry_run=False):
    ts_path = os.path.join(I18N, '%s.ts' % lang)
    if not os.path.isfile(ts_path):
        print('ERROR: %s not found' % ts_path, file=sys.stderr)
        return 2

    with open(ts_path, 'r', encoding='utf-8', newline='') as f:
        content = f.read()

    unfinished_before = content.count('type="unfinished"')
    print('[%s] Unfinished before: %d' % (lang, unfinished_before))

    translations = get_lang_dict(lang)
    print('[%s] Dictionary size: %d' % (lang, len(translations)))

    # SAME proven pattern as translate_core_i18n.py.
    pattern = re.compile(
        r'(<source>((?:(?!<source>|</message>).)*?)</source>'
        r'(?:(?!<source>|</source>|</message>).)*?<translation\s+)'
        r'type="unfinished"[^>]*>([^<]*)</translation>',
        re.DOTALL,
    )

    translated_count = 0

    def lookup(source):
        tr = translations.get(source)
        if tr is not None:
            return tr
        if '\r' in source:
            tr = translations.get(source.replace('\r\n', '\n').replace('\r', '\n'))
        return tr

    def repl(m):
        nonlocal translated_count
        prefix = m.group(1)
        source = m.group(2)
        tr = lookup(source)
        if tr is None:
            return m.group(0)
        translated_count += 1
        tr_escaped = saxutils.escape(tr)
        return '%s>%s</translation>' % (prefix, tr_escaped)

    new_content = pattern.sub(repl, content)

    unfinished_after = new_content.count('type="unfinished"')
    delta = unfinished_before - unfinished_after
    total = content.count('<message>')
    finished_after = total - unfinished_after
    pct = (finished_after / total * 100.0) if total else 0.0
    print('[%s] Translated entries: %d' % (lang, translated_count))
    print('[%s] Unfinished after: %d' % (lang, unfinished_after))
    print('[%s] Coverage: %d/%d (%.1f%%)' % (lang, finished_after, total, pct))
    print('[%s] Target <=252 unfinished: %s' % (lang, 'PASS' if unfinished_after <= 252 else 'FAIL'))

    if dry_run:
        print('[%s] DRY RUN - no write' % lang)
        return 0

    with open(ts_path, 'w', encoding='utf-8', newline='') as f:
        f.write(new_content)
    print('[%s] Wrote %s' % (lang, ts_path))

    # Validate XML.
    try:
        import xml.etree.ElementTree as ET
        ET.parse(ts_path)
        print('[%s] XML validation: OK' % lang)
    except Exception as e:
        print('[%s] XML validation FAILED: %s' % (lang, e), file=sys.stderr)
        return 1

    return 0


# ---------------------------------------------------------------------------
# --clean-garbage: repair v5.3 left-over mixed-script "finished" entries.
#
# The v5.3 _v53_i18n_translate.py used naive substring concatenation, which
# produced mixed-script strings like  自动换色 -> "Auto换色"  (Chinese tail
# left untranslated). These are FINISHED (no type="unfinished"), so the normal
# translate pass never touches them. This mode re-translates FINISHED entries
# ONLY when:
#   (a) the <source> is an exact key in the combined (CORE + LONGTAIL) glossary, AND
#   (b) the current translation looks like v5.3 garbage (contains a Hanzi CJK
#       char AND a latin letter or Hangul syllable — i.e. it is not a clean,
#       fully-target-language translation), AND
#   (c) the current translation differs from the glossary value (so legit
#       entries that already equal the glossary are never churned).
#
# This leaves all cleanly-translated entries untouched and only overwrites the
# mixed-script garbage. Idempotent.
# ---------------------------------------------------------------------------

# Characters that are PRC-simplified forms NOT used as Japanese kanji in the
# same meaning. Used to detect untranslated Chinese leaking into ja translations
# (ja legitimately uses kanji, so we cannot use a plain "has Hanzi" test there).
# NOTE: 号 (number/signal) and 体 (cube/volume/body) are valid ja kanji that appear
# in ja translations (信号/番号/立方体/体積); excluding them prevents false positives.
_PRC_ONLY_CHARS = set(
    '换网扫码设视续线获确认态为时这们过给对从还并当发见场关问种样层讲边么义头专机'
)


def _is_v53_garbage(translation, lang, source=''):
    """Heuristic: a FINISHED translation is suspect v5.3 garbage.

    - For de/fr/ko: translation mixes a Hanzi character with a latin letter or
      (for ko) a Hangul syllable. These locales never use Hanzi, so any Hanzi in
      a finished translation is untranslated Chinese.
    - For ja: translation contains a PRC-simplified char that ALSO appears in the
      source string (i.e. the char was supposed to be translated but leaked
      verbatim). This avoids false positives on legitimate ja kanji.
    """
    has_hanzi = any(0x4E00 <= ord(ch) <= 0x9FFF for ch in translation)
    has_latin = any('a' <= ch.lower() <= 'z' for ch in translation)
    has_hangul = any(0xAC00 <= ord(ch) <= 0xD7A3 for ch in translation)
    if not has_hanzi:
        return False
    if lang == 'ja':
        # a PRC-only char present in BOTH source and translation => untranslated leak
        return any((ch in _PRC_ONLY_CHARS and ch in source) for ch in translation)
    return has_latin or has_hangul


def clean_garbage(lang, dry_run=False):
    # Combine CORE + LONGTAIL + GARBAGE_FIX; later dict wins per-lang on conflicts.
    try:
        from translate_core_i18n import CORE
    except Exception:
        CORE = {}
    try:
        from _v56_phase204_garbage_fix import GARBAGE_FIX
    except Exception:
        GARBAGE_FIX = {}
    combined = dict(CORE)
    for k, v in LONGTAIL.items():
        combined[k] = {**combined.get(k, {}), **v}
    for k, v in GARBAGE_FIX.items():
        combined[k] = {**combined.get(k, {}), **v}

    ts_path = os.path.join(I18N, '%s.ts' % lang)
    if not os.path.isfile(ts_path):
        print('ERROR: %s not found' % ts_path, file=sys.stderr)
        return 2

    with open(ts_path, 'r', encoding='utf-8', newline='') as f:
        content = f.read()

    # Match a finished translation line (no type attr) OR one with an
    # explicit type that is not "unfinished"/"obsolete"/"vanished".
    # Group 1 = '<source>...</source> ... <translation '
    # Group 2 = source text
    # Group 3 = current translation text
    # Group 4 = optional trailing attribute junk already consumed in prefix
    pattern = re.compile(
        r'(<source>((?:(?!<source>|</message>).)*?)</source>'
        r'(?:(?!<source>|</source>|</message>).)*?<translation)'
        r'(?:\s+type="unfinished"|\s+type="obsolete"|\s+type="vanished")?'
        r'[^>]*>([^<]*)</translation>',
        re.DOTALL,
    )

    cleaned = 0
    inspected = 0

    def repl(m):
        nonlocal cleaned, inspected
        prefix = m.group(1)
        source = m.group(2)
        current = m.group(3)
        if 'type="unfinished"' in prefix:
            return m.group(0)   # leave unfinished handling to translate_file
        norm = source.replace('\r\n', '\n').replace('\r', '\n')
        entry = combined.get(source) or combined.get(norm)
        if not entry or lang not in entry:
            return m.group(0)
        target = entry[lang]
        inspected += 1
        if target == current:
            return m.group(0)   # already correct, do not churn
        if not _is_v53_garbage(current, lang, source):
            return m.group(0)   # not garbage, leave alone
        cleaned += 1
        return '%s>%s</translation>' % (prefix, saxutils.escape(target))

    new_content = pattern.sub(repl, content)

    print('[%s] clean-garbage: glossary entries available=%d, inspected=%d, garbage replaced=%d'
          % (lang, len(combined), inspected, cleaned))

    if dry_run:
        print('[%s] DRY RUN - no write' % lang)
        return 0

    if cleaned == 0:
        print('[%s] no changes, skipping write' % lang)
        return 0

    with open(ts_path, 'w', encoding='utf-8', newline='') as f:
        f.write(new_content)
    print('[%s] Wrote %s' % (lang, ts_path))

    try:
        import xml.etree.ElementTree as ET
        ET.parse(ts_path)
        print('[%s] XML validation: OK' % lang)
    except Exception as e:
        print('[%s] XML validation FAILED: %s' % (lang, e), file=sys.stderr)
        return 1
    return 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('lang', choices=list(LANGS) + ['all'])
    ap.add_argument('--dry-run', action='store_true')
    ap.add_argument('--clean-garbage', action='store_true',
                    help='Also repair v5.3 left-over mixed-script FINISHED entries')
    args = ap.parse_args()

    def run(lang):
        rc = translate_file(lang, dry_run=args.dry_run)
        if args.clean_garbage:
            rc = clean_garbage(lang, dry_run=args.dry_run) or rc
        return rc

    if args.lang == 'all':
        rc = 0
        for lang in LANGS:
            rc = run(lang) or rc
            print()
        return rc
    return run(args.lang)


if __name__ == '__main__':
    sys.exit(main())
