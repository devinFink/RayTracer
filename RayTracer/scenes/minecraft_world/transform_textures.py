from pathlib import Path

from PIL import Image

def process_images():
    for file in Path("tex").glob("*"):
        with Image.open(file) as img:
            # Resize by 10x using nearest neighbor
            new_size = (img.width * 10, img.height * 10)
            upscaled = img.resize(new_size, Image.NEAREST)

            # Flip along Y-axis (vertical flip)
            flipped = upscaled.transpose(Image.FLIP_TOP_BOTTOM)

            # Save with new name
            flipped.save(file)
            print(f"Processed: {file}")

if __name__ == "__main__":
    process_images()
