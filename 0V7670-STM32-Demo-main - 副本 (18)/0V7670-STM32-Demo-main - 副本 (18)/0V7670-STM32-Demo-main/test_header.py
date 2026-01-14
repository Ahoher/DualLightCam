#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# 测试协议头长度
for photo_type in [0, 1, 2, 3]:
    header = f"IMG_START,320,240,16,{photo_type},1\r\n"
    print(f"photo_type={photo_type}: '{header}' (长度={len(header)})")
    print(f"  HEX: {header.encode().hex(' ')}")
    print()
