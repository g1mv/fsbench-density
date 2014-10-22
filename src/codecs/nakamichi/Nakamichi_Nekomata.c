// Nakamichi is 100% FREE LZSS SUPERFAST decompressor.
// Home of Nakamichi: www.sanmayce.com/Nakamichi/index.html

// Nakamichi_Nekomata.c, is a variant of 'Aratama'.
// Nakamichi_Kaidanji.c, is the very same '1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX_Kaidanji_FIX'.

// Results without m^2's tweak:
/*
D:\_KAZE\Nakamichi_Kaidanji_benchmark\Nakamichi_benchmark\Nakamichi_brutal_tests_Nekomata>timer32.exe Nakamichi_Nekomata\Nakamichi_Nekomata_GP.exe alice29.txt
Nakamichi 'Nekomata', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced.
Compressing 152089 bytes ...
-; Each rotation means 64KB are encoded; Done 100%
NumberOfFullLiterals (lower-the-better): 366
NumberOfShortMatches: 19460
NumberOfLongMatches: 7362
RAM-to-RAM performance: 50 KB/s.

Kernel  Time =     0.000 =    0%
User    Time =     2.609 =   86%
Process Time =     2.609 =   86%    Virtual  Memory =     33 MB
Global  Time =     3.015 =  100%    Physical Memory =      1 MB

D:\_KAZE\Nakamichi_Kaidanji_benchmark\Nakamichi_benchmark\Nakamichi_brutal_tests_Nekomata>timer32.exe Nakamichi_Nekomata\Nakamichi_Nekomata_GP.exe CalgaryCorpus.tar
Nakamichi 'Nekomata', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced.
Compressing 3153408 bytes ...
|; Each rotation means 64KB are encoded; Done 100%
NumberOfFullLiterals (lower-the-better): 21638
NumberOfShortMatches: 321809
NumberOfLongMatches: 178855
RAM-to-RAM performance: 31 KB/s.

Kernel  Time =     0.031 =    0%
User    Time =    93.125 =   94%
Process Time =    93.156 =   94%    Virtual  Memory =     39 MB
Global  Time =    98.750 =  100%    Physical Memory =      6 MB

D:\_KAZE\Nakamichi_Kaidanji_benchmark\Nakamichi_benchmark\Nakamichi_brutal_tests_Nekomata>timer32.exe Nakamichi_Nekomata\Nakamichi_Nekomata_GP.exe shaks12.txt
Nakamichi 'Nekomata', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced.
Compressing 5582655 bytes ...
/; Each rotation means 64KB are encoded; Done 100%
NumberOfFullLiterals (lower-the-better): 3395
NumberOfShortMatches: 758372
NumberOfLongMatches: 267520
RAM-to-RAM performance: 43 KB/s.

Kernel  Time =     0.046 =    0%
User    Time =   121.750 =   97%
Process Time =   121.796 =   97%    Virtual  Memory =     43 MB
Global  Time =   125.343 =  100%    Physical Memory =      9 MB

D:\_KAZE\Nakamichi_Kaidanji_benchmark\Nakamichi_benchmark\Nakamichi_brutal_tests_Nekomata>timer32.exe Nakamichi_Nekomata\Nakamichi_Nekomata_GP.exe dickens
Nakamichi 'Nekomata', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced.
Compressing 10192446 bytes ...
\; Each rotation means 64KB are encoded; Done 100%
NumberOfFullLiterals (lower-the-better): 5497
NumberOfShortMatches: 1525256
NumberOfLongMatches: 422593
RAM-to-RAM performance: 42 KB/s.

Kernel  Time =     0.109 =    0%
User    Time =   228.031 =   97%
Process Time =   228.140 =   97%    Virtual  Memory =     52 MB
Global  Time =   233.562 =  100%    Physical Memory =     16 MB

D:\_KAZE\Nakamichi_Kaidanji_benchmark\Nakamichi_benchmark\Nakamichi_brutal_tests_Nekomata>timer32.exe Nakamichi_Nekomata\Nakamichi_Nekomata_GP.exe enwik8
Nakamichi 'Nekomata', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced.
Compressing 100000000 bytes ...
/; Each rotation means 64KB are encoded; Done 100%
NumberOfFullLiterals (lower-the-better): 329532
NumberOfShortMatches: 12452997
NumberOfLongMatches: 4615880
RAM-to-RAM performance: 35 KB/s.

Kernel  Time =     1.843 =    0%
User    Time =  2663.171 =   98%
Process Time =  2665.015 =   98%    Virtual  Memory =    224 MB
Global  Time =  2715.640 =  100%    Physical Memory =    147 MB

D:\_KAZE\Nakamichi_Kaidanji_benchmark\Nakamichi_benchmark\Nakamichi_brutal_tests_Nekomata>timer32.exe Nakamichi_Nekomata\Nakamichi_Nekomata_GP.exe silesia.tar
Nakamichi 'Nekomata', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced.
Compressing 211948544 bytes ...
-; Each rotation means 64KB are encoded; Done 100%
NumberOfFullLiterals (lower-the-better): 3046097
NumberOfShortMatches: 16893795
NumberOfLongMatches: 13102393
RAM-to-RAM performance: 22 KB/s.

Kernel  Time =     4.968 =    0%
User    Time =  9305.468 =   98%
Process Time =  9310.437 =   99%    Virtual  Memory =    438 MB
Global  Time =  9404.140 =  100%    Physical Memory =    308 MB

D:\_KAZE\Nakamichi_Kaidanji_benchmark\Nakamichi_benchmark\Nakamichi_brutal_tests_Nekomata>timer32.exe Nakamichi_Nekomata\Nakamichi_Nekomata_GP.exe Bible_Bible_Bible.tar
Nakamichi 'Nekomata', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced.
Compressing 13846016 bytes ...
\; Each rotation means 64KB are encoded; Done 100%
NumberOfFullLiterals (lower-the-better): 6024
NumberOfShortMatches: 1505289
NumberOfLongMatches: 881898
RAM-to-RAM performance: 55 KB/s.

Kernel  Time =     0.140 =    0%
User    Time =   241.640 =   98%
Process Time =   241.781 =   99%    Virtual  Memory =     59 MB
Global  Time =   244.187 =  100%    Physical Memory =     20 MB

D:\_KAZE\Nakamichi_Kaidanji_benchmark\Nakamichi_benchmark\Nakamichi_brutal_tests_Nekomata>timer32.exe Nakamichi_Nekomata\Nakamichi_Nekomata_GP.exe Goyathlay.txt
Nakamichi 'Nekomata', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced.
Compressing 11546860 bytes ...
|; Each rotation means 64KB are encoded; Done 100%
NumberOfFullLiterals (lower-the-better): 1231
NumberOfShortMatches: 829550
NumberOfLongMatches: 957892
RAM-to-RAM performance: 57 KB/s.

Kernel  Time =     0.109 =    0%
User    Time =   193.593 =   98%
Process Time =   193.703 =   98%    Virtual  Memory =     55 MB
Global  Time =   195.906 =  100%    Physical Memory =     16 MB

D:\_KAZE\Nakamichi_Kaidanji_benchmark\Nakamichi_benchmark\Nakamichi_brutal_tests_Nekomata>timer32.exe Nakamichi_Nekomata\Nakamichi_Nekomata_GP.exe Large_traffic_log_file_of_a_popular_website_fp.log
Nakamichi 'Nekomata', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced.
Compressing 20617071 bytes ...
-; Each rotation means 64KB are encoded; Done 100%
NumberOfFullLiterals (lower-the-better): 20380
NumberOfShortMatches: 210202
NumberOfLongMatches: 2427685
RAM-to-RAM performance: 224 KB/s.

Kernel  Time =     0.125 =    0%
User    Time =    88.828 =   98%
Process Time =    88.953 =   98%    Virtual  Memory =     72 MB
Global  Time =    90.359 =  100%    Physical Memory =     26 MB

D:\_KAZE\Nakamichi_Kaidanji_benchmark\Nakamichi_benchmark\Nakamichi_brutal_tests_Nekomata>timer32.exe Nakamichi_Nekomata\Nakamichi_Nekomata_GP.exe New_Shorter_Oxford_English_Dictionary_fifth_edition.tar
Nakamichi 'Nekomata', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced.
Compressing 132728832 bytes ...
/; Each rotation means 64KB are encoded; Done 100%
NumberOfFullLiterals (lower-the-better): 162229
NumberOfShortMatches: 7279000
NumberOfLongMatches: 11761618
RAM-to-RAM performance: 67 KB/s.

Kernel  Time =     1.453 =    0%
User    Time =  1896.906 =   99%
Process Time =  1898.359 =   99%    Virtual  Memory =    286 MB
Global  Time =  1915.156 =  100%    Physical Memory =    177 MB

D:\_KAZE\Nakamichi_Kaidanji_benchmark\Nakamichi_benchmark\Nakamichi_brutal_tests_Nekomata>timer32.exe Nakamichi_Nekomata\Nakamichi_Nekomata_GP.exe OSHO.TXT
Nakamichi 'Nekomata', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced.
Compressing 206908949 bytes ...
/; Each rotation means 64KB are encoded; Done 100%
NumberOfFullLiterals (lower-the-better): 178791
NumberOfShortMatches: 21627301
NumberOfLongMatches: 13547634
RAM-to-RAM performance: 53 KB/s.

Kernel  Time =     2.328 =    0%
User    Time =  3720.187 =   99%
Process Time =  3722.515 =   99%    Virtual  Memory =    428 MB
Global  Time =  3755.593 =  100%    Physical Memory =    283 MB

D:\_KAZE\Nakamichi_Kaidanji_benchmark\Nakamichi_benchmark\Nakamichi_brutal_tests_Nekomata>timer32.exe Nakamichi_Nekomata\Nakamichi_Nekomata_GP.exe Kazahana_on.PAGODA-order-5.txt
Nakamichi 'Nekomata', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced.
Compressing 846351894 bytes ...
-; Each rotation means 64KB are encoded; Done 100%
NumberOfFullLiterals (lower-the-better): 327685
NumberOfShortMatches: 19455517
NumberOfLongMatches: 92634386
RAM-to-RAM performance: 122 KB/s.

Kernel  Time =     6.078 =    0%
User    Time =  6672.796 =   98%
Process Time =  6678.875 =   99%    Virtual  Memory =   1650 MB
Global  Time =  6741.484 =  100%    Physical Memory =   1061 MB

09/26/1996  04:51 PM           152,089 alice29.txt
06/27/2014  05:45 AM            75,024 alice29.txt.Nakamichi
06/07/2014  02:59 PM        13,846,016 Bible_Bible_Bible.tar
06/27/2014  09:18 AM         5,914,996 Bible_Bible_Bible.tar.Nakamichi
05/16/2014  07:22 AM         3,153,408 CalgaryCorpus.tar
06/27/2014  05:46 AM         1,572,102 CalgaryCorpus.tar.Nakamichi
05/16/2014  07:22 AM        10,192,446 dickens
06/27/2014  05:52 AM         4,953,552 dickens.Nakamichi
05/16/2014  07:22 AM       100,000,000 enwik8
06/27/2014  06:38 AM        52,240,129 enwik8.Nakamichi
05/16/2014  07:22 AM        11,546,860 Goyathlay.txt
06/27/2014  09:22 AM         4,405,462 Goyathlay.txt.Nakamichi
05/16/2014  07:22 AM       846,351,894 Kazahana_on.PAGODA-order-5.txt
06/27/2014  12:50 PM       261,523,352 Kazahana_on.PAGODA-order-5.txt.Nakamichi
05/16/2014  07:22 AM        20,617,071 Large_traffic_log_file_of_a_popular_website_fp.log
06/27/2014  09:23 AM         5,732,800 Large_traffic_log_file_of_a_popular_website_fp.log.Nakamichi
06/05/2014  07:35 PM       132,728,832 New_Shorter_Oxford_English_Dictionary_fifth_edition.tar
06/27/2014  09:55 AM        50,945,409 New_Shorter_Oxford_English_Dictionary_fifth_edition.tar.Nakamichi
05/16/2014  07:22 AM       206,908,949 OSHO.TXT
06/27/2014  10:58 AM        87,735,794 OSHO.TXT.Nakamichi
06/03/2014  07:35 PM         5,582,655 shaks12.txt
06/27/2014  05:48 AM         2,656,312 shaks12.txt.Nakamichi
05/16/2014  07:22 AM       211,948,544 silesia.tar
06/27/2014  09:14 AM       109,164,162 silesia.tar.Nakamichi

D:\_KAZE\Nakamichi_Kaidanji_benchmark\Nakamichi_benchmark\Nakamichi_brutal_tests_Nekomata\more>Nakamichi_Nekomata_GP.exe 3333_Latin_Powers.TXT
Nakamichi 'Nekomata', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced.
Compressing 98848 bytes ...
/; Each rotation means 64KB are encoded; Done 100%
NumberOfFullLiterals (lower-the-better): 6
NumberOfShortMatches: 525
NumberOfLongMatches: 12056
RAM-to-RAM performance: 384 KB/s.

D:\_KAZE\Nakamichi_Kaidanji_benchmark\Nakamichi_benchmark\Nakamichi_brutal_tests_Nekomata\more>Nakamichi_Nekomata_GP.exe A_Latin-English_Dictionary_Wordlist.txt
Nakamichi 'Nekomata', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced.
Compressing 3473608 bytes ...
/; Each rotation means 64KB are encoded; Done 100%
NumberOfFullLiterals (lower-the-better): 3140
NumberOfShortMatches: 405336
NumberOfLongMatches: 194059
RAM-to-RAM performance: 38 KB/s.

D:\_KAZE\Nakamichi_Kaidanji_benchmark\Nakamichi_benchmark\Nakamichi_brutal_tests_Nekomata\more>Nakamichi_Nekomata_GP.exe edict_(JIS_X_0208_coding_in_EUC-JP_encapsulation)
Nakamichi 'Nekomata', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced.
Compressing 15023476 bytes ...
/; Each rotation means 64KB are encoded; Done 100%
NumberOfFullLiterals (lower-the-better): 31211
NumberOfShortMatches: 1674279
NumberOfLongMatches: 804360
RAM-to-RAM performance: 32 KB/s.

D:\_KAZE\Nakamichi_Kaidanji_benchmark\Nakamichi_benchmark\Nakamichi_brutal_tests_Nekomata\more>Nakamichi_Nekomata_GP.exe Goyathlay.txt
Nakamichi 'Nekomata', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced.
Compressing 11546860 bytes ...
|; Each rotation means 64KB are encoded; Done 100%
NumberOfFullLiterals (lower-the-better): 1231
NumberOfShortMatches: 829550
NumberOfLongMatches: 957892
RAM-to-RAM performance: 48 KB/s.

D:\_KAZE\Nakamichi_Kaidanji_benchmark\Nakamichi_benchmark\Nakamichi_brutal_tests_Nekomata\more>Nakamichi_Nekomata_GP.exe JMdict_(Unicode-ISO-10646_coding_using_UTF-8_encapsulation)
Nakamichi 'Nekomata', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced.
Compressing 60681877 bytes ...
/; Each rotation means 64KB are encoded; Done 100%
NumberOfFullLiterals (lower-the-better): 108310
NumberOfShortMatches: 1496518
NumberOfLongMatches: 6426260
RAM-to-RAM performance: 82 KB/s.

D:\_KAZE\Nakamichi_Kaidanji_benchmark\Nakamichi_benchmark\Nakamichi_brutal_tests_Nekomata\more>Nakamichi_Nekomata_GP.exe Latin_DICTPAGE.RAW.TXT
Nakamichi 'Nekomata', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced.
Compressing 6175246 bytes ...
-; Each rotation means 64KB are encoded; Done 100%
NumberOfFullLiterals (lower-the-better): 6542
NumberOfShortMatches: 380058
NumberOfLongMatches: 531288
RAM-to-RAM performance: 53 KB/s.

D:\_KAZE\Nakamichi_Kaidanji_benchmark\Nakamichi_benchmark\Nakamichi_brutal_tests_Nekomata\more>Nakamichi_Nekomata_GP.exe MASAKARI_General-Purpose_Grade_English_Wordlist.wrd
Nakamichi 'Nekomata', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced.
Compressing 3903143 bytes ...
\; Each rotation means 64KB are encoded; Done 100%
NumberOfFullLiterals (lower-the-better): 724
NumberOfShortMatches: 454792
NumberOfLongMatches: 223771
RAM-to-RAM performance: 31 KB/s.

D:\_KAZE\Nakamichi_Kaidanji_benchmark\Nakamichi_benchmark\Nakamichi_brutal_tests_Nekomata\more>Nakamichi_Nekomata_GP.exe Sahih_Bukhari.tar
Nakamichi 'Nekomata', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced.
Compressing 4999168 bytes ...
|; Each rotation means 64KB are encoded; Done 100%
NumberOfFullLiterals (lower-the-better): 4721
NumberOfShortMatches: 432279
NumberOfLongMatches: 368840
RAM-to-RAM performance: 50 KB/s.

D:\_KAZE\Nakamichi_Kaidanji_benchmark\Nakamichi_benchmark\Nakamichi_brutal_tests_Nekomata\more>Nakamichi_Nekomata_GP.exe Thus_Spake_Zarathustra_by_Friedrich_Nietzsche_revision_4_ASCII.txt
Nakamichi 'Nekomata', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced.
Compressing 522911 bytes ...
\; Each rotation means 64KB are encoded; Done 100%
NumberOfFullLiterals (lower-the-better): 641
NumberOfShortMatches: 74115
NumberOfLongMatches: 23145
RAM-to-RAM performance: 38 KB/s.

05/16/2014  07:22 AM            98,848 3333_Latin_Powers.TXT
06/27/2014  03:25 PM            25,581 3333_Latin_Powers.TXT.Nakamichi
05/16/2014  07:22 AM         3,473,608 A_Latin-English_Dictionary_Wordlist.txt
06/27/2014  03:26 PM         1,628,011 A_Latin-English_Dictionary_Wordlist.txt.Nakamichi
05/16/2014  07:22 AM        15,023,476 edict_(JIS_X_0208_coding_in_EUC-JP_encapsulation)
06/27/2014  03:34 PM         7,539,094 edict_(JIS_X_0208_coding_in_EUC-JP_encapsulation).Nakamichi
05/16/2014  07:22 AM        11,546,860 Goyathlay.txt
06/27/2014  03:38 PM         4,405,462 Goyathlay.txt.Nakamichi
05/16/2014  07:22 AM        60,681,877 JMdict_(Unicode-ISO-10646_coding_using_UTF-8_encapsulation)
06/27/2014  03:50 PM        20,155,177 JMdict_(Unicode-ISO-10646_coding_using_UTF-8_encapsulation).Nakamichi
05/16/2014  07:22 AM         6,175,246 Latin_DICTPAGE.RAW.TXT
06/27/2014  03:51 PM         2,382,150 Latin_DICTPAGE.RAW.TXT.Nakamichi
05/16/2014  07:22 AM         3,903,143 MASAKARI_General-Purpose_Grade_English_Wordlist.wrd
06/27/2014  03:54 PM         1,787,974 MASAKARI_General-Purpose_Grade_English_Wordlist.wrd.Nakamichi
05/16/2014  07:22 AM         4,999,168 Sahih_Bukhari.tar
06/27/2014  03:55 PM         2,055,646 Sahih_Bukhari.tar.Nakamichi
05/16/2014  07:22 AM           522,911 Thus_Spake_Zarathustra_by_Friedrich_Nietzsche_revision_4_ASCII.txt
06/27/2014  03:55 PM           254,255 Thus_Spake_Zarathustra_by_Friedrich_Nietzsche_revision_4_ASCII.txt.Nakamichi
*/

