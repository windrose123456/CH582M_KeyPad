from PIL import Image, ImageDraw, ImageFont
import os

# 设置字符集（空格 ~ 波浪号）
chars = [chr(i) for i in range(0x20, 0x7F)]

def generate_font(width, height, font_path=None, font_size=None):
    """
    生成指定宽高的 ASCII 字模（纵向取模，高位在上）
    width  : 字符宽度（像素）
    height : 字符高度（像素）
    font_path : ttf 字体路径，None 表示使用默认
    font_size : 如果指定，则用该字号渲染，否则自动缩放以适应 width/height
    """
    # 创建临时图像，渲染每个字符
    if font_path is None:
        # 使用系统默认字体，但可能不支持所有字符，建议指定一个 monospace 字体
        try:
            font = ImageFont.load_default()
        except:
            font = ImageFont.truetype("arial.ttf", size=height)  # 需要系统有 arial
    else:
        font = ImageFont.truetype(font_path, size=font_size if font_size else height)

    data = []
    for ch in chars:
        # 创建足够大的图像，确保字符完整
        img = Image.new('1', (width*2, height*2), color=1)  # 1=白色背景
        draw = ImageDraw.Draw(img)
        # 计算文本尺寸，居中绘制
        bbox = draw.textbbox((0, 0), ch, font=font)
        tw = bbox[2] - bbox[0]
        th = bbox[3] - bbox[1]
        x = (width - tw) // 2 - bbox[0]
        y = (height - th) // 2 - bbox[1]
        draw.text((x, y), ch, font=font, fill=0)  # 0=黑色前景

        # 裁剪到指定尺寸
        cropped = img.crop((0, 0, width, height))
        # 转换为字节数组：每列一个字节，从上到下，bit0=第一行
        byte_array = []
        for col in range(width):
            byte = 0
            for row in range(height):
                if cropped.getpixel((col, row)) == 0:  # 黑色像素为1
                    byte |= (1 << row)
            byte_array.append(byte)
        data.append(byte_array)

    return data

def print_array(data, array_name, width, height):
    """打印 C 语言数组格式"""
    print(f"// {width}x{height} 字体 (纵向取模，高位在上)")
    print(f"const uint8_t {array_name}[{len(data)}][{width}] = {{")
    for i, char_bytes in enumerate(data):
        hex_str = ', '.join(f"0x{byte:02X}" for byte in char_bytes)
        print(f"    {{ {hex_str} }},  // 0x{0x20+i:02X} '{chr(0x20+i)}'")
    print("};")

if __name__ == "__main__":
    # 生成 6x8 字体 (宽6，高8)
    print("========== FONT_12x6 ==========")
    data_6x8 = generate_font(6, 8, font_path="arial.ttf")  # 换成你系统的等宽字体
    print_array(data_6x8, "FONT_8x6", 6, 8)

    # 生成 8x8 字体 (宽8，高8)
    print("\n========== FONT_8x8 ==========")
    data_8x8 = generate_font(8, 8, font_path="arial.ttf")
    print_array(data_8x8, "FONT_8x8", 8, 8)