# CR52 Benchmark

Cortex-R52 (Renesas R-Car **V4H** / **X5H**) 向けのベアメタル・ベンチマーク集です。
**CoreMark** と **Dhrystone** を、同一のツールチェーン・コンパイルフラグ・
ボードサポート (BSP) でビルドし、PMU（性能モニタユニット）でサイクル数・命令数・
キャッシュリフィル・分岐予測ミスを計測します。

両ベンチマークは CPU 起動・キャッシュ/TCM/MPU 設定・SCMT タイマ・EL2→EL1 遷移・
PMU 計測といった共通部分を `bsp/cr52/` の BSP として共有しています。

## ディレクトリ構成

```
cr52-benchmark/
├─ Makefile.common          # 共通: ツールチェーン / CFLAGS / LSI / BSP の場所
├─ bsp/cr52/                # CR52 ベアメタル共有 BSP
│   ├─ bsp.h / ee_types.h   #   共通型・プロトタイプ・SCMT レジスタ定義
│   ├─ ee_printf.c / cvt.c  #   コンソール出力（%f はソフトウェア実装、libm 不要）
│   ├─ pmu.c                #   PMU ラッパ（enable/disable/report）
│   ├─ cr52_hw.c            #   SCMT タイマ・キャッシュ/TCM/MPU・drop_to_el1
│   ├─ boot_mon.s / boot_mon.h
│   ├─ memory_cr52_boot.def # V4H 用リンカスクリプト（メモリマップ）
│   └─ memory_x5h_jet2.def  # X5H 用リンカスクリプト
├─ coremark/                # CoreMark 本体 + barebones/ ポートグルー
└─ dhrystone/               # Dhrystone 本体 + barebones/ ポートグルー
```

## 必要なもの

- Arm GNU Toolchain（ベアメタル `arm-none-eabi`、動作確認: **13.2.rel1**）
  - `arm-none-eabi-gcc` / `-ld` / `-as` / `-objcopy` に PATH を通しておくこと
- GNU Make

## ビルド

ターゲット SoC は `LSI` 変数で選択します（既定 `V4H`、`X5H` も可）。
両ベンチマークとも `.elf` / `.bin` / `.mot`(SREC) / `.map` を生成します。

### CoreMark

```sh
cd coremark
make coremark.elf          # coremark.elf / .bin / .mot / .map を生成（既定: PERFORMANCE_RUN）
make coremark.elf LSI=X5H  # X5H 向け
make clean                 # 生成物を削除
```

### Dhrystone

```sh
cd dhrystone
make            # dhrystone.elf / .bin / .mot / .map を生成
make LSI=X5H    # X5H 向け
make clean      # 生成物を削除
```

> いずれもイメージは `0x40040000` にリンクされます（実行時のロード先と一致）。

## 実機での実行

ビルドした `*.bin` を TFTP サーバに配置し（例では `v4h/` 配下）、U-Boot 上から
リモートプロセッサ (rproc) として起動します。

```text
# CoreMark
rproc init; tftp 40040000 v4h/coremark.bin; rproc load 0 0x40040000 0x200000; rproc start 0

# Dhrystone
rproc init; tftp 40040000 v4h/dhrystone.bin; rproc load 0 0x40040000 0x200000; rproc start 0
```

各コマンドの意味:

| コマンド | 内容 |
|---|---|
| `rproc init` | リモートプロセッサ (Cortex-R52) を初期化 |
| `tftp 40040000 v4h/<name>.bin` | バイナリを `0x40040000` へ TFTP 転送 |
| `rproc load 0 0x40040000 0x200000` | `0x40040000` から 2MB をコア0へロード |
| `rproc start 0` | コア0を起動（`writer_main` → ベンチマーク実行） |

実行するとシリアルコンソールにベンチマーク結果と、計測区間の PMU 値が出力されます。

```
---- PMU results ----
  Cycle counter        : <CPU サイクル数>
  Instructions retired (evt 0x08): <実行命令数>
  L1 I-cache refill    (evt 0x01): <I$ リフィル回数>
  L1 D-cache refill    (evt 0x03): <D$ リフィル回数>
  Branch mispredicted  (evt 0x10): <分岐予測ミス回数>
---------------------
```

> PMU 計測区間は各ベンチマークの「測定対象ループ」と一致させています。CoreMark は
> `core_main.c` の計測区間内、Dhrystone は `dhry_portme.c` の `writer_main` で囲っています。

## 設計メモ

- **共有 BSP**: 両ベンチで完全に同一だった起動・PMU・I/O コードは `bsp/cr52/` に集約。
  各ベンチの `barebones/` には固有のポートグルー（タイミング API と `writer_main` の段取り、
  Dhrystone 用の最小 libc シム）だけが残っています。
- **同一フラグ**: コード生成品質を比較できるよう、両者とも
  `-O3 -g -mcpu=cortex-r52 -mfpu=neon-fp-armv8 -static -fno-builtin` でビルドします
  （`Makefile.common` に集約）。
- **libc/libm 非依存**: `ee_printf` の `%f`/`%e`/`%g` は `cvt.c` のソフトウェア実装で
  処理し、`libm` を必要としません。

## ライセンス

- CoreMark: Apache License 2.0（`coremark/LICENSE.md`、EEMBC）
- Dhrystone: パブリックドメイン（Reinhold P. Weicker 版）