// Nakamichi, revision 1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX_Kaidanji_FIX, written by Kaze, babealicious suggestion by m^2 enforced.
// Fixed! TO-DO: Known bug: the decompressed file sometimes has few additional bytes at the end.
// Change #1: Now instead of looking first in the leftmost end of the window a "preemptive" search is done 16*8*128 bytes before the rightmost end of the window, there is the hottest (cachewise&matchwise) zone, as a side effect the compression speed is much higher. Maybe in the future I will try hashing as well.
// Change #2: The full 16bits are used for offsets, 64KB window, that is.

// Compile line:
//icl /O3 Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX_Kaidanji_FIX.c -D_N_GP /FAcs
//ren Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX_Kaidanji_FIX.cod Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX_Kaidanji_FIX_GP.cod
//ren Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX_Kaidanji_FIX.exe Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX_Kaidanji_FIX_GP.exe
//icl /O3 /QxSSE2 Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX_Kaidanji_FIX.c -D_N_XMM /FAcs
//ren Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX_Kaidanji_FIX.cod Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX_Kaidanji_FIX_XMM.cod
//ren Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX_Kaidanji_FIX.exe Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX_Kaidanji_FIX_XMM.exe
//icl /O3 /QxAVX Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX_Kaidanji_FIX.c -D_N_YMM /FAcs
//ren Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX_Kaidanji_FIX.cod Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX_Kaidanji_FIX_YMM.cod
//ren Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX_Kaidanji_FIX.exe Nakamichi_r1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX_Kaidanji_FIX_YMM.exe

// Nakamichi, revision 1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy_FIX, written by Kaze, babealicious suggestion by m^2 enforced.
// Change #1: Nasty bug in Swampshine was fixed.
// Change #2: Sanity check in compression section was added thus avoiding 'index-Min_Match_Length' going below 0.

// Nakamichi, revision 1-RSSBO_1GB_Wordfetcher_TRIAD_NOmemcpy, written by Kaze, babealicious suggestion by m^2 enforced.
// Change #1: 'memcpy' replaced by GP/XMM/YMM TRIADs.
// Nakamichi, revision 1-RSSBO_1GB_Wordfetcher_TRIAD, written by Kaze.
// Change #1: Decompression fetches WORD instead of BYTE+BYTE.
// Change #2: Decompression stores three times 64bit instead of memcpy() for all transfers <=24 bytes.
// Change #3: Fifteenth bit is used and then unused, 16KB -> 32KB -> 16KB.
// 32KB window disappoints speedwise, also sizewise:
/*
D:\_KAZE\_KAZE_GOLD\Nakamichi_projectOLD\Nakamichi_vs_Yappy>Nakamichi_r1-RSSBO_1GB_15bit_Wordfetcher.exe enwik8
Nakamichi, revision 1-RSSBO_1GB_15bit_Wordfetcher, written by Kaze, based on Nobuo Ito's LZSS source.
Compressing 100000000 bytes ...
-; Each rotation means 128KB are encoded; Done 100%
RAM-to-RAM performance: 130 KB/s.

D:\_KAZE\_KAZE_GOLD\Nakamichi_projectOLD\Nakamichi_vs_Yappy>Nakamichi_r1-RSSBO_1GB_15bit_Wordfetcher.exe enwik8.Nakamichi
Nakamichi, revision 1-RSSBO_1GB_15bit_Wordfetcher, written by Kaze, based on Nobuo Ito's LZSS source.
Decompressing 65693566 bytes ...
RAM-to-RAM performance: 358 MB/s.

D:\_KAZE\_KAZE_GOLD\Nakamichi_projectOLD\Nakamichi_vs_Yappy>Nakamichi_r1-RSSBO_1GB_15bit_Wordfetcher.exe enwik9
Nakamichi, revision 1-RSSBO_1GB_15bit_Wordfetcher, written by Kaze, based on Nobuo Ito's LZSS source.
Compressing 1000000000 bytes ...
/; Each rotation means 128KB are encoded; Done 100%
RAM-to-RAM performance: 150 KB/s.

D:\_KAZE\_KAZE_GOLD\Nakamichi_projectOLD\Nakamichi_vs_Yappy>Nakamichi_r1-RSSBO_1GB_15bit_Wordfetcher.exe enwik9.Nakamichi
Nakamichi, revision 1-RSSBO_1GB_15bit_Wordfetcher, written by Kaze, based on Nobuo Ito's LZSS source.
Decompressing 609319736 bytes ...
RAM-to-RAM performance: 379 MB/s.
*/
// 1-RSSBO_1GB vs 1-RSSBO_1GB_15bit_Wordfetcher (16KB/32KB respectively):
// 069,443,065 vs 065,693,566
// 641,441,055 vs 609,319,736

// Nakamichi, revision 1-RSSBO_1GB, written by Kaze.
// Based on Nobuo Ito's source, thanks Ito.
// The main goal of Nakamichi is to allow supersimple and superfast decoding for English x-grams (mainly) in pure C, or not, heh-heh.
// Natively Nakamichi is targeted as 64bit tool with 16 threads, helping Kazahana to traverse faster when I/O is not superior.
// In short, Nakamichi is intended as x-gram decompressor.

// Eightfold Path ~ the Buddhist path to nirvana, comprising eight aspects in which an aspirant must become practised; 
// eightfold way ~ (Physics), the grouping of hadrons into supermultiplets by means of SU(3)); (b) adverb to eight times the number or quantity: OE.

// Note1: Fifteenth bit is not used, making the window wider by 1bit i.e. 32KB is not tempting, rather I think to use it as a flag: 8bytes/16bytes.
// Note2: English x-grams are as English texts but more redundant, in other words they are phraselists in most cases, sometimes wordlists.
// Note3: On OSHO.TXT, being a typical English text, Nakamichi's compression ratio is among the worsts:
//        206,908,949 OSHO.TXT
//        125,022,859 OSHO.TXT.Nakamichi
//        It struggles with English texts but decomprression speed is quite sweet (Core 2 T7500 2200MHz, 32bit code):
//        Nakamichi, revision 1-, written by Kaze.
//        Decompressing 125022859 bytes ...
//        RAM-to-RAM performance: 477681 KB/s.      
// Note4: Also I wanted to see how my 'Railgun_Swampshine_BailOut', being a HEAVYGUN i.e. with big overhead and latency, hits in a real-world application.

