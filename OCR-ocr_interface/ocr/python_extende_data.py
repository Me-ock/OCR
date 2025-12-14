import numpy as np
import random

INPUT_FILE = "data/alphabet_train"
OUTPUT_FILE = "data/alphabet_train_augmented"
NUM_AUGMENTATIONS = 5  # combien de versions augmentées par image

def read_dataset(filename):
    letters = []
    images = []
    with open(filename, "r") as f:
        lines = f.readlines()
        for line in lines:
            line = line.strip().split()
            if len(line) < 2:
                continue
            letter = line[0]
            pixels = [float(p) for p in line[1:65]]
            if len(pixels) != 64:
                print(f"⚠️  Pixel count incorrect pour {letter}: {len(pixels)}")
                continue
            letters.append(letter)
            images.append(pixels)
    return letters, images

def augment_image(image):
    """Simple augmentation : flip horizontal/vertical ou un pixel aléatoire"""
    img = np.array(image).reshape((8,8))
    
    # Choisir une augmentation aléatoire
    choice = random.randint(0,3)
    if choice == 0:
        img = np.fliplr(img)  # flip horizontal
    elif choice == 1:
        img = np.flipud(img)  # flip vertical
    elif choice == 2:
        # Inverser un pixel aléatoire
        x, y = random.randint(0,7), random.randint(0,7)
        img[x,y] = 1 - img[x,y]
    # else: no change

    return img.flatten().tolist()

def main():
    letters, images = read_dataset(INPUT_FILE)
    augmented_lines = []

    for letter, img in zip(letters, images):
        # Ajouter l'image originale
        line = [letter] + [str(int(p)) for p in img]
        augmented_lines.append(" ".join(line))

        # Ajouter des augmentations
        for _ in range(NUM_AUGMENTATIONS):
            aug = augment_image(img)
            line_aug = [letter] + [str(int(p)) for p in aug]
            augmented_lines.append(" ".join(line_aug))

    with open(OUTPUT_FILE, "w") as f:
        for line in augmented_lines:
            f.write(line + "\n")

    print(f"✅ Dataset augmenté sauvegardé dans {OUTPUT_FILE}")

if __name__ == "__main__":
    main()

