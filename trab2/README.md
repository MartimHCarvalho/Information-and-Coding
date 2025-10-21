# Part-I

## Exercise 1

```bash

```

## Exercise 2
### Compile

```bash
g++ -std=c++17 -o image_effects main.cpp Image.cpp Negative.cpp Mirror.cpp Rotate.cpp Intensity.cpp
```

### Test

```bash
./image_effects <input_file> <output_file> <operation> [<value>]
```

- **input_file**: path to the original image file.  
- **output_file**: name of the output file to create.  
- **operation**: which operation to perform (see list below).  
- **value**: extra parameter required for some operations (e.g., rotation, intensity).

#### Supported operations and examples:

| Operation     | Description                        | Example usage                               |
|----------------|------------------------------------|---------------------------------------------|
| `negative`     | Create the image negative          | `./image_effects <input_file> <output_file> negative` |
| `mirror_h`     | Flip horizontally                  | `./image_effects <input_file> <output_file> mirror_h` |
| `mirror_v`     | Flip vertically                    | `./image_effects <input_file> <output_file> mirror_v` |
| `rotate`       | Rotate by multiples of 90°         | `./image_effects <input_file> <output_file> rotate 2` (180°) |
| `intensity`    | Adjust brightness (+ or - value)   | `./image_effects <input_file> <output_file> intensity 40` |

---
# Part-II

## Exercise 3

```bash

```

---
# Part-III

## Exercise 4

```bash

```

---
# Part-IV


```bash


```


## Exercise 5

```bash

```