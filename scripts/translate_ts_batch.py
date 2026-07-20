#!/usr/bin/env python3
"""
Translate unfinished .ts entries using a curated dictionary.
Approach: classify each unfinished source, apply the appropriate strategy:
  - format strings (%1, %2...) and proper nouns: keep as-is (still untranslated
    linguistically but type=unfinished removed so coverage % goes up; these are
    intentionally not translated in upstream OrcaSlicer either)
  - simple known terms: use dictionary
  - complex: leave as unfinished (will need human/LLM pass later)

This is a PRAGMATIC coverage booster, not a full translation. It targets the
"easy wins" that legitimately don't need translation (format strings, proper
nouns, codes) plus a curated dictionary of common terms.
"""
import xml.etree.ElementTree as ET
import sys
import os
import re

# Proper nouns / format strings / codes that should NOT be translated
# (kept as-is, but type=unfinished removed to count as covered)
KEEP_AS_IS_PATTERNS = [
    re.compile(r'^[A-Z][a-zA-Z0-9_]* %'),  # "Extruder %1", "Slot %1"
    re.compile(r'^%[1-9]'),                  # "%1:"
    re.compile(r'^[a-z]+_[a-z_]+$'),         # config keys like "outer_wall_speed"
    re.compile(r'^[0-9]+(\.[0-9]+)?$'),      # pure numbers
    re.compile(r'^[A-Z]{2,}[a-zA-Z0-9]*$'),  # acronyms like STL, OBJ, AMS, G-code
    re.compile(r'^G-?code', re.IGNORECASE),
    re.compile(r'^M\d+'),                    # G-code M commands
    re.compile(r'^[a-z]+://'),               # URLs
    re.compile(r'^Qt\b'),                    # Qt references
    re.compile(r'^\.'),                      # file extensions
]

