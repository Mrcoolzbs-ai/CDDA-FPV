#!/usr/bin/env python3
"""Original deterministic CC0 procedural test sounds; no sampled game assets."""
import json, math, random, wave
from pathlib import Path
out=Path(__file__).resolve().parents[2]/'data/sound/FPV'
out.mkdir(parents=True,exist_ok=True)
rate=22050
rng=random.Random(504)
def emit(name,seconds,kind):
    n=int(seconds*rate); values=[]; low=0
    for i in range(n):
        t=i/rate; u=i/n; noise=rng.uniform(-1,1); low=.96*low+.04*noise
        if kind=='step': v=(noise*.4+math.sin(2*math.pi*95*t)*.45)*math.exp(-24*t)
        elif kind=='gun': v=(noise*.8+math.sin(2*math.pi*64*t)*.2)*math.exp(-12*t)
        elif kind=='hit': v=(noise*.6+math.sin(2*math.pi*170*t)*.2)*math.exp(-19*t)
        elif kind=='door': v=(noise*.25+math.sin(2*math.pi*(180-95*u)*t)*.25)*math.sin(math.pi*u)**2
        elif kind=='beep': v=math.sin(2*math.pi*650*t)*.16*math.sin(math.pi*u)**2
        else:
            # Integer cycles make the ambient components continuous at loop boundaries.
            base=sum(math.sin(2*math.pi*k*t/seconds+ph)*a for k,ph,a in [(37,.4,.04),(71,.8,.03),(113,1.1,.02),(197,.6,.015)])
            v=base + (low*.25 if kind=='rain' else low*.08)
            v*=min(1,i/400,(n-1-i)/400)
        values.append(max(-32767,min(32767,int(v*26000))))
    import array
    with wave.open(str(out/(name+'.wav')),'wb') as w:
        w.setparams((1,2,rate,0,'NONE','not compressed')); w.writeframes(array.array('h',values).tobytes())
for name,sec,kind in [('step',.22,'step'),('gun',.8,'gun'),('hit',.4,'hit'),('door',.4,'door'),('beep',.08,'beep'),('wind',8,'wind'),('rain',8,'rain')]:emit(name,sec,kind)
rows=[]
def event(id,variants,file,volume):
    for variant in variants: rows.append(dict(type='sound_effect',id=id,variant=variant,volume=volume,files=[file+'.wav']))
event('plmove',['default','walk_water','walk_barefoot','walk_grass','walk_dirt','walk_metal','walk_tarmac','clear_obstacle'],'step',65)
for id in ['fire_gun','fire_gun_distant']:event(id,['default'],'gun',85)
event('melee_swing',['default','small_bash','big_bash','small_cutting','big_cutting'],'hit',60)
for id in ['melee_hit_metal','melee_hit_flesh']:event(id,['default'],'hit',70)
event('bullet_hit',['hit_wall','hit_metal','hit_flesh'],'hit',65)
for id in ['open_door','close_door']:event(id,['default'],'door',60)
for id in ['menu_move','menu_error']:event(id,['default'],'beep',45)
event('environment',['daytime','nighttime','WEATHER_CLEAR','WEATHER_SUNNY','WEATHER_CLOUDY'],'wind',30)
event('environment',['underground','indoors'],'wind',8)
event('environment',['indoors_rain','WEATHER_DRIZZLE','WEATHER_RAINY','WEATHER_RAINSTORM','WEATHER_THUNDER','WEATHER_FLURRIES','WEATHER_SNOWSTORM','WEATHER_SNOW','WEATHER_PORTAL_STORM'],'rain',38)
(out/'soundset.json').write_text(json.dumps(rows,indent=2)+'\n')
(out/'soundpack.txt').write_text('NAME: fpv-synth\nVIEW: FPV Synth (basic test audio)\n')
(out/'LICENSE.txt').write_text('These original procedurally generated WAV sounds and their generator are dedicated to the public domain under CC0 1.0. https://creativecommons.org/publicdomain/zero/1.0/\n')
print(f'Generated {len(rows)} sound mappings, 7 original WAV files')
