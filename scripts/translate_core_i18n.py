#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
translate_core_i18n.py
======================
Advance de/fr/ja/ko translation files meaningfully from the 0% baseline by
translating a high-frequency core UI subset (~250 terms per language).

Approach: regex in-place replacement on the .ts text (identical proven pattern
to translate_en_ts.py — NOT xml.etree, which reorders attributes and breaks
the Qt Linguist format).

The dictionary keys are the EXACT escaped <source> text as it appears in the
.ts file (e.g. "&gt;" stays "&gt;"). Translations are XML-escaped before being
written. Output form: <translation>Target Text</translation> (no type
attribute). CRLF line endings preserved via newline=''.

Re-runnable: safe to run multiple times (idempotent on already-finished
entries because they no longer carry type="unfinished").

Usage:
    python scripts/translate_core_i18n.py de
    python scripts/translate_core_i18n.py fr
    python scripts/translate_core_i18n.py ja
    python scripts/translate_core_i18n.py ko
    python scripts/translate_core_i18n.py all
"""

import os
import re
import sys
import xml.sax.saxutils as saxutils

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
I18N = os.path.join(ROOT, 'i18n')

LANGS = ('de', 'fr', 'ja', 'ko')


# ---------------------------------------------------------------------------
# CORE DICTIONARY: Chinese source -> {de, fr, ja, ko}
#
# Keys use the EXACT escaped form found in the .ts file (e.g. "&gt;").
# These are the highest-frequency, most user-visible UI strings: buttons,
# menus, common labels, slicer-domain terms, and status text.
# Translations are native-quality and domain-accurate.
# ---------------------------------------------------------------------------
CORE = {
    # ---- Common buttons / actions ----
    "关闭": {"de": "Schließen", "fr": "Fermer", "ja": "閉じる", "ko": "닫기"},
    "取消": {"de": "Abbrechen", "fr": "Annuler", "ja": "キャンセル", "ko": "취소"},
    "保存": {"de": "Speichern", "fr": "Enregistrer", "ja": "保存", "ko": "저장"},
    "确认": {"de": "OK", "fr": "OK", "ja": "OK", "ko": "확인"},
    "完成": {"de": "Fertig", "fr": "Terminé", "ja": "完了", "ko": "완료"},
    "确定": {"de": "OK", "fr": "OK", "ja": "OK", "ko": "확인"},
    "清空": {"de": "Leeren", "fr": "Effacer", "ja": "クリア", "ko": "비우기"},
    "重试": {"de": "Erneut versuchen", "fr": "Réessayer", "ja": "再試行", "ko": "다시 시도"},
    "继续": {"de": "Fortsetzen", "fr": "Reprendre", "ja": "再開", "ko": "계속"},
    "暂停": {"de": "Pause", "fr": "Pause", "ja": "一時停止", "ko": "일시정지"},
    "停止": {"de": "Stopp", "fr": "Arrêter", "ja": "停止", "ko": "정지"},
    "播放": {"de": "Wiedergabe", "fr": "Lecture", "ja": "再生", "ko": "재생"},
    "上一步": {"de": "Zurück", "fr": "Précédent", "ja": "前へ", "ko": "이전"},
    "下一步": {"de": "Weiter", "fr": "Suivant", "ja": "次へ", "ko": "다음"},
    "高级": {"de": "Erweitert", "fr": "Avancé", "ja": "詳細", "ko": "고급"},
    "可选": {"de": "Optional", "fr": "Optionnel", "ja": "オプション", "ko": "선택 사항"},
    "计算": {"de": "Berechnen", "fr": "Calculer", "ja": "計算", "ko": "계산"},
    "切换": {"de": "Umschalten", "fr": "Basculer", "ja": "切替", "ko": "전환"},
    "翻转": {"de": "Spiegeln", "fr": "Retourner", "ja": "反転", "ko": "뒤집기"},
    "居中": {"de": "Zentrieren", "fr": "Centrer", "ja": "中央揃え", "ko": "가운데 정렬"},

    # ---- Top-level menus / navigation ----
    "新建项目": {"de": "Neues Projekt", "fr": "Nouveau projet", "ja": "新規プロジェクト", "ko": "새 프로젝트"},
    "打开项目...": {"de": "Projekt öffnen...", "fr": "Ouvrir un projet...", "ja": "プロジェクトを開く...", "ko": "프로젝트 열기..."},
    "保存项目": {"de": "Projekt speichern", "fr": "Enregistrer le projet", "ja": "プロジェクトを保存", "ko": "프로젝트 저장"},
    "项目另存为...": {"de": "Projekt speichern unter...", "fr": "Enregistrer le projet sous...", "ja": "プロジェクトを名前を付けて保存...", "ko": "프로젝트 다른 이름으로 저장..."},
    "项目另存为": {"de": "Projekt speichern unter", "fr": "Enregistrer le projet sous", "ja": "プロジェクトを名前を付けて保存", "ko": "프로젝트 다른 이름으로 저장"},
    "校准": {"de": "Kalibrierung", "fr": "Étalonnage", "ja": "キャリブレーション", "ko": "캘리브레이션"},
    "准备": {"de": "Vorbereiten", "fr": "Préparer", "ja": "準備", "ko": "준비"},
    "预览": {"de": "Vorschau", "fr": "Aperçu", "ja": "プレビュー", "ko": "미리보기"},
    "设备": {"de": "Gerät", "fr": "Appareil", "ja": "デバイス", "ko": "장치"},
    "项目": {"de": "Projekt", "fr": "Projet", "ja": "プロジェクト", "ko": "프로젝트"},
    "切片": {"de": "Slicen", "fr": "Trancher", "ja": "スライス", "ko": "슬라이스"},
    "打印": {"de": "Drucken", "fr": "Imprimer", "ja": "プリント", "ko": "출력"},
    "通知中心": {"de": "Benachrichtigungscenter", "fr": "Centre de notifications", "ja": "通知センター", "ko": "알림 센터"},
    "最小化": {"de": "Minimieren", "fr": "Réduire", "ja": "最小化", "ko": "최소화"},
    "还原": {"de": "Wiederherstellen", "fr": "Restaurer", "ja": "元に戻す", "ko": "복원"},
    "最大化": {"de": "Maximieren", "fr": "Agrandir", "ja": "最大化", "ko": "최대화"},
    "切片单盘": {"de": "Einzelne Druckplatte slicen", "fr": "Trancher une seule platine", "ja": "単一プレートをスライス", "ko": "단일 플레이트 슬라이스"},
    "导出G-code文件": {"de": "G-code-Datei exportieren", "fr": "Exporter le fichier G-code", "ja": "G-codeファイルをエクスポート", "ko": "G-code 파일 내보내기"},
    "最近文件": {"de": "Zuletzt verwendete Dateien", "fr": "Fichiers récents", "ja": "最近使ったファイル", "ko": "최근 파일"},
    "清空最近文件": {"de": "Letzte Dateien löschen", "fr": "Effacer les fichiers récents", "ja": "最近使ったファイルをクリア", "ko": "최근 파일 비우기"},

    # ---- Common edit / view operations ----
    "导出": {"de": "Exportieren", "fr": "Exporter", "ja": "エクスポート", "ko": "내보내기"},
    "剪切": {"de": "Ausschneiden", "fr": "Couper", "ja": "切り取り", "ko": "잘라내기"},
    "删除选中": {"de": "Auswahl löschen", "fr": "Supprimer la sélection", "ja": "選択を削除", "ko": "선택 삭제"},
    "取消选择": {"de": "Auswahl aufheben", "fr": "Désélectionner", "ja": "選択を解除", "ko": "선택 해제"},
    "反向选择": {"de": "Auswahl invertieren", "fr": "Inverser la sélection", "ja": "選択を反転", "ko": "선택 반전"},
    "视图": {"de": "Ansicht", "fr": "Vue", "ja": "表示", "ko": "보기"},
    "显示/隐藏 Gizmo": {"de": "Gizmo anzeigen/ausblenden", "fr": "Afficher/Masquer le Gizmo", "ja": "Gizmoの表示/非表示", "ko": "기즈모 표시/숨기기"},
    "重置视图": {"de": "Ansicht zurücksetzen", "fr": "Réinitialiser la vue", "ja": "表示をリセット", "ko": "보기 초기화"},
    "显示层": {"de": "Schicht anzeigen", "fr": "Afficher la couche", "ja": "レイヤーを表示", "ko": "레이어 표시"},
    "隐藏层": {"de": "Schicht ausblenden", "fr": "Masquer la couche", "ja": "レイヤーを非表示", "ko": "레이어 숨기기"},
    "复制选中": {"de": "Auswahl kopieren", "fr": "Copier la sélection", "ja": "選択をコピー", "ko": "선택 복사"},
    "克隆选中": {"de": "Auswahl duplizieren", "fr": "Cloner la sélection", "ja": "選択を複製", "ko": "선택 복제"},
    "重命名": {"de": "Umbenennen", "fr": "Renommer", "ja": "名前の変更", "ko": "이름 바꾸기"},
    "删除对象": {"de": "Objekt löschen", "fr": "Supprimer l'objet", "ja": "オブジェクトを削除", "ko": "객체 삭제"},
    "删除部件": {"de": "Teil löschen", "fr": "Supprimer la pièce", "ja": "パーツを削除", "ko": "부품 삭제"},

    # ---- Toast / status messages ----
    "导入失败": {"de": "Import fehlgeschlagen", "fr": "Échec de l'importation", "ja": "インポート失敗", "ko": "가져오기 실패"},
    "切片失败": {"de": "Slicen fehlgeschlagen", "fr": "Échec du tranchage", "ja": "スライス失敗", "ko": "슬라이스 실패"},
    "项目已保存": {"de": "Projekt gespeichert", "fr": "Projet enregistré", "ja": "プロジェクトを保存しました", "ko": "프로젝트가 저장됨"},
    "切片中": {"de": "Slicen...", "fr": "Tranchage...", "ja": "スライス中", "ko": "슬라이스 중"},
    "切片完成": {"de": "Slicen abgeschlossen", "fr": "Tranchage terminé", "ja": "スライス完了", "ko": "슬라이스 완료"},
    "导出完成": {"de": "Export abgeschlossen", "fr": "Exportation terminée", "ja": "エクスポート完了", "ko": "내보내기 완료"},
    "导出中": {"de": "Exportiere", "fr": "Exportation", "ja": "エクスポート中", "ko": "내보내는 중"},
    "排列中": {"de": "Anordnen...", "fr": "Disposition...", "ja": "配置中", "ko": "정렬 중"},
    "操作失败": {"de": "Vorgang fehlgeschlagen", "fr": "Échec de l'opération", "ja": "操作に失敗しました", "ko": "작업 실패"},
    "克隆失败": {"de": "Duplizieren fehlgeschlagen", "fr": "Échec du clonage", "ja": "複製に失敗しました", "ko": "복제 실패"},
    "连接失败": {"de": "Verbindung fehlgeschlagen", "fr": "Échec de la connexion", "ja": "接続に失敗しました", "ko": "연결 실패"},
    "设置打印状态失败": {"de": "Druckstatus konnte nicht gesetzt werden", "fr": "Échec de la définition de l'état d'impression", "ja": "印刷ステータスの設定に失敗しました", "ko": "출력 상태 설정 실패"},

    # ---- Generic labels / properties ----
    "距离": {"de": "Entfernung", "fr": "Distance", "ja": "距離", "ko": "거리"},
    "尺寸": {"de": "Abmessungen", "fr": "Dimensions", "ja": "サイズ", "ko": "크기"},
    "直径": {"de": "Durchmesser", "fr": "Diamètre", "ja": "直径", "ko": "지름"},
    "大小": {"de": "Größe", "fr": "Taille", "ja": "サイズ", "ko": "크기"},
    "日期": {"de": "Datum", "fr": "Date", "ja": "日付", "ko": "날짜"},
    "文件名": {"de": "Dateiname", "fr": "Nom du fichier", "ja": "ファイル名", "ko": "파일 이름"},
    "名称": {"de": "Name", "fr": "Nom", "ja": "名前", "ko": "이름"},
    "状态": {"de": "Status", "fr": "État", "ja": "状態", "ko": "상태"},
    "类型": {"de": "Typ", "fr": "Type", "ja": "タイプ", "ko": "유형"},
    "模式": {"de": "Modus", "fr": "Mode", "ja": "モード", "ko": "모드"},
    "形状": {"de": "Form", "fr": "Forme", "ja": "形状", "ko": "모양"},
    "方向": {"de": "Richtung", "fr": "Direction", "ja": "方向", "ko": "방향"},
    "颜色": {"de": "Farbe", "fr": "Couleur", "ja": "色", "ko": "색상"},
    "位置": {"de": "Position", "fr": "Position", "ja": "位置", "ko": "위치"},
    "深度": {"de": "Tiefe", "fr": "Profondeur", "ja": "深さ", "ko": "깊이"},
    "半径": {"de": "Radius", "fr": "Rayon", "ja": "半径", "ko": "반지름"},
    "角度": {"de": "Winkel", "fr": "Angle", "ja": "角度", "ko": "각도"},
    "体积": {"de": "Volumen", "fr": "Volume", "ja": "体積", "ko": "부피"},
    "重量": {"de": "Gewicht", "fr": "Poids", "ja": "重量", "ko": "무게"},
    "时间": {"de": "Zeit", "fr": "Heure", "ja": "時間", "ko": "시간"},
    "分辨率": {"de": "Auflösung", "fr": "Résolution", "ja": "解像度", "ko": "해상도"},
    "版本": {"de": "Version", "fr": "Version", "ja": "バージョン", "ko": "버전"},

    # ---- Shapes / primitives ----
    "矩形": {"de": "Rechteck", "fr": "Rectangle", "ja": "長方形", "ko": "사각형"},
    "圆形": {"de": "Kreis", "fr": "Cercle", "ja": "円形", "ko": "원형"},
    "自定义": {"de": "Benutzerdefiniert", "fr": "Personnalisé", "ja": "カスタム", "ko": "사용자 정의"},
    "立方体": {"de": "Würfel", "fr": "Cube", "ja": "立方体", "ko": "정육면체"},
    "球体": {"de": "Kugel", "fr": "Sphère", "ja": "球体", "ko": "구"},
    "圆柱体": {"de": "Zylinder", "fr": "Cylindre", "ja": "円柱", "ko": "원기둥"},
    "圆环": {"de": "Torus", "fr": "Tore", "ja": "トーラス", "ko": "토러스"},
    "圆锥体": {"de": "Kegel", "fr": "Cône", "ja": "円錐", "ko": "원뿔"},
    "三角形": {"de": "Dreieck", "fr": "Triangle", "ja": "三角形", "ko": "삼각형"},
    "方形": {"de": "Quadrat", "fr": "Carré", "ja": "四角形", "ko": "사각형"},

    # ---- Temperature domain ----
    "温度": {"de": "Temperatur", "fr": "Température", "ja": "温度", "ko": "온도"},
    "喷嘴温度": {"de": "Düsentemperatur", "fr": "Température de la buse", "ja": "ノズル温度", "ko": "노즐 온도"},
    "热床温度": {"de": "Druckbetttemperatur", "fr": "Température du plateau", "ja": "熱床温度", "ko": "베드 온도"},
    "腔体温度": {"de": "Kammertemperatur", "fr": "Température de la chambre", "ja": "チャンバー温度", "ko": "챔버 온도"},
    "封闭室温度": {"de": "Kammertemperatur", "fr": "Température de l'enceinte", "ja": "チャンバー温度", "ko": "챔버 온도"},
    "首层温度": {"de": "Erste-Schicht-Temperatur", "fr": "Température de la première couche", "ja": "最初の層の温度", "ko": "첫 레이어 온도"},
    "首层喷嘴温度": {"de": "Düsentemperatur der ersten Schicht", "fr": "Température de buse de la première couche", "ja": "最初の層のノズル温度", "ko": "첫 레이어 노즐 온도"},
    "首层热床温度": {"de": "Druckbetttemperatur der ersten Schicht", "fr": "Température du plateau de la première couche", "ja": "最初の層の熱床温度", "ko": "첫 레이어 베드 온도"},
    "预热": {"de": "Vorheizen", "fr": "Préchauffer", "ja": "予熱", "ko": "예열"},
    "喷嘴": {"de": "Düse", "fr": "Buse", "ja": "ノズル", "ko": "노즐"},
    "热床": {"de": "Druckbett", "fr": "Plateau d'impression", "ja": "熱床", "ko": "출력 베드"},
    "喷头": {"de": "Werkzeugkopf", "fr": "Tête d'outil", "ja": "ツールヘッド", "ko": "툴 헤드"},
    "喷头温度": {"de": "Werkzeugtemperatur", "fr": "Température de l'outil", "ja": "ツール温度", "ko": "툴 온도"},
    "挤出机": {"de": "Extruder", "fr": "Extrudeur", "ja": "エクストルーダー", "ko": "익스트루더"},

    # ---- Speed / acceleration ----
    "速度": {"de": "Geschwindigkeit", "fr": "Vitesse", "ja": "速度", "ko": "속도"},
    "加速度": {"de": "Beschleunigung", "fr": "Accélération", "ja": "加速度", "ko": "가속도"},
    "壁速度": {"de": "Wandgeschwindigkeit", "fr": "Vitesse des parois", "ja": "壁の速度", "ko": "벽 속도"},
    "填充速度": {"de": "Füllungsgeschwindigkeit", "fr": "Vitesse de remplissage", "ja": "インフィル速度", "ko": "채움 속도"},
    "空走速度": {"de": "Eilganggeschwindigkeit", "fr": "Vitesse de déplacement", "ja": "移動速度", "ko": "이동 속도"},
    "首层速度": {"de": "Erste-Schicht-Geschwindigkeit", "fr": "Vitesse de la première couche", "ja": "最初の層の速度", "ko": "첫 레이어 속도"},
    "平均速度": {"de": "Durchschnittliche Geschwindigkeit", "fr": "Vitesse moyenne", "ja": "平均速度", "ko": "평균 속도"},
    "平均打印速度": {"de": "Durchschnittliche Druckgeschwindigkeit", "fr": "Vitesse d'impression moyenne", "ja": "平均プリント速度", "ko": "평균 출력 속도"},
    "速度限制": {"de": "Geschwindigkeitsgrenze", "fr": "Limite de vitesse", "ja": "速度制限", "ko": "속도 제한"},
    "加速度限制": {"de": "Beschleunigungsgrenze", "fr": "Limite d'accélération", "ja": "加速度制限", "ko": "가속도 제한"},

    # ---- Slicer core domain ----
    "耗材": {"de": "Filament", "fr": "Filament", "ja": "フィラメント", "ko": "필라멘트"},
    "耗材丝": {"de": "Filament", "fr": "Filament", "ja": "フィラメント", "ko": "필라멘트"},
    "耗材类型": {"de": "Filamenttyp", "fr": "Type de filament", "ja": "フィラメントの種類", "ko": "필라멘트 유형"},
    "耗材预设": {"de": "Filament-Voreinstellung", "fr": "Préréglage de filament", "ja": "フィラメントプリセット", "ko": "필라멘트 사전 설정"},
    "耗材设置": {"de": "Filamenteinstellungen", "fr": "Paramètres du filament", "ja": "フィラメント設定", "ko": "필라멘트 설정"},
    "耗材重量": {"de": "Filamentgewicht", "fr": "Poids du filament", "ja": "フィラメント重量", "ko": "필라멘트 무게"},
    "耗材用量": {"de": "Filamentverbrauch", "fr": "Consommation de filament", "ja": "フィラメント使用量", "ko": "필라멘트 사용량"},
    "耗材长度": {"de": "Filamentlänge", "fr": "Longueur du filament", "ja": "フィラメント長さ", "ko": "필라멘트 길이"},
    "材料设置": {"de": "Materialeinstellungen", "fr": "Paramètres du matériau", "ja": "材料設定", "ko": "재료 설정"},
    "支撑": {"de": "Stützstruktur", "fr": "Support", "ja": "サポート", "ko": "서포트"},
    "支撑结构": {"de": "Stützstruktur", "fr": "Structure de support", "ja": "サポート構造", "ko": "서포트 구조"},
    "基础支撑": {"de": "Grundstützstruktur", "fr": "Support de base", "ja": "基本サポート", "ko": "기본 서포트"},
    "支撑界面": {"de": "Stützstruktur-Schnittstelle", "fr": "Interface de support", "ja": "サポートインターフェース", "ko": "서포트 인터페이스"},
    "支撑参数": {"de": "Stützstruktur-Parameter", "fr": "Paramètres de support", "ja": "サポートパラメータ", "ko": "서포트 매개변수"},
    "支撑线宽": {"de": "Stützstruktur-Linienbreite", "fr": "Largeur de ligne de support", "ja": "サポート線幅", "ko": "서포트 선 너비"},
    "支撑速度": {"de": "Stützstruktur-Geschwindigkeit", "fr": "Vitesse du support", "ja": "サポート速度", "ko": "서포트 속도"},
    "可溶性": {"de": "Löslich", "fr": "Soluble", "ja": "可溶性", "ko": "수용성"},
    "盘": {"de": "Druckplatte", "fr": "Platine", "ja": "プレート", "ko": "플레이트"},
    "平板": {"de": "Druckplatte", "fr": "Platine", "ja": "プレート", "ko": "플레이트"},
    "底板": {"de": "Druckbett", "fr": "Plateau de construction", "ja": "ビルドプレート", "ko": "빌드 플레이트"},
    "热床形状": {"de": "Druckbettform", "fr": "Forme du plateau", "ja": "熱床の形状", "ko": "베드 형태"},
    "热床类型": {"de": "Druckbetttyp", "fr": "Type de plateau", "ja": "熱床の種類", "ko": "베드 유형"},
    "热床形状设置": {"de": "Druckbettform-Einstellungen", "fr": "Paramètres de forme du plateau", "ja": "熱床の形状設定", "ko": "베드 형태 설정"},
    "层高": {"de": "Schichthöhe", "fr": "Hauteur de couche", "ja": "層の高さ", "ko": "레이어 높이"},
    "层数": {"de": "Schichtanzahl", "fr": "Nombre de couches", "ja": "層数", "ko": "레이어 수"},
    "总层数": {"de": "Gesamte Schichtanzahl", "fr": "Nombre total de couches", "ja": "総層数", "ko": "전체 레이어 수"},
    "切片层数": {"de": "Anzahl der Slicen-Schichten", "fr": "Nombre de couches tranchées", "ja": "スライス層数", "ko": "슬라이스 레이어 수"},
    "首层层高": {"de": "Höhe der ersten Schicht", "fr": "Hauteur de la première couche", "ja": "最初の層の高さ", "ko": "첫 레이어 높이"},
    "层范围": {"de": "Schichtbereich", "fr": "Plage de couches", "ja": "層の範囲", "ko": "레이어 범위"},
    "起始层": {"de": "Startschicht", "fr": "Couche de départ", "ja": "開始層", "ko": "시작 레이어"},
    "结束层": {"de": "Endschicht", "fr": "Couche de fin", "ja": "終了層", "ko": "종료 레이어"},
    "填充": {"de": "Füllung", "fr": "Remplissage", "ja": "インフィル", "ko": "채움"},
    "填充密度": {"de": "Füllungsdichte", "fr": "Densité de remplissage", "ja": "インフィル密度", "ko": "채움 밀도"},
    "填充模式": {"de": "Füllungsmuster", "fr": "Motif de remplissage", "ja": "インフィルパターン", "ko": "채움 패턴"},
    "内部填充": {"de": "Innere Füllung", "fr": "Remplissage interne", "ja": "内部インフィル", "ko": "내부 채움"},
    "填充设置": {"de": "Füllungseinstellungen", "fr": "Paramètres de remplissage", "ja": "インフィル設定", "ko": "채움 설정"},
    "冷却": {"de": "Kühlung", "fr": "Refroidissement", "ja": "冷却", "ko": "냉각"},
    "风扇设置": {"de": "Lüftereinstellungen", "fr": "Paramètres du ventilateur", "ja": "ファン設定", "ko": "팬 설정"},
    "回退": {"de": "Rückzug", "fr": "Rétraction", "ja": "リトラクション", "ko": "리트랙션"},
    "回退设置": {"de": "Rückzugeinstellungen", "fr": "Paramètres de rétraction", "ja": "リトラクション設定", "ko": "리트랙션 설정"},
    "壁": {"de": "Wand", "fr": "Paroi", "ja": "壁", "ko": "벽"},
    "外墙": {"de": "Außenwand", "fr": "Paroi externe", "ja": "外壁", "ko": "외벽"},
    "内壁": {"de": "Innenwand", "fr": "Paroi interne", "ja": "内壁", "ko": "내벽"},
    "墙体层数": {"de": "Wandschichten", "fr": "Nombre de parois", "ja": "壁の層数", "ko": "벽 레이어 수"},
    "顶部层数": {"de": "Deckschichten", "fr": "Couches supérieures", "ja": "上部の層数", "ko": "상단 레이어 수"},
    "底部层数": {"de": "Bodenschichten", "fr": "Couches inférieures", "ja": "下部の層数", "ko": "하단 레이어 수"},
    "底筏": {"de": "Raft", "fr": "Radeau", "ja": "ラフト", "ko": "래프트"},
    "裙边": {"de": "Skirt", "fr": "Jupe", "ja": "スカート", "ko": "스커트"},
    "附着力": {"de": "Haftkraft", "fr": "Adhérence", "ja": "密着性", "ko": "접착력"},
    "附着力类型": {"de": "Haftkraft-Typ", "fr": "Type d'adhérence", "ja": "密着タイプ", "ko": "접착 유형"},
    "熨烫类型": {"de": "Glättungstyp", "fr": "Type de lissage", "ja": "アイロンタイプ", "ko": "다림질 유형"},
    "底座": {"de": "Haftgrund", "fr": "Adhérence", "ja": "アドヘッション", "ko": "접착"},
    "输出": {"de": "Ausgabe", "fr": "Sortie", "ja": "出力", "ko": "출력"},
    "多材料": {"de": "Multimaterial", "fr": "Multi-matériau", "ja": "マルチマテリアル", "ko": "다중 재료"},
    "螺旋花瓶": {"de": "Spiralvasenmodus", "fr": "Mode vase spiral", "ja": "スパイラル花瓶", "ko": "나선형 꽃병"},
    "检测悬空壁": {"de": "Überhangwände erkennen", "fr": "Détecter les parois en surplomb", "ja": "オーバーハング壁を検出", "ko": "현수벽 감지"},
    "减少穿越壁": {"de": "Wandüberquerungen reduzieren", "fr": "Réduire le franchissement de parois", "ja": "壁の横断を減らす", "ko": "벽 횡단 감소"},
    "精确外壁": {"de": "Genaue Außenwand", "fr": "Paroi externe précise", "ja": "正確な外壁", "ko": "정확한 외벽"},
    "仅单壁顶层": {"de": "Einwand-Deckschicht", "fr": "Couche supérieure à paroi unique", "ja": "単一壁の最上層", "ko": "단일 벽 상단 레이어"},

    # ---- Quality / strength group ----
    "质量": {"de": "Qualität", "fr": "Qualité", "ja": "品質", "ko": "품질"},
    "强度": {"de": "Festigkeit", "fr": "Solidité", "ja": "強度", "ko": "강도"},
    "表面质量": {"de": "Oberflächenqualität", "fr": "Qualité de surface", "ja": "表面品質", "ko": "표면 품질"},
    "层高与线宽": {"de": "Schichthöhe und Linienbreite", "fr": "Hauteur de couche et largeur de ligne", "ja": "層の高さと線幅", "ko": "레이어 높이와 선 너비"},
    "壁与壳": {"de": "Wände und Hülle", "fr": "Parois et coque", "ja": "壁とシェル", "ko": "벽과 쉘"},
    "工艺设置": {"de": "Prozesseinstellungen", "fr": "Paramètres de processus", "ja": "プロセス設定", "ko": "공정 설정"},
    "编辑工艺设置": {"de": "Prozesseinstellungen bearbeiten", "fr": "Modifier les paramètres de processus", "ja": "プロセス設定を編集", "ko": "공정 설정 편집"},
    "编辑工艺预设": {"de": "Prozess-Voreinstellung bearbeiten", "fr": "Modifier le préréglage de processus", "ja": "プロセスプリセットを編集", "ko": "공정 사전 설정 편집"},
    "全局": {"de": "Global", "fr": "Global", "ja": "グローバル", "ko": "전역"},
    "其他": {"de": "Sonstige", "fr": "Autre", "ja": "その他", "ko": "기타"},
    "通用": {"de": "Allgemein", "fr": "Général", "ja": "一般", "ko": "일반"},
    "高级模式": {"de": "Erweiterter Modus", "fr": "Mode avancé", "ja": "詳細モード", "ko": "고급 모드"},
    "精简模式": {"de": "Lite-Modus", "fr": "Mode léger", "ja": "ライトモード", "ko": "라이트 모드"},
    "完整模式": {"de": "Vollständiger Modus", "fr": "Mode complet", "ja": "フルモード", "ko": "전체 모드"},
    "预设参数": {"de": "Voreinstellungsparameter", "fr": "Paramètres prédéfinis", "ja": "プリセットパラメータ", "ko": "사전 설정 매개변수"},
    "预设已修改": {"de": "Voreinstellung geändert", "fr": "Préréglage modifié", "ja": "プリセットが変更されました", "ko": "사전 설정 변경됨"},
    "预设不兼容": {"de": "Voreinstellung inkompatibel", "fr": "Préréglage incompatible", "ja": "プリセットに互換性がありません", "ko": "사전 설정 호환 안 됨"},
    "未保存的修改": {"de": "Ungespeicherte Änderungen", "fr": "Modifications non enregistrées", "ja": "未保存の変更", "ko": "저장되지 않은 변경 사항"},
    "丢弃修改": {"de": "Änderungen verwerfen", "fr": "Annuler les modifications", "ja": "変更を破棄", "ko": "변경 사항 취소"},
    "保存为预设...": {"de": "Als Voreinstellung speichern...", "fr": "Enregistrer comme préréglage...", "ja": "プリセットとして保存...", "ko": "사전 설정으로 저장..."},
    "保存到预设": {"de": "In Voreinstellung speichern", "fr": "Enregistrer dans le préréglage", "ja": "プリセットに保存", "ko": "사전 설정에 저장"},
    "保存到历史": {"de": "In Verlauf speichern", "fr": "Enregistrer dans l'historique", "ja": "履歴に保存", "ko": "기록에 저장"},

    # ---- Device / connection states ----
    "打印机": {"de": "Drucker", "fr": "Imprimante", "ja": "プリンター", "ko": "프린터"},
    "打印机设置": {"de": "Druckereinstellungen", "fr": "Paramètres de l'imprimante", "ja": "プリンター設定", "ko": "프린터 설정"},
    "打印机连接": {"de": "Druckerverbindung", "fr": "Connexion de l'imprimante", "ja": "プリンター接続", "ko": "프린터 연결"},
    "打印机型号": {"de": "Druckermodell", "fr": "Modèle d'imprimante", "ja": "プリンター型番", "ko": "프린터 모델"},
    "设备名称": {"de": "Gerätename", "fr": "Nom de l'appareil", "ja": "デバイス名", "ko": "장치 이름"},
    "在线": {"de": "Online", "fr": "En ligne", "ja": "オンライン", "ko": "온라인"},
    "离线": {"de": "Offline", "fr": "Hors ligne", "ja": "オフライン", "ko": "오프라인"},
    "空闲": {"de": "Leerlauf", "fr": "Inactif", "ja": "アイドル", "ko": "대기 중"},
    "打印中": {"de": "Druckt", "fr": "Impression", "ja": "プリント中", "ko": "출력 중"},
    "连接中": {"de": "Verbinde", "fr": "Connexion", "ja": "接続中", "ko": "연결 중"},
    "未选择": {"de": "Nicht ausgewählt", "fr": "Non sélectionné", "ja": "未選択", "ko": "선택 안 됨"},
    "未选择设备": {"de": "Kein Gerät ausgewählt", "fr": "Aucun appareil sélectionné", "ja": "デバイスが選択されていません", "ko": "선택된 장치 없음"},
    "空": {"de": "Leer", "fr": "Vide", "ja": "空", "ko": "비어 있음"},
    "断开": {"de": "Trennen", "fr": "Déconnecter", "ja": "切断", "ko": "연결 해제"},
    "连接已断开": {"de": "Verbindung getrennt", "fr": "Connexion interrompue", "ja": "接続が切断されました", "ko": "연결이 끊어짐"},
    "绑定": {"de": "Binden", "fr": "Lier", "ja": "バインド", "ko": "바인딩"},
    "解绑": {"de": "Trennen", "fr": "Délier", "ja": "バインド解除", "ko": "바인딩 해제"},
    "登录": {"de": "Anmelden", "fr": "Se connecter", "ja": "ログイン", "ko": "로그인"},
    "用户名": {"de": "Benutzername", "fr": "Nom d'utilisateur", "ja": "ユーザー名", "ko": "사용자 이름"},
    "密码": {"de": "Passwort", "fr": "Mot de passe", "ja": "パスワード", "ko": "비밀번호"},
    "IP 地址": {"de": "IP-Adresse", "fr": "Adresse IP", "ja": "IPアドレス", "ko": "IP 주소"},
    "信号强度": {"de": "Signalstärke", "fr": "Force du signal", "ja": "信号強度", "ko": "신호 강도"},
    "SD 卡": {"de": "SD-Karte", "fr": "Carte SD", "ja": "SDカード", "ko": "SD 카드"},
    "视频": {"de": "Video", "fr": "Vidéo", "ja": "動画", "ko": "동영상"},
    "录像": {"de": "Aufnahme", "fr": "Enregistrement", "ja": "録画", "ko": "녹화"},
    "延时摄影": {"de": "Zeitraffer", "fr": "Accéléré", "ja": "タイムラプス", "ko": "타임랩스"},
    "舱室灯": {"de": "Kammerbeleuchtung", "fr": "Éclairage de la chambre", "ja": "チャンバー照明", "ko": "챔버 조명"},
    "工作灯": {"de": "Arbeitsbeleuchtung", "fr": "Éclairage de travail", "ja": "作業照明", "ko": "작업 조명"},
    "当前任务": {"de": "Aktuelle Aufgabe", "fr": "Tâche actuelle", "ja": "現在のタスク", "ko": "현재 작업"},
    "当前活动": {"de": "Aktuelle Aktivität", "fr": "Activité actuelle", "ja": "現在のアクティビティ", "ko": "현재 활동"},
    "严重": {"de": "Kritisch", "fr": "Critique", "ja": "重大", "ko": "심각"},
    "一般": {"de": "Allgemein", "fr": "Général", "ja": "通常", "ko": "일반"},
    "信息": {"de": "Info", "fr": "Info", "ja": "情報", "ko": "정보"},
    "历史记录": {"de": "Verlauf", "fr": "Historique", "ja": "履歴", "ko": "기록"},
    "校准历史记录": {"de": "Kalibrierungsverlauf", "fr": "Historique d'étalonnage", "ja": "キャリブレーション履歴", "ko": "캘리브레이션 기록"},
    "校准结果": {"de": "Kalibrierungsergebnis", "fr": "Résultat d'étalonnage", "ja": "キャリブレーション結果", "ko": "캘리브레이션 결과"},
    "固件版本": {"de": "Firmware-Version", "fr": "Version du firmware", "ja": "ファームウェアバージョン", "ko": "펌웨어 버전"},
    "固件升级": {"de": "Firmware-Upgrade", "fr": "Mise à jour du firmware", "ja": "ファームウェアアップグレード", "ko": "펌웨어 업그레이드"},
    "当前版本": {"de": "Aktuelle Version", "fr": "Version actuelle", "ja": "現在のバージョン", "ko": "현재 버전"},
    "最新版本": {"de": "Neueste Version", "fr": "Dernière version", "ja": "最新バージョン", "ko": "최신 버전"},
    "更新日志": {"de": "Änderungsprotokoll", "fr": "Journal des modifications", "ja": "変更履歴", "ko": "업데이트 내역"},
    "开始升级": {"de": "Upgrade starten", "fr": "Démarrer la mise à jour", "ja": "アップグレードを開始", "ko": "업그레이드 시작"},
    "序列号": {"de": "Seriennummer", "fr": "Numéro de série", "ja": "シリアル番号", "ko": "일련 번호"},

    # ---- Preferences / settings ----
    "设置": {"de": "Einstellungen", "fr": "Paramètres", "ja": "設定", "ko": "설정"},
    "搜索设置...": {"de": "Einstellungen suchen...", "fr": "Rechercher dans les paramètres...", "ja": "設定を検索...", "ko": "설정 검색..."},
    "搜索设置": {"de": "Einstellungen suchen", "fr": "Rechercher dans les paramètres", "ja": "設定を検索", "ko": "설정 검색"},
    "默认页面": {"de": "Standardseite", "fr": "Page par défaut", "ja": "デフォルトページ", "ko": "기본 페이지"},
    "主页": {"de": "Startseite", "fr": "Accueil", "ja": "ホーム", "ko": "홈"},
    "用户角色": {"de": "Benutzerrolle", "fr": "Rôle de l'utilisateur", "ja": "ユーザーロール", "ko": "사용자 역할"},
    "基础": {"de": "Standard", "fr": "Basique", "ja": "基本", "ko": "기본"},
    "专业": {"de": "Profi", "fr": "Professionnel", "ja": "プロ", "ko": "전문"},
    "自动保存": {"de": "Automatisch speichern", "fr": "Enregistrement automatique", "ja": "自動保存", "ko": "자동 저장"},
    "通知设置": {"de": "Benachrichtigungseinstellungen", "fr": "Paramètres de notification", "ja": "通知設定", "ko": "알림 설정"},
    "启用通知": {"de": "Benachrichtigungen aktivieren", "fr": "Activer les notifications", "ja": "通知を有効化", "ko": "알림 활성화"},
    "区域设置": {"de": "Regionseinstellungen", "fr": "Paramètres régionaux", "ja": "地域設定", "ko": "지역 설정"},
    "跟随系统": {"de": "System folgen", "fr": "Suivre le système", "ja": "システムに従う", "ko": "시스템 따르기"},
    "中国": {"de": "China", "fr": "Chine", "ja": "中国", "ko": "중국"},
    "美国": {"de": "USA", "fr": "États-Unis", "ja": "アメリカ", "ko": "미국"},
    "欧洲": {"de": "Europa", "fr": "Europe", "ja": "ヨーロッパ", "ko": "유럽"},
    "日本": {"de": "Japan", "fr": "Japon", "ja": "日本", "ko": "일본"},
    "软件更新": {"de": "Software-Update", "fr": "Mise à jour du logiciel", "ja": "ソフトウェアアップデート", "ko": "소프트웨어 업데이트"},
    "检查更新": {"de": "Nach Updates suchen", "fr": "Rechercher des mises à jour", "ja": "アップデートを確認", "ko": "업데이트 확인"},
    "稳定版": {"de": "Stabil", "fr": "Stable", "ja": "安定版", "ko": "안정"},
    "测试版": {"de": "Beta", "fr": "Bêta", "ja": "ベータ版", "ko": "베타"},
    "开发版": {"de": "Entwicklung", "fr": "Développement", "ja": "開発版", "ko": "개발"},
    "开发者选项": {"de": "Entwickleroptionen", "fr": "Options de développeur", "ja": "開発者オプション", "ko": "개발자 옵션"},
    "开发者模式": {"de": "Entwicklermodus", "fr": "Mode développeur", "ja": "開発者モード", "ko": "개발자 모드"},
    "调试覆盖层": {"de": "Debug-Overlay", "fr": "Calque de débogage", "ja": "デバッグオーバーレイ", "ko": "디버그 오버레이"},
    "日志级别": {"de": "Protokollierungsstufe", "fr": "Niveau de journalisation", "ja": "ログレベル", "ko": "로그 수준"},
    "低细节模式": {"de": "Modus mit geringen Details", "fr": "Mode faible détail", "ja": "低詳細モード", "ko": "저디테일 모드"},
    "减少动画效果": {"de": "Animationen reduzieren", "fr": "Réduire les animations", "ja": "アニメーションを減らす", "ko": "애니메이션 줄이기"},
    "快捷键绑定": {"de": "Tastenkürzel-Zuweisungen", "fr": "Raccourcis clavier", "ja": "ショートカットキーの割り当て", "ko": "단축키 바인딩"},
    "默认打印机设置": {"de": "Standard-Druckereinstellungen", "fr": "Paramètres d'imprimante par défaut", "ja": "デフォルトのプリンター設定", "ko": "기본 프린터 설정"},
    "默认喷嘴直径": {"de": "Standard-Düsendurchmesser", "fr": "Diamètre de buse par défaut", "ja": "デフォルトのノズル径", "ko": "기본 노즐 지름"},
    "默认热床形状": {"de": "Standard-Druckbettform", "fr": "Forme de plateau par défaut", "ja": "デフォルトの熱床形状", "ko": "기본 베드 형태"},

    # ---- Setup wizard ----
    "首次配置向导": {"de": "Einrichtungsassistent", "fr": "Assistant de configuration initiale", "ja": "初期設定ウィザード", "ko": "초기 설정 마법사"},
    "选择打印机": {"de": "Drucker wählen", "fr": "Sélectionner l'imprimante", "ja": "プリンターを選択", "ko": "프린터 선택"},
    "选择耗材": {"de": "Filament wählen", "fr": "Sélectionner le filament", "ja": "フィラメントを選択", "ko": "필라멘트 선택"},
    "开始设置": {"de": "Einrichtung starten", "fr": "Démarrer la configuration", "ja": "セットアップを開始", "ko": "설정 시작"},
    "上一步": {"de": "Zurück", "fr": "Précédent", "ja": "前へ", "ko": "이전"},
    "下一步": {"de": "Weiter", "fr": "Suivant", "ja": "次へ", "ko": "다음"},
    "喷嘴直径:": {"de": "Düsendurchmesser:", "fr": "Diamètre de la buse :", "ja": "ノズル径:", "ko": "노즐 지름:"},
    "打印机:": {"de": "Drucker:", "fr": "Imprimante :", "ja": "プリンター:", "ko": "프린터:"},
    "热床:": {"de": "Druckbett:", "fr": "Plateau :", "ja": "熱床:", "ko": "베드:"},
    "耗材:": {"de": "Filament:", "fr": "Filament :", "ja": "フィラメント:", "ko": "필라멘트:"},

    # ---- Bed types / materials ----
    "光滑 PEI": {"de": "Glatte PEI", "fr": "PEI lisse", "ja": "平滑PEI", "ko": "평활 PEI"},
    "光滑 PEI 板": {"de": "Glatte PEI-Platte", "fr": "Plaque PEI lisse", "ja": "平滑PEIプレート", "ko": "평활 PEI 플레이트"},
    "普通 PEI 板": {"de": "Standard-PEI-Platte", "fr": "Plaque PEI standard", "ja": "標準PEIプレート", "ko": "표준 PEI 플레이트"},
    "纹理 PEI": {"de": "Strukturierte PEI", "fr": "PEI texturé", "ja": "テクスチャPEI", "ko": "텍스처 PEI"},
    "高温 PEI": {"de": "Hochtemperatur-PEI", "fr": "PEI haute température", "ja": "高温PEI", "ko": "고온 PEI"},
    "环氧树脂板": {"de": "Epoxidharz-Platte", "fr": "Plaque en époxy", "ja": "エポキシ板", "ko": "에폭시 보드"},
    "PC 热床": {"de": "PC-Druckbett", "fr": "Plateau PC", "ja": "PC熱床", "ko": "PC 베드"},
    "EP 热床": {"de": "EP-Druckbett", "fr": "Plateau EP", "ja": "EP熱床", "ko": "EP 베드"},

    # ---- Object list / parts ----
    "对象": {"de": "Objekt", "fr": "Objet", "ja": "オブジェクト", "ko": "객체"},
    "部件": {"de": "Teil", "fr": "Pièce", "ja": "パーツ", "ko": "부품"},
    "参数": {"de": "Parameter", "fr": "Paramètres", "ja": "パラメータ", "ko": "매개변수"},
    "图例": {"de": "Legende", "fr": "Légende", "ja": "凡例", "ko": "범례"},
    "当前盘": {"de": "Aktuelle Druckplatte", "fr": "Platine actuelle", "ja": "現在のプレート", "ko": "현재 플레이트"},
    "当前平板": {"de": "Aktuelle Druckplatte", "fr": "Platine actuelle", "ja": "現在のプレート", "ko": "현재 플레이트"},
    "按盘": {"de": "Nach Druckplatte", "fr": "Par platine", "ja": "プレート別", "ko": "플레이트별"},
    "按模块": {"de": "Nach Modul", "fr": "Par module", "ja": "モジュール別", "ko": "모듈별"},
    "添加部件": {"de": "Teil hinzufügen", "fr": "Ajouter une pièce", "ja": "パーツを追加", "ko": "부품 추가"},
    "添加模型...": {"de": "Modell hinzufügen...", "fr": "Ajouter un modèle...", "ja": "モデルを追加...", "ko": "모델 추가..."},
    "添加图元": {"de": "Primitiv hinzufügen", "fr": "Ajouter une primitive", "ja": "図形を追加", "ko": "도형 추가"},
    "添加原始体": {"de": "Primitiv hinzufügen", "fr": "Ajouter une primitive", "ja": "プリミティブを追加", "ko": "기본 도형 추가"},
    "添加文字": {"de": "Text hinzufügen", "fr": "Ajouter du texte", "ja": "テキストを追加", "ko": "텍스트 추가"},
    "添加文字浮雕": {"de": "Text-Prägung hinzufügen", "fr": "Ajouter un texte en relief", "ja": "テキストの浮き彫りを追加", "ko": "텍스트 엠보싱 추가"},
    "文字浮雕": {"de": "Text-Prägung", "fr": "Texte en relief", "ja": "テキストの浮き彫り", "ko": "텍스트 엠보싱"},
    "导入模型": {"de": "Modell importieren", "fr": "Importer un modèle", "ja": "モデルをインポート", "ko": "모델 가져오기"},
    "导入 SVG": {"de": "SVG importieren", "fr": "Importer un SVG", "ja": "SVGをインポート", "ko": "SVG 가져오기"},
    "导出模型": {"de": "Modell exportieren", "fr": "Exporter le modèle", "ja": "モデルをエクスポート", "ko": "모델 내보내기"},
    "导出为 STL": {"de": "Als STL exportieren", "fr": "Exporter en STL", "ja": "STLとしてエクスポート", "ko": "STL로 내보내기"},
    "导出为 STL...": {"de": "Als STL exportieren...", "fr": "Exporter en STL...", "ja": "STLとしてエクスポート...", "ko": "STL로 내보내기..."},
    "导出 G-code": {"de": "G-code exportieren", "fr": "Exporter le G-code", "ja": "G-codeをエクスポート", "ko": "G-code 내보내기"},
    "简化模型": {"de": "Modell vereinfachen", "fr": "Simplifier le modèle", "ja": "モデルを簡略化", "ko": "모델 단순화"},
    "修复网格": {"de": "Netz reparieren", "fr": "Réparer le maillage", "ja": "メッシュを修復", "ko": "메시 수리"},
    "修复模型": {"de": "Modell reparieren", "fr": "Réparer le modèle", "ja": "モデルを修復", "ko": "모델 수리"},
    "拆分": {"de": "Teilen", "fr": "Diviser", "ja": "分割", "ko": "분할"},
    "拆分为对象": {"de": "In Objekte teilen", "fr": "Diviser en objets", "ja": "オブジェクトに分割", "ko": "객체로 분할"},
    "拆分为部件": {"de": "In Teile teilen", "fr": "Diviser en pièces", "ja": "パーツに分割", "ko": "부품으로 분할"},
    "组合": {"de": "Gruppieren", "fr": "Grouper", "ja": "グループ化", "ko": "그룹화"},
    "转换为": {"de": "Umwandeln in", "fr": "Convertir en", "ja": "変換", "ko": "변환"},
    "修改器": {"de": "Modifikator", "fr": "Modificateur", "ja": "モディファイア", "ko": "수정자"},
    "负体积": {"de": "Negatives Volumen", "fr": "Volume négatif", "ja": "負のボリューム", "ko": "네거티브 볼륨"},
    "屏蔽": {"de": "Blocker", "fr": "Bloqueur", "ja": "ブロッカー", "ko": "차단"},
    "增强": {"de": "Erzwinger", "fr": "Forçage", "ja": "エンフォーサー", "ko": "강제"},
    "支撑屏蔽": {"de": "Stützstruktur-Blocker", "fr": "Bloqueur de support", "ja": "サポートブロッカー", "ko": "서포트 차단"},
    "支撑增强": {"de": "Stützstruktur-Erzwingung", "fr": "Forçage de support", "ja": "サポートエンフォーサー", "ko": "서포트 강제"},
    "对象信息": {"de": "Objektinfo", "fr": "Info de l'objet", "ja": "オブジェクト情報", "ko": "객체 정보"},
    "对象名称": {"de": "Objektname", "fr": "Nom de l'objet", "ja": "オブジェクト名", "ko": "객체 이름"},
    "更换耗材": {"de": "Filament wechseln", "fr": "Changer le filament", "ja": "フィラメントを交換", "ko": "필라멘트 교체"},

    # ---- Plates / bed operations ----
    "平板设置": {"de": "Druckplatten-Einstellungen", "fr": "Paramètres de la platine", "ja": "プレート設定", "ko": "플레이트 설정"},
    "平板名称": {"de": "Druckplattenname", "fr": "Nom de la platine", "ja": "プレート名", "ko": "플레이트 이름"},
    "清空平板": {"de": "Druckplatte leeren", "fr": "Vider la platine", "ja": "プレートをクリア", "ko": "플레이트 비우기"},
    "克隆平板": {"de": "Druckplatte duplizieren", "fr": "Cloner la platine", "ja": "プレートを複製", "ko": "플레이트 복제"},
    "删除平板": {"de": "Druckplatte löschen", "fr": "Supprimer la platine", "ja": "プレートを削除", "ko": "플레이트 삭제"},
    "重命名平板": {"de": "Druckplatte umbenennen", "fr": "Renommer la platine", "ja": "プレートの名前を変更", "ko": "플레이트 이름 변경"},
    "锁定平板": {"de": "Druckplatte sperren", "fr": "Verrouiller la platine", "ja": "プレートをロック", "ko": "플레이트 잠금"},
    "解锁平板": {"de": "Druckplatte entsperren", "fr": "Déverrouiller la platine", "ja": "プレートのロックを解除", "ko": "플레이트 잠금 해제"},
    "左移平板": {"de": "Druckplatte nach links", "fr": "Platine vers la gauche", "ja": "プレートを左へ移動", "ko": "플레이트 왼쪽으로"},
    "右移平板": {"de": "Druckplatte nach rechts", "fr": "Platine vers la droite", "ja": "プレートを右へ移動", "ko": "플레이트 오른쪽으로"},
    "排列对象": {"de": "Objekte anordnen", "fr": "Disposer les objets", "ja": "オブジェクトを配置", "ko": "객체 정렬"},
    "排列设置": {"de": "Anordnungseinstellungen", "fr": "Paramètres de disposition", "ja": "配置設定", "ko": "정렬 설정"},
    "对象间距": {"de": "Objektabstand", "fr": "Espacement des objets", "ja": "オブジェクト間隔", "ko": "객체 간격"},
    "自动旋转": {"de": "Automatisch drehen", "fr": "Rotation automatique", "ja": "自動回転", "ko": "자동 회전"},
    "铺满热床": {"de": "Druckbett füllen", "fr": "Remplir le plateau", "ja": "熱床に敷き詰める", "ko": "베드 채우기"},
    "打印序": {"de": "Druckreihenfolge", "fr": "Ordre d'impression", "ja": "印刷順序", "ko": "출력 순서"},
    "打印顺序": {"de": "Druckreihenfolge", "fr": "Ordre d'impression", "ja": "印刷順序", "ko": "출력 순서"},
    "按层打印": {"de": "Nach Schicht drucken", "fr": "Imprimer par couche", "ja": "層ごとにプリント", "ko": "레이어별 출력"},
    "按对象打印": {"de": "Nach Objekt drucken", "fr": "Imprimer par objet", "ja": "オブジェクトごとにプリント", "ko": "객체별 출력"},

    # ---- Slice result / statistics ----
    "切片结果": {"de": "Slice-Ergebnis", "fr": "Résultat du tranchage", "ja": "スライス結果", "ko": "슬라이스 결과"},
    "切片状态": {"de": "Slice-Status", "fr": "État du tranchage", "ja": "スライス状態", "ko": "슬라이스 상태"},
    "发送打印": {"de": "Zum Drucken senden", "fr": "Envoyer pour impression", "ja": "プリントに送信", "ko": "출력으로 보내기"},
    "全部切片": {"de": "Alle slicen", "fr": "Tout trancher", "ja": "すべてスライス", "ko": "전체 슬라이스"},
    "切片当前平板": {"de": "Aktuelle Druckplatte slicen", "fr": "Trancher la platine actuelle", "ja": "現在のプレートをスライス", "ko": "현재 플레이트 슬라이스"},
    "切片全部平板": {"de": "Alle Druckplatten slicen", "fr": "Trancher toutes les platines", "ja": "すべてのプレートをスライス", "ko": "전체 플레이트 슬라이스"},
    "统计": {"de": "Statistik", "fr": "Statistiques", "ja": "統計", "ko": "통계"},
    "打印统计": {"de": "Druckstatistik", "fr": "Statistiques d'impression", "ja": "プリント統計", "ko": "출력 통계"},
    "预估打印时间": {"de": "Geschätzte Druckzeit", "fr": "Temps d'impression estimé", "ja": "推定プリント時間", "ko": "예상 출력 시간"},
    "预估成本": {"de": "Geschätzte Kosten", "fr": "Coût estimé", "ja": "推定コスト", "ko": "예상 비용"},
    "预计成本": {"de": "Geschätzte Kosten", "fr": "Coût estimé", "ja": "推定コスト", "ko": "예상 비용"},
    "模型尺寸": {"de": "Modellgröße", "fr": "Taille du modèle", "ja": "モデルサイズ", "ko": "모델 크기"},
    "工具路径": {"de": "Werkzeugweg", "fr": "Trajectoire d'outil", "ja": "ツールパス", "ko": "툴 패스"},
    "移动路径": {"de": "Bewegungspfad", "fr": "Trajectoire de déplacement", "ja": "移動パス", "ko": "이동 경로"},
    "外壳轮廓": {"de": "Hüllenkontur", "fr": "Contour de la coque", "ja": "シェル輪郭", "ko": "쉘 윤곽선"},

    # ---- Measure / cut / gizmo tools ----
    "测量工具": {"de": "Messwerkzeug", "fr": "Outil de mesure", "ja": "測定ツール", "ko": "측정 도구"},
    "装配测量": {"de": "Montagemessung", "fr": "Mesure d'assemblage", "ja": "アセンブリ測定", "ko": "어셈블리 측정"},
    "切割对象": {"de": "Objekt schneiden", "fr": "Couper l'objet", "ja": "オブジェクトを切断", "ko": "객체 자르기"},
    "高级切割": {"de": "Erweitertes Schneiden", "fr": "Coupe avancée", "ja": "詳細切断", "ko": "고급 자르기"},
    "网格布尔运算": {"de": "Mesh-Boolesche Operation", "fr": "Opération booléenne de maillage", "ja": "メッシュブール演算", "ko": "메시 불 연산"},
    "面检测": {"de": "Flächenerkennung", "fr": "Détection de faces", "ja": "面検出", "ko": "면 감지"},
    "文字工具": {"de": "Textwerkzeug", "fr": "Outil de texte", "ja": "テキストツール", "ko": "텍스트 도구"},
    "SLA 支撑": {"de": "SLA-Stützstruktur", "fr": "Support SLA", "ja": "SLAサポート", "ko": "SLA 서포트"},
    "缝线绘制": {"de": "Naht-Zeichnung", "fr": "Peinture de couture", "ja": "シームペイント", "ko": "심 페인팅"},
    "支撑绘制": {"de": "Stützstruktur-Zeichnung", "fr": "Peinture de support", "ja": "サポートペイント", "ko": "서포트 페인팅"},

    # ---- Plugins / extras ----
    "插件管理": {"de": "Plugin-Verwaltung", "fr": "Gestion des plugins", "ja": "プラグイン管理", "ko": "플러그인 관리"},
    "已安装": {"de": "Installiert", "fr": "Installé", "ja": "インストール済み", "ko": "설치됨"},
    "安装": {"de": "Installieren", "fr": "Installer", "ja": "インストール", "ko": "설치"},
    "卸载": {"de": "Deinstallieren", "fr": "Désinstaller", "ja": "アンインストール", "ko": "제거"},
    "文档": {"de": "Dokumentation", "fr": "Documentation", "ja": "ドキュメント", "ko": "문서"},
    "功能": {"de": "Funktion", "fr": "Fonctionnalité", "ja": "機能", "ko": "기능"},
    "特性": {"de": "Funktion", "fr": "Fonctionnalité", "ja": "機能", "ko": "기능"},
    "开发者": {"de": "Entwickler", "fr": "Développeur", "ja": "開発者", "ko": "개발자"},

    # ---- Misc common labels ----
    "全部": {"de": "Alle", "fr": "Tout", "ja": "すべて", "ko": "전체"},
    "全部重新加载": {"de": "Alle neu laden", "fr": "Tout recharger", "ja": "すべて再読み込み", "ko": "전체 다시 로드"},
    "全部标为已读": {"de": "Alle als gelesen markieren", "fr": "Tout marquer comme lu", "ja": "すべて既読にする", "ko": "모두 읽음으로 표시"},
    "简单": {"de": "Einfach", "fr": "Simple", "ja": "シンプル", "ko": "간단"},
    "标准": {"de": "Standard", "fr": "Standard", "ja": "標準", "ko": "표준"},
    "静音": {"de": "Leise", "fr": "Silencieux", "ja": "静音", "ko": "조용히"},
    "未命名": {"de": "Unbenannt", "fr": "Sans titre", "ja": "無題", "ko": "제목 없음"},
    "每日提示": {"de": "Tipp des Tages", "fr": "Astuce du jour", "ja": "今日のヒント", "ko": "오늘의 팁"},
    "不再提示": {"de": "Nicht mehr anzeigen", "fr": "Ne plus afficher", "ja": "次回から表示しない", "ko": "다시 표시하지 않음"},
    "工作区警告": {"de": "Arbeitsbereich-Warnung", "fr": "Avertissement de l'espace de travail", "ja": "ワークスペース警告", "ko": "작업 공간 경고"},
    "工作区错误": {"de": "Arbeitsbereich-Fehler", "fr": "Erreur de l'espace de travail", "ja": "ワークスペースエラー", "ko": "작업 공간 오류"},
    "验证错误": {"de": "Validierungsfehler", "fr": "Erreur de validation", "ja": "検証エラー", "ko": "검증 오류"},
    "验证警告": {"de": "Validierungswarnung", "fr": "Avertissement de validation", "ja": "検証警告", "ko": "검증 경고"},
    "测试连接": {"de": "Verbindung testen", "fr": "Tester la connexion", "ja": "接続テスト", "ko": "연결 테스트"},
    "开始测试": {"de": "Test starten", "fr": "Démarrer le test", "ja": "テストを開始", "ko": "테스트 시작"},
    "网络测试": {"de": "Netzwerktest", "fr": "Test réseau", "ja": "ネットワークテスト", "ko": "네트워크 테스트"},
    "编辑": {"de": "Bearbeiten", "fr": "Modifier", "ja": "編集", "ko": "편집"},
    "编辑打印机预设": {"de": "Drucker-Voreinstellung bearbeiten", "fr": "Modifier le préréglage d'imprimante", "ja": "プリンタープリセットを編集", "ko": "프린터 사전 설정 편집"},
    "编辑耗材预设": {"de": "Filament-Voreinstellung bearbeiten", "fr": "Modifier le préréglage de filament", "ja": "フィラメントプリセットを編集", "ko": "필라멘트 사전 설정 편집"},
    "无认证": {"de": "Keine Authentifizierung", "fr": "Aucune authentification", "ja": "認証なし", "ko": "인증 없음"},
    "无信号": {"de": "Kein Signal", "fr": "Aucun signal", "ja": "信号なし", "ko": "신호 없음"},
    "网络已连接": {"de": "Netzwerk verbunden", "fr": "Réseau connecté", "ja": "ネットワーク接続あり", "ko": "네트워크 연결됨"},
    "网络未连接": {"de": "Netzwerk nicht verbunden", "fr": "Réseau non connecté", "ja": "ネットワーク未接続", "ko": "네트워크 연결 안 됨"},
    "未检测到打印机": {"de": "Kein Drucker erkannt", "fr": "Aucune imprimante détectée", "ja": "プリンターが検出されません", "ko": "프린터가 감지되지 않음"},
    "未找到匹配的设备": {"de": "Keine passenden Geräte gefunden", "fr": "Aucun appareil correspondant trouvé", "ja": "一致するデバイスが見つかりません", "ko": "일치하는 장치를 찾을 수 없음"},
    "暂无告警": {"de": "Keine Warnungen", "fr": "Aucune alerte", "ja": "アラートなし", "ko": "알림 없음"},
    "暂无通知记录": {"de": "Keine Benachrichtigungen", "fr": "Aucun enregistrement de notification", "ja": "通知記録なし", "ko": "알림 기록 없음"},
    "暂无图例数据": {"de": "Keine Legendendaten", "fr": "Aucune donnée de légende", "ja": "凡例データなし", "ko": "범례 데이터 없음"},
    "暂无校准历史记录": {"de": "Noch kein Kalibrierungsverlauf", "fr": "Pas encore d'historique d'étalonnage", "ja": "キャリブレーション履歴なし", "ko": "캘리브레이션 기록 없음"},
    "摄像头未连接": {"de": "Kamera nicht verbunden", "fr": "Caméra non connectée", "ja": "カメラが接続されていません", "ko": "카메라가 연결되지 않음"},
    "信号优秀": {"de": "Ausgezeichnetes Signal", "fr": "Excellent signal", "ja": "信号：優秀", "ko": "신호 양호"},
    "信号良好": {"de": "Gutes Signal", "fr": "Bon signal", "ja": "信号：良好", "ko": "신호 좋음"},
    "信号较弱": {"de": "Schwaches Signal", "fr": "Signal faible", "ja": "信号：弱い", "ko": "신호 약함"},
    "打印主机设置": {"de": "Druckhost-Einstellungen", "fr": "Paramètres de l'hôte d'impression", "ja": "プリントホスト設定", "ko": "출력 호스트 설정"},
    "预设名称": {"de": "Voreinstellungsname", "fr": "Nom du préréglage", "ja": "プリセット名", "ko": "사전 설정 이름"},
    "擦料塔设置": {"de": "Reinigungsturm-Einstellungen", "fr": "Paramètres de la tour de nettoyage", "ja": "ワイプタワー設定", "ko": "와이프 타워 설정"},
    "预设包 (*.json)": {"de": "Voreinstellungspaket (*.json)", "fr": "Pack de préréglages (*.json)", "ja": "プリセットパック (*.json)", "ko": "사전 설정 패키지 (*.json)"},
    "导出预设包": {"de": "Voreinstellungspaket exportieren", "fr": "Exporter le pack de préréglages", "ja": "プリセットパックをエクスポート", "ko": "사전 설정 패키지 내보내기"},

    # ---- CJK-only math/boolean / axis terms (high frequency) ----
    "并集 (Union)": {"de": "Vereinigung (Union)", "fr": "Union (Union)", "ja": "和集合 (Union)", "ko": "합집합 (Union)"},
    "差集 (Difference)": {"de": "Differenz (Difference)", "fr": "Différence (Difference)", "ja": "差集合 (Difference)", "ko": "차집합 (Difference)"},
    "交集 (Intersection)": {"de": "Schnittmenge (Intersection)", "fr": "Intersection (Intersection)", "ja": "積集合 (Intersection)", "ko": "교집합 (Intersection)"},
    "沿 X 轴": {"de": "Entlang der X-Achse", "fr": "Le long de l'axe X", "ja": "X軸に沿って", "ko": "X축을 따라"},
    "沿 Y 轴": {"de": "Entlang der Y-Achse", "fr": "Le long de l'axe Y", "ja": "Y軸に沿って", "ko": "Y축을 따라"},
    "沿 Z 轴": {"de": "Entlang der Z-Achse", "fr": "Le long de l'axe Z", "ja": "Z軸に沿って", "ko": "Z축을 따라"},
    "沿 X 轴镜像": {"de": "An X-Achse spiegeln", "fr": "Miroir selon l'axe X", "ja": "X軸でミラーリング", "ko": "X축 미러"},
    "沿 Y 轴镜像": {"de": "An Y-Achse spiegeln", "fr": "Miroir selon l'axe Y", "ja": "Y軸でミラーリング", "ko": "Y축 미러"},
    "沿 Z 轴镜像": {"de": "An Z-Achse spiegeln", "fr": "Miroir selon l'axe Z", "ja": "Z軸でミラーリング", "ko": "Z축 미러"},
    "X 轴": {"de": "X-Achse", "fr": "Axe X", "ja": "X軸", "ko": "X축"},
    "Y 轴": {"de": "Y-Achse", "fr": "Axe Y", "ja": "Y軸", "ko": "Y축"},
    "Z 轴": {"de": "Z-Achse", "fr": "Axe Z", "ja": "Z軸", "ko": "Z축"},
    "法线": {"de": "Normale", "fr": "Normale", "ja": "法線", "ko": "법선"},
    "文本": {"de": "Text", "fr": "Texte", "ja": "テキスト", "ko": "텍스트"},
    "文本内容": {"de": "Textinhalt", "fr": "Contenu du texte", "ja": "テキスト内容", "ko": "텍스트 내용"},
    "字号": {"de": "Schriftgröße", "fr": "Taille de police", "ja": "フォントサイズ", "ko": "글꼴 크기"},
    "原点偏移": {"de": "Ursprungsversatz", "fr": "Décalage de l'origine", "ja": "原点オフセット", "ko": "원점 오프셋"},
    "删除已选对象": {"de": "Ausgewählte Objekte löschen", "fr": "Supprimer les objets sélectionnés", "ja": "選択したオブジェクトを削除", "ko": "선택한 객체 삭제"},
    "删除已选部件": {"de": "Ausgewählte Teile löschen", "fr": "Supprimer les pièces sélectionnées", "ja": "選択したパーツを削除", "ko": "선택한 부품 삭제"},
    "选择全部对象": {"de": "Alle Objekte auswählen", "fr": "Sélectionner tous les objets", "ja": "すべてのオブジェクトを選択", "ko": "모든 객체 선택"},
    "居中到热床": {"de": "Auf Druckbett zentrieren", "fr": "Centrer sur le plateau", "ja": "熱床の中央に配置", "ko": "베드 중앙에 배치"},
    "平放至面": {"de": "Auf Fläche abflachen", "fr": "Mettre à plat sur une face", "ja": "面に平面配置", "ko": "면에 평평하게 배치"},
    "拆分为独立对象": {"de": "In separate Objekte teilen", "fr": "Diviser en objets distincts", "ja": "独立したオブジェクトに分割", "ko": "독립 객체로 분할"},
    "设为独立对象": {"de": "Als separates Objekt festlegen", "fr": "Définir comme objet distinct", "ja": "独立したオブジェクトに設定", "ko": "독립 객체로 설정"},
    "设为不参与打印": {"de": "Als nicht druckend festlegen", "fr": "Définir comme non imprimable", "ja": "印刷対象外に設定", "ko": "출력 안 함으로 설정"},
    "设为可打印": {"de": "Als druckbar festlegen", "fr": "Définir comme imprimable", "ja": "印刷可能に設定", "ko": "출력 가능으로 설정"},
    "设为不打印": {"de": "Als nicht druckend festlegen", "fr": "Définir comme non imprimable", "ja": "印刷しないに設定", "ko": "출력 안 함으로 설정"},
    "不参与打印": {"de": "Wird nicht gedruckt", "fr": "Non imprimé", "ja": "印刷対象外", "ko": "출력 안 함"},
    "禁打": {"de": "Nicht drucken", "fr": "Ne pas imprimer", "ja": "印刷しない", "ko": "출력 안 함"},
    "跟随全局": {"de": "Global folgen", "fr": "Suivre le global", "ja": "グローバルに従う", "ko": "전역 따르기"},
    "已用": {"de": "Benutzt", "fr": "Utilisé", "ja": "使用済み", "ko": "사용됨"},
    "分析": {"de": "Analysieren", "fr": "Analyser", "ja": "解析", "ko": "분석"},
    "搜索设备...": {"de": "Geräte suchen...", "fr": "Rechercher des appareils...", "ja": "デバイスを検索...", "ko": "장치 검색..."},
    "选择路径...": {"de": "Pfad wählen...", "fr": "Sélectionner le chemin...", "ja": "パスを選択...", "ko": "경로 선택..."},
    "选择占位符": {"de": "Platzhalter wählen", "fr": "Sélectionner un espace réservé", "ja": "プレースホルダーを選択", "ko": "자리 표시자 선택"},
    "连接方式": {"de": "Verbindungsart", "fr": "Méthode de connexion", "ja": "接続方法", "ko": "연결 방식"},
    "局域网直连": {"de": "LAN-Direktverbindung", "fr": "Connexion LAN directe", "ja": "LAN直接接続", "ko": "LAN 직접 연결"},
    "无绑定设备": {"de": "Keine gebundenen Geräte", "fr": "Aucun appareil lié", "ja": "バインドされたデバイスなし", "ko": "바인딩된 장치 없음"},
    "云端设备": {"de": "Cloud-Geräte", "fr": "Appareils cloud", "ja": "クラウドデバイス", "ko": "클라우드 장치"},
    "绑定设备": {"de": "Gerät binden", "fr": "Lier un appareil", "ja": "デバイスをバインド", "ko": "장치 바인딩"},
    "硬件校准选项": {"de": "Hardware-Kalibrierungsoptionen", "fr": "Options d'étalonnage matériel", "ja": "ハードウェアキャリブレーションオプション", "ko": "하드웨어 캘리브레이션 옵션"},
    "微型激光雷达校准": {"de": "LiDAR-Kalibrierung", "fr": "Étalonnage LiDAR", "ja": "LiDARキャリブレーション", "ko": "LiDAR 캘리브레이션"},
    "热床调平": {"de": "Druckbett-Nivellierung", "fr": "Nivellement du plateau", "ja": "熱床レベリング", "ko": "베드 레벨링"},
    "振动补偿": {"de": "Schwingungskompensation", "fr": "Compensation des vibrations", "ja": "振動補正", "ko": "진동 보정"},
    "电机降噪": {"de": "Motorentlärmung", "fr": "Réduction du bruit du moteur", "ja": "モーター静音化", "ko": "모터 소음 저감"},
    "设备健康监控 (HMS)": {"de": "Geräte-Health-Monitoring (HMS)", "fr": "Surveillance de santé de l'appareil (HMS)", "ja": "デバイスヘルスモニタリング (HMS)", "ko": "장치 상태 모니터링 (HMS)"},
    "设备排错": {"de": "Geräte-Fehlerbehebung", "fr": "Dépannage de l'appareil", "ja": "デバイストラブルシューティング", "ko": "장치 문제 해결"},
    "打印空间": {"de": "Druckraum", "fr": "Volume d'impression", "ja": "プリント空間", "ko": "출력 공간"},
    "打印体积": {"de": "Druckvolumen", "fr": "Volume d'impression", "ja": "プリントボリューム", "ko": "출력 부피"},
    "运动能力": {"de": "Bewegungsfähigkeit", "fr": "Capacité de mouvement", "ja": "運動能力", "ko": "운동 능력"},
    "移动能力": {"de": "Beweglichkeit", "fr": "Mobilité", "ja": "移動能力", "ko": "이동 능력"},
    "基础信息": {"de": "Grundinformationen", "fr": "Informations de base", "ja": "基本情報", "ko": "기본 정보"},
    "打印机G-code": {"de": "Drucker-G-code", "fr": "G-code de l'imprimante", "ja": "プリンターG-code", "ko": "프린터 G-code"},
    "参数覆盖": {"de": "Parameter-Überschreibung", "fr": "Remplacement de paramètres", "ja": "パラメータオーバーライド", "ko": "매개변수 재정의"},
    "依赖": {"de": "Abhängigkeit", "fr": "Dépendance", "ja": "依存関係", "ko": "종속성"},
    "注释": {"de": "Kommentar", "fr": "Commentaire", "ja": "コメント", "ko": "주석"},
    "线型可见性": {"de": "Linentyp-Sichtbarkeit", "fr": "Visibilité du type de ligne", "ja": "線種の表示", "ko": "선 유형 표시"},
    "显示空驶": {"de": "Eilgang anzeigen", "fr": "Afficher les déplacements", "ja": "移動を表示", "ko": "이동 표시"},
    "显示热床": {"de": "Druckbett anzeigen", "fr": "Afficher le plateau", "ja": "熱床を表示", "ko": "베드 표시"},
    "显示工具位置": {"de": "Werkzeugposition anzeigen", "fr": "Afficher la position de l'outil", "ja": "ツール位置を表示", "ko": "툴 위치 표시"},
    "总时间": {"de": "Gesamtzeit", "fr": "Temps total", "ja": "総時間", "ko": "총 시간"},
    "挤出移动": {"de": "Extrusionsbewegungen", "fr": "Mouvements d'extrusion", "ja": "押し出し移動", "ko": "압출 이동"},
    "空驶移动": {"de": "Eilgangbewegungen", "fr": "Déplacements", "ja": "移動", "ko": "이동"},
    "工具切换": {"de": "Werkzeugwechsel", "fr": "Changements d'outil", "ja": "ツール切り替え", "ko": "툴 전환"},
    "按类型耗时": {"de": "Zeit nach Typ", "fr": "Temps par type", "ja": "タイプ別時間", "ko": "유형별 시간"},
    "层时间分布": {"de": "Schichtzeit-Verteilung", "fr": "Distribution du temps par couche", "ja": "層時間の分布", "ko": "레이어 시간 분포"},
    "最短": {"de": "Kürzeste", "fr": "Le plus court", "ja": "最短", "ko": "최단"},
    "平均": {"de": "Durchschnitt", "fr": "Moyenne", "ja": "平均", "ko": "평균"},
    "最长": {"de": "Längste", "fr": "Le plus long", "ja": "最長", "ko": "최장"},
    "范围最小": {"de": "Min. Bereich", "fr": "Plage min.", "ja": "範囲最小", "ko": "범위 최소"},
    "范围最大": {"de": "Max. Bereich", "fr": "Plage max.", "ja": "範囲最大", "ko": "범위 최대"},
    "速度与加速度限制": {"de": "Geschwindigkeits- und Beschleunigungsgrenzen", "fr": "Limites de vitesse et d'accélération", "ja": "速度と加速度の制限", "ko": "속도 및 가속도 제한"},
    "对象: ": {"de": "Objekt: ", "fr": "Objet : ", "ja": "オブジェクト: ", "ko": "객체: "},
    "体积: ": {"de": "Volumen: ", "fr": "Volume : ", "ja": "体積: ", "ko": "부피: "},
    "角度: ": {"de": "Winkel: ", "fr": "Angle : ", "ja": "角度: ", "ko": "각도: "},
    "垂直距离: ": {"de": "Senkrechte Entfernung: ", "fr": "Distance perpendiculaire : ", "ja": "垂直距離: ", "ko": "수직 거리: "},
    "直线距离: ": {"de": "Direkte Entfernung: ", "fr": "Distance directe : ", "ja": "直線距離: ", "ko": "직선 거리: "},
    "类型:": {"de": "Typ:", "fr": "Type :", "ja": "タイプ:", "ko": "유형:"},
    "样式:": {"de": "Stil:", "fr": "Style :", "ja": "スタイル:", "ko": "스타일:"},
    "形状:": {"de": "Form:", "fr": "Forme :", "ja": "形状:", "ko": "모양:"},
    "尺寸:": {"de": "Größe:", "fr": "Taille :", "ja": "サイズ:", "ko": "크기:"},
    "深度:": {"de": "Tiefe:", "fr": "Profondeur :", "ja": "深さ:", "ko": "깊이:"},
    "位置:": {"de": "Position:", "fr": "Position :", "ja": "位置:", "ko": "위치:"},
    "半径:": {"de": "Radius:", "fr": "Rayon :", "ja": "半径:", "ko": "반지름:"},
    "偏移:": {"de": "Versatz:", "fr": "Décalage :", "ja": "オフセット:", "ko": "오프셋:"},
    "当前面数:": {"de": "Aktuelle Flächen:", "fr": "Faces actuelles :", "ja": "現在の面数:", "ko": "현재 면 수:"},
    "目标面数:": {"de": "Ziel-Flächen:", "fr": "Faces cibles :", "ja": "目標面数:", "ko": "목표 면 수:"},
    "最大误差:": {"de": "Max. Fehler:", "fr": "Erreur max. :", "ja": "最大誤差:", "ko": "최대 오차:"},
    "光标:": {"de": "Cursor:", "fr": "Curseur :", "ja": "カーソル:", "ko": "커서:"},
    "分辨率:": {"de": "Auflösung:", "fr": "Résolution :", "ja": "解像度:", "ko": "해상도:"},
    "输入用户名": {"de": "Benutzername eingeben", "fr": "Saisir le nom d'utilisateur", "ja": "ユーザー名を入力", "ko": "사용자 이름 입력"},
    "输入密码": {"de": "Passwort eingeben", "fr": "Saisir le mot de passe", "ja": "パスワードを入力", "ko": "비밀번호 입력"},
    "PIN 码": {"de": "PIN-Code", "fr": "Code PIN", "ja": "PINコード", "ko": "PIN 코드"},
    "认证方式": {"de": "Authentifizierungsmethode", "fr": "Méthode d'authentification", "ja": "認証方式", "ko": "인증 방식"},
    "主机类型": {"de": "Host-Typ", "fr": "Type d'hôte", "ja": "ホストタイプ", "ko": "호스트 유형"},
    "主机地址": {"de": "Host-Adresse", "fr": "Adresse de l'hôte", "ja": "ホストアドレス", "ko": "호스트 주소"},
    "用户名/密码": {"de": "Benutzername/Passwort", "fr": "Nom d'utilisateur/Mot de passe", "ja": "ユーザー名/パスワード", "ko": "사용자 이름/비밀번호"},
    "需要有效的网络连接": {"de": "Eine gültige Netzwerkverbindung ist erforderlich", "fr": "Une connexion réseau valide est requise", "ja": "有効なネットワーク接続が必要です", "ko": "유효한 네트워크 연결이 필요합니다"},
    "显示标签": {"de": "Beschriftungen anzeigen", "fr": "Afficher les étiquettes", "ja": "ラベルを表示", "ko": "레이블 표시"},
    "隐藏标签": {"de": "Beschriftungen ausblenden", "fr": "Masquer les étiquettes", "ja": "ラベルを非表示", "ko": "레이블 숨기기"},
    "适应视图": {"de": "Ansicht anpassen", "fr": "Ajuster la vue", "ja": "表示に合わせる", "ko": "보기 맞춤"},
    "俯视": {"de": "Draufsicht", "fr": "Vue de dessus", "ja": "上面図", "ko": "상단 보기"},
    "前视": {"de": "Vorderansicht", "fr": "Vue de face", "ja": "正面図", "ko": "정면 보기"},
    "右视": {"de": "Seitenansicht", "fr": "Vue de droite", "ja": "右側面図", "ko": "우측 보기"},
    "等轴视": {"de": "Isometrische Ansicht", "fr": "Vue isométrique", "ja": "等角投影図", "ko": "등각 투영"},
    "等轴": {"de": "Isometrisch", "fr": "Isométrique", "ja": "等角", "ko": "등각"},
    "顶": {"de": "Oben", "fr": "Dessus", "ja": "上", "ko": "상단"},
    "前": {"de": "Vorne", "fr": "Avant", "ja": "前", "ko": "전방"},
    "右": {"de": "Rechts", "fr": "Droite", "ja": "右", "ko": "우측"},
    "移动模式": {"de": "Bewegungsmodus", "fr": "Mode déplacement", "ja": "移動モード", "ko": "이동 모드"},
    "旋转模式": {"de": "Rotationsmodus", "fr": "Mode rotation", "ja": "回転モード", "ko": "회전 모드"},
    "缩放模式": {"de": "Skalierungsmodus", "fr": "Mode mise à l'échelle", "ja": "スケールモード", "ko": "크기 조정 모드"},
    "打开项目": {"de": "Projekt öffnen", "fr": "Ouvrir un projet", "ja": "プロジェクトを開く", "ko": "프로젝트 열기"},
    "另存为": {"de": "Speichern unter", "fr": "Enregistrer sous", "ja": "名前を付けて保存", "ko": "다른 이름으로 저장"},
    "打开模型文件": {"de": "Modelldatei öffnen", "fr": "Ouvrir un fichier de modèle", "ja": "モデルファイルを開く", "ko": "모델 파일 열기"},
    "所有文件 (*)": {"de": "Alle Dateien (*)", "fr": "Tous les fichiers (*)", "ja": "すべてのファイル (*)", "ko": "모든 파일 (*)"},
    "3MF 文件 (*.3mf)": {"de": "3MF-Dateien (*.3mf)", "fr": "Fichiers 3MF (*.3mf)", "ja": "3MFファイル (*.3mf)", "ko": "3MF 파일 (*.3mf)"},
    "STL 文件 (*.stl)": {"de": "STL-Dateien (*.stl)", "fr": "Fichiers STL (*.stl)", "ja": "STLファイル (*.stl)", "ko": "STL 파일 (*.stl)"},
    "OBJ 文件 (*.obj)": {"de": "OBJ-Dateien (*.obj)", "fr": "Fichiers OBJ (*.obj)", "ja": "OBJファイル (*.obj)", "ko": "OBJ 파일 (*.obj)"},
    "STEP 文件 (*.step *.stp)": {"de": "STEP-Dateien (*.step *.stp)", "fr": "Fichiers STEP (*.step *.stp)", "ja": "STEPファイル (*.step *.stp)", "ko": "STEP 파일 (*.step *.stp)"},
    "AMF 文件 (*.amf)": {"de": "AMF-Dateien (*.amf)", "fr": "Fichiers AMF (*.amf)", "ja": "AMFファイル (*.amf)", "ko": "AMF 파일 (*.amf)"},
    "G-code 文件 (*.gcode)": {"de": "G-code-Dateien (*.gcode)", "fr": "Fichiers G-code (*.gcode)", "ja": "G-codeファイル (*.gcode)", "ko": "G-code 파일 (*.gcode)"},
    "从 STL 导入": {"de": "Aus STL importieren", "fr": "Importer depuis STL", "ja": "STLからインポート", "ko": "STL에서 가져오기"},
    "从文件导入部件...": {"de": "Teil aus Datei importieren...", "fr": "Importer une pièce depuis un fichier...", "ja": "ファイルからパーツをインポート...", "ko": "파일에서 부품 가져오기..."},
    "导入 SVG 浮雕...": {"de": "SVG-Prägung importieren...", "fr": "Importer un SVG en relief...", "ja": "SVGの浮き彫りをインポート...", "ko": "SVG 엠보싱 가져오기..."},
    "替换为 STL...": {"de": "Durch STL ersetzen...", "fr": "Remplacer par STL...", "ja": "STLで置き換え...", "ko": "STL로 교체..."},
    "替换为 STL": {"de": "Durch STL ersetzen", "fr": "Remplacer par STL", "ja": "STLで置き換え", "ko": "STL로 교체"},
    "项目文件 (*.3mf *.cxprj *.json)": {"de": "Projektdateien (*.3mf *.cxprj *.json)", "fr": "Fichiers de projet (*.3mf *.cxprj *.json)", "ja": "プロジェクトファイル (*.3mf *.cxprj *.json)", "ko": "프로젝트 파일 (*.3mf *.cxprj *.json)"},
    "项目文件 (*.3mf *.cxprj)": {"de": "Projektdateien (*.3mf *.cxprj)", "fr": "Fichiers de projet (*.3mf *.cxprj)", "ja": "プロジェクトファイル (*.3mf *.cxprj)", "ko": "프로젝트 파일 (*.3mf *.cxprj)"},
    "项目元数据 (*.json)": {"de": "Projekt-Metadaten (*.json)", "fr": "Métadonnées de projet (*.json)", "ja": "プロジェクトメタデータ (*.json)", "ko": "프로젝트 메타데이터 (*.json)"},
    "编辑自定义 G-code (%1)": {"de": "Benutzerdefinierten G-code bearbeiten (%1)", "fr": "Modifier le G-code personnalisé (%1)", "ja": "カスタムG-codeを編集 (%1)", "ko": "사용자 지정 G-code 편집 (%1)"},
    "选择 SVG 文件...": {"de": "SVG-Datei wählen...", "fr": "Sélectionner un fichier SVG...", "ja": "SVGファイルを選択...", "ko": "SVG 파일 선택..."},
    "选择 SVG 文件": {"de": "SVG-Datei wählen", "fr": "Sélectionner un fichier SVG", "ja": "SVGファイルを選択", "ko": "SVG 파일 선택"},
    "选择 STL 文件替换部件": {"de": "STL-Datei zum Ersetzen des Teils wählen", "fr": "Sélectionner un fichier STL pour remplacer la pièce", "ja": "パーツを置き換えるSTLファイルを選択", "ko": "부품을 교체할 STL 파일 선택"},
    "选择模型文件导入为部件": {"de": "Modelldatei zum Import als Teil wählen", "fr": "Sélectionner un fichier de modèle à importer comme pièce", "ja": "パーツとしてインポートするモデルファイルを選択", "ko": "부품으로 가져올 모델 파일 선택"},
    "请输入文字...": {"de": "Bitte Text eingeben...", "fr": "Veuillez saisir le texte...", "ja": "テキストを入力...", "ko": "텍스트를 입력..."},
    "输入浮雕文字": {"de": "Prägetext eingeben", "fr": "Saisir le texte en relief", "ja": "浮き彫りテキストを入力", "ko": "엠보싱 텍스트 입력"},
    "输入新名称:": {"de": "Neuen Namen eingeben:", "fr": "Saisir le nouveau nom :", "ja": "新しい名前を入力:", "ko": "새 이름 입력:"},
    "编辑参数表": {"de": "Einstellungstabelle bearbeiten", "fr": "Modifier la table des paramètres", "ja": "パラメータ表を編集", "ko": "매개변수 표 편집"},
    "在参数表中编辑": {"de": "In Einstellungstabelle bearbeiten", "fr": "Modifier dans la table des paramètres", "ja": "パラメータ表で編集", "ko": "매개변수 표에서 편집"},
    "查看所在平板": {"de": "Zugehörige Druckplatte anzeigen", "fr": "Afficher la platine associée", "ja": "所属プレートを表示", "ko": "소속 플레이트 보기"},
    "删除已选对象": {"de": "Ausgewählte Objekte löschen", "fr": "Supprimer les objets sélectionnés", "ja": "選択したオブジェクトを削除", "ko": "선택한 객체 삭제"},
}


def get_lang_dict(lang):
    """Build {source_key: translation} for the given language."""
    out = {}
    for src, langs in CORE.items():
        if lang in langs:
            out[src] = langs[lang]
    return out


def translate_file(lang):
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

    # SAME proven regex pattern as translate_en_ts.py. The gap group forbids
    # <source>, </source>, </message> so the match cannot cross message
    # boundaries (each message has exactly one <source> + one <translation>).
    pattern = re.compile(
        r'(<source>((?:(?!<source>|</message>).)*?)</source>'
        r'(?:(?!<source>|</source>|</message>).)*?<translation\s+)'
        r'type="unfinished"[^>]*>([^<]*)</translation>',
        re.DOTALL,
    )

    translated_count = 0

    def lookup(source):
        """Look up translation, tolerating CRLF vs LF differences."""
        tr = translations.get(source)
        if tr is not None:
            return tr
        if '\r' in source:
            tr = translations.get(source.replace('\r\n', '\n').replace('\r', '\n'))
        return tr

    def repl(m):
        nonlocal translated_count
        prefix = m.group(1)          # '<source>...</source> ... <translation '
        source = m.group(2)          # raw (escaped) source text
        tr = lookup(source)
        if tr is None:
            return m.group(0)        # leave untouched
        translated_count += 1
        tr_escaped = saxutils.escape(tr)
        return '%s>%s</translation>' % (prefix, tr_escaped)

    new_content = pattern.sub(repl, content)

    unfinished_after = new_content.count('type="unfinished"')
    print('[%s] Translated entries: %d' % (lang, translated_count))
    print('[%s] Unfinished after: %d' % (lang, unfinished_after))
    remaining_pct = (unfinished_after / unfinished_before * 100.0) if unfinished_before else 0.0
    print('[%s] Remaining: %d/%d (%.1f%%)' % (lang, unfinished_after, unfinished_before, remaining_pct))

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


def main():
    if len(sys.argv) < 2 or sys.argv[1] not in LANGS + ('all',):
        print('Usage: python scripts/translate_core_i18n.py <%s|all>' % '|'.join(LANGS))
        return 2

    target = sys.argv[1]
    if target == 'all':
        rc = 0
        for lang in LANGS:
            rc = translate_file(lang) or rc
            print()
        return rc
    return translate_file(target)


if __name__ == '__main__':
    sys.exit(main())
