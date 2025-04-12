#!/bin/env python3
help="""
Usage:
obz2cobz.py <src>.obz <dst>.cobz

Descr:
Compiles Open Board Zip files (exported from Board Builder),
into a Compiled Open Board Zip for this custom AAC program I wrote (AACpp).

This is intended to be nothing more than a prototype. A full C++ (probably)
version will be written in the future. It's just that zip+json is such a hassle
to parse in C.
"""

import json
from sys import argv, stderr, exit
import zipfile as zipf
from struct import pack
from typing import Tuple, Self
import requests as rq
from PIL import Image, UnidentifiedImageError
import os
import io
from tqdm import tqdm
from time import sleep
import traceback
from configparser import ConfigParser
from subprocess import Popen, PIPE
from threading import Thread
import time
import pickle
import math
from copy import copy
from pprint import pprint
from collections import namedtuple
from dataclasses import dataclass

def svg2png(data: bytes) -> bytes:
    f = Popen(['cairosvg', '-u', '-W', '128', '-H', '128', '-f', 'png', '-'], stdin=PIPE, stdout=PIPE)
    f.stdin.write(data)
    f.stdin.flush()
    f.stdin.close()
    raw = f.stdout.read()
    f.stdout.close()
    return bytes(raw)

DEFAULTS = ConfigParser()
DEFAULTS.read('config/defaults.ini')
SUPPORTED_IMAGE_FORMATS = ('png','jpg','jpeg','tga','bmp','psd','gif','hdr','pic','pnm','svg')
LEGACY_COLORS = {
"AliceBlue":"#F0F8FF",
"AntiqueWhite":"#FAEBD7",
"Aqua":"#00FFFF",
"Aquamarine":"#7FFFD4",
"Azure":"#F0FFFF",
"Beige":"#F5F5DC",
"Bisque":"#FFE4C4",
"Black":"#000000",
"BlanchedAlmond":"#FFEBCD",
"Blue":"#0000FF",
"BlueViolet":"#8A2BE2",
"Brown":"#A52A2A",
"BurlyWood":"#DEB887",
"CadetBlue":"#5F9EA0",
"Chartreuse":"#7FFF00",
"Chocolate":"#D2691E",
"Coral":"#FF7F50",
"CornflowerBlue":"#6495ED",
"Cornsilk":"#FFF8DC",
"Crimson":"#DC143C",
"Cyan":"#00FFFF",
"DarkBlue":"#00008B",
"DarkCyan":"#008B8B",
"DarkGoldenRod":"#B8860B",
"DarkGray":"#A9A9A9",
"DarkGrey":"#A9A9A9",
"DarkGreen":"#006400",
"DarkKhaki":"#BDB76B",
"DarkMagenta":"#8B008B",
"DarkOliveGreen":"#556B2F",
"DarkOrange":"#FF8C00",
"DarkOrchid":"#9932CC",
"DarkRed":"#8B0000",
"DarkSalmon":"#E9967A",
"DarkSeaGreen":"#8FBC8F",
"DarkSlateBlue":"#483D8B",
"DarkSlateGray":"#2F4F4F",
"DarkSlateGrey":"#2F4F4F",
"DarkTurquoise":"#00CED1",
"DarkViolet":"#9400D3",
"DeepPink":"#FF1493",
"DeepSkyBlue":"#00BFFF",
"DimGray":"#696969",
"DimGrey":"#696969",
"DodgerBlue":"#1E90FF",
"FireBrick":"#B22222",
"FloralWhite":"#FFFAF0",
"ForestGreen":"#228B22",
"Fuchsia":"#FF00FF",
"Gainsboro":"#DCDCDC",
"GhostWhite":"#F8F8FF",
"Gold":"#FFD700",
"GoldenRod":"#DAA520",
"Gray":"#808080",
"Grey":"#808080",
"Green":"#008000",
"GreenYellow":"#ADFF2F",
"HoneyDew":"#F0FFF0",
"HotPink":"#FF69B4",
"IndianRed":"#CD5C5C",
"Indigo":"#4B0082",
"Ivory":"#FFFFF0",
"Khaki":"#F0E68C",
"Lavender":"#E6E6FA",
"LavenderBlush":"#FFF0F5",
"LawnGreen":"#7CFC00",
"LemonChiffon":"#FFFACD",
"LightBlue":"#ADD8E6",
"LightCoral":"#F08080",
"LightCyan":"#E0FFFF",
"LightGoldenRodYellow":"#FAFAD2",
"LightGray":"#D3D3D3",
"LightGrey":"#D3D3D3",
"LightGreen":"#90EE90",
"LightPink":"#FFB6C1",
"LightSalmon":"#FFA07A",
"LightSeaGreen":"#20B2AA",
"LightSkyBlue":"#87CEFA",
"LightSlateGray":"#778899",
"LightSlateGrey":"#778899",
"LightSteelBlue":"#B0C4DE",
"LightYellow":"#FFFFE0",
"Lime":"#00FF00",
"LimeGreen":"#32CD32",
"Linen":"#FAF0E6",
"Magenta":"#FF00FF",
"Maroon":"#800000",
"MediumAquaMarine":"#66CDAA",
"MediumBlue":"#0000CD",
"MediumOrchid":"#BA55D3",
"MediumPurple":"#9370DB",
"MediumSeaGreen":"#3CB371",
"MediumSlateBlue":"#7B68EE",
"MediumSpringGreen":"#00FA9A",
"MediumTurquoise":"#48D1CC",
"MediumVioletRed":"#C71585",
"MidnightBlue":"#191970",
"MintCream":"#F5FFFA",
"MistyRose":"#FFE4E1",
"Moccasin":"#FFE4B5",
"NavajoWhite":"#FFDEAD",
"Navy":"#000080",
"OldLace":"#FDF5E6",
"Olive":"#808000",
"OliveDrab":"#6B8E23",
"Orange":"#FFA500",
"OrangeRed":"#FF4500",
"Orchid":"#DA70D6",
"PaleGoldenRod":"#EEE8AA",
"PaleGreen":"#98FB98",
"PaleTurquoise":"#AFEEEE",
"PaleVioletRed":"#DB7093",
"PapayaWhip":"#FFEFD5",
"PeachPuff":"#FFDAB9",
"Peru":"#CD853F",
"Pink":"#FFC0CB",
"Plum":"#DDA0DD",
"PowderBlue":"#B0E0E6",
"Purple":"#800080",
"RebeccaPurple":"#663399",
"Red":"#FF0000",
"RosyBrown":"#BC8F8F",
"RoyalBlue":"#4169E1",
"SaddleBrown":"#8B4513",
"Salmon":"#FA8072",
"SandyBrown":"#F4A460",
"SeaGreen":"#2E8B57",
"SeaShell":"#FFF5EE",
"Sienna":"#A0522D",
"Silver":"#C0C0C0",
"SkyBlue":"#87CEEB",
"SlateBlue":"#6A5ACD",
"SlateGray":"#708090",
"SlateGrey":"#708090",
"Snow":"#FFFAFA",
"SpringGreen":"#00FF7F",
"SteelBlue":"#4682B4",
"Tan":"#D2B48C",
"Teal":"#008080",
"Thistle":"#D8BFD8",
"Tomato":"#FF6347",
"Turquoise":"#40E0D0",
"Violet":"#EE82EE",
"Wheat":"#F5DEB3",
"White":"#FFFFFF",
"WhiteSmoke":"#F5F5F5",
"Yellow":"#FFFF00",
"YellowGreen":"#9ACD32"
}
LEGACY_COLORS = { k:pack('>4B', *(int(v[1:], 16) for j in range(3))) for k,v in LEGACY_COLORS.items() }
print(LEGACY_COLORS["AliceBlue"])
ERROR_COLOR = bytes([0, 0, 0, 0])
_SAVE_CRASH_FILE = open(f"savecrash{int(time.time())}.dat", 'wb')

