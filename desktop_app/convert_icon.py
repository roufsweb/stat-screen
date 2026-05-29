import os
import sys
import subprocess

try:
    from PIL import Image
except ImportError:
    print("Pillow not found, installing Pillow...")
    subprocess.check_call([sys.executable, "-m", "pip", "install", "Pillow"])
    from PIL import Image

def main():
    png_source = r"C:\Users\rakib\.gemini\antigravity-ide\brain\b5368e21-a5a6-4dcc-b4f6-52e13d565aa6\app_icon_1780055229674.png"
    dest_dir = r"e:\rouf\hardware-project\stat-screen\desktop_app"
    
    png_dest = os.path.join(dest_dir, "icon.png")
    ico_dest = os.path.join(dest_dir, "icon.ico")
    
    if not os.path.exists(png_source):
        print(f"Source PNG not found: {png_source}")
        return
        
    print(f"Loading generated PNG: {png_source}")
    img = Image.open(png_source)
    
    # Save standard PNG to destination
    print(f"Saving destination PNG: {png_dest}")
    img.save(png_dest)
    
    # Save standard Windows ICO file with multiple standard dimensions
    print(f"Converting and saving ICO file: {ico_dest}")
    img.save(ico_dest, format='ICO', sizes=[(256, 256), (128, 128), (64, 64), (32, 32), (16, 16)])
    print("Icon conversion completed successfully!")

if __name__ == "__main__":
    main()
