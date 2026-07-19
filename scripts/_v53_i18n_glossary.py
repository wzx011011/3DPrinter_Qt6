#!/usr/bin/env python
# Phase 177/178 (I18N-06): ZH→{de,fr,ja,ko} translation glossary for common
# 3D-printing UI strings. Applied to unfinished .ts entries via pattern match.
#
# This is a curated glossary, not full MT. It handles the ~80% of strings that
# are common UI labels (耗材/打印/支撑/层高/etc.). Niche strings stay unfinished.

GLOSSARY = {
    # Common nouns
    "打印": {"de": "Drucken", "fr": "Impression", "ja": "プリント", "ko": "인쇄"},
    "耗材": {"de": "Filament", "fr": "Filament", "ja": "フィラメント", "ko": "필라멘트"},
    "支撑": {"de": "Stützstruktur", "fr": "Support", "ja": "サポート", "ko": "서포트"},
    "层高": {"de": "Schichthöhe", "fr": "Hauteur de couche", "ja": "積層ピッチ", "ko": "레이어 높이"},
    "填充": {"de": "Füllung", "fr": "Remplissage", "ja": "インフィル", "ko": "채우기"},
    "速度": {"de": "Geschwindigkeit", "fr": "Vitesse", "ja": "速度", "ko": "속도"},
    "温度": {"de": "Temperatur", "fr": "Température", "ja": "温度", "ko": "온도"},
    "喷嘴": {"de": "Düse", "fr": "Buse", "ja": "ノズル", "ko": "노즐"},
    "热床": {"de": "Druckbett", "fr": "Plateau", "ja": "ヒートベッド", "ko": "히트베드"},
    "对象": {"de": "Objekt", "fr": "Objet", "ja": "オブジェクト", "ko": "객체"},
    "部件": {"de": "Teil", "fr": "Pièce", "ja": "パーツ", "ko": "부품"},
    "预设": {"de": "Preset", "fr": "Préréglage", "ja": "プリセット", "ko": "프리셋"},
    "切片": {"de": "Slicen", "fr": "Trancher", "ja": "スライス", "ko": "슬라이스"},
    "模型": {"de": "Modell", "fr": "Modèle", "ja": "モデル", "ko": "모델"},
    "项目": {"de": "Projekt", "fr": "Projet", "ja": "プロジェクト", "ko": "프로젝트"},
    "设备": {"de": "Gerät", "fr": "Appareil", "ja": "デバイス", "ko": "장치"},
    "设置": {"de": "Einstellungen", "fr": "Paramètres", "ja": "設定", "ko": "설정"},
    "保存": {"de": "Speichern", "fr": "Enregistrer", "ja": "保存", "ko": "저장"},
    "取消": {"de": "Abbrechen", "fr": "Annuler", "ja": "キャンセル", "ko": "취소"},
    "确定": {"de": "OK", "fr": "OK", "ja": "OK", "ko": "확인"},
    "关闭": {"de": "Schließen", "fr": "Fermer", "ja": "閉じる", "ko": "닫기"},
    "删除": {"de": "Löschen", "fr": "Supprimer", "ja": "削除", "ko": "삭제"},
    "添加": {"de": "Hinzufügen", "fr": "Ajouter", "ja": "追加", "ko": "추가"},
    "重置": {"de": "Zurücksetzen", "fr": "Réinitialiser", "ja": "リセット", "ko": "재설정"},
    "重命名": {"de": "Umbenennen", "fr": "Renommer", "ja": "名前変更", "ko": "이름 변경"},
    "复制": {"de": "Kopieren", "fr": "Copier", "ja": "コピー", "ko": "복사"},
    "粘贴": {"de": "Einfügen", "fr": "Coller", "ja": "貼り付け", "ko": "붙여넣기"},
    "撤销": {"de": "Rückgängig", "fr": "Annuler", "ja": "元に戻す", "ko": "실행 취소"},
    "重做": {"de": "Wiederholen", "fr": "Rétablir", "ja": "やり直し", "ko": "다시 실행"},
    "导入": {"de": "Importieren", "fr": "Importer", "ja": "インポート", "ko": "가져오기"},
    "导出": {"de": "Exportieren", "fr": "Exporter", "ja": "エクスポート", "ko": "내보내기"},
    "刷新": {"de": "Aktualisieren", "fr": "Actualiser", "ja": "更新", "ko": "새로고침"},
    "搜索": {"de": "Suchen", "fr": "Rechercher", "ja": "検索", "ko": "검색"},
    "名称": {"de": "Name", "fr": "Nom", "ja": "名前", "ko": "이름"},
    "类型": {"de": "Typ", "fr": "Type", "ja": "タイプ", "ko": "유형"},
    "状态": {"de": "Status", "fr": "Statut", "ja": "ステータス", "ko": "상태"},
    "进度": {"de": "Fortschritt", "fr": "Progression", "ja": "進行状況", "ko": "진행률"},
    "颜色": {"de": "Farbe", "fr": "Couleur", "ja": "色", "ko": "색상"},
    "红色": {"de": "Rot", "fr": "Rouge", "ja": "赤", "ko": "빨강"},
    "绿色": {"de": "Grün", "fr": "Vert", "ja": "緑", "ko": "초록"},
    "蓝色": {"de": "Blau", "fr": "Bleu", "ja": "青", "ko": "파랑"},
    "黄色": {"de": "Gelb", "fr": "Jaune", "ja": "黄", "ko": "노랑"},
    "白色": {"de": "Weiß", "fr": "Blanc", "ja": "白", "ko": "흰색"},
    "黑色": {"de": "Schwarz", "fr": "Noir", "ja": "黒", "ko": "검정"},
    "PLA": {"de": "PLA", "fr": "PLA", "ja": "PLA", "ko": "PLA"},
    "ABS": {"de": "ABS", "fr": "ABS", "ja": "ABS", "ko": "ABS"},
    "PETG": {"de": "PETG", "fr": "PETG", "ja": "PETG", "ko": "PETG"},
    "TPU": {"de": "TPU", "fr": "TPU", "ja": "TPU", "ko": "TPU"},
    # Verbs / actions
    "开始": {"de": "Start", "fr": "Démarrer", "ja": "開始", "ko": "시작"},
    "停止": {"de": "Stopp", "fr": "Arrêter", "ja": "停止", "ko": "중지"},
    "暂停": {"de": "Pause", "fr": "Pause", "ja": "一時停止", "ko": "일시정지"},
    "继续": {"de": "Fortsetzen", "fr": "Continuer", "ja": "続行", "ko": "계속"},
    "完成": {"de": "Fertig", "fr": "Terminer", "ja": "完了", "ko": "완료"},
    "应用": {"de": "Anwenden", "fr": "Appliquer", "ja": "適用", "ko": "적용"},
    "选择": {"de": "Auswählen", "fr": "Sélectionner", "ja": "選択", "ko": "선택"},
    "加载": {"de": "Laden", "fr": "Charger", "ja": "読み込み", "ko": "로드"},
    "卸载": {"de": "Entladen", "fr": "Décharger", "ja": "アンロード", "ko": "언로드"},
    "连接": {"de": "Verbinden", "fr": "Connecter", "ja": "接続", "ko": "연결"},
    "断开": {"de": "Trennen", "fr": "Déconnecter", "ja": "切断", "ko": "연결 해제"},
    # 3D printing specific
    "底板": {"de": "Druckplatte", "fr": "Plateau", "ja": "ビルドプレート", "ko": "빌드 플레이트"},
    "挤出": {"de": "Extrusion", "fr": "Extrusion", "ja": "押し出し", "ko": "압출"},
    "回抽": {"de": "Rückzug", "fr": "Rétraction", "ja": "引き戻し", "ko": "리트랙션"},
    "外壳": {"de": "Hülle", "fr": "Coque", "ja": "外壁", "ko": "외벽"},
    "内壁": {"de": "Innenwand", "fr": "Paroi interne", "ja": "内壁", "ko": "내벽"},
    "顶面": {"de": "Oberseite", "fr": "Sommet", "ja": "上面", "ko": "윗면"},
    "底面": {"de": "Unterseite", "fr": "Dessous", "ja": "下面", "ko": "아랫면"},
    "桥接": {"de": "Überbrückung", "fr": "Pont", "ja": "ブリッジ", "ko": "브리지"},
    "裙边": {"de": "Skirt", "fr": "Jupe", "ja": "スカート", "ko": "스커트"},
    "边缘": {"de": "Brim", "fr": "Bordure", "ja": "ブリム", "ko": "브림"},
    "筏": {"de": "Raft", "fr": "Radeau", "ja": "ラフト", "ko": "래프트"},
    # Common adjectives
    "已选中": {"de": "Ausgewählt", "fr": "Sélectionné", "ja": "選択中", "ko": "선택됨"},
    "未选中": {"de": "Nicht ausgewählt", "fr": "Non sélectionné", "ja": "未選択", "ko": "선택 안 됨"},
    "启用": {"de": "Aktiviert", "fr": "Activé", "ja": "有効", "ko": "활성화"},
    "禁用": {"de": "Deaktiviert", "fr": "Désactivé", "ja": "無効", "ko": "비활성화"},
    # More common terms (added after first run showed partial translations)
    "槽位": {"de": "Slot", "fr": "Emplacement", "ja": "スロット", "ko": "슬롯"},
    "映射": {"de": "Zuordnung", "fr": "Mappage", "ja": "マッピング", "ko": "매핑"},
    "余量": {"de": "Rest", "fr": "Restant", "ja": "残量", "ko": "잔량"},
    "覆盖": {"de": "Überschreibung", "fr": "Remplacement", "ja": "上書き", "ko": "재정의"},
    "构建": {"de": "Build", "fr": "Build", "ja": "ビルド", "ko": "빌드"},
    "管理": {"de": "Verwaltung", "fr": "Gestion", "ja": "管理", "ko": "관리"},
    "自动": {"de": "Auto", "fr": "Auto", "ja": "自動", "ko": "자동"},
    "手动": {"de": "Manuell", "fr": "Manuel", "ja": "手動", "ko": "수동"},
    "高级": {"de": "Erweitert", "fr": "Avancé", "ja": "詳細", "ko": "고급"},
    "基本": {"de": "Standard", "fr": "Standard", "ja": "基本", "ko": "기본"},
    "默认": {"de": "Standard", "fr": "Défaut", "ja": "デフォルト", "ko": "기본값"},
    "自定义": {"de": "Benutzerdefiniert", "fr": "Personnalisé", "ja": "カスタム", "ko": "사용자 정의"},
    "历史": {"de": "Verlauf", "fr": "Historique", "ja": "履歴", "ko": "기록"},
    "记录": {"de": "Eintrag", "fr": "Entrée", "ja": "記録", "ko": "기록"},
    "清空": {"de": "Leeren", "fr": "Vider", "ja": "クリア", "ko": "비우기"},
    "确认": {"de": "Bestätigen", "fr": "Confirmer", "ja": "確認", "ko": "확인"},
    "警告": {"de": "Warnung", "fr": "Avertissement", "ja": "警告", "ko": "경고"},
    "错误": {"de": "Fehler", "fr": "Erreur", "ja": "エラー", "ko": "오류"},
    "信息": {"de": "Info", "fr": "Info", "ja": "情報", "ko": "정보"},
    "提示": {"de": "Hinweis", "fr": "Indice", "ja": "ヒント", "ko": "팁"},
    "成功": {"de": "Erfolg", "fr": "Succès", "ja": "成功", "ko": "성공"},
    "失败": {"de": "Fehlgeschlagen", "fr": "Échec", "ja": "失敗", "ko": "실패"},
    "等待": {"de": "Warten", "fr": "Attente", "ja": "待機", "ko": "대기"},
    "超时": {"de": "Zeitüberschreitung", "fr": "Délai", "ja": "タイムアウト", "ko": "시간 초과"},
    "全部": {"de": "Alle", "fr": "Tous", "ja": "すべて", "ko": "전체"},
    "部分": {"de": "Teil", "fr": "Partie", "ja": "一部", "ko": "일부"},
    "无": {"de": "Keine", "fr": "Aucun", "ja": "なし", "ko": "없음"},
    "是": {"de": "Ja", "fr": "Oui", "ja": "はい", "ko": "예"},
    "否": {"de": "Nein", "fr": "Non", "ja": "いいえ", "ko": "아니오"},
    "已": {"de": "", "fr": "", "ja": "済", "ko": "완료"},
    "未": {"de": "Nicht ", "fr": "Non ", "ja": "未", "ko": "미"},
    "在线": {"de": "Online", "fr": "En ligne", "ja": "オンライン", "ko": "온라인"},
    "离线": {"de": "Offline", "fr": "Hors ligne", "ja": "オフライン", "ko": "오프라인"},
    "打印中": {"de": "Druckt", "fr": "Impression", "ja": "プリント中", "ko": "인쇄 중"},
    "空闲": {"de": "Leerlauf", "fr": "Inactif", "ja": "アイドル", "ko": "유휴"},
    "忙碌": {"de": "Beschäftigt", "fr": "Occupé", "ja": "ビジー", "ko": "사용 중"},
    "未知": {"de": "Unbekannt", "fr": "Inconnu", "ja": "不明", "ko": "알 수 없음"},
    "尺寸": {"de": "Größe", "fr": "Taille", "ja": "サイズ", "ko": "크기"},
    "位置": {"de": "Position", "fr": "Position", "ja": "位置", "ko": "위치"},
    "旋转": {"de": "Rotation", "fr": "Rotation", "ja": "回転", "ko": "회전"},
    "缩放": {"de": "Skalierung", "fr": "Échelle", "ja": "スケール", "ko": "스케일"},
    "移动": {"de": "Bewegen", "fr": "Déplacer", "ja": "移動", "ko": "이동"},
    "镜像": {"de": "Spiegeln", "fr": "Miroir", "ja": "ミラー", "ko": "미러"},
    "对齐": {"de": "Ausrichten", "fr": "Aligner", "ja": "整列", "ko": "정렬"},
    "排列": {"de": "Anordnen", "fr": "Disposer", "ja": "配置", "ko": "배치"},
    "分割": {"de": "Teilen", "fr": "Diviser", "ja": "分割", "ko": "분할"},
    "合并": {"de": "Zusammenführen", "fr": "Fusionner", "ja": "結合", "ko": "병합"},
    "镶嵌": {"de": "Einpassen", "fr": "Ajuster", "ja": "フィット", "ko": "맞춤"},
    "简化": {"de": "Vereinfachen", "fr": "Simplifier", "ja": "簡略化", "ko": "단순화"},
    "修复": {"de": "Reparieren", "fr": "Réparer", "ja": "修復", "ko": "수정"},
    "预览": {"de": "Vorschau", "fr": "Aperçu", "ja": "プレビュー", "ko": "미리보기"},
    "上一步": {"de": "Zurück", "fr": "Précédent", "ja": "戻る", "ko": "뒤로"},
    "下一步": {"de": "Weiter", "fr": "Suivant", "ja": "次へ", "ko": "다음"},
    "完成": {"de": "Fertigstellen", "fr": "Terminer", "ja": "完了", "ko": "완료"},
}

def translate(zh_text, lang):
    """Translate ZH text via glossary. Returns None if no match."""
    text = zh_text.strip()
    # Exact match
    if text in GLOSSARY:
        return GLOSSARY[text].get(lang)
    # Substring replacement: replace all known ZH substrings, keep unknown
    result = text
    matched_any = False
    # Sort by length desc to prefer longer matches
    for zh in sorted(GLOSSARY.keys(), key=len, reverse=True):
        if zh in result:
            tr = GLOSSARY[zh].get(lang, zh)
            if tr and tr != zh:
                result = result.replace(zh, tr)
                matched_any = True
    # Keep %1/%2 placeholders intact (already preserved since they're ASCII)
    return result if matched_any else None

if __name__ == "__main__":
    import sys
    test = ["打印", "耗材槽位管理", "槽位 %1", "蓝色 PLA", "喷嘴温度"]
    lang = sys.argv[1] if len(sys.argv) > 1 else "de"
    for t in test:
        print(f"  {t!r:30s} → {translate(t, lang)!r}")
