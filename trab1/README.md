# To build de project
```bash
cd src
make
cd .. 
```

## Tests
```bash
cd test
../bin/wav_cp sample.wav copy.wav # copies "sample.wav" into "copy.wav"
../bin/wav_dct sample.wav out.wav # generates a DCT "compressed" version
 ```
 
## Exercise 1 
---
```bash
../bin/wav_hist <input_file> <output_file> <channel> <k>
```
>channel = 0 → Left </br>
 channel = 1 → Right </br>
 channel = 2 → Mid </br>
 channel = 3 → Side </br>
 k = número de bins do histograma

## Exercise 2
---
```bash
../bin/wav_quant <input_file> <num_bits> <output_file>
```

## Exercise 3

---
## Exercise 4
---
```bash
../bin/wav_effects <input_file> <output_file> <effect_type>
```

## Exercise 6
---
```bash
cd part2/bit_stream/src
make

../bin/wav_quant_enc <input_file> <num_bits> <output_file>
../bin/wav_quant_dec <input_file> <output_file>
```