// Quick notes on PAGODAs (the padded x-gram lists):
// Every single word in English has its own PAGODA, in example below 'on' PAGODA is given (Kazahana_on.PAGODA-order-5.txt):
// PAGODA order 5 (i.e. with 5 tiers) has 5*(5+1)/2=15 subtiers, they are concatenated and space-padded in order to form the pillar 'on':
/*
D:\_KAZE\Nakamichi_r1-RSSBO>dir \_GW\ka*

04/12/2014  05:07 AM                14 Kazahana_on.1-1.txt
04/12/2014  05:07 AM         1,635,389 Kazahana_on.2-1.txt
04/12/2014  05:07 AM         1,906,734 Kazahana_on.2-2.txt
04/12/2014  05:07 AM        10,891,415 Kazahana_on.3-1.txt
04/12/2014  05:07 AM        15,797,703 Kazahana_on.3-2.txt
04/12/2014  05:07 AM        20,419,280 Kazahana_on.3-3.txt
04/12/2014  05:07 AM        22,141,823 Kazahana_on.4-1.txt
04/12/2014  05:07 AM        36,002,113 Kazahana_on.4-2.txt
04/12/2014  05:07 AM        33,236,772 Kazahana_on.4-3.txt
04/12/2014  05:07 AM        33,902,425 Kazahana_on.4-4.txt
04/12/2014  05:07 AM        24,795,989 Kazahana_on.5-1.txt
04/12/2014  05:07 AM        30,766,220 Kazahana_on.5-2.txt
04/12/2014  05:07 AM        38,982,816 Kazahana_on.5-3.txt
04/12/2014  05:07 AM        38,089,575 Kazahana_on.5-4.txt
04/12/2014  05:07 AM        34,309,057 Kazahana_on.5-5.txt
04/12/2014  05:07 AM       846,351,894 Kazahana_on.PAGODA-order-5.txt

D:\_KAZE\Nakamichi_r1-RSSBO>type \_GW\Kazahana_on.1-1.txt
9,999,999       on

D:\_KAZE\Nakamichi_r1-RSSBO>type \_GW\Kazahana_on.2-1.txt
9,999,999       on_the
1,148,054       on_his
0,559,694       on_her
0,487,856       on_this
0,399,485       on_your
0,381,570       on_my
0,367,282       on_their
...

D:\_KAZE\Nakamichi_r1-RSSBO>type \_GW\Kazahana_on.2-2.txt
0,545,191       based_on
0,397,408       and_on
0,334,266       go_on
0,329,561       went_on
0,263,035       was_on
0,246,332       it_on
0,229,041       down_on
0,202,151       going_on
...

D:\_KAZE\Nakamichi_r1-RSSBO>type \_GW\Kazahana_on.5-5.txt
0,083,564       foundation_osho_s_books_on
0,012,404       medium_it_may_be_on
0,012,354       if_you_received_it_on
0,012,152       medium_they_may_be_on
0,012,144       agree_to_also_provide_on
0,012,139       a_united_states_copyright_on
0,008,067       we_are_constantly_working_on
0,008,067       questions_we_have_received_on
0,006,847       file_was_first_posted_on
0,006,441       of_we_are_already_on
0,006,279       you_received_this_ebook_on
0,005,865       you_received_this_etext_on
0,005,833       to_keep_an_eye_on
...

D:\_KAZE\Nakamichi_r1-RSSBO>dir

04/12/2014  05:07 AM       846,351,894 Kazahana_on.PAGODA-order-5.txt

D:\_KAZE\Nakamichi_r1-RSSBO>Nakamichi.exe Kazahana_on.PAGODA-order-5.txt
Nakamichi, revision 1-RSSBO, written by Kaze.
Compressing 846351894 bytes ...
/; Each rotation means 128KB are encoded; Done 100%
RAM-to-RAM performance: 512 KB/s.

D:\_KAZE\Nakamichi_r1-RSSBO>dir

04/12/2014  05:07 AM       846,351,894 Kazahana_on.PAGODA-order-5.txt
04/15/2014  06:30 PM       293,049,398 Kazahana_on.PAGODA-order-5.txt.Nakamichi

D:\_KAZE\Nakamichi_r1-RSSBO>Nakamichi.exe Kazahana_on.PAGODA-order-5.txt.Nakamichi
Nakamichi, revision 1-RSSBO, written by Kaze.
Decompressing 293049398 bytes ...
RAM-to-RAM performance: 607 MB/s.

D:\_KAZE\Nakamichi_r1-RSSBO>Yappy.exe Kazahana_on.PAGODA-order-5.txt 4096
YAPPY: [b 4K] bytes 846351894 -> 191149889  22.6%  comp  33.8 MB/s  uncomp 875.4 MB/s

D:\_KAZE\Nakamichi_r1-RSSBO>Yappy.exe Kazahana_on.PAGODA-order-5.txt 8192
YAPPY: [b 8K] bytes 846351894 -> 184153244  21.8%  comp  35.0 MB/s  uncomp 898.3 MB/s

D:\_KAZE\Nakamichi_r1-RSSBO>Yappy.exe Kazahana_on.PAGODA-order-5.txt 16384
YAPPY: [b 16K] bytes 846351894 -> 180650931  21.3%  comp  28.8 MB/s  uncomp 906.4 MB/s

D:\_KAZE\Nakamichi_r1-RSSBO>Yappy.exe Kazahana_on.PAGODA-order-5.txt 32768
YAPPY: [b 32K] bytes 846351894 -> 178902966  21.1%  comp  35.0 MB/s  uncomp 906.4 MB/s

D:\_KAZE\Nakamichi_r1-RSSBO>Yappy.exe Kazahana_on.PAGODA-order-5.txt 65536
YAPPY: [b 64K] bytes 846351894 -> 178027899  21.0%  comp  34.5 MB/s  uncomp 914.6 MB/s

D:\_KAZE\Nakamichi_r1-RSSBO>Yappy.exe Kazahana_on.PAGODA-order-5.txt 131072
YAPPY: [b 128K] bytes 846351894 -> 177591807  21.0%  comp  34.9 MB/s  uncomp 906.4 MB/s

D:\_KAZE\Nakamichi_r1-RSSBO>
*/

// During compilation use one of these, the granularity of the padded 'memcpy', 4x2x8/2x2x16/1x2x32/1x1x64 respectively as GP/XMM/YMM/ZMM, the maximum literal length reduced from 127 to 63:
#define _N_GP
//#define _N_XMM
//#define _N_YMM
//#define _N_ZMM

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> // uint64_t needed
#include <time.h>
#include <string.h>

#ifdef _N_XMM
#include <emmintrin.h> // SSE2 intrinsics
#include <smmintrin.h> // SSE4.1 intrinsics
#endif
#ifdef _N_YMM
#include <emmintrin.h> // SSE2 intrinsics
#include <smmintrin.h> // SSE4.1 intrinsics
#include <immintrin.h> // AVX intrinsics
#endif
#ifdef _N_ZMM
#include <emmintrin.h> // SSE2 intrinsics
#include <smmintrin.h> // SSE4.1 intrinsics
#include <immintrin.h> // AVX intrinsics
#include <zmmintrin.h> // AVX2 intrinsics, definitions and declarations for use with 512-bit compiler intrinsics.
#endif

#ifdef _N_XMM
void SlowCopy128bit (const char *SOURCE, char *TARGET) { _mm_storeu_si128((__m128i *)(TARGET), _mm_loadu_si128((const __m128i *)(SOURCE))); }
#endif
#ifdef _N_YMM
void SlowCopy128bit (const char *SOURCE, char *TARGET) { _mm_storeu_si128((__m128i *)(TARGET), _mm_loadu_si128((const __m128i *)(SOURCE))); }
#endif
#ifdef _N_ZMM
void SlowCopy128bit (const char *SOURCE, char *TARGET) { _mm_storeu_si128((__m128i *)(TARGET), _mm_loadu_si128((const __m128i *)(SOURCE))); }
#endif
/*
 * Move Unaligned Packed Integer Values
 * **** VMOVDQU ymm1, m256
 * **** VMOVDQU m256, ymm1
 * Moves 256 bits of packed integer values from the source operand to the
 * destination
 */
//extern __m256i __ICL_INTRINCC _mm256_loadu_si256(__m256i const *);
//extern void    __ICL_INTRINCC _mm256_storeu_si256(__m256i *, __m256i);
#ifdef _N_YMM
void SlowCopy256bit (const char *SOURCE, char *TARGET) { _mm256_storeu_si256((__m256i *)(TARGET), _mm256_loadu_si256((const __m256i *)(SOURCE))); }
#endif
//extern __m512i __ICL_INTRINCC _mm512_loadu_si512(void const*);
//extern void    __ICL_INTRINCC _mm512_storeu_si512(void*, __m512i);
#ifdef _N_ZMM
void SlowCopy512bit (const char *SOURCE, char *TARGET) { _mm512_storeu_si512((__m512i *)(TARGET), _mm512_loadu_si512((const __m512i *)(SOURCE))); }
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

// Comment it to see how slower 'BruteForce' is, for Wikipedia 100MB the ratio is 41KB/s versus 197KB/s.
#define ReplaceBruteForceWithRailgunSwampshineBailOut

static void SearchIntoSlidingWindow(unsigned int* retIndex, unsigned int* retMatch, char* refStart,char* refEnd,char* encStart,char* encEnd);
static unsigned int SlidingWindowVsLookAheadBuffer(char* refStart, char* refEnd, char* encStart, char* encEnd);
unsigned int NekomataCompress(char* ret, char* src, unsigned int srcSize);
unsigned int NekomataDecompress(char* ret, char* src, unsigned int srcSize);
static char * Railgun_Swampshine_BailOut(char * pbTarget, char * pbPattern, uint32_t cbTarget, uint32_t cbPattern);
static char * Railgun_Doublet (char * pbTarget, char * pbPattern, uint32_t cbTarget, uint32_t cbPattern);

// The pair SHORT/LONG to be respectively in range 3..8/9..24:
// 4/12:
// 846,351,894 Kazahana_on.PAGODA-order-5.txt
// 219,459,398 Kazahana_on.PAGODA-order-5.txt.Nakamichi
// 6/13:
// 846,351,894 Kazahana_on.PAGODA-order-5.txt
// 213,629,110 Kazahana_on.PAGODA-order-5.txt.Nakamichi
// 5/13:
//   846,351,894 Kazahana_on.PAGODA-order-5.txt
//   210,396,428 Kazahana_on.PAGODA-order-5.txt.Nakamichi
//   206,908,949 OSHO.TXT
//    99,739,184 OSHO.TXT.Nakamichi
// 1,000,000,000 enwik9
//   531,893,878 enwik9.Nakamichi
// 6/14:
// 846,351,894 Kazahana_on.PAGODA-order-5.txt
// 207,213,691 Kazahana_on.PAGODA-order-5.txt.Nakamichi
// 5/14:
// 846,351,894 Kazahana_on.PAGODA-order-5.txt
// 205,946,653 Kazahana_on.PAGODA-order-5.txt.Nakamichi
// 5/8:
// 1,000,000,000 enwik9
//   525,215,362 enwik9.Nakamichi
//   846,351,894 Kazahana_on.PAGODA-order-5.txt
//   271,833,018 Kazahana_on.PAGODA-order-5.txt.Nakamichi
//   206,908,949 OSHO.TXT
//    96,001,059 OSHO.TXT.Nakamichi

// Min_Match_Length=THRESHOLD=4 means 4 and bigger are to be encoded:
#define Min_Match_BAILOUT_Length (8)
#define Min_Match_Length (4)
// 5/13:
//      57,108,834 enwik8.Nakamichi
// 4/12:
//      56,396,919 enwik8.Nakamichi
// 3/11:
//      61,063,754 enwik8.Nakamichi

#define Min_Match_Length_SHORT (5)
#define OffsetBITS (16)
#define LengthBITS (1)

