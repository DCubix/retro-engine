import os, sys
from PIL import Image
from io import BytesIO

def image_to_pack(image: Image.Image) -> str:
    # Verify that the image is not a NPOT (non-power-of-two) image
    if image.size[0] & (image.size[0] - 1) != 0 or image.size[1] & (image.size[1] - 1) != 0:
        raise ValueError("Image dimensions must be a power of two.")

    # 1. convert to black and white
    image = image.convert("1")  # 1-bit pixels, black and white

    # 2. save to PNG in memory
    img_buffer = BytesIO()
    image.save(img_buffer, format="PNG")
    png_data = img_buffer.getvalue()
    img_buffer.close()

    # 3. convert to string of chars
    packed_data = ""
    for byte in png_data:
        # convert to \xHH format
        packed_data += "\\x" + format(byte, '02x')
    
    return packed_data
    

def generate_header_file(name: str, packed_data: str) -> str:
    # 5. generate header file
    header = f"// {name} image data\n"
    header += "#pragma once\n"
    header += f"const unsigned char {name}_data_png[] = \n\t"

    # split by a fixed string width
    str_width = 64
    packed_data = [packed_data[i:i + str_width] for i in range(0, len(packed_data), str_width)]

    for row in packed_data:
        header += f'"{row}"'
        if row != packed_data[-1]:
            header += ",\n\t"

    header += ";\n"
    return header

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python image_pack.py <image_path>")
        sys.exit(1)

    image_path = sys.argv[1]
    if not os.path.isfile(image_path):
        print(f"Error: {image_path} is not a valid file.")
        sys.exit(1)

    # Load the image
    image = Image.open(image_path)
    name, ext = os.path.splitext(os.path.basename(image_path))
    
    # Convert the image to a bit-packed format
    packed_data = image_to_pack(image)
    
    # Generate the header file
    header_file_content = generate_header_file(name, packed_data)
    
    # Write the header file
    with open(f"{name}.h", "w", encoding='utf-8') as f:
        f.write(header_file_content)
    
    print(f"Header file {name}.h generated successfully.")