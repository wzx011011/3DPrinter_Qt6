#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
_v56_phase204_garbage_fix.py
============================
Phase 204 — high-quality glossary to overwrite v5.3 left-over mixed-script
"finished" translations in the de/fr/ja/ko .ts files.

Background: the v5.3 _v53_i18n_translate.py used naive substring
concatenation and wrote strings like  自动换色 -> "Auto换色"  (Chinese tail
untranslated) directly into the .ts as FINISHED entries. The main Phase 204
long-tail pass only touches `type="unfinished"` entries, so it could not
repair these. This glossary provides clean, native-quality de/fr/ja/ko
translations for the ~295 distinct sources that carry such garbage; the
`--clean-garbage` mode of _v56_phase204_i18n_longtail.py overwrites them
when the current value tests positive for mixed-script garbage.

Quality rules (same as the long-tail glossary):
  - Technical terms (PLA/ABS/ASA/TPU/PETG/STL/G-code/3MF/AMS/Brim/Skirt/
    Raft/Wi-Fi/MQTT/IP/PIN/SLA/ID/Qt/AI) kept untranslated.
  - Placeholders (%1, %2), units (mm, °C, mm/s, MB, %) preserved.
  - Native, domain-accurate 3D-printer UI translations; no MT garbage.
"""

GARBAGE_FIX = {
    # ---- Counts / multipliers ----
    "%1 个对象": {"de": "%1 Objekt(e)", "fr": "%1 objet(s)", "ja": "%1 オブジェクト", "ko": "%1개 객체"},
    "0 个对象": {"de": "0 Objekte", "fr": "0 objet", "ja": "0 オブジェクト", "ko": "0개 객체"},
    "%1 台设备": {"de": "%1 Gerät(e)", "fr": "%1 appareil(s)", "ja": "%1 台のデバイス", "ko": "%1대 장치"},
    " 个对象  ·  层高 0.20 mm": {"de": " Objekt(e)  ·  Schichthöhe 0.20 mm", "fr": " objet(s)  ·  Hauteur de couche 0.20 mm", "ja": " オブジェクト  ·  層の高さ 0.20 mm", "ko": "객체  ·  레이어 높이 0.20 mm"},
    "(异步执行后状态显示于此)": {"de": "(Status nach asynchroner Ausführung wird hier angezeigt)", "fr": "(L'état après exécution asynchrone s'affiche ici)", "ja": "(非同期実行後の状態はここに表示されます)", "ko": "(비동기 실행 후 상태가 여기에 표시됨)"},
    "+ 添加层范围": {"de": "+ Schichtbereich hinzufügen", "fr": "+ Ajouter une plage de couches", "ja": "+ 層範囲を追加", "ko": "+ 레이어 범위 추가"},
    "%1 个部件": {"de": "%1 Teil(e)", "fr": "%1 pièce(s)", "ja": "%1 パーツ", "ko": "%1개 부품"},

    # ---- Object / device / filament quick labels ----
    "AI 切片优化": {"de": "AI-Slice-Optimierung", "fr": "Optimisation de tranchage IA", "ja": "AIスライス最適化", "ko": "AI 슬라이스 최적화"},
    "↻ 重新连接": {"de": "↻ Neu verbinden", "fr": "↻ Reconnecter", "ja": "↻ 再接続", "ko": "↻ 다시 연결"},
    "仅底板支撑": {"de": "Nur Druckplatten-Stützstruktur", "fr": "Support de plateau uniquement", "ja": "プレートサポートのみ", "ko": "플레이트 서포트만"},
    "从磁盘重新加载": {"de": "Neu von Festplatte laden", "fr": "Recharger depuis le disque", "ja": "ディスクから再読み込み", "ko": "디스크에서 다시 로드"},
    "从空白开始创建": {"de": "Neu (leer) erstellen", "fr": "Créer à partir de zéro", "ja": "新規作成（空）", "ko": "빈 프로젝트에서 만들기"},
    "偏好设置": {"de": "Einstellungen", "fr": "Préférences", "ja": "環境設定", "ko": "기본 설정"},
    "允许多耗材": {"de": "Mehrere Filamente erlauben", "fr": "Autoriser plusieurs filaments", "ja": "マルチフィラメントを許可", "ko": "다중 필라멘트 허용"},
    "全部保留": {"de": "Alle behalten", "fr": "Tout conserver", "ja": "すべて保持", "ko": "전체 유지"},
    "共 %1 条记录": {"de": "%1 Einträge gesamt", "fr": "%1 entrée(s) au total", "ja": "全 %1 件", "ko": "총 %1건"},
    "关闭测量": {"de": "Messen schließen", "fr": "Fermer la mesure", "ja": "測定を閉じる", "ko": "측정 닫기"},
    "关闭风扇层数": {"de": "Lüfter-aus-Schichten", "fr": "Couches avec ventilateur arrêté", "ja": "ファン停止層数", "ko": "팬 정지 레이어 수"},
    "其他层耗材顺序": {"de": "Filamentreihenfolge andere Schichten", "fr": "Ordre des filaments autres couches", "ja": "他の層のフィラメント順序", "ko": "다른 레이어 필라멘트 순서"},
    "内壁→填充→外壁": {"de": "Innenwand → Füllung → Außenwand", "fr": "Paroi interne → remplissage → paroi externe", "ja": "内壁→インフィル→外壁", "ko": "내벽→채움→외벽"},
    "内部填充打印速度。": {"de": "Druckgeschwindigkeit der Innenfüllung.", "fr": "Vitesse d'impression du remplissage interne.", "ja": "内部インフィルのプリント速度。", "ko": "내부 채움 출력 속도."},
    "准备/预览": {"de": "Vorbereiten/Vorschau", "fr": "Préparer/Aperçu", "ja": "準備/プレビュー", "ko": "준비/미리보기"},
    "准备开始校准": {"de": "Bereit zum Starten der Kalibrierung", "fr": "Prêt à démarrer l'étalonnage", "ja": "キャリブレーション開始の準備", "ko": "캘리브레이션 시작 준비"},
    "切片后自动发送": {"de": "Nach dem Slicen automatisch senden", "fr": "Envoyer automatiquement après le tranchage", "ja": "スライス後に自動送信", "ko": "슬라이스 후 자동 전송"},
    "切片完成后自动上传": {"de": "Nach Slicen automatisch hochladen", "fr": "Téléverser automatiquement après le tranchage", "ja": "スライス完了後に自動アップロード", "ko": "슬라이스 완료 후 자동 업로드"},
    "创建新的用户预设": {"de": "Neue Benutzervoreinstellung erstellen", "fr": "Créer un nouveau préréglage utilisateur", "ja": "新しいユーザープリセットを作成", "ko": "새 사용자 사전 설정 만들기"},
    "创建预设": {"de": "Voreinstellung erstellen", "fr": "Créer un préréglage", "ja": "プリセットを作成", "ko": "사전 설정 만들기"},
    "删除该范围": {"de": "Diesen Bereich löschen", "fr": "Supprimer cette plage", "ja": "この範囲を削除", "ko": "이 범위 삭제"},
    "删除选中 (%1)": {"de": "Auswahl löschen (%1)", "fr": "Supprimer la sélection (%1)", "ja": "選択を削除 (%1)", "ko": "선택 삭제 (%1)"},
    "刷新设备列表": {"de": "Geräteliste aktualisieren", "fr": "Actualiser la liste des appareils", "ja": "デバイスリストを更新", "ko": "장치 목록 새로고침"},
    "另存为预设": {"de": "Als Voreinstellung speichern", "fr": "Enregistrer comme préréglage", "ja": "プリセットとして保存", "ko": "사전 설정으로 저장"},
    "可在设置中随时切换": {"de": "Jederzeit in den Einstellungen umschaltbar", "fr": "Modifiable à tout moment dans les paramètres", "ja": "設定でいつでも切り替え可能", "ko": "설정에서 언제든지 전환 가능"},
    "右键删除单个支撑": {"de": "Rechtsklick entfernt einzelne Stützstruktur", "fr": "Clic droit pour supprimer un support", "ja": "右クリックで個別サポートを削除", "ko": "우클릭으로 개별 서포트 삭제"},
    "启用回退": {"de": "Rückzug aktivieren", "fr": "Activer la rétraction", "ja": "リトラクションを有効化", "ko": "리트랙션 활성화"},
    "启用空洞化:": {"de": "Aushöhlen aktivieren:", "fr": "Activer le creusement :", "ja": "中空化を有効化:", "ko": "속 비움 활성화:"},
    "启用精简预览模式": {"de": "Lite-Vorschaumodus aktivieren", "fr": "Activer le mode d'aperçu léger", "ja": "ライトプレビューモードを有効化", "ko": "라이트 미리보기 모드 활성화"},
    "启用镂空:": {"de": "Aushöhlen aktivieren:", "fr": "Activer le creusement :", "ja": "中空化を有効化:", "ko": "속 비움 활성화:"},
    "喷嘴直径 (mm)": {"de": "Düsendurchmesser (mm)", "fr": "Diamètre de buse (mm)", "ja": "ノズル径 (mm)", "ko": "노즐 지름 (mm)"},
    "喷嘴（热端）温度。": {"de": "Düsentemperatur (Hotend).", "fr": "Température de la buse (hotend).", "ja": "ノズル（ホットエンド）温度。", "ko": "노즐(핫엔드) 온도."},
    "回抽长度 (mm)": {"de": "Rückzugslänge (mm)", "fr": "Longueur de rétraction (mm)", "ja": "リトラクション長さ (mm)", "ko": "리트랙션 길이 (mm)"},
    "在线下载模型": {"de": "Modell online herunterladen", "fr": "Télécharger un modèle en ligne", "ja": "オンラインでモデルをダウンロード", "ko": "온라인으로 모델 다운로드"},
    "填充图案": {"de": "Füllungsmuster", "fr": "Motif de remplissage", "ja": "インフィルパターン", "ko": "채움 패턴"},
    "填充密度 (%)": {"de": "Füllungsdichte (%)", "fr": "Densité de remplissage (%)", "ja": "インフィル密度 (%)", "ko": "채움 밀도 (%)"},
    "填充角度 (°)": {"de": "Füllungswinkel (°)", "fr": "Angle de remplissage (°)", "ja": "インフィル角度 (°)", "ko": "채움 각도 (°)"},
    "填充重叠 (%)": {"de": "Füllungsüberlappung (%)", "fr": "Chevauchement de remplissage (%)", "ja": "インフィルオーバーラップ (%)", "ko": "채움 겹침 (%)"},
    "壁/填充顺序": {"de": "Wand-/Füllungsreihenfolge", "fr": "Ordre paroi/remplissage", "ja": "壁/インフィル順序", "ko": "벽/채움 순서"},
    "壁打印顺序": {"de": "Wanddruckreihenfolge", "fr": "Ordre d'impression des parois", "ja": "壁プリント順序", "ko": "벽 출력 순서"},
    "外壁→填充→内壁": {"de": "Außenwand → Füllung → Innenwand", "fr": "Paroi externe → remplissage → paroi interne", "ja": "外壁→インフィル→内壁", "ko": "외벽→채움→내벽"},
    "多设备": {"de": "Mehrere Geräte", "fr": "Plusieurs appareils", "ja": "複数デバイス", "ko": "다중 장치"},
    "完成后通知": {"de": "Nach Abschluss benachrichtigen", "fr": "Notifier à la fin", "ja": "完了時に通知", "ko": "완료 후 알림"},
    "完成校准后会在此显示记录": {"de": "Nach Abschluss der Kalibrierung werden hier Aufzeichnungen angezeigt", "fr": "Les enregistrements s'afficheront ici après l'étalonnage", "ja": "キャリブレーション完了後に記録がここに表示されます", "ko": "캘리브레이션 완료 후 기록이 여기에 표시됩니다"},
    "对象列表": {"de": "Objektliste", "fr": "Liste des objets", "ja": "オブジェクトリスト", "ko": "객체 목록"},
    "对齐 Y 轴": {"de": "An Y-Achse ausrichten", "fr": "Aligner sur l'axe Y", "ja": "Y軸に整列", "ko": "Y축 정렬"},
    "将选中对象最大面朝下平放": {"de": "Ausgewählte Objekte mit größter Fläche nach unten hinlegen", "fr": "Mettre à plat les objets sélectionnés sur leur plus grande face", "ja": "選択したオブジェクトの最大面を下にして配置", "ko": "선택한 객체의 가장 큰 면이 아래로 가게 놓기"},
    "层高范围": {"de": "Schichthöhenbereich", "fr": "Plage de hauteurs de couche", "ja": "層の高さの範囲", "ko": "레이어 높이 범위"},
    "层高范围...": {"de": "Schichthöhenbereich...", "fr": "Plage de hauteurs de couche...", "ja": "層の高さの範囲...", "ko": "레이어 높이 범위..."},
    "已导出到: %1": {"de": "Exportiert nach: %1", "fr": "Exporté vers : %1", "ja": "エクスポート先: %1", "ko": "내보낸 위치: %1"},
    "已用耗材重量 (g)": {"de": "Verbrauchtes Filamentgewicht (g)", "fr": "Poids de filament utilisé (g)", "ja": "使用済みフィラメント重量 (g)", "ko": "사용된 필라멘트 무게 (g)"},
    "已用耗材长度 (mm)": {"de": "Verbrauchte Filamentlänge (mm)", "fr": "Longueur de filament utilisée (mm)", "ja": "使用済みフィラメント長さ (mm)", "ko": "사용된 필라멘트 길이 (mm)"},
    "已连接，等待视频流": {"de": "Verbunden, warte auf Videostream", "fr": "Connecté, en attente du flux vidéo", "ja": "接続済み、動画ストリームを待機中", "ko": "연결됨, 비디오 스트림 대기 중"},
    "已选 %1 个部件": {"de": "%1 Teil(e) ausgewählt", "fr": "%1 pièce(s) sélectionnée(s)", "ja": "%1 個のパーツを選択", "ko": "%1개 부품 선택됨"},
    "平板切片状态": {"de": "Druckplatten-Slicestatus", "fr": "État du tranchage de la platine", "ja": "プレートスライス状態", "ko": "플레이트 슬라이스 상태"},
    "底筏层数": {"de": "Raft-Schichtanzahl", "fr": "Nombre de couches de radeau", "ja": "ラフト層数", "ko": "래프트 레이어 수"},
    "当前对象 ID": {"de": "Aktuelle Objekt-ID", "fr": "ID de l'objet actuel", "ja": "現在のオブジェクト ID", "ko": "현재 객체 ID"},
    "当前对象名称": {"de": "Aktueller Objektname", "fr": "Nom de l'objet actuel", "ja": "現在のオブジェクト名", "ko": "현재 객체 이름"},
    "当前对象高度 (mm)": {"de": "Aktuelle Objekthöhe (mm)", "fr": "Hauteur de l'objet actuel (mm)", "ja": "現在のオブジェクト高さ (mm)", "ko": "현재 객체 높이 (mm)"},
    "当前挤出机 ID": {"de": "Aktuelle Extruder-ID", "fr": "ID de l'extrudeur actuel", "ja": "現在のエクストルーダー ID", "ko": "현재 익스트루더 ID"},
    "当前挤出机温度": {"de": "Aktuelle Extrudertemperatur", "fr": "Température de l'extrudeur actuel", "ja": "現在のエクストルーダー温度", "ko": "현재 익스트루더 온도"},
    "当前耗材: %1": {"de": "Aktuelles Filament: %1", "fr": "Filament actuel : %1", "ja": "現在のフィラメント: %1", "ko": "현재 필라멘트: %1"},
    "当前覆盖项：%1": {"de": "Aktuelle Überschreibung: %1", "fr": "Remplacement actuel : %1", "ja": "現在の上書き項目: %1", "ko": "현재 재정의 항목: %1"},
    "总对象数": {"de": "Objekte gesamt", "fr": "Total d'objets", "ja": "総オブジェクト数", "ko": "전체 객체 수"},
    "恢复默认": {"de": "Auf Standard zurücksetzen", "fr": "Restaurer par défaut", "ja": "デフォルトに戻す", "ko": "기본값 복원"},
    "悬停特征: 无": {"de": "Merkmal: keines", "fr": "Caractéristique : aucune", "ja": "特徴: なし", "ko": "특징: 없음"},
    "悬停特征: 未知": {"de": "Merkmal: unbekannt", "fr": "Caractéristique : inconnue", "ja": "特徴: 不明", "ko": "특징: 알 수 없음"},
    "所有平板已切片完成": {"de": "Alle Druckplatten wurden geslict", "fr": "Toutes les platines ont été tranchées", "ja": "すべてのプレートのスライスが完了しました", "ko": "모든 플레이트 슬라이스 완료"},
    "打印冷却风扇速度百分比。": {"de": "Prozentuale Lüftergeschwindigkeit für Druckkühlung.", "fr": "Pourcentage de vitesse du ventilateur de refroidissement d'impression.", "ja": "プリント冷却ファン速度の割合。", "ko": "출력 냉각 팬 속도 비율."},
    "打印机 IP 地址": {"de": "IP-Adresse des Druckers", "fr": "Adresse IP de l'imprimante", "ja": "プリンターの IP アドレス", "ko": "프린터 IP 주소"},
    "打印机校准向导": {"de": "Drucker-Kalibrierungsassistent", "fr": "Assistant d'étalonnage de l'imprimante", "ja": "プリンターキャリブレーションウィザード", "ko": "프린터 캘리브레이션 마법사"},
    "打印质量": {"de": "Druckqualität", "fr": "Qualité d'impression", "ja": "プリント品質", "ko": "출력 품질"},
    "执行简化": {"de": "Vereinfachung ausführen", "fr": "Exécuter la simplification", "ja": "簡略化を実行", "ko": "단순화 실행"},
    "扫描并添加新设备": {"de": "Nach neuen Geräten suchen und hinzufügen", "fr": "Rechercher et ajouter de nouveaux appareils", "ja": "スキャンして新しいデバイスを追加", "ko": "검색하여 새 장치 추가"},
    "拆分对象": {"de": "Objekt teilen", "fr": "Diviser l'objet", "ja": "オブジェクトを分割", "ko": "객체 분할"},
    "挤出机 ": {"de": "Extruder ", "fr": "Extrudeur ", "ja": "エクストルーダー ", "ko": "익스트루더 "},
    "挤出机 %1": {"de": "Extruder %1", "fr": "Extrudeur %1", "ja": "エクストルーダー %1", "ko": "익스트루더 %1"},
    "挤出机顺序（拖拽调整）": {"de": "Extruderreihenfolge (per Drag anpassen)", "fr": "Ordre des extrudeurs (glisser pour ajuster)", "ja": "エクストルーダー順序（ドラッグで調整）", "ko": "익스트루더 순서(드래그로 조정)"},
    "撞击设置": {"de": "Kollisionseinstellungen", "fr": "Paramètres de collision", "ja": "衝突設定", "ko": "충돌 설정"},
    "撤销栈上限": {"de": "Maximale Rückgängig-Historie", "fr": "Limite de l'historique d'annulation", "ja": "元に戻す履歴の上限", "ko": "실행 취소 스택 한도"},
    "播放/暂停": {"de": "Wiedergabe/Pause", "fr": "Lecture/Pause", "ja": "再生/一時停止", "ko": "재생/일시정지"},
    "播放/暂停预览动画": {"de": "Vorschauanimation wiedergeeben/pausieren", "fr": "Lecture/Pause de l'animation d'aperçu", "ja": "プレビューアニメーションの再生/一時停止", "ko": "미리보기 애니메이션 재생/일시정지"},
    "擦洗设置": {"de": "Wisch-Einstellungen", "fr": "Paramètres d'essuyage", "ja": "ワイプ設定", "ko": "와이프 설정"},
    "支撑XY间距 (mm)": {"de": "Stützstruktur XY-Abstand (mm)", "fr": "Espacement XY du support (mm)", "ja": "サポートXY間隔 (mm)", "ko": "서포트 XY 간격 (mm)"},
    "支撑密度 (%)": {"de": "Stützstrukturdichte (%)", "fr": "Densité du support (%)", "ja": "サポート密度 (%)", "ko": "서포트 밀도 (%)"},
    "支撑底面层数": {"de": "Stützstruktur-Bodenschichten", "fr": "Couches inférieures du support", "ja": "サポート下面層数", "ko": "서포트 하단 레이어 수"},
    "支撑界面间距": {"de": "Stützstruktur-Schnittstellenabstand", "fr": "Espacement de l'interface de support", "ja": "サポートインターフェース間隔", "ko": "서포트 인터페이스 간격"},
    "支撑结构的密度百分比。": {"de": "Dichte der Stützstruktur in Prozent.", "fr": "Pourcentage de densité de la structure de support.", "ja": "サポート構造の密度の割合。", "ko": "서포트 구조의 밀도 비율."},
    "支撑结构的打印速度。": {"de": "Druckgeschwindigkeit der Stützstruktur.", "fr": "Vitesse d'impression de la structure de support.", "ja": "サポート構造のプリント速度。", "ko": "서포트 구조의 출력 속도."},
    "支撑膨胀 (mm)": {"de": "Stützstruktur-Ausdehnung (mm)", "fr": "Expansion du support (mm)", "ja": "サポート膨張 (mm)", "ko": "서포트 확장 (mm)"},
    "支撑角度 (°)": {"de": "Stützstrukturwinkel (°)", "fr": "Angle du support (°)", "ja": "サポート角度 (°)", "ko": "서포트 각도 (°)"},
    "支撑顶面层数": {"de": "Stützstruktur-Deckschichten", "fr": "Couches supérieures du support", "ja": "サポート上面層数", "ko": "서포트 상단 레이어 수"},
    "无差异": {"de": "Kein Unterschied", "fr": "Aucune différence", "ja": "差異なし", "ko": "차이 없음"},
    "映射配置": {"de": "Zuordnungskonfiguration", "fr": "Configuration du mappage", "ja": "マッピング設定", "ko": "매핑 설정"},
    "是否启用擦料塔": {"de": "Reinigungsturm aktivieren?", "fr": "Activer la tour de nettoyage ?", "ja": "ワイプタワーを有効化?", "ko": "와이프 타워 활성화?"},
    "是否生成支撑": {"de": "Stützstruktur erzeugen?", "fr": "Générer le support ?", "ja": "サポートを生成?", "ko": "서포트 생성?"},
    "显示提示": {"de": "Hinweise anzeigen", "fr": "Afficher les astuces", "ja": "ヒントを表示", "ko": "팁 표시"},
    "显示进度通知": {"de": "Fortschrittsbenachrichtigungen anzeigen", "fr": "Afficher les notifications de progression", "ja": "進行状況通知を表示", "ko": "진행률 알림 표시"},
    "最大打印高度 (mm)": {"de": "Maximale Druckhöhe (mm)", "fr": "Hauteur d'impression maximale (mm)", "ja": "最大プリント高さ (mm)", "ko": "최대 출력 높이 (mm)"},
    "最近项目": {"de": "Zuletzt verwendete Projekte", "fr": "Projets récents", "ja": "最近のプロジェクト", "ko": "최근 프로젝트"},
    "未知任务": {"de": "Unbekannte Aufgabe", "fr": "Tâche inconnue", "ja": "不明なタスク", "ko": "알 수 없는 작업"},
    "松开以导入模型": {"de": "Loslassen, um Modell zu importieren", "fr": "Relâcher pour importer un modèle", "ja": "離すとモデルをインポート", "ko": "놓으면 모델 가져오기"},
    "构建日期": {"de": "Builddatum", "fr": "Date de build", "ja": "ビルド日付", "ko": "빌드 날짜"},
    "校准完成！": {"de": "Kalibrierung abgeschlossen!", "fr": "Étalonnage terminé !", "ja": "キャリブレーション完了！", "ko": "캘리브레이션 완료!"},
    "检查网络连接": {"de": "Netzwerkverbindung prüfen", "fr": "Vérifier la connexion réseau", "ja": "ネットワーク接続を確認", "ko": "네트워크 연결 확인"},
    "检查设备电源": {"de": "Gerätstromversorgung prüfen", "fr": "Vérifier l'alimentation de l'appareil", "ja": "デバイスの電源を確認", "ko": "장치 전원 확인"},
    "模型商城": {"de": "Modell-Store", "fr": "Magasin de modèles", "ja": "モデルストア", "ko": "모델 스토어"},
    "模型底面的实心层数。": {"de": "Anzahl der massiven Schichten an der Modellunterseite.", "fr": "Nombre de couches pleines à la base du modèle.", "ja": "モデル下面の実心層数。", "ko": "모델 하단의 솔리드 레이어 수."},
    "模型顶面的实心层数。": {"de": "Anzahl der massiven Schichten an der Modelloberseite.", "fr": "Nombre de couches pleines au sommet du modèle.", "ja": "モデル上面の実心層数。", "ko": "모델 상단의 솔리드 레이어 수."},
    "正在切片... %1%": {"de": "Slicen... %1%", "fr": "Tranchage... %1%", "ja": "スライス中... %1%", "ko": "슬라이스 중... %1%"},
    "正在连接...": {"de": "Verbinde...", "fr": "Connexion...", "ja": "接続中...", "ko": "연결 중..."},
    "比较预设": {"de": "Voreinstellungen vergleichen", "fr": "Comparer les préréglages", "ja": "プリセットを比較", "ko": "사전 설정 비교"},
    "浮雕失败：": {"de": "Prägung fehlgeschlagen:", "fr": "Échec de l'embossage :", "ja": "エンボス失敗:", "ko": "엠보싱 실패:"},
    "浮雕完成：": {"de": "Prägung abgeschlossen:", "fr": "Embossage terminé :", "ja": "エンボス完了:", "ko": "엠보싱 완료:"},
    "深色 (默认)": {"de": "Dunkel (Standard)", "fr": "Sombre (par défaut)", "ja": "ダーク (デフォルト)", "ko": "다크 (기본값)"},
    "添加修改器": {"de": "Modifikator hinzufügen", "fr": "Ajouter un modificateur", "ja": "モディファイアを追加", "ko": "수정자 추가"},
    "添加支撑增强": {"de": "Stützstruktur-Erzwingung hinzufügen", "fr": "Ajouter un forçage de support", "ja": "サポートエンフォーサーを追加", "ko": "서포트 강제 추가"},
    "添加支撑屏蔽": {"de": "Stützstruktur-Blocker hinzufügen", "fr": "Ajouter un bloqueur de support", "ja": "サポートブロッカーを追加", "ko": "서포트 차단 추가"},
    "添加负体积": {"de": "Negatives Volumen hinzufügen", "fr": "Ajouter un volume négatif", "ja": "ネガティブボリュームを追加", "ko": "네거티브 볼륨 추가"},
    "清空校准历史": {"de": "Kalibrierungsverlauf leeren", "fr": "Vider l'historique d'étalonnage", "ja": "キャリブレーション履歴をクリア", "ko": "캘리브레이션 기록 비우기"},
    "清除全部": {"de": "Alle löschen", "fr": "Tout effacer", "ja": "すべてクリア", "ko": "전체 지우기"},
    "点击下方按钮切换预设": {"de": "Auf die Schaltfläche unten klicken, um die Voreinstellung zu wechseln", "fr": "Cliquez sur le bouton ci-dessous pour changer de préréglage", "ja": "下のボタンをクリックしてプリセットを切り替え", "ko": "아래 버튼을 클릭하여 사전 설정 전환"},
    "点击模型表面添加支撑点": {"de": "Auf Modeloberfläche klicken, um Stützpunkt hinzuzufügen", "fr": "Cliquez sur la surface du modèle pour ajouter un point de support", "ja": "モデル表面をクリックしてサポート点を追加", "ko": "모델 표면을 클릭하여 서포트 점 추가"},
    "界面缩放": {"de": "Schnittstellenskalierung", "fr": "Mise à l'échelle de l'interface", "ja": "インターフェース倍率", "ko": "인터페이스 배율"},
    "省耗材": {"de": "Filament sparen", "fr": "Économiser le filament", "ja": "フィラメント節約", "ko": "필라멘트 절약"},
    "确保打印机固件为最新版本": {"de": "Sicherstellen, dass die Drucker-Firmware aktuell ist", "fr": "Assurez-vous que le firmware de l'imprimante est à jour", "ja": "プリンターのファームウェアが最新であることを確認", "ko": "프린터 펌웨어가 최신 버전인지 확인"},
    "移动平板失败": {"de": "Druckplatte verschieben fehlgeschlagen", "fr": "Échec du déplacement de la platine", "ja": "プレートの移動に失敗", "ko": "플레이트 이동 실패"},
    "空走（非打印）移动速度。": {"de": "Eilganggeschwindigkeit (Nicht-Druck-Bewegungen).", "fr": "Vitesse de déplacement (hors impression).", "ja": "移動（非プリント）速度。", "ko": "이동(비출력) 속도."},
    "精简预览模式": {"de": "Lite-Vorschaumodus", "fr": "Mode d'aperçu léger", "ja": "ライトプレビューモード", "ko": "라이트 미리보기 모드"},
    "编辑自定义 G-code": {"de": "Benutzerdefinierten G-code bearbeiten", "fr": "Modifier le G-code personnalisé", "ja": "カスタムG-codeを編集", "ko": "사용자 지정 G-code 편집"},
    "耗材分组": {"de": "Filamentgruppierung", "fr": "Regroupement de filaments", "ja": "フィラメントグループ", "ko": "필라멘트 그룹"},
    "耗材用量明细": {"de": "Filamentverbrauch-Details", "fr": "Détails de consommation de filament", "ja": "フィラメント使用量の詳細", "ko": "필라멘트 사용량 상세"},
    "自动备份项目到云端": {"de": "Projekt automatisch in der Cloud sichern", "fr": "Sauvegarder automatiquement le projet dans le cloud", "ja": "プロジェクトを自動的にクラウドにバックアップ", "ko": "프로젝트를 클라우드에 자동 백업"},
    "自动换色": {"de": "Automatischer Farbwechsel", "fr": "Changement de couleur automatique", "ja": "自動色変更", "ko": "자동 색상 변경"},
    "自动朝向": {"de": "Automatische Ausrichtung", "fr": "Orientation automatique", "ja": "自動向き設定", "ko": "자동 방향"},
    "自动检查更新": {"de": "Automatisch nach Updates suchen", "fr": "Vérifier automatiquement les mises à jour", "ja": "自動でアップデートを確認", "ko": "자동으로 업데이트 확인"},
    "自动消失时间": {"de": "Auto-Ausblendzeit", "fr": "Durée d'auto-masquage", "ja": "自動非表示時間", "ko": "자동 숨김 시간"},
    "装配体信息": {"de": "Assembly-Informationen", "fr": "Informations d'assemblage", "ja": "アセンブリ情報", "ko": "어셈블리 정보"},
    "裙边宽度 (mm)": {"de": "Skirt-Breite (mm)", "fr": "Largeur de jupe (mm)", "ja": "スカート幅 (mm)", "ko": "스커트 너비 (mm)"},
    "裙边线数": {"de": "Skirt-Linienanzahl", "fr": "Nombre de lignes de jupe", "ja": "スカート線数", "ko": "스커트 선 수"},
    "裙边距离 (mm)": {"de": "Skirt-Abstand (mm)", "fr": "Distance de la jupe (mm)", "ja": "スカート距離 (mm)", "ko": "스커트 거리 (mm)"},
    "裙边高度 (层)": {"de": "Skirt-Höhe (Schichten)", "fr": "Hauteur de jupe (couches)", "ja": "スカート高さ (層)", "ko": "스커트 높이 (레이어)"},
    "解绑云设备": {"de": "Cloud-Gerät trennen", "fr": "Délier l'appareil cloud", "ja": "クラウドデバイスのバインド解除", "ko": "클라우드 장치 바인딩 해제"},
    "设备运行正常，暂无告警": {"de": "Gerät läuft normal, keine Warnungen", "fr": "L'appareil fonctionne normalement, aucune alerte", "ja": "デバイスは正常に動作しています、アラートなし", "ko": "장치가 정상적으로 작동 중, 알림 없음"},
    "该名称的预设已存在": {"de": "Eine Voreinstellung mit diesem Namen existiert bereits", "fr": "Un préréglage avec ce nom existe déjà", "ja": "この名前のプリセットは既に存在します", "ko": "이 이름의 사전 설정이 이미 존재합니다"},
    "请从左侧选择一台设备": {"de": "Bitte links ein Gerät auswählen", "fr": "Veuillez sélectionner un appareil à gauche", "ja": "左側からデバイスを1つ選択してください", "ko": "왼쪽에서 장치를 선택하세요"},
    "请选择两个预设后点击对比": {"de": "Bitte zwei Voreinstellungen wählen und auf Vergleichen klicken", "fr": "Sélectionnez deux préréglages puis cliquez sur Comparer", "ja": "2つのプリセットを選択してから比較をクリック", "ko": "두 사전 설정을 선택한 후 비교를 클릭하세요"},
    "请选择您常用的耗材类型：": {"de": "Bitte Ihren üblichen Filamenttyp wählen:", "fr": "Veuillez sélectionner votre type de filament habituel :", "ja": "よく使うフィラメントの種類を選択してください:", "ko": "자주 사용하는 필라멘트 유형을 선택하세요:"},
    "输入预设名称": {"de": "Voreinstellungsname eingeben", "fr": "Saisir le nom du préréglage", "ja": "プリセット名を入力", "ko": "사전 설정 이름 입력"},
    "运算类型": {"de": "Operationstyp", "fr": "Type d'opération", "ja": "演算タイプ", "ko": "연산 유형"},
    "连接 Bambu 打印机": {"de": "Bambu-Drucker verbinden", "fr": "Connecter une imprimante Bambu", "ja": "Bambuプリンターに接続", "ko": "Bambu 프린터 연결"},
    "连接到 %1": {"de": "Verbinden mit %1", "fr": "Connecter à %1", "ja": "%1 に接続", "ko": "%1에 연결"},
    "选择文件查看详情": {"de": "Datei auswählen, um Details zu sehen", "fr": "Sélectionner un fichier pour voir les détails", "ja": "ファイルを選択して詳細を表示", "ko": "파일을 선택하여 상세 보기"},
    "重置为预设值": {"de": "Auf Voreinstellungswert zurücksetzen", "fr": "Réinitialiser à la valeur du préréglage", "ja": "プリセット値にリセット", "ko": "사전 설정값으로 초기화"},
    "镂空设置": {"de": "Aushöhlungs-Einstellungen", "fr": "Paramètres de creusement", "ja": "中空化設定", "ko": "속 비움 설정"},
    "防火墙设置": {"de": "Firewall-Einstellungen", "fr": "Paramètres du pare-feu", "ja": "ファイアウォール設定", "ko": "방화벽 설정"},
    "需选中 2 个以上对象": {"de": "Mindestens 2 Objekte auswählen", "fr": "Sélectionner au moins 2 objets", "ja": "2つ以上のオブジェクトを選択する必要があります", "ko": "2개 이상의 객체를 선택해야 합니다"},
    "顶底填充重叠 (%)": {"de": "Überlappung obere/untere Füllung (%)", "fr": "Chevauchement remplissage haut/bas (%)", "ja": "上下インフィルオーバーラップ (%)", "ko": "상하 채움 겹침 (%)"},
    "项目已保存到: %1": {"de": "Projekt gespeichert unter: %1", "fr": "Projet enregistré sous : %1", "ja": "プロジェクトを保存しました: %1", "ko": "프로젝트가 저장됨: %1"},
    "项目资源": {"de": "Projektressourcen", "fr": "Ressources du projet", "ja": "プロジェクトリソース", "ko": "프로젝트 리소스"},
    "预览模式设置": {"de": "Vorschaumodus-Einstellungen", "fr": "Paramètres du mode d'aperçu", "ja": "プレビューモード設定", "ko": "미리보기 모드 설정"},
    "预览步进 ±100": {"de": "Vorschau-Schritt ±100", "fr": "Pas d'aperçu ±100", "ja": "プレビューステップ ±100", "ko": "미리보기 스텝 ±100"},
    "预览跳到头/尾": {"de": "Vorschau an Anfang/Ende springen", "fr": "Aperçu : aller au début/à la fin", "ja": "プレビューの先頭/末尾へジャンプ", "ko": "미리보기 처음/끝으로 이동"},
    "预计打印时长": {"de": "Geschätzte Druckdauer", "fr": "Durée d'impression estimée", "ja": "推定プリント時間", "ko": "예상 출력 시간"},
    "预设对比": {"de": "Voreinstellungs-Vergleich", "fr": "Comparaison des préréglages", "ja": "プリセット比較", "ko": "사전 설정 비교"},
    "预设已修改（未保存）": {"de": "Voreinstellung geändert (nicht gespeichert)", "fr": "Préréglage modifié (non enregistré)", "ja": "プリセットが変更されました（未保存）", "ko": "사전 설정 수정됨 (저장 안 됨)"},
    "预设视角 前视": {"de": "Voreingestellte Ansicht Vorderansicht", "fr": "Vue préréglée Vue de face", "ja": "プリセットビュー 正面図", "ko": "사전 설정 보기 정면"},
    "预设视角 顶/右/等轴": {"de": "Voreingestellte Ansicht Oben/Rechts/Isometrisch", "fr": "Vue préréglée Dessus/Droite/Isométrique", "ja": "プリセットビュー 上/右/等角", "ko": "사전 설정 보기 상/우/등각"},
    "首层喷嘴温度 (°C)": {"de": "Düsentemperatur der ersten Schicht (°C)", "fr": "Température de buse de la première couche (°C)", "ja": "最初の層のノズル温度 (°C)", "ko": "첫 레이어 노즐 온도 (°C)"},
    "首层层高 (mm)": {"de": "Höhe der ersten Schicht (mm)", "fr": "Hauteur de la première couche (mm)", "ja": "最初の層の高さ (mm)", "ko": "첫 레이어 높이 (mm)"},
    "首层打印中": {"de": "Erste Schicht druckt", "fr": "Impression de la première couche", "ja": "最初の層をプリント中", "ko": "첫 레이어 출력 중"},
    "首层打印高度 (mm)": {"de": "Druckhöhe der ersten Schicht (mm)", "fr": "Hauteur d'impression de la première couche (mm)", "ja": "最初の層のプリント高さ (mm)", "ko": "첫 레이어 출력 높이 (mm)"},
    "首层耗材顺序": {"de": "Filamentreihenfolge erste Schicht", "fr": "Ordre des filaments première couche", "ja": "最初の層のフィラメント順序", "ko": "첫 레이어 필라멘트 순서"},
    "高级支撑生成器": {"de": "Erweiterter Stützstruktur-Generator", "fr": "Générateur de support avancé", "ja": "詳細サポートジェネレーター", "ko": "고급 서포트 생성기"},
    "默认加速度": {"de": "Standard-Beschleunigung", "fr": "Accélération par défaut", "ja": "デフォルト加速度", "ko": "기본 가속도"},
    "（双击添加）": {"de": "(Doppelklick zum Hinzufügen)", "fr": "(double-cliquer pour ajouter)", "ja": "(ダブルクリックで追加)", "ko": "(더블클릭으로 추가)"},
    "（继承预设）": {"de": "(Voreinstellung vererbt)", "fr": "(préréglage hérité)", "ja": "(プリセットを継承)", "ko": "(사전 설정 상속)"},
    "（需 SLA 切片配置）": {"de": "(SLA-Slice-Konfiguration erforderlich)", "fr": "(configuration de tranchage SLA requise)", "ja": "(SLAスライス設定が必要)", "ko": "(SLA 슬라이스 설정 필요)"},
    "🔍 扫描设备": {"de": "🔍 Geräte suchen", "fr": "🔍 Rechercher des appareils", "ja": "🔍 デバイスをスキャン", "ko": "🔍 장치 검색"},
    "自动消失时间 (s)": {"de": "Auto-Ausblendzeit (s)", "fr": "Durée d'auto-masquage (s)", "ja": "自動非表示時間 (s)", "ko": "자동 숨김 시간 (s)"},
    "构建类型": {"de": "Build-Typ", "fr": "Type de build", "ja": "ビルドタイプ", "ko": "빌드 유형"},

    # ---- Long descriptions / tips / changelog ----
    "ABS 强度高，耐热性好，适合工程零件。打印时建议关闭风扇，防止翘边。": {
        "de": "ABS hat hohe Festigkeit und gute Hitzebeständigkeit, geeignet für technische Teile. Beim Drucken wird empfohlen, den Lüfter auszuschalten, um Verzug zu vermeiden.",
        "fr": "L'ABS offre une grande solidité et une bonne résistance à la chaleur, idéal pour les pièces techniques. Il est recommandé de couper le ventilateur pendant l'impression pour éviter le délaminage.",
        "ja": "ABSは強度が高く耐熱性に優れ、エンジニアリング部品に適しています。反りを防ぐためプリント時はファンをオフにすることを推奨します。",
        "ko": "ABS는 강도가 높고 내열성이 좋아 엔지니어링 부품에 적합합니다. 출력 시 변형 방지를 위해 팬을 끄는 것을 권장합니다."},
    "ASA 具有优异的户外耐候性，适合户外使用的零件。打印时建议关闭风扇。": {
        "de": "ASA hat hervorragende Wetterbeständigkeit für den Außeneinsatz, geeignet für Bauteile im Freien. Beim Drucken wird empfohlen, den Lüfter auszuschalten.",
        "fr": "L'ASA offre une excellente résistance aux intempéries, idéal pour les pièces d'extérieur. Il est recommandé de couper le ventilateur pendant l'impression.",
        "ja": "ASAは優れた屋外耐候性を持ち、屋外使用の部品に適しています。プリント時はファンをオフにすることを推奨します。",
        "ko": "ASA는 뛰어난 실내외 내후성을 가져 야외용 부품에 적합합니다. 출력 시 팬을 끄는 것을 권장합니다."},
    "Bambu Lab 打印机网络通信支持": {
        "de": "Netzwerkkommunikations-Support für Bambu Lab-Drucker", "fr": "Support de communication réseau pour imprimantes Bambu Lab", "ja": "Bambu Labプリンターのネットワーク通信サポート", "ko": "Bambu Lab 프린터 네트워크 통신 지원"},
    "Bambu 打印机需在设置中启用局域网访问码": {
        "de": "Für Bambu-Drucker muss der LAN-Zugriffscode in den Einstellungen aktiviert werden", "fr": "Les imprimantes Bambu nécessitent l'activation du code d'accès LAN dans les paramètres", "ja": "Bambuプリンターは設定でLANアクセスコードを有効化する必要があります", "ko": "Bambu 프린터는 설정에서 LAN 액세스 코드를 활성화해야 합니다"},
    "Brim（裙边）可以增加模型与热床的附着力，防止翘边。": {
        "de": "Brim (Saum) erhöht die Haftung zwischen Modell und Druckbett und verhindert Verzug.",
        "fr": "Le brim (bordure) augmente l'adhérence entre le modèle et le plateau et évite le délaminage.",
        "ja": "Brim（スカート）はモデルと熱床の密着性を高め、反りを防ぎます。",
        "ko": "Brim(스커트)은 모델과 베드의 접착력을 높여 변형을 방지합니다."},
    "PLA 是最常用的耗材，易于打印，适合初学者。建议打印时开启风扇冷却。": {
        "de": "PLA ist das am häufigsten verwendete Filament, einfach zu drucken und für Anfänger geeignet. Es wird empfohlen, beim Drucken den Lüfter zur Kühlung einzuschalten.",
        "fr": "Le PLA est le filament le plus courant, facile à imprimer et adapté aux débutants. Il est recommandé d'activer le ventilateur pendant l'impression.",
        "ja": "PLAは最も一般的なフィラメントで、プリントしやすく初心者に適しています。プリント時はファン冷却をオンにすることを推奨します。",
        "ko": "PLA는 가장 일반적인 필라멘트로 출력하기 쉽고 초보자에게 적합합니다. 출력 시 팬 냉각을 켜는 것을 권장합니다."},
    "Shift = 点测量 (对齐上游 GLGizmoMeasure)": {
        "de": "Shift = Punktmessung (ausgerichtet am Upstream-GLGizmoMeasure)", "fr": "Shift = Mesure de point (aligné avec le GLGizmoMeasure amont)", "ja": "Shift = 点測定（アップストリーム GLGizmoMeasure に合わせる）", "ko": "Shift = 점 측정 (업스트림 GLGizmoMeasure에 정렬)"},
    "TPU 是柔性耗材，适合打印弹性零件。建议降低打印速度以获得更好质量。": {
        "de": "TPU ist ein flexibles Filament, geeignet für elastische Bauteile. Für bessere Qualität wird eine niedrigere Druckgeschwindigkeit empfohlen.",
        "fr": "Le TPU est un filament flexible, adapté aux pièces élastiques. Une vitesse d'impression réduite est recommandée pour une meilleure qualité.",
        "ja": "TPUは柔軟なフィラメントで、弾性部品のプリントに適しています。より良い品質のためプリント速度を下げることを推奨します。",
        "ko": "TPU는 유연한 필라멘트로 탄성 부품 출력에 적합합니다. 더 나은 품질을 위해 출력 속도를 낮추는 것을 권장합니다."},
    "v%1 更新内容:\n\n1. 优化切片算法，提升打印质量\n2. 修复 AMS 多耗材切换偶尔失败的问题\n3. 新增 timelapse 视频录制优化\n4. 改善 Wi-Fi 连接稳定性\n5. 修复部分情况下热床温度显示异常": {
        "de": "v%1 Änderungen:\n\n1. Slice-Algorithmus optimiert, Druckqualität verbessert\n2. Gelegentliches Fehlschlagen des AMS-Multifilament-Wechsels behoben\n3. Timelapse-Videoaufzeichnung optimiert\n4. Wi-Fi-Verbindungsstabilität verbessert\n5. Anomalie der Druckbetttemperaturanzeige in einigen Fällen behoben",
        "fr": "v%1 Notes de mise à jour :\n\n1. Algorithme de tranchage optimisé, qualité d'impression améliorée\n2. Correction des échecs occasionnels du changement multifilament AMS\n3. Optimisation de l'enregistrement vidéo timelapse\n4. Amélioration de la stabilité Wi-Fi\n5. Correction de l'affichage anormal de la température du plateau dans certains cas",
        "ja": "v%1 更新内容:\n\n1. スライスアルゴリズムを最適化しプリント品質を向上\n2. AMSマルチフィラメント切替の偶発的な失敗を修正\n3. タイムラプス動画録画を最適化\n4. Wi-Fi接続の安定性を改善\n5. 一部の状況での熱床温度表示異常を修正",
        "ko": "v%1 업데이트 내용:\n\n1. 슬라이스 알고리즘 최적화, 출력 품질 향상\n2. AMS 다중 필라멘트 전환 간헐적 실패 수정\n3. 타임랩스 동영상 녹화 최적화\n4. Wi-Fi 연결 안정성 개선\n5. 일부 상황에서 베드 온도 표시 이상 수정"},
    "专业级 3D 打印切片软件": {"de": "Professionelle 3D-Druck-Slice-Software", "fr": "Logiciel de tranchage 3D professionnel", "ja": "プロフェッショナル向け 3D プリントスライスソフトウェア", "ko": "전문가용 3D 출력 슬라이스 소프트웨어"},
    "为对象的不同高度设置不同层高（例如底部精细、上部加速）。": {
        "de": "Unterschiedliche Schichthöhen für verschiedene Höhen des Objekts festlegen (z. B. unten fein, oben beschleunigt).",
        "fr": "Définir différentes hauteurs de couche selon la hauteur de l'objet (ex. fin en bas, accéléré en haut).",
        "ja": "オブジェクトの高さに応じて異なる層の高さを設定します（例：下部は精細、上部は高速）。",
        "ko": "객체의 높이에 따라 다른 레이어 높이를 설정합니다(예: 하단은 정밀, 상단은 가속)."},
    "为本盘选择耗材到喷嘴的映射方式。": {
        "de": "Zuordnung Filament → Düse für diese Druckplatte wählen.", "fr": "Choisir le mappage filament → buse pour cette platine.", "ja": "このプレートのフィラメント→ノズルのマッピング方式を選択します。", "ko": "이 플레이트의 필라멘트→노즐 매핑 방식을 선택합니다."},
    "以下为当前版本支持的快捷键列表。部分快捷键仅在特定页面生效。": {
        "de": "Nachfolgend die Liste der in der aktuellen Version unterstützten Tastenkürzel. Einige funktionieren nur auf bestimmten Seiten.",
        "fr": "Voici la liste des raccourcis clavier pris en charge dans la version actuelle. Certains ne fonctionnent que sur des pages spécifiques.",
        "ja": "以下は現在のバージョンでサポートされるショートカットキーの一覧です。一部のショートカットは特定のページでのみ有効です。",
        "ko": "다음은 현재 버전에서 지원되는 단축키 목록입니다. 일부 단축키는 특정 페이지에서만 작동합니다."},
    "使用显式的每喷嘴耗材映射。": {
        "de": "Explizite Filament-Zuordnung pro Düse verwenden.", "fr": "Utiliser un mappage explicite de filament par buse.", "ja": "ノズルごとに明示的なフィラメントマッピングを使用します。", "ko": "노즐별 명시적 필라멘트 매핑을 사용합니다."},
    "克隆平板失败：可能已达到最大平板数（36）": {
        "de": "Druckplatte duplizieren fehlgeschlagen: max. Plattenanzahl (36) möglicherweise erreicht", "fr": "Échec du clonage de la platine : nombre maximum de platines (36) peut-être atteint", "ja": "プレートの複製に失敗：最大プレート数（36）に達した可能性があります", "ko": "플레이트 복제 실패: 최대 플레이트 수(36)에 도달했을 수 있습니다"},
    "关闭后将不再显示切片进度弹窗，切片完成后仍会通知。": {
        "de": "Nach dem Schließen wird kein Slice-Fortschritts-Popup mehr angezeigt; nach Abschluss wird weiterhin benachrichtigt.",
        "fr": "Après fermeture, la fenêtre de progression du tranchage ne s'affichera plus ; une notification reste envoyée à la fin.",
        "ja": "閉じるとスライス進行のポップアップは表示されなくなりますが、完了時には引き続き通知されます。",
        "ko": "닫으면 슬라이스 진행 팝업이 더 이상 표시되지 않지만, 완료 시에는 여전히 알림이 표시됩니다."},
    "内壁打印速度。可以比外壁更快。": {
        "de": "Innenwand-Druckgeschwindigkeit. Kann schneller sein als die Außenwand.", "fr": "Vitesse d'impression de la paroi interne. Peut être plus rapide que la paroi externe.", "ja": "内壁のプリント速度。外壁より速くできます。", "ko": "내벽 출력 속도. 외벽보다 빠를 수 있습니다."},
    "内置占位符（双击项添加到 G-code）：": {
        "de": "Eingebaute Platzhalter (Doppelklick, um zum G-code hinzuzufügen):", "fr": "Espaces réservés intégrés (double-cliquer pour ajouter au G-code) :", "ja": "組み込みプレースホルダー（ダブルクリックで G-code に追加）:", "ko": "내장 자리 표시자(더블클릭하여 G-code에 추가):"},
    "内部填充使用的几何图案。不同图案影响强度、速度和耗材用量。": {
        "de": "Geometrisches Muster der Innenfüllung. Verschiedene Muster beeinflussen Festigkeit, Geschwindigkeit und Filamentverbrauch.",
        "fr": "Motif géométrique du remplissage interne. Différents motifs affectent la solidité, la vitesse et la consommation de filament.",
        "ja": "内部インフィルに使用する幾何パターン。強度、速度、フィラメント使用量に影響します。",
        "ko": "내부 채움에 사용되는 기하학적 패턴. 패턴에 따라 강도, 속도, 필라멘트 사용량이 영향을 받습니다."},
    "内部桥接速度 (mm/s)": {"de": "Innen-Überbrückungsgeschwindigkeit (mm/s)", "fr": "Vitesse de pont interne (mm/s)", "ja": "内部ブリッジ速度 (mm/s)", "ko": "내부 브리지 속도 (mm/s)"},
    "切片前确保模型已平放在热床上。使用 W/E/R 切换移动/旋转/缩放工具。": {
        "de": "Vor dem Slicen sicherstellen, dass das Modell flach auf dem Druckbett liegt. W/E/R schaltet Bewegungs-/Rotations-/Skalierungswerkzeuge um.",
        "fr": "Avant le tranchage, assurez-vous que le modèle est à plat sur le plateau. Utilisez W/E/R pour basculer entre déplacement/rotation/mise à l'échelle.",
        "ja": "スライス前にモデルが熱床に平らに置かれていることを確認します。W/E/R で移動/回転/スケールツールを切り替えます。",
        "ko": "슬라이스 전 모델이 베드에 평평하게 놓여 있는지 확인하세요. W/E/R로 이동/회전/크기 조정 도구를 전환합니다."},
    "切片已完成，可以预览或导出 G-code": {
        "de": "Slicen abgeschlossen, Vorschau oder G-code-Export möglich", "fr": "Tranchage terminé, vous pouvez prévisualiser ou exporter le G-code", "ja": "スライス完了、プレビューまたは G-code をエクスポートできます", "ko": "슬라이스 완료, 미리보기 또는 G-code 내보내기 가능"},
    "创建失败（名称重复或范围无效）": {
        "de": "Erstellen fehlgeschlagen (Name doppelt oder Bereich ungültig)", "fr": "Échec de la création (nom en double ou plage invalide)", "ja": "作成失敗（名前重複または範囲無効）", "ko": "생성 실패(이름 중복 또는 범위 무효)"},
    "匹配 AMS 已装载耗材（自动推荐）。": {
        "de": "Mit bereits geladenem AMS-Filament abgleichen (automatische Empfehlung).", "fr": "Correspondre au filament AMS déjà chargé (recommandation automatique).", "ja": "AMSに装着済みのフィラメントと一致（自動推奨）。", "ko": "AMS에 장착된 필라멘트와 일치(자동 추천)."},
    "升级失败，请检查网络连接后重试。": {
        "de": "Upgrade fehlgeschlagen, bitte Netzwerkverbindung prüfen und erneut versuchen.", "fr": "Échec de la mise à niveau, vérifiez la connexion réseau puis réessayez.", "ja": "アップグレード失敗、ネットワーク接続を確認して再試行してください。", "ko": "업그레이드 실패, 네트워크 연결을 확인 후 다시 시도하세요."},
    "升级成功！打印机将自动重启。": {
        "de": "Upgrade erfolgreich! Der Drucker startet automatisch neu.", "fr": "Mise à niveau réussie ! L'imprimante va redémarrer automatiquement.", "ja": "アップグレード成功！プリンターは自動的に再起動します。", "ko": "업그레이드 성공! 프린터가 자동으로 재시작됩니다."},
    "升级过程中请勿断开电源或网络连接": {
        "de": "Während des Upgrades Strom- und Netzwerkverbindung nicht trennen", "fr": "Ne pas couper l'alimentation ni la connexion réseau pendant la mise à niveau", "ja": "アップグレード中は電源やネットワーク接続を切断しないでください", "ko": "업그레이드 중 전원이나 네트워크 연결을 끊지 마세요"},
    "启用后，3D 视口将降低渲染细节以提升性能。适合模型较多或硬件性能不足时使用。": {
        "de": "Wenn aktiviert, reduziert der 3D-Viewport die Rendering-Details zur Leistungssteigerung. Geeignet bei vielen Modellen oder schwacher Hardware.",
        "fr": "Si activé, la vue 3D réduit les détails de rendu pour améliorer les performances. Utile avec de nombreux modèles ou du matériel limité.",
        "ja": "有効化すると、3Dビューポートはパフォーマンス向上のためレンダリング詳細を下げます。モデルが多い場合やハードウェア性能が不足する場合に適しています。",
        "ko": "활성화하면 3D 뷰포트가 성능 향상을 위해 렌더링 디테일을 낮춥니다. 모델이 많거나 하드웨어 성능이 부족할 때 적합합니다."},
    "启用后，切片完成后将自动上传 G-code 到连接的打印机（需先在设备页面连接打印机）。": {
        "de": "Wenn aktiviert, wird nach dem Slicen der G-code automatisch auf den verbundenen Drucker hochgeladen (Drucker zuerst auf der Geräteseite verbinden).",
        "fr": "Si activé, le G-code est téléversé automatiquement vers l'imprimante connectée après le tranchage (connectez d'abord l'imprimante sur la page Appareils).",
        "ja": "有効化すると、スライス完了後に接続中のプリンターへ自動的に G-code をアップロードします（先にデバイスページでプリンターを接続してください）。",
        "ko": "활성화하면 슬라이스 완료 후 연결된 프린터로 G-code를 자동 업로드합니다(먼저 장치 페이지에서 프린터를 연결하세요)."},
    "启用后，项目文件将自动备份到您的云端账户。需要先登录云端账号。": {
        "de": "Wenn aktiviert, wird die Projektdatei automatisch in Ihrem Cloud-Konto gesichert. Zuerst beim Cloud-Konto anmelden.",
        "fr": "Si activé, le fichier de projet est sauvegardé automatiquement dans votre compte cloud. Connectez-vous d'abord au compte cloud.",
        "ja": "有効化すると、プロジェクトファイルがクラウドアカウントに自動バックアップされます。先にクラウドアカウントへログインしてください。",
        "ko": "활성화하면 프로젝트 파일이 클라우드 계정에 자동 백업됩니다. 먼저 클라우드 계정에 로그인해야 합니다."},
    "启用回退（retraction）以减少拉丝。": {
        "de": "Rückzug (Retraction) aktivieren, um Fadenziehen zu reduzieren.", "fr": "Activer la rétraction pour réduire les fils.", "ja": "リトラクションを有効化して糸引きを減らします。", "ko": "리트랙션을 활성화하여 잔털을 줄입니다."},
    "启用支撑结构以支撑悬空部分。": {
        "de": "Stützstruktur aktivieren, um Überhänge abzustützen.", "fr": "Activer la structure de support pour soutenir les surplombs.", "ja": "オーバーハングを支えるためにサポート構造を有効化します。", "ko": "현수 부분을 지탱하기 위해 서포트 구조를 활성화합니다."},
    "启用精简模式可隐藏内部填充结构，仅显示关键工具路径，显著提升预览响应速度。建议在内存较低的设备上启用。": {
        "de": "Der Lite-Modus verbirgt die Innenfüllung und zeigt nur wesentliche Werkzeugwege, was die Vorschaugeschwindigkeit deutlich erhöht. Empfohlen auf Geräten mit wenig Speicher.",
        "fr": "Le mode léger masque la structure de remplissage interne et n'affiche que les trajectoires clés, accélérant nettement l'aperçu. Recommandé sur les appareils à faible mémoire.",
        "ja": "ライトモードは内部インフィル構造を非表示にし、主要なツールパスのみを表示してプレビュー応答を大幅に向上させます。メモリの少ないデバイスで有効化を推奨します。",
        "ko": "라이트 모드는 내부 채움 구조를 숨기고 핵심 툴 패스만 표시하여 미리보기 응답 속도를 크게 높입니다. 메모리가 적은 장치에서 활성화를 권장합니다."},
    "在打印机屏幕：设置 &gt; 网络 &gt; 局域网访问码": {
        "de": "Auf dem Drucker-Display: Einstellungen &gt; Netzwerk &gt; LAN-Zugriffscode", "fr": "Sur l'écran de l'imprimante : Paramètres &gt; Réseau &gt; Code d'accès LAN", "ja": "プリンター画面：設定 &gt; ネットワーク &gt; LANアクセスコード", "ko": "프린터 화면: 설정 &gt; 네트워크 &gt; LAN 액세스 코드"},
    "在打印机设置中查看 IP，确认可 ping 通": {
        "de": "IP in den Druckereinstellungen prüfen und per ping erreichbar bestätigen", "fr": "Vérifier l'IP dans les paramètres de l'imprimante et confirmer qu'elle répond au ping", "ja": "プリンター設定で IP を確認し、ping が通るか確認", "ko": "프린터 설정에서 IP를 확인하고 ping이 통하는지 확인"},
    "在模型底部边缘添加 Brim 以增加附着力。": {
        "de": "Brim am unteren Rand des Modells hinzufügen, um die Haftung zu erhöhen.", "fr": "Ajouter un brim au bord inférieur du modèle pour augmenter l'adhérence.", "ja": "モデル下部の縁に Brim を追加して密着性を高めます。", "ko": "모델 하단 가장자리에 Brim을 추가하여 접착력을 높입니다."},
    "在此编辑自定义 G-code...": {
        "de": "Hier benutzerdefinierten G-code bearbeiten...", "fr": "Modifier le G-code personnalisé ici...", "ja": "ここでカスタムG-codeを編集...", "ko": "여기서 사용자 지정 G-code 편집..."},
    "场景中无对象\n请从顶部菜单导入模型": {
        "de": "Keine Objekte in der Szene\nBitte Modell über das obere Menü importieren", "fr": "Aucun objet dans la scène\nVeuillez importer un modèle depuis le menu supérieur", "ja": "シーンにオブジェクトがありません\n上部メニューからモデルをインポートしてください", "ko": "장면에 객체가 없습니다\n상단 메뉴에서 모델을 가져오세요"},
    "基于 AI 模型的切片参数自动优化": {
        "de": "Automatische Slice-Parameter-Optimierung per AI-Modell", "fr": "Optimisation automatique des paramètres de tranchage par modèle IA", "ja": "AIモデルによるスライスパラメータの自動最適化", "ko": "AI 모델 기반 슬라이스 매개변수 자동 최적화"},
    "基于树形结构的智能支撑生成": {
        "de": "Intelligente Stützstruktur-Generierung mit Baumstruktur", "fr": "Génération intelligente de support en arborescence", "ja": "ツリー構造に基づくインテリジェントサポート生成", "ko": "트리 구조 기반 지능형 서포트 생성"},
    "填充密度影响模型强度和重量。20% 适合大多数场景，100% 为实心。": {
        "de": "Die Füllungsdichte beeinflusst Festigkeit und Gewicht des Modells. 20 % passt für die meisten Fälle, 100 % ist massiv.",
        "fr": "La densité de remplissage affecte la solidité et le poids du modèle. 20 % convient à la plupart des cas, 100 % est plein.",
        "ja": "インフィル密度はモデルの強度と重量に影響します。20%はほとんどのケースに適し、100%はソリッドです。",
        "ko": "채움 밀도는 모델의 강도와 무게에 영향을 줍니다. 20%는 대부분의 경우에 적합하고 100%는 솔리드입니다."},
    "外壁打印速度。较低速度获得更好的表面质量。": {
        "de": "Außenwand-Druckgeschwindigkeit. Niedrigere Geschwindigkeit ergibt bessere Oberflächenqualität.", "fr": "Vitesse d'impression de la paroi externe. Une vitesse plus basse offre une meilleure qualité de surface.", "ja": "外壁のプリント速度。低い速度でより良い表面品質が得られます。", "ko": "외벽 출력 속도. 낮은 속도에서 더 나은 표면 품질."},
    "导出全部平板 G-code": {
        "de": "G-code aller Druckplatten exportieren", "fr": "Exporter le G-code de toutes les platines", "ja": "全プレートの G-code をエクスポート", "ko": "모든 플레이트 G-code 내보내기"},
    "将创建新项目，当前未保存的更改将丢失。\n是否继续？": {
        "de": "Es wird ein neues Projekt erstellt, ungespeicherte Änderungen gehen verloren.\nFortfahren?", "fr": "Un nouveau projet sera créé, les modifications non enregistrées seront perdues.\nContinuer ?", "ja": "新しいプロジェクトが作成され、現在の未保存の変更は失われます。\n続行しますか？", "ko": "새 프로젝트가 생성되며, 저장되지 않은 변경 사항은 손실됩니다.\n계속하시겠습니까?"},
    "将当前所有自定义预设导出为可分享的预设包文件。": {
        "de": "Alle aktuellen benutzerdefinierten Voreinstellungen als teilbares Voreinstellungspaket exportieren.", "fr": "Exporter tous les préréglages personnalisés actuels dans un pack partageable.", "ja": "現在のすべてのカスタムプリセットを共有可能なプリセットパックファイルとしてエクスポートします。", "ko": "현재의 모든 사용자 지정 사전 설정을 공유 가능한 패키지 파일로 내보냅니다."},
    "将文字贴附到模型表面（实验性，完整效果将在后续版本提供）": {
        "de": "Text an die Modeloberfläche anbringen (experimentell, volle Funktion in späteren Versionen)", "fr": "Fixer le texte à la surface du modèle (expérimental, fonction complète dans les versions ultérieures)", "ja": "モデル表面にテキストを貼り付けます（実験的、完全な効果は今後のバージョンで提供）", "ko": "모델 표면에 텍스트 부착 (실험적, 전체 효과는 향후 버전에서 제공)"},
    "将选中的占位符添加到 G-code": {
        "de": "Ausgewählten Platzhalter zum G-code hinzufügen", "fr": "Ajouter l'espace réservé sélectionné au G-code", "ja": "選択したプレースホルダーを G-code に追加", "ko": "선택한 자리 표시자를 G-code에 추가"},
    "尚未获得校准结果，请先完成校准步骤": {
        "de": "Noch keine Kalibrierungsergebnisse, bitte zuerst die Kalibrierungsschritte abschließen", "fr": "Pas encore de résultats d'étalonnage, veuillez d'abord terminer les étapes d'étalonnage", "ja": "まだキャリブレーション結果がありません、先にキャリブレーション手順を完了してください", "ko": "아직 캘리브레이션 결과가 없습니다, 먼저 캘리브레이션 단계를 완료하세요"},
    "层范围序列（从第 2 层起，自动排序）": {
        "de": "Schichtbereichs-Sequenz (ab Schicht 2, automatisch sortiert)", "fr": "Séquence des plages de couches (à partir de la couche 2, tri automatique)", "ja": "層範囲シーケンス（第2層から、自動ソート）", "ko": "레이어 범위 시퀀스(2번째 레이어부터, 자동 정렬)"},
    "层高越小打印越精细，但耗时越长。常用范围: 0.1mm - 0.3mm。": {
        "de": "Je geringer die Schichthöhe, desto feiner der Druck, aber desto länger die Dauer. Üblicher Bereich: 0.1 mm - 0.3 mm.",
        "fr": "Plus la hauteur de couche est faible, plus l'impression est fine, mais plus la durée augmente. Plage courante : 0.1 mm - 0.3 mm.",
        "ja": "層の高さが小さいほどプリントは精細になりますが、時間がかかります。一般的な範囲: 0.1mm - 0.3mm。",
        "ko": "레이어 높이가 작을수록 출력이 정밀해지지만 시간이 길어집니다. 일반적 범위: 0.1mm - 0.3mm."},
    "当前为 Mock 模式，更新检查功能需要连接更新服务器后启用。": {
        "de": "Aktuell im Mock-Modus, die Update-Prüfung wird nach Verbindung mit dem Update-Server aktiviert.", "fr": "Actuellement en mode Mock, la vérification des mises à jour s'active après connexion au serveur de mise à jour.", "ja": "現在モックモードです。更新チェック機能は更新サーバーに接続すると有効化されます。", "ko": "현재 모의 모드입니다. 업데이트 확인 기능은 업데이트 서버에 연결되면 활성화됩니다."},
    "当前层号 (从 1 开始)": {"de": "Aktuelle Schichtnummer (ab 1)", "fr": "Numéro de couche actuel (à partir de 1)", "ja": "現在の層番号 (1から開始)", "ko": "현재 레이어 번호 (1부터 시작)"},
    "当前打印 Z 高度 (mm)": {"de": "Aktuelle Druck-Z-Höhe (mm)", "fr": "Hauteur Z d'impression actuelle (mm)", "ja": "現在のプリント Z 高さ (mm)", "ko": "현재 출력 Z 높이 (mm)"},
    "当前预设已修改但未保存。切换前请选择如何处理这些修改：": {
        "de": "Die aktuelle Voreinstellung wurde geändert, aber nicht gespeichert. Vor dem Wechseln bitte wählen, wie mit den Änderungen verfahren werden soll:",
        "fr": "Le préréglage actuel a été modifié mais non enregistré. Avant de changer, choisissez comment traiter ces modifications :",
        "ja": "現在のプリセットは変更されましたが保存されていません。切り替え前にこれらの変更の扱いを選択してください:",
        "ko": "현재 사전 설정이 수정되었지만 저장되지 않았습니다. 전환 전에 이 변경 사항을 어떻게 처리할지 선택하세요:"},
    "您的基本配置已保存，可以开始使用了。": {
        "de": "Ihre Grundeinstellungen wurden gespeichert, Sie können loslegen.", "fr": "Votre configuration de base a été enregistrée, vous pouvez commencer.", "ja": "基本設定が保存されました、ご利用いただけます。", "ko": "기본 설정이 저장되었으며, 사용을 시작할 수 있습니다."},
    "悬空角度超过 45° 的部分需要支撑。合理使用支撑可以提升打印质量。": {
        "de": "Überhänge über 45° benötigen Stützstruktur. Sinnvoller Einsatz von Stützstrukturen verbessert die Druckqualität.",
        "fr": "Les surplombs de plus de 45° nécessitent un support. Un usage judicieux des supports améliore la qualité d'impression.",
        "ja": "45°を超えるオーバーハングにはサポートが必要です。サポートを適切に使うことでプリント品質が向上します。",
        "ko": "45°를 초과하는 현수 부분에는 서포트가 필요합니다. 서포트를 합리적으로 사용하면 출력 품질이 향상됩니다."},
    "我们将引导您完成基本配置，选择您的打印机和耗材，以获得最佳切片体验。": {
        "de": "Wir führen Sie durch die Grundeinrichtung, wählen Sie Ihren Drucker und Ihr Filament, um das beste Slice-Erlebnis zu erhalten.",
        "fr": "Nous vous guiderons à travers la configuration de base, choisissez votre imprimante et votre filament pour une expérience de tranchage optimale.",
        "ja": "基本設定をガイドします。プリンターとフィラメントを選択して、最適なスライス体験を得ましょう。",
        "ko": "기본 설정을 안내해 드립니다. 프린터와 필라멘트를 선택하여 최적의 슬라이스 경험을 얻으세요."},
    "所有打印移动的最大速度限制。": {
        "de": "Maximale Geschwindigkeitsbegrenzung für alle Druckbewegungen.", "fr": "Limite de vitesse maximale pour tous les déplacements d'impression.", "ja": "すべてのプリント移動の最大速度制限。", "ko": "모든 출력 이동의 최대 속도 제한."},
    "打印机连接已断开，请重试或检查网络": {
        "de": "Druckerverbindung getrennt, bitte erneut versuchen oder Netzwerk prüfen", "fr": "Connexion de l'imprimante interrompue, veuillez réessayer ou vérifier le réseau", "ja": "プリンター接続が切断されました、再試行またはネットワークを確認してください", "ko": "프린터 연결이 끊어졌습니다, 다시 시도하거나 네트워크를 확인하세요"},
    "打印速度越快效率越高，但可能影响表面质量。建议先慢后快测试。": {
        "de": "Höhere Druckgeschwindigkeit erhöht die Effizienz, kann aber die Oberflächenqualität beeinflussen. Empfohlen: erst langsam, dann schneller testen.",
        "fr": "Une vitesse d'impression plus élevée augmente l'efficacité mais peut affecter la qualité de surface. Recommandé : tester d'abord lentement puis plus vite.",
        "ja": "プリント速度が速いほど効率は上がりますが、表面品質に影響する可能性があります。まず遅く、次に速くテストすることを推奨します。",
        "ko": "출력 속도가 빠를수록 효율은 높아지지만 표면 품질에 영향을 줄 수 있습니다. 먼저 느리게, 그다음 빠르게 테스트를 권장합니다."},
    "挤出线的宽度。默认自动匹配喷嘴直径。": {
        "de": "Breite der extrudierten Linie. Standardmäßig automatisch an den Düsendurchmesser angepasst.", "fr": "Largeur de la ligne extrudée. Par défaut, s'adapte automatiquement au diamètre de la buse.", "ja": "押し出し線の幅。デフォルトでノズル径に自動的に一致します。", "ko": "압출 선의 너비. 기본적으로 노즐 지름에 자동으로 맞춰집니다."},
    "搜索 G-code 占位符...": {"de": "G-code-Platzhalter suchen...", "fr": "Rechercher des espaces réservés G-code...", "ja": "G-code プレースホルダーを検索...", "ko": "G-code 자리 표시자 검색..."},
    "支撑类型：普通/树状/可溶性。树状支撑更省材料。": {
        "de": "Stützstrukturtyp: Normal/Baum/Löslich. Baumstützen sparen Material.", "fr": "Type de support : normal/arborescent/soluble. Les supports arborescents économisent du matériau.", "ja": "サポートタイプ：通常/ツリー/可溶性。ツリーサポートはより材料を節約します。", "ko": "서포트 유형: 일반/트리/수용성. 트리 서포트가 재료를 더 절약합니다."},
    "日志文件达到指定大小后将自动轮转。增大此值可保留更多历史日志，但占用更多磁盘空间。": {
        "de": "Protokolldateien werden ab der angegebenen Größe automatisch rotiert. Ein größerer Wert bewahrt mehr Verlauf auf, benötigt aber mehr Speicherplatz.",
        "fr": "Les fichiers journaux sont automatiquement rotatés à la taille indiquée. Une valeur plus grande conserve plus d'historique mais consomme plus d'espace disque.",
        "ja": "ログファイルは指定サイズに達すると自動的にローテーションします。この値を大きくするとより多くの履歴ログを保持できますが、ディスク容量をより消費します。",
        "ko": "로그 파일이 지정된 크기에 도달하면 자동으로 순환됩니다. 이 값을 늘리면 더 많은 기록 로그를 보존할 수 있지만 디스크 공간을 더 사용합니다."},
    "暂无层高范围（使用全局层高）": {
        "de": "Keine Schichthöhenbereiche (globale Schichthöhe wird verwendet)", "fr": "Aucune plage de hauteurs de couche (hauteur globale utilisée)", "ja": "層の高さの範囲なし（グローバルの層の高さを使用）", "ko": "레이어 높이 범위 없음 (전역 레이어 높이 사용)"},
    "最大打印速度 (mm/s)": {"de": "Maximale Druckgeschwindigkeit (mm/s)", "fr": "Vitesse d'impression maximale (mm/s)", "ja": "最大プリント速度 (mm/s)", "ko": "최대 출력 속도 (mm/s)"},
    "最小化冲刷量（自动推荐）。": {
        "de": "Spülmenge minimieren (automatische Empfehlung).", "fr": "Minimiser le volume de nettoyage (recommandation automatique).", "ja": "パージ量を最小化（自動推奨）。", "ko": "세척량 최소화(자동 추천)."},
    "本软件基于 Qt 6 框架构建，遵循 GNU LGPL v3 协议。使用本软件即代表您同意相关使用条款。": {
        "de": "Diese Software basiert auf dem Qt 6-Framework und steht unter der GNU LGPL v3. Durch die Nutzung stimmen Sie den Nutzungsbedingungen zu.",
        "fr": "Ce logiciel est basé sur le framework Qt 6 et est publié sous licence GNU LGPL v3. L'utilisation de ce logiciel implique l'acceptation des conditions d'utilisation.",
        "ja": "本ソフトウェアは Qt 6 フレームワークに基づき、GNU LGPL v3 ライセンスに従います。本ソフトウェアを使用することで関連利用規約に同意したことになります。",
        "ko": "이 소프트웨어는 Qt 6 프레임워크를 기반으로 하며 GNU LGPL v3 라이선스를 따릅니다. 이 소프트웨어를 사용하면 관련 사용 약관에 동의하는 것으로 간주됩니다."},
    "校准进行中，请勿移动打印机…": {
        "de": "Kalibrierung läuft, Drucker nicht bewegen…", "fr": "Étalonnage en cours, ne pas déplacer l'imprimante…", "ja": "キャリブレーション中、プリンターを動かさないでください…", "ko": "캘리브레이션 진행 중, 프린터를 이동하지 마세요…"},
    "格式: .zip（含 print/filament/printer 自定义预设）": {
        "de": "Format: .zip (enthält print/filament/printer-Benutzervoreinstellungen)", "fr": "Format : .zip (contient les préréglages personnalisés print/filament/printer)", "ja": "形式: .zip（print/filament/printer のカスタムプリセットを含む）", "ko": "형식: .zip(print/filament/printer 사용자 지정 사전 설정 포함)"},
    "模型内部填充的密度百分比。0% = 空心，100% = 实心。": {
        "de": "Prozentuale Dichte der Innenfüllung des Modells. 0 % = hohl, 100 % = massiv.", "fr": "Pourcentage de densité du remplissage interne. 0 % = vide, 100 % = plein.", "ja": "モデル内部インフィルの密度の割合。0% = 中空、100% = ソリッド。", "ko": "모델 내부 채움의 밀도 비율. 0% = 빈 공간, 100% = 솔리드."},
    "模型外壁的打印圈数。更多圈数增加壁厚和强度。": {
        "de": "Anzahl der Druckringe der Außenwand. Mehr Ringe erhöhen Wanddicke und Festigkeit.", "fr": "Nombre de boucles de la paroi externe. Plus de boucles augmentent l'épaisseur et la solidité.", "ja": "モデル外壁のプリント巻数。巻数を増やすと壁厚と強度が上がります。", "ko": "모델 외벽의 출력 루프 수. 루프가 많을수록 벽 두께와 강도가 증가합니다."},
    "模型最大 Z 高度 (mm)": {"de": "Maximale Z-Höhe des Modells (mm)", "fr": "Hauteur Z maximale du modèle (mm)", "ja": "モデルの最大 Z 高さ (mm)", "ko": "모델 최대 Z 높이 (mm)"},
    "欢迎使用 3D 打印切片软件": {"de": "Willkommen zur 3D-Druck-Slice-Software", "fr": "Bienvenue dans le logiciel de tranchage 3D", "ja": "3D プリントスライスソフトウェアへようこそ", "ko": "3D 출력 슬라이스 소프트웨어에 오신 것을 환영합니다"},
    "正在与打印机建立连接，请稍候": {"de": "Verbindung zum Drucker wird hergestellt, bitte warten", "fr": "Connexion à l'imprimante en cours, veuillez patienter", "ja": "プリンターと接続を確立しています、お待ちください", "ko": "프린터와 연결을 설정 중입니다, 잠시만 기다려주세요"},
    "正在导出 G-code...": {"de": "G-code wird exportiert...", "fr": "Exportation du G-code...", "ja": "G-code をエクスポート中...", "ko": "G-code 내보내는 중..."},
    "正在自动排列... %1%": {"de": "Automatisches Anordnen... %1%", "fr": "Disposition automatique... %1%", "ja": "自動配置中... %1%", "ko": "자동 정렬 중... %1%"},
    "每层的打印高度。较小的值提高表面质量但增加打印时间。": {
        "de": "Druckhöhe jeder Schicht. Kleinere Werte verbessern die Oberflächenqualität, erhöhen aber die Druckzeit.",
        "fr": "Hauteur d'impression de chaque couche. Une valeur plus petite améliore la qualité de surface mais augmente le temps d'impression.",
        "ja": "各層のプリント高さ。値を小さくすると表面品質は向上しますがプリント時間が増加します。",
        "ko": "각 레이어의 출력 높이. 값이 작을수록 표면 품질은 향상되지만 출력 시간이 늘어납니다."},
    "注：完整 SLA 切片待 v5.1+": {
        "de": "Hinweis: vollständiges SLA-Slicen kommt mit v5.1+", "fr": "Note : tranchage SLA complet à venir en v5.1+", "ja": "注：完全な SLA スライスは v5.1+ で対応", "ko": "참고: 완전한 SLA 슬라이스는 v5.1+에서 지원"},
    "测试与打印机的网络连接性：": {"de": "Netzwerkverbindung zum Drucker testen:", "fr": "Tester la connectivité réseau avec l'imprimante :", "ja": "プリンターとのネットワーク接続性をテスト:", "ko": "프린터와의 네트워크 연결 테스트:"},
    "点测量模式 — 显示选中对象尺寸": {"de": "Punktmessmodus — zeigt die Abmessungen des ausgewählten Objekts", "fr": "Mode mesure de point — affiche les dimensions de l'objet sélectionné", "ja": "点測定モード — 選択したオブジェクトのサイズを表示", "ko": "점 측정 모드 — 선택한 객체의 크기 표시"},
    "热床温度。确保首层良好附着。": {"de": "Druckbetttemperatur. Sichert gute Haftung der ersten Schicht.", "fr": "Température du plateau. Assure une bonne adhérence de la première couche.", "ja": "熱床温度。最初の層の密着を確保します。", "ko": "베드 온도. 첫 레이어의 접착을 보장합니다."},
    "熨烫类型：在打印后用热喷嘴熨平表面。": {
        "de": "Glättungstyp: Glättet die Oberfläche nach dem Drucken mit der heißen Düse.", "fr": "Type de lissage : lisse la surface après impression avec la buse chaude.", "ja": "アイロンタイプ：プリント後に熱したノズルで表面を平滑にします。", "ko": "다림질 유형: 출력 후 뜨거운 노즐로 표면을 평평하게 합니다."},
    "确保打印机和电脑在同一局域网": {"de": "Sicherstellen, dass Drucker und PC im selben LAN sind", "fr": "Assurez-vous que l'imprimante et le PC sont sur le même LAN", "ja": "プリンターとパソコンが同じ LAN 上にあることを確認", "ko": "프린터와 컴퓨터가 같은 LAN에 있는지 확인"},
    "确保打印机已开机且启动完成": {"de": "Sicherstellen, dass der Drucker eingeschaltet und hochgefahren ist", "fr": "Assurez-vous que l'imprimante est allumée et a fini de démarrer", "ja": "プリンターの電源が入り起動が完了していることを確認", "ko": "프린터 전원이 켜져 있고 부팅이 완료되었는지 확인"},
    "确定要删除选中的对象吗？可通过撤销（Ctrl+Z）恢复。": {
        "de": "Ausgewählte Objekte wirklich löschen? Können über Rückgängig (Strg+Z) wiederhergestellt werden.", "fr": "Supprimer les objets sélectionnés ? Récupérables via Annuler (Ctrl+Z).", "ja": "選択したオブジェクトを削除しますか？元に戻す（Ctrl+Z）で復元できます。", "ko": "선택한 객체를 삭제하시겠습니까? 실행 취소(Ctrl+Z)로 복원할 수 있습니다."},
    "确定要断开与该设备的连接吗？断开后可通过设备列表重新连接。": {
        "de": "Verbindung zu diesem Gerät wirklich trennen? Nach dem Trennen über die Geräteliste erneut verbindbar.", "fr": "Déconnecter cet appareil ? Reconnectable via la liste des appareils après déconnexion.", "ja": "このデバイスとの接続を切断しますか？切断後はデバイスリストから再接続できます。", "ko": "이 장치와의 연결을 끊으시겠습니까? 끊은 후 장치 목록에서 다시 연결할 수 있습니다."},
    "确定要清空所有校准历史记录吗？此操作不可撤销。": {
        "de": "Wirklich den gesamten Kalibrierungsverlauf leeren? Diese Aktion kann nicht rückgängig gemacht werden.", "fr": "Vider tout l'historique d'étalonnage ? Action irréversible.", "ja": "すべてのキャリブレーション履歴をクリアしますか？この操作は取り消せません。", "ko": "모든 캘리브레이션 기록을 비우시겠습니까? 이 작업은 되돌릴 수 없습니다."},
    "确定要解绑该云设备吗？解绑后将不再显示该设备的状态。": {
        "de": "Dieses Cloud-Gerät wirklich trennen? Nach dem Trennen wird der Status dieses Geräts nicht mehr angezeigt.", "fr": "Délier cet appareil cloud ? Son état ne s'affichera plus après.", "ja": "このクラウドデバイスのバインドを解除しますか？解除後はこのデバイスの状態は表示されません。", "ko": "이 클라우드 장치의 바인딩을 해제하시겠습니까? 해제 후 이 장치의 상태는 더 이상 표시되지 않습니다."},
    "编辑扫描范围（起始 / 结束 / 步长），覆盖默认值": {
        "de": "Scan-Bereich bearbeiten (Start / Ende / Schrittweite), Standardwerte überschreiben", "fr": "Modifier la plage de balayage (début / fin / pas), remplacer les valeurs par défaut", "ja": "スキャン範囲を編集（開始 / 終了 / ステップ）、デフォルト値を上書き", "ko": "스캔 범위 편집(시작 / 끝 / 스텝), 기본값 재정의"},
    "自动推荐映射（模式 %1）": {"de": "Automatisch empfohlene Zuordnung (Modus %1)", "fr": "Mappage recommandé automatiquement (mode %1)", "ja": "自動推奨マッピング（モード %1）", "ko": "자동 추천 매핑(모드 %1)"},
    "视频流传输中，等待首帧...": {"de": "Videostream wird übertragen, warte auf erstes Bild...", "fr": "Flux vidéo en transmission, en attente de la première image...", "ja": "動画ストリーム転送中、最初のフレームを待機中...", "ko": "비디오 스트림 전송 중, 첫 프레임 대기 중..."},
    "设备屏幕上显示的 PIN 码": {"de": "Die auf dem Gerätebildschirm angezeigte PIN", "fr": "Le code PIN affiché sur l'écran de l'appareil", "ja": "デバイス画面に表示される PIN コード", "ko": "장치 화면에 표시되는 PIN 코드"},
    "设备连接问题排查（按顺序检查）：": {"de": "Fehlerbehebung der Geräteverbindung (der Reihe nach prüfen):", "fr": "Dépannage de la connexion de l'appareil (à vérifier dans l'ordre) :", "ja": "デバイス接続のトラブルシューティング（順番に確認）:", "ko": "장치 연결 문제 해결(순서대로 확인):"},
    "设置撤销/重做的历史记录上限。值越大可回退的操作越多，但占用更多内存。": {
        "de": "Maximale Rückgängig/Wiederholen-Historie setzen. Ein größerer Wert erlaubt mehr Schritte zurück, benötigt aber mehr Speicher.",
        "fr": "Définir la limite de l'historique annuler/refaire. Une valeur plus grande permet plus de retours en arrière mais consomme plus de mémoire.",
        "ja": "元に戻す/やり直し履歴の上限を設定します。値が大きいほど多くの操作を戻せますが、より多くのメモリを消費します。",
        "ko": "실행 취소/다시 실행 기록 한도를 설정합니다. 값이 클수록 더 많은 작업을 되돌릴 수 있지만 더 많은 메모리를 사용합니다."},
    "请先切片或载入 G-code": {"de": "Bitte zuerst slicen oder G-code laden", "fr": "Veuillez d'abord trancher ou charger un G-code", "ja": "先にスライスまたは G-code を読み込んでください", "ko": "먼저 슬라이스하거나 G-code를 로드하세요"},
    "请将爆炸比例重置为 1.00 后再使用测量": {"de": "Bitte Explosionsverhältnis auf 1.00 zurücksetzen, bevor gemessen wird", "fr": "Veuillez réinitialiser le ratio d'explosion à 1.00 avant d'utiliser la mesure", "ja": "測定を使用する前に展開比率を 1.00 にリセットしてください", "ko": "측정을 사용하기 전에 분해 비율을 1.00으로 초기화하세요"},
    "请添加打印机或确保打印机已连接到同一局域网": {"de": "Bitte Drucker hinzufügen oder sicherstellen, dass der Drucker im selben LAN ist", "fr": "Veuillez ajouter une imprimante ou vous assurer qu'elle est sur le même LAN", "ja": "プリンターを追加するか、プリンターが同じ LAN に接続されていることを確認してください", "ko": "프린터를 추가하거나 프린터가 같은 LAN에 연결되어 있는지 확인하세요"},
    "请选择您使用的打印机型号：": {"de": "Bitte Ihr Druckermodell auswählen:", "fr": "Veuillez sélectionner votre modèle d'imprimante :", "ja": "ご使用のプリンター型番を選択してください:", "ko": "사용 중인 프린터 모델을 선택하세요:"},
    "输入局域网访问码以建立 MQTT 连接": {"de": "LAN-Zugriffscode eingeben, um eine MQTT-Verbindung herzustellen", "fr": "Saisir le code d'accès LAN pour établir une connexion MQTT", "ja": "LANアクセスコードを入力して MQTT 接続を確立", "ko": "LAN 액세스 코드를 입력하여 MQTT 연결 설정"},
    "这些选项面向开发者调试使用，普通用户无需更改。": {
        "de": "Diese Optionen sind für Entwickler-Debugging gedacht; normale Benutzer müssen sie nicht ändern.", "fr": "Ces options sont destinées au débogage des développeurs ; les utilisateurs normaux n'ont pas besoin de les modifier.", "ja": "これらのオプションは開発者向けのデバッグ用であり、一般ユーザーは変更する必要はありません。", "ko": "이 옵션은 개발자 디버깅용이며 일반 사용자는 변경할 필요가 없습니다."},
    "选择目标打印机发送 G-code：": {"de": "Zieldrucker zum Senden des G-code wählen:", "fr": "Sélectionner l'imprimante cible pour envoyer le G-code :", "ja": "G-code を送信する対象プリンターを選択:", "ko": "G-code를 보낼 대상 프린터 선택:"},
    "配置打印机默认参数。此处设置将作为新项目的初始值。": {
        "de": "Standardparameter des Druckers konfigurieren. Diese Einstellungen sind der Anfangswert für neue Projekte.",
        "fr": "Configurer les paramètres par défaut de l'imprimante. Ces paramètres seront la valeur initiale des nouveaux projets.",
        "ja": "プリンターのデフォルトパラメータを設定します。ここでの設定は新しいプロジェクトの初期値になります。",
        "ko": "프린터 기본 매개변수를 설정합니다. 이 설정은 새 프로젝트의 초기값이 됩니다."},
    "配置通知的显示方式和自动消失行为。通知将在切片完成、导出等操作时弹出。": {
        "de": "Anzeige und Auto-Ausblendeverhalten von Benachrichtigungen konfigurieren. Benachrichtigungen erscheinen beim Slicen-Abschluss, Export usw.",
        "fr": "Configurer l'affichage et le masquage automatique des notifications. Elles apparaissent à la fin du tranchage, à l'export, etc.",
        "ja": "通知の表示方法と自動非表示動作を設定します。通知はスライス完了やエクスポートなどで表示されます。",
        "ko": "알림 표시 방식과 자동 숨김 동작을 설정합니다. 알림은 슬라이스 완료, 내보내기 등의 작업 시 표시됩니다."},
    "附着力类型：裙边/Brim/底筏。不同的方式确保模型附着在热床上。": {
        "de": "Haftkraft-Typ: Skirt/Brim/Raft. Unterschiedliche Methoden sichern die Haftung des Modells auf dem Druckbett.",
        "fr": "Type d'adhérence : jupe/brim/radeau. Différentes méthodes assurent l'adhérence du modèle au plateau.",
        "ja": "密着タイプ：スカート/Brim/ラフト。異なる方式でモデルが熱床に密着するようにします。",
        "ko": "접착 유형: 스커트/Brim/래프트. 다양한 방식으로 모델이 베드에 접착되도록 합니다."},
    "顶面打印速度。较低速度获得更平滑的顶面。": {
        "de": "Druckgeschwindigkeit der Oberseite. Niedrigere Geschwindigkeit ergibt eine glattere Oberseite.", "fr": "Vitesse d'impression du dessus. Une vitesse plus basse donne un dessus plus lisse.", "ja": "上面のプリント速度。低い速度でより滑らかな上面が得られます。", "ko": "상면 출력 속도. 낮은 속도에서 더 매끄러운 상면."},
    "首层填充速度 (mm/s)": {"de": "Füllungsgeschwindigkeit der ersten Schicht (mm/s)", "fr": "Vitesse de remplissage de la première couche (mm/s)", "ja": "最初の層のインフィル速度 (mm/s)", "ko": "첫 레이어 채움 속도 (mm/s)"},
    "首层打印速度。较低速度确保良好的热床附着力。": {
        "de": "Druckgeschwindigkeit der ersten Schicht. Niedrigere Geschwindigkeit sichert gute Druckbetthaftung.", "fr": "Vitesse d'impression de la première couche. Une vitesse plus basse assure une bonne adhérence au plateau.", "ja": "最初の層のプリント速度。低い速度で熱床への密着を確保します。", "ko": "첫 레이어 출력 속도. 낮은 속도에서 베드 접착력을 보장합니다."},
    "首层的打印高度，通常比标准层高更高以确保附着力。": {
        "de": "Druckhöhe der ersten Schicht, üblicherweise höher als die Standard-Schichthöhe zur Sicherung der Haftung.",
        "fr": "Hauteur d'impression de la première couche, généralement supérieure à la hauteur standard pour assurer l'adhérence.",
        "ja": "最初の層のプリント高さ。通常は密着を確保するため標準の層の高さより高くします。",
        "ko": "첫 레이어의 출력 높이. 일반적으로 접착력 확보를 위해 표준 레이어 높이보다 높습니다."},
    "默认加速度 (mm/s²)": {"de": "Standard-Beschleunigung (mm/s²)", "fr": "Accélération par défaut (mm/s²)", "ja": "デフォルト加速度 (mm/s²)", "ko": "기본 가속도 (mm/s²)"},
    "停止延时": {"de": "Stop-Verzögerung", "fr": "Délai d'arrêt", "ja": "停止遅延", "ko": "정지 지연"},
    "停止录像": {"de": "Aufnahme stoppen", "fr": "Arrêter l'enregistrement", "ja": "録画を停止", "ko": "녹화 중지"},
}