//12bit
//#define REF_SIZE (4095+Min_Match_Length)
//#define REF_SIZE ( ((1<<OffsetBITS)-1) + Min_Match_Length )
#define REF_SIZE ( ((1<<OffsetBITS)-1) )
//3bit
//#define ENC_SIZE (7+Min_Match_Length)
#define ENC_SIZE ( ((1<<LengthBITS)-1) + Min_Match_Length +8)
/*

int main( int argc, char *argv[] ) {
	FILE *fp;
	int SourceSize;
	int TargetSize;
	char* SourceBlock=NULL;
	char* TargetBlock=NULL;
	char* Nakamichi = ".Nakamichi\0";
	char NewFileName[256];
	clock_t clocks1, clocks2;

char *pointerALIGN;
int i, j;
clock_t clocks3, clocks4;
double duration;
int BandwidthFlag=0;

unsigned long long k;

	printf("Nakamichi 'Nekomata', written by Kaze, based on Nobuo Ito's LZSS source, babealicious suggestion by m^2 enforced.\n");
	if (argc==1) {
		printf("Usage: Nakamichi filename\n"); exit(13);
	}
	if (argc==3) BandwidthFlag=1;
	if ((fp = fopen(argv[1], "rb")) == NULL) {
		printf("Nakamichi: Can't open '%s' file.\n", argv[1]); exit(13);
	}
	fseek(fp, 0, SEEK_END);
	SourceSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	// If filename ends in '.Nakamichi' then mode is decompression otherwise compression.
	if (strcmp(argv[1]+(strlen(argv[1])-strlen(Nakamichi)), Nakamichi) == 0) {
	SourceBlock = (char*)malloc(SourceSize+512);
	//TargetBlock = (char*)malloc(5*SourceSize+512);
	TargetBlock = (char*)malloc(1024*1024*1024+512);
	fread(SourceBlock, 1, SourceSize, fp);
	fclose(fp);
		printf("Decompressing %d bytes ...\n", SourceSize );
		clocks1 = clock();
		while (clocks1 == clock());
		clocks1 = clock();
		TargetSize = Decompress(TargetBlock, SourceBlock, SourceSize);
		clocks2 = clock();
		k = (((float)1000*TargetSize/(clocks2 - clocks1 + 1))); k=k>>20;
		printf("RAM-to-RAM performance: %d MB/s.\n", k);
		strcpy(NewFileName, argv[1]);
		*( NewFileName + strlen(argv[1])-strlen(Nakamichi) ) = '\0';
	} else {
	SourceBlock = (char*)malloc(SourceSize+512);
	TargetBlock = (char*)malloc(SourceSize+512+32*1024*1024); //+32*1024*1024, some files may be expanded instead of compressed.
	fread(SourceBlock, 1, SourceSize, fp);
	fclose(fp);
		printf("Compressing %d bytes ...\n", SourceSize );
		clocks1 = clock();
		while (clocks1 == clock());
		clocks1 = clock();
		TargetSize = Compress(TargetBlock, SourceBlock, SourceSize);
		clocks2 = clock();
		k = (((float)1000*SourceSize/(clocks2 - clocks1 + 1))); k=k>>10;
		printf("RAM-to-RAM performance: %d KB/s.\n", k);
		strcpy(NewFileName, argv[1]);
		strcat(NewFileName, Nakamichi);
	}
	if ((fp = fopen(NewFileName, "wb")) == NULL) {
		printf("Nakamichi: Can't write '%s' file.\n", NewFileName); exit(13);
	}
	fwrite(TargetBlock, 1, TargetSize, fp);
	fclose(fp);

	if (BandwidthFlag) {
// Benchmark memcpy() [
pointerALIGN = TargetBlock + 64 - (((size_t)TargetBlock) % 64);
//offset=64-int((long)data&63);
printf("Memory pool starting address: %p ... ", pointerALIGN);
if (((uintptr_t)(const void *)pointerALIGN & (64 - 1)) == 0) printf( "64 byte aligned, OK\n"); else printf( "NOT 64 byte aligned, FAILURE\n");
clocks3 = clock();
while (clocks3 == clock());
clocks3 = clock();
printf("Copying a %dMB block 1024 times i.e. %dGB READ + %dGB WRITTEN ...\n", 256, 256, 256);
	for (i = 0; i < 1024; i++) {
	memcpy(pointerALIGN+256*1024*1024, pointerALIGN, 256*1024*1024);
	}
clocks4 = clock();
duration = (double) (clocks4 - clocks3 + 1);
printf("memcpy(): (%dMB block); %dMB copied in %d clocks or %.3fMB per clock\n", 256, 1024*( 256 ), (int) duration, (float)1024*( 256 )/ ((int) duration));
// Benchmark memcpy() ]
k = (((float)1000*TargetSize/(clocks2 - clocks1 + 1))); k=k>>20;
j = (float)1000*1024*( 256 )/ ((int) duration);
printf("RAM-to-RAM performance vs memcpy() ratio (bigger-the-better): %d%%\n", (int)(k*100/j));
	}

	free(TargetBlock);
	free(SourceBlock);
	exit(0);
}*/

void SearchIntoSlidingWindow(unsigned int* retIndex, unsigned int* retMatch, char* refStart,char* refEnd,char* encStart,char* encEnd){
	char* FoundAtPosition;
	unsigned int match=0;
	char* refStartHOTTER = refStart+((1<<OffsetBITS)-16*8*128);
	char* refStartHOTTER2 = refStart+((1<<OffsetBITS)-31*8*128);
	char* refStartHOTTERs = refStart+((1<<OffsetBITS)-16*8*128);
	char* refStartHOTTER2s = refStart+((1<<OffsetBITS)-31*8*128);
	char* refStart1 = refStart;
	char* refStarts = refStart;
	int k;
	*retIndex=0;
	*retMatch=0;

// ShortM=7, LongM=7+8

#ifdef ReplaceBruteForceWithRailgunSwampshineBailOut

	// Step #1: LONG MATCH is sought [
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) [
	while (refStartHOTTER < refEnd) {
	//FoundAtPosition = Railgun_Doublet(refStartHOTTER, encStart, (uint32_t)(refEnd-refStartHOTTER), Min_Match_Length);	
	FoundAtPosition = Railgun_Swampshine_BailOut(refStartHOTTER, encStart, (uint32_t)(refEnd-refStartHOTTER), Min_Match_Length +8/2);	
		if (FoundAtPosition!=NULL) {
			// Stupid sanity check, in next revision I will discard 'Min_Match_Length' additions/subtractions altogether:
			//if ( refEnd-FoundAtPosition >= Min_Match_Length ) {
			//if ( (refEnd-FoundAtPosition) & 0x07 ) { // Discard matches that have OFFSET with lower 3bits ALL zero.
			if ( ((refEnd-FoundAtPosition) & 0xF0)!=0 && ((refEnd-FoundAtPosition) & 0x08)==0x08 ) {
				*retMatch=Min_Match_Length +8/2;
				*retIndex=refEnd-FoundAtPosition;
				return;
			}
			refStartHOTTER=FoundAtPosition+1; // Exhaust the pool.
		} else break;
	}
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) ]
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) [
	while (refStartHOTTER2 < refEnd) {
	FoundAtPosition = Railgun_Swampshine_BailOut(refStartHOTTER2, encStart, (uint32_t)(refEnd-refStartHOTTER2), Min_Match_Length +8/2);	
		if (FoundAtPosition!=NULL) {
			// Stupid sanity check, in next revision I will discard 'Min_Match_Length' additions/subtractions altogether:
			//if ( refEnd-FoundAtPosition >= Min_Match_Length ) {
			//if ( (refEnd-FoundAtPosition) & 0x07 ) { // Discard matches that have OFFSET with lower 3bits ALL zero.
			if ( ((refEnd-FoundAtPosition) & 0xF0)!=0 && ((refEnd-FoundAtPosition) & 0x08)==0x08 ) {
				*retMatch=Min_Match_Length +8/2;
				*retIndex=refEnd-FoundAtPosition;
				return;
			}
			refStartHOTTER2=FoundAtPosition+1; // Exhaust the pool.
		} else break;
	}
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) ]
	while (refStart1 < refEnd) {
		FoundAtPosition = Railgun_Swampshine_BailOut(refStart1, encStart, (uint32_t)(refEnd-refStart1), Min_Match_Length +8/2);
		//FoundAtPosition = Railgun_Doublet(refStart, encStart, (uint32_t)(refEnd-refStart), 8);
		// For bigger windows 'Doublet' is slower:
		// Nakamichi, revision 1-RSSBO_1GB_15bit performance with 'Swampshine':
		// Compressing 846351894 bytes ...
		// RAM-to-RAM performance: 370 KB/s.
		// Nakamichi, revision 1-RSSBO_1GB_15bit performance with 'Doublet':
		// Compressing 846351894 bytes ...
		// RAM-to-RAM performance: 213 KB/s.
		if (FoundAtPosition!=NULL) {
			// Stupid sanity check, in next revision I will discard 'Min_Match_Length' additions/subtractions altogether:
			//if ( refEnd-FoundAtPosition >= Min_Match_Length ) {
			//if ( (refEnd-FoundAtPosition) & 0x07 ) { // Discard matches that have OFFSET with lower 3bits ALL zero.
			if ( ((refEnd-FoundAtPosition) & 0xF0)!=0 && ((refEnd-FoundAtPosition) & 0x08)==0x08 ) {
				*retMatch=Min_Match_Length +8/2;
				*retIndex=refEnd-FoundAtPosition;
				return;
			}
			refStart1=FoundAtPosition+1; // Exhaust the pool.
		} else break;
	}
	// Step #1: LONG MATCH is sought ]

	// Step #2: SHORT MATCH is sought [
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) [
	while (refStartHOTTERs < refEnd) {
	FoundAtPosition = Railgun_Swampshine_BailOut(refStartHOTTERs, encStart, (uint32_t)(refEnd-refStartHOTTERs), Min_Match_Length);	
		if (FoundAtPosition!=NULL) {
			// Stupid sanity check, in next revision I will discard 'Min_Match_Length' additions/subtractions altogether:
			//if ( refEnd-FoundAtPosition >= Min_Match_Length ) {
			//if ( (refEnd-FoundAtPosition) & 0x07 ) { // Discard matches that have OFFSET with lower 3bits ALL zero.
			if ( ((refEnd-FoundAtPosition) & 0xF0)!=0 && ((refEnd-FoundAtPosition) & 0x08)==0 ) {
				*retMatch=Min_Match_Length;
				*retIndex=refEnd-FoundAtPosition;
				return;
			}
			refStartHOTTERs=FoundAtPosition+1; // Exhaust the pool.
		} else break;
	}
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) ]
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) [
	while (refStartHOTTER2s < refEnd) {
	FoundAtPosition = Railgun_Swampshine_BailOut(refStartHOTTER2s, encStart, (uint32_t)(refEnd-refStartHOTTER2s), Min_Match_Length);	
		if (FoundAtPosition!=NULL) {
			// Stupid sanity check, in next revision I will discard 'Min_Match_Length' additions/subtractions altogether:
			//if ( refEnd-FoundAtPosition >= Min_Match_Length ) {
			//if ( (refEnd-FoundAtPosition) & 0x07 ) { // Discard matches that have OFFSET with lower 3bits ALL zero.
			if ( ((refEnd-FoundAtPosition) & 0xF0)!=0 && ((refEnd-FoundAtPosition) & 0x08)==0 ) {
				*retMatch=Min_Match_Length;
				*retIndex=refEnd-FoundAtPosition;
				return;
			}
			refStartHOTTER2s=FoundAtPosition+1; // Exhaust the pool.
		} else break;
	}
	// Pre-emptive strike, matches should be sought close to the lookahead (cache-friendliness) ]
	while (refStarts < refEnd) {
		FoundAtPosition = Railgun_Swampshine_BailOut(refStarts, encStart, (uint32_t)(refEnd-refStarts), Min_Match_Length);
		//FoundAtPosition = Railgun_Doublet(refStart, encStart, (uint32_t)(refEnd-refStart), 8);
		// For bigger windows 'Doublet' is slower:
		// Nakamichi, revision 1-RSSBO_1GB_15bit performance with 'Swampshine':
		// Compressing 846351894 bytes ...
		// RAM-to-RAM performance: 370 KB/s.
		// Nakamichi, revision 1-RSSBO_1GB_15bit performance with 'Doublet':
		// Compressing 846351894 bytes ...
		// RAM-to-RAM performance: 213 KB/s.
		if (FoundAtPosition!=NULL) {
			// Stupid sanity check, in next revision I will discard 'Min_Match_Length' additions/subtractions altogether:
			//if ( refEnd-FoundAtPosition >= Min_Match_Length ) {
			//if ( (refEnd-FoundAtPosition) & 0x07 ) { // Discard matches that have OFFSET with lower 3bits ALL zero.
			if ( ((refEnd-FoundAtPosition) & 0xF0)!=0 && ((refEnd-FoundAtPosition) & 0x08)==0 ) {
				*retMatch=Min_Match_Length;
				*retIndex=refEnd-FoundAtPosition;
				return;
			}
			refStarts=FoundAtPosition+1; // Exhaust the pool.
		} else break;
	}
	// Step #2: SHORT MATCH is sought ]

#else				
	while(refStart < refEnd){
		match=SlidingWindowVsLookAheadBuffer(refStart,refEnd,encStart,encEnd);
		if(match > *retMatch){
			*retMatch=match;
			*retIndex=refEnd-refStart;
		}
		if(*retMatch >= Min_Match_BAILOUT_Length) break;
		refStart++;
	}
#endif
}

unsigned int SlidingWindowVsLookAheadBuffer( char* refStart, char* refEnd, char* encStart,char* encEnd){
	int ret = 0;
	while(refStart[ret] == encStart[ret]){
		if(&refStart[ret] >= refEnd) break;
		if(&encStart[ret] >= encEnd) break;
		ret++;
		if(ret >= Min_Match_BAILOUT_Length) break;
	}
	return ret;
}

