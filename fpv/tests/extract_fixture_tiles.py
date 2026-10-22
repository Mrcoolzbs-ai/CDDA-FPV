"""Extract six actual tiles for the standalone renderer test. Requires Pillow.
Usage: python extract_fixture_tiles.py PATH_TO_CDDA_SOURCE OUTPUT_DIRECTORY
Does not modify game files. Texture assets stay under CDDA's original license.
"""
import json,struct,sys
from pathlib import Path
from PIL import Image
root=Path(sys.argv[1])/'gfx'/'UltimateCataclysm'; out=Path(sys.argv[2]); out.mkdir(parents=True,exist_ok=True)
d=json.loads((root/'tile_config.json').read_text()); sprites={}; tiles={}; offset=0
wanted={'t_wall','t_floor','mon_zombie','npc_male','f_table','bottle_plastic'}
for entry in d['tiles-new']:
 im=Image.open(root/entry['file']).convert('RGBA'); w=entry.get('sprite_width',32); h=entry.get('sprite_height',32)
 cols=im.width//w; count=cols*(im.height//h)
 for tile in entry.get('tiles',[]):
  ids=tile.get('id',[]); ids=[ids] if isinstance(ids,str) else ids
  for tid in ids:
   if tid in wanted:
    chosen=next((t for t in tile.get('additional_tiles',[]) if t.get('id')=='center'),tile)
    tiles[tid]=chosen
 # Map only requested sprites after collecting tile definitions below.
 sprites[offset]=(offset+count,im,w,h,cols); offset+=count

def index(value):
 if isinstance(value,int): return value
 if isinstance(value,list): return index(value[0]) if value else None
 if isinstance(value,dict): return index(value.get('sprite'))
 return None

def crop(i):
 if i is None:return None
 for start,(end,im,w,h,cols) in sprites.items():
  if start<=i<end:
   j=i-start; x=(j%cols)*w; y=(j//cols)*h
   return im.crop((x,y,x+w,y+h))
 raise ValueError(i)
for tid in sorted(wanted):
 tile=tiles[tid]; fg=crop(index(tile.get('fg'))); bg=crop(index(tile.get('bg')))
 im=Image.new('RGBA',(fg or bg).size)
 if bg:im.alpha_composite(bg.resize(im.size))
 if fg:im.alpha_composite(fg)
 pixels=[(a<<24)|(r<<16)|(g<<8)|b for r,g,b,a in im.getdata()]
 (out/(tid+'.bin')).write_bytes(struct.pack('<ii',*im.size)+struct.pack('<'+'I'*len(pixels),*pixels))
print('Extracted',len(wanted),'fixture sprites from the game tileset')
