import numpy as np

INPUT_FILE = "data/alphabet_train"
OUTPUT_FILE = "data/alphabet_train_augmented"
AUG_PER_LETTER = 5  # nombre de variantes par lettre

def read_dataset(filename):
    letters = []
    images = []
    with open(filename, "r") as f:
        lines = f.readlines()
        idx = 0
        while idx < len(lines):
            letter = lines[idx].strip()
            idx += 1
            image = []
            for _ in range(64):
                image.append(float(lines[idx].strip()))
                idx += 1
            letters.append(letter)
            images.append(np.array(image).reshape((8,8)))
    return letters, images

def augment_image(img):
    augmented = []

    for _ in range(AUG_PER_LETTER):
        new_img = img.copy()
        
        # --- Translation ---
        shift_x = np.random.randint(-1,2)  # -1,0,1
        shift_y = np.random.randint(-1,2)
        new_img = np.roll(new_img, shift_x, axis=1)
        new_img = np.roll(new_img, shift_y, axis=0)
        
        # --- Noise ---
        noise = np.random.binomial(1, 0.05, size=new_img.shape)  # 5% pixels flip
        new_img = np.clip(new_img + noise, 0, 1)
        
        augmented.append(new_img)
    return augmented

def save_dataset(filename, letters, images):
    with open(filename, "w") as f:
        for letter, img in zip(letters, images):
            f.write(letter + "\n")
            for val in img.flatten():
                f.write(f"{val}\n")

def main():
    letters, images = read_dataset(INPUT_FILE)
    all_letters = []
    all_images = []

    for letter, img in zip(letters, images):
        # ajoute l'image originale
        all_letters.append(letter)
        all_images.append(img)
        
        # ajoute les variantes augmentées
        aug_imgs = augment_image(img)
        all_letters.extend([letter]*AUG_PER_LETTER)
        all_images.extend(aug_imgs)

    save_dataset(OUTPUT_FILE, all_letters, all_images)
    print(f"✅ Dataset augmenté enregistré dans {OUTPUT_FILE}")

if __name__ == "__main__":
    main()