unsigned int NekomataCompress(char* ret, char* src, unsigned int srcSize){
	unsigned int srcIndex=0;
	unsigned int retIndex=0;
	unsigned int index=0;
	unsigned int match=0;
	unsigned int notMatch=0;
	unsigned char* notMatchStart=NULL;
	char* refStart=NULL;
	char* encEnd=NULL;
	/*int Melnitchka=0;
	char *Auberge[4] = {"|\0","/\0","-\0","\\\0"};
	int ProgressIndicator;*/

	unsigned int NumberOfFullLiterals=0;
	int GLOBALlongM=0;
	int GLOBALshortM=0;

	while(srcIndex < srcSize){
		if(srcIndex>=REF_SIZE)
			refStart=&src[srcIndex-REF_SIZE];
		else
			refStart=src;
		if(srcIndex>=srcSize-ENC_SIZE)
			encEnd=&src[srcSize];
		else
			encEnd=&src[srcIndex+ENC_SIZE];
		// Fixing the stupid 'search-beyond-end' bug:
		if(srcIndex+ENC_SIZE < srcSize) {
			SearchIntoSlidingWindow(&index,&match,refStart,&src[srcIndex],&src[srcIndex],encEnd);
			if ( match==4 ) GLOBALshortM++; // debug
			if ( match==4+8/2 ) GLOBALlongM++; // debug
		}
		else
			match=0; // Nothing to find.
		//if ( match<Min_Match_Length ) {
		//if ( match<Min_Match_Length || match<8 ) {
  		if ( match==0 ) {
			if(notMatch==0){
				notMatchStart=&ret[retIndex];
				retIndex++;
			}
			//else if (notMatch==(127-64-32-16-8)) {
			else if (notMatch==(127-64-32-16-8 +1)) { // +1 in order to use all bits, m^2's tweak
NumberOfFullLiterals++;
				//*notMatchStart=(unsigned char)((127-64-32)<<3);
				*notMatchStart=(unsigned char)((127-64-32-16-8)<<0);
				notMatch=0;
				notMatchStart=&ret[retIndex];
				retIndex++;
			}
			ret[retIndex]=src[srcIndex];
			retIndex++;
			notMatch++;
			srcIndex++;
			/*if ((srcIndex-1) % (1<<16) > srcIndex % (1<<16)) {
				ProgressIndicator = (int)( (srcIndex+1)*(float)100/(srcSize+1) );
				printf("%s; Each rotation means 64KB are encoded; Done %d%%\r", Auberge[Melnitchka++], ProgressIndicator );
				Melnitchka = Melnitchka & 3; // 0 1 2 3: 00 01 10 11
			}*/
		} else {
			if(notMatch > 0){
				//*notMatchStart=(unsigned char)((notMatch)<<3);
				//*notMatchStart=(unsigned char)((notMatch)<<0);
				*notMatchStart=(unsigned char)((notMatch -1)<<0); // -1 in order to use all bits, m^2's tweak
				notMatch=0;
			}
// ---------------------| 
//                     \ /

			//ret[retIndex] = 0x80; // Assuming seventh/fifteenth bit is zero i.e. LONG MATCH i.e. Min_Match_BAILOUT_Length*4
	  		//if ( match==Min_Match_BAILOUT_Length ) ret[retIndex] = 0xC0; // 8bit&7bit set, SHORT MATCH if seventh/fifteenth bit is not zero i.e. Min_Match_BAILOUT_Length
//                     / \
// ---------------------|
/*
			ret[retIndex] = 0x01; // Assuming seventh/fifteenth bit is zero i.e. LONG MATCH i.e. Min_Match_BAILOUT_Length*4
	  		if ( match==Min_Match_BAILOUT_Length ) ret[retIndex] = 0x03; // 2bit&1bit set, LONG MATCH if 2bit is not zero i.e. Min_Match_BAILOUT_Length
*/
// No need of above, during compression we demanded lowest 2bits to be not 00.
			// 1bit+3bits+12bits:
			//ret[retIndex] = ret[retIndex] | ((match-Min_Match_Length)<<4);
			//ret[retIndex] = ret[retIndex] | (((index-Min_Match_Length) & 0x0F00)>>8);
			// 1bit+1bit+14bits:
			//ret[retIndex] = ret[retIndex] | ((match-Min_Match_Length)<<(8-(LengthBITS+1))); // No need to set the matchlength
// The fragment below is outrageously ineffective - instead of 8bit&7bit I have to use the lower TWO bits i.e. 2bit&1bit as flags, thus in decompressing one WORD can be fetched instead of two BYTE loads followed by SHR by 2.
// ---------------------| 
//                     \ /
			//ret[retIndex] = ret[retIndex] | (((index-Min_Match_Length) & 0x3F00)>>8); // 2+4+8=14
			//retIndex++;
			//ret[retIndex] = (char)((index-Min_Match_Length) & 0x00FF);
			//retIndex++;
//                     / \
// ---------------------|
			// Now the situation is like LOW:HIGH i.e. FF:3F i.e. 0x3FFF, 16bit&15bit used as flags,
			// should become LOW:HIGH i.e. FC:FF i.e. 0xFFFC, 2bit&1bit used as flags.
/*
			ret[retIndex] = ret[retIndex] | (((index-Min_Match_Length) & 0x00FF)<<2); // 6+8=14
			//ret[retIndex] = ret[retIndex] | (((index-Min_Match_Length) & 0x00FF)<<1); // 7+8=15
			retIndex++;
			ret[retIndex] = (char)(((index-Min_Match_Length) & 0x3FFF)>>6);
			//ret[retIndex] = (char)(((index-Min_Match_Length) & 0x7FFF)>>7);
			retIndex++;
*/
// No need of above, during compression we demanded lowest 2bits to be not 00, use the full 16bits and get rid of the stupid '+/-' Min_Match_Length.
			//if (index>0xFFFF) {printf ("\nFatal error: Overflow!\n"); exit(13);}
			memcpy(&ret[retIndex],&index,2); // copy lower 2 bytes
			retIndex++;
			retIndex++;
//                     / \
// ---------------------|
			srcIndex+=match;
			/*if ((srcIndex-match) % (1<<16) > srcIndex % (1<<16)) {
				ProgressIndicator = (int)( (srcIndex+1)*(float)100/(srcSize+1) );
				printf("%s; Each rotation means 64KB are encoded; Done %d%%\r", Auberge[Melnitchka++], ProgressIndicator );
				Melnitchka = Melnitchka & 3; // 0 1 2 3: 00 01 10 11
			}*/
		}
	}
	if(notMatch > 0){
		//*notMatchStart=(unsigned char)((notMatch)<<3);
		//*notMatchStart=(unsigned char)((notMatch)<<0);
		*notMatchStart=(unsigned char)((notMatch -1)<<0); // -1 in order to use all bits, m^2's tweak
	}
	/*printf("%s; Each rotation means 64KB are encoded; Done %d%%\n", Auberge[Melnitchka], 100 );
	printf("NumberOfFullLiterals (lower-the-better): %d\n", NumberOfFullLiterals );

printf("NumberOfShortMatches: %d\n", GLOBALshortM); // debug
printf("NumberOfLongMatches: %d\n", GLOBALlongM); // debug*/

	return retIndex;
}

unsigned int NekomataDecompress(char* ret, char* src, unsigned int srcSize){
	unsigned int srcIndex=0;
	unsigned int retIndex=0;
	unsigned int WORDpair;
	unsigned int Flag;
	uint64_t QWORD;

/*
	while(srcIndex < srcSize){
		WORDpair = *(unsigned short int*)&src[srcIndex];
		//if((WORDpair & 0x07) == 0){ // It is tempting to reduce literals even more, to 3x8 (instead of 31) would be nice:
		//if((WORDpair & 0xF0) == 0){ // It is tempting to reduce literals even more, to 3x8 (instead of 31) would be nice:
		if((WORDpair & 0xFF)<16){ // It is tempting to reduce literals even more, to 3x8 (instead of 31) would be nice:
				#ifdef _N_GP
				*(uint64_t*)(ret+retIndex+8*(0)) = *(uint64_t*)(src+srcIndex+1+8*(0));
				*(uint64_t*)(ret+retIndex+8*(1)) = *(uint64_t*)(src+srcIndex+1+8*(1));
				//*(uint64_t*)(ret+retIndex+8*(2)) = *(uint64_t*)(src+srcIndex+1+8*(2));
				//*(uint64_t*)(ret+retIndex+8*(3)) = *(uint64_t*)(src+srcIndex+1+8*(3));
				#endif
				#ifdef _N_XMM
				SlowCopy128bit((src+srcIndex+1+16*(0)), (ret+retIndex+16*(0)));
				//SlowCopy128bit((src+srcIndex+1+16*(1)), (ret+retIndex+16*(1)));
				#endif
				#ifdef _N_YMM
				//SlowCopy256bit((src+srcIndex+1+32*(0)), (ret+retIndex+32*(0)));
				SlowCopy128bit((src+srcIndex+1+16*(0)), (ret+retIndex+16*(0)));
				#endif
			//retIndex+=(WORDpair & 0xFF)>>3;
			//srcIndex+=(((WORDpair & 0xFF)>>3)+1);
			retIndex+=(WORDpair & 0xFF)>>0;
			srcIndex+=(((WORDpair & 0xFF)>>0)+1);
		}
		else{
			srcIndex=srcIndex+2;
			*(uint64_t*)(ret+retIndex) = *(uint64_t*)(ret+retIndex-WORDpair);
			retIndex+=Min_Match_Length;
		}
	}
*/

/*
	while(srcIndex < srcSize){
		WORDpair = *(unsigned short int*)&src[srcIndex];

		//if((WORDpair & 0xFF)<16){
				#ifdef _N_GP                                                          
				*(uint64_t*)(ret+retIndex+8*(0)) = *(uint64_t*)(src+srcIndex+1+8*(0));
				//*(uint64_t*)(ret+retIndex+8*(1)) = *(uint64_t*)(src+srcIndex+1+8*(1));
				//*(uint64_t*)(ret+retIndex+8*(2)) = *(uint64_t*)(src+srcIndex+1+8*(2));
				//*(uint64_t*)(ret+retIndex+8*(3)) = *(uint64_t*)(src+srcIndex+1+8*(3));
				#endif                                                                  
				#ifdef _N_XMM                                                           
				SlowCopy128bit((src+srcIndex+1+16*(0)), (ret+retIndex+16*(0)));         
				//SlowCopy128bit((src+srcIndex+1+16*(1)), (ret+retIndex+16*(1)));       
				#endif                                                                  
				#ifdef _N_YMM                                                           
				//SlowCopy256bit((src+srcIndex+1+32*(0)), (ret+retIndex+32*(0)));       
				SlowCopy128bit((src+srcIndex+1+16*(0)), (ret+retIndex+16*(0)));         
				#endif                                                                  
			//retIndex+=(WORDpair & 0xFF)>>3;                                               
			//srcIndex+=(((WORDpair & 0xFF)>>3)+1);                                         
			srcIndex+=(((WORDpair & 0xFF)>>0)+1);                                           
			retIndex+=(WORDpair & 0xFF)>>0;                                                 
		//}    
		//else{
		//if((WORDpair & 0xF0)){
		if((WORDpair & 0xFF)>=8){
			retIndex-=(WORDpair & 0xFF)>>0;                                                 
			*(uint64_t*)(ret+retIndex) = *(uint64_t*)(ret+retIndex-WORDpair);
			//srcIndex-=(((WORDpair & 0xFF)>>0)+1);                                           
			//srcIndex=srcIndex+2;                                             
			srcIndex= srcIndex - (((WORDpair & 0xFF)>>0)+1) +2;                                           
			retIndex+=Min_Match_Length;                                      
		}
		//}
	}
*/

	while(srcIndex < srcSize){
		WORDpair = *(unsigned short int*)&src[srcIndex];
		//QWORD = *(uint64_t*)&src[srcIndex];

			//*(uint64_t*)(ret+retIndex+8*(0)) = *(uint64_t*)(src+srcIndex+1+8*(0));
			//srcIndex+=(((WORDpair & 0xFF)>>0)+1);                                           
			//retIndex+=(WORDpair & 0xFF)>>0;                                                 

			//*(uint64_t*)(ret+retIndex) = *(uint64_t*)(ret+retIndex-WORDpair);
			//srcIndex=srcIndex+2;
			//retIndex+=Min_Match_Length;

				// WORDpair=0xf0; // 1
				// WORDpair=0x02; // 0
				//Flag=!(WORDpair & 0xF8);
				//Flag=!Flag;
			//Flag=!(WORDpair & 0xF8);
			//Flag=!(QWORD & 0xF0);
			Flag=!(WORDpair & 0xF0);

//				#ifdef _N_GP                                                          
				//*(uint64_t*)(ret+retIndex) = *(uint64_t*)( (unsigned int)(src+srcIndex+1)*(Flag) + (unsigned int)(ret+retIndex-WORDpair)*(!Flag) );
				//*(uint64_t*)(ret+retIndex+8) = *(uint64_t*)( (unsigned int)(src+srcIndex+1+8)*(Flag) + (unsigned int)(ret+retIndex-WORDpair+8)*(!Flag) );
				*(uint64_t*)(ret+retIndex) = *(uint64_t*)( (uint64_t)(src+srcIndex+1)*(Flag) + (uint64_t)(ret+retIndex-WORDpair)*(!Flag) );
//				*(uint64_t*)(ret+retIndex+8) = *(uint64_t*)( (uint64_t)(src+srcIndex+1+8)*(Flag) + (uint64_t)(ret+retIndex-WORDpair+8)*(!Flag) );
//				#endif                                                                  
//				#ifdef _N_XMM                                                         
				//SlowCopy128bit((char*)( (unsigned int)(src+srcIndex+1)*(Flag) + (unsigned int)(ret+retIndex-WORDpair)*(!Flag) ), (char*)(ret+retIndex));  
//				SlowCopy128bit((char*)( (uint64_t)(src+srcIndex+1)*(Flag) + (uint64_t)(ret+retIndex-WORDpair)*(!Flag) ), (char*)(ret+retIndex));  
//				#endif                                                                  
			srcIndex+= ((WORDpair & 0xFF)+1 +1)*(Flag) + (2)*(!Flag) ; // +1 due to m^2's tweak
			retIndex+= ((WORDpair & 0xFF) +1)*(Flag) + (Min_Match_Length + ((WORDpair & 0x08)>>1) )*(!Flag) ; // +1 due to m^2's tweak
	}

	return retIndex;
}