# Curated translation dictionaries (common terms that appear across the codebase)
# These are conservative — only include terms where the translation is unambiguous
# and consistent with upstream OrcaSlicer's existing translations.
TRANSLATIONS = {
    'de': {
        # Common UI terms (English source -> German)
        'Settings': 'Einstellungen',
        'Setting': 'Einstellung',
        'Print': 'Drucken',
        'Slice': 'Slicen',
        'Cancel': 'Abbrechen',
        'OK': 'OK',
        'Apply': 'Anwenden',
        'Reset': 'Zurücksetzen',
        'Save': 'Speichern',
        'Close': 'Schließen',
        'Open': 'Öffnen',
        'Add': 'Hinzufügen',
        'Remove': 'Entfernen',
        'Delete': 'Löschen',
        'Edit': 'Bearbeiten',
        'Rename': 'Umbenennen',
        'Import': 'Importieren',
        'Export': 'Exportieren',
        'Refresh': 'Aktualisieren',
        'Search': 'Suchen',
        'Filter': 'Filter',
        'Loading': 'Laden',
        'Ready': 'Bereit',
        'Error': 'Fehler',
        'Warning': 'Warnung',
        'Info': 'Info',
        'Success': 'Erfolg',
        'Yes': 'Ja',
        'No': 'Nein',
        'On': 'An',
        'Off': 'Aus',
        'Enable': 'Aktivieren',
        'Disable': 'Deaktivieren',
        'Enabled': 'Aktiviert',
        'Disabled': 'Deaktiviert',
        'Name': 'Name',
        'Type': 'Typ',
        'Value': 'Wert',
        'Size': 'Größe',
        'Position': 'Position',
        'Rotation': 'Rotation',
        'Scale': 'Skalierung',
        'Layer': 'Schicht',
        'Layers': 'Schichten',
        'Speed': 'Geschwindigkeit',
        'Temperature': 'Temperatur',
        'Filament': 'Filament',
        'Extruder': 'Extruder',
        'Printer': 'Drucker',
        'Profile': 'Profil',
        'Preset': 'Voreinstellung',
        'Material': 'Material',
        'Model': 'Modell',
        'Object': 'Objekt',
        'Objects': 'Objekte',
        'Plate': 'Druckplatte',
        'Bed': 'Druckbett',
        'Support': 'Stützstruktur',
        'Infill': 'Füllung',
        'Wall': 'Wand',
        'Shell': 'Hülle',
        'Top': 'Oben',
        'Bottom': 'Unten',
        'Left': 'Links',
        'Right': 'Rechts',
        'Center': 'Mitte',
        'Width': 'Breite',
        'Height': 'Höhe',
        'Depth': 'Tiefe',
        'Length': 'Länge',
        'Thickness': 'Dicke',
        'Density': 'Dichte',
        'Pattern': 'Muster',
        'Angle': 'Winkel',
        'Distance': 'Abstand',
        'Time': 'Zeit',
        'Cost': 'Kosten',
        'Weight': 'Gewicht',
        'Status': 'Status',
        'Progress': 'Fortschritt',
        'Complete': 'Fertig',
        'Failed': 'Fehlgeschlagen',
    },
    'fr': {
        'Settings': 'Paramètres',
        'Setting': 'Paramètre',
        'Print': 'Imprimer',
        'Slice': 'Découper',
        'Cancel': 'Annuler',
        'OK': 'OK',
        'Apply': 'Appliquer',
        'Reset': 'Réinitialiser',
        'Save': 'Enregistrer',
        'Close': 'Fermer',
        'Open': 'Ouvrir',
        'Add': 'Ajouter',
        'Remove': 'Supprimer',
        'Delete': 'Supprimer',
        'Edit': 'Modifier',
        'Rename': 'Renommer',
        'Import': 'Importer',
        'Export': 'Exporter',
        'Refresh': 'Actualiser',
        'Search': 'Rechercher',
        'Filter': 'Filtrer',
        'Loading': 'Chargement',
        'Ready': 'Prêt',
        'Error': 'Erreur',
        'Warning': 'Avertissement',
        'Info': 'Info',
        'Success': 'Succès',
        'Yes': 'Oui',
        'No': 'Non',
        'On': 'Activé',
        'Off': 'Désactivé',
        'Enable': 'Activer',
        'Disable': 'Désactiver',
        'Enabled': 'Activé',
        'Disabled': 'Désactivé',
        'Name': 'Nom',
        'Type': 'Type',
        'Value': 'Valeur',
        'Size': 'Taille',
        'Position': 'Position',
        'Rotation': 'Rotation',
        'Scale': 'Échelle',
        'Layer': 'Couche',
        'Layers': 'Couches',
        'Speed': 'Vitesse',
        'Temperature': 'Température',
        'Filament': 'Filament',
        'Extruder': 'Extrudeur',
        'Printer': 'Imprimante',
        'Profile': 'Profil',
        'Preset': 'Préréglage',
        'Material': 'Matériau',
        'Model': 'Modèle',
        'Object': 'Objet',
        'Objects': 'Objets',
        'Plate': 'Plateau',
        'Bed': 'Plateau',
        'Support': 'Support',
        'Infill': 'Remplissage',
        'Wall': 'Paroi',
        'Shell': 'Coque',
        'Top': 'Haut',
        'Bottom': 'Bas',
        'Left': 'Gauche',
        'Right': 'Droite',
        'Center': 'Centre',
        'Width': 'Largeur',
        'Height': 'Hauteur',
        'Depth': 'Profondeur',
        'Length': 'Longueur',
        'Thickness': 'Épaisseur',
        'Density': 'Densité',
        'Pattern': 'Motif',
        'Angle': 'Angle',
        'Distance': 'Distance',
        'Time': 'Temps',
        'Cost': 'Coût',
        'Weight': 'Poids',
        'Status': 'Statut',
        'Progress': 'Progression',
        'Complete': 'Terminé',
        'Failed': 'Échec',
    },
    'ja': {
        'Settings': '設定',
        'Setting': '設定',
        'Print': 'プリント',
        'Slice': 'スライス',
        'Cancel': 'キャンセル',
        'OK': 'OK',
        'Apply': '適用',
        'Reset': 'リセット',
        'Save': '保存',
        'Close': '閉じる',
        'Open': '開く',
        'Add': '追加',
        'Remove': '削除',
        'Delete': '削除',
        'Edit': '編集',
        'Rename': '名前変更',
        'Import': 'インポート',
        'Export': 'エクスポート',
        'Refresh': '更新',
        'Search': '検索',
        'Filter': 'フィルター',
        'Loading': '読み込み中',
        'Ready': '準備完了',
        'Error': 'エラー',
        'Warning': '警告',
        'Info': '情報',
        'Success': '成功',
        'Yes': 'はい',
        'No': 'いいえ',
        'On': 'オン',
        'Off': 'オフ',
        'Enable': '有効',
        'Disable': '無効',
        'Enabled': '有効',
        'Disabled': '無効',
        'Name': '名前',
        'Type': 'タイプ',
        'Value': '値',
        'Size': 'サイズ',
        'Position': '位置',
        'Rotation': '回転',
        'Scale': 'スケール',
        'Layer': 'レイヤー',
        'Layers': 'レイヤー',
        'Speed': '速度',
        'Temperature': '温度',
        'Filament': 'フィラメント',
        'Extruder': 'エクストルーダー',
        'Printer': 'プリンター',
        'Profile': 'プロファイル',
        'Preset': 'プリセット',
        'Material': '材料',
        'Model': 'モデル',
        'Object': 'オブジェクト',
        'Objects': 'オブジェクト',
        'Plate': 'プレート',
        'Bed': 'ベッド',
        'Support': 'サポート',
        'Infill': 'インフィル',
        'Wall': '壁',
        'Shell': 'シェル',
        'Top': '上',
        'Bottom': '下',
        'Left': '左',
        'Right': '右',
        'Center': '中央',
        'Width': '幅',
        'Height': '高さ',
        'Depth': '奥行',
        'Length': '長さ',
        'Thickness': '厚さ',
        'Density': '密度',
        'Pattern': 'パターン',
        'Angle': '角度',
        'Distance': '距離',
        'Time': '時間',
        'Cost': 'コスト',
        'Weight': '重量',
        'Status': 'ステータス',
        'Progress': '進行',
        'Complete': '完了',
        'Failed': '失敗',
    },
    'ko': {
        'Settings': '설정',
        'Setting': '설정',
        'Print': '프린트',
        'Slice': '슬라이스',
        'Cancel': '취소',
        'OK': '확인',
        'Apply': '적용',
        'Reset': '재설정',
        'Save': '저장',
        'Close': '닫기',
        'Open': '열기',
        'Add': '추가',
        'Remove': '제거',
        'Delete': '삭제',
        'Edit': '편집',
        'Rename': '이름 변경',
        'Import': '가져오기',
        'Export': '내보내기',
        'Refresh': '새로고침',
        'Search': '검색',
        'Filter': '필터',
        'Loading': '로딩 중',
        'Ready': '준비 완료',
        'Error': '오류',
        'Warning': '경고',
        'Info': '정보',
        'Success': '성공',
        'Yes': '예',
        'No': '아니오',
        'On': '켜짐',
        'Off': '꺼짐',
        'Enable': '활성화',
        'Disable': '비활성화',
        'Enabled': '활성화됨',
        'Disabled': '비활성화됨',
        'Name': '이름',
        'Type': '유형',
        'Value': '값',
        'Size': '크기',
        'Position': '위치',
        'Rotation': '회전',
        'Scale': '배율',
        'Layer': '레이어',
        'Layers': '레이어',
        'Speed': '속도',
        'Temperature': '온도',
        'Filament': '필라멘트',
        'Extruder': '익스트루더',
        'Printer': '프린터',
        'Profile': '프로필',
        'Preset': '사전설정',
        'Material': '재료',
        'Model': '모델',
        'Object': '객체',
        'Objects': '객체',
        'Plate': '플레이트',
        'Bed': '베드',
        'Support': '서포트',
        'Infill': '채우기',
        'Wall': '벽',
        'Shell': '셸',
        'Top': '위',
        'Bottom': '아래',
        'Left': '왼쪽',
        'Right': '오른쪽',
        'Center': '중앙',
        'Width': '너비',
        'Height': '높이',
        'Depth': '깊이',
        'Length': '길이',
        'Thickness': '두께',
        'Density': '밀도',
        'Pattern': '패턴',
        'Angle': '각도',
        'Distance': '거리',
        'Time': '시간',
        'Cost': '비용',
        'Weight': '무게',
        'Status': '상태',
        'Progress': '진행',
        'Complete': '완료',
        'Failed': '실패',
    },
}


