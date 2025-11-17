# Information and Coding - Assignment 2
## Building

### Image Effects (Part 1)

#### Image Effects - Exe1
```bash
  make run-exe1 INPUT=in.ppm OUTPUT=out.ppm CHANNEL=number
```
**Available effects:** `0=Blue, 1=Green, 2=Red`

### Image Effects - Exe2
```bash
make run-effects INPUT=<input.ppm> OUTPUT=<output.ppm> EFFECT=<effect>
```

**Available effects:** `negative`, `mirror`, `rotate`, `intensity`

**Example:**
```bash
make run-effects INPUT=input.ppm OUTPUT=output.ppm EFFECT=negative
make run-effects INPUT=input.ppm OUTPUT=output.ppm EFFECT=rotate ANGLE=90
make run-effects INPUT=input.ppm OUTPUT=output.ppm EFFECT=intensity FACTOR=1.5
``` 
**Direct Order:**
```bash
./bin/image_effects <input.ppm> <output.ppm> <effect>
```
**Example:**
```bash
./bin/image_effects images/lena.ppm output/result.ppm mirror
```

## Audio Codec - Exe 4
**Test with synthetic data:**
```bash
make test-audio
```

**Test with a specified audio:**
```bash
make run-audio FILE=path/to/audio.wav
```

**Alternative:**
```bash
make run FILE=myaudio.wav  
```

## Image Codec - Exe 5
**Test with synthetic data:**
```bash
make test-image
```

**Test with your own image:**
```bash
make run-image FILE=path/to/image.pgm
```

**HELP:** `make help`