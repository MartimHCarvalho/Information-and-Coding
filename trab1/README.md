# Part-I

```bash
cd part1/sndfile-examples/src
make
```

## Tests

```bash
cd test
../bin/wav_cp sample.wav copy.wav # copies "sample.wav" into "copy.wav"
../bin/wav_dct sample.wav out.wav # generates a DCT "compressed" version
```

## Exercise 1

```bash
../bin/wav_hist <input_file> <output_file> <channel> <k>
```

> channel = 0 → Left </br>
> channel = 1 → Right </br>
> channel = 2 → Mid </br>
> channel = 3 → Side </br>
> k = number of bits in the histogram

## Exercise 2

```bash
../bin/wav_quant <input_file> <num_bits> <output_file>
```

## Exercise 3

```bash
../bin/wav_quant <original_file> <modified_file>
```

## Exercise 4

```bash
../bin/wav_effects <input_file> <output_file> <effect_type>
```

> effect_type = 0 → None </br>
> effect_type = 1 → Single Echo </br>
> effect_type = 2 → Multiple Echo </br>
> effect_type = 3 → Amplitude Modulation </br>
> effect_type = 4 → Time Varying Delay </br>
> effect_type = 5 → Bass Boosted </br>

# Part-II

```bash
cd part2_3/bit_Stream/src
make
```

## Tests:

```bash
	cd test
	../bin/text2bin text-bits bin-bits # converts textual binary digits into binary bits
	../bin/bin2text bin-bits text-out # converts binary bits into textual binary digits
	cmp text-bits text-out # compares the original and recovered text files; should be silent
```

## Exercise 6

```bash
../bin/wav_quant_enc <input_file> <num_bits> <output_file>
../bin/wav_quant_dec <input_file> <output_file>
```

## Exercise 7

```bash
../bin/wav_dct_enc <input_file> <quant_step> <output_file>
../bin/wav_dct_dec <input_file> <output_file>
```
