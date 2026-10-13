# MIDI to FF14 アダプター（演奏10号）
PS4/PS5/Switch 2 のファイナルファンタジー 14 で、MIDIキーボードを使って楽器演奏を可能にするアダプタです。  
名前は**演奏10号**です。  
MIDIのノートオン/ノートオフ信号を、HIDキーボードのキーダウン/キーアップ信号に変換しています。
以前公開した[演奏9号](https://github.com/Lybfip/Arduino-MIDI-to-FF14-adapter)の改良型です。対応するMIDIキーボードが増えました。
詳しい説明は[こちらのページ](https://tech-miqo.com/ff14-%e6%a5%bd%e5%99%a8%e6%bc%94%e5%a5%8f%e7%94%a8-midi-%e3%82%ad%e3%83%bc%e3%83%9c%e3%83%bc%e3%83%89%e3%82%a2%e3%83%80%e3%83%97%e3%82%bf%e3%81%ae%e4%bd%9c%e3%82%8a%e6%96%b9/)を見てください。

## 用意するもの
- [Picossi USBホスト（RP2040搭載）](https://www.switch-science.com/products/9158?_pos=1&_sid=e86a6cdc5&_ss=r) 1個
- PC に USB Type-C のデバイスを接続できる USB 通信ケーブル

## 作り方
1. [build/conductor_10.uf2](https://github.com/Lybfip/Conductor_10/blob/main/build/conductor_10.uf2) ファイルをダウンロードしてください。
2. Raspberry Pi PicoボードのBOOTSELボタンを押しながら、USBケーブルを通してPCに接続してください。USBメモリーのようにPicoのドライブが現れます。
3. ダウンロードしたconductor_10.uf2ファイルを、Picoのドライブのルートにドラッグアンドドロップしてください。
## 演奏の設定
USB Type-C コネクタ側を PS4 / PS5 / Switch 2 に接続し、USB AコネクタをMIDIキーボードに接続してください。
FINAL FANTASY XIVの楽器演奏の設定で、MIDIキーボードを押して音階を登録してください。
## 補足
Picossi USBホストが入手できない場合、Raspberry Pi Pico でも動作します。
USB A コネクタを購入し、
USB_VBUS ------- Pico_VBUS  
USB_D- -- 22Ω --- Pico_GP1  
USB_D- -- 15kΩ -- Pico_GND  
USB_D+ -- 22Ω ----Pico_GP0  
USB_D+ -- 15kΩ -- Pico_GND  
USB_GND ---------- Pico_GND  
と結線すれば、Picossi USBホストと同様に使えます。

© SQUARE ENIX