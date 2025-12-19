# Análise Completa de Frequência de Bytes - Ficheiro SafeTensors (513.13 MB)

## Informações do Modelo

- **Ficheiro**: test/model.safetensors
- **Tamanho total**: 513.13 MB de dados de tensores
- **Formato**: F32 (Float32, não BFloat16!)
- **Número de tensores**: 272
- **Total de parâmetros**: 134,515,008
- **Valores analisados**: 269,030,016 bytes

## Tabela de Frequência de Bytes

| Byte Value | Frequência (ocorrências) | Percentagem | Cumulativo | Interpretação |
|------------|-------------------------|-------------|------------|---------------|
| 0x00 | 269,581,080 | 50.10% | 50.10% | Byte zero (alta frequência por zeros no modelo) |
| 0xbe | 35,783,548 | 6.65% | 56.75% | Negative small value |
| 0x3e | 35,534,192 | 6.60% | 63.35% | Positive small value |
| 0xbd | 22,005,817 | 4.09% | 67.44% | Negative smaller value |
| 0x3d | 21,891,231 | 4.07% | 71.51% | Positive smaller value |
| 0xbc | 6,476,891 | 1.20% | 72.71% | Negative value |
| 0x3c | 6,444,222 | 1.20% | 73.91% | Positive value |
| 0xbf | 3,237,860 | 0.60% | 74.51% | Negative larger value |
| 0x3f | 3,208,008 | 0.60% | 75.11% | Positive larger value |
| 0xbb | 2,039,586 | 0.38% | 75.49% | Negative very small value |

## Análise por Componentes (High/Low Bytes)

### High Bytes (Exponent + Sign)
| Byte Value | Percentagem |
|------------|-------------|
| 0x00 | 50.20% |
| 0x81 | 0.29% |
| 0x82 | 0.29% |
| 0x83 | 0.29% |
| 0x84 | 0.28% |

### Low Bytes (Mantissa)
| Byte Value | Percentagem |
|------------|-------------|
| 0x00 | 50.00% |
| 0xbe | 13.11% |
| 0x3e | 13.02% |
| 0xbd | 7.98% |
| 0x3d | 7.95% |

## Entropia e Compressibilidade

| Métrica | Valor | % do Máximo |
|---------|-------|-------------|
| Entropia geral | 4.10 bits/byte | 51.2% |
| Entropia high bytes | 4.97 bits/byte | 62.1% |
| Entropia low bytes | 2.35 bits/byte | 29.4% |
| Potencial de compressão teórico | 48.8% | - |
| Ratio máximo (Shannon) | 1.95× | - |
| Ratio prático esperado | 1.27× | - |

## Análise de Valores BFloat16

| Característica | Contagem | Percentagem |
|----------------|----------|-------------|
| Zeros | 134,515,008 | 50.00% |
| Valores positivos | 67,099,205 | 24.94% |
| Valores negativos | 67,415,803 | 25.06% |
| Valores pequenos (\|x\| < 0.1) | 193,295,340 | 71.85% |

## Observações Importantes

⚠️ **NOTA CRÍTICA**: Este ficheiro usa **Float32 (F32)**, não BFloat16 como esperado nos dados anteriores!

### Principais Descobertas:

1. **50% de zeros**: Metade do ficheiro é composto por bytes 0x00, indicando sparsidade massiva
2. **Top 2 bytes não-zero** (0xbe, 0x3e): Apenas 13.25% combinados (muito diferente dos 28.33% em BFloat16)
3. **Entropia baixa**: 4.10 bits/byte (vs 6.18 bits/byte em BFloat16)
4. **Byte reordering ineficaz**: Não reduz entropia (devido à dominância de zeros)

### Comparação com Dados Anteriores (BFloat16):

| Métrica | BFloat16 (10MB sample) | Float32 (513MB full) |
|---------|------------------------|----------------------|
| Top byte | 0x3c (14.21%) | 0x00 (50.10%) |
| Top 2 cumulativo | 28.33% | 56.75% |
| Entropia | 6.18 bits/byte | 4.10 bits/byte |
| Zeros | ~0% | 50.00% |

**Conclusão**: Este modelo tem características completamente diferentes - é extremamente esparso (50% zeros) e usa Float32, não BFloat16.