def expect(cond: bool, err_msg: str):
    if not cond:
        print('ERR:', err_msg)
        quit(1)

def perror(msg: str):
    stderr.write(str(msg) + '\n')

def want(cond: bool, warn_msg: str):
    if not cond:
        print('WARN:', warn_msg)
_want1_cache = 0
def want1(cond: bool, warn_msg: str, wid: int):
    """
        Warns user once. Preferable, use small numbers for wid.
    """
    global _want1_cache
    if _want1_cache & (1 << wid):
        return
    if not cond:
        print('WARN:', warn_msg)
        _want1_cache |= (1 << wid)

def info(msg: str):
    print("INFO:", msg)

def todo():
    raise NotImplementedError()


class Cell:
    name: str
    tex_id: int
    child: int
    background: bytes
    border: bytes
    actions: list[bytes]
    obz_child_id: str | None
    obz_tex_id: str | None
    obz_id: str
    obz_xy: Tuple[int, int]
    def set_child(self, cobz, child_board_id):
        if isinstance(child_board_id, int):
            self.child = child_board_id
            cobz.boards[self.child].parent = child_board_id
        else:
            assert isinstance(child_board_id, str)
            self.obz_child_id = child_board_id
            if not isinstance(cobz.boards[self.child].parent, int):
                cobz.boards[self.child].parent = child_board_id
    def init(self, xy: None | Tuple[int, int] = None) -> Self:
        self.name = ""
        self.tex_id = -1
        self.parent = -1
        self.child = -1
        self.background = ERROR_COLOR
        self.border = ERROR_COLOR
        self.actions = []
        self.obz_child_id = None
        self.obz_tex_id = None
        self.obz_id = None
        self.obz_xy = xy
        return self
    def serialize(self) -> bytes:
        self.background = self.background or ERROR_COLOR
        self.border = self.border or ERROR_COLOR
        byts = self.name.encode('utf-8')
        actions = b''
        for a in self.actions:
            actions += pack(f'=q{len(a)}s', len(a), a)
        return pack(
            f'=3siq{len(byts)}sq{len(actions)}si4B4B',
            b'CLL',
            self.tex_id,
            len(byts),
            byts,
            len(self.actions),
            actions,
            self.child,
            *self.background,
            *self.border
        )