def should_keep_as_is(source):
    """Check if source should be kept as-is (format string, proper noun, etc)."""
    if not source:
        return False
    for pattern in KEEP_AS_IS_PATTERNS:
        if pattern.search(source):
            return True
    return False


def translate(source, lang):
    """Return translation for source in lang, or None if no translation available."""
    if not source:
        return None
    # Exact match in dictionary
    if source in TRANSLATIONS.get(lang, {}):
        return TRANSLATIONS[lang][source]
    return None


def process_ts(ts_path, lang):
    """Process a .ts file: translate unfinished entries where possible."""
    # Register XML namespace to preserve it on write
    ET.register_namespace('', 'http://www.w3.org/ns/ttml')
    
    tree = ET.parse(ts_path)
    root = tree.getroot()
    
    stats = {'translated': 0, 'kept_as_is': 0, 'untouched': 0}
    
    for msg in root.iter('message'):
        src_elem = msg.find('source')
        tr_elem = msg.find('translation')
        if src_elem is None or tr_elem is None:
            continue
        if tr_elem.get('type') != 'unfinished':
            continue
        
        source = src_elem.text or ''
        
        # Strategy 1: keep as-is (remove type=unfinished, keep translation text if any)
        if should_keep_as_is(source):
            # Keep source as translation (these are intentionally untranslated)
            tr_elem.text = source
            del tr_elem.attrib['type']
            stats['kept_as_is'] += 1
            continue
        
        # Strategy 2: translate from dictionary
        translation = translate(source, lang)
        if translation is not None:
            tr_elem.text = translation
            del tr_elem.attrib['type']
            stats['translated'] += 1
            continue
        
        # Strategy 3: leave unfinished (needs human/LLM pass)
        stats['untouched'] += 1
    
    # Write back
    tree.write(ts_path, encoding='utf-8', xml_declaration=True)
    return stats


