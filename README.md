# Альфа Блендинг

## Цель

Знакомство с методами оптимизации программ с ипользованием SIMD инструкций.

## Программа

Данная программа совмещает две картинки путём накладывания одной на другую с учётом прозрачности передней картинки. Накладываемое изображение должно быть по размеру не меньше фонового, а также их размеры (длина и ширина) должны быть кратны 16 для возможности оптимизированного расчёта с помощью SIMD инструкций. 

Программа поддерживает аргументы командной строки:

**<смещение передней картинки по оси X>**\
**<смещение передней картинки по оси Y>**\
**<путь к к переднему изображению>**\
**<путь к фоновому изображению>**.

По умолчанию значения равны:

**0, 0, img.png, background.jpg**

## Оптимизация

Ускорения расчётов удалось достичь путём использования инструкций AVX512. С их помощью удаётся за одну итерацию обрабатывать сразу 16 пикселей.

Рассмотрим фрагменты кода для расчёта пикселя наивной и оптимизированной версий программы, а также их ассемблерное представление. Код на ассемблере получен с помощью сервиса godbolt.org. При компиляции использовались следующие флаги: 

	-O2 -mavx512f -march=native

Часть кода без оптимизации:

``` C
result[writer + 0] = ((imgR - backR)*Alpha + (backR << 8) - backR) >> 8;
result[writer + 1] = ((imgG - backG)*Alpha + (backG << 8) - backG) >> 8;
result[writer + 2] = ((imgB - backB)*Alpha + (backB << 8) - backB) >> 8;
```
Его ассемблерное представление:
``` asm
movzx   ebx, BYTE PTR [rdi]
movzx   ebp, BYTE PTR [rdi+3]
add     r9d, 4
movzx   r12d, BYTE PTR [r14+r11]
movzx   r10d, BYTE PTR [r14+1+r11]
lea     r15, [r11+1]
lea     r13, [r11+2]
movzx   edx, BYTE PTR [rdi+1]
movzx   r8d, BYTE PTR [r14+2+r11]
movzx   eax, BYTE PTR [rdi+2]
sub     ebx, r12d
mov     ecx, r12d
imul    ebx, ebp
sal     ecx, 8
sub     edx, r10d
imul    edx, ebp
sub     eax, r8d
imul    eax, ebp
add     ebx, ecx
sub     ebx, r12d
movzx   ecx, bh
mov     BYTE PTR [rsi+r11], cl
mov     r11d, r10d
sal     r11d, 8
add     edx, r11d
sub     edx, r10d
movzx   ecx, dh
mov     edx, r8d
sal     edx, 8
mov     BYTE PTR [rsi+r15], cl
add     eax, edx
sub     eax, r8d
movzx   eax, ah
mov     BYTE PTR [rsi+r13], al
```
Теперь версия с использованием SIMD команд:
``` C
image_l = _mm512_mullo_epi16(image_l, alpha_l);
image_h = _mm512_mullo_epi16(image_h, alpha_h);

back_l = _mm512_mullo_epi16(back_l, _mm512_sub_epi16(_255, alpha_l));
back_h = _mm512_mullo_epi16(back_h, _mm512_sub_epi16(_255, alpha_h));

__m512i  result_l = _mm512_srli_epi16(_mm512_add_epi16(image_l, back_l), 8);
__m512i  result_h = _mm512_srli_epi16(_mm512_add_epi16(image_h, back_h), 8);
```
И его ассемблерное представление:
``` asm
vmovdqu32       zmm5, ZMMWORD PTR [rdi-64+rcx]
movsx   rax, r9d
vmovdqu32       zmm6, ZMMWORD PTR [r10+rax]
vpmovzxbw       zmm1, ymm5
vextracti32x8   ymm4, zmm6, 0x1
vpmovzxbw       zmm3, ymm6
vpshufb zmm6, zmm1, zmm8
vpsubw  zmm0, zmm7, zmm6
vpmullw zmm1, zmm1, zmm6
vpmullw zmm0, zmm0, zmm3
vextracti32x8   ymm2, zmm5, 0x1
vpmovzxbw       zmm2, ymm2
vpshufb zmm5, zmm2, zmm8
vpmovzxbw       zmm4, ymm4
vpaddw  zmm0, zmm0, zmm1
vpsubw  zmm1, zmm7, zmm5
vpmullw zmm1, zmm1, zmm4
vpmullw zmm2, zmm2, zmm5
vpsrlw  zmm0, zmm0, 8
vpmovwb ymm0, zmm0
add     r9d, 64
vpaddw  zmm1, zmm1, zmm2
vpsrlw  zmm1, zmm1, 8
vpmovwb ymm1, zmm1
vinserti64x4    zmm0, zmm0, ymm1, 0x1
vmovdqu64       ZMMWORD PTR [r11+rax], zmm0
```
Видно, что, в отличие от первой версии, при использовании SIMD инструкций используются zmm регистры, что позволяет обрабатывать 16 пикселей за итерацию.

## Результаты измерений

Измерения проводились на ноутбуке, подключённом к сети. Размер накладываемого изображения - 512x512  пикселей. Расчёт пикселей в рамках одного запуска проводился 5000 раз. Всего было сделано 5 запусков. Среднее время расчёта приведено в таблице ниже.

Версия 		      | Время, мс
:--------------:|:----:
Наивная     		| 3578   
Оптимизированная| 438 

##  Вывод
В данной задаче использование SIMD команд позволяет ускорить расчёты изображения в 8.17 раз. Теоретический максимум при использовании инструкций AVX512 - 16 раз.