class Board:
    w: int
    h: int
    parent: int | str
    cells: list[Cell]
    name: str
    obz_id: str
    def solve_parent(self, cobz):
        pass
    def serialize(self) -> bytes:
        res = pack('=iiii', self.w, self.h, self.parent, len(self.cells))
        for i in self.cells:
            res += i.serialize()
        return res

class Rect:
  def __init__(self, x=0, y=0, w=0, h=0):
    self.x = x
    self.y = y
    self.w = w
    self.h = h
    self.spritesheet_id = None
    self.locked = False
  def serialize(self) -> bytes:
    return pack('iffff', self.spritesheet_id, self.x, self.y, self.w, self.h)
  def __getitem__(self, idx):
    return (self.x, self.y, self.w, self.h)[idx]
  def __setitem__(self, idx, value):
    (self.x, self.y, self.w, self.h)[idx] = value

@dataclass
class Obj:
  board_id: str
  img: Image
  rect: Rect
@dataclass
class Fit:
    rect: Rect
    obj_idx: int

def could_contain(rbig: Rect, rsmol: Rect):
    return rbig.w >= rsmol.w and rbig.h >= rsmol.h

class CompiledOBZ:
    def __init__(self):
        self.textures: dict[str, Obj] | Tuple[Obj] = {}
        self.boards: list[Board] | Tuple[Board, ...] = []
    def gen_spritesheet_precursors(self, tex_set):
        objs: list[Obj] = tex_set
        MAXW = 0
        for o in objs:
          o.rect.w = o.img.width
          o.rect.h = o.img.height
          while o.rect.w > 300:
            o.rect.w = o.rect.w//2
            o.rect.h = o.rect.h//2
          if o.rect.w > MAXW:
            MAXW = o.rect.w
          o.img = o.img.resize((o.rect.w, o.rect.h))
        MAXW *= 2
        objs.sort(key=lambda x: (x.rect.w, x.rect.h), reverse=True) # same as sorting for width. texs are squares
        minw = objs[-1].rect.w
        # Set y positions accordingly
        y = 0
        for o in objs:
          o.rect.x = 0
          o.rect.y = y
          y += o.rect.h
        # Now run down mode simple optimization (+y goes downward)
        # Probably the worst algorithm i've ever written
        # avg of o(n^3/2) (maybe even more. I haven't made the math)
        fits = [Fit(Rect(o.rect.x+o.rect.w, o.rect.y, MAXW - o.rect.w, o.rect.h), i) for i,o in enumerate(objs)]
        max_dim = [0,0]
        base_fixed = 0
        for i in range(1, len(objs)):
          for j in range(base_fixed, i):
            if could_contain(fits[j].rect, objs[i].rect):
              objs[i].rect.x = fits[j].rect.x
              objs[i].rect.y = fits[j].rect.y
              fits.insert(
                j, Fit(
                  Rect(
                       objs[i].rect.x + objs[i].rect.w,
                       objs[i].rect.y,
                       MAXW - (objs[i].rect.x + objs[i].rect.w),
                       objs[i].rect.h
                  ),
                  i
              ))
              fits[j+1].rect.y += objs[i].rect.h
              fits[j+1].rect.h -= objs[i].rect.h
              for fi in range(j+2, len(fits)):
                if fits[fi].obj_idx == i:
                  del fits[fi]
                  break
              for fi in range(fi, len(fits)):
                fits[fi].rect.y -= objs[i].rect.h
              for j in range(i+1, len(objs)):
                objs[j].rect.y -= objs[i].rect.h
              break
            if MAXW - (objs[i].rect.x + objs[i].rect.w) <= minw:
              base_fixed = i+1
        max_dim[0] = MAXW
        max_dim[1] = max(o.rect.y+o.rect.h for o in objs)
        return objs, max_dim
    def gen_one_spritesheet(self, objs: list[Obj], dim, spritesheet_id: int):
      spritesheet = Image.new('RGBA', dim, (0,0,0,0))
      for obj in objs:
        assert obj.rect.spritesheet_id is None, "Image reuse is not permitted."
        obj.rect.spritesheet_id = spritesheet_id
        spritesheet.paste(
          obj.img,
          (
            obj.rect.x,
            obj.rect.y
          )
        )
      return spritesheet
    def gen_all_spritesheets(self) -> list[Image]:
      assert isinstance(self.textures, tuple)
      spritesheets = []
      for board in tqdm(self.boards, desc="Generating spritesheets..."):
        texs = list(filter(lambda x: x[1].board_id == board.obz_id, self.textures))
        objs, dim = self.gen_spritesheet_precursors([o[1] for o in texs])
        spritesheets.append(self.gen_one_spritesheet(objs, dim, len(spritesheets)))
      return spritesheets
    def find_board_with_name(self, name: str) -> int | None:
        for idx, board in enumerate(self.boards):
            if board.name == name:
                return idx
        return None