/*
; 'Nekomata' decompression loop, af-40+2=113 bytes long:
; mark_description "Intel(R) C++ Intel(R) 64 Compiler XE for applications running on Intel(R) 64, Version 12.1.1.258 Build 20111";
; mark_description "-O3 -D_N_GP -FAcs";

.B6.3::                         
  00040 45 33 e4         xor r12d, r12d                         
  00043 41 89 c5         mov r13d, eax                          
  00046 33 db            xor ebx, ebx                           
  00048 4c 03 e9         add r13, rcx                           
  0004b 45 0f b7 1c 12   movzx r11d, WORD PTR [r10+rdx]         
  00050 41 f7 c3 f0 00 
        00 00            test r11d, 240                         
  00057 44 89 de         mov esi, r11d                          
  0005a 4a 8d 7c 12 01   lea rdi, QWORD PTR [1+rdx+r10]         
  0005f 45 0f 44 e1      cmove r12d, r9d                        
  00063 48 f7 de         neg rsi                                
  00066 49 03 f5         add rsi, r13                           
  00069 45 85 e4         test r12d, r12d                        
  0006c 41 0f 44 d9      cmove ebx, r9d                         
  00070 49 0f af fc      imul rdi, r12                          
  00074 48 0f af f3      imul rsi, rbx                          
  00078 48 8b 34 3e      mov rsi, QWORD PTR [rsi+rdi]           
  0007c 49 89 75 00      mov QWORD PTR [r13], rsi               
  00080 41 0f b6 f3      movzx esi, r11b                        
  00084 41 83 e3 08      and r11d, 8                            
  00088 41 d1 eb         shr r11d, 1                            
  0008b 41 83 c3 04      add r11d, 4                            
  0008f 44 0f af db      imul r11d, ebx                         
  00093 8d 7e 02         lea edi, DWORD PTR [2+rsi]             
  00096 ff c6            inc esi                                
  00098 41 0f af fc      imul edi, r12d                         
  0009c 41 0f af f4      imul esi, r12d                         
  000a0 44 03 d7         add r10d, edi                          
  000a3 03 c6            add eax, esi                           
  000a5 41 03 c3         add eax, r11d                          
  000a8 45 8d 14 5a      lea r10d, DWORD PTR [r10+rbx*2]        
  000ac 45 3b d0         cmp r10d, r8d                          
  000af 72 8f            jb .B6.3 
*/

// For enwik8.Nakamichi on my T7500:
// Decompressing 66251713 bytes ...
// RAM-to-RAM performance: 676 MB/s.
// For enwik8.Nakamichi on my Q9550s:
// Decompressing 66251713 bytes ...
// RAM-to-RAM performance: 866 MB/s.
/*
; 'Aratama' decompression loop, a0-40+2=98 bytes long:
; mark_description "Intel(R) C++ Intel(R) 64 Compiler XE for applications running on Intel(R) 64, Version 12.1.1.258 Build 20111";
; mark_description "-O3 -D_N_GP -FAcs";

.B6.3::                         
  00040 45 33 e4         xor r12d, r12d                         

;;; 			*(uint64_t*)(ret+retIndex+8*(0)) = *(uint64_t*)( (uint64_t)(src+srcIndex+1+8*(0))*(unsigned int)(Flag) + (uint64_t)(ret+retIndex-WORDpair)*(unsigned int)(!Flag) );

  00043 41 89 c5         mov r13d, eax                          
  00046 33 db            xor ebx, ebx                           
  00048 4c 03 e9         add r13, rcx                           
  0004b 45 0f b7 1c 12   movzx r11d, WORD PTR [r10+rdx]         
  00050 41 f7 c3 f8 00 
        00 00            test r11d, 248                         
  00057 44 89 de         mov esi, r11d                          
  0005a 4a 8d 7c 12 01   lea rdi, QWORD PTR [1+rdx+r10]         
  0005f 45 0f 44 e1      cmove r12d, r9d                        
  00063 48 f7 de         neg rsi                                
  00066 49 03 f5         add rsi, r13                           
  00069 45 85 e4         test r12d, r12d                        

;;; 			srcIndex+= (((WORDpair & 0xFF)>>0)+1)*(unsigned int)(Flag) + (2)*(unsigned int)(!Flag) ;

  0006c 45 0f b6 db      movzx r11d, r11b                       
  00070 41 0f 44 d9      cmove ebx, r9d                         
  00074 49 0f af fc      imul rdi, r12                          
  00078 48 0f af f3      imul rsi, rbx                          
  0007c 48 8b 34 3e      mov rsi, QWORD PTR [rsi+rdi]           
  00080 41 8d 7b 01      lea edi, DWORD PTR [1+r11]             
  00084 41 0f af fc      imul edi, r12d                         

;;; 			retIndex+= ((WORDpair & 0xFF)>>0)*(unsigned int)(Flag) + (Min_Match_Length)*(unsigned int)(!Flag) ;

  00088 45 0f af dc      imul r11d, r12d                        
  0008c 44 03 d7         add r10d, edi                          
  0008f 41 03 c3         add eax, r11d                          
  00092 49 89 75 00      mov QWORD PTR [r13], rsi               
  00096 45 8d 14 5a      lea r10d, DWORD PTR [r10+rbx*2]        
  0009a 45 3b d0         cmp r10d, r8d                          
  0009d 8d 04 d8         lea eax, DWORD PTR [rax+rbx*8]         
  000a0 72 9e            jb .B6.3 
*/


