# The default keymap for sarasarado_ble

## layer list
|number|symbol|description|
|---|---|---|
|0|\_BL0|base layer|
|1|\_BL1|altanatibe layer|
|2|\_ML0|momentary layer 0 (eg. RAISE)|
|3|\_ML1|momentary layer 1 (eg. LOWER)|
|4|\_ML2|momentary layer 2 (eg. ADJUST)|
|5|\_ML3|momentary layer 3|

## addtional key list
|symbol|description|
|------|-----------|
|xEISU|EISU(英数)|
|xKANA|KANA(かな)|
|SEL_BLE|select BLE connection|
|SEL_USB|select USB connection|
|TOG_HID|toggle HID connection(BLE / USB)|
|m0EISU|tap EISU(英数) / press layer #2|
|m0KANA|tap KANA(かな) / press layer #2|
|m1EISU|tap EISU(英数) / press layer #3|
|m1KANA|tap KANA(かな) / press layer #3|
|m2EISU|tap EISU(英数) / press layer #4|
|m2KANA|tap KANA(かな) / press layer #4|
|m3EISU|tap EISU(英数) / press layer #5|
|m3KANA|tap KANA(かな) / press layer #5|

## m0SPCとm1ENT / ML0～ML3
廃止された。LT() / MO() マクロを使用すること。
LT()/MO()マクロを使用してもupdate_tri_layerの機能は動作する。
他のレイヤとキーコードをTAP/LAYER機能で使用することができる。

## define update_tri_layer function
update_tri_layerを実行するときのレイヤーをTAPTERM.JSNで指定することができる。
ANY(24320)とANY(24321)で設定した値のレイヤの両方を押したときに、ANY(24322)で設定された値のレイヤがオンになる。

24320は0x5F00で現在使用されていない0x5C00-0x5FFFの範囲である。

例

'''
{
    "tapping_term":{
	"ANY(24320)":2,
	"ANY(24321)":3,
	"ANY(24322)":4,
	"KC_NO":200
    }
}
'''
レイヤ2とレイヤ3がオンになるとレイヤ4がオンになる(元の設定)