def depercent(expr: str, max: float):
    if expr.endswith('%'):
        return float(expr[:-1])/100.0 * max
    else:
        return float(expr)
def parse_color(expr: str | None, default) -> bytes:
    if expr is None:
        return default
    if expr.startswith('rgb(') and expr.endswith(')'):
        r,g,b = (int(depercent(i.strip(), 255)) for i in expr[4:-1].split(','))
        a = 255
    elif expr.startswith('rgba(') and expr.endswith(')'):
        r,g,b,a = (int(depercent(i.strip(), 255)) for i in expr[5:-1].split(','))
    elif expr.startswith('#'):
        if len(expr) == 7:
            r,g,b = (int(c0+c1, 16) for c0,c1 in zip(expr[1::2],expr[2::2]))
            a = 255
        elif len(expr) == 9:
            r,g,b,a = (int(c0+c1, 16) for c0,c1 in zip(expr[1::2],expr[2::2]))
        else:
            want(False, f"Expected 6 or 8 digit hex color format, but got {repr(expr)}.")
            return default
    elif expr in LEGACY_COLORS:
        return LEGACY_COLORS[expr]
    else:
        want(False, f"Unknown color format: {repr(expr)}")
        return default
    return bytes([r,g,b,a])


type Img = Image
def load_img(z: zipf.ZipFile, src: str | None, cell_name: str) -> Img | None:
    if not src: # src is None or len(src) == 0
        return None
    raw: bytes
    if src.startswith('http'):
        try:
            extension = src[src.rfind('.')+1:]
            resp = rq.get(src)
            sleep(0.2) # avoid getting flagged as some kind of ddos stuff by any website
            raw = resp.content
            if extension.lower() == 'svg':
                raw = svg2png(raw)
        except rq.RequestException as e:
            perror(f"Failed to load image with error {type(e).__name__}:{e} from web address: {repr(src)}")
            return None
    elif src.startswith('data:image/'):
        extension = src[11:11+3]
        cur = 11+3
        if extension == 'jpe':
            extension += src[11+3]
            cur += 1
        expect(extension.lower() in SUPPORTED_IMAGE_FORMATS, f">Unknown file extension for inline image: {repr(extension)}. See README.md for supported formats. Aborting.")
        cur2 = src.find(',', cur)
        encoding = src[cur:cur2]
        expect(encoding.lower() == 'base64', f">Unsupported encoding {encoding} for inline image (only base64 is supported).")
        raw = base64.b64decode(src[cur2:])
        if extension.lower() == 'svg':
            raw = svg2png(raw)
    elif (extension := src[src.rfind('.')+1:].lower()) in SUPPORTED_IMAGE_FORMATS:
        with z.open(src) as _f_image:
            raw =_f_image.read()
        if extension.lower() == 'svg':
            raw = svg2png(raw)
    else:
        print(extension)
        expect(False, f">Unknown image source information format ({repr(src[:50])}{'...' if len(src) >= 50 else ''}).")
    # Last checks just to make sure everything is good:
    try:
        img = Image.open(io.BytesIO(raw))
        try:
            img.verify()
        except Exception as e:
            expect(False, f'>Given image is broken: {e}')
    except UnidentifiedImageError:
        perror(f"Failed to identify image for cell {repr(cell_name)}, {src=}.")
    # img.verify closed the io buffer
    img = Image.open(io.BytesIO(raw))
    return img

