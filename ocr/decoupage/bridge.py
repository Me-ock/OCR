import os
import numpy as np
from PIL import Image
import re

# Chemins des dossiers et du fichier de sortie
LETTERS_DIR = 'decoupage/letters'
WORDS_DIR = 'decoupage/words_letters'
OUTPUT_FILE = 'decoupage/decoupage_output.txt'
IMAGE_SIZE = (32, 32)

def process_image(path):
    """
    Lit une image, la redimensionne à 32x32, la convertit en niveaux de gris
    et normalise les pixels en flottants entre 0.0 et 1.0.
    """
    try:
        img = Image.open(path).convert('L')
        img = img.resize(IMAGE_SIZE)
        # Convertit l'image en un tableau numpy de flottants et normalise les valeurs
        pixels = np.array(img, dtype=np.float32) / 255.0
        return pixels.flatten()
    except Exception as e:
        print(f"Erreur lors du traitement de {path}: {e}")
        return None

def run_bridge():
    """
    Analyse les dossiers 'letters' et 'words_letters', traite les images
    et génère le fichier 'decoupage_output.txt'.
    """
    output_lines = []
    
    # Traitement du dossier 'letters' pour la grille
    print(f"Analyse de {LETTERS_DIR}...")
    for filename in sorted(os.listdir(LETTERS_DIR)):
        if filename.endswith('.png') and ':Zone.Identifier' not in filename:
            match = re.match(r'letter_(\d+)_(\d+)\.png', filename)
            if match:
                row, col = map(int, match.groups())
                
                image_path = os.path.join(LETTERS_DIR, filename)
                pixels = process_image(image_path)
                
                if pixels is not None:
                    pixel_str = ' '.join([f"{p:.6f}" for p in pixels])
                    output_lines.append(f"GRID {col} {row} {pixel_str}\n")

    # Traitement du dossier 'words_letters' pour les mots
    print(f"Analyse de {WORDS_DIR}...")
    for filename in sorted(os.listdir(WORDS_DIR)):
        if filename.endswith('.png') and ':Zone.Identifier' not in filename:
            match = re.match(r'word_(\d+)_let_(\d+)\.png', filename)
            if match:
                word_idx, letter_idx = map(int, match.groups())
                
                image_path = os.path.join(WORDS_DIR, filename)
                pixels = process_image(image_path)
                
                if pixels is not None:
                    pixel_str = ' '.join([f"{p:.6f}" for p in pixels])
                    output_lines.append(f"WORD {word_idx} {letter_idx} {pixel_str}\n")

    # Écriture du fichier de sortie
    with open(OUTPUT_FILE, 'w') as f:
        f.writelines(output_lines)
        
    print(f"✅ Fichier '{OUTPUT_FILE}' généré avec {len(output_lines)} entrées.")

if __name__ == "__main__":
    run_bridge()