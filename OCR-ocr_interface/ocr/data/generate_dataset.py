import os
import numpy as np
from PIL import Image

# --- Paramètres de configuration ---
# Source: Le dossier 'cells_predicted' se trouve dans le même répertoire que ce script (data/)
IMAGE_DIR = 'cells_predicted' 
# Destination: Le fichier de sortie doit se trouver dans le même répertoire (data/)
OUTPUT_FILE = 'alphabet_train_augmented' 
# Taille attendue de l'image (32x32 pixels = 1024 floats)
IMAGE_SIZE = (32, 32)

def process_image_to_float_vector(file_path):
    # ... (Le reste de cette fonction reste le même, elle est correcte)
    try:
        img = Image.open(file_path).convert('L')
        img = img.resize(IMAGE_SIZE)
        pixels = np.array(img, dtype=np.float32) / 255.0
        return pixels.flatten()
    except Exception as e:
        print(f"Erreur lors du traitement de {file_path}: {e}")
        return None

def generate_augmented_dataset():
    data_lines = []
    
    # ⚠️ Utilise os.path.join pour construire un chemin correct vers les images
    full_image_dir = os.path.abspath(IMAGE_DIR)
    print(f"Scanning directory: {full_image_dir}")
    
    for filename in os.listdir(IMAGE_DIR):
        if not filename.endswith('.png') or ':Zone.Identifier' in filename:
            continue
            
        label = filename[0]
        if not ('A' <= label <= 'Z'):
            print(f"⚠️ Ignoré: Fichier {filename} n'a pas une étiquette de lettre majuscule valide.")
            continue
            
        file_path = os.path.join(IMAGE_DIR, filename)
        
        pixel_vector = process_image_to_float_vector(file_path)
        
        if pixel_vector is not None:
            pixel_string = ' '.join([f"{p:.6f}" for p in pixel_vector])
            data_lines.append(f"{label} {pixel_string}\n")

    output_path = os.path.abspath(OUTPUT_FILE)
    
    with open(OUTPUT_FILE, 'w') as f:
        f.writelines(data_lines)
    
    print(f"✅ Conversion terminée. {len(data_lines)} échantillons écrits dans {output_path}")

if __name__ == '__main__':
    generate_augmented_dataset()