// Railgun_Swampshine_BailOut, copyleft 2014-Apr-27, Kaze.
// 2014-Apr-27: The nasty SIGNED/UNSIGNED bug in 'Swampshines' which I illustrated several months ago in my fuzzy search article now is fixed here too:
/*
The bug is this (the variables 'i' and 'PRIMALposition' are uint32_t):
Next line assumes -19 >= 0 is true:
if ( (i-(PRIMALposition-1)) >= 0) printf ("THE NASTY BUG AGAIN: %d >= 0\n", i-(PRIMALposition-1));
Next line assumes -19 >= 0 is false:
if ( (signed int)(i-(PRIMALposition-1)) >= 0) printf ("THE NASTY BUG AGAIN: %d >= 0\n", i-(PRIMALposition-1));
And the actual fix:
...
if ( count <= 0 ) {
// I have to add out-of-range checks...
// i-(PRIMALposition-1) >= 0
// &pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4
// i-(PRIMALposition-1)+(count-1) >= 0
// &pbTarget[i-(PRIMALposition-1)+(count-1)] <= pbTargetMax - 4
// FIX from 2014-Apr-27:
// Because (count-1) is negative, above fours are reduced to next twos:
// i-(PRIMALposition-1)+(count-1) >= 0
// &pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4
	// The line below is BUGGY:
	//if ( (i-(PRIMALposition-1) >= 0) && (&pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4) && (&pbTarget[i-(PRIMALposition-1)+(count-1)] <= pbTargetMax - 4) ) {
	// The line below is OKAY:
	if ( ((signed int)(i-(PRIMALposition-1)+(count-1)) >= 0) && (&pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4) ) {
...
*/
// Railgun_Swampshine_BailOut, copyleft 2014-Jan-31, Kaze.
// Caution: For better speed the case 'if (cbPattern==1)' was removed, so Pattern must be longer than 1 char.
#define NeedleThreshold2vs4swampLITE 9+10 // Should be bigger than 9. BMH2 works up to this value (inclusive), if bigger then BMH4 takes over.
char * Railgun_Swampshine_BailOut (char * pbTarget, char * pbPattern, uint32_t cbTarget, uint32_t cbPattern)
{
	char * pbTargetMax = pbTarget + cbTarget;
	register uint32_t ulHashPattern;
	signed long count;

	unsigned char bm_Horspool_Order2[256*256]; // Bitwise soon...
	uint32_t i, Gulliver;

	uint32_t PRIMALposition, PRIMALpositionCANDIDATE;
	uint32_t PRIMALlength, PRIMALlengthCANDIDATE;
	uint32_t j, FoundAtPosition;

	if (cbPattern > cbTarget) return(NULL);

	if ( cbPattern<4 ) { 
		// SSE2 i.e. 128bit Assembly rules here:
		// ...
        	pbTarget = pbTarget+cbPattern;
		ulHashPattern = ( (*(char *)(pbPattern))<<8 ) + *(pbPattern+(cbPattern-1));
		if ( cbPattern==3 ) {
			for ( ;; ) {
				if ( ulHashPattern == ( (*(char *)(pbTarget-3))<<8 ) + *(pbTarget-1) ) {
					if ( *(char *)(pbPattern+1) == *(char *)(pbTarget-2) ) return((pbTarget-3));
				}
				if ( (char)(ulHashPattern>>8) != *(pbTarget-2) ) { 
					pbTarget++;
					if ( (char)(ulHashPattern>>8) != *(pbTarget-2) ) pbTarget++;
				}
				pbTarget++;
				if (pbTarget > pbTargetMax) return(NULL);
			}
		} else {
		}
		for ( ;; ) {
			if ( ulHashPattern == ( (*(char *)(pbTarget-2))<<8 ) + *(pbTarget-1) ) return((pbTarget-2));
			if ( (char)(ulHashPattern>>8) != *(pbTarget-1) ) pbTarget++;
			pbTarget++;
			if (pbTarget > pbTargetMax) return(NULL);
		}
	} else { //if ( cbPattern<4 )
		if ( cbPattern<=NeedleThreshold2vs4swampLITE ) { 
			// BMH order 2, needle should be >=4:
			ulHashPattern = *(uint32_t *)(pbPattern); // First four bytes
			for (i=0; i < 256*256; i++) {bm_Horspool_Order2[i]=0;}
			for (i=0; i < cbPattern-1; i++) bm_Horspool_Order2[*(unsigned short *)(pbPattern+i)]=1;
			i=0;
			while (i <= cbTarget-cbPattern) {
				Gulliver = 1; // 'Gulliver' is the skip
				if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+cbPattern-1-1]] != 0 ) {
					if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+cbPattern-1-1-2]] == 0 ) Gulliver = cbPattern-(2-1)-2; else {
						if ( *(uint32_t *)&pbTarget[i] == ulHashPattern) { // This fast check ensures not missing a match (for remainder) when going under 0 in loop below:
							count = cbPattern-4+1; 
							while ( count > 0 && *(uint32_t *)(pbPattern+count-1) == *(uint32_t *)(&pbTarget[i]+(count-1)) )
								count = count-4;
							if ( count <= 0 ) return(pbTarget+i);
						}
					}
				} else Gulliver = cbPattern-(2-1);
				i = i + Gulliver;
				//GlobalI++; // Comment it, it is only for stats.
			}
			return(NULL);
		} else { // if ( cbPattern<=NeedleThreshold2vs4swampLITE )

// Swampwalker_BAILOUT heuristic order 4 (Needle should be bigger than 4) [
// Needle: 1234567890qwertyuiopasdfghjklzxcv            PRIMALposition=01 PRIMALlength=33  '1234567890qwertyuiopasdfghjklzxcv'
// Needle: vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv            PRIMALposition=29 PRIMALlength=04  'vvvv'
// Needle: vvvvvvvvvvBOOMSHAKALAKAvvvvvvvvvv            PRIMALposition=08 PRIMALlength=20  'vvvBOOMSHAKALAKAvvvv'
// Needle: Trollland                                    PRIMALposition=01 PRIMALlength=09  'Trollland'
// Needle: Swampwalker                                  PRIMALposition=01 PRIMALlength=11  'Swampwalker'
// Needle: licenselessness                              PRIMALposition=01 PRIMALlength=15  'licenselessness'
// Needle: alfalfa                                      PRIMALposition=02 PRIMALlength=06  'lfalfa'
// Needle: Sandokan                                     PRIMALposition=01 PRIMALlength=08  'Sandokan'
// Needle: shazamish                                    PRIMALposition=01 PRIMALlength=09  'shazamish'
// Needle: Simplicius Simplicissimus                    PRIMALposition=06 PRIMALlength=20  'icius Simplicissimus'
// Needle: domilliaquadringenquattuorquinquagintillion  PRIMALposition=01 PRIMALlength=32  'domilliaquadringenquattuorquinqu'
// Needle: boom-boom                                    PRIMALposition=02 PRIMALlength=08  'oom-boom'
// Needle: vvvvv                                        PRIMALposition=01 PRIMALlength=04  'vvvv'
// Needle: 12345                                        PRIMALposition=01 PRIMALlength=05  '12345'
// Needle: likey-likey                                  PRIMALposition=03 PRIMALlength=09  'key-likey'
// Needle: BOOOOOM                                      PRIMALposition=03 PRIMALlength=05  'OOOOM'
// Needle: aaaaaBOOOOOM                                 PRIMALposition=02 PRIMALlength=09  'aaaaBOOOO'
// Needle: BOOOOOMaaaaa                                 PRIMALposition=03 PRIMALlength=09  'OOOOMaaaa'
PRIMALlength=0;
for (i=0+(1); i < cbPattern-((4)-1)+(1)-(1); i++) { // -(1) because the last BB order 4 has no counterpart(s)
	FoundAtPosition = cbPattern - ((4)-1) + 1;
	PRIMALpositionCANDIDATE=i;
	while ( PRIMALpositionCANDIDATE <= (FoundAtPosition-1) ) {
		j = PRIMALpositionCANDIDATE + 1;
		while ( j <= (FoundAtPosition-1) ) {
			if ( *(uint32_t *)(pbPattern+PRIMALpositionCANDIDATE-(1)) == *(uint32_t *)(pbPattern+j-(1)) ) FoundAtPosition = j;
			j++;
		}
		PRIMALpositionCANDIDATE++;
	}
	PRIMALlengthCANDIDATE = (FoundAtPosition-1)-i+1 +((4)-1);
	if (PRIMALlengthCANDIDATE >= PRIMALlength) {PRIMALposition=i; PRIMALlength = PRIMALlengthCANDIDATE;}
	if (cbPattern-i+1 <= PRIMALlength) break;
	if (PRIMALlength > 128) break; // Bail Out for 129[+]
}
// Swampwalker_BAILOUT heuristic order 4 (Needle should be bigger than 4) ]

// Here we have 4 or bigger NewNeedle, apply order 2 for pbPattern[i+(PRIMALposition-1)] with length 'PRIMALlength' and compare the pbPattern[i] with length 'cbPattern':
PRIMALlengthCANDIDATE = cbPattern;
cbPattern = PRIMALlength;
pbPattern = pbPattern + (PRIMALposition-1);

// Revision 2 commented section [
/*
if (cbPattern-1 <= 255) {
// BMH Order 2 [
			ulHashPattern = *(uint32_t *)(pbPattern); // First four bytes
			for (i=0; i < 256*256; i++) {bm_Horspool_Order2[i]= cbPattern-1;} // cbPattern-(Order-1) for Horspool; 'memset' if not optimized
			for (i=0; i < cbPattern-1; i++) bm_Horspool_Order2[*(unsigned short *)(pbPattern+i)]=i; // Rightmost appearance/position is needed
			i=0;
			while (i <= cbTarget-cbPattern) { 
				Gulliver = bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+cbPattern-1-1]];
				if ( Gulliver != cbPattern-1 ) { // CASE #2: if equal means the pair (char order 2) is not found i.e. Gulliver remains intact, skip the whole pattern and fall back (Order-1) chars i.e. one char for Order 2
				if ( Gulliver == cbPattern-2 ) { // CASE #1: means the pair (char order 2) is found
					if ( *(uint32_t *)&pbTarget[i] == ulHashPattern) {
						count = cbPattern-4+1; 
						while ( count > 0 && *(uint32_t *)(pbPattern+count-1) == *(uint32_t *)(&pbTarget[i]+(count-1)) )
							count = count-4;
// If we miss to hit then no need to compare the original: Needle
if ( count <= 0 ) {
// I have to add out-of-range checks...
// i-(PRIMALposition-1) >= 0
// &pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4
// i-(PRIMALposition-1)+(count-1) >= 0
// &pbTarget[i-(PRIMALposition-1)+(count-1)] <= pbTargetMax - 4

// FIX from 2014-Apr-27:
// Because (count-1) is negative, above fours are reduced to next twos:
// i-(PRIMALposition-1)+(count-1) >= 0
// &pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4
	// The line below is BUGGY:
	//if ( (i-(PRIMALposition-1) >= 0) && (&pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4) && (&pbTarget[i-(PRIMALposition-1)+(count-1)] <= pbTargetMax - 4) ) {
	// The line below is OKAY:
	if ( ((signed int)(i-(PRIMALposition-1)+(count-1)) >= 0) && (&pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4) ) {

		if ( *(uint32_t *)&pbTarget[i-(PRIMALposition-1)] == *(uint32_t *)(pbPattern-(PRIMALposition-1))) { // This fast check ensures not missing a match (for remainder) when going under 0 in loop below:
			count = PRIMALlengthCANDIDATE-4+1; 
			while ( count > 0 && *(uint32_t *)(pbPattern-(PRIMALposition-1)+count-1) == *(uint32_t *)(&pbTarget[i-(PRIMALposition-1)]+(count-1)) )
				count = count-4;
			if ( count <= 0 ) return(pbTarget+i-(PRIMALposition-1));	
		}
	}
}
					}
					Gulliver = 1;
				} else
					Gulliver = cbPattern - Gulliver - 2; // CASE #3: the pair is found and not as suffix i.e. rightmost position
				}
				i = i + Gulliver;
				//GlobalI++; // Comment it, it is only for stats.
			}
			return(NULL);
// BMH Order 2 ]
} else {
			// BMH order 2, needle should be >=4:
			ulHashPattern = *(uint32_t *)(pbPattern); // First four bytes
			for (i=0; i < 256*256; i++) {bm_Horspool_Order2[i]=0;}
			for (i=0; i < cbPattern-1; i++) bm_Horspool_Order2[*(unsigned short *)(pbPattern+i)]=1;
			i=0;
			while (i <= cbTarget-cbPattern) {
				Gulliver = 1; // 'Gulliver' is the skip
				if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+cbPattern-1-1]] != 0 ) {
					if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+cbPattern-1-1-2]] == 0 ) Gulliver = cbPattern-(2-1)-2; else {
						if ( *(uint32_t *)&pbTarget[i] == ulHashPattern) { // This fast check ensures not missing a match (for remainder) when going under 0 in loop below:
							count = cbPattern-4+1; 
							while ( count > 0 && *(uint32_t *)(pbPattern+count-1) == *(uint32_t *)(&pbTarget[i]+(count-1)) )
								count = count-4;
// If we miss to hit then no need to compare the original: Needle
if ( count <= 0 ) {
// I have to add out-of-range checks...
// i-(PRIMALposition-1) >= 0
// &pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4
// i-(PRIMALposition-1)+(count-1) >= 0
// &pbTarget[i-(PRIMALposition-1)+(count-1)] <= pbTargetMax - 4

// FIX from 2014-Apr-27:
// Because (count-1) is negative, above fours are reduced to next twos:
// i-(PRIMALposition-1)+(count-1) >= 0
// &pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4
	// The line below is BUGGY:
	//if ( (i-(PRIMALposition-1) >= 0) && (&pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4) && (&pbTarget[i-(PRIMALposition-1)+(count-1)] <= pbTargetMax - 4) ) {
	// The line below is OKAY:
	if ( ((signed int)(i-(PRIMALposition-1)+(count-1)) >= 0) && (&pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4) ) {

		if ( *(uint32_t *)&pbTarget[i-(PRIMALposition-1)] == *(uint32_t *)(pbPattern-(PRIMALposition-1))) { // This fast check ensures not missing a match (for remainder) when going under 0 in loop below:
			count = PRIMALlengthCANDIDATE-4+1; 
			while ( count > 0 && *(uint32_t *)(pbPattern-(PRIMALposition-1)+count-1) == *(uint32_t *)(&pbTarget[i-(PRIMALposition-1)]+(count-1)) )
				count = count-4;
			if ( count <= 0 ) return(pbTarget+i-(PRIMALposition-1));	
		}
	}
}
						}
					}
				} else Gulliver = cbPattern-(2-1);
				i = i + Gulliver;
				//GlobalI++; // Comment it, it is only for stats.
			}
			return(NULL);
}
*/
// Revision 2 commented section ]

		if ( cbPattern<=NeedleThreshold2vs4swampLITE ) { 

			// BMH order 2, needle should be >=4:
			ulHashPattern = *(uint32_t *)(pbPattern); // First four bytes
			for (i=0; i < 256*256; i++) {bm_Horspool_Order2[i]=0;}
			for (i=0; i < cbPattern-1; i++) bm_Horspool_Order2[*(unsigned short *)(pbPattern+i)]=1;
			i=0;
			while (i <= cbTarget-cbPattern) {
				Gulliver = 1; // 'Gulliver' is the skip
				if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+cbPattern-1-1]] != 0 ) {
					if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+cbPattern-1-1-2]] == 0 ) Gulliver = cbPattern-(2-1)-2; else {
						if ( *(uint32_t *)&pbTarget[i] == ulHashPattern) { // This fast check ensures not missing a match (for remainder) when going under 0 in loop below:
							count = cbPattern-4+1; 
							while ( count > 0 && *(uint32_t *)(pbPattern+count-1) == *(uint32_t *)(&pbTarget[i]+(count-1)) )
								count = count-4;
// If we miss to hit then no need to compare the original: Needle
if ( count <= 0 ) {
// I have to add out-of-range checks...
// i-(PRIMALposition-1) >= 0
// &pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4
// i-(PRIMALposition-1)+(count-1) >= 0
// &pbTarget[i-(PRIMALposition-1)+(count-1)] <= pbTargetMax - 4

// FIX from 2014-Apr-27:
// Because (count-1) is negative, above fours are reduced to next twos:
// i-(PRIMALposition-1)+(count-1) >= 0
// &pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4
	// The line below is BUGGY:
	//if ( (i-(PRIMALposition-1) >= 0) && (&pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4) && (&pbTarget[i-(PRIMALposition-1)+(count-1)] <= pbTargetMax - 4) ) {
	// The line below is OKAY:
	if ( ((signed int)(i-(PRIMALposition-1)+(count-1)) >= 0) && (&pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4) ) {

		if ( *(uint32_t *)&pbTarget[i-(PRIMALposition-1)] == *(uint32_t *)(pbPattern-(PRIMALposition-1))) { // This fast check ensures not missing a match (for remainder) when going under 0 in loop below:
			count = PRIMALlengthCANDIDATE-4+1; 
			while ( count > 0 && *(uint32_t *)(pbPattern-(PRIMALposition-1)+count-1) == *(uint32_t *)(&pbTarget[i-(PRIMALposition-1)]+(count-1)) )
				count = count-4;
			if ( count <= 0 ) return(pbTarget+i-(PRIMALposition-1));	
		}
	}
}
						}
					}
				} else Gulliver = cbPattern-(2-1);
				i = i + Gulliver;
				//GlobalI++; // Comment it, it is only for stats.
			}
			return(NULL);

		} else { // if ( cbPattern<=NeedleThreshold2vs4swampLITE )

			// BMH pseudo-order 4, needle should be >=8+2:
			ulHashPattern = *(uint32_t *)(pbPattern); // First four bytes
			for (i=0; i < 256*256; i++) {bm_Horspool_Order2[i]=0;}
			// In line below we "hash" 4bytes to 2bytes i.e. 16bit table, how to compute TOTAL number of BBs, 'cbPattern - Order + 1' is the number of BBs for text 'cbPattern' bytes long, for example, for cbPattern=11 'fastest fox' and Order=4 we have BBs = 11-4+1=8:
			//"fast"
			//"aste"
			//"stes"
			//"test"
			//"est "
			//"st f"
			//"t fo"
			//" fox"
			//for (i=0; i < cbPattern-4+1; i++) bm_Horspool_Order2[( *(unsigned short *)(pbPattern+i+0) + *(unsigned short *)(pbPattern+i+2) ) & ( (1<<16)-1 )]=1;
			//for (i=0; i < cbPattern-4+1; i++) bm_Horspool_Order2[( (*(uint32_t *)(pbPattern+i+0)>>16)+(*(uint32_t *)(pbPattern+i+0)&0xFFFF) ) & ( (1<<16)-1 )]=1;
			// Above line is replaced by next one with better hashing:
			for (i=0; i < cbPattern-4+1; i++) bm_Horspool_Order2[( (*(uint32_t *)(pbPattern+i+0)>>(16-1))+(*(uint32_t *)(pbPattern+i+0)&0xFFFF) ) & ( (1<<16)-1 )]=1;
			i=0;
			while (i <= cbTarget-cbPattern) {
				Gulliver = 1;
				//if ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2]>>16)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2]&0xFFFF) ) & ( (1<<16)-1 )] != 0 ) { // DWORD #1
				// Above line is replaced by next one with better hashing:
				if ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2]>>(16-1))+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2]&0xFFFF) ) & ( (1<<16)-1 )] != 0 ) { // DWORD #1
					//if ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]>>16)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]&0xFFFF) ) & ( (1<<16)-1 )] == 0 ) Gulliver = cbPattern-(2-1)-2-4; else {
					// Above line is replaced in order to strengthen the skip by checking the middle DWORD,if the two DWORDs are 'ab' and 'cd' i.e. [2x][2a][2b][2c][2d] then the middle DWORD is 'bc'.
					// The respective offsets (backwards) are: -10/-8/-6/-4 for 'xa'/'ab'/'bc'/'cd'.
					//if ( ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-6]>>16)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-6]&0xFFFF) ) & ( (1<<16)-1 )] ) + ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]>>16)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]&0xFFFF) ) & ( (1<<16)-1 )] ) + ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-2]>>16)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-2]&0xFFFF) ) & ( (1<<16)-1 )] ) < 3 ) Gulliver = cbPattern-(2-1)-2-4-2; else {
					// Above line is replaced by next one with better hashing:
					// When using (16-1) right shifting instead of 16 we will have two different pairs (if they are equal), the highest bit being lost do the job especialy for ASCII texts with no symbols in range 128-255.
					// Example for genomesque pair TT+TT being shifted by (16-1):
					// T            = 01010100
					// TT           = 01010100 01010100
					// TTTT         = 01010100 01010100 01010100 01010100
					// TTTT>>16     = 00000000 00000000 01010100 01010100
					// TTTT>>(16-1) = 00000000 00000000 10101000 10101000 <--- Due to the left shift by 1, the 8th bits of 1st and 2nd bytes are populated - usually they are 0 for English texts & 'ACGT' data.
					//if ( ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-6]>>(16-1))+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-6]&0xFFFF) ) & ( (1<<16)-1 )] ) + ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]>>(16-1))+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]&0xFFFF) ) & ( (1<<16)-1 )] ) + ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-2]>>(16-1))+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-2]&0xFFFF) ) & ( (1<<16)-1 )] ) < 3 ) Gulliver = cbPattern-(2-1)-2-4-2; else {
					// 'Maximus' uses branched 'if', again.
					if ( \
					( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-6 +1]>>(16-1))+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-6 +1]&0xFFFF) ) & ( (1<<16)-1 )] ) == 0 \
					|| ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4 +1]>>(16-1))+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4 +1]&0xFFFF) ) & ( (1<<16)-1 )] ) == 0 \
					) Gulliver = cbPattern-(2-1)-2-4-2 +1; else {
					// Above line is not optimized (several a SHR are used), we have 5 non-overlapping WORDs, or 3 overlapping WORDs, within 4 overlapping DWORDs so:
// [2x][2a][2b][2c][2d]
// DWORD #4
// [2a] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-6]>>16) =     !SHR to be avoided! <--
// [2x] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-6]&0xFFFF) =                        |
//     DWORD #3                                                                       |
// [2b] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]>>16) =     !SHR to be avoided!   |<--
// [2a] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]&0xFFFF) = ------------------------  |
//         DWORD #2                                                                      |
// [2c] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-2]>>16) =     !SHR to be avoided!      |<--
// [2b] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-2]&0xFFFF) = ---------------------------  |
//             DWORD #1                                                                     |
// [2d] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-0]>>16) =                                 |
// [2c] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-0]&0xFFFF) = ------------------------------
//
// So in order to remove 3 SHR instructions the equal extractions are:
// DWORD #4
// [2a] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]&0xFFFF) =  !SHR to be avoided! <--
// [2x] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-6]&0xFFFF) =                        |
//     DWORD #3                                                                       |
// [2b] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-2]&0xFFFF) =  !SHR to be avoided!   |<--
// [2a] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]&0xFFFF) = ------------------------  |
//         DWORD #2                                                                      |
// [2c] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-0]&0xFFFF) =  !SHR to be avoided!      |<--
// [2b] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-2]&0xFFFF) = ---------------------------  |
//             DWORD #1                                                                     |
// [2d] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-0]>>16) =                                 |
// [2c] (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-0]&0xFFFF) = ------------------------------
					//if ( ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]&0xFFFF)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-6]&0xFFFF) ) & ( (1<<16)-1 )] ) + ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-2]&0xFFFF)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]&0xFFFF) ) & ( (1<<16)-1 )] ) + ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-0]&0xFFFF)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-2]&0xFFFF) ) & ( (1<<16)-1 )] ) < 3 ) Gulliver = cbPattern-(2-1)-2-6; else {
// Since the above Decumanus mumbo-jumbo (3 overlapping lookups vs 2 non-overlapping lookups) is not fast enough we go DuoDecumanus or 3x4:
// [2y][2x][2a][2b][2c][2d]
// DWORD #3
//         DWORD #2
//                 DWORD #1
					//if ( ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]>>16)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-4]&0xFFFF) ) & ( (1<<16)-1 )] ) + ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-8]>>16)+(*(uint32_t *)&pbTarget[i+cbPattern-1-1-2-8]&0xFFFF) ) & ( (1<<16)-1 )] ) < 2 ) Gulliver = cbPattern-(2-1)-2-8; else {
						if ( *(uint32_t *)&pbTarget[i] == ulHashPattern) {
							// Order 4 [
						// Let's try something "outrageous" like comparing with[out] overlap BBs 4bytes long instead of 1 byte back-to-back:
						// Inhere we are using order 4, 'cbPattern - Order + 1' is the number of BBs for text 'cbPattern' bytes long, for example, for cbPattern=11 'fastest fox' and Order=4 we have BBs = 11-4+1=8:
						//0:"fast" if the comparison failed here, 'count' is 1; 'Gulliver' is cbPattern-(4-1)-7
						//1:"aste" if the comparison failed here, 'count' is 2; 'Gulliver' is cbPattern-(4-1)-6
						//2:"stes" if the comparison failed here, 'count' is 3; 'Gulliver' is cbPattern-(4-1)-5
						//3:"test" if the comparison failed here, 'count' is 4; 'Gulliver' is cbPattern-(4-1)-4
						//4:"est " if the comparison failed here, 'count' is 5; 'Gulliver' is cbPattern-(4-1)-3
						//5:"st f" if the comparison failed here, 'count' is 6; 'Gulliver' is cbPattern-(4-1)-2
						//6:"t fo" if the comparison failed here, 'count' is 7; 'Gulliver' is cbPattern-(4-1)-1
						//7:" fox" if the comparison failed here, 'count' is 8; 'Gulliver' is cbPattern-(4-1)
							count = cbPattern-4+1; 
							// Below comparison is UNIdirectional:
							while ( count > 0 && *(uint32_t *)(pbPattern+count-1) == *(uint32_t *)(&pbTarget[i]+(count-1)) )
								count = count-4;
// count = cbPattern-4+1 = 23-4+1 = 20
// boomshakalakaZZZZZZ[ZZZZ] 20
// boomshakalakaZZ[ZZZZ]ZZZZ 20-4
// boomshakala[kaZZ]ZZZZZZZZ 20-8 = 12
// boomsha[kala]kaZZZZZZZZZZ 20-12 = 8
// boo[msha]kalakaZZZZZZZZZZ 20-16 = 4

// If we miss to hit then no need to compare the original: Needle
if ( count <= 0 ) {
// I have to add out-of-range checks...
// i-(PRIMALposition-1) >= 0
// &pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4
// i-(PRIMALposition-1)+(count-1) >= 0
// &pbTarget[i-(PRIMALposition-1)+(count-1)] <= pbTargetMax - 4

// FIX from 2014-Apr-27:
// Because (count-1) is negative, above fours are reduced to next twos:
// i-(PRIMALposition-1)+(count-1) >= 0
// &pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4
	// The line below is BUGGY:
	//if ( (i-(PRIMALposition-1) >= 0) && (&pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4) && (&pbTarget[i-(PRIMALposition-1)+(count-1)] <= pbTargetMax - 4) ) {
	// The line below is OKAY:
	if ( ((signed int)(i-(PRIMALposition-1)+(count-1)) >= 0) && (&pbTarget[i-(PRIMALposition-1)] <= pbTargetMax - 4) ) {

		if ( *(uint32_t *)&pbTarget[i-(PRIMALposition-1)] == *(uint32_t *)(pbPattern-(PRIMALposition-1))) { // This fast check ensures not missing a match (for remainder) when going under 0 in loop below:
			count = PRIMALlengthCANDIDATE-4+1; 
			while ( count > 0 && *(uint32_t *)(pbPattern-(PRIMALposition-1)+count-1) == *(uint32_t *)(&pbTarget[i-(PRIMALposition-1)]+(count-1)) )
				count = count-4;
			if ( count <= 0 ) return(pbTarget+i-(PRIMALposition-1));	
		}
	}
}

							// In order to avoid only-left or only-right WCS the memcmp should be done as left-to-right and right-to-left AT THE SAME TIME.
							// Below comparison is BIdirectional. It pays off when needle is 8+++ long:
//							for (count = cbPattern-4+1; count > 0; count = count-4) {
//								if ( *(uint32_t *)(pbPattern+count-1) != *(uint32_t *)(&pbTarget[i]+(count-1)) ) {break;};
//								if ( *(uint32_t *)(pbPattern+(cbPattern-4+1)-count) != *(uint32_t *)(&pbTarget[i]+(cbPattern-4+1)-count) ) {count = (cbPattern-4+1)-count +(1); break;} // +(1) because two lookups are implemented as one, also no danger of 'count' being 0 because of the fast check outwith the 'while': if ( *(uint32_t *)&pbTarget[i] == ulHashPattern)
//							}
//							if ( count <= 0 ) return(pbTarget+i);
								// Checking the order 2 pairs in mismatched DWORD, all the 3:
								//if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+count-1]] == 0 ) Gulliver = count; // 1 or bigger, as it should
								//if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+count-1+1]] == 0 ) Gulliver = count+1; // 1 or bigger, as it should
								//if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+count-1+1+1]] == 0 ) Gulliver = count+1+1; // 1 or bigger, as it should
							//	if ( bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+count-1]] + bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+count-1+1]] + bm_Horspool_Order2[*(unsigned short *)&pbTarget[i+count-1+1+1]] < 3 ) Gulliver = count; // 1 or bigger, as it should, THE MIN(count,count+1,count+1+1)
								// Above compound 'if' guarantees not that Gulliver > 1, an example:
								// Needle:    fastest tax
								// Window: ...fastast tax...
								// After matching ' tax' vs ' tax' and 'fast' vs 'fast' the mismathced DWORD is 'test' vs 'tast':
								// 'tast' when factorized down to order 2 yields: 'ta','as','st' - all the three when summed give 1+1+1=3 i.e. Gulliver remains 1.
								// Roughly speaking, this attempt maybe has its place in worst-case scenarios but not in English text and even not in ACGT data, that's why I commented it in original 'Shockeroo'.
								//if ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+count-1]>>16)+(*(uint32_t *)&pbTarget[i+count-1]&0xFFFF) ) & ( (1<<16)-1 )] == 0 ) Gulliver = count; // 1 or bigger, as it should
								// Above line is replaced by next one with better hashing:
//								if ( bm_Horspool_Order2[( (*(uint32_t *)&pbTarget[i+count-1]>>(16-1))+(*(uint32_t *)&pbTarget[i+count-1]&0xFFFF) ) & ( (1<<16)-1 )] == 0 ) Gulliver = count; // 1 or bigger, as it should
							// Order 4 ]
						}
					}
				} else Gulliver = cbPattern-(2-1)-2; // -2 because we check the 4 rightmost bytes not 2.
				i = i + Gulliver;
				//GlobalI++; // Comment it, it is only for stats.
			}
			return(NULL);

		} // if ( cbPattern<=NeedleThreshold2vs4swampLITE )
		} // if ( cbPattern<=NeedleThreshold2vs4swampLITE )
	} //if ( cbPattern<4 )
}

