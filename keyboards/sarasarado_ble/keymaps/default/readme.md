# The default keymap for sarasarado_ble

## レイヤ数
0-15が扱える。もともとLTマクロ(QK_LAYER_TAPキーコード)ではレイヤに4bitしか割り当てられてない。16以降のレイヤは指定できない。

## addtional key list
|symbol|description|
|------|-----------|
|xEISU|EISU(英数)|
|xKANA|KANA(かな)|
|SEL_BLE|select BLE connection|
|SEL_USB|select USB connection|
|TOG_HID|toggle HID connection(BLE / USB)|
|m00EISU|tap EISU(英数) / press layer #0|
|m00KANA|tap KANA(かな) / press layer #0|
| ...  | ... | ... |
|m15EISU|tap EISU(英数) / press layer #15|
|m15KANA|tap KANA(かな) / press layer #15|

## define update_tri_layer function
update_tri_layerを実行するときのレイヤーをTAPTERM.JSNで指定することができる。
ANY(24320)とANY(24321)で設定した値のレイヤの両方を押したときに、ANY(24322)で設定された値のレイヤがオンになる。

ANY(24320),ANY(24321),ANY(24321)が定義されていない場合はupdate_tri_layerの処理は行われない。

24320(0x5F00)で現在使用されていないキーコード0x5C00-0x5FFFの範囲にある。

例

```
{
    "tapping_term":{
	"ANY(24320)":2,
	"ANY(24321)":3,
	"ANY(24322)":4,
	"KC_NO":200
    }
}
```
レイヤ2とレイヤ3がオンになるとレイヤ4がオンになる(元の設定)

## 従来ファームウェアとの変更点
!!注意!!

ファームウェアをアップデートする前には設定ファイルのバックアップを取ること。
バックアップを以下の修正点に合わせて修正し、ファームウェアのアップデート後に書き直すこと。

### m0SPCとm1ENT / ML0～ML3
廃止された。LT() / MO() マクロを使用すること。
本ファームウェアではLT()/MO()マクロを使用してもupdate_tri_layerの機能は動作する。
他のレイヤとキーコードをTAP/LAYER機能で使用することができる。

このファームウェアで独自に定義したキーコードや、BMPで拡張されたキーコードはLT()マクロには使用できない。
ただし、xKANA/xEISUに関しては、m_nn_KANA/m_nn_EISUを使用することによって使用できる。

### m0KANA/m0EISU～m3KANA/m3EISU
廃止された。
xKANA/xEISUのコンビネーションは、m00KANA/m00EISU～m15KANA/m15EISUに置き換えられた。
m_nn_の_nn_はオフセットはないため、m00KANAはKANAとレイヤ0のコンボになる。

|old|new|
|---|---|
|m0|m02|
|m1|m03|
|m2|m04|
|m3|m05|