def find_position(id: str, dbl: list[list[str]]):
    for y, l in enumerate(dbl):
        for x, check_id in enumerate(l):
            if str(check_id) == str(id):
                return (x,y)
    # expect(False, f"Could not find cell with {id=} in grid.")
    return None

def index_when(it, func):
    for idx, elem in enumerate(it):
        if func(elem):
            return idx
    return -1

def parse_board(
    z: zipf.ZipFile,
    cobz: CompiledOBZ,
    file_handler,
    obz_id: str,
    manifest
) -> Board:
    board = Board()
    obf = json.load(file_handler)
    want(obz_id == obf['id'], f"ID mismatch (manifest specifies {obz_id} while descriptor specifies {obf['id']})")
    board.obz_id = str(obz_id) # different sources use different types, safer to use strings
    board.name = obf['name']
    board.parent = -1
    celld: dict[str, Cell] = {}

    info(f"Loading board named {obf['name']}.")
    board.w = obf['grid']['columns']
    board.h = obf['grid']['rows']
    board.cells = []
    for b in obf['buttons']:
        # want1(b['border_color'] is None, "*Border color for cell/button isn't supported.", 0)
        # want1(b['background_color'] is None, "*Background color for cell/button isn't supported.", 1)
        c = Cell()
        c.obz_id = str(b['id'])
        c.obz_tex_id = None
        c.obz_child_id = None
        c.background = ERROR_COLOR
        c.border = ERROR_COLOR
        c.name = b.get('label') or "" # Not always specified
        c.actions = []
        if 'image_id' in b:
          if b['image_id'] in cobz.textures:
            c.obz_tex_id = b['image_id']
            cobz.textures[c.obz_tex_id].board_id = obz_id
          else:
            img_src = None
            for i in obf['images']:
              if i['id'] == b['id']:
                if 'url' in i:
                  img_src = i['url']
                elif 'data' in i:
                  img_src = i['data']
                else:
                  want(False, '>No known image format found.')
                # TODO: add other keys that give an image source/data
                break
            else:
              c.tex_id = -1
              c.obz_tex_id = None
            if img := load_img(z, img_src, c.name):
              cobz.textures[b['image_id']] = Obj(c.obz_id, img, Rect())
              c.obz_tex_id = b['image_id']
            else:
              c.tex_id = -1
              c.obz_tex_id = None
        if 'load_board' in b:
            for id, path in manifest['paths']['boards'].items():
                if path == b['load_board']['path']:
                    break
            else:
                expect(False, f"Could not find known board id with path {repr(b['load_board']['path'])}.")
            c.obz_child_id = id
            # TODO: Add other ways to link boards
        if 'actions' in b:
            c.actions = [a.encode('utf-8') for a in b['actions']]
        elif 'action' in b:
            c.actions = [b['action'].encode('utf-8')]
        elif c.name:
            c.actions = [('+ ' + c.name).encode('utf-8')]
        if 'background_color' in b:
            c.background = parse_color(b['background_color'], parse_color(DEFAULTS['cell']['background'], ERROR_COLOR))
        if 'border_color' in b:
            c.border = parse_color(b['border_color'], parse_color(DEFAULTS['cell']['border'], ERROR_COLOR))
        if pos := find_position(c.obz_id, obf['grid']['order']):
            c.obz_xy = pos
            board.cells.append(c)
        else:
            print(f"Had to discard cell {repr(c.name)} because it doesn't have a grid position.")
    # Sorting cells based on position
    board.cells.sort(key=lambda c: c.obz_xy[::-1]) # '[::-1]' because y position is the most important when sorting, followed by x positions.
    if len(board.cells) == 0:
        board.cells = [Cell().init((x, y)) for y in range(board.h) for x in range(board.w)]
    else:
        for y in range(board.h):
            for x in range(board.w):
                if x+y*board.w >= len(board.cells) or board.cells[x+y*board.w].obz_xy != (x, y):
                    board.cells.insert(x+y*board.w, Cell().init((x,y)))
    return board