// Fixed version from 2012-Feb-27.
// Caution: For better speed the case 'if (cbPattern==1)' was removed, so Pattern must be longer than 1 char.
char * Railgun_Doublet (char * pbTarget, char * pbPattern, uint32_t cbTarget, uint32_t cbPattern)
{
	char * pbTargetMax = pbTarget + cbTarget;
	register uint32_t ulHashPattern;
	uint32_t ulHashTarget, count, countSTATIC;

	if (cbPattern > cbTarget) return(NULL);

	countSTATIC = cbPattern-2;

	pbTarget = pbTarget+cbPattern;
	ulHashPattern = (*(uint16_t *)(pbPattern));

	for ( ;; ) {
		if ( ulHashPattern == (*(uint16_t *)(pbTarget-cbPattern)) ) {
			count = countSTATIC;
			while ( count && *(char *)(pbPattern+2+(countSTATIC-count)) == *(char *)(pbTarget-cbPattern+2+(countSTATIC-count)) ) {
				count--;
			}
			if ( count == 0 ) return((pbTarget-cbPattern));
		}
		pbTarget++;
		if (pbTarget > pbTargetMax) return(NULL);
	}
}

// Last change: 2014-Apr-29
// If you want to help me to improve it, email me at: sanmayce@sanmayce.com
// Enfun!