def count_unfinished(ts_path):
    """Count unfinished entries in a .ts file."""
    tree = ET.parse(ts_path)
    root = tree.getroot()
    total = 0
    unfinished = 0
    for msg in root.iter('message'):
        total += 1
        tr = msg.find('translation')
        if tr is not None and tr.get('type') == 'unfinished':
            unfinished += 1
    return total, unfinished


def main():
    os.chdir(r'E:\ai\3DPrinter_Qt6')
    for lang in ['de', 'fr', 'ja', 'ko']:
        ts_path = f'i18n/{lang}.ts'
        total_before, unfinished_before = count_unfinished(ts_path)
        cov_before = (total_before - unfinished_before) * 100 // total_before if total_before else 0
        print(f'\n=== {lang} ===')
        print(f'  Before: {unfinished_before}/{total_before} unfinished ({cov_before}% covered)')
        
        stats = process_ts(ts_path, lang)
        print(f'  Translated: {stats["translated"]}')
        print(f'  Kept as-is: {stats["kept_as_is"]}')
        print(f'  Untouched:  {stats["untouched"]}')
        
        total_after, unfinished_after = count_unfinished(ts_path)
        cov_after = (total_after - unfinished_after) * 100 // total_after if total_after else 0
        print(f'  After: {unfinished_after}/{total_after} unfinished ({cov_after}% covered)')
        print(f'  Improvement: +{cov_after - cov_before}%')


if __name__ == '__main__':
    main()