THREAD_COUNT = 1
MIN_BATCH_SIZE = 256
def _image_preloader_batch(z: zipf.ZipFile, batch: Tuple[Tuple[str, str], ...], inout: dict[str, bytes], thid: int):
    for img_id, path in tqdm(batch, f"Thread {thid}", position=thid, nrows=THREAD_COUNT+1):
        inout[str(img_id)] = Obj(-1, load_img(z, path, "<image preloading: no cell name>"), Rect())

def parse_file(filename: str, import_images_from: str | None = None) -> CompiledOBZ:
    global THREAD_COUNT
    cobz = CompiledOBZ()
    z = zipf.ZipFile(filename, 'r')
    if import_images_from:
        cobz.textures = {k: Obj(board_id, Image.open(io.BytesIO(raw)), rect) for k, (board_id, raw, rect) in pickle.load(open(import_images_from, "rb")).items()}
    expect("manifest.json" in (i.filename for i in z.filelist), f"Missing manifest.json in {filename} !")
    with z.open('manifest.json', 'r') as _f_manifest:
        manifest = json.load(_f_manifest)
    expect(manifest['format'] == "open-board-0.1", f"Unknown board set format: {repr(manifest['format'])}. Expected 'open-board-0.1' instead.")
    # want(len(manifest['paths']['images']) == 0, "Image specification in manifest.json is not supported.")
    want(len(manifest['paths']['sounds']) == 0, "Sound specification in manifest.json is not supported.")
    if not import_images_from:
        print("Preloading referenced images:")
        full = tuple(manifest['paths']['images'].items())
        if len(full) >= MIN_BATCH_SIZE*2:
            # If we can split at least two batches, then we do
            for unit_count in range(os.cpu_count(), 1, -1):
                if len(full) >= MIN_BATCH_SIZE * unit_count:
                    TH_COUNT = os.cpu_count()
                    SIZES = [len(full) // os.cpu_count() for _ in range(TH_COUNT)]
                    SIZES[0] += len(full) - (len(full)//os.cpu_count()*os.cpu_count())
                    break
            else:
                assert False, "This branch shouldn't be reached"
            out_batches = [{} for _ in range(TH_COUNT)]
            in_batches = [tuple() for i in range(TH_COUNT)]
            s = 0
            for i in range(TH_COUNT):
                in_batches[i] = full[s:s+SIZES[i]]
                s += SIZES[i]
            assert s == len(full), f"These should be equal: {s=} == {len(full)=}"
            THREAD_COUNT = TH_COUNT
            thrds: list[Thread] = [
                Thread(target=_image_preloader_batch, args=(z, in_batches[i], out_batches[i], i))
                for i in range(1,TH_COUNT)
            ]
            for i in thrds:
                i.start()
            _image_preloader_batch(z, in_batches[0], out_batches[0], 0)
            for i in thrds:
                i.join()
            for batch in out_batches:
                cobz.textures |= batch
            bufs = {}
            for k, tex in cobz.textures.items():
                temp = io.BytesIO()
                tex.img.save(temp, 'png')
                bufs[k] = (tex.board_id, temp.getvalue(), tex.rect)
            pickle.dump(bufs, _SAVE_CRASH_FILE)
        else:
            for img_id, path in tqdm():
                cobz.textures[str(img_id)] = load_img_raw(z, path, "<image preloading: no cell name>")
    for board_id, board_path in manifest['paths']['boards'].items():
        with z.open(board_path, 'r') as _f_board:
            cobz.boards.append(
                parse_board(z, cobz, _f_board, board_id, manifest)
            )
    z.close()
    # WE THEN NEED TO LINK BOARDS BY INDEX, NOT BY ID ANYMORE
    # 1. Select root board and put it first (swap)
    for root_id, path in manifest['paths']['boards'].items():
        if path == manifest['root']:
            break
    root_idx = index_when(cobz.boards, lambda x: x.obz_id == root_id)
    cobz.boards[0], cobz.boards[root_idx] = cobz.boards[root_idx], cobz.boards[0]
    # 2. Fix and resolve indicies (in all cells)
    cobz.boards = tuple(cobz.boards)
    cobz.textures = tuple(cobz.textures.items())
    for idx, board in enumerate(cobz.boards):
        for cell in board.cells:
            cell.tex_id = index_when(
                cobz.textures,
                lambda x: x[0] == cell.obz_tex_id
            )
            if cell.obz_child_id is None:
                # TODO: REMOVE: THIS IS JUST BECAUSE BOARD BUILDER DOESN'T
                # SUPPORT LINKING WITHIN THE EXPORTED FILES
                if child_idx := cobz.find_board_with_name(cell.name):
                    cell.set_child(cobz, child_idx)
                else:
                    cell.child = -1
            else:
                cell.child = index_when(
                    cobz.boards,
                    lambda x: x.obz_id == cell.obz_child_id
                )
            if cell.child != -1:
                for c in cobz.boards[cell.child].cells:
                    c.parent = idx
    return cobz

if __name__ == '__main__':
    if len(argv) < 3:
        print(help)
        exit(1)
    try:
        cobz = parse_file(argv[1], None if len(argv) == 3 else argv[3])
        spritesheets = cobz.gen_all_spritesheets()
        with open(argv[2], 'wb') as f:
            # NOTE: using 'q' because resman.cpp reads a i64
            f.write(pack('q', len(cobz.boards)))
            for board in cobz.boards:
                f.write(b'BRD\x00')
                f.write(board.serialize())
            info("Loading spritesheet precursors...")
            info("Loaded spritesheet precursors.")
            info("Generating spritesheet...")
            f.write(pack('q', len(cobz.textures)))
            for obj in cobz.textures:
              f.write(obj[1].rect.serialize())
            f.write(pack('q', len(spritesheets)))
            for idx,spritesheet in enumerate(spritesheets):
              f.write(b"IMG\x00")
              buf = io.BytesIO()
              spritesheet.save(buf, 'png')
              raw = buf.getvalue()
              f.write(raw)
              with open(f"test-{idx}.png", 'wb') as ftest:
                ftest.write(raw)
              print(f"size of spritesheet [id {idx}] data is {len(raw)}")
            info("Generated spritesheets !")
        print("Done !")
    except Exception:
        perror(traceback.format_exc())
        _SAVE_CRASH_FILE.close()
        exit(1)
    _SAVE_CRASH_FILE.close()
    exit(0)



